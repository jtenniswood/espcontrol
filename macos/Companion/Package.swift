// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "EspControlCompanion",
    platforms: [.macOS(.v13)],
    products: [.executable(name: "EspControl Companion", targets: ["Companion"])],
    targets: [.executableTarget(name: "Companion", path: "Sources/Companion")]
)
