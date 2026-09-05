import XCTest
@testable import Companion

final class CompanionPairingInputTests: XCTestCase {
    func testPastedCodeAcceptsLowercaseAndSurroundingWhitespace() {
        XCTAssertTrue(CompanionPairingInput.isValid(host: " display.local ", code: " abcdEFgh\n"))
        XCTAssertEqual(CompanionPairingInput.normalizedCode(" abcdEFgh\n"), "ABCD-EFGH")
        XCTAssertEqual(CompanionPairingInput.normalizedCode(" abcd-efgh\n"), "ABCD-EFGH")
        XCTAssertTrue(CompanionPairingInput.isValid(host: "display.local", code: "ABCD-EFGH"))
    }

    func testIncompleteOrNonASCIIInputCannotSubmitPairing() {
        for code in ["", "ABC", "ABCDEFGHI", "ABCD1234", "ABCD EFG", "ÄBCDEFGH", "ßabcdef", "ABC-DEFGH", "ABCD--EFGH"] {
            XCTAssertFalse(CompanionPairingInput.isValid(host: "192.168.1.20", code: code), code)
        }
        XCTAssertFalse(CompanionPairingInput.isValid(host: " \n", code: "ABCDEFGH"))
    }
}
