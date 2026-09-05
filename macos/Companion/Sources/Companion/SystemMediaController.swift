import CoreAudio
import Foundation
#if !APP_STORE
import MediaRemoteShim
#endif

protocol MediaCommandProviding {
    var isAvailable: Bool { get }
    func send(command: UInt32) -> Bool
}

struct MediaRemoteCommandSource: MediaCommandProviding {
#if APP_STORE
    var isAvailable: Bool { false }
    func send(command: UInt32) -> Bool { false }
#else
    var isAvailable: Bool { ECMediaRemoteBridge.isCommandAvailable() }
    func send(command: UInt32) -> Bool { ECMediaRemoteBridge.sendCommand(command) }
#endif
}

@MainActor
final class SystemMediaController {
    static let playPauseID = CompanionCapabilities.mediaPlayPauseID
    static let outputVolumeID = "media.output_volume"
    static let inputVolumeID = "media.input_volume"
    static let volumeControlIDs = Set([outputVolumeID, inputVolumeID])
    static var mediaActionsAvailable: Bool {
#if APP_STORE
        false
#else
        MediaRemoteCommandSource().isAvailable
#endif
    }

    private let commandSource: MediaCommandProviding

    init(commandSource: MediaCommandProviding = MediaRemoteCommandSource()) {
        self.commandSource = commandSource
    }

    static func unavailableVolumeIDs(
        values: [String: Int],
        previousValues: [String: Int],
        force: Bool
    ) -> Set<String> {
        let candidates = force ? volumeControlIDs : Set(previousValues.keys)
        return candidates.subtracting(values.keys)
    }

    private enum RemoteCommand: UInt32 {
        case togglePlayPause = 2
        case nextTrack = 4
        case previousTrack = 5
    }

    static func supports(actionIdentifier: String) -> Bool {
        CompanionCapabilities.mediaCommandByActionID[actionIdentifier] != nil
    }

    func perform(actionIdentifier: String) -> Bool {
#if APP_STORE
        return false
#else
        let command: RemoteCommand
        switch CompanionCapabilities.mediaCommandByActionID[actionIdentifier] {
        case "togglePlayPause": command = .togglePlayPause
        case "previousTrack": command = .previousTrack
        case "nextTrack": command = .nextTrack
        default: return false
        }
        return commandSource.isAvailable && commandSource.send(command: command.rawValue)
#endif
    }

    func values() -> [String: Int] {
        var result: [String: Int] = [:]
        if let output = volume(scope: kAudioDevicePropertyScopeOutput) {
            result[Self.outputVolumeID] = Int((output * 100).rounded()).clampedPercentage
        }
        if let input = volume(scope: kAudioDevicePropertyScopeInput) {
            result[Self.inputVolumeID] = Int((input * 100).rounded()).clampedPercentage
        }
        return result
    }

    func setValue(_ value: Int, controlIdentifier: String) -> Bool {
        let scope: AudioObjectPropertyScope
        switch controlIdentifier {
        case Self.outputVolumeID: scope = kAudioDevicePropertyScopeOutput
        case Self.inputVolumeID: scope = kAudioDevicePropertyScopeInput
        default: return false
        }
        return setVolume(Float32(value.clampedPercentage) / 100, scope: scope)
    }

    private func defaultDevice(scope: AudioObjectPropertyScope) -> AudioDeviceID? {
        let selector = scope == kAudioDevicePropertyScopeInput
            ? kAudioHardwarePropertyDefaultInputDevice
            : kAudioHardwarePropertyDefaultOutputDevice
        var address = AudioObjectPropertyAddress(
            mSelector: selector,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain
        )
        var device = AudioDeviceID(kAudioObjectUnknown)
        var size = UInt32(MemoryLayout<AudioDeviceID>.size)
        guard AudioObjectGetPropertyData(
            AudioObjectID(kAudioObjectSystemObject), &address, 0, nil, &size, &device
        ) == noErr, device != kAudioObjectUnknown else { return nil }
        return device
    }

    private func volume(scope: AudioObjectPropertyScope) -> Float32? {
        guard let device = defaultDevice(scope: scope) else { return nil }
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyVolumeScalar,
            mScope: scope,
            mElement: kAudioObjectPropertyElementMain
        )
        guard AudioObjectHasProperty(device, &address) else { return nil }
        var settable = DarwinBoolean(false)
        guard AudioObjectIsPropertySettable(device, &address, &settable) == noErr,
              settable.boolValue else { return nil }
        var value: Float32 = 0
        var size = UInt32(MemoryLayout<Float32>.size)
        guard AudioObjectGetPropertyData(device, &address, 0, nil, &size, &value) == noErr else { return nil }
        return value
    }

    private func setVolume(_ value: Float32, scope: AudioObjectPropertyScope) -> Bool {
        guard let device = defaultDevice(scope: scope) else { return false }
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyVolumeScalar,
            mScope: scope,
            mElement: kAudioObjectPropertyElementMain
        )
        guard AudioObjectHasProperty(device, &address) else { return false }
        var settable = DarwinBoolean(false)
        guard AudioObjectIsPropertySettable(device, &address, &settable) == noErr,
              settable.boolValue else { return false }
        var scalar = value
        return AudioObjectSetPropertyData(
            device, &address, 0, nil, UInt32(MemoryLayout<Float32>.size), &scalar
        ) == noErr
    }
}

private extension Int {
    var clampedPercentage: Int { Swift.min(100, Swift.max(0, self)) }
}
