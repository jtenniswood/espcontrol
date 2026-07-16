#include <cassert>

#include "artwork_controller.h"
#include "cover_art.h"

using espcontrol::artwork::SourceCandidates;
using espcontrol::artwork::RemoteUpdatePolicy;
using espcontrol::artwork::source_response_can_apply_immediately;
using espcontrol::cover_art::RuntimeState;

int main() {
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

  // When a stable local proxy URL still points at the previous track, a fresh
  // remote URL wins for the refresh and the local URL remains the fallback.
  sources.update(true, "stable-local");
  sources.remote_url = "remote-c";
  selected = sources.select("stable-local", true);
  assert(selected.primary == "remote-c");
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
