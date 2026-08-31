import AppKit
import Combine
import SwiftUI

@main
struct CompanionApp: App {
    @NSApplicationDelegateAdaptor(CompanionApplicationDelegate.self) private var appDelegate

    var body: some Scene {
        Settings {
            CompanionSettings(store: appDelegate.store)
                .frame(width: 560, height: 500)
        }
    }
}

@MainActor
final class CompanionApplicationDelegate: NSObject, NSApplicationDelegate {
    let store = CompanionStore()
    private var statusItem: NSStatusItem?
    private var connectionObservation: AnyCancellable?

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.setActivationPolicy(.accessory)
        let item = NSStatusBar.system.statusItem(withLength: NSStatusItem.squareLength)
        statusItem = item
        if let button = item.button {
            button.target = self
            button.action = #selector(statusItemClicked(_:))
            button.sendAction(on: [.leftMouseUp, .rightMouseUp])
            button.toolTip = "EspControl Companion"
        }
        updateStatusItemImage()
        connectionObservation = store.$isConnected
            .removeDuplicates()
            .sink { [weak self] _ in self?.updateStatusItemImage() }
    }

    func applicationDidBecomeActive(_ notification: Notification) {
        store.refreshLaunchAtLoginStatus()
    }

    @objc private func statusItemClicked(_ sender: NSStatusBarButton) {
        guard let event = NSApp.currentEvent else {
            openCompanionWindow()
            return
        }
        if event.type == .rightMouseUp {
            NSMenu.popUpContextMenu(contextMenu(), with: event, for: sender)
        } else {
            openCompanionWindow()
        }
    }

    private func contextMenu() -> NSMenu {
        let menu = NSMenu()
        let connectItem = NSMenuItem(
            title: store.isConnected ? "Reconnect" : "Connect",
            action: #selector(connect),
            keyEquivalent: ""
        )
        connectItem.target = self
        menu.addItem(connectItem)
        menu.addItem(.separator())

        let settingsItem = NSMenuItem(title: "Settings…", action: #selector(openSettings), keyEquivalent: ",")
        settingsItem.target = self
        menu.addItem(settingsItem)
        menu.addItem(.separator())

        let quitItem = NSMenuItem(title: "Quit EspControl Companion", action: #selector(quit), keyEquivalent: "q")
        quitItem.target = self
        menu.addItem(quitItem)
        return menu
    }

    private func updateStatusItemImage() {
        guard let button = statusItem?.button else { return }
        let image = NSImage(systemSymbolName: store.connectionSymbol, accessibilityDescription: "EspControl Companion")
        image?.isTemplate = true
        button.image = image
    }

    @objc private func connect() { store.connect() }
    @objc private func openSettings() { openCompanionWindow() }
    @objc private func quit() { NSApp.terminate(nil) }

    private func openCompanionWindow() {
        activateCompanionApplication()
        let opened = NSApp.sendAction(Selector(("showSettingsWindow:")), to: nil, from: nil)
        if !opened {
            NSApp.sendAction(Selector(("showPreferencesWindow:")), to: nil, from: nil)
        }
        focusSettingsWindow()
    }
}

private func activateCompanionApplication() {
    NSApp.setActivationPolicy(.regular)
    NSApp.unhide(nil)
    NSRunningApplication.current.activate(options: [
        .activateAllWindows,
        .activateIgnoringOtherApps,
    ])
    NSApp.activate(ignoringOtherApps: true)
}

private func focusSettingsWindow(
    attemptsRemaining: Int = 8,
    completion: (() -> Void)? = nil
) {
    activateCompanionApplication()
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
        guard let window = NSApp.windows.first(where: {
            $0.title.localizedCaseInsensitiveContains("Settings")
        }) else {
            if attemptsRemaining > 1 {
                focusSettingsWindow(
                    attemptsRemaining: attemptsRemaining - 1,
                    completion: completion
                )
            }
            return
        }

        window.level = .normal
        window.orderFrontRegardless()
        window.makeMain()
        window.makeKeyAndOrderFront(nil)
        activateCompanionApplication()
        completion?()
    }
}

private enum CompanionSettingsField: Hashable {
    case panelHost, panelName, pairingCode, verificationCode
}

private struct CompanionPairingDetails {
    let panelHost: String
    let pairingCode: String
    let verificationCode: String

