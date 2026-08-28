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
using espcontrol::artwork::artwork_batch_needs_response_timer;
using espcontrol::artwork::artwork_empty_selection_preserves_pending_refresh;
using espcontrol::artwork::artwork_empty_selection_preserves_retry;
using espcontrol::artwork::artwork_entity_picture_present;
using espcontrol::artwork::artwork_timeout_retry_mask;
using espcontrol::artwork::artwork_pending_refresh_needs_reschedule;
using espcontrol::artwork::artwork_timeout_retry_allowed;
using espcontrol::artwork::artwork_timeout_exhaustion_preserves_current;
using espcontrol::artwork::artwork_metadata_refresh_clears_retry;
using espcontrol::artwork::artwork_refresh_forced;
using espcontrol::artwork::artwork_response_needs_processing;
using espcontrol::artwork::artwork_selection_needs_download;
using espcontrol::artwork::source_response_can_apply_immediately;
using espcontrol::cover_art::RuntimeState;
using espcontrol::cover_art::media_card_artwork_suppressed;
using espcontrol::cover_art::media_external_source_stale_for_current_content;

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

  // A source attribute omitted from the latest Home Assistant state must not
  // leave a retained TV route active once current music content arrives.
  assert(media_external_source_stale_for_current_content(true, false, true));
  assert(!media_external_source_stale_for_current_content(true, true, true));
  assert(!media_external_source_stale_for_current_content(true, false, false));
  assert(!media_external_source_stale_for_current_content(false, false, true));

  SourceCandidates sources;
  assert(sources.empty());
  assert(!artwork_entity_picture_present(""));
  assert(!artwork_entity_picture_present("unknown"));
  assert(!artwork_entity_picture_present("unavailable"));
  assert(artwork_entity_picture_present("/api/media_player_proxy/player"));

  // Home Assistant normally publishes the remote value first. It is usable
  // immediately, then the matching local value takes priority when it arrives.
  assert(sources.update(false, "remote-a"));
  assert(sources.select("", false).primary == "remote-a");
  assert(sources.update(true, "local-a"));
  auto selected = sources.select("remote-a", false);
  assert(selected.primary == "local-a");
  assert(selected.fallback == "remote-a");

  // A genuinely changed remote source is promoted while the stable local
  // proxy remains available as fallback.
  sources.begin_refresh();
  assert(sources.update(false, "remote-b"));
  assert(sources.local_url == "local-a");
  selected = sources.select("local-a", true);
  assert(selected.primary == "remote-b");
  assert(selected.fallback == "local-a");
  assert(selected.preferred_refreshed_remote);
  sources.finish_refresh();

  // Repeated events are idempotent and an empty local result retains the
  // current remote source.
  assert(!sources.update(false, "remote-b"));
  assert(sources.update(true, ""));
  assert(sources.select("remote-b", false).primary == "remote-b");

  // Media-card remote/local requests can finish out of order. A delayed
  // remote callback must preserve the newer local result.
  sources.clear();
  sources.begin_refresh();
  assert(sources.update(true, "local-new"));
  assert(sources.update(false, "remote-old", RemoteUpdatePolicy::PRESERVE_LOCAL));
  selected = sources.select("", false);
  assert(selected.primary == "local-new");
  assert(selected.fallback == "remote-old");
  sources.finish_refresh();

  // Metadata-only refreshes retain both unchanged candidates and select the
  // local proxy for a forced cache-busting download. Callback order is
  // irrelevant because neither response clears its companion.
  sources.begin_refresh();
  assert(!sources.update(false, "remote-old"));
  assert(!sources.update(true, "local-new"));
  selected = sources.select("local-new", true);
  assert(selected.primary == "local-new");
  assert(selected.fallback == "remote-old");
  assert(!selected.preferred_refreshed_remote);
  sources.finish_refresh();

  sources.begin_refresh();
  assert(sources.update(false, "remote-new"));
  assert(!sources.update(true, "local-new"));
  selected = sources.select("local-new", true);
  assert(selected.primary == "remote-new");
  assert(selected.fallback == "local-new");
  assert(selected.preferred_refreshed_remote);
  sources.finish_refresh();

  // A second paired read can supersede the first while its 300 ms selection
  // debounce is still running. An unchanged repeat of the new remote URL must
  // not erase the first read's change marker or select the stale local proxy.
  sources.begin_refresh();
  assert(sources.update(false, "remote-latest"));
  sources.begin_refresh();
  assert(!sources.update(false, "remote-latest"));
  selected = sources.select("local-new", sources.remote_changed_in_refresh());
  assert(selected.primary == "remote-latest");
  assert(selected.fallback == "local-new");
  assert(selected.preferred_refreshed_remote);
  sources.finish_refresh();

  // Paired callbacks settle exactly once regardless of arrival order. This is
  // the contract that prevents a local response and its remote companion from
  // starting two serialized image requests for one Home Assistant refresh.
  auto settle_pair = [](bool local_first) {
    SourceCandidates paired;
    paired.remote_url = "remote-stable";
    paired.local_url = "local-stable";
    paired.begin_refresh();
    RefreshBatch paired_batch;
    const uint32_t generation = paired_batch.begin(ARTWORK_SOURCE_BOTH, true);
    int selections = 0;
    auto receive = [&](bool local) {
      assert(paired_batch.receive(generation, local));
      assert(!paired.update(local, local ? "local-stable" : "remote-stable"));
      if (!paired_batch.complete()) return;
      const auto pair_selection = paired.select(
          "local-stable", paired.remote_changed_in_refresh());
      assert(pair_selection.primary == "local-stable");
      ++selections;
      assert(paired_batch.finish());
      paired.finish_refresh();
    };
    receive(local_first);
    receive(!local_first);
    assert(selections == 1);
  };
  settle_pair(true);
  settle_pair(false);

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
  assert(artwork_empty_selection_preserves_retry(true, ARTWORK_SOURCE_REMOTE));
  assert(!artwork_empty_selection_preserves_retry(true, 0));
  assert(!artwork_empty_selection_preserves_retry(false, ARTWORK_SOURCE_REMOTE));
  assert(artwork_timeout_retry_mask(0, ARTWORK_SOURCE_REMOTE, false) ==
         ARTWORK_SOURCE_REMOTE);
  assert(artwork_timeout_retry_mask(ARTWORK_SOURCE_LOCAL,
                                    ARTWORK_SOURCE_REMOTE, false) ==
         ARTWORK_SOURCE_BOTH);
  assert(artwork_timeout_retry_mask(ARTWORK_SOURCE_LOCAL,
                                    ARTWORK_SOURCE_REMOTE, true) == 0);
  assert(artwork_pending_refresh_needs_reschedule(true, false));
  assert(!artwork_pending_refresh_needs_reschedule(true, true));
  assert(!artwork_pending_refresh_needs_reschedule(false, false));
  assert(artwork_timeout_retry_allowed(0, 3));
  assert(artwork_timeout_retry_allowed(2, 3));
  assert(!artwork_timeout_retry_allowed(3, 3));
  assert(artwork_timeout_exhaustion_preserves_current(true, true));
  assert(!artwork_timeout_exhaustion_preserves_current(true, false));
  assert(!artwork_timeout_exhaustion_preserves_current(false, true));
  assert(artwork_metadata_refresh_clears_retry(ARTWORK_SOURCE_REMOTE));
  assert(artwork_metadata_refresh_clears_retry(ARTWORK_SOURCE_BOTH));
  assert(!artwork_metadata_refresh_clears_retry(0));
  assert(artwork_batch_needs_response_timer(true, false));
  assert(!artwork_batch_needs_response_timer(true, true));
  assert(!artwork_batch_needs_response_timer(false, false));
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

  // entity_picture is authoritative. An empty remote response immediately
  // suppresses even a retained local proxy, so it cannot restore stale art.
  assert(sources.update(false, "", RemoteUpdatePolicy::PRESERVE_LOCAL));
  selected = sources.select("local-stable", false);
  assert(selected.primary.empty());
  assert(sources.local_url == "local-stable");
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
  assert(batch.missing_mask() == ARTWORK_SOURCE_LOCAL);
  assert(batch.finish());
  assert(!batch.receive(timed_out_read, true));

  // A queued provider that never invokes its callback remains eligible for a
  // bounded retry instead of being treated as an authoritative empty result.
  const uint32_t stalled_read = batch.begin(ARTWORK_SOURCE_BOTH, false);
  assert(stalled_read != timed_out_read);
  assert(batch.missing_mask() == ARTWORK_SOURCE_BOTH);
  assert(batch.receive(stalled_read, true));
  assert(batch.missing_mask() == ARTWORK_SOURCE_REMOTE);
  assert(batch.finish());

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
