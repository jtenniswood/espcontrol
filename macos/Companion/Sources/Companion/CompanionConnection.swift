import CryptoKit
@preconcurrency import Foundation
import Security

@MainActor
final class CompanionConnection: NSObject, @preconcurrency URLSessionDelegate, @preconcurrency URLSessionWebSocketDelegate {
    enum Mode { case authenticate, pair(code: String) }

    private unowned let store: CompanionStore
    private var session: URLSession?
    private var task: URLSessionWebSocketTask?
    private var mode: Mode = .authenticate
    private var receiveTask: Task<Void, Never>?
    private var reconnectTask: Task<Void, Never>?
    private var connectionTimeoutTask: Task<Void, Never>?
    private var heartbeatTask: Task<Void, Never>?
    private var pendingCertificateFingerprint: String?
    private var shouldReconnect = false
    private var hasTerminalConnectionError = false
    private var artworkData: Data?
    private var artworkGeneration: UInt32 = 0
    private var artworkOffset = 0
    private var lastArtworkGeneration: UInt32 = 0
    private var lastArtworkSHA256: String?
    private var lastFocusedActionIdentifier: String?
    private static let artworkChunkBytes = 12 * 1024
    private static let maximumTextFrameBytes = 16 * 1024

    init(store: CompanionStore) { self.store = store }

    func connect(mode: Mode) {
        reconnectTask?.cancel()
        reconnectTask = nil
        tearDownConnection()
        hasTerminalConnectionError = false
        shouldReconnect = {
            if case .authenticate = mode { return true }
            return false
        }()
        guard !store.panelHost.isEmpty, let url = connectionURL() else {
            shouldReconnect = false
            store.updateStatus("Enter the panel address first")
            return
        }
        self.mode = mode
        let configuration = URLSessionConfiguration.ephemeral
        configuration.waitsForConnectivity = true
        session = URLSession(configuration: configuration, delegate: self, delegateQueue: .main)
        guard let task = session?.webSocketTask(with: url) else {
            store.updateStatus("Could not create the panel connection")
            return
        }
        self.task = task
        task.resume()
        store.updateStatus("Connecting…")
        receive(from: task)
        startConnectionTimeout(for: task)
    }

    func disconnect() {
        shouldReconnect = false
        reconnectTask?.cancel()
        reconnectTask = nil
        tearDownConnection()
        store.updateStatus("Not connected")
    }

    private func tearDownConnection() {
        connectionTimeoutTask?.cancel()
        connectionTimeoutTask = nil
        heartbeatTask?.cancel()
        heartbeatTask = nil
        receiveTask?.cancel()
        receiveTask = nil
        task?.cancel(with: .goingAway, reason: nil)
        task = nil
        session?.invalidateAndCancel()
        session = nil
    }

    func urlSession(_: URLSession, webSocketTask: URLSessionWebSocketTask, didOpenWithProtocol _: String?) {
        guard task === webSocketTask else { return }
        switch mode {
        case .pair(let code): send("PAIR|\(code)")
        case .authenticate:
            authenticate(on: webSocketTask)
        }
    }

    private func authenticate(on webSocketTask: URLSessionWebSocketTask) {
        connectionTimeoutTask?.cancel()
        connectionTimeoutTask = nil
        let host = store.panelHost
        store.updateStatus("Authenticating…")
        Task { [weak self, weak webSocketTask] in
            let credential = await Task.detached(priority: .userInitiated) {
                KeychainStore.load(service: KeychainStore.service, account: host)
            }.value
            guard let self, let webSocketTask, self.task === webSocketTask else { return }
            guard let credential else {
                self.store.updateStatus("Unlock the Mac or allow Keychain access to reconnect")
                self.handleConnectionFailure(for: webSocketTask)
                return
            }
            let sequence = self.nextAuthenticationSequence()
            let nonce = UUID().uuidString
            let signed = "AUTH|\(sequence)|\(nonce)"
            let key = SymmetricKey(data: credential)
            let signature = HMAC<SHA256>.authenticationCode(for: Data(signed.utf8), using: key)
            self.startConnectionTimeout(for: webSocketTask)
            self.send("\(signed)|\(signature.map { String(format: "%02x", $0) }.joined())")
        }
    }

