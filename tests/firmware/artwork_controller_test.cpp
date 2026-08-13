#include <cassert>

#include "artwork_controller.h"
#include "cover_art.h"

using espcontrol::artwork::SourceCandidates;
using espcontrol::artwork::RemoteUpdatePolicy;
using espcontrol::artwork::ARTWORK_SOURCE_BOTH;
using espcontrol::artwork::ARTWORK_SOURCE_LOCAL;
using espcontrol::artwork::ARTWORK_SOURCE_REMOTE;
using espcontrol::artwork::RefreshBatch;
using espcontrol::artwork::RefreshTrigger;
using espcontrol::artwork::artwork_source_failed_mask;
using espcontrol::artwork::artwork_source_mark_received;
using espcontrol::artwork::artwork_source_request_mask;
using espcontrol::artwork::artwork_picture_response_clears_retry;
using espcontrol::artwork::artwork_batch_waits_for_companion;
using espcontrol::artwork::artwork_empty_selection_preserves_pending_refresh;
using espcontrol::artwork::artwork_refresh_forced;
using espcontrol::artwork::artwork_response_needs_processing;
using espcontrol::artwork::artwork_selection_needs_download;
using espcontrol::artwork::source_response_can_apply_immediately;
using espcontrol::cover_art::RuntimeState;
using espcontrol::cover_art::media_card_artwork_suppressed;

