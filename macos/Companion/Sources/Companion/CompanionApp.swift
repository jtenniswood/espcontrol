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
                .dynamicTypeSize(.large)
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
                Button("Companion Settings") { appDelegate.openCompanionWindow() }
                    .keyboardShortcut(",", modifiers: .command)
            }
        }
    }
}

@MainActor
final class CompanionApplicationDelegate: NSObject, NSApplicationDelegate, NSMenuDelegate, NSWindowDelegate {
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
            title: "Customize Panel",
            action: #selector(openDisplaySettings),
            keyEquivalent: "d"
        )
        panelWebpageItem.target = self
        panelWebpageItem.keyEquivalentModifierMask = [.command]
        panelWebpageItem.image = NSImage(
            systemSymbolName: "display", accessibilityDescription: "Configure")
        panelWebpageItem.isEnabled = !store.panelHost.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
        menu.addItem(panelWebpageItem)

        addMenuItem("Companion Settings", action: #selector(openSettings), key: ",", to: menu)
        addMenuItem(
            "Quit App", action: #selector(quit), key: "q",
            image: NSImage(systemSymbolName: "power", accessibilityDescription: "Quit"), to: menu)
    }

    private func connectionStatusItem() -> NSMenuItem {
        let item = NSMenuItem()
        let container = NSView(frame: NSRect(x: 0, y: 0, width: 260, height: 58))
        let menuFont = NSFont.menuFont(ofSize: 0)

        let title = NSTextField(labelWithString: "EspControl")
        title.font = .systemFont(ofSize: menuFont.pointSize, weight: .semibold)

        let status = NSTextField(labelWithString: store.isConnected ? "Connected" : "Disconnected")
        status.font = menuFont
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

    private func addMenuItem(
        _ title: String, action: Selector, key: String = "", image: NSImage? = nil, to menu: NSMenu
    ) {
        let item = NSMenuItem(title: title, action: action, keyEquivalent: key)
        item.target = self
        item.image = image
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
        window.delegate = self
        positionWindowControls(in: window)
        window.minSize = NSSize(width: 760, height: 500)
        window.isReleasedWhenClosed = false
        window.contentView = NSHostingView(
            rootView: CompanionSettings(store: store).dynamicTypeSize(.large)
        )
        window.center()
        settingsWindow = window
        window.makeKeyAndOrderFront(nil)
        activateCompanionApplication()
    }

    func windowWillClose(_ notification: Notification) {
        guard let closedWindow = notification.object as? NSWindow,
              closedWindow === settingsWindow else { return }
        // Keep the menu-bar companion running, but remove its Dock presence
        // once the settings window has been closed.
        NSApp.setActivationPolicy(.accessory)
    }

    private func positionWindowControls(in window: NSWindow) {
        let buttons: [NSButton?] = [
            window.standardWindowButton(.closeButton),
            window.standardWindowButton(.miniaturizeButton),
            window.standardWindowButton(.zoomButton),
        ]
        let inset: CGFloat = 32
        let topPadding: CGFloat = 12
        let spacing: CGFloat = 8
        var nextX = inset
        for button in buttons.compactMap({ $0 }) {
            var frame = button.frame
            frame.origin.x = nextX
            frame.origin.y = max(0, frame.origin.y - topPadding)
            button.frame = frame
            button.contentTintColor = .secondaryLabelColor
            nextX += frame.width + spacing
        }
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
    case connection, applications, folders, general, help

    var id: String { rawValue }
    var title: String {
        switch self {
        case .connection: return "Display"
        case .applications: return "Applications"
        case .folders: return "Folders"
        case .general: return "Permissions"
        case .help: return "Help"
        }
    }
    var icon: String {
        switch self {
        case .connection: return "display"
        case .applications: return "square.grid.2x2"
        case .folders: return "folder"
        case .general: return "gearshape"
        case .help: return "questionmark.circle"
        }
    }
}

private enum CompanionPairingStep {
    case address, code, connecting, connected
}

private struct CompanionOnboardingPage<Content: View>: View {
    let icon: String
    let title: String
    let summary: String
    @ViewBuilder let content: () -> Content

    var body: some View {
        VStack(alignment: .leading, spacing: 22) {
            Image(systemName: icon)
                .font(.system(size: 36, weight: .medium))
                .foregroundStyle(.tint)
                .accessibilityHidden(true)
            VStack(alignment: .leading, spacing: 8) {
                Text(title)
                    .font(.system(size: 28, weight: .semibold))
                Text(summary)
                    .font(.title3)
                    .foregroundStyle(.secondary)
            }
            content()
        }
        .frame(maxWidth: 620, alignment: .leading)
    }
}

private struct CompanionStatsToggle: View {
    @Binding var isEnabled: Bool

    var body: some View {
        HStack(spacing: 12) {
            Toggle("", isOn: $isEnabled)
                .labelsHidden()
                .toggleStyle(.switch)
                .tint(.green)
                .accessibilityLabel("Share Mac system statistics")
            Text(isEnabled ? "Stats enabled" : "Stats disabled")
                .font(.headline)
                .foregroundStyle(isEnabled ? .primary : .secondary)
        }
    }
}

private struct CompanionLaunchAtLoginToggle: View {
    @Binding var isEnabled: Bool

    var body: some View {
        HStack(spacing: 12) {
            Toggle("", isOn: $isEnabled)
                .labelsHidden()
                .toggleStyle(.switch)
                .tint(.green)
                .accessibilityLabel("Open EspControl Companion at Login")
            Text(isEnabled ? "Login enabled" : "Login disabled")
                .font(.headline)
                .foregroundStyle(isEnabled ? .primary : .secondary)
        }
    }
}

private struct CompanionAccessibilityToggle: View {
    @Binding var isEnabled: Bool
    let openSettings: () -> Void

    var body: some View {
        HStack(spacing: 12) {
            Toggle("", isOn: Binding(
                get: { isEnabled },
                set: { _ in openSettings() }
            ))
            .labelsHidden()
            .toggleStyle(.switch)
            .tint(.green)
            .accessibilityLabel("Enable keyboard shortcuts and window controls")
            Text(isEnabled ? "Shortcuts enabled" : "Shortcuts disabled")
                .font(.headline)
                .foregroundStyle(isEnabled ? .primary : .secondary)
        }
    }
}

private struct CompanionOnboarding: View {
    @ObservedObject var store: CompanionStore
    let onComplete: () -> Void
    @State private var step = 0
    @State private var accessibilityGranted = false

    private let totalSteps = 3

    var body: some View {
        VStack(spacing: 0) {
            VStack(alignment: .leading, spacing: 12) {
                Text("Set up EspControl Companion")
                    .font(.title2.weight(.semibold))
                HStack(spacing: 10) {
                    ProgressView(value: Double(step + 1), total: Double(totalSteps))
                    Text("\(step + 1) of \(totalSteps)")
                        .font(.callout)
                        .foregroundStyle(.secondary)
                        .monospacedDigit()
                }
            }
            .frame(maxWidth: 620, alignment: .leading)
            .padding(.top, 34)

            Spacer(minLength: 28)

            currentPage
                .id(step)
                .transition(.opacity)

            Spacer(minLength: 28)

            Divider()
            HStack {
                Button("Skip for now") { onComplete() }
                    .buttonStyle(.borderless)
                Spacer()
                if step > 0 {
                    Button("Back") { step -= 1 }
                }
                Button(step == totalSteps - 1 ? "Finish" : "Continue") {
                    if step == totalSteps - 1 {
                        onComplete()
                    } else {
                        step += 1
                    }
                }
                .buttonStyle(.borderedProminent)
            }
            .frame(maxWidth: 620)
            .padding(.vertical, 18)
        }
        .padding(.horizontal, 44)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .animation(.easeInOut(duration: 0.2), value: step)
        .onAppear { refreshAccessibilityStatus() }
        .onReceive(NotificationCenter.default.publisher(for: NSApplication.didBecomeActiveNotification)) { _ in
            refreshAccessibilityStatus()
        }
    }

    @ViewBuilder private var currentPage: some View {
        switch step {
        case 0:
            CompanionOnboardingPage(
                icon: "keyboard",
                title: "Enable shortcut support",
                summary: "Shortcut and window-control cards need macOS Accessibility permission to send commands to your active Mac app."
            ) {
                CompanionAccessibilityToggle(isEnabled: $accessibilityGranted, openSettings: enableAccessibility)
                Text(accessibilityGranted
                     ? "Keyboard shortcuts and window controls are enabled for your display."
                     : "Turn on EspControl Companion in System Settings → Privacy & Security → Accessibility to enable keyboard shortcuts and window controls.")
                    .font(.callout)
                    .foregroundStyle(.secondary)
            }
        case 1:
            CompanionOnboardingPage(
                icon: "chart.bar.xaxis",
                title: "Statistics card support",
                summary: "Stats cards can show processor, memory, storage, network, and battery information from this Mac. Data is only shared to your local device."
            ) {
                CompanionStatsToggle(isEnabled: $store.shareSystemMetricsEnabled)
                Text("Statistics are shared only with your paired display on the local network. You can change this later in Permissions settings.")
                    .font(.callout)
                    .foregroundStyle(.secondary)
            }
        default:
            CompanionOnboardingPage(
                icon: "power",
                title: "Stay connected at login",
                summary: "Start Companion automatically when you sign in so your paired display can reconnect to the Mac."
            ) {
                if store.supportsLaunchAtLogin {
                    CompanionLaunchAtLoginToggle(isEnabled: store.launchAtLoginBinding())
                    Text(store.launchAtLoginMessage)
                        .font(.callout)
                        .foregroundStyle(.secondary)
                } else {
                    Text("Install EspControl Companion in Applications before enabling automatic startup.")
                        .font(.callout)
                        .foregroundStyle(.secondary)
                }
            }
        }
    }

    private func refreshAccessibilityStatus() {
        accessibilityGranted = CompanionAccessibilityAuthorizer.shared.hasAccess
    }

    private func enableAccessibility() {
        _ = CompanionAccessibilityAuthorizer.shared.isTrusted()
        if let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility") {
            NSWorkspace.shared.open(url)
        }
    }
}

