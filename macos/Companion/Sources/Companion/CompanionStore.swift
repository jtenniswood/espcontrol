import AppKit
import Combine
import Foundation
import ServiceManagement
import SwiftUI

struct LaunchableApp: Identifiable, Hashable {
    let bundleIdentifier: String
    let name: String
    let url: URL

    var id: String { bundleIdentifier }
}

struct ApprovedFolder: Codable, Identifiable, Hashable {
    static let actionPrefix = "folder."

    let id: UUID
    let name: String
    let bookmarkData: Data?
    private let legacyPath: String?

    init(id: UUID, name: String, path: String) {
        self.id = id
        self.name = name
        self.bookmarkData = nil
        self.legacyPath = path
    }

    init(id: UUID, name: String, bookmarkData: Data) {
        self.id = id
        self.name = name
        self.bookmarkData = bookmarkData
        self.legacyPath = nil
    }

    private enum CodingKeys: String, CodingKey {
        case id, name, bookmarkData, path
    }

    init(from decoder: Decoder) throws {
        let values = try decoder.container(keyedBy: CodingKeys.self)
        id = try values.decode(UUID.self, forKey: .id)
        name = try values.decode(String.self, forKey: .name)
        bookmarkData = try values.decodeIfPresent(Data.self, forKey: .bookmarkData)
        legacyPath = try values.decodeIfPresent(String.self, forKey: .path)
    }

    func encode(to encoder: Encoder) throws {
        var values = encoder.container(keyedBy: CodingKeys.self)
        try values.encode(id, forKey: .id)
        try values.encode(name, forKey: .name)
        try values.encodeIfPresent(bookmarkData, forKey: .bookmarkData)
    }

    var path: String {
        securityScopedURL()?.url.path ?? legacyPath ?? "Access needs approval"
    }

    var needsReapproval: Bool { bookmarkData == nil }

    struct SecurityScopedURL {
        let url: URL
        let refreshedBookmarkData: Data?
    }

    func securityScopedURL() -> SecurityScopedURL? {
        guard let bookmarkData else { return nil }
        var isStale = false
        guard let url = try? URL(
            resolvingBookmarkData: bookmarkData,
            options: [.withSecurityScope],
            relativeTo: nil,
            bookmarkDataIsStale: &isStale
        ) else { return nil }
        let refreshedBookmarkData = isStale
            ? try? url.bookmarkData(options: [.withSecurityScope], includingResourceValuesForKeys: nil, relativeTo: nil)
            : nil
        return SecurityScopedURL(url: url, refreshedBookmarkData: refreshedBookmarkData)
    }

    func withSecurityScopedAccess<T>(_ body: (URL) -> T) -> T? {
        guard let access = securityScopedURL(), access.url.startAccessingSecurityScopedResource() else {
            return nil
        }
        defer { access.url.stopAccessingSecurityScopedResource() }
        return body(access.url)
    }

    var actionIdentifier: String { Self.actionPrefix + id.uuidString.lowercased() }

    static func identifier(from actionIdentifier: String) -> UUID? {
        guard actionIdentifier.hasPrefix(actionPrefix) else { return nil }
        return UUID(uuidString: String(actionIdentifier.dropFirst(actionPrefix.count)))
    }

    static func actionIdentifier(forFocusedPath path: String, in folders: [ApprovedFolder]) -> String {
        guard !path.isEmpty else { return "" }
        let focusedPath = URL(fileURLWithPath: path).standardizedFileURL.resolvingSymlinksInPath().path
        return folders.first { folder in
            URL(fileURLWithPath: folder.path).standardizedFileURL.resolvingSymlinksInPath().path == focusedPath
        }?.actionIdentifier ?? ""
    }
}

@MainActor
final class CompanionStore: NSObject, ObservableObject {
    static let privacyPolicyURL = URL(string: "https://jtenniswood.github.io/espcontrol/reference/privacy")!
    static let supportURL = URL(string: "https://github.com/jtenniswood/espcontrol/issues")!

