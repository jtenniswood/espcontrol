import Foundation

// Narrow OS-facing ports keep CompanionStore focused on product state and
// transport orchestration. Tests and future macOS implementations can replace
// any provider without constructing the real system integration.

@MainActor
protocol NowPlayingProviding: AnyObject {
    var onSnapshot: ((CompanionNowPlayingSnapshot) -> Void)? { get set }
    var onStatus: ((String) -> Void)? { get set }
    func start()
    func stop()
    func stopAndPublishUnavailable()
}

@MainActor
protocol SystemMetricsProviding: AnyObject {
    var onSnapshot: ((CompanionSystemMetricsSnapshot) -> Void)? { get set }
    func start()
    func stop()
}

@MainActor
protocol MediaControlling: AnyObject {
    var actionsAvailable: Bool { get }
    func supports(actionIdentifier: String) -> Bool
    func perform(actionIdentifier: String) -> Bool
    func values() -> [String: Int]
    func setValue(_ value: Int, controlIdentifier: String) -> Bool
    func unavailableVolumeIDs(
        values: [String: Int],
        previousValues: [String: Int],
        force: Bool
    ) -> Set<String>
}

extension SystemNowPlayingProvider: NowPlayingProviding {}
extension SystemMetricsProvider: SystemMetricsProviding {}

extension SystemMediaController: MediaControlling {
    var actionsAvailable: Bool { Self.mediaActionsAvailable }

    func supports(actionIdentifier: String) -> Bool {
        Self.supports(actionIdentifier: actionIdentifier)
    }

    func unavailableVolumeIDs(
        values: [String: Int],
        previousValues: [String: Int],
        force: Bool
    ) -> Set<String> {
        Self.unavailableVolumeIDs(values: values, previousValues: previousValues, force: force)
    }
}
