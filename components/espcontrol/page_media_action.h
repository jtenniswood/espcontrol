#pragma once

#include <cstdint>
#include <string_view>

#include "media_card_mode.h"

namespace espcontrol::pages {
// Prepared configuration for the media modes which use the common icon/label
// shell. Other media modes still fail preparation until their view is ported.
using MediaActionKind = media::Mode;
struct MediaActionOptions {
  MediaActionKind kind{MediaActionKind::PLAY_PAUSE};
  bool show_state{false}, progress{false}, play_pause_background{false};
  std::string_view playlist_id, playlist_type;
};
}  // namespace espcontrol::pages
