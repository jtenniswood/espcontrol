#pragma once

#include <cctype>
#include <string>

#include "i18n_generated.h"

inline std::string network_status_trim_copy(const std::string& value) {
  const size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  const size_t last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

inline bool network_status_is_specific_firmware_version(const std::string& version) {
  std::string trimmed = network_status_trim_copy(version);
  const size_t len = trimmed.size();
  if (len < 6 || (trimmed[0] != 'v' && trimmed[0] != 'V')) return false;

  size_t pos = 1;
  auto read_number = [&]() -> bool {
    if (pos >= len || !std::isdigit(static_cast<unsigned char>(trimmed[pos]))) return false;
    while (pos < len && std::isdigit(static_cast<unsigned char>(trimmed[pos]))) ++pos;
    return true;
  };

  if (!read_number()) return false;
  for (int part = 0; part < 2; ++part) {
    if (pos >= len || trimmed[pos] != '.') return false;
    ++pos;
    if (!read_number()) return false;
  }
  if (pos == len) return true;
  if (trimmed[pos] != '-' && trimmed[pos] != '+') return false;
  ++pos;
  if (pos == len) return false;
  while (pos < len) {
    unsigned char c = static_cast<unsigned char>(trimmed[pos]);
    if (!std::isalnum(c) && trimmed[pos] != '.' && trimmed[pos] != '-') {
      return false;
    }
    ++pos;
  }
  return true;
}

inline std::string network_status_firmware_label(const std::string& version) {
  std::string trimmed = network_status_trim_copy(version);
  if (trimmed.empty() || trimmed == "Version unknown") {
    return espcontrol_i18n(std::string("Version unknown"));
  }
  if (network_status_is_specific_firmware_version(trimmed)) return trimmed;
  return espcontrol_i18n(std::string("Dev build"));
}
