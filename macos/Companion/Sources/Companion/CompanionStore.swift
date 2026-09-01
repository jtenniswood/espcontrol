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

@MainActor
final class CompanionStore: NSObject, ObservableObject {
    @Published var panelHost: String { didSet { defaults.set(panelHost, forKey: Keys.host) } }
    @Published var panelName: String { didSet { defaults.set(panelName, forKey: Keys.name) } }
    @Published private(set) var availableApps: [LaunchableApp] = []
    @Published private(set) var allowedBundleIdentifiers: Set<String>
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

    private enum Keys {
        static let host = "panelHost"
        static let name = "panelName"
        static let allowed = "allowedApps"
    }
    private let defaults = UserDefaults.standard
    private lazy var connection = CompanionConnection(store: self)
    private let nowPlayingProvider = SystemNowPlayingProvider()
    private var latestNowPlayingSnapshot: CompanionNowPlayingSnapshot?

    override init() {
        panelHost = defaults.string(forKey: Keys.host) ?? ""
        panelName = defaults.string(forKey: Keys.name) ?? "My EspControl"
        allowedBundleIdentifiers = Set(defaults.stringArray(forKey: Keys.allowed) ?? [])
        nowPlayingSharingEnabled = defaults.object(forKey: "nowPlayingSharingEnabled") as? Bool ?? true
        super.init()
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

    var hasSavedPairing: Bool { KeychainStore.load(service: KeychainStore.service, account: panelHost) != nil }
    var connectionSymbol: String { isConnected ? "laptopcomputer" : "laptopcomputer.slash" }
    var allAvailableAppsAllowed: Bool {
        !availableApps.isEmpty && availableApps.allSatisfy { allowedBundleIdentifiers.contains($0.bundleIdentifier) }
    }
    var allowedAvailableAppCount: Int {
        availableApps.filter { allowedBundleIdentifiers.contains($0.bundleIdentifier) }.count
    }
    var hasAllowedApps: Bool { !allowedBundleIdentifiers.isEmpty }
    var supportsLaunchAtLogin: Bool {
        Bundle.main.bundleURL.pathExtension.lowercased() == "app"
    }

    func allowedBinding(for app: LaunchableApp) -> Binding<Bool> {
        Binding(
            get: { self.allowedBundleIdentifiers.contains(app.bundleIdentifier) },
            set: { enabled in
                if enabled { self.allowedBundleIdentifiers.insert(app.bundleIdentifier) }
                else { self.allowedBundleIdentifiers.remove(app.bundleIdentifier) }
                self.persistAllowedApplications()
            }
        )
    }

    func allowAllApplications() {
        allowedBundleIdentifiers = Set(availableApps.map(\.bundleIdentifier))
        persistAllowedApplications()
    }

    func disallowAllApplications() {
        allowedBundleIdentifiers.removeAll()
        persistAllowedApplications()
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

    private func persistAllowedApplications() {
        defaults.set(Array(allowedBundleIdentifiers).sorted(), forKey: Keys.allowed)
        connection.publishCatalogue()
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
        let additionalSystemApps = [URL(fileURLWithPath: "/System/Library/CoreServices/Finder.app")]
        let keys: Set<URLResourceKey> = [.isDirectoryKey]
        var found: [LaunchableApp] = []

        func appendApplication(at url: URL) {
            guard let bundle = Bundle(url: url), let id = bundle.bundleIdentifier else { return }
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
        additionalSystemApps.forEach { appendApplication(at: $0) }
        scanApplicationRoots(cryptexRoots)
        availableApps = Dictionary(grouping: found, by: \.bundleIdentifier).compactMap { $0.value.first }.sorted { $0.name.localizedCaseInsensitiveCompare($1.name) == .orderedAscending }
        if isConnected { connection.publishCatalogue() }
    }

    func launchableApps() -> [LaunchableApp] {
        availableApps.filter { allowedBundleIdentifiers.contains($0.bundleIdentifier) }
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
        panelName = ""
        statusDescription = "Panel forgotten"
    }

    func updateStatus(_ message: String, connected: Bool = false) {
        print("[EspControl Companion] \(message)")
        statusDescription = message
        isConnected = connected
        updateNowPlayingProvider()
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
    func launch(bundleIdentifier: String) -> Bool {
        guard let app = launchableApps().first(where: { $0.bundleIdentifier == bundleIdentifier }) else { return false }
        if app.bundleIdentifier == "com.apple.finder" {
            return NSWorkspace.shared.open(FileManager.default.homeDirectoryForCurrentUser)
        }
        NSWorkspace.shared.openApplication(at: app.url, configuration: .init()) { _, _ in }
        return true
    }

    func perform(actionIdentifier: String) -> Bool {
        guard actionIdentifier.hasPrefix(CompanionKeyboardShortcut.actionPrefix) else {
            return launch(bundleIdentifier: actionIdentifier)
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
            updateStatus("Blocked an invalid URL or unapproved app", connected: isConnected)
            return false
        }
        NSWorkspace.shared.open([url], withApplicationAt: app.url, configuration: .init()) { _, _ in }
        return true
    }
}