int main() {
  RefreshTrigger trigger;
  trigger.schedule(false);
  trigger.schedule(true);
  trigger.schedule(false);
  assert(trigger.pending && trigger.forced);
  assert(trigger.consume());
  assert(!trigger.pending && !trigger.forced);
  trigger.schedule(false);
  assert(!trigger.consume());

  // A media entity may not expose a source attribute. Valid downloaded artwork
  // remains visible until an external input is positively identified.
  assert(!media_card_artwork_suppressed(false, false));
  assert(!media_card_artwork_suppressed(false, true));
  assert(!media_card_artwork_suppressed(true, false));
  assert(media_card_artwork_suppressed(true, true));

  SourceCandidates sources;
  assert(sources.empty());

  // Home Assistant normally publishes the remote value first. It is usable
  // immediately, then the matching local value takes priority when it arrives.
  assert(sources.update(false, "remote-a"));
  assert(sources.select("", false).primary == "remote-a");
  assert(sources.update(true, "local-a"));
  auto selected = sources.select("remote-a", false);
  assert(selected.primary == "local-a");
  assert(selected.fallback == "remote-a");

  // A new remote event represents a new artwork generation. Stale local art
  // must not remain selected while the matching local attribute is in flight.
  assert(sources.update(false, "remote-b"));
  assert(sources.local_url.empty());
  assert(sources.select("local-a", false).primary == "remote-b");

  // Repeated events are idempotent and an empty local result retains the
  // current remote fallback.
  assert(!sources.update(false, "remote-b"));
  assert(!sources.update(true, ""));
  assert(sources.select("remote-b", false).primary == "remote-b");

  // Media-card remote/local requests can finish out of order. A delayed
  // remote callback must preserve the newer local result.
  sources.clear();
  assert(sources.update(true, "local-new"));
  assert(sources.update(false, "remote-old", RemoteUpdatePolicy::PRESERVE_LOCAL));
  selected = sources.select("", false);
  assert(selected.primary == "local-new");
  assert(selected.fallback == "remote-old");

  // A valid local proxy is already preferred and can skip the media-card
  // debounce. Remote or empty responses must retain fallback scheduling.
  assert(source_response_can_apply_immediately(true, true));
  assert(!source_response_can_apply_immediately(false, true));
  assert(!source_response_can_apply_immediately(true, false));

  // Ordinary player-state updates with the same artwork remain no-ops. Only a
  // changed source or an explicit reconnect/recovery refresh is processed.
  assert(!artwork_response_needs_processing(false, false));
  assert(artwork_response_needs_processing(false, true));
  assert(artwork_response_needs_processing(true, false));
  assert(artwork_response_needs_processing(true, true));
  assert(!artwork_selection_needs_download(false, true));
  assert(artwork_selection_needs_download(false, false));
  assert(artwork_selection_needs_download(true, true));
  assert(artwork_batch_waits_for_companion(false, true));
  assert(!artwork_batch_waits_for_companion(false, true, true));
  assert(!artwork_batch_waits_for_companion(false, false));
  assert(!artwork_batch_waits_for_companion(true, true));
  assert(artwork_empty_selection_preserves_pending_refresh(true, true));
  assert(!artwork_empty_selection_preserves_pending_refresh(true, false));
  assert(!artwork_empty_selection_preserves_pending_refresh(false, true));
  assert(artwork_refresh_forced(true, false, false));
  assert(artwork_refresh_forced(false, true, false));
  assert(artwork_refresh_forced(false, false, true));
  assert(!artwork_refresh_forced(false, false, false));

  // A state update with the same selected local artwork does not download it
  // again. Reconnect and attribute-read retry use the forced path instead.
  sources.clear();
  assert(sources.update(false, "remote-stable"));
  assert(sources.update(true, "local-stable"));
  selected = sources.select("local-stable", false);
  assert(selected.primary == "local-stable");
  assert(!artwork_selection_needs_download(false,
                                            selected.primary == "local-stable"));
  assert(artwork_selection_needs_download(true,
                                          selected.primary == "local-stable"));

  // A forced reconnect refresh keeps the current local proxy selected. The
  // forced flag requests a new download; it must not promote an older remote
  // fallback simply because both candidates are unchanged.
  selected = sources.select("local-stable", false);
  assert(selected.primary == "local-stable");
  assert(selected.fallback == "remote-stable");

  // Empty responses are a real artwork update. Once both sources are empty,
  // the caller must clear/cancel the currently displayed artwork.
  assert(sources.update(false, "", RemoteUpdatePolicy::PRESERVE_LOCAL));
  assert(sources.update(true, ""));
  selected = sources.select("local-stable", false);
  assert(selected.primary.empty());

  // A partial queue failure retries only the source that failed. Reads that
  // were already accepted must not accumulate duplicate deferred callbacks.
  assert(artwork_source_request_mask(0) == ARTWORK_SOURCE_BOTH);
  assert(artwork_source_failed_mask(ARTWORK_SOURCE_BOTH, true, false) ==
         ARTWORK_SOURCE_LOCAL);
  assert(artwork_source_request_mask(ARTWORK_SOURCE_LOCAL) == ARTWORK_SOURCE_LOCAL);
  assert(artwork_source_failed_mask(ARTWORK_SOURCE_LOCAL, true, false) ==
         ARTWORK_SOURCE_LOCAL);
  assert(artwork_source_failed_mask(ARTWORK_SOURCE_LOCAL, true, true) == 0);
  assert(artwork_source_mark_received(ARTWORK_SOURCE_BOTH, false) ==
         ARTWORK_SOURCE_LOCAL);
  assert(artwork_source_mark_received(ARTWORK_SOURCE_BOTH, true) ==
         ARTWORK_SOURCE_REMOTE);
  assert(artwork_picture_response_clears_retry(false, ARTWORK_SOURCE_LOCAL));
  assert(artwork_picture_response_clears_retry(true, 0));
  assert(!artwork_picture_response_clears_retry(true, ARTWORK_SOURCE_LOCAL));

  // One Home Assistant refresh reads both artwork attributes. Arrival order
  // cannot produce two image requests, and callbacks from a superseded read
  // must be ignored.
  RefreshBatch batch;
  const uint32_t first_read = batch.begin(ARTWORK_SOURCE_BOTH, false);
  assert(batch.accepts(first_read, false));
  assert(batch.receive(first_read, false));
  assert(!batch.complete());
  assert(batch.receive(first_read, true));
  assert(batch.complete());
  assert(batch.finish());
  assert(!batch.active());

  const uint32_t second_read = batch.begin(ARTWORK_SOURCE_BOTH, true);
  assert(second_read != first_read && batch.forced);
  assert(!batch.receive(first_read, true));
  assert(batch.receive(second_read, true));
  assert(!batch.complete());
  assert(batch.receive(second_read, false));
  assert(batch.complete());
  assert(batch.finish());

  // Each paired read captures its own generation. A delayed response from the
  // previous track cannot be accepted into the newer track's batch.
  const uint32_t track_a_read = batch.begin(ARTWORK_SOURCE_BOTH, false);
  const uint32_t track_b_read = batch.begin(ARTWORK_SOURCE_BOTH, false);
  assert(track_b_read != track_a_read);
  assert(!batch.receive(track_a_read, true));
  assert(batch.receive(track_b_read, false));
  assert(batch.receive(track_b_read, true));
  assert(batch.complete());
  assert(batch.finish());

  // A timeout may settle a one-sided response, after which its delayed
  // companion must not replace the selected artwork mid-download.
  const uint32_t timed_out_read = batch.begin(ARTWORK_SOURCE_BOTH, false);
  assert(batch.receive(timed_out_read, false));
  assert(batch.finish());
  assert(!batch.receive(timed_out_read, true));

  // Partial retry reads track only the failed source and complete after that
  // one response, preserving the existing source as the fallback.
  const uint32_t local_retry = batch.begin(ARTWORK_SOURCE_LOCAL, true);
  assert(batch.receive(local_retry, true));
  assert(batch.complete());
  assert(batch.finish());

  // A retry of a failed local read leaves the earlier remote result available
  // when the retry returns empty.
  sources.clear();
  assert(sources.update(false, "remote-retry", RemoteUpdatePolicy::PRESERVE_LOCAL));
  assert(!sources.update(true, ""));
  selected = sources.select("", false);
  assert(selected.primary == "remote-retry");

  // When a stable local proxy URL still points at the previous track, a fresh
  // remote URL wins for every changed track and the local URL remains the
  // fallback—even when the prior refresh had already selected a remote URL.
  sources.clear();
  assert(sources.update(true, "stable-local"));
  assert(sources.update(false, "remote-c", RemoteUpdatePolicy::PRESERVE_LOCAL));
  selected = sources.select("stable-local", true);
  assert(selected.primary == "remote-c");
  assert(selected.fallback == "stable-local");
  assert(selected.preferred_refreshed_remote);
  assert(sources.update(false, "remote-d", RemoteUpdatePolicy::PRESERVE_LOCAL));
  selected = sources.select("remote-c", true);
  assert(selected.primary == "remote-d");
  assert(selected.fallback == "stable-local");
  assert(selected.preferred_refreshed_remote);

  // Download completions from an old track must not make the controller think
  // the newly selected track is current.
  RuntimeState runtime;
  runtime.select_source("track-a");
  runtime.begin_download("track-a?refresh=1");
  runtime.select_source("track-b");
  assert(runtime.apply_download("track-a?refresh=1"));
  assert(runtime.loaded_url == "track-a");
  assert(runtime.needs_download());

  runtime.sources.update(false, "remote-track-b");
  runtime.sources.update(true, "local-track-b");
  runtime.clear_image();
  assert(runtime.sources.empty());
  assert(!runtime.download_active());
  assert(!runtime.needs_download());
}
