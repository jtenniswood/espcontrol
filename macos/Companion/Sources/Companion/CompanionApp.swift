import SwiftUI

@main
struct CompanionApp: App {
    @StateObject private var store = CompanionStore()

    var body: some Scene {
        MenuBarExtra("EspControl Companion", systemImage: store.connectionSymbol) {
            CompanionMenu(store: store)
        }
        .menuBarExtraStyle(.window)

        Settings {
            CompanionSettings(store: store)
                .frame(width: 560, height: 500)
        }
    }
}

private struct CompanionMenu: View {
    @ObservedObject var store: CompanionStore

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("EspControl Companion").font(.headline)
            Text(store.statusDescription).foregroundStyle(.secondary)
            Divider()
            Button(store.isConnected ? "Reconnect" : "Connect") { store.connect() }
            if #available(macOS 14.0, *) {
                SettingsLink { Text("Open Settings…") }
            } else {
                Button("Open Settings…") {
                    NSApp.sendAction(Selector(("showPreferencesWindow:")), to: nil, from: nil)
                }
            }
            Divider()
            Button("Quit") { NSApp.terminate(nil) }
        }
        .padding()
    }
}

private struct CompanionSettings: View {
    @ObservedObject var store: CompanionStore
    @State private var pairingCode = ""
    @State private var verificationCode = ""

    var body: some View {
        Form {
            Section("Panel") {
                TextField("Panel address", text: $store.panelHost)
                TextField("Panel name", text: $store.panelName)
                HStack {
                    TextField("Pairing code", text: $pairingCode)
                    TextField("Verification code", text: $verificationCode)
                    Button("Pair") { store.pair(code: pairingCode, verificationCode: verificationCode) }
                }
                Text("On the display, start Companion pairing first. Enter both the eight-letter code and the Verify code exactly as shown.")
                    .font(.caption).foregroundStyle(.secondary)
                if store.hasSavedPairing {
                    Button("Forget this panel", role: .destructive) { store.forgetPanel() }
                }
            }
            Section("Apps shown on the panel") {
                if store.availableApps.isEmpty {
                    Text("No apps found in /Applications yet.").foregroundStyle(.secondary)
                } else {
                    List(store.availableApps) { app in
                        Toggle(app.name, isOn: store.allowedBinding(for: app))
                    }
                }
                Button("Rescan Applications") { store.refreshApplications() }
            }
            Section("Connection") {
                Text(store.statusDescription)
                Text("The panel is the secure server. This app only makes an outbound connection to the panel and does not listen for incoming network connections.")
                    .font(.caption).foregroundStyle(.secondary)
            }
        }
        .formStyle(.grouped)
        .padding()
        .task { store.refreshApplications() }
    }
}
