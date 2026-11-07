#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string_view>

#include "ha_state_limits.h"
#include "media_volume_capability.h"
#include "page_entity_state_runtime.h"

namespace espcontrol::pages {
struct PageMediaVolumeSnapshot {
  uint8_t reported{0}, display{0};
  bool known{false};
  media::VolumeControlMode mode{media::VolumeControlMode::ABSOLUTE};
};
// Raw values stay in the shared graph. Use the existing numeric-prefix policy,
// clamp before integer conversion and retain step/read-only display behavior.
inline PageMediaVolumeSnapshot read_media_volume(PageEntityStateRuntime& state, ConfigurationHandle card,
                                                 std::string_view entity, uint8_t maximum) {
  PageMediaVolumeSnapshot result;
  char input[HA_STATE_TEXT_MAX_LEN + 1];
  const auto copy = [&](const PageEntityValue& value) {
    const auto raw = value.status == PageEntityStatus::AVAILABLE
                         ? value.value.substr(0, HA_STATE_TEXT_MAX_LEN)
                         : std::string_view{};
    if (!raw.empty()) std::memcpy(input, raw.data(), raw.size());
    input[raw.size()] = '\0';
  };
  const auto capabilities = state.read_attribute(card, entity, "supported_features");
  copy(capabilities);
  char* end = nullptr;
  const auto features = std::strtoull(input, &end, 10);
  // Only these two capability bits affect the volume card. Do not narrow a
  // potentially large feature mask into a signed platform-sized integer.
  result.mode = media::volume_control_mode(
      end != input, static_cast<int>(features & (media::SUPPORT_VOLUME_SET | media::SUPPORT_VOLUME_STEP)));
  // A failed cache reservation is distinct from an unknown HA capability.
  // Keep its value readable without admitting a new absolute-volume preview.
  if (capabilities.status == PageEntityStatus::EXHAUSTED) result.mode = media::VolumeControlMode::READ_ONLY;
  copy(state.read_attribute(card, entity, "volume_level"));
  const float level = std::strtof(input, &end);
  if (end == input || !std::isfinite(level)) return result;
  result.known = true;
  result.reported = static_cast<uint8_t>(std::clamp(level, 0.0f, 1.0f) * 100.0f + 0.5f);
  result.display = static_cast<uint8_t>(media::volume_display_value(result.mode, result.reported, maximum));
  return result;
}
}  // namespace espcontrol::pages