private struct CompanionSettings: View {
    @ObservedObject var store: CompanionStore
    @State private var pairingCode = ""
    @State private var applicationSearch = ""
    @FocusState private var applicationSearchFocused: Bool
    @State private var confirmingForget = false
    @State private var folderToRemove: ApprovedFolder?
    @State private var accessibilityGranted = false
    @State private var pairingStep: CompanionPairingStep = .address
    @State private var pairingFlowActive = false
    @State private var pairingFlowError = ""
    @AppStorage("companion.onboarding.completed") private var onboardingCompleted = false
    @AppStorage("settings.selectedPage") private var selectedPageID = CompanionSettingsPage.connection.rawValue
    @FocusState private var focusedField: CompanionSettingsField?

    var body: some View {
        if onboardingCompleted {
            settingsContent
                .dynamicTypeSize(.xLarge)
        } else {
            CompanionOnboarding(store: store) {
                onboardingCompleted = true
            }
        }
    }

    private var settingsContent: some View {
        HStack(spacing: 0) {
            VStack(alignment: .leading, spacing: 0) {
                HStack(spacing: 8) {
                    Text("EspControl")
                        .font(.system(size: 26, weight: .semibold))
                }
                .padding(.leading, 22)
                .padding(.top, 34)
                .padding(.bottom, 18)

                Text("Settings")
                    .font(.system(size: 18, weight: .semibold))
                    .foregroundStyle(.secondary)
                    .padding(.leading, 22)
                    .padding(.bottom, 12)

                List(CompanionSettingsPage.allCases, selection: selectedPageBinding) { page in
                    Label {
                        Text(page.title)
                            .font(.system(size: 19, weight: .medium))
                    } icon: {
                        Image(systemName: page.icon)
                            .font(.system(size: 19, weight: .regular))
                            .frame(width: 24)
                    }
                    .padding(.vertical, 7)
                    .listRowInsets(EdgeInsets(top: 4, leading: 14, bottom: 4, trailing: 14))
                    .listRowBackground(
                        RoundedRectangle(cornerRadius: 14, style: .continuous)
                            .fill(selectedPage == page ? Color.white.opacity(0.14) : .clear)
                    )
                    .listRowSeparator(.hidden)
                    .tag(page)
                }
                .listStyle(.sidebar)
                .scrollContentBackground(.hidden)
            }
            .frame(minWidth: 180, idealWidth: 200, maxWidth: 240)
            .background(Color(nsColor: .windowBackgroundColor))

            Divider()
                .ignoresSafeArea(.container, edges: .top)

            detailView
                .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        }
        .overlay(alignment: .bottomTrailing) {
            floatingSupportButton
                .padding(.trailing, 24)
                .padding(.bottom, 24)
        }
        .onAppear {
            if !store.hasSavedPairing { selectedPageID = CompanionSettingsPage.connection.rawValue }
            if !store.hasSavedPairing && !pairingFlowActive { startPairingFlow() }
            refreshAccessibilityStatus()
        }
        .onReceive(NotificationCenter.default.publisher(for: NSApplication.didBecomeActiveNotification)) { _ in
            refreshAccessibilityStatus()
        }
        .onChange(of: store.isConnected) { connected in
            if connected {
                pairingCode = ""
                if pairingFlowActive { pairingStep = .connected }
            }
        }
        .onChange(of: store.connectionState) { state in
            if pairingFlowActive && pairingStep == .connecting && state == .failed {
                pairingStep = .code
                pairingFlowError = store.connectionRecoveryMessage
            }
        }
        .onChange(of: pairingStep) { step in
            switch step {
            case .address: focusedField = .panelHost
            case .code: focusedField = .pairingCode
            case .connecting, .connected: focusedField = nil
            }
        }
        .alert("Forget this display?", isPresented: $confirmingForget) {
            Button("Cancel", role: .cancel) {}
            Button("Forget Display", role: .destructive) {
                store.forgetPanel()
                pairingCode = ""
                pairingFlowActive = true
                pairingStep = .address
                pairingFlowError = ""
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
        case .general: permissionsPage
        case .help: helpPage
        }
    }

    private var selectedPage: CompanionSettingsPage {
        CompanionSettingsPage(rawValue: selectedPageID) ?? .connection
    }

    private var selectedPageBinding: Binding<CompanionSettingsPage> {
        Binding(get: { selectedPage }, set: { selectedPageID = $0.rawValue })
    }

    private var connectionPage: some View {
        Group {
            if store.hasSavedPairing && !pairingFlowActive {
                List {
                    Section {
                        HStack {
                            Toggle("Connection", isOn: connectionToggleBinding)
                                .labelsHidden()
                                .toggleStyle(.switch)
                                .tint(.green)
                                .disabled(store.connectionState.isBusy)
                            Text(store.isConnected ? "Connected" : "Disconnected")
                                .font(.headline)
                                .foregroundStyle(store.isConnected ? .primary : .secondary)
                            Spacer()
                        }
                        Button("Forget Display…", role: .destructive) { confirmingForget = true }
                        Button("Customize Display") { store.openPanelWebServer() }
                            .help("Open the display’s configuration in your browser")
                    }
                }
                .listStyle(.inset)
            } else {
                pairingFlowPage
            }
        }
    }

    @ViewBuilder
    private var pairingFlowPage: some View {
        Form {
            switch pairingStep {
            case .address:
                Section("Step 1 of 3 · Display address") {
                    Text("Enter the local address of your EspControl display.")
                        .foregroundStyle(.secondary)
                    TextField("IP address or name.local", text: $store.panelHost)
                        .textFieldStyle(.roundedBorder)
                        .accessibilityLabel("Display address")
                        .focused($focusedField, equals: .panelHost)
                        .onSubmit { openPairingPage() }
                    if !pairingFlowError.isEmpty {
                        Text(pairingFlowError)
                            .font(.callout)
                            .foregroundStyle(.orange)
                    }
                    HStack {
                        Spacer()
                        Button("Continue") { openPairingPage() }
                            .buttonStyle(.borderedProminent)
                            .disabled(!canOpenPairingPage)
                    }
                }
            case .code:
                Section("Step 2 of 3 · Pairing code") {
                    Text("A pairing page has opened for your display. Start pairing there, copy the eight-letter code, then enter it below.")
                        .foregroundStyle(.secondary)
                    TextField("ABCD-EFGH", text: $pairingCode)
                        .textFieldStyle(.roundedBorder)
                        .font(.system(.body, design: .monospaced))
                        .accessibilityLabel("Pairing code")
                        .focused($focusedField, equals: .pairingCode)
                        .onSubmit { pairDisplay() }
                    if !pairingFlowError.isEmpty {
                        Text(pairingFlowError)
                            .font(.callout)
                            .foregroundStyle(.orange)
                    }
                    HStack {
                        Button("Back") {
                            pairingFlowError = ""
                            pairingStep = .address
                        }
                        Spacer()
                        Button("Continue") { pairDisplay() }
                            .buttonStyle(.borderedProminent)
                            .disabled(!canPair)
                    }
                }
            case .connecting:
                Section("Step 3 of 3 · Confirm connection") {
                    HStack(spacing: 10) {
                        ProgressView().controlSize(.small)
                        Text("Pairing and connecting to your display…")
                    }
                    .accessibilityElement(children: .combine)
                    Text("Keep both devices connected to the same local network.")
                        .font(.callout)
                        .foregroundStyle(.secondary)
                }
            case .connected:
                Section("Step 3 of 3 · Connection established") {
                    Label("Connected", systemImage: "checkmark.circle.fill")
                        .foregroundStyle(.green)
                    Text("Your Mac is paired with the EspControl display and ready to use.")
                        .foregroundStyle(.secondary)
                    HStack {
                        Spacer()
                        Button("Done") {
                            pairingFlowActive = false
                            pairingStep = .address
                        }
                        .buttonStyle(.borderedProminent)
                    }
                }
            }
        }
        .formStyle(.grouped)
        .navigationTitle("Connect Display")
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
        }
    }

