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
final class CompanionApplicationDelegate: NSObject, NSApplicationDelegate, NSWindowDelegate {
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
        updateStatusItemImage(connected: store.isConnected)
        connectionObservation = store.$isConnected
            .removeDuplicates()
            .sink { [weak self] connected in self?.updateStatusItemImage(connected: connected) }
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

    func windowWillClose(_ notification: Notification) {
        guard let window = notification.object as? NSWindow,
              window === settingsWindow else { return }
        DispatchQueue.main.async {
            guard self.settingsWindow?.isVisible != true else { return }
            NSApp.setActivationPolicy(.accessory)
        }
    }

    @objc private func statusItemClicked(_ sender: NSStatusBarButton) {
        guard let statusItem else { return }
        // Temporarily attach the menu to the status item so AppKit positions it
        // directly below the menu-bar icon instead of at the pointer location.
        statusItem.menu = contextMenu()
        sender.performClick(nil)
        statusItem.menu = nil
    }

    private func contextMenu() -> NSMenu {
        let menu = NSMenu()
        menu.addItem(connectionStatusItem())
        menu.addItem(.separator())

        let settingsItem = NSMenuItem(title: "Settings…", action: #selector(openSettings), keyEquivalent: ",")
        settingsItem.target = self
        settingsItem.image = nil
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

    private func updateStatusItemImage(connected: Bool) {
        guard let button = statusItem?.button else { return }
        let description = connected ? "EspControl Companion connected" : "EspControl Companion disconnected"
        let symbol = connected ? "laptopcomputer" : "laptopcomputer.slash"
        let image = NSImage(systemSymbolName: symbol, accessibilityDescription: description)
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
                .frame(width: 980, height: 620)
            let controller = NSHostingController(rootView: content)
            let window = NSWindow(
                contentRect: NSRect(x: 0, y: 0, width: 980, height: 620),
                styleMask: [.titled, .closable, .miniaturizable, .resizable],
                backing: .buffered,
                defer: false
            )
            window.title = "EspControl Companion Settings"
            window.titleVisibility = .hidden
            window.titlebarAppearsTransparent = true
            window.isMovableByWindowBackground = true
            window.contentMinSize = NSSize(width: 760, height: 500)
            window.contentViewController = controller
            window.delegate = self
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
    case panelHost, pairingCode
}

private enum CompanionSettingsPage: String, CaseIterable, Identifiable {
    case about
    case connection
    case applications
    case folders
    case nowPlaying
    case general

    var id: String { rawValue }

    var title: String {
        switch self {
        case .about: return "About EspControl"
        case .connection: return "Device"
        case .applications: return "Applications"
        case .folders: return "Folders"
        case .nowPlaying: return "Now Playing"
        case .general: return "General"
        }
    }

    var icon: String {
        switch self {
        case .about: return "info.circle"
        case .connection: return "network"
        case .applications: return "app.badge.checkmark"
        case .folders: return "folder"
        case .nowPlaying: return "music.note"
        case .general: return "gearshape"
        }
    }
}

private struct CompanionSettings: View {
    @ObservedObject var store: CompanionStore
    @State private var pairingCode = ""
    @State private var applicationSearch = ""
    @State private var selectedPage: CompanionSettingsPage = .about
    @FocusState private var focusedField: CompanionSettingsField?

    var body: some View {
        HStack(spacing: 0) {
            settingsSidebar
            Rectangle()
                .fill(Color.primary.opacity(0.08))
                .frame(width: 1)
            detailView
        }
        .background(Color(nsColor: .windowBackgroundColor))
        .onAppear {
            focusSettingsWindow()
        }
        .task { store.refreshApplications() }
        .onChange(of: store.isConnected) { connected in
            if connected { pairingCode = "" }
        }
    }

    private var settingsSidebar: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text("Settings")
                .font(.system(size: 28, weight: .semibold))
                .padding(.horizontal, 24)
                .padding(.top, 28)
                .padding(.bottom, 26)

            VStack(spacing: 4) {
                ForEach(CompanionSettingsPage.allCases) { page in
                    Button {
                        selectedPage = page
                    } label: {
                        HStack(spacing: 12) {
                            Image(systemName: page.icon)
                                .font(.system(size: 17, weight: .medium))
                                .frame(width: 22)
                            Text(page.title)
                                .font(.system(size: 15, weight: .medium))
                            Spacer(minLength: 0)
                        }
                        .foregroundStyle(.primary)
                        .padding(.horizontal, 14)
                        .frame(height: 44)
                        .background(
                            RoundedRectangle(cornerRadius: 10, style: .continuous)
                                .fill(selectedPage == page ? Color.primary.opacity(0.10) : .clear)
                        )
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(.horizontal, 12)

            Spacer()
        }
        .frame(minWidth: 236, maxWidth: 236, maxHeight: .infinity, alignment: .top)
        .background(Color(nsColor: .underPageBackgroundColor))
    }

    @ViewBuilder
    private var detailView: some View {
        switch selectedPage {
        case .about:
            settingsPage(title: "About EspControl", subtitle: "Connection status and quick access to your display") {
                connectionStatusPanel
                if hasPanelAddress {
                    deviceWebserverPanel
                }
            }
        case .connection:
            settingsPage(title: "Device", subtitle: "Pair EspControl Companion with a display") {
                settingsSection("Connection") {
                    deviceConnectionSettings
                }
            }
        case .applications:
            settingsPage(title: "Applications", subtitle: "Choose the Mac apps your display may launch or control") {
                applicationSettings
            }
        case .folders:
            settingsPage(title: "Folders", subtitle: "Choose the folders that can be opened from your display") {
                folderSettings
            }
        case .nowPlaying:
            settingsPage(title: "Now Playing", subtitle: "Share the media currently playing on your Mac") {
                settingsSection("Now Playing") {
                    nowPlayingSettings
                }
            }
        case .general:
            settingsPage(title: "General", subtitle: "Manage optional macOS integration") {
                if store.supportsLaunchAtLogin {
                    settingsSection("Startup") {
                        startupSettings
                    }
                }
            }
        }
    }

    @ViewBuilder
    private func settingsPage<Content: View>(
        title: String,
        subtitle: String,
        @ViewBuilder content: () -> Content
    ) -> some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 0) {
                VStack(alignment: .leading, spacing: 7) {
                    Text(title)
                        .font(.system(size: 25, weight: .semibold))
                    Text(subtitle)
                        .font(.system(size: 14))
                        .foregroundStyle(.secondary)
                }
                .padding(.bottom, 22)

                Divider()
                    .padding(.bottom, 24)

                VStack(alignment: .leading, spacing: 22) {
                    content()
                }
            }
            .padding(.horizontal, 42)
            .padding(.top, 32)
            .padding(.bottom, 32)
            .frame(maxWidth: 900, alignment: .leading)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        .background(Color(nsColor: .windowBackgroundColor))
    }

    @ViewBuilder
    private func settingsSection<Content: View>(
        _ title: String,
        @ViewBuilder content: () -> Content
    ) -> some View {
        VStack(alignment: .leading, spacing: 14) {
            Text(title)
                .font(.system(size: 17, weight: .medium))
                .foregroundStyle(.secondary)
            content()
                .padding(.top, 2)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private var deviceConnectionSettings: some View {
        VStack(alignment: .leading, spacing: 18) {
            SettingsActionRow(
                title: "Panel address",
                description: "The local network address of your EspControl display."
            ) {
                TextField("e.g. 192.168.6.100", text: $store.panelHost)
                    .textFieldStyle(.roundedBorder)
                    .controlSize(.large)
                    .focused($focusedField, equals: .panelHost)
                    .frame(width: 240)
            }

            SettingsActionRow(
                title: "Pairing code",
                description: "The eight-letter code shown while pairing is active."
            ) {
                HStack(spacing: 8) {
                    TextField("Eight-letter code", text: $pairingCode)
                        .textFieldStyle(.roundedBorder)
                        .controlSize(.large)
                        .focused($focusedField, equals: .pairingCode)
                        .frame(width: 160)
                    Button("Pair") {
                        store.pair(code: pairingCode)
                    }
                    .controlSize(.large)
                }
            }

            Text("Press and hold the Wi-Fi icon on the display, then enter its local address and the code shown on the touchscreen.")
                .font(.caption)
                .foregroundStyle(.secondary)

            if store.hasSavedPairing {
                Button("Forget this panel", role: .destructive) {
                    store.forgetPanel()
                    pairingCode = ""
                    focusedField = .panelHost
                }
                .controlSize(.large)
            }
        }
    }

    private var applicationSettings: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Only selected applications are shared with the display.")
                .font(.caption)
                .foregroundStyle(.secondary)

            TextField("Search applications", text: $applicationSearch)
                .textFieldStyle(.roundedBorder)
                .frame(maxWidth: 360)

            HStack(spacing: 16) {
                Button("Select All") {
                    store.setApplications(filteredApplications, approved: true)
                }
                .disabled(filteredApplications.isEmpty || allFilteredApplicationsApproved)
                Button("Deselect All") {
                    store.setApplications(filteredApplications, approved: false)
                }
                .disabled(!hasApprovedFilteredApplications)
            }
            .buttonStyle(.borderless)
            .padding(.bottom, 2)

            if store.availableApps.isEmpty {
                Text("No applications were found.")
                    .foregroundStyle(.secondary)
                    .padding(.vertical, 8)
            } else if filteredApplications.isEmpty {
                Text("No applications match your search.")
                    .foregroundStyle(.secondary)
                    .padding(.vertical, 8)
            } else {
                ForEach(filteredApplications) { application in
                    Toggle(isOn: Binding(
                        get: { store.applicationIsApproved(application) },
                        set: { store.setApplication(application, approved: $0) }
                    )) {
                        Text(application.name)
                    }
                    Divider()
                }
            }

            Button("Refresh Applications") { store.refreshApplications() }
                .controlSize(.large)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private var filteredApplications: [LaunchableApp] {
        let query = applicationSearch.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !query.isEmpty else { return store.availableApps }
        return store.availableApps.filter {
            $0.name.localizedCaseInsensitiveContains(query)
        }
    }

    private var allFilteredApplicationsApproved: Bool {
        !filteredApplications.isEmpty && filteredApplications.allSatisfy(store.applicationIsApproved)
    }

    private var hasApprovedFilteredApplications: Bool {
        filteredApplications.contains(where: store.applicationIsApproved)
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

    private var folderSettings: some View {
        VStack(alignment: .leading, spacing: 12) {
            if store.approvedFolders.isEmpty {
                Text("No folders have been added.")
                    .foregroundStyle(.secondary)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(.vertical, 8)
            } else {
                ForEach(store.approvedFolders) { folder in
                    HStack(spacing: 12) {
                        Image(systemName: "folder.fill")
                            .foregroundStyle(.secondary)
                        VStack(alignment: .leading, spacing: 2) {
                            Text(folder.name).font(.headline)
                            Text(folder.needsReapproval ? "Select this folder again to restore access" : folder.path)
                                .font(.caption)
                                .foregroundStyle(folder.needsReapproval ? .orange : .secondary)
                                .lineLimit(1)
                                .truncationMode(.middle)
                        }
                        Spacer()
                        Button(role: .destructive) {
                            store.removeFolder(folder)
                        } label: {
                            Image(systemName: "minus.circle")
                        }
                        .buttonStyle(.borderless)
                        .help("Remove folder")
                    }
                    Divider()
                }
            }

            Button("Add Folder…") { store.chooseFolder() }
                .controlSize(.large)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private var nowPlayingSettings: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Shares the active session shown by macOS Control Centre automatically. This uses a private macOS system interface and may need an EspControl update after a future macOS release. No additional macOS permission is required.")
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

    private var aboutSettings: some View {
        VStack(alignment: .leading, spacing: 8) {
            Link("Privacy Policy", destination: CompanionStore.privacyPolicyURL)
            Link("EspControl support", destination: CompanionStore.supportURL)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private var connectionStatusPanel: some View {
        SettingsCard {
            SettingsActionRow(
                title: "EspControl Companion",
                description: store.isConnected ? "Connected to your display" : "Not connected to a display"
            ) {
                if canConnect || store.isConnected {
                    Toggle("Connect EspControl Companion", isOn: connectionBinding)
                        .labelsHidden()
                        .toggleStyle(.switch)
                } else {
                    Text("Pair a device")
                        .font(.callout)
                        .foregroundStyle(.secondary)
                }
            }
        }
    }

    private var deviceWebserverPanel: some View {
        SettingsCard {
            SettingsActionRow(
                title: "Device Webserver",
                description: "Open the display settings in your browser."
            ) {
                Button("Open Device Webserver") { store.openPanelWebServer() }
                    .controlSize(.large)
            }
        }
    }

    private var connectionBinding: Binding<Bool> {
        Binding(
            get: { store.isConnected },
            set: { connected in
                if connected {
                    store.connect()
                } else {
                    store.disconnect()
                }
            }
        )
    }

    private var hasPanelAddress: Bool {
        !store.panelHost.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
    }

    private var canConnect: Bool {
        hasPanelAddress && store.hasSavedPairing
    }

}

private struct SettingsCard<Content: View>: View {
    @ViewBuilder let content: Content

    var body: some View {
        content
            .padding(.horizontal, 18)
            .padding(.vertical, 5)
            .background(
                RoundedRectangle(cornerRadius: 10, style: .continuous)
                    .fill(Color(nsColor: .controlBackgroundColor).opacity(0.55))
            )
            .overlay(
                RoundedRectangle(cornerRadius: 10, style: .continuous)
                    .stroke(Color.primary.opacity(0.08), lineWidth: 1)
            )
    }
}

private struct SettingsActionRow<Control: View>: View {
    let title: String
    let description: String
    @ViewBuilder let control: Control

    var body: some View {
        HStack(alignment: .center, spacing: 24) {
            VStack(alignment: .leading, spacing: 4) {
                Text(title)
                    .font(.system(size: 15, weight: .medium))
                Text(description)
                    .font(.system(size: 13))
                    .foregroundStyle(.secondary)
                    .fixedSize(horizontal: false, vertical: true)
            }

            Spacer(minLength: 12)

            control
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }
}
