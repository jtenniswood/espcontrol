import XCTest
@testable import Companion

final class CompanionTransportPolicyTests: XCTestCase {
    func testReconnectBackoffGrowsAndCapsWithBoundedJitter() {
        XCTAssertEqual(ReconnectBackoff.delaySeconds(attempt: 0, randomUnit: 0.5), 1)
        XCTAssertEqual(ReconnectBackoff.delaySeconds(attempt: 3, randomUnit: 0.5), 8)
        XCTAssertEqual(ReconnectBackoff.delaySeconds(attempt: 10, randomUnit: 1), 30)
        XCTAssertEqual(ReconnectBackoff.delaySeconds(attempt: -1, randomUnit: 0), 0.8)
    }

    func testMetricsPublishOnlyForMeaningfulChangeOrHeartbeat() {
        let initial = snapshot(generation: 1, cpu: 20, memory: 40, storage: 60, battery: 80, network: 100)
        let noise = snapshot(generation: 2, cpu: 20.4, memory: 40.2, storage: 60.05, battery: 80.5, network: 120)
        let changed = snapshot(generation: 3, cpu: 21, memory: 40, storage: 60, battery: 80, network: 100)

        XCTAssertTrue(CompanionConnection.shouldPublishSystemMetrics(
            previous: nil, current: initial, elapsedSeconds: 0
        ))
        XCTAssertFalse(CompanionConnection.shouldPublishSystemMetrics(
            previous: initial, current: noise, elapsedSeconds: 2
        ))
        XCTAssertTrue(CompanionConnection.shouldPublishSystemMetrics(
            previous: initial, current: changed, elapsedSeconds: 2
        ))
        XCTAssertTrue(CompanionConnection.shouldPublishSystemMetrics(
            previous: initial, current: noise, elapsedSeconds: 30
        ))
    }

    private func snapshot(
        generation: UInt32, cpu: Double, memory: Double, storage: Double,
        battery: Double?, network: Double?
    ) -> CompanionSystemMetricsSnapshot {
        CompanionSystemMetricsSnapshot(
            generation: generation,
            cpuUsagePercent: cpu,
            memoryUsagePercent: memory,
            storageUsagePercent: storage,
            batteryPercent: battery,
            networkThroughputKBps: network
        )
    }
}
