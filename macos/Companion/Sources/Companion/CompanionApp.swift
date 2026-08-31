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
        guard let event = NSApp.currentEvent else { return }
        NSMenu.popUpContextMenu(contextMenu(), with: event, for: sender)
    }

    private func contextMenu() -> NSMenu {
        let menu = NSMenu()
        menu.addItem(connectionStatusItem())
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

    private func connectionStatusItem() -> NSMenuItem {
        let item = NSMenuItem()
        let container = NSView(frame: NSRect(x: 0, y: 0, width: 300, height: 58))

        let title = NSTextField(labelWithString: "EspControl Companion")
        title.font = .systemFont(ofSize: 14, weight: .semibold)

        let status = NSTextField(labelWithString: store.isConnected ? "Connected" : "Disconnected")
        status.font = .systemFont(ofSize: 13)
        status.textColor = .secondaryLabelColor

        let labels = NSStackView(views: [title, status])
        labels.orientation = .vertical
        labels.alignment = .leading
        labels.spacing = 1

        let connectionSwitch = NSSwitch()
        connectionSwitch.state = store.isConnected ? .on : .off
        connectionSwitch.target = self
        connectionSwitch.action = #selector(connectionSwitchChanged(_:))
        connectionSwitch.toolTip = store.isConnected ? "Disconnect from the display" : "Connect to the display"
        connectionSwitch.isEnabled = store.isConnected
            || !store.panelHost.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty

        container.addSubview(labels)
        container.addSubview(connectionSwitch)
        labels.translatesAutoresizingMaskIntoConstraints = false
        connectionSwitch.translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            labels.leadingAnchor.constraint(equalTo: container.leadingAnchor, constant: 14),
            labels.centerYAnchor.constraint(equalTo: container.centerYAnchor),
            labels.trailingAnchor.constraint(lessThanOrEqualTo: connectionSwitch.leadingAnchor, constant: -12),
            connectionSwitch.trailingAnchor.constraint(equalTo: container.trailingAnchor, constant: -14),
            connectionSwitch.centerYAnchor.constraint(equalTo: container.centerYAnchor),
        ])

        item.view = container
        return item
    }

    private func updateStatusItemImage() {
        guard let button = statusItem?.button else { return }
        let description = store.isConnected ? "EspControl Companion connected" : "EspControl Companion disconnected"
        let image = NSImage(systemSymbolName: store.connectionSymbol, accessibilityDescription: description)
        image?.isTemplate = true
        button.image = image
    }

    @objc private func connectionSwitchChanged(_ sender: NSSwitch) {
        if sender.state == .on {
            store.connect()
        } else {
            store.disconnect()
        }
    }

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
    case panelHost, panelName, pairingCode
}

private struct CompanionPairingDetails {
    let panelHost: String
    let pairingCode: String

    static func parse(_ text: String) -> CompanionPairingDetails? {
        var values: [String: String] = [:]
        for line in text.components(separatedBy: .newlines) {
            let parts = line.split(separator: ":", maxSplits: 1).map {
                $0.trimmingCharacters(in: .whitespacesAndNewlines)
            }
            if parts.count == 2 { values[parts[0].lowercased()] = parts[1] }
        }
        guard var panel = values["panel"],
              let pairingCode = values["pairing code"] else { return nil }
        if let url = URL(string: panel.contains("://") ? panel : "http://\(panel)"),
           let host = url.host {
            panel = host
        }
        guard !panel.isEmpty, !pairingCode.isEmpty else { return nil }
        return CompanionPairingDetails(
            panelHost: panel,
            pairingCode: pairingCode.uppercased()
        )
    }
}

private struct CompanionSettings: View {
    @ObservedObject var store: CompanionStore
    @State private var pairingCode = ""
    @FocusState private var focusedField: CompanionSettingsField?

