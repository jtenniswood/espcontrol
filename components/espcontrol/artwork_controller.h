#ifndef ESPCONTROL_ARTWORK_CONTROLLER_H
#define ESPCONTROL_ARTWORK_CONTROLLER_H
#pragma once

#include <string>

namespace espcontrol::artwork {

struct SourceSelection {
  std::string primary;
  std::string fallback;
  bool preferred_refreshed_remote{false};
};

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

  bool update(bool local, const std::string &url) {
    std::string &candidate = local ? local_url : remote_url;
    if (candidate == url) return false;
    candidate = url;
    if (!local) local_url.clear();
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
