#ifndef ESPCONTROL_ARTWORK_CONTROLLER_H
#define ESPCONTROL_ARTWORK_CONTROLLER_H
#pragma once

#include <cstdint>
#include <string>

namespace espcontrol::artwork {

struct SourceSelection {
  std::string primary;
  std::string fallback;
  bool preferred_refreshed_remote{false};
};

enum class RemoteUpdatePolicy {
  START_NEW_GENERATION,
  PRESERVE_LOCAL,
};

constexpr uint8_t ARTWORK_SOURCE_REMOTE = 1u << 0;
constexpr uint8_t ARTWORK_SOURCE_LOCAL = 1u << 1;
constexpr uint8_t ARTWORK_SOURCE_BOTH = ARTWORK_SOURCE_REMOTE | ARTWORK_SOURCE_LOCAL;

constexpr uint8_t artwork_source_mask(bool local) {
  return local ? ARTWORK_SOURCE_LOCAL : ARTWORK_SOURCE_REMOTE;
}

// A zero retry mask means this is a normal refresh and both sources should be
// requested. A non-zero mask contains only the source reads that previously
// failed to queue.
constexpr uint8_t artwork_source_request_mask(uint8_t retry_mask) {
  return retry_mask == 0 ? ARTWORK_SOURCE_BOTH : retry_mask;
}

constexpr uint8_t artwork_source_failed_mask(uint8_t request_mask,
                                             bool remote_queued,
                                             bool local_queued) {
  uint8_t failed = 0;
  if ((request_mask & ARTWORK_SOURCE_REMOTE) != 0 && !remote_queued) {
    failed |= ARTWORK_SOURCE_REMOTE;
  }
  if ((request_mask & ARTWORK_SOURCE_LOCAL) != 0 && !local_queued) {
    failed |= ARTWORK_SOURCE_LOCAL;
  }
  return failed;
}

constexpr uint8_t artwork_source_mark_received(uint8_t retry_mask, bool local) {
  return retry_mask & static_cast<uint8_t>(~artwork_source_mask(local));
}

// A successful response must not cancel a retry that is still needed for the
// other media-artwork source.
constexpr bool artwork_picture_response_clears_retry(bool media_artwork,
                                                     uint8_t retry_mask) {
  return !media_artwork || retry_mask == 0;
}

// A usable local proxy response is already the preferred source, so there is
// no benefit in waiting for the remote fallback response before applying it.
constexpr bool source_response_can_apply_immediately(bool local_response,
                                                     bool usable_url) {
  return local_response && usable_url;
}

// Owns the ordering rules for Home Assistant's remote and local artwork URLs.
// A new remote URL starts a new artwork generation, so any cached local URL is
// discarded until the matching local attribute arrives.
struct SourceCandidates {
  std::string remote_url;
  std::string local_url;

  bool empty() const { return remote_url.empty() && local_url.empty(); }

  const std::string &get(bool local) const {
    return local ? local_url : remote_url;
  }

  bool update(
      bool local, const std::string &url,
      RemoteUpdatePolicy remote_policy = RemoteUpdatePolicy::START_NEW_GENERATION) {
    std::string &candidate = local ? local_url : remote_url;
    if (candidate == url) return false;
    candidate = url;
    if (!local && remote_policy == RemoteUpdatePolicy::START_NEW_GENERATION) {
      local_url.clear();
    }
    return true;
  }

  SourceSelection select(const std::string &current_url,
                         bool refresh_needed) const {
    SourceSelection selection;
    selection.primary = local_url.empty() ? remote_url : local_url;
    if (!local_url.empty() && !remote_url.empty() &&
        remote_url != selection.primary) {
      selection.fallback = remote_url;
    }

    // Local proxy URLs can remain unchanged while Home Assistant publishes a
    // refreshed remote URL for a new track. Prefer that fresh URL immediately.
    if (refresh_needed && !remote_url.empty() && remote_url != current_url &&
        selection.primary == current_url) {
      selection.fallback = selection.primary;
      selection.primary = remote_url;
      selection.preferred_refreshed_remote = true;
    }
    return selection;
  }

  void clear() {
    remote_url.clear();
    local_url.clear();
  }
};

}  // namespace espcontrol::artwork

#endif
