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

    private unowned let preferences: any CompanionSessionPreferences
    private unowned let resources: any CompanionSessionResources
    private let credentials: any CompanionSessionCredentials
    var onEvent: ((CompanionSessionEvent) -> Void)?
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
    private var connectionGeneration: UInt64 = 0
    private func makeSessionDelegate() -> CompanionSessionDelegate {
        let generation = connectionGeneration
        let delegate = CompanionSessionDelegate()
        delegate.onOpen = { [weak self] task in
            Task { @MainActor [weak self] in self?.connectionDidOpen(task) }
        }
        delegate.onClose = { [weak self] task in
            Task { @MainActor [weak self] in self?.handleConnectionFailure(for: task) }
        }
        delegate.onChallenge = { [weak self] challenge, completion in
            Task { @MainActor [weak self] in
                guard let self, self.connectionGeneration == generation else {
                    completion(.cancelAuthenticationChallenge, nil)
                    return
                }
                self.handleAuthenticationChallenge(challenge, completionHandler: completion)
            }
        }
        return delegate
    }

    init(preferences: any CompanionSessionPreferences,
         resources: any CompanionSessionResources,
         credentials: any CompanionSessionCredentials = CompanionKeychainCredentials()) {
        self.preferences = preferences
        self.resources = resources
        self.credentials = credentials
    }

    private func updateConnectionStatus(_ message: String, state: CompanionConnectionState, recovery: String? = nil) {
        onEvent?(.connection(message: message, state: state, recovery: recovery))
    }

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
        guard !preferences.panelHost.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else {
            shouldReconnect = false
            updateConnectionStatus("Enter the display address first", state: .failed)
            return
        }
        guard let url = connectionURL() else {
            shouldReconnect = false
            return
        }
        self.mode = mode
        let configuration = URLSessionConfiguration.ephemeral
        configuration.waitsForConnectivity = true
        session = URLSession(configuration: configuration, delegate: makeSessionDelegate(), delegateQueue: nil)
        guard let task = session?.webSocketTask(with: url) else {
            updateConnectionStatus("Could not create the display connection", state: .failed)
            return
        }
        self.task = task
        task.resume()
        updateConnectionStatus("Connecting…", state: resetBackoff ? .connecting : .reconnecting)
        receive(from: task)
        startConnectionTimeout(for: task)
    }

    func disconnect() {
        shouldReconnect = false
        reconnectTask?.cancel()
        reconnectTask = nil
        tearDownConnection()
        updateConnectionStatus("Not connected", state: .disconnected)
    }

    private func tearDownConnection() {
        connectionGeneration &+= 1
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
        let account = preferences.pairingAccount
        updateConnectionStatus("Authenticating…", state: reconnectAttempt > 0 ? .reconnecting : .connecting)
        Task { [weak self, weak webSocketTask] in
            // Stay on the main actor so Security can present an interactive
            // Keychain access prompt when the saved credential requires it.
            let credential = self?.credentials.load(account: account)
            guard let self, let webSocketTask, self.task === webSocketTask else { return }
            guard let credential else {
                self.hasTerminalConnectionError = true
                self.shouldReconnect = false
                self.updateConnectionStatus("Unlock the Mac or allow Keychain access to reconnect", state: .failed, recovery: "Unlock your Mac and allow Companion to access Keychain, then try again.")
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
        let raw = preferences.panelHost.trimmingCharacters(in: .whitespacesAndNewlines)
        guard let parsed = URL(string: raw.contains("://") ? raw : "wss://\(raw)"),
              let host = parsed.host,
              ConnectionEndpointPolicy.isLocalHost(host) else {
            updateConnectionStatus("Display address must be on the local network", state: .failed)
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
        let saved = preferences.stringPreference(forKey: certificateFingerprintKey)
        if let saved, saved != fingerprint {
            shouldReconnect = false
            hasTerminalConnectionError = true
            updateConnectionStatus("Blocked: display certificate changed", state: .failed, recovery: "The display’s identity has changed. If you reset or replaced it, forget this display and pair again using the code from its webpage.")
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
            updateConnectionStatus("Open the device webpage to start pairing", state: .failed)
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
            guard !Task.isCancelled, let self, self.task === task, !self.sessionAuthenticated else { return }
            if case .pair = self.mode {
                self.updateConnectionStatus("Pairing failed — try again", state: .failed)
            } else {
                self.updateConnectionStatus("Display did not respond — reconnecting…", state: .reconnecting)
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
            updateConnectionStatus("Pairing failed — try again", state: .failed)
        }
        connectionGeneration &+= 1
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
        guard preferences.hasSavedPairing else {
            updateConnectionStatus("Display disconnected", state: .disconnected)
            return
        }
        guard reconnectTask == nil else { return }
        updateConnectionStatus("Display unavailable — reconnecting…", state: .reconnecting)
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
            hasTerminalConnectionError = true
            shouldReconnect = false
            tearDownConnection()
            updateConnectionStatus("Display sent an unsupported protocol message", state: .failed, recovery: "Update Companion and your display to matching versions, then reconnect.")
            return
        }
    }

    private func handleJSON(_ message: String) -> Bool {
        let state: CompanionProtocolState = sessionAuthenticated ? .connected : {
            if case .pair = mode { return .pairing }
            return .authenticating
        }()
        guard let data = message.data(using: .utf8),
              let decoded = CompanionProtocolDecoder.decode(data, direction: .panelToMac, state: state),
              let object = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
              let type = object["type"] as? String else { return false }
        print("[EspControl Companion] Received \(type)")
        switch decoded {
        case .hello:
            break
        case .pairAccepted(let payload):
            guard let credential = Data(hex: payload.credential) else { updateConnectionStatus("Pairing failed", state: .failed); return true }
            guard let fingerprint = pendingCertificateFingerprint else { updateConnectionStatus("Pairing failed", state: .failed); return true }
            let pairingAccount = preferences.panelHost
            guard credentials.save(credential, account: pairingAccount) else {
                updateConnectionStatus("Pairing failed: the credential could not be saved in Keychain", state: .failed, recovery: "Unlock your Mac and allow Companion to access Keychain, then try again.")
                return true
            }
            preferences.rememberPairingAccount(pairingAccount)
            preferences.setPreference(fingerprint, forKey: certificateFingerprintKey)
            preferences.removePreference(forKey: authenticationSequenceKey)
            pendingCertificateFingerprint = nil
            updateConnectionStatus("Paired — reconnecting", state: .connecting)
            connect(mode: .authenticate)
        case .authAccepted(let payload):
            guard case .authenticate = mode, authenticationRequestOutstanding else { return false }
            authenticationRequestOutstanding = false
            sessionAuthenticated = true
            connectionTimeoutTask?.cancel()
            connectionTimeoutTask = nil
            reconnectAttempt = 0
            updateConnectionStatus("Connected to \(preferences.panelHost)", state: .connected)
            let capabilityVersion = payload.capabilityVersion
            onEvent?(.capabilities(systemMetrics: capabilityVersion >= 2))
            if let task { startHeartbeat(for: task) }
            publishTimezone()
            publishCatalogue()
            onEvent?(.publishCurrentState)
        case .catalogueRequest:
            guard sessionAuthenticated else { return false }
            publishCatalogue()
        case .actionInvoke(let payload):
            guard sessionAuthenticated else { return false }
            let requestIdentifier = payload.requestId
            let kind = payload.kind
            if kind == "action", let actionIdentifier = payload.actionId {
                let requestingTask = task
                Task { [weak self, weak requestingTask] in
                    guard let self, let requestingTask, self.task === requestingTask, self.sessionAuthenticated else { return }
                    let status = await self.resources.performResultStatus(actionIdentifier: actionIdentifier)
                    guard self.task === requestingTask, self.sessionAuthenticated else { return }
                    self.sendJSON(["type": "action.result", "requestId": requestIdentifier, "status": status])
                }
            } else if kind == "url", let appIdentifier = payload.appId,
                      let encodedURL = payload.encodedUrl {
                let requestingTask = task
                Task { [weak self, weak requestingTask] in
                    guard let self, let requestingTask, self.task === requestingTask, self.sessionAuthenticated else { return }
                    let opened = await self.resources.openURL(encodedURL: encodedURL, bundleIdentifier: appIdentifier)
                    guard self.task === requestingTask, self.sessionAuthenticated else { return }
                    self.sendJSON(["type": "action.result", "requestId": requestIdentifier,
                                   "status": opened ? "opened" : "not_allowed"])
                }
            } else { return false }
        case .valueSet(let payload):
            guard sessionAuthenticated else { return false }
            let requestIdentifier = payload.requestId
            let controlIdentifier = payload.controlId
            let value = Int(payload.value)
            let changed = resources.setMediaControlValue(value, controlIdentifier: controlIdentifier)
            sendJSON(["type": "action.result", "requestId": requestIdentifier,
                      "status": changed ? "performed" : "not_allowed"])
        case .error(let payload):
            let code = payload.code
            if code == "authentication_sequence",
               let panelSequence = payload.lastSequence,
               panelSequence < UInt32.max {
                preferences.setPreference(Int(panelSequence), forKey: authenticationSequenceKey)
                updateConnectionStatus("Authentication counter repaired — reconnecting", state: .reconnecting)
                connect(mode: .authenticate)
            } else {
                let message = code.replacingOccurrences(of: "_", with: " ")
                if sessionAuthenticated {
                    onEvent?(.status(message))
                } else {
                    hasTerminalConnectionError = true
                    shouldReconnect = false
                    tearDownConnection()
                    updateConnectionStatus(message, state: .failed)
                }
            }
        case .artworkAck(let payload):
            guard sessionAuthenticated else { return false }
            if payload.generation == artworkGeneration, Int(payload.nextOffset) == artworkOffset {
                sendNextArtworkChunk()
            }
        case .artworkAbort:
            guard sessionAuthenticated else { return false }
            resetArtworkTransferState()
        case .artworkRequest(let payload):
            guard sessionAuthenticated else { return false }
            onEvent?(.artworkRequested(payload.generation))
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
        guard let sendingTask = task else { resetArtworkTransferState(); return }
        sendingTask.send(.data(frame)) { [weak self, weak sendingTask] error in
            guard error != nil else { return }
            Task { @MainActor [weak self, weak sendingTask] in
                guard let self, let sendingTask, self.task === sendingTask else { return }
                self.resetArtworkTransferState()
            }
        }
    }

    private func sendJSON(_ object: [String: Any]) {
        var envelope = object
        envelope["protocol"] = CompanionCapabilities.protocolVersion
        guard JSONSerialization.isValidJSONObject(envelope),
              let data = try? JSONSerialization.data(withJSONObject: envelope), data.count <= Self.maximumTextFrameBytes,
              let value = String(data: data, encoding: .utf8) else { return }
        let state: CompanionProtocolState = sessionAuthenticated ? .connected : {
            if case .pair = mode { return .pairing }; return .authenticating
        }()
        guard CompanionProtocolDecoder.decode(data, direction: .macToPanel, state: state) != nil else { return }
        send(value)
    }

    func publishCatalogue() {
        guard sessionAuthenticated || task != nil else { return }
        lastFocusedActionIdentifier = nil
        let supportedWindowActions = Self.supportedWindowActionIDs(
            for: ProcessInfo.processInfo.operatingSystemVersion
        )
        var capabilities = (resources.mediaActionsAvailable ? ["media_actions"] : []) + supportedWindowActions
        capabilities.append("keyboard_shortcuts")
        sendJSON(["type": "capabilities", "values": capabilities])
        // Bundle identifiers are stable and opaque to the browser layout editor;
        // it never receives a path or an arbitrary shell command.
        // Approved folders are sent first so they remain available even when
        // the installed application catalogue reaches the frame limit.
        let entries: [[String: String]] = resources.folderActions().compactMap { folder -> [String: String]? in
            guard Self.validCatalogueIdentifier(folder.actionIdentifier) else { return nil }
            return ["id": folder.actionIdentifier, "label": Self.catalogueLabel(folder.name, fallback: "Folder")]
        } + resources.launchableApps().compactMap { app -> [String: String]? in
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
        let identifier = resources.focusedCompanionActionIdentifier()
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

    private var certificateFingerprintKey: String { "companion.certificateFingerprint.\(preferences.pairingAccount)" }
    private var authenticationSequenceKey: String { "companion.authenticationSequence.\(preferences.pairingAccount)" }

    private func nextAuthenticationSequence() -> UInt32 {
        let previous = UInt32(clamping: preferences.integerPreference(forKey: authenticationSequenceKey))
        let next = previous &+ 1
        preferences.setPreference(Int(next), forKey: authenticationSequenceKey)
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
