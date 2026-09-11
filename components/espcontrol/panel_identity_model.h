#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include "panel_config_document.h"

namespace espcontrol {
constexpr size_t PANEL_NAME_MAX_BYTES = 120;

inline bool normalize_panel_name(const std::string &input, std::string &output) {
  // Match String.trim() for ASCII whitespace; reject other control characters.
  size_t start = input.find_first_not_of(" \t\r\n\f\v");
  output = start == std::string::npos ? "" :
      input.substr(start, input.find_last_not_of(" \t\r\n\f\v") - start + 1);
  if (output.size() > PANEL_NAME_MAX_BYTES ||
      !configuration::panel_config_valid_utf8(
          reinterpret_cast<const uint8_t *>(output.data()), output.size())) return false;
  for (size_t i = 0; i < output.size(); ++i) {
    const auto c = static_cast<uint8_t>(output[i]);
    if (c < 32 || c == 127 ||
        (c == 0xc2 && i + 1 < output.size() &&
         static_cast<uint8_t>(output[i + 1]) >= 0x80 &&
         static_cast<uint8_t>(output[i + 1]) <= 0x9f)) return false;
  }
  return true;
}

inline std::string panel_hostname(const std::string &name, const std::string &suffix) {
  std::string slug;
  bool separator = false;
  for (unsigned char c : name) {
    if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      if (separator && !slug.empty()) slug += '-';
      slug += static_cast<char>(c);
      separator = false;
    } else separator = true;
  }
  if (slug.empty()) slug = "panel";
  slug.resize(std::min<size_t>(13, slug.size()));
  while (!slug.empty() && slug.back() == '-') slug.pop_back();
  return "espcontrol-" + slug + "-" + suffix;
}
}  // namespace espcontrol
