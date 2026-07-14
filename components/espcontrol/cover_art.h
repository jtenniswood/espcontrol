#ifndef ESPCONTROL_COVER_ART_H
#define ESPCONTROL_COVER_ART_H
#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

namespace espcontrol::cover_art {

constexpr int MAX_DOWNLOAD_RETRIES = 5;
constexpr uint32_t DEFERRED_DOWNLOAD_MS = 100;
constexpr uint32_t CACHED_ARTWORK_DEBOUNCE_MS = 300;
constexpr uint32_t ARTWORK_ATTRIBUTE_RETRY_MS = 1500;
constexpr uint32_t SUBSCRIPTION_RECONCILE_MS = 5000;
constexpr size_t MAX_ARTWORK_URL_LENGTH = 4096;
constexpr int ACCENT_SAMPLE_GRID = 20;

struct RuntimeState {
  std::string source_url, effective_download_url, active_download_source_url;
  std::string loaded_url, last_good_url, retry_url, fallback_url;
  int retry_count{0};
  bool image_available{false};
  bool refresh_needed{false};

  bool download_active() const { return !active_download_source_url.empty() && !effective_download_url.empty(); }
  bool current_image_loaded() const { return image_available && !source_url.empty() && source_url == loaded_url; }
  bool needs_download() const { return !source_url.empty() && (!image_available || refresh_needed || source_url != loaded_url); }
  void select_source(const std::string &url) {
    if (url == source_url) return;
    source_url = url; refresh_needed = !url.empty(); retry_url.clear(); retry_count = 0;
    if (loaded_url.empty()) image_available = false;
  }
  void begin_download(const std::string &effective_url) {
    active_download_source_url = source_url; effective_download_url = effective_url;
  }
  bool apply_download(const std::string &completed_effective_url) {
    if (completed_effective_url != effective_download_url) return false;
    const std::string completed_source = active_download_source_url;
    effective_download_url.clear(); active_download_source_url.clear();
    if (completed_source.empty()) return false;
    loaded_url = completed_source; last_good_url = completed_source;
    image_available = true; retry_count = 0; retry_url = completed_source;
    refresh_needed = completed_source != source_url; return true;
  }
  bool can_retry() const { return retry_count < MAX_DOWNLOAD_RETRIES; }
  void record_failure() {
    effective_download_url.clear(); active_download_source_url.clear();
    if (retry_url != source_url) { retry_url = source_url; retry_count = 0; }
  }
  bool begin_retry() { if (!can_retry()) return false; ++retry_count; return true; }
  void clear_image() {
    source_url.clear(); effective_download_url.clear(); active_download_source_url.clear(); loaded_url.clear();
    last_good_url.clear();
    retry_url.clear(); fallback_url.clear(); retry_count = 0; image_available = false; refresh_needed = false;
  }
};

struct PolicyInput {
  bool enabled{false}, media_playing{false}, entity_configured{false};
  bool attribute_conditions_match{true}, hide_external_input{false}, external_input_active{false};
  bool schedule_blocks{false}, alarm_takeover_active{false}, voice_interaction_active{false};
};
inline bool policy_allows_feature(const PolicyInput &i) {
  return i.enabled && i.entity_configured && i.attribute_conditions_match && !(i.hide_external_input && i.external_input_active);
}
inline bool policy_allows_download(const PolicyInput &i) { return policy_allows_feature(i); }
inline bool policy_allows_display(const PolicyInput &i) {
  return policy_allows_feature(i) && i.media_playing && !i.schedule_blocks && !i.alarm_takeover_active && !i.voice_interaction_active;
}
inline bool feature_allowed(bool enabled, bool entity_configured, bool conditions_match,
                            bool hide_external_input, bool external_input_active) {
  PolicyInput input;
  input.enabled = enabled;
  input.entity_configured = entity_configured;
  input.attribute_conditions_match = conditions_match;
  input.hide_external_input = hide_external_input;
  input.external_input_active = external_input_active;
  return policy_allows_feature(input);
}
inline bool display_allowed(bool enabled, bool media_playing, bool entity_configured,
                            bool conditions_match, bool hide_external_input,
                            bool external_input_active, bool schedule_blocks,
                            bool alarm_active, bool voice_active) {
  PolicyInput input;
  input.enabled = enabled;
  input.media_playing = media_playing;
  input.entity_configured = entity_configured;
  input.attribute_conditions_match = conditions_match;
  input.hide_external_input = hide_external_input;
  input.external_input_active = external_input_active;
  input.schedule_blocks = schedule_blocks;
  input.alarm_takeover_active = alarm_active;
  input.voice_interaction_active = voice_active;
  return policy_allows_display(input);
}

struct Layout {
  int screen_width, screen_height, art_x, art_y, art_size;
  int accent_x, accent_y, accent_width, accent_height;
  int panel_x, panel_y, panel_width, panel_height, title_max_height, panel_padding;
  bool split;
};
inline bool rotation_is_landscape(const std::string &slug, const std::string &rotation) {
  return slug == "guition-esp32-p4-jc4880p443" ? rotation == "90" || rotation == "270"
                                                : rotation == "0" || rotation == "180";
}
inline Layout cover_art_layout(const std::string &slug, const std::string &rotation,
                               int screen_width, int screen_height, int art_size, int title_height) {
  const bool landscape = rotation_is_landscape(slug, rotation);
  if (slug == "guition-esp32-p4-jc1060p470") return landscape
    ? Layout{1024,600,0,0,600,585,0,439,600,615,34,377,430,260,0,true}
    : Layout{600,1024,0,0,600,0,600,600,424,30,634,540,360,162,0,true};
  if (slug == "guition-esp32-p4-jc4880p443") return landscape
    ? Layout{800,480,0,0,480,480,0,320,480,504,34,272,330,220,0,true}
    : Layout{480,800,0,0,480,0,480,480,320,24,514,324,262,130,0,true};
  if (slug == "guition-esp32-p4-jc8012p4a1" || slug == "guition-esp32-p4-jc8012p4a1-v2") return landscape
    ? Layout{1280,800,0,0,800,800,0,480,800,840,40,400,720,506,0,true}
    : Layout{800,1280,0,0,800,0,800,800,480,40,834,720,422,216,0,true};
  art_size = std::max(1, std::min(art_size, std::min(screen_width, screen_height)));
  int x = std::max(0, (screen_width - art_size) / 2), y = std::max(0, (screen_height - art_size) / 2);
  return Layout{screen_width,screen_height,x,y,art_size,x,y,art_size,art_size,x,y,art_size,art_size,
                title_height,art_size >= 700 ? 36 : 24,false};
}
inline int progress_percent(float position, float duration) {
  if (duration <= 0.0f) return 0;
  return std::max(0, std::min(100, static_cast<int>((position / duration) * 100.0f + 0.5f)));
}

}  // namespace espcontrol::cover_art
#endif
