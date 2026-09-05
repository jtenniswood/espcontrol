// swift-tools-version: 6.0
import Foundation
import PackageDescription

let appStoreBuild = ProcessInfo.processInfo.environment["ESPCONTROL_APP_STORE"] == "1"

let package = Package(
    name: "EspControlCompanion",
    platforms: [.macOS(.v13)],
    products: [.executable(name: "EspControl Companion", targets: ["Companion"])],
    targets: [
        .target(
            name: "MediaRemoteShim",
            path: "Sources/MediaRemoteShim",
            publicHeadersPath: "include",
            linkerSettings: [.linkedFramework("Foundation")]
        ),
        .executableTarget(
            name: "Companion",
            dependencies: appStoreBuild ? [] : ["MediaRemoteShim"],
            path: "Sources/Companion",
            swiftSettings: appStoreBuild ? [.define("APP_STORE")] : [],
            linkerSettings: [
                .linkedFramework("IOKit"),
                .linkedFramework("SystemConfiguration"),
            ]
        ),
        .testTarget(
            name: "CompanionTests",
            dependencies: ["Companion"],
            path: "Tests/CompanionTests",
            swiftSettings: appStoreBuild ? [.define("APP_STORE")] : []
        ),
    ]
)
