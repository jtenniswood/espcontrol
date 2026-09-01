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
    let path: String

    var actionIdentifier: String { Self.actionPrefix + id.uuidString.lowercased() }

    static func identifier(from actionIdentifier: String) -> UUID? {
        guard actionIdentifier.hasPrefix(actionPrefix) else { return nil }
        return UUID(uuidString: String(actionIdentifier.dropFirst(actionPrefix.count)))
    }
}

@MainActor
final class CompanionStore: NSObject, ObservableObject {
    @Published var panelHost: String { didSet { defaults.set(panelHost, forKey: Keys.host) } }
    @Published private(set) var availableApps: [LaunchableApp] = []
    @Published private(set) var approvedFolders: [ApprovedFolder]
    @Published private(set) var statusDescription = "Not connected"
    @Published private(set) var isConnected = false
    @Published private(set) var launchAtLoginEnabled = false
    @Published private(set) var launchAtLoginMessage = ""
    private let nowPlayingSharingEnabled = false
    @Published private(set) var nowPlayingStatus = "Waiting for a panel connection"
    @Published private(set) var nowPlayingApplication = ""
    @Published private(set) var nowPlayingTitle = ""
    @Published private(set) var nowPlayingArtwork: NSImage?

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
    private var latestNowPlayingSnapshot: CompanionNowPlayingSnapshot?
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
        super.init()
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
        guard !approvedFolders.contains(where: { $0.path == url.path }) else {
            updateStatus("That folder is already available", connected: isConnected)
            return
        }
        let displayName = url.lastPathComponent.isEmpty ? url.path : url.lastPathComponent
        approvedFolders.append(ApprovedFolder(id: UUID(), name: displayName, path: url.path))
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
        updateNowPlayingProvider()
        if connected {
            startMediaControlPublishing()
        } else {
            mediaControlTimer?.invalidate()
            mediaControlTimer = nil
            lastMediaControlValues = [:]
        }
    }

    private func updateNowPlayingProvider() {
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
        var isDirectory: ObjCBool = false
        guard FileManager.default.fileExists(atPath: folder.path, isDirectory: &isDirectory),
              isDirectory.boolValue else {
            updateStatus("The selected folder is no longer available", connected: isConnected)
            return false
        }
        return NSWorkspace.shared.open(URL(fileURLWithPath: folder.path, isDirectory: true))
    }

    func perform(actionIdentifier: String) async -> Bool {
        if actionIdentifier.hasPrefix(ApprovedFolder.actionPrefix) {
            return openFolder(actionIdentifier: actionIdentifier)
        }
        if SystemMediaController.supports(actionIdentifier: actionIdentifier) {
            return mediaController.perform(actionIdentifier: actionIdentifier)
        }
        guard actionIdentifier.hasPrefix(CompanionKeyboardShortcut.actionPrefix) else {
            return await launch(bundleIdentifier: actionIdentifier)
        }
        guard let shortcut = CompanionKeyboardShortcut(actionIdentifier: actionIdentifier) else {
            updateStatus("Blocked an invalid keyboard shortcut", connected: isConnected)
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
                Task { @MainActor in self?.publishMediaControlValues() }
            }
        }
        publishMediaControlValues(force: true)
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
