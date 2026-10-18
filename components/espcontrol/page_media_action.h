#pragma once

#include <cstdint>
#include <string_view>

namespace espcontrol::pages {
// Prepared configuration for the media modes which use the common icon/label
// shell. Other media modes still fail preparation until their view is ported.
enum class MediaActionKind : uint8_t { PLAY_PAUSE, PREVIOUS, NEXT, PLAYLIST };
struct MediaActionOptions {
  MediaActionKind kind{MediaActionKind::PLAY_PAUSE};
  bool show_state{false};
  std::string_view playlist_id, playlist_type;
};
}  // namespace espcontrol::pages
