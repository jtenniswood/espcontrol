#pragma once

#include <string>

namespace espcontrol::image_card {

// A Home Assistant `image.*` entity reports its image-update timestamp as the
// entity state. The proxy URL only changes when the access token rotates, so a
// new timestamp is the only signal that a new picture exists at the same URL.
// Camera entities keep the existing picture/state handling untouched.

inline bool image_entity_revision_state_valid(const std::string &value) {
  return !value.empty() && value != "unknown" && value != "unavailable" &&
         value != "None";
}

struct ImageEntityRevision {
  std::string state;
  bool pending = false;

  // Records a state update. Returns true while a revision has been seen but
  // not yet requested from Home Assistant.
  bool observe(const std::string &value) {
    if (!image_entity_revision_state_valid(value) || value == state) return pending;
    state = value;
    pending = true;
    return true;
  }

  // The pending revision has been handed to a downloader.
  void acknowledge() { pending = false; }

  void reset() {
    state.clear();
    pending = false;
  }
};

// The recent-refresh guard exists to avoid re-downloading an unchanged
// picture. A pending image-entity revision is a changed picture by definition.
constexpr bool image_entity_revision_bypasses_refresh_guard(bool image_entity,
                                                            bool pending) {
  return image_entity && pending;
}

}  // namespace espcontrol::image_card