    private var connectionToggleBinding: Binding<Bool> {
        Binding(
            get: { store.isConnected },
            set: { enabled in
                if enabled {
                    store.connect()
                } else {
                    store.disconnect()
                }
            }
        )
    }

    private var canPair: Bool {
        !store.connectionState.isBusy && CompanionPairingInput.isValid(host: store.panelHost, code: pairingCode)
    }

    private var canOpenPairingPage: Bool {
        CompanionStore.panelWebServerURL(from: store.panelHost) != nil
    }

    private func startPairingFlow() {
        pairingFlowActive = true
        pairingStep = .address
        pairingFlowError = ""
    }

    private func openPairingPage() {
        guard canOpenPairingPage else {
            pairingFlowError = "Enter a valid local IP address or name.local."
            return
        }
        guard store.openPanelPairing() else {
            pairingFlowError = "Could not open the display pairing page."
            return
        }
        pairingFlowError = ""
        pairingStep = .code
    }

    private func pairDisplay() {
        guard canPair else {
            pairingFlowError = "Enter the eight-letter pairing code shown on the display."
            return
        }
        pairingFlowError = ""
        pairingStep = .connecting
        store.pair(code: pairingCode)
    }

    private var applicationsPage: some View {
        Form {
            Section {
                HStack(spacing: 8) {
                    Image(systemName: "magnifyingglass")
                        .foregroundStyle(.secondary)
                        .accessibilityHidden(true)
                    TextField("Search", text: $applicationSearch)
                        .textFieldStyle(.plain)
                        .multilineTextAlignment(.leading)
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .focused($applicationSearchFocused)
                    if !applicationSearch.isEmpty {
                        Button {
                            applicationSearch = ""
                        } label: {
                            Image(systemName: "xmark.circle.fill")
                                .foregroundStyle(.secondary)
                        }
                        .buttonStyle(.plain)
                        .accessibilityLabel("Clear Search")
                    }
                }
                .padding(.horizontal, 18)
                .padding(.vertical, 14)
                .background(.background, in: Capsule())
                .overlay {
                    Capsule()
                        .stroke(Color.primary.opacity(0.22), lineWidth: 1)
                }
                .contentShape(Capsule())
                .simultaneousGesture(
                    TapGesture().onEnded { applicationSearchFocused = true }
                )
            }
            Section {
                HStack(spacing: 12) {
                    Toggle("Select All", isOn: selectAllBinding)
                        .toggleStyle(.checkbox)
                        .disabled(filteredApplications.isEmpty)
                    Spacer()
                }
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
        .navigationTitle("Applications")
    }

    private var isSearching: Bool { !applicationSearch.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty }
    private var selectAllBinding: Binding<Bool> {
        Binding(
            get: {
                !filteredApplications.isEmpty && filteredApplications.allSatisfy(store.applicationIsApproved)
            },
            set: { store.setApplications(filteredApplications, approved: $0) }
        )
    }

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
                                    Label("Folder unavailable", systemImage: "exclamationmark.triangle")
                                        .font(.callout)
                                    Button("Choose Again…") { store.chooseFolder(restoring: folder) }
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
                    Button("Add Folder…") { store.chooseFolder() }
                        .buttonStyle(.bordered)
                }
                if let message = store.folderMessage {
                    Text(message).font(.callout).foregroundStyle(.secondary)
                }
            }
        }
        .formStyle(.grouped)
        .navigationTitle("Folders")
    }

    private func emptyState(_ title: String, symbol: String, detail: String) -> some View {
        VStack(alignment: .leading, spacing: 8) {
            Label(title, systemImage: symbol).font(.headline)
            Text(detail).foregroundStyle(.secondary)
        }
        .padding(.vertical, 8)
    }

    private var permissionsPage: some View {
        Form {
            Section("Startup") {
                if store.supportsLaunchAtLogin {
                    CompanionLaunchAtLoginToggle(isEnabled: store.launchAtLoginBinding())
                    Text(store.launchAtLoginMessage).font(.callout).foregroundStyle(.secondary)
                } else {
                    Text("Install EspControl Companion in Applications to open it automatically at login.")
                        .foregroundStyle(.secondary)
                }
            }
            Section("Privacy") {
                CompanionStatsToggle(isEnabled: $store.shareSystemMetricsEnabled)
                Text("Share processor, memory, storage, network, and battery statistics only with your paired display on the local network.")
                    .font(.callout).foregroundStyle(.secondary)
            }
            Section("Keyboard & Window Controls") {
                CompanionAccessibilityToggle(isEnabled: $accessibilityGranted, openSettings: enableAccessibility)
                Text(accessibilityGranted
                     ? "Keyboard shortcuts and window controls are enabled for your display."
                     : "Turn on EspControl Companion in System Settings → Privacy & Security → Accessibility to enable keyboard shortcuts and window controls.")
                    .font(.callout)
                    .foregroundStyle(.secondary)
            }
        }
        .formStyle(.grouped)
        .navigationTitle("Permissions")
    }

    private var helpPage: some View {
        Form {
            Section("Support") {
                VStack(alignment: .leading, spacing: 8) {
                    Text("Support EspControl")
                        .font(.headline)
                    Text("If EspControl is useful to you, you can support ongoing development and user support by buying me a coffee.")
                        .font(.callout)
                        .foregroundStyle(.secondary)
                    Link("Buy Me a Coffee", destination: CompanionStore.buyMeACoffeeURL)
                }
            }
            Section("Resources") {
                Link("EspControl Support", destination: CompanionStore.supportURL)
                Link("Privacy Policy", destination: CompanionStore.privacyPolicyURL)
            }
        }
        .formStyle(.grouped)
        .navigationTitle("Help")
    }

    private var companionResourceBundle: Bundle {
        // Installed apps keep resources inside Contents/Resources; SwiftPM runs
        // use the generated module bundle beside the build output.
        Bundle.main.url(forResource: "EspControlCompanion_Companion", withExtension: "bundle")
            .flatMap { Bundle(url: $0) } ?? .module
    }

    private var floatingSupportButton: some View {
        Link(destination: CompanionStore.buyMeACoffeeURL) {
            Image("buy-me-a-coffee-button", bundle: companionResourceBundle)
                .resizable()
                .scaledToFit()
                .frame(width: 214, height: 60)
                .clipShape(Capsule())
        }
        .buttonStyle(.plain)
        .help("Support EspControl by buying me a coffee")
        .accessibilityLabel("Buy me a coffee to support EspControl")
    }

    private func refreshAccessibilityStatus() {
        accessibilityGranted = CompanionAccessibilityAuthorizer.shared.hasAccess
    }

    private func enableAccessibility() {
        _ = CompanionAccessibilityAuthorizer.shared.isTrusted()
        if let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility") {
            NSWorkspace.shared.open(url)
        }
    }
}
