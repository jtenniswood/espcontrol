#pragma once

#include <cstddef>
#include <vector>

// A grid rebuild must temporarily return to the home page only when the grid
// already owns the visible screen. Setup, update, and other full-screen flows
// are independent of the grid and must remain visible while its widgets are
// refreshed in the background.
inline bool grid_navigation_rebuild_should_return_home(
    bool home_screen_active, int active_subpage_slot) {
  return home_screen_active || active_subpage_slot > 0;
}

// Owns the runtime navigation registries for one button-grid instance.  The
// entries remain defined by the UI layer so this service stays independent of
// LVGL and can be exercised on the host.
template <typename HomeTarget, typename Subpage>
class GridNavigationService {
 public:
  std::vector<HomeTarget> &home_targets() { return home_targets_; }
  const std::vector<HomeTarget> &home_targets() const { return home_targets_; }

  std::vector<Subpage> &subpages() { return subpages_; }
  const std::vector<Subpage> &subpages() const { return subpages_; }

  size_t home_target_count() const { return home_targets_.size(); }
  size_t subpage_count() const { return subpages_.size(); }

  void clear_home_targets() { home_targets_.clear(); }
  void clear_subpages() { subpages_.clear(); }

 private:
  std::vector<HomeTarget> home_targets_;
  std::vector<Subpage> subpages_;
};
