import AppKit
import Combine
import Darwin
import SwiftUI

@main
struct CompanionApp: App {
    @NSApplicationDelegateAdaptor(CompanionApplicationDelegate.self) private var appDelegate

    var body: some Scene {
        Settings {
            EmptyView()
        }
        .commands {
            CommandGroup(replacing: .appSettings) {
                Button("Settings…") { appDelegate.openCompanionWindow() }
                    .keyboardShortcut(",", modifiers: .command)
            }
        }
    }
}

@MainActor
final class CompanionApplicationDelegate: NSObject, NSApplicationDelegate {
    private static let openSettingsNotification = Notification.Name("io.espcontrol.companion.open-settings")
    let store = CompanionStore()
    private var statusItem: NSStatusItem?
    private var settingsWindow: NSWindow?
    private var connectionObservation: AnyCancellable?
    private var instanceLockFileDescriptor: Int32 = -1

    func applicationDidFinishLaunching(_ notification: Notification) {
        guard acquireInstanceLock() else {
            DistributedNotificationCenter.default().postNotificationName(
                Self.openSettingsNotification,
                object: nil,
                userInfo: nil,
                deliverImmediately: true
            )
            NSApp.terminate(nil)
            return
        }
        DistributedNotificationCenter.default().addObserver(
            self,
            selector: #selector(existingInstanceWasOpened),
            name: Self.openSettingsNotification,
            object: nil
        )
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
        if store.hasSavedPairing && !store.panelHost.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
            store.connect()
        }
    }

    func applicationWillTerminate(_ notification: Notification) {
        DistributedNotificationCenter.default().removeObserver(self)
        if instanceLockFileDescriptor >= 0 {
            Darwin.lockf(instanceLockFileDescriptor, F_ULOCK, 0)
            Darwin.close(instanceLockFileDescriptor)
            instanceLockFileDescriptor = -1
        }
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
    @objc private func existingInstanceWasOpened(_ notification: Notification) { openCompanionWindow() }

    private func acquireInstanceLock() -> Bool {
        let lockURL = FileManager.default.temporaryDirectory
            .appendingPathComponent("io.espcontrol.companion.instance.lock")
        let descriptor = Darwin.open(lockURL.path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)
        guard descriptor >= 0 else { return false }
        guard Darwin.lockf(descriptor, F_TLOCK, 0) == 0 else {
            Darwin.close(descriptor)
            return false
        }
        instanceLockFileDescriptor = descriptor
        return true
    }

    func openCompanionWindow() {
        activateCompanionApplication()
        if settingsWindow == nil {
            let content = CompanionSettings(store: store)
                .frame(width: 560, height: 500)
            let controller = NSHostingController(rootView: content)
            let window = NSWindow(
                contentRect: NSRect(x: 0, y: 0, width: 560, height: 500),
                styleMask: [.titled, .closable, .miniaturizable, .resizable],
                backing: .buffered,
                defer: false
            )
            window.title = "EspControl Companion Settings"
            window.contentMinSize = NSSize(width: 520, height: 420)
            window.contentViewController = controller
            window.isReleasedWhenClosed = false
            window.center()
            settingsWindow = window
        }
        settingsWindow?.level = .normal
        settingsWindow?.makeKeyAndOrderFront(nil)
        settingsWindow?.orderFrontRegardless()
        activateCompanionApplication()
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
        ScrollView {
            GroupBox("Device connection") {
                deviceConnectionSettings
                    .padding(8)
            }
            .padding()
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
            connectionStatusPanel

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

    private var connectionStatusPanel: some View {
        HStack(spacing: 12) {
            Image(systemName: store.isConnected ? "checkmark.circle.fill" : "circle.dashed")
                .font(.title2)
                .foregroundStyle(store.isConnected ? Color.green : Color.secondary)

            VStack(alignment: .leading, spacing: 3) {
                Text(store.isConnected ? "Connected to \(store.panelName)" : "Mac Companion is not connected")
                    .font(.headline)
                Text(store.statusDescription)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            VStack(alignment: .trailing, spacing: 8) {
                Button(store.isConnected ? "Reconnect" : "Connect") { store.connect() }
                    .controlSize(.large)

                Button("Open Device Webserver") { store.openPanelWebServer() }
                    .controlSize(.large)
            }
            .disabled(store.panelHost.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty)
        }
        .padding(12)
        .background(
            RoundedRectangle(cornerRadius: 10)
                .fill(Color(nsColor: .controlBackgroundColor))
        )
        .overlay(
            RoundedRectangle(cornerRadius: 10)
                .stroke(store.isConnected ? Color.green.opacity(0.55) : Color.secondary.opacity(0.25), lineWidth: 1)
        )
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
