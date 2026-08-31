import AppKit
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
                CompanionSettingsButton()
            } else {
                Button("Open Settings…") {
                    NSApp.activate(ignoringOtherApps: true)
                    NSApp.sendAction(Selector(("showPreferencesWindow:")), to: nil, from: nil)
                    focusSettingsWindow()
                }
            }
            Divider()
            Button("Quit") { NSApp.terminate(nil) }
        }
        .padding()
    }
}

@available(macOS 14.0, *)
private struct CompanionSettingsButton: View {
    @Environment(\.openSettings) private var openSettings

    var body: some View {
        Button("Open Settings…") {
            NSApp.activate(ignoringOtherApps: true)
            openSettings()
            focusSettingsWindow()
        }
    }
}

private func focusSettingsWindow() {
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
        NSApp.activate(ignoringOtherApps: true)
        NSApp.windows.first { $0.title.localizedCaseInsensitiveContains("Settings") }?
            .makeKeyAndOrderFront(nil)
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
            VStack(alignment: .leading, spacing: 20) {
                GroupBox("Panel") {
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

                        if store.hasSavedPairing {
                            Button("Forget this panel", role: .destructive) { store.forgetPanel() }
                        }
                    }
                    .padding(8)
                }

                GroupBox("Apps shown on the panel") {
                    VStack(alignment: .leading, spacing: 10) {
                        if store.availableApps.isEmpty {
                            Text("No apps found in /Applications yet.").foregroundStyle(.secondary)
                        } else {
                            ForEach(store.availableApps) { app in
                                Toggle(app.name, isOn: store.allowedBinding(for: app))
                            }
                        }
                        Button("Rescan Applications") { store.refreshApplications() }
                    }
                    .padding(8)
                }

                GroupBox("Connection") {
                    VStack(alignment: .leading, spacing: 8) {
                        Text(store.statusDescription)
                        Text("The panel is the secure server. This app only makes an outbound connection to the panel and does not listen for incoming network connections.")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(8)
                }
            }
            .padding()
        }
        .onAppear {
            focusSettingsWindow()
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
                focusedField = .panelHost
            }
        }
        .task { store.refreshApplications() }
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
