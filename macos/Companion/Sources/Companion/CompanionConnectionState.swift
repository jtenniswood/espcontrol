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
    static func normalizedCode(_ code: String) -> String? {
        var letters = Array(code.trimmingCharacters(in: .whitespacesAndNewlines).utf8)
        // Firmware displays and verifies ABCD-EFGH. Accept typing without the
        // separator, but always send the exact format the display generates.
        if letters.count == 9, letters[4] == 45 { letters.remove(at: 4) }
        guard letters.count == 8,
              letters.allSatisfy({ (65...90).contains($0) || (97...122).contains($0) }) else { return nil }
        let uppercase = String(decoding: letters, as: UTF8.self).uppercased()
        return "\(uppercase.prefix(4))-\(uppercase.suffix(4))"
    }

    static func isValid(host: String, code: String) -> Bool {
        !host.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty && normalizedCode(code) != nil
    }
}