    @Published var panelHost: String { didSet { defaults.set(panelHost, forKey: Keys.host) } }
    @Published private(set) var availableApps: [LaunchableApp] = []
    @Published private(set) var approvedFolders: [ApprovedFolder]
    @Published private(set) var statusDescription = "Not connected"
    @Published private(set) var isConnected = false
    @Published private(set) var launchAtLoginEnabled = false
    @Published private(set) var launchAtLoginMessage = ""
    @Published var nowPlayingSharingEnabled: Bool {
        didSet {
            defaults.set(nowPlayingSharingEnabled, forKey: "nowPlayingSharingEnabled")
            updateNowPlayingProvider()
        }
    }
    @Published private(set) var nowPlayingStatus = "Waiting for a panel connection"
    @Published private(set) var nowPlayingApplication = ""
    @Published private(set) var nowPlayingTitle = ""
    @Published private(set) var nowPlayingArtwork: NSImage?
    @Published var systemMetricsSharingEnabled: Bool {
        didSet {
            defaults.set(systemMetricsSharingEnabled, forKey: "systemMetricsSharingEnabled")
            updateSystemMetricsProvider()
        }
    }
    @Published private(set) var systemMetricsStatus = "Waiting for a panel connection"
    @Published private(set) var systemMetricsSupported = false

    private enum Keys {
        static let host = "panelHost"
        static let approvedFolders = "approvedFolders"
    }
    private static let preferencesSuite = "io.espcontrol.companion"
    private static let legacyPreferencesSuite = "EspControl Companion"
    private let defaults: UserDefaults
    private lazy var connection = CompanionConnection(store: self)
    private let nowPlayingProvider = SystemNowPlayingProvider()
    private let mediaController = SystemMediaController()
    private let systemMetricsProvider = SystemMetricsProvider()
    private var latestNowPlayingSnapshot: CompanionNowPlayingSnapshot?
    private var latestSystemMetricsSnapshot: CompanionSystemMetricsSnapshot?
    private var mediaControlTimer: Timer?
    private var lastMediaControlValues: [String: Int] = [:]

