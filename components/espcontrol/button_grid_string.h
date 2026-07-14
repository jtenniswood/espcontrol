#pragma once

#include <cstddef>
#include <string>

#include "esphome/core/string_ref.h"

// Copy an ESPHome StringRef while applying the existing fixed display limit.
// Keeping this at the module boundary avoids assuming the source is null
// terminated at the requested length.
inline std::string string_ref_limited(esphome::StringRef value, size_t max_len) {
  size_t len = value.size();
  if (len > max_len) len = max_len;
  return std::string(value.c_str(), len);
}
