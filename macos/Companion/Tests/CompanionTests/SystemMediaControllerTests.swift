import XCTest
@testable import Companion

@MainActor
final class SystemMediaControllerTests: XCTestCase {
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
