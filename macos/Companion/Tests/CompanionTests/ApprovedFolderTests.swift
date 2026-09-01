import Foundation
import XCTest
@testable import Companion

final class ApprovedFolderTests: XCTestCase {
    func testFolderActionIdentifierRoundTripsWithoutExposingPath() throws {
        let identifier = try XCTUnwrap(UUID(uuidString: "00000000-0000-0000-0000-000000000001"))
        let folder = ApprovedFolder(id: identifier, name: "Projects", path: "/Users/example/Projects")

        XCTAssertEqual(folder.actionIdentifier, "folder.00000000-0000-0000-0000-000000000001")
        XCTAssertEqual(ApprovedFolder.identifier(from: folder.actionIdentifier), identifier)
        XCTAssertFalse(folder.actionIdentifier.contains("Users"))
        XCTAssertNil(ApprovedFolder.identifier(from: "com.apple.finder"))
    }
}