    static func parse(_ text: String) -> CompanionPairingDetails? {
        var values: [String: String] = [:]
        for line in text.components(separatedBy: .newlines) {
            let parts = line.split(separator: ":", maxSplits: 1).map {
                $0.trimmingCharacters(in: .whitespacesAndNewlines)
            }
            if parts.count == 2 { values[parts[0].lowercased()] = parts[1] }
        }
        guard var panel = values["panel"],
              let pairingCode = values["pairing code"],
              let verificationCode = values["verify code"] else { return nil }
        if let url = URL(string: panel.contains("://") ? panel : "http://\(panel)"),
           let host = url.host {
            panel = host
        }
        guard !panel.isEmpty, !pairingCode.isEmpty, !verificationCode.isEmpty else { return nil }
        return CompanionPairingDetails(
            panelHost: panel,
            pairingCode: pairingCode.uppercased(),
            verificationCode: verificationCode.uppercased()
        )
    }
}

private struct CompanionSettings: View {
    @ObservedObject var store: CompanionStore
    @State private var pairingCode = ""
    @State private var verificationCode = ""
    @FocusState private var focusedField: CompanionSettingsField?

    var body: some View {
        TabView {
            ScrollView {
                GroupBox("Device connection") {
                    deviceConnectionSettings
                        .padding(8)
                }
                .padding()
            }
            .tabItem {
                Label("Device", systemImage: "display")
            }

            ScrollView {
                GroupBox("Supported apps") {
                    supportedAppsSettings
                        .padding(8)
                }
                .padding()
            }
            .tabItem {
                Label("Apps", systemImage: "square.grid.2x2")
            }
        }
        .onAppear {
            focusSettingsWindow {
                focusedField = .panelHost
            }
        }
        .task { store.refreshApplications() }
    }

    private var deviceConnectionSettings: some View {
        VStack(alignment: .leading, spacing: 14) {
            Button("Paste pairing details") {
                guard let clipboard = NSPasteboard.general.string(forType: .string),
                      let details = CompanionPairingDetails.parse(clipboard) else {
                    store.updateStatus("Copy pairing details from the panel web settings first")
                    return
                }
                store.panelHost = details.panelHost
                pairingCode = details.pairingCode
                verificationCode = details.verificationCode
                focusedField = nil
                store.updateStatus("Pairing details pasted — click Pair to continue")
            }
            .controlSize(.large)

            SettingsTextField(
                label: "Panel address",
                placeholder: "e.g. 192.168.6.100",
                text: $store.panelHost,
                field: .panelHost,
                focusedField: $focusedField
            )

            SettingsTextField(
                label: "Panel name",
                placeholder: "e.g. Kitchen display",
                text: $store.panelName,
                field: .panelName,
                focusedField: $focusedField
            )

            HStack(alignment: .bottom, spacing: 12) {
                SettingsTextField(
                    label: "Pairing code",
                    placeholder: "Eight-letter code",
                    text: $pairingCode,
                    field: .pairingCode,
                    focusedField: $focusedField
                )

                SettingsTextField(
                    label: "Verification code",
                    placeholder: "Verify code",
                    text: $verificationCode,
                    field: .verificationCode,
                    focusedField: $focusedField
                )

                Button("Pair") {
                    store.pair(code: pairingCode, verificationCode: verificationCode)
                }
                .controlSize(.large)
            }

            Text("In the panel web editor, open Settings → Companion, start pairing, then copy and paste the pairing details here.")
                .font(.caption)
                .foregroundStyle(.secondary)

            Divider()

            Toggle("Open EspControl Companion at Login", isOn: store.launchAtLoginBinding())
            Text(store.launchAtLoginMessage)
                .font(.caption)
                .foregroundStyle(.secondary)

            if store.hasSavedPairing {
                Button("Forget this panel", role: .destructive) { store.forgetPanel() }
            }
        }
    }

    private var supportedAppsSettings: some View {
        VStack(alignment: .leading, spacing: 10) {
            if store.availableApps.isEmpty {
                Text("No apps found in /Applications yet.").foregroundStyle(.secondary)
            } else {
                HStack(spacing: 10) {
                    Button("Enable All") { store.allowAllApplications() }
                        .disabled(store.allAvailableAppsAllowed)
                    Button("Disable All") { store.disallowAllApplications() }
                        .disabled(!store.hasAllowedApps)
                    Spacer()
                    Text("\(store.allowedAvailableAppCount) enabled")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }

                Divider()

                ForEach(store.availableApps) { app in
                    Toggle(app.name, isOn: store.allowedBinding(for: app))
                }
            }
            Button("Rescan Applications") { store.refreshApplications() }
        }
    }
}

private struct SettingsTextField: View {
    let label: String
    let placeholder: String
    @Binding var text: String
    let field: CompanionSettingsField
    @FocusState.Binding var focusedField: CompanionSettingsField?

    var body: some View {
        VStack(alignment: .leading, spacing: 5) {
            Text(label)
                .font(.caption)
                .foregroundStyle(.secondary)
            TextField(placeholder, text: $text)
                .textFieldStyle(.roundedBorder)
                .controlSize(.large)
                .focused($focusedField, equals: field)
        }
    }
}
