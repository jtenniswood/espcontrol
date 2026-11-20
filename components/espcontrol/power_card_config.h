#pragma once

#include <cstdint>
#include <string>

// Included after ParsedCfg and compact-option primitives, like Media's config.
namespace espcontrol::power {

struct ConfigV1 {
  std::string home_entity;
  std::string solar_entity;
  std::string grid_entity;
  std::string battery_entity;
  std::string battery_soc_entity;
  bool invert_grid = false;
  bool invert_battery = false;
  uint8_t installed_components = 0x0F; // Model order: Solar, Home, Grid, Battery.
};

inline ConfigV1 decode_config_v1(const ParsedCfg &saved) {
  ConfigV1 config{
    trim_saved_option_value(saved.entity),
    trim_saved_option_value(cfg_option_value(saved.options, "solar_entity")),
    trim_saved_option_value(cfg_option_value(saved.options, "grid_entity")),
    trim_saved_option_value(cfg_option_value(saved.options, "battery_entity")),
    trim_saved_option_value(cfg_option_value(saved.options, "battery_soc_entity")),
    cfg_option_token_present(saved.options, "invert_grid"),
    cfg_option_token_present(saved.options, "invert_battery"),
  };
  // Legacy exclude_home is ignored: Home is always part of the system.
  const char *excluded[] = {"exclude_solar", nullptr, "exclude_grid", "exclude_battery"};
  for (size_t i = 0; i < 4; ++i)
    if (excluded[i] && cfg_option_token_present(saved.options, excluded[i])) config.installed_components &= ~(1u << i);
  return config;
}

inline std::string normalize_options(const std::string &options) {
  std::string out;
  for (const char *name : {"solar_entity", "grid_entity", "battery_entity", "battery_soc_entity"}) {
    const auto value = trim_saved_option_value(cfg_option_value(options, name));
    if (value.empty()) continue;
    if (!out.empty()) out += ',';
    out += std::string(name) + '=' + encode_compact_field(value);
  }
  for (const char *name : {"invert_grid", "invert_battery", "exclude_solar", "exclude_grid", "exclude_battery"}) {
    if (!cfg_option_token_present(options, name)) continue;
    if (!out.empty()) out += ',';
    out += name;
  }
  return out;
}

}  // namespace espcontrol::power