    var body: some View {
        TabView {
            ScrollView {
                GroupBox("Device connection") {
                    connectionStatusPanel
                        .padding(8)
                }
                .padding()
            }
            .tabItem {
                Label("Status", systemImage: "dot.radiowaves.left.and.right")
            }

            ScrollView {
                GroupBox("Connection settings") {
                    deviceConnectionSettings
                        .padding(8)
                }
                .padding()
            }
            .tabItem {
                Label("Connection", systemImage: "network")
            }

            ScrollView {
                GroupBox("Mac Now Playing") {
                    nowPlayingSettings.padding(8)
                }
                .padding()
            }
            .tabItem {
                Label("Now Playing", systemImage: "music.note")
            }

            ScrollView {
                GroupBox("Startup") {
                    startupSettings
                        .padding(8)
                }
                .padding()
            }
            .tabItem {
                Label("General", systemImage: "gearshape")
            }
        }
        .onAppear {
            focusSettingsWindow()
        }
        .task { store.refreshApplications() }
        .onChange(of: store.isConnected) { connected in
            if connected { pairingCode = "" }
        }
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
                focusedField = nil
                store.pair(code: details.pairingCode)
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

                Button("Pair") {
                    store.pair(code: pairingCode)
                }
                .controlSize(.large)
            }

            Text("In the panel web editor, open Settings → Companion, start pairing, then copy and paste the pairing details here.")
                .font(.caption)
                .foregroundStyle(.secondary)

            if store.hasSavedPairing {
                Button("Forget this panel", role: .destructive) {
                    store.forgetPanel()
                    pairingCode = ""
                    focusedField = .panelHost
                }
            }
        }
    }

    private var startupSettings: some View {
        VStack(alignment: .leading, spacing: 8) {
            Toggle("Open EspControl Companion at Login", isOn: store.launchAtLoginBinding())
            Text(store.launchAtLoginMessage)
                .font(.caption)
                .foregroundStyle(.secondary)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private var nowPlayingSettings: some View {
        VStack(alignment: .leading, spacing: 12) {
            Toggle("Share Mac Now Playing with the display", isOn: $store.nowPlayingSharingEnabled)
            Text("Shares the active session shown by macOS Control Centre. This uses a private macOS system interface and may need an EspControl update after a future macOS release.")
                .font(.caption)
                .foregroundStyle(.secondary)
            HStack(spacing: 12) {
                if let artwork = store.nowPlayingArtwork {
                    Image(nsImage: artwork).resizable().scaledToFit().frame(width: 72, height: 72)
                        .background(Color.black).clipShape(RoundedRectangle(cornerRadius: 8))
                } else {
                    Image(systemName: "music.note").frame(width: 72, height: 72)
                        .background(Color.secondary.opacity(0.12)).clipShape(RoundedRectangle(cornerRadius: 8))
                }
                VStack(alignment: .leading, spacing: 3) {
                    if !store.nowPlayingTitle.isEmpty { Text(store.nowPlayingTitle).font(.headline) }
                    if !store.nowPlayingApplication.isEmpty { Text(store.nowPlayingApplication).foregroundStyle(.secondary) }
                    Text(store.nowPlayingStatus).font(.caption).foregroundStyle(.secondary)
                }
            }
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private var connectionStatusPanel: some View {
        HStack(spacing: 12) {
            Image(systemName: store.isConnected ? "checkmark.circle.fill" : "circle.dashed")
                .font(.title2)
                .foregroundStyle(store.isConnected ? Color.green : Color.secondary)

            VStack(alignment: .leading, spacing: 3) {
                Text(store.isConnected ? "Connected to \(connectionDisplayName)" : "Mac Companion is not connected")
                    .font(.headline)
                Text(store.statusDescription)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            VStack(alignment: .trailing, spacing: 8) {
                if canConnect {
                    Button(store.isConnected ? "Disconnect" : "Connect") {
                        if store.isConnected {
                            store.disconnect()
                        } else {
                            store.connect()
                        }
                    }
                    .controlSize(.large)
                }

                if hasPanelAddress {
                    Button("Open Device Webserver") { store.openPanelWebServer() }
                        .controlSize(.large)
                }
            }
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

    private var hasPanelAddress: Bool {
        !store.panelHost.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
    }

    private var canConnect: Bool {
        hasPanelAddress && store.hasSavedPairing
    }

    private var connectionDisplayName: String {
        let name = store.panelName.trimmingCharacters(in: .whitespacesAndNewlines)
        return name.isEmpty ? store.panelHost : name
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
