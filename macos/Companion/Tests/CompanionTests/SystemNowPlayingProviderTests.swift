import AppKit
import XCTest
@testable import Companion

private final class FakeMediaRemoteSource: MediaRemoteProviding {
    var isAvailable = true
    var payload: [AnyHashable: Any]?
    var pid: NSNumber?
    var isPlaying: NSNumber?
    var observer: (() -> Void)?
    private(set) var fetchCount = 0
    private(set) var stopped = false

    func startObserving(_ handler: @escaping () -> Void) -> Bool {
        observer = handler
        return isAvailable
    }
    func stopObserving() { stopped = true }
    func fetch(_ completion: @escaping ([AnyHashable: Any]?, NSNumber?, NSNumber?) -> Void) {
        fetchCount += 1
        completion(payload, pid, isPlaying)
    }
}

@MainActor
final class SystemNowPlayingProviderTests: XCTestCase {
func testMapsSnapshotAndDeduplicatesSameNotificationAndPoll() {
    let source = FakeMediaRemoteSource()
    source.payload = [
        "kMRMediaRemoteNowPlayingInfoTitle": "Track",
        "kMRMediaRemoteNowPlayingInfoArtist": "Artist",
        "kMRMediaRemoteNowPlayingInfoAlbum": "Album",
        "kMRMediaRemoteNowPlayingInfoUniqueIdentifier": "track-1",
        "kMRMediaRemoteNowPlayingInfoDuration": 125.5,
        "kMRMediaRemoteNowPlayingInfoElapsedTime": 12.25,
        "kMRMediaRemoteNowPlayingInfoPlaybackRate": 1.0,
    ]
    let provider = SystemNowPlayingProvider(source: source)
    var snapshots: [CompanionNowPlayingSnapshot] = []
    provider.onSnapshot = { snapshots.append($0) }
    provider.refresh()
    provider.refresh()
    XCTAssertEqual(snapshots.count, 1)
    XCTAssertEqual(snapshots[0].generation, 1)
    XCTAssertEqual(snapshots[0].state, .playing)
    XCTAssertEqual(snapshots[0].title, "Track")
    XCTAssertEqual(snapshots[0].durationMilliseconds, 125_500)
    XCTAssertEqual(snapshots[0].positionMilliseconds, 12_250)
}

func testGenerationChangesOnlyWhenContentIdentityChanges() {
    let source = FakeMediaRemoteSource()
    let provider = SystemNowPlayingProvider(source: source)
    var snapshots: [CompanionNowPlayingSnapshot] = []
    provider.onSnapshot = { snapshots.append($0) }
    source.payload = ["Title": "One", "UniqueIdentifier": "one", "PlaybackRate": 1]
    provider.refresh()
    source.payload = ["Title": "One", "UniqueIdentifier": "one", "PlaybackRate": 0]
    provider.refresh()
    source.payload = ["Title": "Two", "UniqueIdentifier": "two", "PlaybackRate": 1]
    provider.refresh()
    XCTAssertEqual(snapshots.map(\.generation), [1, 1, 2])
    XCTAssertEqual(snapshots.map(\.state), [.playing, .paused, .playing])
}

func testMissingMetadataStillProducesSafeStoppedSnapshot() {
    let source = FakeMediaRemoteSource()
    source.payload = ["Duration": 10]
    let provider = SystemNowPlayingProvider(source: source)
    var snapshot: CompanionNowPlayingSnapshot?
    provider.onSnapshot = { snapshot = $0 }
    provider.refresh()
    XCTAssertEqual(snapshot?.state, .stopped)
    XCTAssertEqual(snapshot?.title, "")
    XCTAssertNil(snapshot?.artworkJPEG)
}

func testUnavailableSourcePublishesDiagnosticSnapshot() {
    let source = FakeMediaRemoteSource()
    source.isAvailable = false
    let provider = SystemNowPlayingProvider(source: source)
    var status = ""
    var snapshot: CompanionNowPlayingSnapshot?
    provider.onStatus = { status = $0 }
    provider.onSnapshot = { snapshot = $0 }
    provider.start()
    XCTAssertEqual(snapshot?.state, .unavailable)
    XCTAssertTrue(status.contains("unavailable"))
}

func testDisablingSharingPublishesUnavailableAfterActiveTrack() {
    let source = FakeMediaRemoteSource()
    source.payload = ["Title": "Track", "UniqueIdentifier": "track", "PlaybackRate": 1]
    let provider = SystemNowPlayingProvider(source: source)
    var states: [CompanionPlaybackState] = []
    provider.onSnapshot = { states.append($0.state) }
    provider.refresh()
    provider.stopAndPublishUnavailable()
    XCTAssertEqual(states, [.playing, .unavailable])
    XCTAssertTrue(source.stopped)
}

func testArtworkIsLetterboxedAsBoundedJPEG() throws {
    let sourceSize = NSSize(width: 800, height: 400)
    let image = NSImage(size: sourceSize)
    image.lockFocus()
    NSColor.red.setFill()
    NSRect(origin: .zero, size: sourceSize).fill()
    image.unlockFocus()
    let sourceData = try XCTUnwrap(image.tiffRepresentation)
    let jpeg = try XCTUnwrap(SystemNowPlayingProvider.normalizedArtwork(sourceData))
    XCTAssertLessThanOrEqual(jpeg.count, 256 * 1024)
    XCTAssertEqual(Array(jpeg.prefix(2)), [0xff, 0xd8])
    let decoded = try XCTUnwrap(NSImage(data: jpeg))
    XCTAssertEqual(decoded.size.width, 480, accuracy: 1)
    XCTAssertEqual(decoded.size.height, 480, accuracy: 1)
}

func testUtf8FieldsNeverExceedProtocolLimit() {
    let clamped = SystemNowPlayingProvider.clamped(String(repeating: "🎵", count: 100), bytes: 256)
    XCTAssertLessThanOrEqual(clamped.utf8.count, 256)
    XCTAssertNotNil(clamped.data(using: .utf8))
}

func testNonFiniteAndOversizedPlaybackTimesAreSafe() {
    XCTAssertEqual(SystemNowPlayingProvider.clampedMilliseconds(.nan), 0)
    XCTAssertEqual(SystemNowPlayingProvider.clampedMilliseconds(.infinity), 0)
    XCTAssertEqual(SystemNowPlayingProvider.clampedMilliseconds(-1), 0)
    XCTAssertEqual(SystemNowPlayingProvider.clampedMilliseconds(100_000), 86_400_000)
}

func testRestartRepublishesAnUnchangedSnapshot() {
    let source = FakeMediaRemoteSource()
    source.payload = ["Title": "Paused", "UniqueIdentifier": "paused", "PlaybackRate": 0]
    let provider = SystemNowPlayingProvider(source: source)
    var snapshots: [CompanionNowPlayingSnapshot] = []
    provider.onSnapshot = { snapshots.append($0) }
    provider.start()
    provider.stop()
    provider.start()
    provider.stop()
    XCTAssertEqual(snapshots.count, 2)
    XCTAssertEqual(snapshots.map(\.generation), [1, 1])
}

func testPlaybackStateChangesOnlyAfterAConfirmedSnapshot() {
    let source = FakeMediaRemoteSource()
    source.payload = ["Title": "Track", "UniqueIdentifier": "track", "PlaybackRate": 1]
    let provider = SystemNowPlayingProvider(source: source)
    var states: [CompanionPlaybackState] = []
    provider.onSnapshot = { states.append($0.state) }
    provider.refresh()

    XCTAssertEqual(states, [.playing])

    source.payload = ["Title": "Track", "UniqueIdentifier": "track", "PlaybackRate": 0]
    provider.refresh()
    XCTAssertEqual(states, [.playing, .paused])
}

func testExplicitMediaRemotePlaybackStateWinsOverStaleRate() {
    let source = FakeMediaRemoteSource()
    source.payload = ["Title": "Track", "UniqueIdentifier": "track", "PlaybackRate": 0]
    source.isPlaying = true
    let provider = SystemNowPlayingProvider(source: source)
    var state: CompanionPlaybackState?
    provider.onSnapshot = { state = $0.state }
    provider.refresh()
    XCTAssertEqual(state, .playing)

    source.isPlaying = false
    provider.refresh()
    XCTAssertEqual(state, .paused)
}

func testExplicitStoppedStateDoesNotRequireAPlaybackRate() {
    let source = FakeMediaRemoteSource()
    source.payload = ["UniqueIdentifier": "track"]
    source.isPlaying = false
    let provider = SystemNowPlayingProvider(source: source)
    var state: CompanionPlaybackState?
    provider.onSnapshot = { state = $0.state }
    provider.refresh()
    XCTAssertEqual(state, .stopped)
}

func testPanelWebServerURLDropsCompanionPort() {
    XCTAssertEqual(
        CompanionStore.panelWebServerURL(from: "192.168.6.100:9443")?.absoluteString,
        "http://192.168.6.100"
    )
    XCTAssertEqual(
        CompanionStore.panelWebServerURL(from: "https://panel.example:9443")?.absoluteString,
        "https://panel.example"
    )
}
}
