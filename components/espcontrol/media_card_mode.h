#pragma once
#include <cstdint>

namespace espcontrol::media {
// Shared compiled view; saved configuration continues to use the existing mode strings.
enum class Mode : uint8_t {
  CONTROL_MODAL,
  SPEAKER_GROUP,
  PLAY_PAUSE,
  PREVIOUS,
  NEXT,
  VOLUME,
  POSITION,
  NOW_PLAYING,
  COVER_ART,
  PLAYLIST,
};
}  // namespace espcontrol::media
