import Darwin
import Foundation

/// The Companion protocol is intentionally limited to a local-network panel.
/// Keeping this policy next to URL construction prevents a future feature from
/// accidentally sending panel-bound telemetry to a public host.
enum ConnectionEndpointPolicy {
    static func isLocalHost(_ value: String) -> Bool {
        let host = value.trimmingCharacters(in: .whitespacesAndNewlines)
            .lowercased()
            .trimmingCharacters(in: CharacterSet(charactersIn: "[]"))
        guard !host.isEmpty else { return false }

        let dnsHost = host.hasSuffix(".") ? String(host.dropLast()) : host
        if dnsHost == "localhost" || dnsHost.hasSuffix(".local") || dnsHost.hasSuffix(".home.arpa") {
            return true
        }

        let addressHost = host.split(separator: "%", maxSplits: 1, omittingEmptySubsequences: true)
            .first.map(String.init) ?? host
        if isLocalIPv4(addressHost) || isLocalIPv6(addressHost) {
            return true
        }
        return false
    }

    private static func isLocalIPv4(_ host: String) -> Bool {
        let parts = host.split(separator: ".", omittingEmptySubsequences: false)
        guard parts.count == 4,
              parts.allSatisfy({
                  !$0.isEmpty && ($0.count == 1 || $0.first != "0")
                      && $0.utf8.allSatisfy { (48...57).contains($0) }
              }) else { return false }
        let octets = parts.compactMap { Int(String($0)) }
        guard octets.count == 4, octets.allSatisfy({ (0...255).contains($0) }) else { return false }
        switch octets {
        case let values where values[0] == 10: return true
        case let values where values[0] == 127: return true
        case let values where values[0] == 169 && values[1] == 254: return true
        case let values where values[0] == 172 && (16...31).contains(values[1]): return true
        case let values where values[0] == 192 && values[1] == 168: return true
        default: return false
        }
    }

    private static func isLocalIPv6(_ host: String) -> Bool {
        var address = in6_addr()
        guard host.withCString({ inet_pton(AF_INET6, $0, &address) }) == 1 else { return false }
        let bytes = withUnsafeBytes(of: address) { Array($0) }
        let isLoopback = bytes.dropLast().allSatisfy { $0 == 0 } && bytes.last == 1
        let isUniqueLocal = (bytes[0] & 0xfe) == 0xfc
        let isLinkLocal = bytes[0] == 0xfe && (bytes[1] & 0xc0) == 0x80
        return isLoopback || isUniqueLocal || isLinkLocal
    }
}
