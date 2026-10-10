#pragma once

#include <cstdint>
#include <string>

enum class MediaControlTab : uint8_t {
  CONTROLS = 0,
  PROGRESS = 1,
  VOLUME = 2,
  SPEAKERS = 3,
  POWER = 4,
};

namespace espcontrol::media {

constexpr const char *DEFAULT_CONTROL_TABS = "controls|progress|volume|speakers";
constexpr const char *CONTROL_TABS_WITH_POWER = "controls|progress|volume|speakers|power";

struct ControlTabs {
  MediaControlTab tabs[5] = {};
  uint8_t count = 0;

  bool contains(MediaControlTab tab) const {
    for (uint8_t i = 0; i < count; ++i) if (tabs[i] == tab) return true;
    return false;
  }

  bool operator==(const ControlTabs &other) const {
    if (count != other.count) return false;
    for (uint8_t i = 0; i < count; ++i) if (tabs[i] != other.tabs[i]) return false;
    return true;
  }

  void add(MediaControlTab tab) {
    if (count < 5 && !contains(tab)) tabs[count++] = tab;
  }
};

inline const char *control_tab_token(MediaControlTab tab) {
  switch (tab) {
    case MediaControlTab::CONTROLS: return "controls";
    case MediaControlTab::PROGRESS: return "progress";
    case MediaControlTab::VOLUME: return "volume";
    case MediaControlTab::SPEAKERS: return "speakers";
    case MediaControlTab::POWER: return "power";
  }
  return "controls";
}

inline ControlTabs parse_control_tabs(const std::string &value, bool power_available = false) {
  const size_t first = value.find_first_not_of(" \t\r\n");
  const std::string raw = first == std::string::npos
    ? (power_available ? CONTROL_TABS_WITH_POWER : DEFAULT_CONTROL_TABS) : value;
  ControlTabs result;
  size_t start = 0;
  while (start <= raw.size()) {
    const size_t end = raw.find('|', start);
    std::string token = raw.substr(start, end == std::string::npos ? end : end - start);
    const size_t begin = token.find_first_not_of(" \t\r\n");
    if (begin != std::string::npos) {
      token = token.substr(begin, token.find_last_not_of(" \t\r\n") - begin + 1);
      for (uint8_t i = 0; i < 5; ++i) {
        const auto tab = static_cast<MediaControlTab>(i);
        if (token == control_tab_token(tab) && (tab != MediaControlTab::POWER || power_available)) result.add(tab);
      }
    }
    if (end == std::string::npos) break;
    start = end + 1;
  }
  if (!result.count) result.add(MediaControlTab::CONTROLS);
  return result;
}

inline std::string normalize_control_tabs_value(const std::string &value, bool power_available = false) {
  const ControlTabs tabs = parse_control_tabs(value, power_available);
  std::string result;
  for (uint8_t i = 0; i < tabs.count; ++i) {
    if (i) result += '|';
    result += control_tab_token(tabs.tabs[i]);
  }
  return result;
}

inline ControlTabs visible_control_tabs(const ControlTabs &configured,
                                       bool progress_supported,
                                       bool speakers_supported,
                                       bool power_supported) {
  ControlTabs visible;
  for (uint8_t i = 0; i < configured.count; ++i) {
    const MediaControlTab tab = configured.tabs[i];
    if (tab == MediaControlTab::PROGRESS && !progress_supported) continue;
    if (tab == MediaControlTab::SPEAKERS && !speakers_supported) continue;
    if (tab == MediaControlTab::POWER && !power_supported) continue;
    visible.add(tab);
  }
  // Keep the modal usable while optional capabilities are unavailable.
  if (!visible.count) visible.add(MediaControlTab::CONTROLS);
  return visible;
}

}  // namespace espcontrol::media
