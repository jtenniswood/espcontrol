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
func activateCompanionApplication() {
    NSApp.setActivationPolicy(.regular)
    NSApp.unhide(nil)
    NSRunningApplication.current.activate(options: [
        .activateAllWindows,
        .activateIgnoringOtherApps,
    ])
    NSApp.activate(ignoringOtherApps: true)
}
