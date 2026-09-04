import CoreGraphics
import XCTest
@testable import Companion

final class CompanionKeyboardShortcutTests: XCTestCase {
    func testAccessibilityPromptIsRequestedOnlyOnceWhileUntrusted() {
        var promptCount = 0
        let authorizer = CompanionAccessibilityAuthorizer(
            isProcessTrusted: { false },
            requestPrompt: { promptCount += 1 }
        )

        XCTAssertFalse(authorizer.isTrusted())
        XCTAssertFalse(authorizer.isTrusted())
        XCTAssertEqual(promptCount, 1)
    }

    func testAccessibilityTrustDoesNotPrompt() {
        var promptCount = 0
        let authorizer = CompanionAccessibilityAuthorizer(
            isProcessTrusted: { true },
            requestPrompt: { promptCount += 1 }
        )

        XCTAssertTrue(authorizer.isTrusted())
        XCTAssertEqual(promptCount, 0)
    }

    func testMapsEveryWindowActionToItsDocumentedShortcut() throws {
        let fnControl: CGEventFlags = [.maskSecondaryFn, .maskControl]
        let fnControlShift: CGEventFlags = [.maskSecondaryFn, .maskControl, .maskShift]
        let fnControlOptionShift: CGEventFlags = [.maskSecondaryFn, .maskControl, .maskAlternate, .maskShift]
        let expected: [(String, CGKeyCode, CGEventFlags, Bool)] = [
            ("window.close", 13, [.maskCommand], false),
            ("window.minimize", 46, [.maskCommand], false),
            ("window.hide", 4, [.maskCommand], false),
            ("window.fullscreen", 3, [.maskControl, .maskCommand], false),
            ("window.fill", 3, fnControl, true),
            ("window.center", 8, fnControl, true),
            ("window.left", 123, fnControl, true),
            ("window.right", 124, fnControl, true),
            ("window.top", 126, fnControl, true),
            ("window.bottom", 125, fnControl, true),
            ("window.restore", 15, fnControl, true),
            ("window.arrange.left-right", 123, fnControlShift, true),
            ("window.arrange.right-left", 124, fnControlShift, true),
            ("window.arrange.top-bottom", 126, fnControlShift, true),
            ("window.arrange.bottom-top", 125, fnControlShift, true),
            ("window.arrange.left-quarters", 123, fnControlOptionShift, true),
            ("window.arrange.right-quarters", 124, fnControlOptionShift, true),
            ("window.arrange.top-quarters", 126, fnControlOptionShift, true),
            ("window.arrange.bottom-quarters", 125, fnControlOptionShift, true),
        ]

        for (identifier, keyCode, flags, requiresMacOS15) in expected {
            let shortcut = try XCTUnwrap(CompanionKeyboardShortcut(actionIdentifier: identifier), identifier)
            XCTAssertEqual(shortcut.keyCode, keyCode, identifier)
            XCTAssertEqual(shortcut.flags, flags, identifier)
            XCTAssertEqual(shortcut.requiresMacOS15, requiresMacOS15, identifier)
        }
    }

    func testRejectsUnknownWindowActionsAndKeepsFreeFormFnUnavailable() {
        XCTAssertNil(CompanionKeyboardShortcut(actionIdentifier: "window.top-left"))
        XCTAssertNil(CompanionKeyboardShortcut(actionIdentifier: "window.arrange"))
        XCTAssertNil(CompanionKeyboardShortcut(actionIdentifier: "shortcut.fn+control+f"))
    }

    func testGatesOnlyTilingActionsToMacOS15() throws {
        let macOS13 = OperatingSystemVersion(majorVersion: 13, minorVersion: 0, patchVersion: 0)
        let macOS15 = OperatingSystemVersion(majorVersion: 15, minorVersion: 0, patchVersion: 0)
        let close = try XCTUnwrap(CompanionKeyboardShortcut(actionIdentifier: "window.close"))
        let tile = try XCTUnwrap(CompanionKeyboardShortcut(actionIdentifier: "window.left"))
        XCTAssertTrue(close.isSupported(on: macOS13))
        XCTAssertFalse(tile.isSupported(on: macOS13))
        XCTAssertTrue(tile.isSupported(on: macOS15))
    }
}
