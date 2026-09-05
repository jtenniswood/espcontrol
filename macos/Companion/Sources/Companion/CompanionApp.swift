import AppKit
import Combine
import Darwin
import SwiftUI

@main
struct CompanionApp: App {
    @NSApplicationDelegateAdaptor(CompanionApplicationDelegate.self) private var appDelegate

    var body: some Scene {
        Settings {
            CompanionSettings(store: appDelegate.store)
                .frame(minWidth: 760, minHeight: 500)
        }
        .windowStyle(.hiddenTitleBar)
        .windowToolbarStyle(.unifiedCompact(showsTitle: false))
        .commands {
            CommandGroup(replacing: .appInfo) {
                Button("About EspControl Companion") {
                    NSApp.orderFrontStandardAboutPanel(nil)
                }
            }
            CommandGroup(replacing: .appSettings) {
                Button("Settings") { appDelegate.openCompanionWindow() }
                    .keyboardShortcut(",", modifiers: .command)
            }
        }
    }
}

@MainActor
final class CompanionApplicationDelegate: NSObject, NSApplicationDelegate, NSMenuDelegate {
    private static let openSettingsNotification = Notification.Name("io.espcontrol.companion.open-settings")
    let store = CompanionStore()
    private var statusItem: NSStatusItem?
    private var connectionObservation: AnyCancellable?
    private var instanceLockFileDescriptor: Int32 = -1
    private var settingsWindow: NSWindow?

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
            button.toolTip = "EspControl Companion"
        }
        item.menu = NSMenu()
        item.menu?.delegate = self
        updateStatusItemImage(connected: store.isConnected)
        connectionObservation = store.$isConnected
            .removeDuplicates()
            .sink { [weak self] connected in self?.updateStatusItemImage(connected: connected) }
        if store.hasSavedPairing && !store.panelHost.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
            store.connect()
        }
        if !launchedAsLoginItem() {
            DispatchQueue.main.async { [weak self] in self?.openCompanionWindow() }
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

    func applicationShouldHandleReopen(_ sender: NSApplication, hasVisibleWindows flag: Bool) -> Bool {
        openCompanionWindow()
        return true
    }

    func menuNeedsUpdate(_ menu: NSMenu) {
        menu.removeAllItems()
        menu.addItem(connectionStatusItem())
        menu.addItem(.separator())

        let panelWebpageItem = NSMenuItem(
            title: "Configure",
            action: #selector(openDisplaySettings),
            keyEquivalent: ""
        )
        panelWebpageItem.target = self
        panelWebpageItem.image = nil
        panelWebpageItem.isEnabled = !store.panelHost.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
        menu.addItem(panelWebpageItem)

        addMenuItem("Settings", action: #selector(openSettings), key: ",", to: menu)
        addMenuItem("Quit", action: #selector(quit), key: "q", to: menu)
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
        connectionSwitch.setAccessibilityLabel("Companion connector")

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

    private func addMenuItem(_ title: String, action: Selector, key: String = "", to menu: NSMenu) {
        let item = NSMenuItem(title: title, action: action, keyEquivalent: key)
        item.target = self
        item.image = nil
        menu.addItem(item)
    }

    @objc private func connectionSwitchChanged(_ sender: NSSwitch) {
        if sender.state == .on {
            store.connect()
        } else {
            store.disconnect()
        }
    }

    @objc private func openDisplaySettings() { store.openPanelWebServer() }

    private func updateStatusItemImage(connected: Bool) {
        guard let button = statusItem?.button else { return }
        let description = connected ? "EspControl Companion connected" : "EspControl Companion disconnected"
        let symbol = connected ? "laptopcomputer" : "laptopcomputer.slash"
        let image = NSImage(systemSymbolName: symbol, accessibilityDescription: description)
        image?.isTemplate = true
        button.image = image
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

    private func launchedAsLoginItem() -> Bool {
        NSAppleEventManager.shared().currentAppleEvent?
            .paramDescriptor(forKeyword: keyAELaunchedAsLogInItem)?
            .booleanValue == true
    }

    func openCompanionWindow() {
        activateCompanionApplication()

        if let settingsWindow {
            settingsWindow.orderFrontRegardless()
            settingsWindow.makeKeyAndOrderFront(nil)
            return
        }

        let window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 900, height: 600),
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false
        )
        window.identifier = NSUserInterfaceItemIdentifier("io.espcontrol.companion.settings")
        window.title = "EspControl Companion"
        window.titleVisibility = .hidden
        window.titlebarAppearsTransparent = true
        window.styleMask.insert(.fullSizeContentView)
        window.minSize = NSSize(width: 760, height: 500)
        window.isReleasedWhenClosed = false
        window.contentView = NSHostingView(rootView: CompanionSettings(store: store))
        window.center()
        settingsWindow = window
        window.makeKeyAndOrderFront(nil)
        activateCompanionApplication()
    }

}

@MainActor
private func activateCompanionApplication() {
    NSApp.setActivationPolicy(.regular)
    NSApp.unhide(nil)
    NSRunningApplication.current.activate(options: [
        .activateAllWindows,
        .activateIgnoringOtherApps,
    ])
    NSApp.activate(ignoringOtherApps: true)
}

private enum CompanionSettingsField: Hashable {
    case panelHost, pairingCode
}

private enum CompanionSettingsPage: String, CaseIterable, Identifiable {
    // Retain the saved selection identifiers from earlier versions.
    case connection, applications, folders, general

    var id: String { rawValue }
    var title: String {
        switch self {
        case .connection: return "Display"
        case .applications: return "Applications"
        case .folders: return "Folders"
        case .general: return "General"
        }
    }
    var icon: String {
        switch self {
        case .connection: return "display"
        case .applications: return "square.grid.2x2"
        case .folders: return "folder"
        case .general: return "gearshape"
        }
    }
}

private struct CompanionSettings: View {
    @ObservedObject var store: CompanionStore
    @State private var pairingCode = ""
    @State private var applicationSearch = ""
    @State private var confirmingForget = false
    @State private var folderToRemove: ApprovedFolder?
    @State private var accessibilityGranted = false
    @AppStorage("settings.selectedPage") private var selectedPageID = CompanionSettingsPage.connection.rawValue
    @FocusState private var focusedField: CompanionSettingsField?

    var body: some View {
        HStack(spacing: 0) {
            List(CompanionSettingsPage.allCases, selection: selectedPageBinding) { page in
                Label(page.title, systemImage: page.icon).tag(page)
            }
            .listStyle(.sidebar)
            .safeAreaInset(edge: .top, spacing: 0) {
                Color.clear.frame(height: 28)
            }
            .frame(minWidth: 180, idealWidth: 200, maxWidth: 240)

            Divider()

            detailView
                .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        }
        .onAppear {
            if !store.hasSavedPairing { selectedPageID = CompanionSettingsPage.connection.rawValue }
            refreshAccessibilityStatus()
        }
        .onReceive(NotificationCenter.default.publisher(for: NSApplication.didBecomeActiveNotification)) { _ in
            refreshAccessibilityStatus()
        }
        .onChange(of: store.isConnected) { connected in
            if connected { pairingCode = "" }
        }
        .alert("Forget this display?", isPresented: $confirmingForget) {
            Button("Cancel", role: .cancel) {}
            Button("Forget Display", role: .destructive) {
                store.forgetPanel()
                pairingCode = ""
                focusedField = .panelHost
            }
        } message: {
            Text("Your Mac will disconnect and remove its saved pairing. You’ll need the code from the device webpage to pair again. Your application and folder choices will be kept.")
        }
        .alert("Remove folder?", isPresented: Binding(
            get: { folderToRemove != nil },
            set: { if !$0 { folderToRemove = nil } }
        )) {
            Button("Cancel", role: .cancel) { folderToRemove = nil }
            Button("Remove Folder", role: .destructive) {
                if let folder = folderToRemove { store.removeFolder(folder) }
                folderToRemove = nil
            }
        } message: {
            Text("This folder will no longer be available to your display. The folder and its files will stay on your Mac.")
        }
    }

    @ViewBuilder private var detailView: some View {
        switch selectedPage {
        case .connection: connectionPage
        case .applications: applicationsPage
        case .folders: foldersPage
        case .general: generalPage
        }
    }

    private var selectedPage: CompanionSettingsPage {
        CompanionSettingsPage(rawValue: selectedPageID) ?? .connection
    }

    private var selectedPageBinding: Binding<CompanionSettingsPage> {
        Binding(get: { selectedPage }, set: { selectedPageID = $0.rawValue })
    }

    private var connectionPage: some View {
        Form {
            if store.hasSavedPairing {
                Section("Paired Display") {
                    LabeledContent("Address", value: store.panelHost)
                        .textSelection(.enabled)
                    connectionStatus
                    HStack {
                        if store.connectionState.isBusy {
                            Button("Cancel Connection") { store.disconnect() }
                        } else if store.isConnected {
                            Button("Disconnect") { store.disconnect() }
                        } else {
                            Button("Connect") { store.connect() }
                                .buttonStyle(.borderedProminent)
                        }
                        Spacer()
                        Button("Customize Display") { store.openPanelWebServer() }
                            .help("Open the display’s configuration in your browser")
                    }
                }
                Section {
                    Button("Forget Display…", role: .destructive) { confirmingForget = true }
                }
            } else {
                Section {
                    Text("Connect your Mac to an EspControl display to launch applications, open folders, and use Mac controls from its touchscreen.")
                    Label("Open the device webpage to start pairing and get its pairing code.", systemImage: "safari")
                        .foregroundStyle(.secondary)
                }
                Section("Pair Display") {
                    LabeledContent("Display address") {
                        TextField("IP address or name.local", text: $store.panelHost)
                            .textFieldStyle(.roundedBorder)
                            .frame(maxWidth: 260)
                            .accessibilityLabel("Display address")
                            .disabled(store.connectionState.isBusy)
                            .focused($focusedField, equals: .panelHost)
                            .onSubmit { focusedField = .pairingCode }
                    }
                    LabeledContent("Pairing code") {
                        TextField("ABCD-EFGH", text: $pairingCode)
                            .textFieldStyle(.roundedBorder)
                            .font(.system(.body, design: .monospaced))
                            .frame(maxWidth: 260)
                            .accessibilityLabel("Pairing code")
                            .disabled(store.connectionState.isBusy)
                            .focused($focusedField, equals: .pairingCode)
                            .onSubmit { pairDisplay() }
                    }
                    Text("Open the device webpage at its local address, start pairing, and enter the eight-letter code it shows. You can include or omit the hyphen. Both devices must be on the same local network.")
                        .font(.callout)
                        .foregroundStyle(.secondary)
                    HStack {
                        if store.connectionState.isBusy {
                            Button("Cancel") { store.disconnect() }
                        }
                        Spacer()
                        Button("Pair Display") { pairDisplay() }
                            .buttonStyle(.borderedProminent)
                            .disabled(!canPair)
                    }
                    connectionStatus
                }
            }
        }
        .formStyle(.grouped)
        .navigationTitle("Display")
    }

    private var connectionStatus: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack(spacing: 8) {
                if store.connectionState.isBusy {
                    ProgressView().controlSize(.small)
                } else {
                    Image(systemName: store.connectionState.symbol)
                        .foregroundStyle(store.connectionState == .failed ? Color.orange : Color.secondary)
                }
                Text(store.connectionState.title)
            }
            .accessibilityElement(children: .combine)
            if store.connectionState == .failed || store.connectionState == .reconnecting {
                Text(store.connectionState == .reconnecting
                     ? "Check that your display is powered on and connected to the same network. Companion will try again automatically."
                     : store.connectionRecoveryMessage)
                    .font(.callout)
                    .foregroundStyle(.secondary)
            }
            if !store.statusDescription.isEmpty {
                DisclosureGroup("Connection Details") {
                    Text(store.statusDescription)
                        .font(.callout)
                        .foregroundStyle(.secondary)
                        .textSelection(.enabled)
                }
            }
        }
    }

    private var canPair: Bool {
        !store.connectionState.isBusy && CompanionPairingInput.isValid(host: store.panelHost, code: pairingCode)
    }

    private func pairDisplay() {
        guard canPair else { return }
        store.pair(code: pairingCode)
    }

    private var applicationsPage: some View {
        Form {
            Section {
                Text("Choose which applications are available on your display.")
                Text("\(store.launchableApps().count) of \(store.availableApps.count) enabled")
                    .font(.callout)
                    .foregroundStyle(.secondary)
            }
            Section {
                if store.availableApps.isEmpty {
                    emptyState("No Applications Found", symbol: "app.dashed",
                               detail: "Install applications in your Applications folder, then refresh this list.")
                    Button("Refresh Applications") { store.refreshApplications() }
                } else if filteredApplications.isEmpty {
                    emptyState("No Results", symbol: "magnifyingglass",
                               detail: "Try another application name or clear your search.")
                    Button("Clear Search") { applicationSearch = "" }
                } else {
                    ForEach(filteredApplications) { application in
                        Toggle(isOn: Binding(
                            get: { store.applicationIsApproved(application) },
                            set: { store.setApplication(application, approved: $0) }
                        )) {
                            HStack(spacing: 10) {
                                Image(nsImage: NSWorkspace.shared.icon(forFile: application.url.path))
                                    .resizable()
                                    .frame(width: 24, height: 24)
                                    .accessibilityHidden(true)
                                Text(application.name)
                            }
                        }
                        .toggleStyle(.checkbox)
                        .padding(.vertical, 2)
                    }
                }
            }
        }
        .formStyle(.grouped)
        .searchable(text: $applicationSearch, placement: .toolbar, prompt: "Search Applications")
        .navigationTitle("Applications")
        .toolbar {
            ToolbarItemGroup {
                Menu {
                    Button(isSearching ? "Enable All Results" : "Enable All Applications") {
                        store.setApplications(filteredApplications, approved: true)
                    }
                    .disabled(filteredApplications.isEmpty || filteredApplications.allSatisfy(store.applicationIsApproved))
                    Button(isSearching ? "Disable All Results" : "Disable All Applications") {
                        store.setApplications(filteredApplications, approved: false)
                    }
                    .disabled(!filteredApplications.contains(where: store.applicationIsApproved))
                } label: {
                    Label("Application Actions", systemImage: "ellipsis.circle")
                }
                .help("Enable or disable the applications shown")
                Button { store.refreshApplications() } label: {
                    Label("Refresh Applications", systemImage: "arrow.clockwise")
                }
                .help("Refresh installed applications")
            }
        }
    }

    private var isSearching: Bool { !applicationSearch.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty }
    private var filteredApplications: [LaunchableApp] {
        let query = applicationSearch.trimmingCharacters(in: .whitespacesAndNewlines)
        return query.isEmpty ? store.availableApps : store.availableApps.filter { $0.name.localizedCaseInsensitiveContains(query) }
    }

    private var foldersPage: some View {
        Form {
            Section {
                Text("Add folders you want to open from your display. Their paths stay on this Mac.")
            }
            Section {
                if store.approvedFolders.isEmpty {
                    emptyState("Add Your First Folder", symbol: "folder.badge.plus",
                               detail: "Keep a project, documents, or downloads one tap away on your display.")
                    Button("Add Folder…") { store.chooseFolder() }
                        .buttonStyle(.borderedProminent)
                } else {
                    ForEach(store.approvedFolders) { folder in
                        HStack(spacing: 10) {
                            Image(systemName: "folder")
                                .foregroundStyle(.secondary)
                                .accessibilityHidden(true)
                            VStack(alignment: .leading, spacing: 4) {
                                Text(folder.name)
                                Text(folder.path)
                                    .font(.caption)
                                    .foregroundStyle(.secondary)
                                    .lineLimit(1)
                                    .truncationMode(.middle)
                                    .help(folder.path)
                                if store.folderNeedsAccess(folder) {
                                    Label("Access needed", systemImage: "exclamationmark.triangle")
                                        .font(.callout)
                                    Button("Restore Access…") { store.chooseFolder(restoring: folder) }
                                }
                            }
                            Spacer()
                            Button(role: .destructive) { folderToRemove = folder } label: {
                                Image(systemName: "minus.circle")
                            }
                            .buttonStyle(.borderless)
                            .accessibilityLabel("Remove \(folder.name)")
                            .help("Remove folder from your display")
                        }
                        .padding(.vertical, 2)
                    }
                }
                if let message = store.folderMessage {
                    Text(message).font(.callout).foregroundStyle(.secondary)
                }
            }
        }
        .formStyle(.grouped)
        .navigationTitle("Folders")
        .toolbar {
            if !store.approvedFolders.isEmpty {
                ToolbarItem(placement: .primaryAction) {
                    Button { store.chooseFolder() } label: {
                        Label("Add Folder…", systemImage: "folder.badge.plus")
                    }
                    .help("Add a folder to your display")
                }
            }
        }
    }

    private func emptyState(_ title: String, symbol: String, detail: String) -> some View {
        VStack(alignment: .leading, spacing: 8) {
            Label(title, systemImage: symbol).font(.headline)
            Text(detail).foregroundStyle(.secondary)
        }
        .padding(.vertical, 8)
    }

    private var generalPage: some View {
        Form {
            Section("Startup") {
                if store.supportsLaunchAtLogin {
                    Toggle("Open EspControl Companion at Login", isOn: store.launchAtLoginBinding())
                    Text(store.launchAtLoginMessage).font(.callout).foregroundStyle(.secondary)
                } else {
                    Text("Install EspControl Companion in Applications to open it automatically at login.")
                        .foregroundStyle(.secondary)
                }
            }
            Section("Privacy") {
                Toggle("Share Mac system statistics", isOn: $store.shareSystemMetricsEnabled)
                Text("Share processor, memory, storage, network, and battery statistics only with your paired display on the local network.")
                    .font(.callout).foregroundStyle(.secondary)
            }
#if !APP_STORE
            Section("Keyboard & Window Controls") {
                Label(accessibilityGranted ? "Accessibility access enabled" : "Accessibility access needed",
                      systemImage: accessibilityGranted ? "checkmark.circle" : "lock")
                Text("Allow Accessibility access to use keyboard shortcuts and window controls from your display. Other features work without it.")
                    .font(.callout).foregroundStyle(.secondary)
                Button("Open System Settings…") {
                    _ = CompanionAccessibilityAuthorizer.shared.isTrusted()
                    if let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility") {
                        NSWorkspace.shared.open(url)
                    }
                }
            }
#endif
            Section("Help") {
                Link("EspControl Support", destination: CompanionStore.supportURL)
                Link("Privacy Policy", destination: CompanionStore.privacyPolicyURL)
            }
        }
        .formStyle(.grouped)
        .navigationTitle("General")
    }

    private func refreshAccessibilityStatus() {
#if !APP_STORE
        accessibilityGranted = CompanionAccessibilityAuthorizer.shared.hasAccess
#endif
    }
}
