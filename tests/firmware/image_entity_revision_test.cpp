#include <cassert>
#include <cstdlib>

#include "image_entity_revision.h"

using espcontrol::image_card::ImageEntityRevision;
using espcontrol::image_card::image_entity_revision_bypasses_refresh_guard;
using espcontrol::image_card::image_entity_revision_state_valid;

int main() {
  // Placeholder states never describe a picture.
  if (image_entity_revision_state_valid("")) return EXIT_FAILURE;
  if (image_entity_revision_state_valid("unknown")) return EXIT_FAILURE;
  if (image_entity_revision_state_valid("unavailable")) return EXIT_FAILURE;
  if (image_entity_revision_state_valid("None")) return EXIT_FAILURE;
  if (!image_entity_revision_state_valid("2026-09-21T15:28:00+00:00")) return EXIT_FAILURE;

  ImageEntityRevision revision;
  if (revision.pending) return EXIT_FAILURE;

  // The first valid timestamp is a revision until a download picks it up.
  if (!revision.observe("2026-09-21T15:28:00+00:00")) return EXIT_FAILURE;
  if (!revision.pending) return EXIT_FAILURE;
  revision.acknowledge();
  if (revision.pending) return EXIT_FAILURE;

  // Repeating the same timestamp (token rotation, reconnect replay) is not new.
  if (revision.observe("2026-09-21T15:28:00+00:00")) return EXIT_FAILURE;
  if (revision.pending) return EXIT_FAILURE;

  // A placeholder state neither clears nor creates a revision.
  if (revision.observe("unavailable")) return EXIT_FAILURE;
  if (revision.state != "2026-09-21T15:28:00+00:00") return EXIT_FAILURE;

  // A newer timestamp is a new picture at the same URL.
  if (!revision.observe("2026-09-21T15:28:10+00:00")) return EXIT_FAILURE;
  if (!revision.pending) return EXIT_FAILURE;

  // A second update before the download starts stays a single pending revision.
  if (!revision.observe("2026-09-21T15:28:20+00:00")) return EXIT_FAILURE;
  if (revision.state != "2026-09-21T15:28:20+00:00") return EXIT_FAILURE;
  revision.acknowledge();
  if (revision.pending) return EXIT_FAILURE;

  revision.reset();
  if (!revision.state.empty() || revision.pending) return EXIT_FAILURE;

  // Only image entities with a pending revision may skip the refresh guard.
  if (!image_entity_revision_bypasses_refresh_guard(true, true)) return EXIT_FAILURE;
  if (image_entity_revision_bypasses_refresh_guard(true, false)) return EXIT_FAILURE;
  if (image_entity_revision_bypasses_refresh_guard(false, true)) return EXIT_FAILURE;
  if (image_entity_revision_bypasses_refresh_guard(false, false)) return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
