#ifndef ESPCONTROL_COVER_ART_H
#define ESPCONTROL_COVER_ART_H
#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>

#include "artwork_controller.h"
#include "media_display_text.h"

// Defined by the firmware Home Assistant transport. Keeping this declaration
// lightweight lets host-side Cover Art tests use the controller without
// pulling in ESPHome's generated API types.
inline void ha_schedule_metadata_refresh(
    const std::string &entity_id,
    std::initializer_list<const char *> attributes, uint32_t scope);
inline void ha_cancel_metadata_refresh(uint32_t scope);

namespace espcontrol::cover_art {

constexpr int MAX_DOWNLOAD_RETRIES = 5;
constexpr uint32_t DEFERRED_DOWNLOAD_MS = 100;
constexpr uint32_t CACHED_ARTWORK_DEBOUNCE_MS = 300;
constexpr uint32_t ARTWORK_TRIGGER_DEBOUNCE_MS = 75;
constexpr uint32_t ARTWORK_ATTRIBUTE_RETRY_MS = 1500;
constexpr uint32_t SUBSCRIPTION_RECONCILE_MS = 5000;
constexpr size_t MAX_ARTWORK_URL_LENGTH = 4096;
constexpr int ACCENT_SAMPLE_GRID = 20;

enum class TrackOverlayMode { HIDDEN, PERSISTENT, TIMED };

inline TrackOverlayMode track_overlay_mode(bool playing, bool retained_pause,
                                          bool square, bool image_available,
                                          float duration_seconds) {
  if (retained_pause) return TrackOverlayMode::PERSISTENT;
  if (!playing) return TrackOverlayMode::HIDDEN;
  if (!square || !image_available || duration_seconds < 0) return TrackOverlayMode::PERSISTENT;
  return duration_seconds > 0 ? TrackOverlayMode::TIMED : TrackOverlayMode::HIDDEN;
}

inline std::string normalized_media_source(std::string source) {
  while (!source.empty() && std::isspace(static_cast<unsigned char>(source.front()))) {
    source.erase(source.begin());
  }
  while (!source.empty() && std::isspace(static_cast<unsigned char>(source.back()))) {
    source.pop_back();
  }
  for (char &ch : source) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return source;
}

inline bool external_media_source(const std::string &source) {
  const std::string normalized = normalized_media_source(source);
  return normalized == "tv" || normalized == "line-in" ||
         normalized == "line in" || normalized.rfind("hdmi", 0) == 0;
}

inline bool media_card_artwork_suppressed(bool source_known,
                                          bool external_source) {
  return source_known && external_source;
}

inline bool media_now_playing_artist_visible(bool artist_present,
                                             bool external_source,
                                             bool show_track_details,
                                             bool external_source_fallback) {
  return (show_track_details || external_source_fallback) &&
         (artist_present || external_source);
}

inline bool media_external_source_stale_for_current_content(
    bool external_source, bool source_observed_for_state,
    bool current_content_present) {
  return external_source && !source_observed_for_state &&
         current_content_present;
}

inline bool media_entity_state_usable(const std::string &state) {
  const std::string normalized = normalized_media_source(state);
  return normalized == "playing" || normalized == "paused" ||
         normalized == "buffering";
}

inline bool media_artwork_content_current(bool state_known, bool available,
                                          const std::string &state,
                                          bool artwork_present) {
  return artwork_present && state_known && available &&
         media_entity_state_usable(state);
}

inline bool media_state_change_invalidates_retained_content(
    bool previous_state_known, const std::string &previous_state,
    const std::string &next_state) {
  const std::string normalized_next = normalized_media_source(next_state);
  if (media_entity_state_usable(normalized_next)) return false;
  return !previous_state_known ||
         normalized_media_source(previous_state) != normalized_next;
}

inline bool media_state_change_needs_content_resync(
    bool previous_state_known, const std::string &previous_state,
    const std::string &next_state, bool has_content) {
  return previous_state_known &&
         !media_entity_state_usable(previous_state) &&
         media_entity_state_usable(next_state) && !has_content;
}

inline bool media_card_artwork_should_clear(bool state_known, bool available,
                                            const std::string &state,
                                            bool has_content) {
  return state_known &&
         (!available || (!media_entity_state_usable(state) && !has_content));
}

inline bool media_entity_content_available(bool state_known, bool available,
                                           bool has_content) {
  return state_known && available && has_content;
}

inline bool media_cover_art_idle_placeholder_visible(
    bool state_known, bool available, const std::string &state,
    bool has_content, bool external_source_fallback) {
  return state_known && available && !media_entity_state_usable(state) &&
         !has_content && !external_source_fallback;
}

inline bool use_secondary_media_entity(bool primary_external,
                                       bool secondary_configured,
                                       bool secondary_playback_active,
                                       bool secondary_has_content) {
  return primary_external && secondary_configured && secondary_playback_active &&
         secondary_has_content;
}

struct AccentColor {
  uint8_t red{0};
  uint8_t green{0};
  uint8_t blue{0};
  bool valid{false};
};

inline float accent_luminance(AccentColor color) {
  const auto linear = [](uint8_t channel) {
    const float value = channel / 255.0f;
    return value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
  };
  return 0.2126f * linear(color.red) + 0.7152f * linear(color.green) + 0.0722f * linear(color.blue);
}

inline uint32_t playback_icon_color(AccentColor normal, AccentColor pressed) {
  // Use one foreground for both states, choosing the strongest worst-case
  // contrast so pressing the button never makes the glyph disappear.
  const float normal_luminance = accent_luminance(normal);
  const float pressed_luminance = accent_luminance(pressed);
  const float black_contrast = (std::min(normal_luminance, pressed_luminance) + 0.05f) / 0.05f;
  const float white_contrast = 1.05f / (std::max(normal_luminance, pressed_luminance) + 0.05f);
  return black_contrast >= white_contrast ? 0x000000 : 0xFFFFFF;
}

inline AccentColor extract_accent_color_rgb565(
    const uint8_t *data, int image_width, int image_height, bool big_endian,
    int content_x, int content_y, int content_width, int content_height) {
  if (!data || image_width <= 0 || image_height <= 0) return {};
  if (content_width <= 0 || content_height <= 0 || content_x < 0 || content_y < 0 ||
      content_x > image_width - content_width ||
      content_y > image_height - content_height) {
    content_x = 0;
    content_y = 0;
    content_width = image_width;
    content_height = image_height;
  }

  const int step_x = std::max(1, content_width / ACCENT_SAMPLE_GRID);
  const int step_y = std::max(1, content_height / ACCENT_SAMPLE_GRID);
  int64_t red_weighted = 0;
  int64_t green_weighted = 0;
  int64_t blue_weighted = 0;
  int64_t total_weight = 0;

  for (int y = content_y + step_y / 2; y < content_y + content_height; y += step_y) {
    for (int x = content_x + step_x / 2; x < content_x + content_width; x += step_x) {
      const size_t offset = (static_cast<size_t>(y) * image_width + x) * 2u;
      const uint16_t pixel = big_endian
        ? (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1]
        : data[offset] | (static_cast<uint16_t>(data[offset + 1]) << 8);
      int red = (pixel >> 11) & 0x1F;
      int green = (pixel >> 5) & 0x3F;
      int blue = pixel & 0x1F;
      red = (red << 3) | (red >> 2);
      green = (green << 2) | (green >> 4);
      blue = (blue << 3) | (blue >> 2);
      const int maximum = std::max(red, std::max(green, blue));
      const int minimum = std::min(red, std::min(green, blue));
      const int saturation = maximum - minimum;
      const int weight = saturation * saturation + 1;
      red_weighted += static_cast<int64_t>(red) * weight;
      green_weighted += static_cast<int64_t>(green) * weight;
      blue_weighted += static_cast<int64_t>(blue) * weight;
      total_weight += weight;
    }
  }
  if (total_weight <= 0) return {};
  return {
    static_cast<uint8_t>(red_weighted / total_weight),
    static_cast<uint8_t>(green_weighted / total_weight),
    static_cast<uint8_t>(blue_weighted / total_weight),
    true,
  };
}

inline AccentColor darken_accent_color(AccentColor color) {
  if (!color.valid) return {};
  color.red = static_cast<uint8_t>(color.red / 3);
  color.green = static_cast<uint8_t>(color.green / 3);
  color.blue = static_cast<uint8_t>(color.blue / 3);
  return color;
}

struct RuntimeState {
  espcontrol::artwork::SourceCandidates sources;
  espcontrol::artwork::RefreshBatch artwork_refresh;
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
    sources.clear();
    artwork_refresh.reset();
    source_url.clear(); effective_download_url.clear(); active_download_source_url.clear(); loaded_url.clear();
    last_good_url.clear();
    retry_url.clear(); fallback_url.clear(); retry_count = 0; image_available = false; refresh_needed = false;
  }
};

