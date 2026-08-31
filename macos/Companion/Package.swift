// swift-tools-version: 5.9
import PackageDescription

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
            dependencies: ["MediaRemoteShim"],
            path: "Sources/Companion"
        ),
        .testTarget(
            name: "CompanionTests",
            dependencies: ["Companion"],
            path: "Tests/CompanionTests"
        ),
    ]
)
