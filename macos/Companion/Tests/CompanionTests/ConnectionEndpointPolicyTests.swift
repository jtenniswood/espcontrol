import XCTest
@testable import Companion

@MainActor
final class ConnectionEndpointPolicyTests: XCTestCase {
    func testAcceptsLocalNetworkHosts() {
        for host in ["192.168.1.20", "10.0.0.4", "172.16.0.8", "169.254.1.2", "::1", "[fd12::20]", "fd12::20", "panel.local"] {
            XCTAssertTrue(ConnectionEndpointPolicy.isLocalHost(host), host)
        }
    }

    func testRejectsPublicAndMalformedHosts() {
        for host in ["8.8.8.8", "172.15.0.8", "192.168.1", "010.0.0.1", ".10.0.0.1", "panel.example.com", ""] {
            XCTAssertFalse(ConnectionEndpointPolicy.isLocalHost(host), host)
        }
    }

    func testPanelWebserverURLUsesTheSameLocalHostPolicy() {
        XCTAssertNotNil(CompanionStore.panelWebServerURL(from: "192.168.1.20"))
        XCTAssertNil(CompanionStore.panelWebServerURL(from: "https://8.8.8.8"))
    }
}