    private func connectionURL() -> URL? {
        let raw = store.panelHost.trimmingCharacters(in: .whitespacesAndNewlines)
        guard let parsed = URL(string: raw.contains("://") ? raw : "wss://\(raw)"),
              let host = parsed.host else { return nil }
        var components = URLComponents()
        components.scheme = "wss"
        components.host = host
        components.port = parsed.port ?? 8443
        components.path = "/companion/v1"
        return components.url
    }

    func urlSession(_: URLSession, webSocketTask: URLSessionWebSocketTask, didCloseWith _: URLSessionWebSocketTask.CloseCode, reason _: Data?) {
        handleConnectionFailure(for: webSocketTask)
    }

    func urlSession(_: URLSession, didReceive challenge: URLAuthenticationChallenge, completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void) {
        guard challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust,
              let trust = challenge.protectionSpace.serverTrust,
              let certificates = SecTrustCopyCertificateChain(trust) as? [SecCertificate],
              let certificate = certificates.first else {
            completionHandler(.performDefaultHandling, nil); return
        }
        let fingerprint = SHA256.hash(data: SecCertificateCopyData(certificate) as Data).map { String(format: "%02x", $0) }.joined()
        let saved = store.stringPreference(forKey: certificateFingerprintKey)
        if let saved, saved != fingerprint {
            shouldReconnect = false
            hasTerminalConnectionError = true
            store.updateStatus("Blocked: panel certificate changed")
            completionHandler(.cancelAuthenticationChallenge, nil)
        } else if saved != nil {
            if case .pair = mode { pendingCertificateFingerprint = fingerprint }
            completionHandler(.useCredential, URLCredential(trust: trust))
        } else if case .pair = mode {
            // The one-time pairing code authorizes this first connection. Pin
            // the certificate when the panel returns the paired credential.
            pendingCertificateFingerprint = fingerprint
            completionHandler(.useCredential, URLCredential(trust: trust))
        } else {
            store.updateStatus("Start pairing in the panel web settings")
            completionHandler(.cancelAuthenticationChallenge, nil)
        }
    }

    private func receive(from task: URLSessionWebSocketTask) {
        receiveTask = Task { [weak self] in
            guard let self else { return }
            while !Task.isCancelled && self.task === task {
                do {
                    let message = try await task.receive()
                    guard !Task.isCancelled, self.task === task else { return }
                    if case .string(let value) = message { self.handle(value) }
                } catch {
                    guard !Task.isCancelled else { return }
                    self.handleConnectionFailure(for: task)
                    return
                }
            }
        }
    }

    private func startConnectionTimeout(for task: URLSessionWebSocketTask) {
        connectionTimeoutTask?.cancel()
        connectionTimeoutTask = Task { [weak self] in
            try? await Task.sleep(for: .seconds(10))
            guard !Task.isCancelled, let self, self.task === task, !self.store.isConnected else { return }
            self.store.updateStatus("Panel did not respond — reconnecting…")
            self.handleConnectionFailure(for: task)
        }
    }

    private func startHeartbeat(for task: URLSessionWebSocketTask) {
        heartbeatTask?.cancel()
        heartbeatTask = Task { [weak self] in
            while !Task.isCancelled {
                try? await Task.sleep(for: .seconds(15))
                guard !Task.isCancelled, let self, self.task === task else { return }
                task.sendPing { [weak self, weak task] error in
                    guard error != nil else { return }
                    Task { @MainActor in
                        guard let self, let task else { return }
                        self.handleConnectionFailure(for: task)
                    }
                }
            }
        }
    }

    private func handleConnectionFailure(for failedTask: URLSessionWebSocketTask) {
        guard task === failedTask else { return }
        connectionTimeoutTask?.cancel()
        connectionTimeoutTask = nil
        heartbeatTask?.cancel()
        heartbeatTask = nil
        receiveTask?.cancel()
        receiveTask = nil
        task = nil
        failedTask.cancel(with: .goingAway, reason: nil)
        session?.invalidateAndCancel()
        session = nil
        scheduleReconnect()
    }

