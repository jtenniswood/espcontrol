#pragma once

#include <cstdint>

namespace espcontrol::configuration {

// Compatibility is a separate outcome after the native document is durable.
// Only temporary failures may be retried automatically.
enum class CompatibilityStatus : uint8_t {
  OK,
  RETRYABLE_FAILURE,
  NOT_REPRESENTABLE,
};

struct ConfigurationCompatibilityState {
  uint32_t generation{0};
  bool known{false};
  CompatibilityStatus publication{CompatibilityStatus::OK};
  CompatibilityStatus persistence{CompatibilityStatus::OK};

  bool retry_pending() const {
    return publication == CompatibilityStatus::RETRYABLE_FAILURE ||
           persistence == CompatibilityStatus::RETRYABLE_FAILURE;
  }
};

} // namespace espcontrol::configuration
