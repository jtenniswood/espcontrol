import Foundation

struct ReconnectBackoff {
    static let maximumDelaySeconds = 30.0

    static func delaySeconds(attempt: Int, randomUnit: Double) -> Double {
        let exponent = min(max(attempt, 0), 5)
        let base = min(pow(2.0, Double(exponent)), maximumDelaySeconds)
        let jitter = 0.8 + min(max(randomUnit, 0), 1) * 0.4
        return min(base * jitter, maximumDelaySeconds)
    }
}
