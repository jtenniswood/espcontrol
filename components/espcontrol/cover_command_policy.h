#pragma once
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string_view>
namespace espcontrol::cover_command {
inline constexpr int normal_feature(std::string_view mode) {
  return mode == "open" ? 1 : mode == "close" ? 2 : mode == "stop" ? 8 : mode == "set_position" ? 4 : 0;
}
inline constexpr int tilt_feature(std::string_view mode) {
  return mode == "open" ? 16 : mode == "close" ? 32 : mode == "stop" ? 64 : mode == "set_position" ? 128 : 0;
}
inline constexpr bool supported(std::string_view mode, bool known, int features) {
  return !known || (features & (normal_feature(mode) | tilt_feature(mode))) != 0;
}
inline constexpr bool use_tilt(std::string_view mode, bool known, int features) {
  return known && normal_feature(mode) != 0 && tilt_feature(mode) != 0 &&
         !(features & normal_feature(mode)) && (features & tilt_feature(mode));
}
inline bool parse_features(std::string_view input, int& features) {
  char buffer[33];
  input = input.substr(0, sizeof(buffer) - 1);
  if (input.empty()) return false;
  std::memcpy(buffer, input.data(), input.size());
  buffer[input.size()] = '\0';
  char* end = nullptr;
  errno = 0;
  const auto value = std::strtoll(buffer, &end, 10);
  if (end == buffer || errno == ERANGE || value < std::numeric_limits<int>::min() ||
      value > std::numeric_limits<int>::max())
    return false;
  features = static_cast<int>(value);
  return true;
}
}  // namespace espcontrol::cover_command