    override init() {
        let stableDefaults = UserDefaults(suiteName: Self.preferencesSuite) ?? .standard
        let legacyDefaults = UserDefaults(suiteName: Self.legacyPreferencesSuite)
        defaults = stableDefaults
        approvedFolders = stableDefaults.data(forKey: Keys.approvedFolders)
            .flatMap { try? JSONDecoder().decode([ApprovedFolder].self, from: $0) }
            ?? []
        panelHost = stableDefaults.string(forKey: Keys.host)
            ?? legacyDefaults?.string(forKey: Keys.host)
            ?? UserDefaults.standard.string(forKey: Keys.host)
            ?? KeychainStore.accounts(service: KeychainStore.service).first
            ?? ""
        nowPlayingSharingEnabled = stableDefaults.object(forKey: "nowPlayingSharingEnabled") as? Bool ?? true
        systemMetricsSharingEnabled = stableDefaults.object(forKey: "systemMetricsSharingEnabled") as? Bool
            ?? legacyDefaults?.object(forKey: "systemMetricsSharingEnabled") as? Bool
            ?? UserDefaults.standard.object(forKey: "systemMetricsSharingEnabled") as? Bool
            ?? true
        super.init()
        NSWorkspace.shared.notificationCenter.addObserver(
            self,
            selector: #selector(frontmostApplicationDidChange(_:)),
            name: NSWorkspace.didActivateApplicationNotification,
            object: nil
        )
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(systemTimeZoneDidChange(_:)),
            name: NSNotification.Name.NSSystemTimeZoneDidChange,
            object: nil
        )
        migrateConnectionPreferences(from: [legacyDefaults, UserDefaults.standard].compactMap { $0 })
        nowPlayingProvider.onStatus = { [weak self] value in self?.nowPlayingStatus = value }
        nowPlayingProvider.onSnapshot = { [weak self] snapshot in
            guard let self else { return }
            nowPlayingApplication = snapshot.applicationName
            nowPlayingTitle = snapshot.title
            nowPlayingArtwork = snapshot.artworkJPEG.flatMap(NSImage.init(data:))
            latestNowPlayingSnapshot = snapshot
            if isConnected { connection.publishNowPlaying(snapshot) }
        }
        systemMetricsProvider.onSnapshot = { [weak self] snapshot in
            guard let self else { return }
            guard isConnected && systemMetricsSharingEnabled && systemMetricsSupported else { return }
            latestSystemMetricsSnapshot = snapshot
            systemMetricsStatus = "Sharing processor, memory, storage, network and battery statistics"
            if isConnected { connection.publishSystemMetrics(snapshot) }
        }
        if supportsLaunchAtLogin { refreshLaunchAtLoginStatus() }
        refreshApplications()
    }

    private func migrateConnectionPreferences(from legacyStores: [UserDefaults]) {
        guard !panelHost.isEmpty else { return }
        defaults.set(panelHost, forKey: Keys.host)
        let keys = [
            "companion.certificateFingerprint.\(panelHost)",
            "companion.authenticationSequence.\(panelHost)",
        ]
        for key in keys where defaults.object(forKey: key) == nil {
            if let value = legacyStores.lazy.compactMap({ $0.object(forKey: key) }).first {
                defaults.set(value, forKey: key)
            }
        }
    }

    func stringPreference(forKey key: String) -> String? { defaults.string(forKey: key) }
    func integerPreference(forKey key: String) -> Int { defaults.integer(forKey: key) }
    func setPreference(_ value: Any, forKey key: String) { defaults.set(value, forKey: key) }
    func removePreference(forKey key: String) { defaults.removeObject(forKey: key) }

    var hasSavedPairing: Bool {
        !panelHost.isEmpty && KeychainStore.accounts(service: KeychainStore.service).contains(panelHost)
    }
    var supportsLaunchAtLogin: Bool {
        Bundle.main.bundleURL.pathExtension.lowercased() == "app"
    }
    func launchAtLoginBinding() -> Binding<Bool> {
        Binding(
            get: { self.launchAtLoginEnabled },
            set: { self.setLaunchAtLogin($0) }
        )
    }

    private func setLaunchAtLogin(_ enabled: Bool) {
        guard supportsLaunchAtLogin else { return }
        let service = SMAppService.mainApp
        do {
            if enabled {
                if service.status == .notRegistered || service.status == .notFound {
                    try service.register()
                }
            } else if service.status != .notRegistered && service.status != .notFound {
                try service.unregister()
            }
            refreshLaunchAtLoginStatus()
        } catch {
            refreshLaunchAtLoginStatus()
            launchAtLoginMessage = "macOS could not update this login item. Install the app in Applications and try again."
        }
    }

    func refreshLaunchAtLoginStatus() {
        switch SMAppService.mainApp.status {
        case .enabled:
            launchAtLoginEnabled = true
            launchAtLoginMessage = "The app will open automatically after you sign in."
        case .requiresApproval:
            launchAtLoginEnabled = true
            launchAtLoginMessage = "Allow the app in System Settings → General → Login Items to finish enabling it."
        case .notFound:
            launchAtLoginEnabled = false
            launchAtLoginMessage = "Install the app in Applications before enabling this setting."
        default:
            launchAtLoginEnabled = false
            launchAtLoginMessage = "The app will stay closed when you sign in."
        }
    }

    func refreshApplications() {
        let standardRoots = [
            URL(fileURLWithPath: "/Applications"),
            URL(fileURLWithPath: "/System/Applications"),
            FileManager.default.homeDirectoryForCurrentUser.appendingPathComponent("Applications", isDirectory: true),
        ]
        let cryptexRoots = [
            // Current macOS releases install Safari in the protected Cryptex
            // application volume and expose only a compatibility link in /Applications.
            URL(fileURLWithPath: "/System/Cryptexes/App/System/Applications"),
        ]
        let keys: Set<URLResourceKey> = [.isDirectoryKey]
        var found: [LaunchableApp] = []

        func appendApplication(at url: URL) {
            guard let bundle = Bundle(url: url), let id = bundle.bundleIdentifier else { return }
            guard id != "com.apple.finder" else { return }
            let name = (bundle.object(forInfoDictionaryKey: "CFBundleDisplayName") as? String)
                ?? (bundle.object(forInfoDictionaryKey: "CFBundleName") as? String)
                ?? url.deletingPathExtension().lastPathComponent
            found.append(LaunchableApp(bundleIdentifier: id, name: name, url: url))
        }

        func scanApplicationRoots(_ roots: [URL]) {
            for root in roots {
                guard let enumerator = FileManager.default.enumerator(at: root, includingPropertiesForKeys: Array(keys), options: [.skipsHiddenFiles, .skipsPackageDescendants]) else { continue }
                for case let url as URL in enumerator where url.pathExtension == "app" {
                    appendApplication(at: url)
                }
            }
        }
        scanApplicationRoots(standardRoots)
        scanApplicationRoots(cryptexRoots)
        availableApps = Dictionary(grouping: found, by: \.bundleIdentifier).compactMap { $0.value.first }.sorted { $0.name.localizedCaseInsensitiveCompare($1.name) == .orderedAscending }
        if isConnected { connection.publishCatalogue() }
    }

    func launchableApps() -> [LaunchableApp] { availableApps }
    func folderActions() -> [ApprovedFolder] { approvedFolders }
    func focusedLaunchableApplicationIdentifier() -> String {
        guard let application = NSWorkspace.shared.frontmostApplication,
              CompanionWindowDetector.hasVisibleWindow(for: application),
              let identifier = application.bundleIdentifier,
              availableApps.contains(where: { $0.bundleIdentifier == identifier }) else { return "" }
        return identifier
    }

    func focusedCompanionActionIdentifier() -> String {
        guard let application = NSWorkspace.shared.frontmostApplication,
              let bundleIdentifier = application.bundleIdentifier,
              CompanionWindowDetector.hasVisibleWindow(for: application) else { return "" }
        if bundleIdentifier == "com.apple.finder" {
            let folderAction = focusedFinderFolderActionIdentifier()
            return folderAction.isEmpty ? bundleIdentifier : folderAction
        }
        return bundleIdentifier
    }

    private func focusedFinderFolderActionIdentifier() -> String {
        guard let path = focusedFinderFolderPath() else { return "" }
        return ApprovedFolder.actionIdentifier(forFocusedPath: path, in: approvedFolders)
    }

    private func focusedFinderFolderPath() -> String? {
        let source = """
        tell application "Finder"
            if (count of windows) is 0 then return ""
            try
                set currentTarget to target of front window
                return POSIX path of (currentTarget as alias)
            on error
                return ""
            end try
        end tell
        """
        var error: NSDictionary?
        guard let descriptor = NSAppleScript(source: source)?.executeAndReturnError(&error),
              let path = descriptor.stringValue?.trimmingCharacters(in: .whitespacesAndNewlines),
              !path.isEmpty else { return nil }
        return path
    }

    @objc private func frontmostApplicationDidChange(_: Notification) {
        if isConnected { connection.publishFocusedAction() }
    }

    @objc private func systemTimeZoneDidChange(_: Notification) {
        if isConnected { connection.publishTimezone() }
    }

    func chooseFolder() {
        let panel = NSOpenPanel()
        panel.title = "Choose a folder for EspControl cards"
        panel.prompt = "Add Folder"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.allowsMultipleSelection = false
        panel.canCreateDirectories = true
        guard panel.runModal() == .OK, let selected = panel.url else { return }

        let url = selected.standardizedFileURL
        guard let bookmarkData = try? url.bookmarkData(
            options: [.withSecurityScope],
            includingResourceValuesForKeys: nil,
            relativeTo: nil
        ) else {
            updateStatus("macOS could not save access to that folder", connected: isConnected)
            return
        }
        guard !approvedFolders.contains(where: { $0.path == url.path }) else {
            updateStatus("That folder is already available", connected: isConnected)
            return
        }
        let displayName = url.lastPathComponent.isEmpty ? url.path : url.lastPathComponent
        approvedFolders.append(ApprovedFolder(id: UUID(), name: displayName, bookmarkData: bookmarkData))
        approvedFolders.sort { $0.name.localizedCaseInsensitiveCompare($1.name) == .orderedAscending }
        persistApprovedFolders()
        if isConnected { connection.publishCatalogue() }
    }

    func removeFolder(_ folder: ApprovedFolder) {
        approvedFolders.removeAll { $0.id == folder.id }
        persistApprovedFolders()
        if isConnected { connection.publishCatalogue() }
    }

    private func persistApprovedFolders() {
        if let data = try? JSONEncoder().encode(approvedFolders) {
            defaults.set(data, forKey: Keys.approvedFolders)
        }
    }

    private func refreshBookmark(for folder: ApprovedFolder, with data: Data) {
        guard let index = approvedFolders.firstIndex(where: { $0.id == folder.id }) else { return }
        approvedFolders[index] = ApprovedFolder(id: folder.id, name: folder.name, bookmarkData: data)
        persistApprovedFolders()
    }
    func connect() { connection.connect(mode: .authenticate) }
    func disconnect() { connection.disconnect() }
    func openPanelWebServer() {
        guard !panelHost.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else {
            updateStatus("Enter the panel address first", connected: isConnected)
            return
        }
        guard let url = Self.panelWebServerURL(from: panelHost),
              NSWorkspace.shared.open(url) else {
            updateStatus("Could not open the panel webserver", connected: isConnected)
            return
        }
    }

    static func panelWebServerURL(from value: String) -> URL? {
        let raw = value.trimmingCharacters(in: .whitespacesAndNewlines)
        guard let parsed = URL(string: raw.contains("://") ? raw : "http://\(raw)"),
              let host = parsed.host else { return nil }
        var components = URLComponents()
        components.scheme = parsed.scheme?.lowercased() == "https" ? "https" : "http"
        components.host = host
        return components.url
    }
    func pair(code: String) {
        connection.connect(mode: .pair(
            code: code.trimmingCharacters(in: .whitespacesAndNewlines).uppercased()))
    }

    func forgetPanel() {
        let forgottenHost = panelHost
        KeychainStore.remove(service: KeychainStore.service, account: forgottenHost)
        defaults.removeObject(forKey: "companion.certificateFingerprint.\(forgottenHost)")
        defaults.removeObject(forKey: "companion.authenticationSequence.\(forgottenHost)")
        connection.disconnect()
        panelHost = ""
        statusDescription = "Panel forgotten"
    }

    func updateStatus(_ message: String, connected: Bool = false) {
        print("[EspControl Companion] \(message)")
        statusDescription = message
        isConnected = connected
        if !connected { systemMetricsSupported = false }
        updateNowPlayingProvider()
        updateSystemMetricsProvider()
        if connected {
            startMediaControlPublishing()
        } else {
            mediaControlTimer?.invalidate()
            mediaControlTimer = nil
            lastMediaControlValues = [:]
        }
    }

    private func updateSystemMetricsProvider() {
        if isConnected && systemMetricsSharingEnabled && systemMetricsSupported {
            systemMetricsStatus = "Collecting Mac system statistics…"
            systemMetricsProvider.start()
        } else {
            if isConnected && systemMetricsSupported && !systemMetricsSharingEnabled {
                connection.publishSystemMetricsUnavailable()
                latestSystemMetricsSnapshot = nil
            }
            systemMetricsProvider.stop()
            systemMetricsStatus = systemMetricsSharingEnabled
                ? "Waiting for a panel connection" : "Mac system statistics sharing is disabled"
        }
    }

    func setSystemMetricsSupported(_ supported: Bool) {
        systemMetricsSupported = supported
        updateSystemMetricsProvider()
    }

    private func updateNowPlayingProvider() {
        if !isConnected || !nowPlayingSharingEnabled {
            // Do not carry a confirmed session across disconnects or disabled sharing.
            latestNowPlayingSnapshot = nil
        }
        if isConnected && nowPlayingSharingEnabled {
            nowPlayingProvider.start()
        } else {
            if isConnected && !nowPlayingSharingEnabled {
                nowPlayingProvider.stopAndPublishUnavailable()
            } else {
                nowPlayingProvider.stop()
            }
            nowPlayingStatus = nowPlayingSharingEnabled
                ? "Waiting for a panel connection" : "Now Playing sharing is disabled"
        }
    }

    func republishNowPlayingArtwork(generation: UInt32) {
        guard isConnected, nowPlayingSharingEnabled,
              let snapshot = latestNowPlayingSnapshot,
              snapshot.generation == generation else { return }
        connection.publishNowPlaying(snapshot, forceArtwork: true)
    }

    func republishCurrentNowPlaying() {
        guard isConnected, nowPlayingSharingEnabled, let snapshot = latestNowPlayingSnapshot else { return }
        connection.publishNowPlaying(snapshot, forceArtwork: true)
    }
    func republishCurrentSystemMetrics() {
        guard isConnected, systemMetricsSharingEnabled, systemMetricsSupported,
              let snapshot = latestSystemMetricsSnapshot else { return }
        connection.publishSystemMetrics(snapshot)
    }

    func publishSystemMetricsUnavailable() {
        guard isConnected, systemMetricsSharingEnabled, systemMetricsSupported else { return }
        latestSystemMetricsSnapshot = nil
        connection.publishSystemMetricsUnavailable()
    }
    func launch(bundleIdentifier: String) async -> Bool {
        guard let app = launchableApps().first(where: { $0.bundleIdentifier == bundleIdentifier }) else { return false }
        let configuration = NSWorkspace.OpenConfiguration()
        configuration.activates = true
        let runningApplication: NSRunningApplication? = await withCheckedContinuation { continuation in
            NSWorkspace.shared.openApplication(at: app.url, configuration: configuration) { application, error in
                continuation.resume(returning: error == nil ? application : nil)
            }
        }
        guard let runningApplication else { return false }
        _ = runningApplication.activate(options: [.activateIgnoringOtherApps])
        for _ in 0..<30 {
            if NSWorkspace.shared.frontmostApplication?.bundleIdentifier == bundleIdentifier {
                return true
            }
            try? await Task.sleep(nanoseconds: 100_000_000)
        }
        updateStatus("\(app.name) did not become active", connected: isConnected)
        return false
    }

    func performResultStatus(actionIdentifier: String) async -> String {
        let isApplicationLaunch = !actionIdentifier.hasPrefix(ApprovedFolder.actionPrefix)
            && !SystemMediaController.supports(actionIdentifier: actionIdentifier)
            && !actionIdentifier.hasPrefix(CompanionKeyboardShortcut.actionPrefix)
            && !actionIdentifier.hasPrefix(CompanionKeyboardShortcut.windowActionPrefix)
        let performed = await perform(actionIdentifier: actionIdentifier)
        guard performed else { return "not_allowed" }
        return isApplicationLaunch ? "activated" : "performed"
    }

    func openFolder(actionIdentifier: String) -> Bool {
        guard let identifier = ApprovedFolder.identifier(from: actionIdentifier),
              let folder = approvedFolders.first(where: { $0.id == identifier }) else {
            updateStatus("Blocked an unavailable folder", connected: isConnected)
            return false
        }
        guard let access = folder.securityScopedURL(),
              access.url.startAccessingSecurityScopedResource() else {
            updateStatus("Re-add this folder to restore its macOS permission", connected: isConnected)
            return false
        }
        defer { access.url.stopAccessingSecurityScopedResource() }
        if let refreshedBookmarkData = access.refreshedBookmarkData {
            refreshBookmark(for: folder, with: refreshedBookmarkData)
        }
        var isDirectory: ObjCBool = false
        guard FileManager.default.fileExists(atPath: access.url.path, isDirectory: &isDirectory),
              isDirectory.boolValue else {
            updateStatus("The selected folder is no longer available", connected: isConnected)
            return false
        }
        return NSWorkspace.shared.open(access.url)
    }

    func perform(actionIdentifier: String) async -> Bool {
        if actionIdentifier.hasPrefix(ApprovedFolder.actionPrefix) {
            return openFolder(actionIdentifier: actionIdentifier)
        }
        if SystemMediaController.supports(actionIdentifier: actionIdentifier) {
            return mediaController.perform(actionIdentifier: actionIdentifier)
        }
        guard actionIdentifier.hasPrefix(CompanionKeyboardShortcut.actionPrefix) ||
              actionIdentifier.hasPrefix(CompanionKeyboardShortcut.windowActionPrefix) else {
            return await launch(bundleIdentifier: actionIdentifier)
        }
        guard let shortcut = CompanionKeyboardShortcut(actionIdentifier: actionIdentifier) else {
            updateStatus("Blocked an invalid keyboard shortcut", connected: isConnected)
            return false
        }
        guard shortcut.isSupported(on: ProcessInfo.processInfo.operatingSystemVersion) else {
            updateStatus("Window tiling requires macOS 15 or later", connected: isConnected)
            return false
        }
        guard shortcut.replay() else {
            updateStatus("Allow EspControl Companion in Privacy & Security → Accessibility", connected: isConnected)
            return false
        }
        return true
    }

    func setMediaControlValue(_ value: Int, controlIdentifier: String) -> Bool {
        guard mediaController.setValue(value, controlIdentifier: controlIdentifier) else { return false }
        publishMediaControlValues(force: true)
        return true
    }

    private func startMediaControlPublishing() {
        if mediaControlTimer == nil {
            mediaControlTimer = Timer.scheduledTimer(withTimeInterval: 1, repeats: true) { [weak self] _ in
                Task { @MainActor in
                    self?.publishMediaControlValues()
                    self?.connection.publishFocusedAction()
                }
            }
        }
        publishMediaControlValues(force: true)
        connection.publishFocusedAction()
    }

    private func publishMediaControlValues(force: Bool = false) {
        guard isConnected else { return }
        let values = mediaController.values()
        guard force || values != lastMediaControlValues else { return }
        let unavailable = SystemMediaController.unavailableVolumeIDs(
            values: values,
            previousValues: lastMediaControlValues,
            force: force
        )
        lastMediaControlValues = values
        connection.publishMediaControlValues(values, unavailable: unavailable)
    }

    func openURL(encodedURL: String, bundleIdentifier: String) -> Bool {
        guard encodedURL.utf8.count <= 128,
              let value = encodedURL.removingPercentEncoding,
              let components = URLComponents(string: value),
              let scheme = components.scheme?.lowercased(),
              (scheme == "http" || scheme == "https"),
              components.host != nil,
              components.user == nil,
              components.password == nil,
              let url = components.url,
              let app = launchableApps().first(where: { $0.bundleIdentifier == bundleIdentifier }) else {
            updateStatus("Blocked an invalid URL or unavailable app", connected: isConnected)
            return false
        }
        NSWorkspace.shared.open([url], withApplicationAt: app.url, configuration: .init()) { _, _ in }
        return true
    }
}
