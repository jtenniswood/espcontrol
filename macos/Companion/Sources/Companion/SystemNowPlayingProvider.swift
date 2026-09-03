import AppKit
import CryptoKit
import Foundation
#if !APP_STORE
import MediaRemoteShim
#endif

enum CompanionPlaybackState: String, Codable {
    case playing, paused, stopped, unavailable
}

struct CompanionNowPlayingSnapshot: Equatable {
    let generation: UInt32
    let applicationIdentifier: String
    let applicationName: String
    let state: CompanionPlaybackState
    let contentIdentifier: String
    let title: String
    let artist: String
    let album: String
    let durationMilliseconds: Int64
    let positionMilliseconds: Int64
    let playbackRate: Double
    let artworkJPEG: Data?

    var artworkSHA256: String? {
        artworkJPEG.map { SHA256.hash(data: $0).map { String(format: "%02x", $0) }.joined() }
    }
}

protocol MediaRemoteProviding {
    var isAvailable: Bool { get }
    func startObserving(_ handler: @escaping () -> Void) -> Bool
    func stopObserving()
    func fetch(_ completion: @escaping ([AnyHashable: Any]?, NSNumber?, NSNumber?) -> Void)
}

#if !APP_STORE
struct MediaRemoteSystemSource: MediaRemoteProviding {
    var isAvailable: Bool { ECMediaRemoteBridge.isAvailable() }
    func startObserving(_ handler: @escaping () -> Void) -> Bool {
        ECMediaRemoteBridge.startObservingChanges(handler)
    }
    func stopObserving() { ECMediaRemoteBridge.stopObservingChanges() }
    func fetch(_ completion: @escaping ([AnyHashable: Any]?, NSNumber?, NSNumber?) -> Void) {
        ECMediaRemoteBridge.fetchNowPlaying(completion)
    }
}
#endif

#if APP_STORE
struct AppStoreMediaRemoteSource: MediaRemoteProviding {
    var isAvailable: Bool { false }
    func startObserving(_ handler: @escaping () -> Void) -> Bool { false }
    func stopObserving() {}
    func fetch(_ completion: @escaping ([AnyHashable: Any]?, NSNumber?) -> Void) {
        completion(nil, nil)
    }
}
#endif

@MainActor
final class SystemNowPlayingProvider {
    var onSnapshot: ((CompanionNowPlayingSnapshot) -> Void)?
    var onStatus: ((String) -> Void)?

    private var timer: Timer?
    private var generation: UInt32 = 0
    private var lastIdentity = ""
    private var lastState: CompanionPlaybackState = .unavailable
    private var lastSnapshot: CompanionNowPlayingSnapshot?
    // Tests and one-shot callers may explicitly refresh before observation starts;
    // stop() still invalidates every outstanding asynchronous result.
    private var active = true
    private var lifecycleToken: UInt64 = 0
    private let source: MediaRemoteProviding

    init(source: MediaRemoteProviding? = nil) {
#if APP_STORE
        self.source = source ?? AppStoreMediaRemoteSource()
#else
        self.source = source ?? MediaRemoteSystemSource()
#endif
    }

    var isAvailable: Bool { source.isAvailable }
    func start() {
        stop()
        guard isAvailable else {
            onStatus?("The macOS Now Playing system feed is unavailable")
            publishUnavailable()
            return
        }
        active = true
        lifecycleToken &+= 1
        // A restarted provider represents a new panel connection. The panel may
        // have discarded its runtime state even when the track is unchanged.
        lastSnapshot = nil
        _ = source.startObserving { [weak self] in self?.refresh() }
        timer = Timer.scheduledTimer(withTimeInterval: 2, repeats: true) { [weak self] _ in
            Task { @MainActor in self?.refresh() }
        }
        refresh()
    }

    func stop() {
        active = false
        lifecycleToken &+= 1
        timer?.invalidate()
        timer = nil
        source.stopObserving()
    }

    func stopAndPublishUnavailable() {
        stop()
        publishUnavailable()
    }

