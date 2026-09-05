import CoreGraphics
import XCTest
@testable import Companion

final class CompanionWindowDetectorTests: XCTestCase {
    func testVisibleApplicationWindowIsDetected() {
        let pid = pid_t(1234)
        let windows: [[String: Any]] = [[
            kCGWindowOwnerPID as String: NSNumber(value: pid),
            kCGWindowLayer as String: NSNumber(value: 0),
            kCGWindowAlpha as String: NSNumber(value: 1),
        ]]

        XCTAssertTrue(CompanionWindowDetector.hasVisibleWindow(for: pid, in: windows))
    }

    func testClosedOrNonApplicationWindowsAreIgnored() {
        let pid = pid_t(1234)
        let windows: [[String: Any]] = [
            [
                kCGWindowOwnerPID as String: NSNumber(value: pid),
                kCGWindowLayer as String: NSNumber(value: 1),
                kCGWindowAlpha as String: NSNumber(value: 1),
            ],
            [
                kCGWindowOwnerPID as String: NSNumber(value: pid + 1),
                kCGWindowLayer as String: NSNumber(value: 0),
                kCGWindowAlpha as String: NSNumber(value: 1),
            ],
        ]

        XCTAssertFalse(CompanionWindowDetector.hasVisibleWindow(for: pid, in: windows))
    }
}
