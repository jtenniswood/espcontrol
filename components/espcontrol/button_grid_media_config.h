#pragma once

#include <cstdint>
#include <string>

// Versioned runtime view of the compact Media-card saved configuration.
// Included by button_grid_config_parser.h after ParsedCfg and option helpers.
namespace espcontrol::media {

constexpr uint8_t CONFIG_VERSION = 1;

enum class Mode : uint8_t {
  CONTROL_MODAL,
  PLAY_PAUSE,
  PREVIOUS,
  NEXT,
  VOLUME,
  POSITION,
  NOW_PLAYING,
  COVER_ART,
  PLAYLIST,
};

enum class StateDisplay : uint8_t { LABEL, STATE };
enum class NowPlayingControl : uint8_t { NONE, PROGRESS, PLAY_PAUSE };
enum class CoverArtAction : uint8_t { PLAY_PAUSE, CONTROL_MODAL };
enum class ControlLabelDisplay : uint8_t { STATUS, LABEL };
enum class ControlNumberDisplay : uint8_t { ICON, VOLUME };

struct ConfigV1 {
  uint8_t version = CONFIG_VERSION;
  const ParsedCfg *saved = nullptr;
  Mode mode = Mode::PLAY_PAUSE;
  StateDisplay state_display = StateDisplay::LABEL;
  NowPlayingControl now_playing_control = NowPlayingControl::NONE;
  CoverArtAction cover_art_action = CoverArtAction::PLAY_PAUSE;
  bool show_track_details = false;
  ControlLabelDisplay control_label_display = ControlLabelDisplay::STATUS;
  ControlNumberDisplay control_number_display = ControlNumberDisplay::ICON;
  int max_volume_percent = 100;
  std::string playlist_content_id;
  std::string playlist_content_type = "playlist";
  std::string playlist_player_source;
  bool large_numbers = false;
};

inline Mode mode_from_saved(const std::string &value) {
  const std::string mode = card_runtime_media_mode(value);
  if (mode == "control_modal") return Mode::CONTROL_MODAL;
  if (mode == "previous") return Mode::PREVIOUS;
  if (mode == "next") return Mode::NEXT;
  if (mode == "volume") return Mode::VOLUME;
  if (mode == "position") return Mode::POSITION;
  if (mode == "now_playing") return Mode::NOW_PLAYING;
  if (mode == "cover_art") return Mode::COVER_ART;
  if (mode == "playlist") return Mode::PLAYLIST;
  return Mode::PLAY_PAUSE;
}

inline ConfigV1 decode_config_v1(const ParsedCfg &saved) {
  ConfigV1 config;
  config.saved = &saved;
  config.mode = mode_from_saved(saved.sensor);
  if (config.mode == Mode::NOW_PLAYING &&
      cfg_option_token_present(saved.options, MEDIA_COVER_ART_OPTION)) {
    config.mode = Mode::COVER_ART;
  }
  if ((config.mode == Mode::PLAY_PAUSE || config.mode == Mode::POSITION) &&
      saved.precision == "state") {
    config.state_display = StateDisplay::STATE;
  }
  if (config.mode == Mode::NOW_PLAYING) {
    if (saved.precision == "progress") {
      config.now_playing_control = NowPlayingControl::PROGRESS;
    } else if (saved.precision == "play_pause") {
      config.now_playing_control = NowPlayingControl::PLAY_PAUSE;
    }
  }
  if (cfg_option_value(saved.options, MEDIA_COVER_ART_ACTION_OPTION) == "control_modal") {
    config.cover_art_action = CoverArtAction::CONTROL_MODAL;
  }
  config.show_track_details = cfg_option_token_present(
      saved.options, MEDIA_COVER_ART_DETAILS_OPTION);
  if (cfg_option_value(saved.options, LABEL_DISPLAY_OPTION) == "label") {
    config.control_label_display = ControlLabelDisplay::LABEL;
  }
  if (cfg_option_value(saved.options, NUMBER_DISPLAY_OPTION) == "volume") {
    config.control_number_display = ControlNumberDisplay::VOLUME;
  }
  config.max_volume_percent = normalize_media_volume_max_percent(
      cfg_option_value(saved.options, VOLUME_MAX_OPTION));
  config.playlist_content_id = trim_saved_option_value(
      cfg_option_value(saved.options, MEDIA_PLAYLIST_CONTENT_ID_OPTION));
  config.playlist_content_type = trim_saved_option_value(
      cfg_option_value(saved.options, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION));
  if (config.playlist_content_type.empty()) config.playlist_content_type = "playlist";
  config.playlist_player_source = trim_saved_option_value(
      cfg_option_value(saved.options, MEDIA_PLAYLIST_PLAYER_SOURCE_OPTION));
  config.large_numbers = cfg_option_token_present(saved.options, "large_numbers");
  return config;
}

}  // namespace espcontrol::media
