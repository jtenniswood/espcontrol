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
            button.toolTip = "EspControl Companion"
        }
        item.menu = contextMenu()
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
        return false
    }

    func windowWillClose(_ notification: Notification) {
        guard let window = notification.object as? NSWindow,
              window === settingsWindow else { return }
        DispatchQueue.main.async {
            guard self.settingsWindow?.isVisible != true else { return }
            NSApp.setActivationPolicy(.accessory)
        }
    }

    private func contextMenu() -> NSMenu {
        let menu = NSMenu()
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
        if settingsWindow == nil {
            let content = CompanionSettings(store: store)
                .frame(minWidth: 760, minHeight: 500)
            let controller = NSHostingController(rootView: content)
            let window = NSWindow(
                contentRect: NSRect(x: 0, y: 0, width: 980, height: 620),
                styleMask: [.titled, .closable, .miniaturizable, .resizable],
                backing: .buffered,
                defer: false
            )
            window.title = "EspControl Companion"
            window.titleVisibility = .visible
            window.titlebarAppearsTransparent = true
            window.titlebarSeparatorStyle = .none
            window.toolbarStyle = .unified
            window.isMovableByWindowBackground = false
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
    case about
    case connection
    case applications
    case folders
    case general

    var id: String { rawValue }

    var title: String {
        switch self {
        case .about: return "About EspControl"
        case .connection: return "Device"
        case .applications: return "Applications"
        case .folders: return "Folders"
        case .general: return "General"
        }
    }

    var icon: String {
        switch self {
        case .about: return "info.circle"
        case .connection: return "network"
        case .applications: return "app.badge.checkmark"
        case .folders: return "folder"
        case .general: return "gearshape"
        }
    }
}

private struct CompanionSettings: View {
    @ObservedObject var store: CompanionStore
    @State private var pairingCode = ""
    @State private var applicationSearch = ""
    @AppStorage("settings.selectedPage") private var selectedPageID = CompanionSettingsPage.about.rawValue
    @FocusState private var focusedField: CompanionSettingsField?

    var body: some View {
        NavigationSplitView {
            List(CompanionSettingsPage.allCases, selection: selectedPageBinding) { page in
                Label(page.title, systemImage: page.icon)
                    .tag(page)
            }
            .listStyle(.sidebar)
            .navigationSplitViewColumnWidth(min: 190, ideal: 220, max: 260)
        } detail: {
            detailView
        }
        .navigationSplitViewStyle(.balanced)
        .task { store.refreshApplications() }
        .onChange(of: store.isConnected) { connected in
            if connected { pairingCode = "" }
        }
    }

    @ViewBuilder
    private var detailView: some View {
        switch selectedPage {
        case .about:
            aboutPage
        case .connection:
            connectionPage
        case .applications:
            applicationsPage
        case .folders:
            foldersPage
        case .general:
            generalPage
        }
    }

    private var selectedPage: CompanionSettingsPage {
        CompanionSettingsPage(rawValue: selectedPageID) ?? .about
    }

    private var selectedPageBinding: Binding<CompanionSettingsPage> {
        Binding(
            get: { selectedPage },
            set: { selectedPageID = $0.rawValue }
        )
    }

    private var aboutPage: some View {
        Form {
            Section("Status") {
                NativeSettingsRow(
                    title: "EspControl Companion",
                    description: store.isConnected ? "Connected to your display" : "Not connected to a display"
                ) {
                    if canConnect || store.isConnected {
                        Toggle("Connect EspControl Companion", isOn: connectionBinding)
                            .labelsHidden()
                            .toggleStyle(.switch)
                    } else {
                        Text("Pair a device")
                            .foregroundStyle(.secondary)
                    }
                }

                if hasPanelAddress {
                    NativeSettingsRow(
                        title: "Device Webserver",
                        description: "Open the display settings in your browser."
                    ) {
                        Button("Open Webserver") { store.openPanelWebServer() }
                    }
                }
            }

            Section("Help") {
                Link("Privacy Policy", destination: CompanionStore.privacyPolicyURL)
                Link("EspControl Support", destination: CompanionStore.supportURL)
            }
        }
        .formStyle(.grouped)
        .navigationTitle("About EspControl")
    }

    private var connectionPage: some View {
        Form {
            Section("Connection") {
                NativeSettingsRow(
                    title: "Panel address",
                    description: "The local network address of your EspControl display."
                ) {
                    TextField("192.168.6.100", text: $store.panelHost)
                        .textFieldStyle(.roundedBorder)
                        .focused($focusedField, equals: .panelHost)
                        .frame(width: 220)
                }

                NativeSettingsRow(
                    title: "Pairing code",
                    description: "The eight-letter code shown while pairing is active."
                ) {
                    HStack {
                        TextField("Eight-letter code", text: $pairingCode)
                            .textFieldStyle(.roundedBorder)
                            .focused($focusedField, equals: .pairingCode)
                            .frame(width: 150)
                        Button("Pair") { store.pair(code: pairingCode) }
                            .buttonStyle(.borderedProminent)
                    }
                }

                Text("Press and hold the Wi-Fi icon on the display, then enter its local address and the code shown on the touchscreen.")
                    .font(.callout)
                    .foregroundStyle(.secondary)
            }

            if store.hasSavedPairing {
                Section {
                    Button("Forget This Panel", role: .destructive) {
                        store.forgetPanel()
                        pairingCode = ""
                        focusedField = .panelHost
                    }
                    .buttonStyle(.bordered)
                }
            }
        }
        .formStyle(.grouped)
        .navigationTitle("Device")
    }

    private var applicationsPage: some View {
        List {
            if store.availableApps.isEmpty {
                Text("No applications were found.")
                    .foregroundStyle(.secondary)
            } else if filteredApplications.isEmpty {
                Text("No applications match your search.")
                    .foregroundStyle(.secondary)
            } else {
                ForEach(filteredApplications) { application in
                    Toggle(isOn: Binding(
                        get: { store.applicationIsApproved(application) },
                        set: { store.setApplication(application, approved: $0) }
                    )) {
                        Text(application.name)
                    }
                    .toggleStyle(.switch)
                    .controlSize(.small)
                }
            }
        }
        .listStyle(.inset)
        .searchable(text: $applicationSearch, placement: .toolbar, prompt: "Search Applications")
        .navigationTitle("Applications")
        .safeAreaInset(edge: .bottom) {
            HStack {
                Button("Select All") {
                    store.setApplications(filteredApplications, approved: true)
                }
                .disabled(filteredApplications.isEmpty || allFilteredApplicationsApproved)

                Button("Deselect All") {
                    store.setApplications(filteredApplications, approved: false)
                }
                .disabled(!hasApprovedFilteredApplications)

                Spacer()

                Button {
                    store.refreshApplications()
                } label: {
                    Label("Refresh", systemImage: "arrow.clockwise")
                }
            }
            .buttonStyle(.bordered)
            .padding()
            .background(.bar)
        }
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

    private var foldersPage: some View {
        List {
            if store.approvedFolders.isEmpty {
                Text("No folders have been added.")
                    .foregroundStyle(.secondary)
            } else {
                ForEach(store.approvedFolders) { folder in
                    HStack {
                        VStack(alignment: .leading, spacing: 2) {
                            Text(folder.name)
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
                            Image(systemName: "minus")
                        }
                        .buttonStyle(.bordered)
                        .controlSize(.small)
                        .help("Remove folder")
                    }
                }
            }
        }
        .listStyle(.inset)
        .navigationTitle("Folders")
        .safeAreaInset(edge: .bottom) {
            HStack {
                Button("Add Folder…") { store.chooseFolder() }
                    .buttonStyle(.borderedProminent)
                Spacer()
            }
            .padding()
            .background(.bar)
        }
    }

    private var generalPage: some View {
        Form {
            if store.supportsLaunchAtLogin {
                Section("Startup") {
                    Toggle("Open EspControl Companion at Login", isOn: store.launchAtLoginBinding())
                    Text(store.launchAtLoginMessage)
                        .font(.callout)
                        .foregroundStyle(.secondary)
                }
            } else {
                Text("Login item management is not available on this version of macOS.")
                    .foregroundStyle(.secondary)
            }

            Section("Privacy") {
                Toggle("Share Mac system statistics", isOn: $store.shareSystemMetricsEnabled)
                Text("When enabled, processor, memory, storage, network and battery percentages are sent only to your paired display on the local network.")
                    .font(.callout)
                    .foregroundStyle(.secondary)
            }
        }
        .formStyle(.grouped)
        .navigationTitle("General")
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

private struct NativeSettingsRow<Control: View>: View {
    let title: String
    let description: String
    @ViewBuilder let control: Control

    var body: some View {
        LabeledContent {
            control
        } label: {
            VStack(alignment: .leading, spacing: 2) {
                Text(title)
                Text(description)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
        }
    }
}
