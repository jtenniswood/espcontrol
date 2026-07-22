#include <cstdlib>

#include "card_reconciler.h"

namespace {

using espcontrol::card_runtime::CardDriverId;
using espcontrol::card_runtime::CardTypeId;
using espcontrol::cards::CardAddress;
using espcontrol::cards::CardMutation;
using espcontrol::cards::CardNode;
using espcontrol::cards::CardSurface;
using espcontrol::cards::FixedCardGraph;

CardNode card(uint16_t slot, CardTypeId type = CardTypeId::SWITCH,
              CardDriverId driver = CardDriverId::TOGGLE) {
  CardNode node;
  node.address = {CardSurface::MAIN_GRID, 0, slot};
  node.type = type;
  node.driver = driver;
  node.resource_signature = 10;
  node.binding_signature = 20;
  node.visual_signature = 30;
  node.layout_signature = 40;
  return node;
}

bool identical_graphs_need_no_work() {
  FixedCardGraph<4> before;
  FixedCardGraph<4> after;
  if (!before.add(card(1)) || !after.add(card(1))) return false;
  return espcontrol::cards::reconcile_cards(before, after).empty();
}

bool removal_precedes_addition() {
  FixedCardGraph<4> before;
  FixedCardGraph<4> after;
  if (!before.add(card(1)) || !after.add(card(2))) return false;
  const auto plan = espcontrol::cards::reconcile_cards(before, after);
  return plan.valid() && plan.size() == 2 &&
         plan[0].mutation == CardMutation::REMOVE && plan[0].before != nullptr &&
         plan[0].after == nullptr && plan[1].mutation == CardMutation::ADD &&
         plan[1].before == nullptr && plan[1].after != nullptr;
}

bool replacement_has_priority() {
  FixedCardGraph<4> before;
  FixedCardGraph<4> after;
  CardNode changed = card(1, CardTypeId::MEDIA, CardDriverId::MEDIA_COVER_ART);
  changed.resource_signature = 11;
  changed.binding_signature = 21;
  changed.visual_signature = 31;
  if (!before.add(card(1)) || !after.add(changed)) return false;
  const auto plan = espcontrol::cards::reconcile_cards(before, after);
  return plan.size() == 1 && plan[0].mutation == CardMutation::REPLACE &&
         plan[0].changed(espcontrol::cards::CHANGE_IDENTITY) &&
         plan[0].changed(espcontrol::cards::CHANGE_RESOURCES) &&
         plan[0].changed(espcontrol::cards::CHANGE_BINDINGS) &&
         plan[0].changed(espcontrol::cards::CHANGE_VISUAL);
}

bool binding_change_preserves_patch_details() {
  FixedCardGraph<4> before;
  FixedCardGraph<4> after;
  CardNode changed = card(1);
  changed.binding_signature = 99;
  changed.visual_signature = 98;
  changed.layout_signature = 97;
  if (!before.add(card(1)) || !after.add(changed)) return false;
  const auto plan = espcontrol::cards::reconcile_cards(before, after);
  return plan.size() == 1 && plan[0].mutation == CardMutation::REBIND &&
         plan[0].changed(espcontrol::cards::CHANGE_BINDINGS) &&
         plan[0].changed(espcontrol::cards::CHANGE_VISUAL) &&
         plan[0].changed(espcontrol::cards::CHANGE_LAYOUT) &&
         !plan[0].changed(espcontrol::cards::CHANGE_RESOURCES);
}

bool visual_and_layout_changes_are_distinct() {
  FixedCardGraph<4> before;
  FixedCardGraph<4> visual_after;
  FixedCardGraph<4> layout_after;
  CardNode visual = card(1);
  CardNode layout = card(1);
  visual.visual_signature = 31;
  layout.layout_signature = 41;
  if (!before.add(card(1)) || !visual_after.add(visual) ||
      !layout_after.add(layout)) {
    return false;
  }
  const auto visual_plan = espcontrol::cards::reconcile_cards(before, visual_after);
  const auto layout_plan = espcontrol::cards::reconcile_cards(before, layout_after);
  return visual_plan.size() == 1 &&
         visual_plan[0].mutation == CardMutation::UPDATE_VISUAL &&
         layout_plan.size() == 1 &&
         layout_plan[0].mutation == CardMutation::UPDATE_LAYOUT;
}

bool binding_owned_visual_state_is_reconstructed() {
  using espcontrol::cards::CHANGE_BINDINGS;
  const uint8_t domains = CHANGE_BINDINGS;
  return !espcontrol::cards::requires_visual_reconstruction(
             domains, CardMutation::REBIND, false) &&
         espcontrol::cards::requires_visual_reconstruction(
             domains, CardMutation::REBIND, true);
}

bool subpage_addresses_are_unambiguous() {
  FixedCardGraph<4> graph;
  CardNode first = card(1);
  first.address = {CardSurface::SUBPAGE, 1, 1};
  CardNode second = first;
  second.address.parent_slot = 2;
  return graph.add(first) && graph.add(second) && graph.size() == 2 &&
         graph.find(first.address) != nullptr && graph.find(second.address) != nullptr;
}

bool capacity_and_duplicates_are_rejected() {
  FixedCardGraph<2> graph;
  return graph.add(card(1)) && !graph.add(card(1)) && graph.add(card(2)) &&
         !graph.add(card(3)) && graph.size() == graph.capacity();
}

bool repeated_revisions_remain_bounded() {
  FixedCardGraph<1> active;
  if (!active.add(card(1))) return false;
  for (uint64_t revision = 1; revision <= 100; ++revision) {
    FixedCardGraph<1> next;
    CardNode changed = card(1);
    changed.binding_signature = revision;
    if (!next.add(changed)) return false;
    const auto plan = espcontrol::cards::reconcile_cards(active, next);
    if (plan.size() != 1 || plan[0].mutation != CardMutation::REBIND) return false;
    active = next;
  }
  return active.size() == 1 && active.capacity() == 1;
}

bool signatures_are_stable() {
  constexpr uint64_t first = espcontrol::cards::card_signature("media");
  constexpr uint64_t second = espcontrol::cards::card_signature("media");
  constexpr uint64_t different = espcontrol::cards::card_signature("switch");
  static_assert(first == second);
  static_assert(first != different);
  return espcontrol::cards::combine_card_signatures(first, different) != first;
}

}  // namespace

int main() {
  if (!identical_graphs_need_no_work()) return EXIT_FAILURE;
  if (!removal_precedes_addition()) return EXIT_FAILURE;
  if (!replacement_has_priority()) return EXIT_FAILURE;
  if (!binding_change_preserves_patch_details()) return EXIT_FAILURE;
  if (!visual_and_layout_changes_are_distinct()) return EXIT_FAILURE;
  if (!binding_owned_visual_state_is_reconstructed()) return EXIT_FAILURE;
  if (!subpage_addresses_are_unambiguous()) return EXIT_FAILURE;
  if (!capacity_and_duplicates_are_rejected()) return EXIT_FAILURE;
  if (!repeated_revisions_remain_bounded()) return EXIT_FAILURE;
  if (!signatures_are_stable()) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
