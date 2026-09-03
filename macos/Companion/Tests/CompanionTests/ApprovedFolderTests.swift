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

    func testBookmarkRepresentationDoesNotPersistFolderPath() throws {
        let identifier = try XCTUnwrap(UUID(uuidString: "00000000-0000-0000-0000-000000000002"))
        let folder = ApprovedFolder(id: identifier, name: "Projects", bookmarkData: Data([1, 2, 3]))
        let data = try JSONEncoder().encode(folder)
        let encoded = try XCTUnwrap(String(data: data, encoding: .utf8))

        XCTAssertTrue(encoded.contains("bookmarkData"))
        XCTAssertFalse(encoded.contains("path"))
        XCTAssertFalse(encoded.contains("Users"))
        XCTAssertEqual(try JSONDecoder().decode(ApprovedFolder.self, from: data), folder)
    }

    func testLegacyPathDataLoadsAsNeedingFolderReapproval() throws {
        let data = Data(#"{"id":"00000000-0000-0000-0000-000000000003","name":"Projects","path":"/Users/example/Projects"}"#.utf8)
        let folder = try JSONDecoder().decode(ApprovedFolder.self, from: data)

        XCTAssertTrue(folder.needsReapproval)
        XCTAssertEqual(folder.path, "/Users/example/Projects")
    }
}