    func refresh() {
        guard active else { return }
        let token = lifecycleToken
        source.fetch { [weak self] raw, pid, isPlaying in
            guard let self, self.active, self.lifecycleToken == token else { return }
            guard let raw, !raw.isEmpty else {
                self.onStatus?("No active macOS Now Playing session")
                self.publishUnavailable()
                return
            }
            let values = Self.normalizedValues(raw)
            let title = Self.string(values, keys: ["title"])
            let artist = Self.string(values, keys: ["artist"])
            let album = Self.string(values, keys: ["album"])
            let contentIdentifier = Self.string(values, keys: ["uniqueidentifier", "contentidentifier"])
            let duration = Self.number(values, keys: ["duration"])
            let position = Self.number(values, keys: ["elapsedtime", "elapsed"])
            let rawRate = Self.number(values, keys: ["playbackrate", "rate"])
            let rate = rawRate.isFinite ? min(16, max(-16, rawRate)) : 0
            let state = Self.playbackState(values: values, rate: rate, isPlaying: isPlaying)
            let application: NSRunningApplication?
            if let processIdentifier = pid {
                application = NSRunningApplication(processIdentifier: processIdentifier.int32Value)
            } else {
                application = nil
            }
            let appIdentifier = Self.clamped(application?.bundleIdentifier ?? "", bytes: 256)
            let appName = Self.clamped(application?.localizedName ?? "Unknown application", bytes: 256)
            let identity = contentIdentifier.isEmpty
                ? [appIdentifier, title, artist, album].joined(separator: "\u{1f}")
                : [appIdentifier, contentIdentifier].joined(separator: "\u{1f}")
            if identity != lastIdentity {
                generation &+= 1
                lastIdentity = identity
            }
            lastState = state
            let artwork = Self.data(values, keys: ["artworkdata"]).flatMap(Self.normalizedArtwork)
            let snapshot = CompanionNowPlayingSnapshot(
                generation: generation,
                applicationIdentifier: appIdentifier,
                applicationName: appName,
                state: state,
                contentIdentifier: Self.clamped(contentIdentifier, bytes: 256),
                title: Self.clamped(title, bytes: 256),
                artist: Self.clamped(artist, bytes: 256),
                album: Self.clamped(album, bytes: 256),
                durationMilliseconds: Self.clampedMilliseconds(duration),
                positionMilliseconds: Self.clampedMilliseconds(position),
                playbackRate: rate,
                artworkJPEG: artwork
            )
            onStatus?("Sharing \(appName): \(snapshot.title.isEmpty ? "Now Playing" : snapshot.title)")
            if snapshot != lastSnapshot {
                lastSnapshot = snapshot
                onSnapshot?(snapshot)
            }
        }
    }

    private func publishUnavailable() {
        guard lastSnapshot?.state != .unavailable else { return }
        generation &+= 1
        lastIdentity = ""
        lastState = .unavailable
        let snapshot = CompanionNowPlayingSnapshot(
            generation: generation, applicationIdentifier: "", applicationName: "",
            state: .unavailable, contentIdentifier: "", title: "", artist: "", album: "",
            durationMilliseconds: 0, positionMilliseconds: 0, playbackRate: 0, artworkJPEG: nil)
        lastSnapshot = snapshot
        onSnapshot?(snapshot)
    }

    private static func normalizedValues(_ raw: [AnyHashable: Any]) -> [String: Any] {
        var result: [String: Any] = [:]
        for (key, value) in raw {
            let normalized = String(describing: key).lowercased().filter { $0.isLetter || $0.isNumber }
            result[normalized] = value
            if normalized.hasPrefix("kmrmediaremotenowplayinginfo") {
                result[String(normalized.dropFirst("kmrmediaremotenowplayinginfo".count))] = value
            }
        }
        return result
    }

    private static func string(_ values: [String: Any], keys: [String]) -> String {
        for key in keys {
            if let value = values[key] as? String { return value }
        }
        return ""
    }

    private static func number(_ values: [String: Any], keys: [String]) -> Double {
        for key in keys {
            if let value = values[key] as? NSNumber { return value.doubleValue }
        }
        return 0
    }

    static func playbackState(values: [String: Any], rate: Double,
                              isPlaying: NSNumber?) -> CompanionPlaybackState {
        if let isPlaying {
            return isPlaying.boolValue ? .playing : (string(values, keys: ["title"]).isEmpty ? .stopped : .paused)
        }
        return rate > 0 ? .playing : (string(values, keys: ["title"]).isEmpty ? .stopped : .paused)
    }

    private static func data(_ values: [String: Any], keys: [String]) -> Data? {
        for key in keys {
            if let value = values[key] as? Data { return value }
        }
        return nil
    }

    static func clamped(_ value: String, bytes: Int) -> String {
        var data = Data(value.utf8.prefix(bytes))
        while !data.isEmpty && String(data: data, encoding: .utf8) == nil { data.removeLast() }
        return String(data: data, encoding: .utf8) ?? ""
    }

    static func clampedMilliseconds(_ seconds: Double) -> Int64 {
        guard seconds.isFinite, seconds > 0 else { return 0 }
        return Int64(min(seconds, 86_400) * 1000)
    }

    static func normalizedArtwork(_ data: Data) -> Data? {
        guard let image = NSImage(data: data) else { return nil }
        let size = NSSize(width: 480, height: 480)
        let output = NSImage(size: size)
        output.lockFocus()
        NSColor.black.setFill()
        NSRect(origin: .zero, size: size).fill()
        let source = image.size
        guard source.width > 0, source.height > 0 else { output.unlockFocus(); return nil }
        let scale = min(size.width / source.width, size.height / source.height)
        let target = NSSize(width: source.width * scale, height: source.height * scale)
        let rect = NSRect(x: (size.width - target.width) / 2, y: (size.height - target.height) / 2,
                          width: target.width, height: target.height)
        image.draw(in: rect, from: .zero, operation: .copy, fraction: 1)
        output.unlockFocus()
        guard let tiff = output.tiffRepresentation, let bitmap = NSBitmapImageRep(data: tiff) else { return nil }
        for quality in stride(from: 0.86, through: 0.36, by: -0.1) {
            if let jpeg = bitmap.representation(using: .jpeg, properties: [.compressionFactor: quality]),
               jpeg.count <= 256 * 1024 { return jpeg }
        }
        return nil
    }
}
