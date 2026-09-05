import CryptoKit
@preconcurrency import Foundation
import Security

private final class AuthenticationChallengeCompletion: @unchecked Sendable {
    private let handler: (URLSession.AuthChallengeDisposition, URLCredential?) -> Void

    init(_ handler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void) {
        self.handler = handler
    }

    func callAsFunction(_ disposition: URLSession.AuthChallengeDisposition, _ credential: URLCredential?) {
        handler(disposition, credential)
    }
}

private final class CompanionSessionDelegate: NSObject, URLSessionDelegate, URLSessionWebSocketDelegate, @unchecked Sendable {
    var onOpen: ((URLSessionWebSocketTask) -> Void)?
    var onClose: ((URLSessionWebSocketTask) -> Void)?
    var onChallenge: ((URLAuthenticationChallenge, AuthenticationChallengeCompletion) -> Void)?

    func urlSession(_: URLSession, webSocketTask: URLSessionWebSocketTask,
                    didOpenWithProtocol _: String?) {
        onOpen?(webSocketTask)
    }

    func urlSession(_: URLSession, webSocketTask: URLSessionWebSocketTask,
                    didCloseWith _: URLSessionWebSocketTask.CloseCode, reason _: Data?) {
        onClose?(webSocketTask)
    }

    func urlSession(_: URLSession, didReceive challenge: URLAuthenticationChallenge,
                    completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void) {
        guard let onChallenge else {
            completionHandler(.performDefaultHandling, nil)
            return
        }
        onChallenge(challenge, AuthenticationChallengeCompletion(completionHandler))
    }
}

@MainActor
final class CompanionConnection: NSObject {
    enum Mode { case authenticate, pair(code: String) }

    private unowned let store: CompanionStore
    private var session: URLSession?
    private var task: URLSessionWebSocketTask?
    private var mode: Mode = .authenticate
    private var receiveTask: Task<Void, Never>?
    private var reconnectTask: Task<Void, Never>?
    private var reconnectAttempt = 0
    private var connectionTimeoutTask: Task<Void, Never>?
    private var heartbeatTask: Task<Void, Never>?
    private var pendingCertificateFingerprint: String?
    private var shouldReconnect = false
    private var hasTerminalConnectionError = false
    private var sessionAuthenticated = false
    private var authenticationRequestOutstanding = false
    private var artworkData: Data?
    private var artworkGeneration: UInt32 = 0
    private var artworkOffset = 0
    private var lastArtworkGeneration: UInt32 = 0
    private var lastArtworkSHA256: String?
    private var lastFocusedActionIdentifier: String?
    private var catalogueGeneration: UInt32 = 0
    private var lastPublishedSystemMetrics: CompanionSystemMetricsSnapshot?
    private var lastSystemMetricsPublication = Date.distantPast
    private static let artworkChunkBytes = CompanionCapabilities.artworkChunkBytes
    private static let maximumTextFrameBytes = CompanionCapabilities.maximumTextFrameBytes
    private lazy var sessionDelegate: CompanionSessionDelegate = {
        let delegate = CompanionSessionDelegate()
        delegate.onOpen = { [weak self] task in
            Task { @MainActor [weak self] in self?.connectionDidOpen(task) }
        }
        delegate.onClose = { [weak self] task in
            Task { @MainActor [weak self] in self?.handleConnectionFailure(for: task) }
        }
        delegate.onChallenge = { [weak self] challenge, completion in
            Task { @MainActor [weak self] in
                guard let self else {
                    completion(.cancelAuthenticationChallenge, nil)
                    return
                }
                self.handleAuthenticationChallenge(challenge, completionHandler: completion)
            }
        }
        return delegate
    }()

    init(store: CompanionStore) { self.store = store }

    func connect(mode: Mode) {
        startConnection(mode: mode, resetBackoff: true)
    }

    private func startConnection(mode: Mode, resetBackoff: Bool) {
        reconnectTask?.cancel()
        reconnectTask = nil
        tearDownConnection()
        if resetBackoff { reconnectAttempt = 0 }
        hasTerminalConnectionError = false
        shouldReconnect = {
            if case .authenticate = mode { return true }
            return false
        }()
        guard !store.panelHost.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else {
            shouldReconnect = false
            store.updateStatus("Enter the panel address first")
            return
        }
        guard let url = connectionURL() else {
            shouldReconnect = false
            return
        }
        self.mode = mode
        let configuration = URLSessionConfiguration.ephemeral
        configuration.waitsForConnectivity = true
        session = URLSession(configuration: configuration, delegate: sessionDelegate, delegateQueue: nil)
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
        sessionAuthenticated = false
        authenticationRequestOutstanding = false
        resetArtworkTransferState()
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
        lastPublishedSystemMetrics = nil
        lastSystemMetricsPublication = .distantPast
    }

