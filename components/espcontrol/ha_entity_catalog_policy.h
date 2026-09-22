#pragma once

#include <string_view>

#include "ha_catalog_contract.h"

namespace espcontrol {

inline bool ha_entity_catalog_filter_valid(std::string_view value, std::size_t field_limit) {
  return value.size() <= field_limit && value.size() <= catalog_contract::MAX_FILTER;
}

inline bool ha_entity_catalog_capabilities_valid(std::string_view capabilities) {
  // The bridge limits the entire CSV; the contract limits each trimmed entry.
  if (capabilities.size() > catalog_contract::MAX_FILTER) return false;
  constexpr std::string_view whitespace = " \t\r\n\f\v";
  while (!capabilities.empty()) {
    const auto comma = capabilities.find(',');
    const auto item = capabilities.substr(0, comma);
    const auto first = item.find_first_not_of(whitespace);
    if (first != std::string_view::npos &&
        item.find_last_not_of(whitespace) - first + 1 >
            catalog_contract::MAX_CAPABILITY_LENGTH) {
      return false;
    }
    if (comma == std::string_view::npos) break;
    capabilities.remove_prefix(comma + 1);
  }
  return true;
}

}  // namespace espcontrol
