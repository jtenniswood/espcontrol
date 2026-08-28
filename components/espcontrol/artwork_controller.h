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

inline bool artwork_entity_picture_present(const std::string &value) {
  return !value.empty() && value != "unknown" && value != "unavailable";
}

// Coalesces adjacent refresh triggers before a paired Home Assistant read.
// A forced trigger must survive later ordinary triggers in the same window.
struct RefreshTrigger {
  bool pending{false};
  bool forced{false};

  void schedule(bool force_refresh) {
    this->pending = true;
    this->forced = this->forced || force_refresh;
  }

  bool consume() {
    const bool force_refresh = this->forced;
    this->pending = false;
    this->forced = false;
    return force_refresh;
  }

  void reset() {
    this->pending = false;
    this->forced = false;
  }
};

constexpr uint8_t artwork_source_mask(bool local) {
  return local ? ARTWORK_SOURCE_LOCAL : ARTWORK_SOURCE_REMOTE;
}

// Tracks one paired Home Assistant artwork read. The two attributes are
// delivered independently, so consumers must not allow the first reply to
// start an image request that the companion reply immediately replaces.
struct RefreshBatch {
  uint32_t generation{0};
  uint8_t expected_mask{0};
  uint8_t received_mask{0};
  bool forced{false};

  uint32_t begin(uint8_t expected, bool force_refresh) {
    ++this->generation;
    if (this->generation == 0) ++this->generation;
    this->expected_mask = expected;
    this->received_mask = 0;
    this->forced = force_refresh;
    return this->generation;
  }

  bool accepts(uint32_t request_generation, bool local) const {
    return this->expected_mask != 0 && request_generation == this->generation &&
           (this->expected_mask & artwork_source_mask(local)) != 0;
  }

  bool receive(uint32_t request_generation, bool local) {
    if (!this->accepts(request_generation, local)) return false;
    this->received_mask |= artwork_source_mask(local);
    return true;
  }

  bool complete() const {
    return this->expected_mask != 0 &&
           (this->received_mask & this->expected_mask) == this->expected_mask;
  }

  uint8_t missing_mask() const {
    return this->expected_mask & static_cast<uint8_t>(~this->received_mask);
  }

  bool active() const { return this->expected_mask != 0; }

  bool finish() {
    if (!this->active()) return false;
    this->expected_mask = 0;
    this->received_mask = 0;
    return true;
  }

  void reset() {
    this->expected_mask = 0;
    this->received_mask = 0;
    this->forced = false;
  }
};

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

// Normal Home Assistant state updates only need processing when their artwork
// source changes. Explicit reconnects and attribute-read recovery deliberately
// refresh an unchanged source.
constexpr bool artwork_response_needs_processing(bool source_changed,
                                                 bool refresh_forced) {
  return source_changed || refresh_forced;
}

// An incomplete batch with no usable candidate remains open only while its
// bounded response window is active. Some media players never return one of
// the paired attributes, so waiting after that window would block every later
// artwork refresh behind a batch that can never complete.
constexpr bool artwork_batch_waits_for_companion(bool batch_complete,
                                                 bool selection_empty,
                                                 bool response_window_expired = false) {
  return !response_window_expired && !batch_complete && selection_empty;
}

// A metadata notification received while the current paired read is settling
// already promises a replacement read. Keep that pending refresh intact rather
// than clearing it with the empty result from the superseded batch.
constexpr bool artwork_empty_selection_preserves_pending_refresh(
    bool selection_empty, bool refresh_pending) {
  return selection_empty && refresh_pending;
}

constexpr bool artwork_empty_selection_preserves_retry(
    bool selection_empty, uint8_t retry_mask) {
  return selection_empty && retry_mask != 0;
}

constexpr uint8_t artwork_timeout_retry_mask(uint8_t current_retry_mask,
                                             uint8_t missing_mask,
                                             bool replacement_refresh_scheduled) {
  return replacement_refresh_scheduled
           ? 0
           : static_cast<uint8_t>(current_retry_mask | missing_mask);
}