enum class PlaybackCommand { NONE, PAUSE, PLAY };

// Ownership belongs to one visible screensaver session, never to all paused
// media. HA state confirms commands; repeated taps cannot queue toggles.
class PlaybackControl {
 public:
  static constexpr uint32_t COMMAND_TIMEOUT_MS = 5000;

  PlaybackCommand begin(const std::string &entity, const std::string &state,
                        uint32_t now) {
    expire(now);
    if (pending() || entity.empty()) return PlaybackCommand::NONE;
    if (state == "playing" || state == "buffering") {
      reset();
      command_ = PlaybackCommand::PAUSE;
    } else if (state == "paused" && retains_pause(entity)) {
      command_ = PlaybackCommand::PLAY;
    } else {
      return PlaybackCommand::NONE;
    }
    entity_ = entity;
    started_ms_ = now;
    return command_;
  }

  void observe(const std::string &entity, const std::string &state, uint32_t now) {
    expire(now);
    if (entity != entity_) { reset(); return; }
    if (state == "paused") {
      if (command_ == PlaybackCommand::PAUSE) {
        retained_ = true;
        command_ = PlaybackCommand::NONE;
      }
    } else if (state == "playing" || state == "buffering") {
      if (command_ != PlaybackCommand::PAUSE) reset();
    } else {
      reset();
    }
  }