    private func connectionDidOpen(_ webSocketTask: URLSessionWebSocketTask) {
        guard task === webSocketTask else { return }
        switch mode {
        case .pair(let code): sendJSON(["type": "pair.request", "code": code])
        case .authenticate:
            authenticate(on: webSocketTask)
        }
    }

    private func authenticate(on webSocketTask: URLSessionWebSocketTask) {
        connectionTimeoutTask?.cancel()
        connectionTimeoutTask = nil
        let account = store.pairingAccount
        store.updateStatus("Authenticating…")
        Task { [weak self, weak webSocketTask] in
            let credential = await Task.detached(priority: .userInitiated) {
                KeychainStore.load(service: KeychainStore.service, account: account)
            }.value
            guard let self, let webSocketTask, self.task === webSocketTask else { return }
            guard let credential else {
                self.store.updateStatus("Unlock the Mac or allow Keychain access to reconnect")
                self.handleConnectionFailure(for: webSocketTask)
                return
            }
            let sequence = self.nextAuthenticationSequence()
            let nonce = UUID().uuidString
            let signed = "auth.request|\(sequence)|\(nonce)"
            let key = SymmetricKey(data: credential)
            let signature = HMAC<SHA256>.authenticationCode(for: Data(signed.utf8), using: key)
            self.startConnectionTimeout(for: webSocketTask)
            self.authenticationRequestOutstanding = true
            self.sendJSON([
                "type": "auth.request", "sequence": sequence, "nonce": nonce,
                "signature": signature.map { String(format: "%02x", $0) }.joined(),
            ])
        }
    }

    private func connectionURL() -> URL? {
        let raw = store.panelHost.trimmingCharacters(in: .whitespacesAndNewlines)
        guard let parsed = URL(string: raw.contains("://") ? raw : "wss://\(raw)"),
              let host = parsed.host,
              ConnectionEndpointPolicy.isLocalHost(host) else {
            store.updateStatus("Panel address must be on the local network")
            return nil
        }
        var components = URLComponents()
        components.scheme = "wss"
        components.host = host
        components.port = parsed.port ?? 8443
        components.path = CompanionCapabilities.protocolPath
        return components.url
    }

