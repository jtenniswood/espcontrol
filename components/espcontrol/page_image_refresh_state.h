#pragma once

#include <algorithm>
#include <type_traits>

#include "page_image_tile_policy.h"

namespace espcontrol::pages {
enum class PageImagePrimaryUpdate : uint8_t { IGNORED, CHANGED, UNAVAILABLE, RECOVERED };
// Fixed bytes can move between a visible tile and one demanded expanded view.
// They own no graph/configuration/URL pointer and allocate nothing on delivery.
struct PageImageRefreshState {
  camera::RefreshSchedule schedule;
  PageImageActivity activity;
  PageImageRevision picture;
  uint32_t connection{0}, last_tile_completed{0}, camera_retry_after{0}, source_changed_at{0};
  uint8_t camera_errors{0};
  bool source_dirty{false}, expanded_completed{false}, expanded_success{false}, tile_invalidated{false};
  bool primary_seeded{false}, activity_seeded{false}, unavailable{false}, picture_event{false};
  bool synchronize(uint32_t epoch) {
    if (connection == epoch) return false;
    connection = epoch;
    primary_seeded = activity_seeded = false;
    activity = {};
    picture.invalidate();
    schedule.in_flight = false;
    camera_errors = 0;
    camera_retry_after = last_tile_completed = 0;
    source_dirty = expanded_completed = expanded_success = false;
    tile_invalidated = true;
    return true;
  }
  PageImagePrimaryUpdate primary(PageImageEntityKind kind, std::string_view value, uint32_t now) {
    primary_seeded = true;
    const bool next = page_image_unavailable(value);
    if (kind == PageImageEntityKind::IMAGE) {
      if (next)
        picture.invalidate();
      else if (!picture.observe(value))
        return PageImagePrimaryUpdate::IGNORED;
    }
    const bool recovered = unavailable && !next;
    unavailable = next;
    picture_event = false;
    source_dirty = true;
    source_changed_at = now;
    if (next) {
      tile_invalidated = true;
      expanded_success = false;
    }
    if (recovered) {
      camera_errors = 0;
      camera_retry_after = last_tile_completed = 0;
    }
    return next        ? PageImagePrimaryUpdate::UNAVAILABLE
           : recovered ? PageImagePrimaryUpdate::RECOVERED
                       : PageImagePrimaryUpdate::CHANGED;
  }
  void picture_update(uint32_t now) {
    picture_event = true;
    source_dirty = true;
    source_changed_at = now;
  }
  bool token_update(std::string_view value, std::string_view previous, uint32_t now) {
    if (value.substr(0, 512) == previous.substr(0, 512)) return false;
    picture_event = false;
    source_dirty = true;
    source_changed_at = now;
    return true;
  }
  void camera_failed(uint32_t now) {
    camera_errors = std::min<unsigned>(5, camera_errors + 1);
    camera_retry_after = now + std::min<uint32_t>(30000, 2000u << (camera_errors - 1));
    tile_invalidated = true;
    expanded_success = false;
  }
  void tile_succeeded(uint32_t now) {
    last_tile_completed = now;
    tile_invalidated = false;
    camera_errors = 0;
    camera_retry_after = 0;
  }
  void expanded_finished(uint32_t now, bool success) {
    schedule.finished(now, success);
    expanded_completed = true;
    expanded_success = success;
    if (success) {
      camera_errors = 0;
      camera_retry_after = 0;
    } else
      camera_failed(now);
  }
  bool trigger(std::string_view value, bool binary, uint32_t now, bool visible) {
    activity_seeded = true;
    const bool edge = activity.observe(value, binary, connection);
    if (edge && visible) {
      if (!schedule.open) schedule.begin(now, false);
      schedule.activate(now);
    }
    return edge && visible;
  }
};
static_assert(std::is_trivially_copyable<PageImageRefreshState>::value);
}  // namespace espcontrol::pages
