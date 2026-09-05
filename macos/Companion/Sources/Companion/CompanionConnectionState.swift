import Foundation

/// Connection progress is independent of diagnostic text and action errors.
enum CompanionConnectionState: Equatable {
    case disconnected, connecting, reconnecting, connected, failed

    var isBusy: Bool { self == .connecting || self == .reconnecting }
    var title: String {
        switch self {
        case .disconnected: return "Not Connected"
        case .connecting: return "Connecting…"
        case .reconnecting: return "Reconnecting…"
        case .connected: return "Connected"
        case .failed: return "Connection Needs Attention"
        }
    }
    var symbol: String {
        switch self {
        case .connected: return "checkmark.circle"
        case .failed: return "exclamationmark.triangle"
        default: return "display"
        }
    }
}

enum CompanionPairingInput {
    static func normalizedCode(_ code: String) -> String {
        code.trimmingCharacters(in: .whitespacesAndNewlines).uppercased()
    }

    static func isValid(host: String, code: String) -> Bool {
        // Accept only the eight ASCII letters used by the physical display.
        let code = code.trimmingCharacters(in: .whitespacesAndNewlines)
        return !host.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
            && code.utf8.count == 8
            && code.utf8.allSatisfy { (65...90).contains($0) || (97...122).contains($0) }
    }
}