    private func handleAuthenticationChallenge(
        _ challenge: URLAuthenticationChallenge,
        completionHandler: AuthenticationChallengeCompletion
    ) {
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
            if case .pair = self.mode {
                self.store.updateStatus("Pairing failed — try again")
            } else {
                self.store.updateStatus("Panel did not respond — reconnecting…")
            }
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
                    Task { @MainActor [weak self, weak task] in
                        guard let self, let task else { return }
                        self.handleConnectionFailure(for: task)
                    }
                }
            }
        }
    }

    private func handleConnectionFailure(for failedTask: URLSessionWebSocketTask) {
        guard task === failedTask else { return }
        if !hasTerminalConnectionError, case .pair = mode {
            store.updateStatus("Pairing failed — try again")
        }
        sessionAuthenticated = false
        resetArtworkTransferState()
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

    private func resetArtworkTransferState() {
        artworkData = nil
        artworkGeneration = 0
        artworkOffset = 0
        lastArtworkGeneration = 0
        lastArtworkSHA256 = nil
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
            guard let self else { return }
            let delay = ReconnectBackoff.delaySeconds(
                attempt: self.reconnectAttempt,
                randomUnit: Double.random(in: 0...1)
            )
            self.reconnectAttempt += 1
            try? await Task.sleep(nanoseconds: UInt64(delay * 1_000_000_000))
            guard !Task.isCancelled, self.shouldReconnect else { return }
            self.reconnectTask = nil
            self.startConnection(mode: .authenticate, resetBackoff: false)
        }
    }

    private func handle(_ message: String) {
        guard handleJSON(message) else {
            store.updateStatus("Panel sent an unsupported protocol message")
            task?.cancel(with: .unsupportedData, reason: nil)
            return
        }
    }

    private func handleJSON(_ message: String) -> Bool {
        guard let data = message.data(using: .utf8),
              let object = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
              let type = object["type"] as? String,
              (object["protocol"] as? NSNumber)?.intValue == CompanionCapabilities.protocolVersion,
              CompanionCapabilities.protocolMessages.contains(type) else { return false }
        print("[EspControl Companion] Received \(type)")
        switch type {
        case "hello":
            break
        case "pair.accepted":
            guard let encodedCredential = object["credential"] as? String,
                  let credential = Data(hex: encodedCredential) else { store.updateStatus("Pairing failed"); return true }
            guard let fingerprint = pendingCertificateFingerprint else { store.updateStatus("Pairing failed"); return true }
            let pairingAccount = store.panelHost
            guard KeychainStore.save(credential, service: KeychainStore.service, account: pairingAccount) else {
                store.updateStatus("Pairing failed: the credential could not be saved in Keychain")
                return true
            }
            store.rememberPairingAccount(pairingAccount)
            store.setPreference(fingerprint, forKey: certificateFingerprintKey)
            store.removePreference(forKey: authenticationSequenceKey)
            pendingCertificateFingerprint = nil
            store.updateStatus("Paired — reconnecting")
            connect(mode: .authenticate)
        case "auth.accepted":
            guard case .authenticate = mode, authenticationRequestOutstanding else { return false }
            authenticationRequestOutstanding = false
            sessionAuthenticated = true
            connectionTimeoutTask?.cancel()
            connectionTimeoutTask = nil
            reconnectAttempt = 0
            store.updateStatus("Connected to \(store.panelHost)", connected: true)
            let capabilityVersion = (object["capabilityVersion"] as? NSNumber)?.intValue ?? 0
            store.setSystemMetricsSupported(capabilityVersion >= 2)
            if let task { startHeartbeat(for: task) }
            publishTimezone()
            publishCatalogue()
            store.republishCurrentNowPlaying()
            store.republishCurrentSystemMetrics()
        case "catalogue.request":
            guard sessionAuthenticated else { return false }
            publishCatalogue()
        case "action.invoke":
            guard sessionAuthenticated else { return false }
            guard let requestIdentifier = object["requestId"] as? String,
                  let kind = object["kind"] as? String else { return false }
            if kind == "action", let actionIdentifier = object["actionId"] as? String {
                Task { [weak self] in
                    guard let self else { return }
                    let status = await self.store.performResultStatus(actionIdentifier: actionIdentifier)
                    self.sendJSON(["type": "action.result", "requestId": requestIdentifier, "status": status])
                }
            } else if kind == "url", let appIdentifier = object["appId"] as? String,
                      let encodedURL = object["encodedUrl"] as? String {
                let opened = store.openURL(encodedURL: encodedURL, bundleIdentifier: appIdentifier)
                sendJSON(["type": "action.result", "requestId": requestIdentifier,
                          "status": opened ? "opened" : "not_allowed"])
            } else { return false }
        case "value.set":
            guard sessionAuthenticated else { return false }
            guard let requestIdentifier = object["requestId"] as? String,
                  let controlIdentifier = object["controlId"] as? String,
                  let value = (object["value"] as? NSNumber)?.intValue,
                  (0...100).contains(value) else { return false }
            let changed = store.setMediaControlValue(value, controlIdentifier: controlIdentifier)
            sendJSON(["type": "action.result", "requestId": requestIdentifier,
                      "status": changed ? "performed" : "not_allowed"])
        case "error":
            let code = object["code"] as? String ?? "unknown_error"
            if code == "authentication_sequence",
               let panelSequence = (object["lastSequence"] as? NSNumber)?.uint32Value,
               panelSequence < UInt32.max {
                store.setPreference(Int(panelSequence), forKey: authenticationSequenceKey)
                store.updateStatus("Authentication counter repaired — reconnecting")
                connect(mode: .authenticate)
            } else {
                store.updateStatus(code.replacingOccurrences(of: "_", with: " "),
                                   connected: sessionAuthenticated)
            }
        case "artwork.ack":
            guard sessionAuthenticated else { return false }
            if let generation = (object["generation"] as? NSNumber)?.uint32Value,
               let nextOffset = (object["nextOffset"] as? NSNumber)?.intValue,
               generation == artworkGeneration, nextOffset == artworkOffset {
                sendNextArtworkChunk()
            }
        case "artwork.abort":
            guard sessionAuthenticated else { return false }
            resetArtworkTransferState()
        case "artwork.request":
            guard sessionAuthenticated else { return false }
            if let generation = (object["generation"] as? NSNumber)?.uint32Value {
                store.republishNowPlayingArtwork(generation: generation)
            }
        default:
            return false
        }
        return true
    }

    func publishNowPlaying(_ snapshot: CompanionNowPlayingSnapshot, forceArtwork: Bool = false) {
        let artworkHash = snapshot.artworkSHA256
        let hasArtwork = snapshot.artworkJPEG != nil
        let shouldSendArtwork = hasArtwork && (forceArtwork ||
            snapshot.generation != lastArtworkGeneration || artworkHash != lastArtworkSHA256)
        if artworkData != nil && (shouldSendArtwork || snapshot.generation != artworkGeneration) {
            sendJSON(["type": "artwork.abort", "generation": artworkGeneration])
            artworkData = nil
            artworkOffset = 0
        }
        var message: [String: Any] = [
            "type": "now_playing", "generation": snapshot.generation,
            "applicationIdentifier": snapshot.applicationIdentifier,
            "applicationName": snapshot.applicationName, "state": snapshot.state.rawValue,
            "contentIdentifier": snapshot.contentIdentifier, "title": snapshot.title,
            "artist": snapshot.artist, "album": snapshot.album,
            "durationMs": snapshot.durationMilliseconds, "positionMs": snapshot.positionMilliseconds,
            "playbackRate": snapshot.playbackRate, "hasArtwork": hasArtwork,
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
        sendJSON(["type": "artwork.begin", "generation": snapshot.generation,
                  "byteLength": artwork.count, "sha256": artworkHash ?? "",
                  "mimeType": "image/jpeg"])
    }

    func publishSystemMetrics(_ snapshot: CompanionSystemMetricsSnapshot, force: Bool = false) {
        let now = Date()
        let elapsed = now.timeIntervalSince(lastSystemMetricsPublication)
        guard force || Self.shouldPublishSystemMetrics(
            previous: lastPublishedSystemMetrics,
            current: snapshot,
            elapsedSeconds: elapsed
        ) else { return }
        lastPublishedSystemMetrics = snapshot
        lastSystemMetricsPublication = now
        var message: [String: Any] = [
            "type": "system_metrics", "generation": snapshot.generation,
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
        sendJSON(["type": "system_metrics", "generation": 1, "available": false])
    }

    private func sendNextArtworkChunk() {
        guard let artworkData else { return }
        if artworkOffset >= artworkData.count {
            sendJSON(["type": "artwork.end", "generation": artworkGeneration])
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
                    self?.resetArtworkTransferState()
                }
            }
        }
    }

    private func sendJSON(_ object: [String: Any]) {
        var envelope = object
        envelope["protocol"] = CompanionCapabilities.protocolVersion
        guard JSONSerialization.isValidJSONObject(envelope),
              let data = try? JSONSerialization.data(withJSONObject: envelope), data.count <= Self.maximumTextFrameBytes,
              let value = String(data: data, encoding: .utf8) else { return }
        send(value)
    }

    func publishCatalogue() {
        guard store.isConnected || task != nil else { return }
        lastFocusedActionIdentifier = nil
#if APP_STORE
        let supportedWindowActions: [String] = []
#else
        let supportedWindowActions = Self.supportedWindowActionIDs(
            for: ProcessInfo.processInfo.operatingSystemVersion
        )
#endif
        var capabilities = (store.mediaActionsAvailable ? ["media_actions"] : []) + supportedWindowActions
#if !APP_STORE
        capabilities.append("keyboard_shortcuts")
#else
        capabilities.append("keyboard_shortcuts_unavailable")
#endif
        sendJSON(["type": "capabilities", "values": capabilities])
        // Bundle identifiers are stable and opaque to the browser layout editor;
        // it never receives a path or an arbitrary shell command.
        // Approved folders are sent first so they remain available even when
        // the installed application catalogue reaches the frame limit.
        let entries: [[String: String]] = store.folderActions().compactMap { folder -> [String: String]? in
            guard Self.validCatalogueIdentifier(folder.actionIdentifier) else { return nil }
            return ["id": folder.actionIdentifier, "label": Self.catalogueLabel(folder.name, fallback: "Folder")]
        } + store.launchableApps().compactMap { app -> [String: String]? in
            guard Self.validCatalogueIdentifier(app.bundleIdentifier) else { return nil }
            return ["id": app.bundleIdentifier, "label": Self.catalogueLabel(app.name, fallback: app.bundleIdentifier)]
        }
        catalogueGeneration &+= 1
        if catalogueGeneration == 0 { catalogueGeneration = 1 }
        let pages = stride(from: 0, to: max(entries.count, 1), by: 48).map {
            Array(entries[$0..<min($0 + 48, entries.count)])
        }
        for (page, items) in pages.enumerated() {
            sendJSON(["type": "catalogue.page", "generation": catalogueGeneration,
                      "page": page, "complete": page == pages.count - 1, "items": items])
        }
        publishFocusedAction()
    }

    static func supportedWindowActionIDs(for version: OperatingSystemVersion) -> [String] {
        CompanionCapabilities.windowActions.compactMap { identifier, capability in
            capability.minimumMacOS <= version.majorVersion ? identifier : nil
        }.sorted()
    }

    func publishTimezone() {
        let identifier = TimeZone.current.identifier
        guard !identifier.isEmpty, identifier.utf8.count <= 96 else { return }
        sendJSON(["type": "timezone.changed", "identifier": identifier])
    }

    func publishFocusedAction() {
        let identifier = store.focusedCompanionActionIdentifier()
        guard identifier.isEmpty || Self.validCatalogueIdentifier(identifier) else { return }
        guard identifier != lastFocusedActionIdentifier else { return }
        lastFocusedActionIdentifier = identifier
        sendJSON(["type": "focus.changed", "actionId": identifier])
    }

    func publishMediaControlValues(_ values: [String: Int], unavailable: Set<String>) {
        for identifier in unavailable.sorted() where Self.validMediaControlIdentifier(identifier) {
            sendJSON(["type": "value.state", "controlId": identifier, "available": false])
        }
        for (identifier, value) in values.sorted(by: { $0.key < $1.key }) {
            guard Self.validMediaControlIdentifier(identifier), (0...100).contains(value) else { continue }
            sendJSON(["type": "value.state", "controlId": identifier, "available": true, "value": value])
        }
    }

    private func send(_ value: String) {
        guard let activeTask = task else { return }
        activeTask.send(.string(value)) { [weak self, weak activeTask] error in
            guard error != nil else { return }
            Task { @MainActor [weak self, weak activeTask] in
                guard let self, let activeTask, self.task === activeTask else { return }
                self.handleConnectionFailure(for: activeTask)
            }
        }
    }

    nonisolated static func shouldPublishSystemMetrics(
        previous: CompanionSystemMetricsSnapshot?,
        current: CompanionSystemMetricsSnapshot,
        elapsedSeconds: TimeInterval
    ) -> Bool {
        guard let previous else { return true }
        if elapsedSeconds >= 30 { return true }
        if abs(current.cpuUsagePercent - previous.cpuUsagePercent) >= 1 { return true }
        if abs(current.memoryUsagePercent - previous.memoryUsagePercent) >= 0.5 { return true }
        if abs(current.storageUsagePercent - previous.storageUsagePercent) >= 0.1 { return true }
        if optionalDifference(current.batteryPercent, previous.batteryPercent) >= 1 { return true }
        if optionalDifference(current.networkThroughputKBps, previous.networkThroughputKBps) >= 32 { return true }
        return false
    }

    nonisolated private static func optionalDifference(_ lhs: Double?, _ rhs: Double?) -> Double {
        guard let lhs, let rhs else { return lhs == nil && rhs == nil ? 0 : .infinity }
        return abs(lhs - rhs)
    }

    private var certificateFingerprintKey: String { "companion.certificateFingerprint.\(store.pairingAccount)" }
    private var authenticationSequenceKey: String { "companion.authenticationSequence.\(store.pairingAccount)" }

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
        var sanitized = ""
        for scalar in value.unicodeScalars {
            guard scalar.value >= 0x20 && scalar.value <= 0x10ffff,
                  scalar.value != 0x7c && scalar.value != 0x3a && scalar.value != 0x2c else { continue }
            let candidate = sanitized + String(scalar)
            guard candidate.utf8.count <= 96 else { break }
            sanitized = candidate
        }
        sanitized = sanitized.trimmingCharacters(in: .whitespacesAndNewlines)
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
