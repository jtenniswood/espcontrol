#pragma once

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <type_traits>

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32
#include "esp_memory_utils.h"
#endif

// Stateless STL allocator for long-lived application metadata. ESPHome's
// RAMAllocator prefers external RAM and falls back to internal RAM. This
// wrapper supplies the failure contract required by STL containers and keeps
// lightweight diagnostics shared by every rebound allocator specialization.
struct EspControlExternalAllocatorStats {
  static inline size_t external_bytes = 0;
  static inline size_t internal_bytes = 0;
  static inline size_t external_allocations = 0;
  static inline size_t internal_fallbacks = 0;
  static inline size_t failed_allocations = 0;
};

template<typename T>
class EspControlExternalAllocator {
 public:
  using value_type = T;
  using is_always_equal = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;

  EspControlExternalAllocator() noexcept = default;
  template<typename U>
  EspControlExternalAllocator(const EspControlExternalAllocator<U> &) noexcept {}

  template<typename U>
  struct rebind {
    using other = EspControlExternalAllocator<U>;
  };

  [[nodiscard]] T *allocate(size_t count) {
    if (count > max_size()) fail_allocation(count, true);
    esphome::RAMAllocator<T> allocator(ram_allocator_flags());
    T *result = allocator.allocate(count);
    if (result == nullptr && count != 0) fail_allocation(count, false);
    if (result == nullptr) return nullptr;

    const size_t bytes = count * sizeof(T);
#ifdef USE_ESP32
    if (esp_ptr_external_ram(result)) {
      EspControlExternalAllocatorStats::external_bytes += bytes;
      EspControlExternalAllocatorStats::external_allocations++;
    } else {
      EspControlExternalAllocatorStats::internal_bytes += bytes;
      EspControlExternalAllocatorStats::internal_fallbacks++;
    }
#else
    EspControlExternalAllocatorStats::internal_bytes += bytes;
    EspControlExternalAllocatorStats::internal_fallbacks++;
#endif
    return result;
  }

  void deallocate(T *pointer, size_t count) noexcept {
    if (pointer == nullptr) return;
    const size_t bytes = count * sizeof(T);
#ifdef USE_ESP32
    if (esp_ptr_external_ram(pointer)) {
      subtract_saturating(EspControlExternalAllocatorStats::external_bytes, bytes);
    } else {
      subtract_saturating(EspControlExternalAllocatorStats::internal_bytes, bytes);
    }
#else
    subtract_saturating(EspControlExternalAllocatorStats::internal_bytes, bytes);
#endif
    esphome::RAMAllocator<T> allocator(ram_allocator_flags());
    allocator.deallocate(pointer, count);
  }

  constexpr size_t max_size() const noexcept {
    return std::numeric_limits<size_t>::max() / sizeof(T);
  }

  template<typename U>
  bool operator==(const EspControlExternalAllocator<U> &) const noexcept {
    return true;
  }

  template<typename U>
  bool operator!=(const EspControlExternalAllocator<U> &) const noexcept {
    return false;
  }

 private:
  static constexpr uint8_t ram_allocator_flags() {
    return static_cast<uint8_t>(esphome::RAMAllocator<T>::ALLOC_EXTERNAL |
                                esphome::RAMAllocator<T>::ALLOC_INTERNAL);
  }

  static void subtract_saturating(size_t &value, size_t amount) {
    value = amount <= value ? value - amount : 0;
  }

  [[noreturn]] static void fail_allocation(size_t count, bool overflow) {
    EspControlExternalAllocatorStats::failed_allocations++;
    if (overflow) {
      ESP_LOGE("ha", "Fatal subscription storage allocation failure: "
                     "requested=%u elements x %u bytes (overflow)",
               static_cast<unsigned>(count), static_cast<unsigned>(sizeof(T)));
    } else {
      ESP_LOGE("ha", "Fatal subscription storage allocation failure: requested=%u bytes",
               static_cast<unsigned>(count * sizeof(T)));
    }
    std::abort();
  }
};