  void expire(uint32_t now) {
    if (pending() && now - started_ms_ >= COMMAND_TIMEOUT_MS) cancel_pending();
  }
  bool update_connection(bool connected) {
    if (connected) return false;
    const bool dismiss = retained_;
    reset();
    return dismiss;
  }
  void cancel_pending() {
    command_ = PlaybackCommand::NONE;
    if (!retained_) entity_.clear();
  }
  void reset() {
    command_ = PlaybackCommand::NONE;
    retained_ = false;
    entity_.clear();
  }
  bool pending() const { return command_ != PlaybackCommand::NONE; }
  bool retains_pause(const std::string &entity) const {
    return retained_ && !entity.empty() && entity == entity_;
  }

 private:
  std::string entity_;
  PlaybackCommand command_{PlaybackCommand::NONE};
  uint32_t started_ms_{0};
  bool retained_{false};
};

struct PolicyInput {
  bool enabled{false}, media_playing{false}, entity_configured{false};
  bool attribute_conditions_match{true}, hide_external_input{false}, external_input_active{false};
  bool schedule_blocks{false}, alarm_takeover_active{false};
};
inline bool policy_allows_feature(const PolicyInput &i) {
  return i.enabled && i.entity_configured && i.attribute_conditions_match && !(i.hide_external_input && i.external_input_active);
}
inline bool policy_allows_download(const PolicyInput &i) { return policy_allows_feature(i); }
inline bool policy_allows_display(const PolicyInput &i) {
  return policy_allows_feature(i) && i.media_playing && !i.schedule_blocks && !i.alarm_takeover_active;
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
                            bool alarm_active) {
  PolicyInput input;
  input.enabled = enabled;
  input.media_playing = media_playing;
  input.entity_configured = entity_configured;
  input.attribute_conditions_match = conditions_match;
  input.hide_external_input = hide_external_input;
  input.external_input_active = external_input_active;
  input.schedule_blocks = schedule_blocks;
  input.alarm_takeover_active = alarm_active;
  return policy_allows_display(input);
}

