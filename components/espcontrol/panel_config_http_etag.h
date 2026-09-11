#pragma once

#include <cstddef>
#include <cstdint>

namespace espcontrol::configuration {

inline bool parse_panel_config_etag(const char *value, uint32_t *generation) {
  if (value == nullptr || generation == nullptr || value[0] != '"') return false;
  uint64_t parsed = 0;
  size_t index = 1;
  if (value[index] == '\0') return false;
  for (; value[index] >= '0' && value[index] <= '9'; ++index) {
    parsed = parsed * 10 + static_cast<uint64_t>(value[index] - '0');
    if (parsed > UINT32_MAX) return false;
  }
  if (index == 1 || value[index] != '"' || value[index + 1] != '\0') return false;
  *generation = static_cast<uint32_t>(parsed);
  return true;
}

}  // namespace espcontrol::configuration
