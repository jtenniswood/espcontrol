import AppKit
import Combine
import Foundation
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

    private enum Keys { static let host = "panelHost"; static let name = "panelName"; static let allowed = "allowedApps"; static let fingerprint = "certificateFingerprint" }
    private let defaults = UserDefaults.standard
    private lazy var connection = CompanionConnection(store: self)

    override init() {
        panelHost = defaults.string(forKey: Keys.host) ?? ""
        panelName = defaults.string(forKey: Keys.name) ?? "My EspControl"
        allowedBundleIdentifiers = Set(defaults.stringArray(forKey: Keys.allowed) ?? [])
        super.init()
        refreshApplications()
    }

    var hasSavedPairing: Bool { KeychainStore.load(service: KeychainStore.service, account: panelHost) != nil }
    var connectionSymbol: String { isConnected ? "laptopcomputer.and.iphone" : "laptopcomputer.slash" }

    func allowedBinding(for app: LaunchableApp) -> Binding<Bool> {
        Binding(
            get: { self.allowedBundleIdentifiers.contains(app.bundleIdentifier) },
            set: { enabled in
                if enabled { self.allowedBundleIdentifiers.insert(app.bundleIdentifier) }
                else { self.allowedBundleIdentifiers.remove(app.bundleIdentifier) }
                self.defaults.set(Array(self.allowedBundleIdentifiers).sorted(), forKey: Keys.allowed)
                self.connection.publishCatalogue()
            }
        )
    }

    func refreshApplications() {
        let roots = [URL(fileURLWithPath: "/Applications"), URL(fileURLWithPath: "/System/Applications")]
        let keys: Set<URLResourceKey> = [.isDirectoryKey]
        var found: [LaunchableApp] = []
        for root in roots {
            guard let enumerator = FileManager.default.enumerator(at: root, includingPropertiesForKeys: Array(keys), options: [.skipsHiddenFiles, .skipsPackageDescendants]) else { continue }
            for case let url as URL in enumerator where url.pathExtension == "app" {
                guard let bundle = Bundle(url: url), let id = bundle.bundleIdentifier else { continue }
                let name = (bundle.object(forInfoDictionaryKey: "CFBundleDisplayName") as? String)
                    ?? (bundle.object(forInfoDictionaryKey: "CFBundleName") as? String) ?? url.deletingPathExtension().lastPathComponent
                found.append(LaunchableApp(bundleIdentifier: id, name: name, url: url))
            }
        }
        availableApps = Dictionary(grouping: found, by: \.bundleIdentifier).compactMap { $0.value.first }.sorted { $0.name.localizedCaseInsensitiveCompare($1.name) == .orderedAscending }
    }

    func selectedApps() -> [LaunchableApp] { availableApps.filter { allowedBundleIdentifiers.contains($0.bundleIdentifier) } }
    func connect() { connection.connect(mode: .authenticate) }
    func pair(code: String, verificationCode: String) {
        connection.connect(mode: .pair(
            code: code.trimmingCharacters(in: .whitespacesAndNewlines).uppercased(),
            verificationCode: verificationCode.trimmingCharacters(in: .whitespacesAndNewlines).uppercased()))
    }

    func forgetPanel() {
        KeychainStore.remove(service: KeychainStore.service, account: panelHost)
        defaults.removeObject(forKey: "companion.certificateFingerprint.\(panelHost)")
        defaults.removeObject(forKey: "companion.authenticationSequence.\(panelHost)")
        connection.disconnect()
        statusDescription = "Panel forgotten"
    }

    func updateStatus(_ message: String, connected: Bool = false) { statusDescription = message; isConnected = connected }
    func launch(bundleIdentifier: String) -> Bool {
        guard let app = selectedApps().first(where: { $0.bundleIdentifier == bundleIdentifier }) else { return false }
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
}
