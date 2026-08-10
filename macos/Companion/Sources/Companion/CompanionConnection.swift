import CryptoKit
@preconcurrency import Foundation
import Security

@MainActor
final class CompanionConnection: NSObject, @preconcurrency URLSessionDelegate, @preconcurrency URLSessionWebSocketDelegate {
    enum Mode { case authenticate, pair(String) }

    private unowned let store: CompanionStore
    private var session: URLSession?
    private var task: URLSessionWebSocketTask?
    private var mode: Mode = .authenticate
    private var receiveTask: Task<Void, Never>?
    private var sequence: UInt32 = 0

    init(store: CompanionStore) { self.store = store }

    func connect(mode: Mode) {
        disconnect()
        guard !store.panelHost.isEmpty, let url = URL(string: "wss://\(store.panelHost):8443/companion/v1") else {
            store.updateStatus("Enter the panel address first")
            return
        }
        self.mode = mode
        let configuration = URLSessionConfiguration.ephemeral
        configuration.waitsForConnectivity = true
        session = URLSession(configuration: configuration, delegate: self, delegateQueue: .main)
        task = session?.webSocketTask(with: url)
        task?.resume()
        store.updateStatus("Connecting…")
        receive()
    }

    func disconnect() {
        receiveTask?.cancel()
        receiveTask = nil
        task?.cancel(with: .goingAway, reason: nil)
        task = nil
        session?.invalidateAndCancel()
        session = nil
        store.updateStatus("Not connected")
    }

    func urlSession(_: URLSession, webSocketTask _: URLSessionWebSocketTask, didOpenWithProtocol _: String?) {
        switch mode {
        case .pair(let code): send("PAIR|\(code)")
        case .authenticate:
            guard let credential = KeychainStore.load(service: KeychainStore.service, account: store.panelHost) else {
                store.updateStatus("Start pairing on the panel")
                return
            }
            sequence &+= 1
            let nonce = UUID().uuidString
            let signed = "AUTH|\(sequence)|\(nonce)"
            let key = SymmetricKey(data: credential)
            let signature = HMAC<SHA256>.authenticationCode(for: Data(signed.utf8), using: key)
            send("\(signed)|\(signature.map { String(format: "%02x", $0) }.joined())")
        }
    }

    func urlSession(_: URLSession, webSocketTask _: URLSessionWebSocketTask, didCloseWith _: URLSessionWebSocketTask.CloseCode, reason _: Data?) {
        store.updateStatus("Panel disconnected")
    }

    func urlSession(_: URLSession, didReceive challenge: URLAuthenticationChallenge, completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void) {
        guard challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust,
              let trust = challenge.protectionSpace.serverTrust,
              let certificates = SecTrustCopyCertificateChain(trust) as? [SecCertificate],
              let certificate = certificates.first else {
            completionHandler(.performDefaultHandling, nil); return
        }
        let fingerprint = SHA256.hash(data: SecCertificateCopyData(certificate) as Data).map { String(format: "%02x", $0) }.joined()
        let saved = UserDefaults.standard.string(forKey: "certificateFingerprint")
        if let saved, saved != fingerprint {
            store.updateStatus("Blocked: panel certificate changed")
            completionHandler(.cancelAuthenticationChallenge, nil)
        } else {
            // The first trust decision is deliberately reachable only from the
            // physical pairing flow. Thereafter this exact certificate is pinned.
            if saved == nil { UserDefaults.standard.set(fingerprint, forKey: "certificateFingerprint") }
            completionHandler(.useCredential, URLCredential(trust: trust))
        }
    }

    private func receive() {
        receiveTask = Task { [weak self] in
            guard let self, let task = self.task else { return }
            do {
                let message = try await task.receive()
                if case .string(let value) = message { self.handle(value) }
                self.receive()
            } catch {
                self.store.updateStatus("Connection ended")
            }
        }
    }

    private func handle(_ message: String) {
        let parts = message.split(separator: "|", omittingEmptySubsequences: false).map(String.init)
        guard let type = parts.first else { return }
        switch type {
        case "PAIRED":
            guard parts.count == 2, let credential = Data(hex: parts[1]) else { store.updateStatus("Pairing failed"); return }
            KeychainStore.save(credential, service: KeychainStore.service, account: store.panelHost)
            store.updateStatus("Paired — reconnecting")
            connect(mode: .authenticate)
        case "AUTHENTICATED":
            store.updateStatus("Connected to \(store.panelName)", connected: true)
            publishCatalogue()
        case "CATALOGUE":
            publishCatalogue()
        case "INVOKE":
            guard parts.count == 3 else { return }
            let launched = store.launch(bundleIdentifier: parts[2])
            send("RESULT|\(parts[1])|\(launched ? "opened" : "not_allowed")")
        case "ERROR":
            store.updateStatus(parts.dropFirst().joined(separator: " "))
        default: break
        }
    }

    func publishCatalogue() {
        guard store.isConnected || task != nil else { return }
        // Bundle identifiers are stable and opaque to the browser layout editor;
        // it never receives a path or an arbitrary shell command.
        let catalogue = store.selectedApps().map { "\($0.bundleIdentifier):\($0.name.replacingOccurrences(of: ":", with: " ").replacingOccurrences(of: ",", with: " "))" }.joined(separator: ",")
        send("CATALOG|\(catalogue)")
    }

    private func send(_ value: String) { task?.send(.string(value)) { _ in } }
}

private extension Data {
    init?(hex: String) {
        guard hex.count.isMultiple(of: 2) else { return nil }
        var result = Data(); result.reserveCapacity(hex.count / 2)
        var index = hex.startIndex
        while index < hex.endIndex {
            let next = hex.index(index, offsetBy: 2)
            guard let byte = UInt8(hex[index..<next], radix: 16) else { return nil }
            result.append(byte); index = next
        }
        self = result
    }
}