    private func scheduleReconnect() {
        guard !hasTerminalConnectionError else { return }
        // Pairing failures deliberately close their unauthenticated socket.
        // Keep the specific server error visible instead of replacing it with
        // a generic disconnect message during that expected teardown.
        guard shouldReconnect else { return }
        guard store.hasSavedPairing else {
            store.updateStatus("Panel disconnected")
            return
        }
        guard reconnectTask == nil else { return }
        store.updateStatus("Panel unavailable — reconnecting…")
        reconnectTask = Task { [weak self] in
            try? await Task.sleep(for: .seconds(3))
            guard !Task.isCancelled, let self, self.shouldReconnect else { return }
            self.reconnectTask = nil
            self.connect(mode: .authenticate)
        }
    }

    private func handle(_ message: String) {
        if message.first == "{", handleJSON(message) { return }
        let parts = message.split(separator: "|", omittingEmptySubsequences: false).map(String.init)
        guard let type = parts.first else { return }
        print("[EspControl Companion] Received \(type)")
        switch type {
        case "PAIRED":
            guard parts.count == 2, let credential = Data(hex: parts[1]) else { store.updateStatus("Pairing failed"); return }
            guard let fingerprint = pendingCertificateFingerprint else { store.updateStatus("Pairing failed"); return }
            guard KeychainStore.save(credential, service: KeychainStore.service, account: store.panelHost) else {
                store.updateStatus("Pairing failed: the credential could not be saved in Keychain")
                return
            }
            store.setPreference(fingerprint, forKey: certificateFingerprintKey)
            store.removePreference(forKey: authenticationSequenceKey)
            pendingCertificateFingerprint = nil
            store.updateStatus("Paired — reconnecting")
            connect(mode: .authenticate)
        case "AUTHENTICATED":
            connectionTimeoutTask?.cancel()
            connectionTimeoutTask = nil
            store.updateStatus("Connected to \(store.panelHost)", connected: true)
            // Version 1 explicitly predates system metrics. A short legacy
            // authentication response has no capability declaration, so
            // retain the original connected-panel behaviour for it.
            let supportsMetrics = parts.count < 2 || (UInt32(parts[1]) ?? 0) >= 2
            store.setSystemMetricsSupported(supportsMetrics)
            if let task { startHeartbeat(for: task) }
            publishCatalogue()
            store.republishCurrentNowPlaying()
            store.republishCurrentSystemMetrics()
        case "CATALOGUE":
            publishCatalogue()
        case "INVOKE":
            guard parts.count == 3 else { return }
            let performed = store.perform(actionIdentifier: parts[2])
            send("RESULT|\(parts[1])|\(performed ? "performed" : "not_allowed")")
        case "OPEN_URL":
            guard parts.count == 4 else { return }
            let opened = store.openURL(encodedURL: parts[3], bundleIdentifier: parts[2])
            send("RESULT|\(parts[1])|\(opened ? "opened" : "not_allowed")")
        case "SET_VALUE":
            guard parts.count == 4, let value = Int(parts[3]), (0...100).contains(value) else { return }
            let changed = store.setMediaControlValue(value, controlIdentifier: parts[2])
            send("RESULT|\(parts[1])|\(changed ? "performed" : "not_allowed")")
        case "ERROR":
            if parts.count == 3, parts[1] == "authentication_sequence",
               let panelSequence = UInt32(parts[2]), panelSequence < UInt32.max {
                UserDefaults.standard.set(Int(panelSequence), forKey: authenticationSequenceKey)
                store.updateStatus("Authentication counter repaired — reconnecting")
                connect(mode: .authenticate)
            } else {
                store.updateStatus(parts.dropFirst().joined(separator: " "))
            }
        default: break
        }
    }