struct Layout {
  int screen_width, screen_height, art_x, art_y, art_size;
  int accent_x, accent_y, accent_width, accent_height;
  int panel_x, panel_y, panel_width, panel_height, title_max_height, panel_padding;
  bool split;
  int title_max_lines{0};
};
inline bool rotation_is_landscape(const std::string &slug, const std::string &rotation) {
  return slug == "guition-esp32-p4-jc4880p443" ? rotation == "90" || rotation == "270"
                                                : rotation == "0" || rotation == "180";
}
inline Layout cover_art_layout(const std::string &slug, const std::string &rotation,
                               int screen_width, int screen_height, int art_size, int title_height) {
  const bool landscape = rotation_is_landscape(slug, rotation);
  if (slug == "guition-esp32-p4-jc1060p470" || slug == "guition-esp32-p4-jc1060p470-v2") return landscape
    ? Layout{1024,600,0,0,600,585,0,439,600,615,24,377,440,332,0,true,4}
    : Layout{600,1024,0,0,600,0,600,600,424,30,634,540,360,162,0,true};
  if (slug == "guition-esp32-p4-jc4880p443") return landscape
    ? Layout{800,480,0,0,480,480,0,320,480,504,34,272,330,207,0,true,3}
    : Layout{480,800,0,0,480,0,480,480,320,24,514,324,262,130,0,true};
  if (slug == "guition-esp32-p4-jc8012p4a1" || slug == "guition-esp32-p4-jc8012p4a1-v2" ||
      slug == "guition-esp32-p4-jc8012p4a1-v3") return landscape
    ? Layout{1280,800,0,0,800,800,0,480,800,840,40,400,720,515,0,true,5}
    : Layout{800,1280,0,0,800,0,800,800,480,40,834,720,422,216,0,true};
  art_size = std::max(1, std::min(art_size, std::min(screen_width, screen_height)));
  int x = std::max(0, (screen_width - art_size) / 2), y = std::max(0, (screen_height - art_size) / 2);
  const int title_max_lines = slug == "guition-esp32-s3-4848s040" || slug == "esp32-p4-86" ? 3 : 0;
  return Layout{screen_width,screen_height,x,y,art_size,x,y,art_size,art_size,x,y,art_size,art_size,
                title_height,art_size >= 700 ? 36 : 24,false,title_max_lines};
}
struct PlaybackButtonLayout {
  int size, margin, panel_width, panel_height, title_max_height, panel_bottom_padding;
};
inline PlaybackButtonLayout playback_button_layout(const Layout &layout, int title_line_height = 0,
                                                   int title_line_space = 0) {
  const int short_side = std::min(layout.screen_width, layout.screen_height);
  const int size = std::clamp(short_side / 5, 80, 112);
  const int margin = std::clamp(short_side / 20, 24, 40);
  int width = layout.panel_width;
  int height = layout.panel_height;
  int title_height = layout.title_max_height;
  if (layout.title_max_lines > 0) {
    // Panels with a line budget keep room for artist/time.
    // Use the space above the button without reserving a second full margin.
    height = std::min(height, layout.screen_height - size - margin - layout.panel_y);
    if (title_line_height > 0) title_height = layout.title_max_lines * title_line_height +
        (layout.title_max_lines - 1) * title_line_space;
  } else if (layout.split && layout.screen_height > layout.screen_width) {
    // Portrait metadata and controls share the area below the artwork.
    width = std::min(width, layout.screen_width - size - 2 * margin - layout.panel_x);
  } else {
    // Square/landscape screens reserve a band below the metadata.
    height = std::min(height, layout.screen_height - size - 2 * margin - layout.panel_y);
    title_height = std::max(1, title_height - (layout.panel_height - height));
  }
  // The button's reserved band supplies the bottom spacing on square screens.
  const int bottom_padding = layout.title_max_lines > 0 ? 0 : layout.panel_padding;
  return {size, margin, width, height, title_height, bottom_padding};
}

inline int artist_height_budget(int panel_height, int title_height, int line_height,
                                int top_padding, int time_height) {
  const int available = std::max(0, panel_height - title_height - time_height - top_padding);
  return top_padding + (available / std::max(1, line_height)) * line_height;
}

inline bool progress_available(float duration) {
  return std::isfinite(duration) && duration > 0.0f;
}
inline int progress_percent(float position, float duration) {
  if (!progress_available(duration) || !std::isfinite(position)) return 0;
  return std::max(0, std::min(100, static_cast<int>((position / duration) * 100.0f + 0.5f)));
}

}  // namespace espcontrol::cover_art
#endif