constexpr bool artwork_pending_refresh_needs_reschedule(
    bool refresh_pending, bool timer_scheduled) {
  return refresh_pending && !timer_scheduled;
}

constexpr bool artwork_timeout_retry_allowed(uint8_t attempts,
                                             uint8_t max_attempts) {
  return attempts < max_attempts;
}

constexpr bool artwork_timeout_exhaustion_preserves_current(
    bool timeout_retry_exhausted, bool image_ready) {
  return timeout_retry_exhausted && image_ready;
}

constexpr bool artwork_metadata_refresh_clears_retry(uint8_t retry_mask) {
  return retry_mask != 0;
}

// Every active attribute-read batch needs a bounded deadline, including a
// retry generation whose provider never invokes the queued callback.
constexpr bool artwork_batch_needs_response_timer(bool batch_active,
                                                  bool timer_scheduled) {
  return batch_active && !timer_scheduled;
}

// A later ordinary refresh supersedes the earlier callbacks, but must retain
// the earlier request's explicit refresh requirement.
constexpr bool artwork_refresh_forced(bool active_forced,
                                     bool pending_forced,
                                     bool requested_forced) {
  return active_forced || pending_forced || requested_forced;
}

// A selected source only needs another download when it differs from the
// artwork already on screen, except for an explicit recovery refresh.
constexpr bool artwork_selection_needs_download(bool refresh_forced,
                                                bool source_matches_current) {
  return refresh_forced || !source_matches_current;
}

// Owns the ordering rules for Home Assistant's remote and local artwork URLs.
// A new remote URL starts a new artwork generation, so any cached local URL is
// discarded until the matching local attribute arrives.
struct SourceCandidates {
  std::string remote_url;
  std::string local_url;
  bool remote_changed{false};

  bool empty() const { return remote_url.empty() && local_url.empty(); }

  const std::string &get(bool local) const {
    return local ? local_url : remote_url;
  }

  bool update(
      bool local, const std::string &url,
      RemoteUpdatePolicy remote_policy = RemoteUpdatePolicy::PRESERVE_LOCAL) {
    std::string &candidate = local ? local_url : remote_url;
    if (candidate == url) return false;
    candidate = url;
    if (!local) this->remote_changed = true;
    if (!local && remote_policy == RemoteUpdatePolicy::START_NEW_GENERATION) {
      local_url.clear();
    }
    return true;
  }

  // A superseding paired read may begin during the debounce before the prior
  // candidates are selected. Preserve its remote-change marker until that
  // selection is actually consumed.
  void begin_refresh() {}

  void finish_refresh() { this->remote_changed = false; }

  bool remote_changed_in_refresh() const { return this->remote_changed; }

  SourceSelection select(const std::string &current_url,
                         bool refresh_needed) const {
    SourceSelection selection;
    // entity_picture is the authoritative indication that the entity has
    // artwork. entity_picture_local is only a transport-friendly version of
    // that image and must never keep stale artwork alive by itself.
    if (remote_url.empty()) return selection;
    selection.primary = local_url.empty() ? remote_url : local_url;
    if (!local_url.empty() && !remote_url.empty() &&
        remote_url != selection.primary) {
      selection.fallback = remote_url;
    }

    // A local proxy URL can remain unchanged across tracks. Promote the remote
    // candidate only when Home Assistant actually changed that attribute in
    // this paired refresh; merely differing from the selected local proxy does
    // not make a stable station favicon fresh artwork.
    if (refresh_needed && this->remote_changed && !remote_url.empty() &&
        remote_url != current_url) {
      selection.fallback = selection.primary;
      selection.primary = remote_url;
      selection.preferred_refreshed_remote = true;
    }
    return selection;
  }

  void clear() {
    remote_url.clear();
    local_url.clear();
    remote_changed = false;
  }
};

}  // namespace espcontrol::artwork

#endif
