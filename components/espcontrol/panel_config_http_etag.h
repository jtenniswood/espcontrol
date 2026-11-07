#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace espcontrol::configuration {

inline bool parse_panel_config_etag(const char *value, uint32_t *generation, uint16_t representation = 1) {
  if (value == nullptr || generation == nullptr || value[0] != '"') return false;
  uint64_t parsed = 0;
  size_t index = 1;
  // Strong validators distinguish the bytes of each representation while
  // both still compare the same durable generation inside the service.
  if (representation == 2) {
    if (std::strncmp(value + index, "v2-", 3) != 0) return false;
    index += 3;
  } else if (representation != 1) return false;
  const size_t start = index;
  if (value[index] == '\0') return false;
  for (; value[index] >= '0' && value[index] <= '9'; ++index) {
    parsed = parsed * 10 + static_cast<uint64_t>(value[index] - '0');
    if (parsed > UINT32_MAX) return false;
  }
  if (index == start || value[index] != '"' || value[index + 1] != '\0') return false;
  *generation = static_cast<uint32_t>(parsed);
  return true;
}

}  // namespace espcontrol::configuration
