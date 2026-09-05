import AppKit
import CoreGraphics

enum CompanionWindowDetector {
    static func hasVisibleWindow(for application: NSRunningApplication) -> Bool {
        guard let windows = CGWindowListCopyWindowInfo(.optionOnScreenOnly, kCGNullWindowID)
                as? [[String: Any]] else { return false }
        return hasVisibleWindow(for: application.processIdentifier, in: windows)
    }

    static func hasVisibleWindow(for processIdentifier: pid_t, in windows: [[String: Any]]) -> Bool {
        windows.contains { window in
            guard let ownerPID = window[kCGWindowOwnerPID as String] as? NSNumber,
                  ownerPID.int32Value == processIdentifier,
                  let layer = window[kCGWindowLayer as String] as? NSNumber,
                  layer.intValue == 0 else { return false }
            return (window[kCGWindowAlpha as String] as? NSNumber)?.doubleValue ?? 1 > 0
        }
    }
}
