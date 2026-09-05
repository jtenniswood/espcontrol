/// Transport state is independent of diagnostic and action-result messages.
enum CompanionConnectionState: Equatable {
    case disconnected
    case connecting
    case pairing
    case authenticating
    case connected
    case reconnecting

    var isAuthenticated: Bool { self == .connected }
}
