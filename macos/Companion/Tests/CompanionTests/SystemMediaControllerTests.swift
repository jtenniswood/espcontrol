import XCTest
@testable import Companion

private final class FakeMediaCommandSource: MediaCommandProviding {
    var isAvailable = true
    var result = true
    private(set) var commands: [UInt32] = []

    func send(command: UInt32) -> Bool {
        commands.append(command)
        return result
    }
}

@MainActor
final class SystemMediaControllerTests: XCTestCase {
    func testReservedMediaActionsRouteToExpectedCommands() {
        let source = FakeMediaCommandSource()
        let controller = SystemMediaController(commandSource: source)

        XCTAssertTrue(controller.perform(actionIdentifier: SystemMediaController.playPauseID))
        XCTAssertTrue(controller.perform(actionIdentifier: "media.previous"))
        XCTAssertTrue(controller.perform(actionIdentifier: "media.next"))
        XCTAssertEqual(source.commands, [2, 5, 4])
    }

    func testUnavailableCommandDoesNotDispatch() {
        let source = FakeMediaCommandSource()
        source.isAvailable = false
        let controller = SystemMediaController(commandSource: source)

        XCTAssertFalse(controller.perform(actionIdentifier: SystemMediaController.playPauseID))
        XCTAssertTrue(source.commands.isEmpty)
    }

    func testFailedCommandIsReported() {
        let source = FakeMediaCommandSource()
        source.result = false
        let controller = SystemMediaController(commandSource: source)

        XCTAssertFalse(controller.perform(actionIdentifier: SystemMediaController.playPauseID))
        XCTAssertEqual(source.commands, [2])
    }

    func testForcedSnapshotMarksEveryMissingControlUnavailable() {
        let unavailable = SystemMediaController.unavailableVolumeIDs(
            values: [SystemMediaController.outputVolumeID: 45],
            previousValues: [:],
            force: true
        )

        XCTAssertEqual(unavailable, [SystemMediaController.inputVolumeID])
    }

    func testIncrementalSnapshotOnlyRemovesPreviouslyPublishedControls() {
        let unavailable = SystemMediaController.unavailableVolumeIDs(
            values: [:],
            previousValues: [SystemMediaController.outputVolumeID: 45],
            force: false
        )

        XCTAssertEqual(unavailable, [SystemMediaController.outputVolumeID])
    }
}
