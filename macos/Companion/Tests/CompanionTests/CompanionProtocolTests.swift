import Foundation
import XCTest
@testable import Companion

final class CompanionProtocolTests: XCTestCase {
    func testSharedWireFixtures() throws {
        let root = URL(fileURLWithPath: #filePath).deletingLastPathComponent()
            .deletingLastPathComponent().deletingLastPathComponent()
            .deletingLastPathComponent().deletingLastPathComponent()
        let data = try Data(contentsOf: root.appendingPathComponent("compatibility/fixtures/companion_protocol_v3.json"))
        let fixtures = try XCTUnwrap(JSONSerialization.jsonObject(with: data) as? [[String: Any]])
        for fixture in fixtures {
            let message = try JSONSerialization.data(withJSONObject: XCTUnwrap(fixture["message"]))
            let direction = try XCTUnwrap(CompanionProtocolDirection(rawValue: fixture["direction"] as! String))
            let state = try XCTUnwrap(CompanionProtocolState(rawValue: fixture["state"] as! String))
            XCTAssertEqual(CompanionProtocolDecoder.decode(message, direction: direction, state: state) != nil,
                           fixture["valid"] as? Bool, fixture["name"] as! String)
        }
    }
}
