#include <cassert>
#include <string>
#include "media_control_tabs.h"

int main() {
  using namespace espcontrol::media;
  using Tab = MediaControlTab;
  const auto defaults = parse_control_tabs("");
  assert(defaults.count == 4 && !defaults.contains(Tab::POWER));
  assert(parse_control_tabs("", true).count == 5);
  assert(!parse_control_tabs("power|volume").contains(Tab::POWER));
  assert(normalize_control_tabs_value("power|volume") == "volume");
  assert(normalize_control_tabs_value(" \t") == DEFAULT_CONTROL_TABS);
  assert(normalize_control_tabs_value(" power |power|invalid| volume ", true) == "power|volume");
  assert(normalize_control_tabs_value("invalid||invalid") == "controls");
  const auto ordered = parse_control_tabs("power|speakers|volume|progress|controls", true);
  auto visible = visible_control_tabs(ordered, true, true, true);
  assert(visible.count == 5 && visible.tabs[0] == Tab::POWER && visible.tabs[1] == Tab::SPEAKERS);
  visible = visible_control_tabs(ordered, false, false, false);
  assert(visible.count == 2 && visible.tabs[0] == Tab::VOLUME && visible.tabs[1] == Tab::CONTROLS);
  visible = visible_control_tabs(parse_control_tabs("power|progress", true), false, false, false);
  assert(visible.count == 1 && visible.tabs[0] == Tab::CONTROLS);
  visible = visible_control_tabs(parse_control_tabs("power|progress", true), false, false, true);
  assert(visible.count == 1 && visible.tabs[0] == Tab::POWER);
  visible = visible_control_tabs(parse_control_tabs("volume"), true, true, true);
  assert(visible.count == 1 && !visible.contains(Tab::CONTROLS));
  // Hidden native power remains hidden even after its capability appears.
  visible = visible_control_tabs(parse_control_tabs("volume|controls"), true, true, true);
  assert(!visible.contains(Tab::POWER));
}
