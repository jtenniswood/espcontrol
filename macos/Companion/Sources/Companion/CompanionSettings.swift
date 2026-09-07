import AppKit
import SwiftUI

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
    let isAvailable: Bool

    var body: some View {
        HStack(spacing: 12) {
            Toggle("", isOn: $isEnabled)
                .labelsHidden()
                .toggleStyle(.switch)
                .tint(.green)
                .disabled(!isAvailable)
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
                CompanionLaunchAtLoginToggle(
                    isEnabled: store.launchAtLoginBinding(),
                    isAvailable: store.supportsLaunchAtLogin
                )
                Text(store.supportsLaunchAtLogin
                     ? store.launchAtLoginMessage
                     : "Install EspControl Companion in Applications before enabling automatic startup.")
                    .font(.callout)
                    .foregroundStyle(.secondary)
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

struct CompanionSettings: View {
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
        } else {
            CompanionOnboarding(store: store) {
                onboardingCompleted = true
            }
        }
    }

    private var settingsContent: some View {
        ZStack(alignment: .bottomTrailing) {
            HStack(spacing: 0) {
                VStack(alignment: .leading, spacing: 0) {
                Text("EspControl")
                    .font(.system(size: 24, weight: .semibold))
                    .padding(.leading, 20)
                    .padding(.top, 28)
                    .padding(.bottom, 20)

                Text("Settings")
                    .font(.system(size: 15, weight: .semibold))
                    .foregroundStyle(Color(white: 0.50))
                    .padding(.leading, 20)
                    .padding(.bottom, 0)

                List {
                    ForEach(CompanionSettingsPage.allCases) { page in
                        Button {
                            selectedPageID = page.rawValue
                        } label: {
                            Label {
                                Text(page.title)
                                    .font(.system(size: 14, weight: .medium))
                            } icon: {
                                Image(systemName: page.icon)
                                    .font(.system(size: 16, weight: .regular))
                                    .frame(width: 22)
                            }
                            .frame(maxWidth: .infinity, alignment: .leading)
                        }
                        .buttonStyle(.plain)
                        .padding(.vertical, 7)
                        .contentShape(Rectangle())
                        .listRowInsets(EdgeInsets(top: 2, leading: 12, bottom: 2, trailing: 12))
                        .listRowBackground(
                            RoundedRectangle(cornerRadius: 9, style: .continuous)
                                .fill(selectedPage == page ? Color(white: 0.21) : .clear)
                                .padding(.horizontal, 10)
                        )
                        .listRowSeparator(.hidden)
                    }
                }
                .listStyle(.sidebar)
                .scrollContentBackground(.hidden)
            }
            .frame(minWidth: 190, idealWidth: 210, maxWidth: 230)
            .foregroundStyle(Color(white: 0.98))
            .background(Color(white: 0.14))

                Divider()
                    .ignoresSafeArea(.container, edges: .top)

                detailView
                    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
            }
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
                CompanionLaunchAtLoginToggle(
                    isEnabled: store.launchAtLoginBinding(),
                    isAvailable: store.supportsLaunchAtLogin
                )
                Text(store.supportsLaunchAtLogin
                     ? store.launchAtLoginMessage
                     : "Install EspControl Companion in Applications to open it automatically at login.")
                    .font(.callout)
                    .foregroundStyle(.secondary)
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

    private var supportButtonImage: NSImage? {
        guard let imageURL = companionResourceBundle.url(
            forResource: "buy-me-a-coffee-button",
            withExtension: "png"
        ) else { return nil }
        return NSImage(contentsOf: imageURL)
    }

    private var floatingSupportButton: some View {
        Link(destination: CompanionStore.buyMeACoffeeURL) {
            if let supportButtonImage {
                Image(nsImage: supportButtonImage)
                    .resizable()
                    .scaledToFit()
                    .frame(width: 171.2, height: 48)
                    .clipShape(Capsule())
            } else {
                Label("Buy me a coffee", systemImage: "cup.and.saucer.fill")
                    .font(.system(size: 12, weight: .semibold))
                    .foregroundStyle(.black.opacity(0.78))
                    .frame(width: 171.2, height: 48)
                    .background(Color(red: 1.0, green: 0.867, blue: 0.0))
                    .clipShape(Capsule())
            }
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
