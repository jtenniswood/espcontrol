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
    @Published private(set) var availableApps: [LaunchableApp] = []
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

    private enum Keys { static let host = "panelHost" }
    private static let preferencesSuite = "io.espcontrol.companion"
    private static let legacyPreferencesSuite = "EspControl Companion"
    private let defaults: UserDefaults
    private lazy var connection = CompanionConnection(store: self)
    private let nowPlayingProvider = SystemNowPlayingProvider()
    private let mediaController = SystemMediaController()
    private var mediaControlTimer: Timer?
    private var lastMediaControlValues: [String: Int] = [:]

    override init() {
        let stableDefaults = UserDefaults(suiteName: Self.preferencesSuite) ?? .standard
        let legacyDefaults = UserDefaults(suiteName: Self.legacyPreferencesSuite)
        defaults = stableDefaults
        panelHost = stableDefaults.string(forKey: Keys.host)
            ?? legacyDefaults?.string(forKey: Keys.host)
            ?? UserDefaults.standard.string(forKey: Keys.host)
            ?? KeychainStore.accounts(service: KeychainStore.service).first
            ?? ""
        nowPlayingSharingEnabled = stableDefaults.object(forKey: "nowPlayingSharingEnabled") as? Bool
            ?? legacyDefaults?.object(forKey: "nowPlayingSharingEnabled") as? Bool
            ?? UserDefaults.standard.object(forKey: "nowPlayingSharingEnabled") as? Bool
            ?? true
        super.init()
        migrateConnectionPreferences(from: [legacyDefaults, UserDefaults.standard].compactMap { $0 })
        nowPlayingProvider.onStatus = { [weak self] value in self?.nowPlayingStatus = value }
        nowPlayingProvider.onSnapshot = { [weak self] snapshot in
            guard let self else { return }
            nowPlayingApplication = snapshot.applicationName
            nowPlayingTitle = snapshot.title
            nowPlayingArtwork = snapshot.artworkJPEG.flatMap(NSImage.init(data:))
            if isConnected { connection.publishNowPlaying(snapshot) }
        }
        refreshLaunchAtLoginStatus()
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
    var connectionSymbol: String { isConnected ? "laptopcomputer" : "laptopcomputer.slash" }
    func launchAtLoginBinding() -> Binding<Bool> {
        Binding(
            get: { self.launchAtLoginEnabled },
            set: { self.setLaunchAtLogin($0) }
        )
    }

    private func setLaunchAtLogin(_ enabled: Bool) {
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

    func launchableApps() -> [LaunchableApp] { availableApps }
    func connect() { connection.connect(mode: .authenticate) }
    func disconnect() { connection.disconnect() }
    func openPanelWebServer() {
        let host = panelHost.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !host.isEmpty else {
            updateStatus("Enter the panel address first", connected: isConnected)
            return
        }
        var components = URLComponents()
        components.scheme = "http"
        components.host = host
        guard let url = components.url, NSWorkspace.shared.open(url) else {
            updateStatus("Could not open the panel webserver", connected: isConnected)
            return
        }
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
    func launch(bundleIdentifier: String) -> Bool {
        guard let app = launchableApps().first(where: { $0.bundleIdentifier == bundleIdentifier }) else { return false }
        if app.bundleIdentifier == "com.apple.finder" {
            return NSWorkspace.shared.open(FileManager.default.homeDirectoryForCurrentUser)
        }
        NSWorkspace.shared.openApplication(at: app.url, configuration: .init()) { _, _ in }
        return true
    }

    func perform(actionIdentifier: String) -> Bool {
        if SystemMediaController.supports(actionIdentifier: actionIdentifier) {
            return mediaController.perform(actionIdentifier: actionIdentifier)
        }
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
        guard encodedURL.utf8.count <= 1024,
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
