import Darwin
import Foundation
import IOKit.ps

struct CompanionSystemMetricsSnapshot: Equatable {
    let generation: UInt32
    let cpuUsagePercent: Double
    let memoryUsagePercent: Double
    let storageUsagePercent: Double
    let batteryPercent: Double?
}

@MainActor
final class SystemMetricsProvider {
    var onSnapshot: ((CompanionSystemMetricsSnapshot) -> Void)?

    private var timer: Timer?
    private var generation: UInt32 = 0
    private var previousCPUTicks: (active: UInt64, total: UInt64)?
    private(set) var lastSnapshot: CompanionSystemMetricsSnapshot?

    func start() {
        stop()
        refresh()
        timer = Timer.scheduledTimer(withTimeInterval: 2, repeats: true) { [weak self] _ in
            Task { @MainActor in self?.refresh() }
        }
    }

    func stop() {
        timer?.invalidate()
        timer = nil
        previousCPUTicks = nil
    }

    func refresh() {
        guard let cpu = sampleCPUUsagePercent(),
              let memory = Self.memoryUsagePercent(),
              let storage = Self.storageUsagePercent() else { return }
        generation &+= 1
        if generation == 0 { generation = 1 }
        let snapshot = CompanionSystemMetricsSnapshot(
            generation: generation,
            cpuUsagePercent: cpu,
            memoryUsagePercent: memory,
            storageUsagePercent: storage,
            batteryPercent: Self.batteryPercent()
        )
        lastSnapshot = snapshot
        onSnapshot?(snapshot)
    }

    private func sampleCPUUsagePercent() -> Double? {
        var load = host_cpu_load_info_data_t()
        var count = mach_msg_type_number_t(
            MemoryLayout<host_cpu_load_info_data_t>.stride / MemoryLayout<integer_t>.stride
        )
        let result = withUnsafeMutablePointer(to: &load) { pointer in
            pointer.withMemoryRebound(to: integer_t.self, capacity: Int(count)) {
                host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, $0, &count)
            }
        }
        guard result == KERN_SUCCESS else { return nil }
        let ticks = load.cpu_ticks
        let active = UInt64(ticks.0) + UInt64(ticks.1) + UInt64(ticks.3)
        let total = active + UInt64(ticks.2)
        defer { previousCPUTicks = (active, total) }
        guard let previous = previousCPUTicks, total > previous.total else { return 0 }
        let activeDelta = active >= previous.active ? active - previous.active : 0
        let totalDelta = total - previous.total
        return Self.clampedPercent(Double(activeDelta) * 100 / Double(totalDelta))
    }

    private static func memoryUsagePercent() -> Double? {
        var statistics = vm_statistics64_data_t()
        var count = mach_msg_type_number_t(
            MemoryLayout<vm_statistics64_data_t>.stride / MemoryLayout<integer_t>.stride
        )
        let result = withUnsafeMutablePointer(to: &statistics) { pointer in
            pointer.withMemoryRebound(to: integer_t.self, capacity: Int(count)) {
                host_statistics64(mach_host_self(), HOST_VM_INFO64, $0, &count)
            }
        }
        guard result == KERN_SUCCESS else { return nil }
        var pageSize: vm_size_t = 0
        guard host_page_size(mach_host_self(), &pageSize) == KERN_SUCCESS else { return nil }
        let usedPages = UInt64(statistics.active_count)
            + UInt64(statistics.wire_count)
            + UInt64(statistics.compressor_page_count)
        let usedBytes = Double(usedPages) * Double(pageSize)
        let totalBytes = Double(ProcessInfo.processInfo.physicalMemory)
        guard totalBytes > 0 else { return nil }
        return clampedPercent(usedBytes * 100 / totalBytes)
    }

    private static func storageUsagePercent() -> Double? {
        let keys: Set<URLResourceKey> = [.volumeTotalCapacityKey, .volumeAvailableCapacityForImportantUsageKey]
        guard let values = try? URL(fileURLWithPath: "/").resourceValues(forKeys: keys),
              let total = values.volumeTotalCapacity,
              let available = values.volumeAvailableCapacityForImportantUsage,
              total > 0 else { return nil }
        return clampedPercent((1 - Double(available) / Double(total)) * 100)
    }

    private static func batteryPercent() -> Double? {
        guard let info = IOPSCopyPowerSourcesInfo()?.takeRetainedValue(),
              let sources = IOPSCopyPowerSourcesList(info)?.takeRetainedValue() as? [CFTypeRef] else {
            return nil
        }
        for source in sources {
            guard let description = IOPSGetPowerSourceDescription(info, source)?.takeUnretainedValue()
                    as? [String: Any],
                  let current = description[kIOPSCurrentCapacityKey] as? NSNumber,
                  let maximum = description[kIOPSMaxCapacityKey] as? NSNumber,
                  maximum.doubleValue > 0 else { continue }
            return clampedPercent(current.doubleValue * 100 / maximum.doubleValue)
        }
        return nil
    }

    private static func clampedPercent(_ value: Double) -> Double {
        min(100, max(0, value.isFinite ? value : 0))
    }
}
