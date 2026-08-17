#include <cassert>

#include "espcontrol_app_core.h"
#include "grid_navigation_service.h"

namespace {

struct HomeTarget {
  int slot = 0;
};

struct Subpage {
  int slot = 0;
};

}  // namespace

int main() {
  GridNavigationService<HomeTarget, Subpage> navigation;

  navigation.home_targets().push_back({1});
  navigation.subpages().push_back({2});

  assert(navigation.home_target_count() == 1);
  assert(navigation.subpage_count() == 1);
  assert(navigation.home_targets().front().slot == 1);
  assert(navigation.subpages().front().slot == 2);

  navigation.clear_home_targets();
  assert(navigation.home_target_count() == 0);
  assert(navigation.subpage_count() == 1);

  navigation.clear_subpages();
  assert(navigation.subpage_count() == 0);

  espcontrol::EspControlAppCore app;
  assert(app.start());
  auto &core_navigation = app.grid_navigation_service<
      GridNavigationService<HomeTarget, Subpage>>();
  core_navigation.home_targets().push_back({3});
  assert(core_navigation.home_target_count() == 1);
  assert(app.stop());
  return 0;
}