    private func handleJSON(_ message: String) -> Bool {
        guard let data = message.data(using: .utf8),
              let object = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
              let type = object["type"] as? String else { return false }
        if type == "artwork.ack",
           let generation = (object["generation"] as? NSNumber)?.uint32Value,
           let nextOffset = (object["nextOffset"] as? NSNumber)?.intValue,
           generation == artworkGeneration, nextOffset == artworkOffset {
            sendNextArtworkChunk()
        } else if type == "artwork.abort" {
            artworkData = nil
            artworkOffset = 0
            lastArtworkGeneration = 0
            lastArtworkSHA256 = nil
        } else if type == "artwork.request",
                  let generation = (object["generation"] as? NSNumber)?.uint32Value {
            store.republishNowPlayingArtwork(generation: generation)
        }
        return true
    }

    func publishNowPlaying(_ snapshot: CompanionNowPlayingSnapshot, forceArtwork: Bool = false) {
        let artworkHash = snapshot.artworkSHA256
        let shouldSendArtwork = snapshot.artworkJPEG != nil && (forceArtwork ||
            snapshot.generation != lastArtworkGeneration || artworkHash != lastArtworkSHA256)
        if artworkData != nil && (shouldSendArtwork || snapshot.generation != artworkGeneration) {
            sendJSON(["type": "artwork.abort", "version": 2, "generation": artworkGeneration])
            artworkData = nil
            artworkOffset = 0
        }
        var message: [String: Any] = [
            "type": "now_playing", "version": 2, "generation": snapshot.generation,
            "applicationIdentifier": snapshot.applicationIdentifier,
            "applicationName": snapshot.applicationName, "state": snapshot.state.rawValue,
            "contentIdentifier": snapshot.contentIdentifier, "title": snapshot.title,
            "artist": snapshot.artist, "album": snapshot.album,
            "durationMs": snapshot.durationMilliseconds, "positionMs": snapshot.positionMilliseconds,
            "playbackRate": snapshot.playbackRate, "hasArtwork": shouldSendArtwork,
        ]
        if shouldSendArtwork, let artworkHash { message["artworkSHA256"] = artworkHash }
        sendJSON(message)
        guard shouldSendArtwork, let artwork = snapshot.artworkJPEG else { return }
        artworkData = nil
        artworkOffset = 0
        artworkGeneration = snapshot.generation
        artworkData = artwork
        lastArtworkGeneration = snapshot.generation
        lastArtworkSHA256 = artworkHash
        sendJSON(["type": "artwork.begin", "version": 2, "generation": snapshot.generation,
                  "byteLength": artwork.count, "sha256": artworkHash ?? "",
                  "mimeType": "image/jpeg"])
    }

    func publishSystemMetrics(_ snapshot: CompanionSystemMetricsSnapshot) {
        var message: [String: Any] = [
            "type": "system_metrics", "version": 2, "generation": snapshot.generation,
            "cpuUsagePercent": snapshot.cpuUsagePercent,
            "memoryUsagePercent": snapshot.memoryUsagePercent,
            "storageUsagePercent": snapshot.storageUsagePercent,
        ]
        if let battery = snapshot.batteryPercent { message["batteryPercent"] = battery }
        if let throughput = snapshot.networkThroughputKBps {
            message["networkThroughputKBps"] = throughput
        }
        sendJSON(message)
    }

    func publishSystemMetricsUnavailable() {
        sendJSON(["type": "system_metrics", "version": 2, "generation": 1, "available": false])
    }

    private func sendNextArtworkChunk() {
        guard let artworkData else { return }
        if artworkOffset >= artworkData.count {
            sendJSON(["type": "artwork.end", "version": 2, "generation": artworkGeneration])
            self.artworkData = nil
            artworkOffset = 0
            return
        }
        let end = min(artworkOffset + Self.artworkChunkBytes, artworkData.count)
        var frame = Data()
        var generation = artworkGeneration.bigEndian
        var offset = UInt32(artworkOffset).bigEndian
        withUnsafeBytes(of: &generation) { frame.append(contentsOf: $0) }
        withUnsafeBytes(of: &offset) { frame.append(contentsOf: $0) }
        frame.append(artworkData[artworkOffset..<end])
        artworkOffset = end
        task?.send(.data(frame)) { [weak self] error in
            if error != nil {
                Task { @MainActor in
                    self?.artworkData = nil
                    self?.lastArtworkGeneration = 0
                    self?.lastArtworkSHA256 = nil
                }
            }
        }
    }

