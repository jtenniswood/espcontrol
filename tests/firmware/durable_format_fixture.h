#pragma once

#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include "device_ui_limits.h"
#include "panel_config_document.h"

namespace espcontrol::configuration::test {
inline std::vector<uint8_t> large_flat_document(size_t roots = 20) {
  const DeviceUiLimits *profile = nullptr;
  for (const auto &limits : DEVICE_UI_PROFILES)
    if (limits.slot_count == 20) { profile = &limits; break; }
  assert(profile && roots <= 20);
  std::vector<uint8_t> bytes(40 * 1024);
  PanelConfigWriter writer(bytes.data(), bytes.size());
  assert(writer.begin() == PanelConfigStatus::OK);
  assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(profile->device_profile),
      std::strlen(profile->device_profile)) == PanelConfigStatus::OK);
  std::string order;
  for (size_t slot = 1; slot <= roots; ++slot) {
    if (!order.empty()) order += ',';
    order += std::to_string(slot);
    const std::string folder = ";Room;Home;Auto;;;subpage";
    assert(writer.append_button(slot, reinterpret_cast<const uint8_t *>(folder.data()), folder.size()) == PanelConfigStatus::OK);
    std::string page = "~B";
    for (size_t child = 1; child <= 19; ++child) page += ',' + std::to_string(child);
    for (size_t child = 1; child <= 19; ++child)
      page += "|H,climate.room_" + std::to_string(slot) + '_' + std::to_string(child) +
          ",Room%20%C2%B0C,Auto,Auto,,,,opaque=%GG";
    assert(writer.append_subpage(slot, reinterpret_cast<const uint8_t *>(page.data()), page.size()) == PanelConfigStatus::OK);
  }
  assert(writer.append_setting(reinterpret_cast<const uint8_t *>("button_order"), 12,
      reinterpret_cast<const uint8_t *>(order.data()), order.size()) == PanelConfigStatus::OK);
  const std::string key = "future_setting", value = "opaque%00;:,|";
  assert(writer.append_setting(reinterpret_cast<const uint8_t *>(key.data()), key.size(),
      reinterpret_cast<const uint8_t *>(value.data()), value.size()) == PanelConfigStatus::OK);
  const std::string inactive = "~sensor.hidden,Retained%GG,Auto,Auto,,,sensor,,opaque";
  assert(writer.append_button(32, reinterpret_cast<const uint8_t *>(inactive.data()), inactive.size()) == PanelConfigStatus::OK);
  size_t size = 0;
  assert(writer.finish(&size) == PanelConfigStatus::OK);
  bytes.resize(size);
  return bytes;
}
}  // namespace espcontrol::configuration::test
