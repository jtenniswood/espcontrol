import CoreAudio
import Foundation
import MediaRemoteShim

@MainActor
final class SystemMediaController {
    static let outputVolumeID = "media.output_volume"
    static let inputVolumeID = "media.input_volume"

    private enum RemoteCommand: UInt32 {
        case togglePlayPause = 2
        case nextTrack = 4
        case previousTrack = 5
    }

    func perform(actionIdentifier: String) -> Bool {
        let command: RemoteCommand
        switch actionIdentifier {
        case "media.play_pause": command = .togglePlayPause
        case "media.previous": command = .previousTrack
        case "media.next": command = .nextTrack
        default: return false
        }
        return ECMediaRemoteBridge.sendCommand(command.rawValue)
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
        var values: [Float32] = []
        for element in [kAudioObjectPropertyElementMain, 1, 2] {
            var address = AudioObjectPropertyAddress(
                mSelector: kAudioDevicePropertyVolumeScalar,
                mScope: scope,
                mElement: AudioObjectPropertyElement(element)
            )
            guard AudioObjectHasProperty(device, &address) else { continue }
            var value: Float32 = 0
            var size = UInt32(MemoryLayout<Float32>.size)
            if AudioObjectGetPropertyData(device, &address, 0, nil, &size, &value) == noErr {
                if element == kAudioObjectPropertyElementMain { return value }
                values.append(value)
            }
        }
        guard !values.isEmpty else { return nil }
        return values.reduce(0, +) / Float32(values.count)
    }

    private func setVolume(_ value: Float32, scope: AudioObjectPropertyScope) -> Bool {
        guard let device = defaultDevice(scope: scope) else { return false }
        var changed = false
        for element in [kAudioObjectPropertyElementMain, 1, 2] {
            var address = AudioObjectPropertyAddress(
                mSelector: kAudioDevicePropertyVolumeScalar,
                mScope: scope,
                mElement: AudioObjectPropertyElement(element)
            )
            guard AudioObjectHasProperty(device, &address) else { continue }
            var settable = DarwinBoolean(false)
            guard AudioObjectIsPropertySettable(device, &address, &settable) == noErr,
                  settable.boolValue else { continue }
            var scalar = value
            if AudioObjectSetPropertyData(
                device, &address, 0, nil, UInt32(MemoryLayout<Float32>.size), &scalar
            ) == noErr {
                changed = true
                if element == kAudioObjectPropertyElementMain { return true }
            }
        }
        return changed
    }
}

private extension Int {
    var clampedPercentage: Int { Swift.min(100, Swift.max(0, self)) }
}
