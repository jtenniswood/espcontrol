import ApplicationServices
import CoreGraphics
import Foundation

struct CompanionKeyboardShortcut {
    static let actionPrefix = "shortcut."
    static let windowActionPrefix = "window."

    let keyCode: CGKeyCode
    let flags: CGEventFlags
    let requiresMacOS15: Bool

    init?(actionIdentifier: String) {
        if let action = CompanionCapabilities.windowActions[actionIdentifier], let keyCode = Self.keyCodes[action.key] {
            self.keyCode = keyCode
            self.flags = action.flags
            self.requiresMacOS15 = action.minimumMacOS >= 15
            return
        }
        guard actionIdentifier.hasPrefix(Self.actionPrefix) else { return nil }
        var parts = actionIdentifier.dropFirst(Self.actionPrefix.count).split(separator: "+").map(String.init)
        guard parts.count >= 2, parts.count <= 5, let key = parts.popLast(),
              let keyCode = Self.keyCodes[key] else { return nil }

        var flags: CGEventFlags = []
        var seen = Set<String>()
        for modifier in parts {
            guard seen.insert(modifier).inserted else { return nil }
            switch modifier {
            case "command": flags.insert(.maskCommand)
            case "control": flags.insert(.maskControl)
            case "option": flags.insert(.maskAlternate)
            case "shift": flags.insert(.maskShift)
            default: return nil
            }
        }
        guard flags.contains(.maskCommand) || flags.contains(.maskControl) || flags.contains(.maskAlternate) else {
            return nil
        }
        self.keyCode = keyCode
        self.flags = flags
        self.requiresMacOS15 = false
    }

    func isSupported(on version: OperatingSystemVersion) -> Bool {
        !requiresMacOS15 || version.majorVersion >= 15
    }

    func replay() -> Bool {
        guard isSupported(on: ProcessInfo.processInfo.operatingSystemVersion) else { return false }
        guard AXIsProcessTrusted() else {
            let prompt = [kAXTrustedCheckOptionPrompt.takeUnretainedValue() as String: true] as CFDictionary
            AXIsProcessTrustedWithOptions(prompt)
            return false
        }
        guard let source = CGEventSource(stateID: .hidSystemState),
              let keyDown = CGEvent(keyboardEventSource: source, virtualKey: keyCode, keyDown: true),
              let keyUp = CGEvent(keyboardEventSource: source, virtualKey: keyCode, keyDown: false) else {
            return false
        }
        keyDown.flags = flags
        keyUp.flags = flags
        keyDown.post(tap: .cghidEventTap)
        keyUp.post(tap: .cghidEventTap)
        return true
    }

    private static let keyCodes: [String: CGKeyCode] = [
        "a": 0, "s": 1, "d": 2, "f": 3, "h": 4, "g": 5, "z": 6, "x": 7,
        "c": 8, "v": 9, "b": 11, "q": 12, "w": 13, "e": 14, "r": 15,
        "y": 16, "t": 17, "1": 18, "2": 19, "3": 20, "4": 21, "6": 22,
        "5": 23, "keyequal": 24, "9": 25, "7": 26, "keyminus": 27, "8": 28,
        "0": 29, "keybracketright": 30, "o": 31, "u": 32, "keybracketleft": 33,
        "i": 34, "p": 35, "enter": 36, "l": 37, "j": 38, "keyquote": 39,
        "k": 40, "keysemicolon": 41, "keybackslash": 42, "keycomma": 43, "keyslash": 44,
        "n": 45, "m": 46, "keyperiod": 47, "tab": 48, "space": 49,
        "keybackquote": 50, "delete": 51, "escape": 53, "forwarddelete": 117,
        "home": 115, "end": 119, "pageup": 116, "pagedown": 121,
        "left": 123, "right": 124, "down": 125, "up": 126,
        "f1": 122, "f2": 120, "f3": 99, "f4": 118, "f5": 96,
        "f6": 97, "f7": 98, "f8": 100, "f9": 101, "f10": 109,
        "f11": 103, "f12": 111, "f13": 105, "f14": 107, "f15": 113,
        "f16": 106, "f17": 64, "f18": 79, "f19": 80, "f20": 90,
    ]
}