    private func sendJSON(_ object: [String: Any]) {
        guard JSONSerialization.isValidJSONObject(object),
              let data = try? JSONSerialization.data(withJSONObject: object), data.count <= Self.maximumTextFrameBytes,
              let value = String(data: data, encoding: .utf8) else { return }
        send(value)
    }

    func publishCatalogue() {
        guard store.isConnected || task != nil else { return }
        lastFocusedActionIdentifier = nil
        if SystemMediaController.mediaActionsAvailable {
            send("CAPABILITIES|media_actions")
        }
        // Bundle identifiers are stable and opaque to the browser layout editor;
        // it never receives a path or an arbitrary shell command.
        let entries = store.launchableApps().compactMap { app -> String? in
            guard Self.validCatalogueIdentifier(app.bundleIdentifier) else { return nil }
            return "\(app.bundleIdentifier):\(Self.catalogueLabel(app.name, fallback: app.bundleIdentifier))"
        } + store.folderActions().compactMap { folder -> String? in
            guard Self.validCatalogueIdentifier(folder.actionIdentifier) else { return nil }
            return "\(folder.actionIdentifier):\(Self.catalogueLabel(folder.name, fallback: "Folder"))"
        }
        var catalogue = "CATALOG|"
        for entry in entries {
            let separator = catalogue.count == "CATALOG|".count ? "" : ","
            guard catalogue.utf8.count + separator.utf8.count + entry.utf8.count <= Self.maximumTextFrameBytes else { break }
            catalogue += separator + entry
        }
        send(catalogue)
        publishFocusedAction()
    }

    func publishFocusedAction() {
        let identifier = store.focusedCompanionActionIdentifier()
        guard identifier.isEmpty || Self.validCatalogueIdentifier(identifier) else { return }
        guard identifier != lastFocusedActionIdentifier else { return }
        lastFocusedActionIdentifier = identifier
        send("FOCUS|\(identifier)")
    }

    func publishMediaControlValues(_ values: [String: Int], unavailable: Set<String>) {
        for identifier in unavailable.sorted() where Self.validMediaControlIdentifier(identifier) {
            send("STATE|\(identifier)|unavailable")
        }
        for (identifier, value) in values.sorted(by: { $0.key < $1.key }) {
            guard Self.validMediaControlIdentifier(identifier), (0...100).contains(value) else { continue }
            send("STATE|\(identifier)|\(value)")
        }
    }

    private func send(_ value: String) { task?.send(.string(value)) { _ in } }

    private var certificateFingerprintKey: String { "companion.certificateFingerprint.\(store.panelHost)" }
    private var authenticationSequenceKey: String { "companion.authenticationSequence.\(store.panelHost)" }

    private func nextAuthenticationSequence() -> UInt32 {
        let previous = UInt32(clamping: store.integerPreference(forKey: authenticationSequenceKey))
        let next = previous &+ 1
        store.setPreference(Int(next), forKey: authenticationSequenceKey)
        return next
    }

    private static func validCatalogueIdentifier(_ value: String) -> Bool {
        !value.isEmpty && value.utf8.count <= 96 && value.utf8.allSatisfy {
            $0 >= 0x20 && $0 <= 0x7e && $0 != 0x7c && $0 != 0x3a && $0 != 0x2c
        }
    }

    private static func validMediaControlIdentifier(_ value: String) -> Bool {
        value == SystemMediaController.outputVolumeID || value == SystemMediaController.inputVolumeID
    }

    private static func catalogueLabel(_ value: String, fallback: String) -> String {
        let bytes = value.utf8.filter {
            $0 >= 0x20 && $0 <= 0x7e && $0 != 0x7c && $0 != 0x3a && $0 != 0x2c
        }
        let sanitized = String(decoding: bytes.prefix(96), as: UTF8.self)
            .trimmingCharacters(in: .whitespacesAndNewlines)
        return sanitized.isEmpty ? fallback : sanitized
    }
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
