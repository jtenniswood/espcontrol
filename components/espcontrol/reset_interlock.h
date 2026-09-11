#pragma once
#include "reset_policy.h"
#include <atomic>
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
    if (pending_) return false;
    (coprocessor ? coprocessor_busy_ : ota_busy_).store(true);
    return true;
  }
  void set_ota_busy(bool busy) { ota_busy_.store(busy); }
  void set_coprocessor_busy(bool busy) { coprocessor_busy_.store(busy); }
  void set_entities_busy(bool busy) {
    entities_busy_.store(busy);
    if (!busy) coprocessor_busy_.store(false);
  }
  bool pending() const { return pending_.load(); }
  bool busy() const { return ota_busy_ || coprocessor_busy_ || entities_busy_; }

 private:
  std::mutex mutex_;
  std::atomic<bool> pending_{false}, ota_busy_{false}, coprocessor_busy_{false}, entities_busy_{false};
};
}  // namespace espcontrol::reset
