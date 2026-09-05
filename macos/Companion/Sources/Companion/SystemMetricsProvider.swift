import Darwin
import Foundation
import IOKit.ps
import SystemConfiguration

struct CompanionSystemMetricsSnapshot: Equatable, Sendable {
    let generation: UInt32
    let cpuUsagePercent: Double
    let memoryUsagePercent: Double
    let storageUsagePercent: Double
    let batteryPercent: Double?
    let networkThroughputKBps: Double?
}

@MainActor
final class SystemMetricsProvider {
    var onSnapshot: ((CompanionSystemMetricsSnapshot) -> Void)?

    private var timer: Timer?
    private var samplingTask: Task<Void, Never>?
    private var samplingSession: UInt64 = 0
    private var generation: UInt32 = 0
    private let sampler = SystemMetricsSampler()
    private(set) var lastSnapshot: CompanionSystemMetricsSnapshot?

    func start() {
        guard timer == nil else { return }
        stop()
        samplingSession &+= 1
        refresh()
        timer = Timer.scheduledTimer(withTimeInterval: 2, repeats: true) { [weak self] _ in
            Task { @MainActor [weak self] in self?.refresh() }
        }
    }

    func stop() {
        timer?.invalidate()
        timer = nil
        samplingTask?.cancel()
        samplingTask = nil
        samplingSession &+= 1
    }

    func refresh() {
        guard samplingTask == nil else { return }
        let session = samplingSession
        samplingTask = Task { [weak self, sampler] in
            let sample = await sampler.sample(session: session)
            guard !Task.isCancelled, let self, self.samplingSession == session else { return }
            self.samplingTask = nil
            guard let sample else { return }
            self.generation &+= 1
            if self.generation == 0 { self.generation = 1 }
            let snapshot = CompanionSystemMetricsSnapshot(
                generation: self.generation,
                cpuUsagePercent: sample.cpuUsagePercent,
                memoryUsagePercent: sample.memoryUsagePercent,
                storageUsagePercent: sample.storageUsagePercent,
                batteryPercent: sample.batteryPercent,
                networkThroughputKBps: sample.networkThroughputKBps
            )
            self.lastSnapshot = snapshot
            self.onSnapshot?(snapshot)
        }
    }
}

private struct SystemMetricsSample: Sendable {
    let cpuUsagePercent: Double
    let memoryUsagePercent: Double
    let storageUsagePercent: Double
    let batteryPercent: Double?
    let networkThroughputKBps: Double?
}

private actor SystemMetricsSampler {
    private var activeSession: UInt64?
    private var previousCPUTicks: (active: UInt64, total: UInt64)?
    private var previousNetworkCounters: NetworkCounters?

    private struct NetworkCounters {
        let interfaceName: String
        let receivedBytes: UInt32
        let sentBytes: UInt32
        let timestamp: TimeInterval
    }

    func sample(session: UInt64) -> SystemMetricsSample? {
        if activeSession != session {
            activeSession = session
            previousCPUTicks = nil
            previousNetworkCounters = nil
        }
        guard let cpu = sampleCPUUsagePercent(),
              let memory = Self.memoryUsagePercent(),
              let storage = Self.storageUsagePercent() else { return nil }
        return SystemMetricsSample(
            cpuUsagePercent: cpu,
            memoryUsagePercent: memory,
            storageUsagePercent: storage,
            batteryPercent: Self.batteryPercent(),
            networkThroughputKBps: sampleNetworkThroughputKBps()
        )
    }

    private func sampleNetworkThroughputKBps() -> Double? {
        guard let current = Self.primaryNetworkCounters() else {
            previousNetworkCounters = nil
            return nil
        }
        defer { previousNetworkCounters = current }
        guard let previous = previousNetworkCounters,
              previous.interfaceName == current.interfaceName,
              current.timestamp > previous.timestamp else { return 0 }
        let received = UInt64(current.receivedBytes &- previous.receivedBytes)
        let sent = UInt64(current.sentBytes &- previous.sentBytes)
        return Double(received + sent) / (current.timestamp - previous.timestamp) / 1024.0
    }

    private static func primaryNetworkCounters() -> NetworkCounters? {
        let keys = ["State:/Network/Global/IPv4", "State:/Network/Global/IPv6"]
        let interfaceName = keys.lazy.compactMap { key -> String? in
            guard let value = SCDynamicStoreCopyValue(nil, key as CFString) as? [String: Any] else {
                return nil
            }
            return value["PrimaryInterface"] as? String
        }.first
        guard let interfaceName else { return nil }

        var interfaces: UnsafeMutablePointer<ifaddrs>?
        guard getifaddrs(&interfaces) == 0, let first = interfaces else { return nil }
        defer { freeifaddrs(first) }
        var item: UnsafeMutablePointer<ifaddrs>? = first
        while let pointer = item {
            let interface = pointer.pointee
            if String(cString: interface.ifa_name) == interfaceName,
               interface.ifa_addr?.pointee.sa_family == UInt8(AF_LINK),
               let data = interface.ifa_data?.assumingMemoryBound(to: if_data.self) {
                return NetworkCounters(
                    interfaceName: interfaceName,
                    receivedBytes: data.pointee.ifi_ibytes,
                    sentBytes: data.pointee.ifi_obytes,
                    timestamp: Date.timeIntervalSinceReferenceDate
                )
            }
            item = interface.ifa_next
        }
        return nil
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
        guard let previous = previousCPUTicks, total > previous.total else { return nil }
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
