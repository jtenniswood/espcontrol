#include <cassert>

#include "climate_target_logic.h"

using espcontrol::climate::CommandKind;
using espcontrol::climate::TargetKind;
using espcontrol::climate::TargetSelection;

int main() {
  using espcontrol::climate::command_kind;
  using espcontrol::climate::constrain_range_target;
  using espcontrol::climate::nearest_handle_is_high;
  using espcontrol::climate::nearest_target_is_high;
  using espcontrol::climate::target_kind;
  using espcontrol::climate::target_selection_for_mode;
  using espcontrol::climate::target_values_complete;

  // Reported Mitsubishi state: Cool, 65-75 F, supported_features 426.
  const TargetKind reported = target_kind(true, 426, false, true, true);
  assert(reported == TargetKind::RANGE);
  assert(target_values_complete(reported, false, true, true));
  assert(command_kind(reported, true) == CommandKind::RANGE);
  assert(target_selection_for_mode("cool") == TargetSelection::HIGH);

  // A valid advertised single target wins even if range support is also present.
  assert(target_kind(true, 427, true, true, true) == TargetKind::SINGLE);
  assert(command_kind(TargetKind::SINGLE, true) == CommandKind::SINGLE);

  // A range-only entity never falls through to a single-temperature command.
  const TargetKind partial_range = target_kind(true, 426, false, true, false);
  assert(partial_range == TargetKind::RANGE);
  assert(!target_values_complete(partial_range, false, true, false));
  assert(command_kind(partial_range, false) == CommandKind::NONE);

  // Before capability data arrives, use whichever complete attribute shape exists.
  assert(target_kind(false, 0, true, true, true) == TargetKind::SINGLE);
  assert(target_kind(false, 0, false, true, true) == TargetKind::RANGE);

  assert(target_selection_for_mode("heat") == TargetSelection::LOW);
  assert(target_selection_for_mode("dry") == TargetSelection::HIGH);
  assert(target_selection_for_mode("auto") == TargetSelection::RETAIN);
  assert(target_selection_for_mode("heat_cool") == TargetSelection::RETAIN);
  assert(target_selection_for_mode("off") == TargetSelection::RETAIN);

  assert(!nearest_target_is_high(650, 650, 750));
  assert(nearest_target_is_high(750, 650, 750));
  assert(!nearest_target_is_high(700, 650, 750));

  // A press starts with whichever visible range handle is closest, without
  // requiring the matching number label to be selected first.
  assert(!nearest_handle_is_high(100, 200, 100, 200, 300, 200));
  assert(nearest_handle_is_high(300, 200, 100, 200, 300, 200));
  assert(!nearest_handle_is_high(200, 200, 100, 200, 300, 200));
  assert(nearest_handle_is_high(290, 210, 100, 200, 300, 200));

  // Low and high remain at least one configured step apart.
  assert(constrain_range_target(760, false, 650, 750, 500, 900, 10) == 740);
  assert(constrain_range_target(640, true, 650, 750, 500, 900, 10) == 660);
  assert(constrain_range_target(705, false, 650, 705, 500, 900, 5) == 700);
  assert(constrain_range_target(700, true, 700, 750, 500, 900, 5) == 705);
  assert(constrain_range_target(700, false, 650, 705, 500, 900, 10) == 695);
  assert(constrain_range_target(705, true, 700, 750, 500, 900, 10) == 710);

  return 0;
}
