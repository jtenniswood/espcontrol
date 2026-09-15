#pragma once
#include "reset_policy.h"
#include <algorithm>
#include <atomic>
#include <vector>
#include <mutex>

namespace espcontrol::reset {
// The HTTP task records reset intent while OTA may start on the main task.
// Reserve either operation under the same lock, before any flash mutation.
class OperationInterlock {
 public:
  Result record(Storage &storage, Journal &journal, Mode mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (busy()) return Result::CONFLICT;
    if (pending_ && !journal.pending()) return Result::FAILED;  // Uncertain write: reboot to resolve it.
    const auto result = request(storage, journal, mode);
    if (result != Result::CONFLICT) pending_.store(true);
    return result;
  }
  bool begin_installation(bool coprocessor) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pending_ || native_reserved_.load() || coprocessor_busy_.load()) return false;
    (coprocessor ? coprocessor_busy_ : native_reserved_).store(true);
    return true;
  }
  // Only the caller that acquired begin_installation may report its result.
  void finish_begin(bool coprocessor, bool success, uint32_t handle = 0) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (coprocessor) {
      if (!success) coprocessor_busy_.store(false);
    } else if (success) {
      native_handle_ = handle;
      native_handle_valid_ = true;
    } else {
      native_handle_valid_ = false;
      native_reserved_.store(false);
    }
  }
  void finish_native_installation(uint32_t handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (native_handle_valid_ && native_handle_ == handle) {
      native_handle_valid_ = false;
      native_reserved_.store(false);
    }
  }
  void set_ota_source_busy(const void *source, bool busy) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto entry = std::find(ota_sources_.begin(), ota_sources_.end(), source);
    if (busy && entry == ota_sources_.end()) ota_sources_.push_back(source);
    else if (!busy && entry != ota_sources_.end()) ota_sources_.erase(entry);
    ota_busy_.store(!ota_sources_.empty());
  }
  void set_coprocessor_busy(bool busy) { coprocessor_busy_.store(busy); }
  void set_entities_busy(bool busy) {
    entities_busy_.store(busy);
    if (!busy) coprocessor_busy_.store(false);
  }
  bool pending() const { return pending_.load(); }
  bool busy() const { return native_reserved_ || ota_busy_ || coprocessor_busy_ || entities_busy_; }

 private:
  std::mutex mutex_;
  std::vector<const void *> ota_sources_;
  uint32_t native_handle_{0};
  bool native_handle_valid_{false};
  std::atomic<bool> native_reserved_{false};
  std::atomic<bool> pending_{false}, ota_busy_{false}, coprocessor_busy_{false}, entities_busy_{false};
};
}  // namespace espcontrol::reset
