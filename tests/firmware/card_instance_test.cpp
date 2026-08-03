#include <array>
#include <cstdlib>

#include "card_instance.h"

namespace {

using namespace espcontrol::cards;

struct ResourceTrace {
  int id = 0;
  std::array<int, 12> *released = nullptr;
  std::size_t *count = nullptr;
};

void record_release(void *opaque) {
  auto *trace = static_cast<ResourceTrace *>(opaque);
  (*trace->released)[(*trace->count)++] = trace->id;
}

CardAddress main_card(uint16_t slot) {
  return {CardSurface::MAIN_GRID, 0, slot};
}

CardResourceHandle resource(ResourceTrace &trace, CardResourceKind kind,
                            uint8_t release_on) {
  return {&trace, record_release, kind, release_on};
}

bool unchanged_media_card_reuses_every_resource() {
  FixedCardInstance<6> instance;
  std::array<int, 12> released{};
  std::size_t count = 0;
  ResourceTrace control{1, &released, &count};
  ResourceTrace artwork{2, &released, &count};
  if (!instance.activate(main_card(1)) ||
      !instance.own(resource(control, CardResourceKind::MEDIA_CONTROL,
                             CHANGE_IDENTITY | CHANGE_RESOURCES |
                                 CHANGE_BINDINGS)) ||
      !instance.own(resource(artwork, CardResourceKind::MEDIA_ARTWORK,
                             CHANGE_IDENTITY | CHANGE_RESOURCES |
                                 CHANGE_BINDINGS))) {
    return false;
  }
  const auto token = instance.callback_token();
  return instance.release_for_changes(CHANGE_NONE) == 0 && count == 0 &&
         instance.resource_count() == 2 && token.valid();
}

bool media_rebind_invalidates_callbacks_before_targeted_cleanup() {
  FixedCardInstance<6> instance;
  std::array<int, 12> released{};
  std::size_t count = 0;
  ResourceTrace widget{1, &released, &count};
  ResourceTrace subscription{2, &released, &count};
  ResourceTrace artwork{3, &released, &count};
  if (!instance.activate(main_card(1)) ||
      !instance.own(resource(widget, CardResourceKind::WIDGET,
                             CHANGE_IDENTITY | CHANGE_RESOURCES)) ||
      !instance.own(resource(subscription, CardResourceKind::SUBSCRIPTION,
                             CHANGE_IDENTITY | CHANGE_RESOURCES |
                                 CHANGE_BINDINGS)) ||
      !instance.own(resource(artwork, CardResourceKind::MEDIA_ARTWORK,
                             CHANGE_IDENTITY | CHANGE_RESOURCES |
                                 CHANGE_BINDINGS))) {
    return false;
  }
  const auto stale = instance.callback_token();
  const std::size_t removed = instance.release_for_changes(CHANGE_BINDINGS);
  return removed == 2 && count == 2 && released[0] == 3 && released[1] == 2 &&
         instance.resource_count() == 1 && !stale.valid() &&
         instance.callback_token().valid();
}

bool visual_update_keeps_media_and_callback_generation() {
  FixedCardInstance<3> instance;
  std::array<int, 12> released{};
  std::size_t count = 0;
  ResourceTrace artwork{1, &released, &count};
  if (!instance.activate(main_card(1)) ||
      !instance.own(resource(artwork, CardResourceKind::MEDIA_ARTWORK,
                             CHANGE_IDENTITY | CHANGE_RESOURCES |
                                 CHANGE_BINDINGS))) {
    return false;
  }
  const auto token = instance.callback_token();
  return instance.release_for_changes(CHANGE_VISUAL | CHANGE_LAYOUT) == 0 &&
         count == 0 && token.valid();
}

bool reset_releases_in_reverse_and_stops_late_callbacks() {
  FixedCardInstance<3> instance;
  std::array<int, 12> released{};
  std::size_t count = 0;
  ResourceTrace first{1, &released, &count};
  ResourceTrace second{2, &released, &count};
  if (!instance.activate(main_card(1)) ||
      !instance.own(resource(first, CardResourceKind::MEDIA_CONTROL, 0)) ||
      !instance.own(resource(second, CardResourceKind::MEDIA_ARTWORK, 0))) {
    return false;
  }
  const auto stale = instance.callback_token();
  return instance.reset() == 2 && count == 2 && released[0] == 2 &&
         released[1] == 1 && !stale.valid() && !instance.active();
}

bool fixed_pool_reuses_slots_without_growing() {
  FixedCardInstancePool<2, 2> pool;
  bool created = false;
  auto *first = pool.acquire(main_card(1), &created);
  if (first == nullptr || !created || pool.size() != 1) return false;
  const uint32_t first_generation = first->generation();
  if (pool.acquire(main_card(1), &created) != first || created) return false;
  if (pool.acquire(main_card(2)) == nullptr ||
      pool.acquire(main_card(3)) != nullptr || pool.size() != 2) {
    return false;
  }
  if (!pool.release(main_card(1)) || pool.size() != 1) return false;
  auto *reused = pool.acquire(main_card(3));
  return reused == first && reused->generation() > first_generation &&
         pool.size() == pool.capacity();
}

bool resource_capacity_failure_retains_existing_ownership() {
  FixedCardInstance<1> instance;
  std::array<int, 12> released{};
  std::size_t count = 0;
  ResourceTrace first{1, &released, &count};
  ResourceTrace second{2, &released, &count};
  return instance.activate(main_card(1)) &&
         instance.own(resource(first, CardResourceKind::CUSTOM,
                               CHANGE_RESOURCES)) &&
         !instance.own(resource(second, CardResourceKind::CUSTOM,
                                CHANGE_RESOURCES)) &&
         instance.resource_count() == 1 && count == 0;
}

}  // namespace

int main() {
  return unchanged_media_card_reuses_every_resource() &&
                 media_rebind_invalidates_callbacks_before_targeted_cleanup() &&
                 visual_update_keeps_media_and_callback_generation() &&
                 reset_releases_in_reverse_and_stops_late_callbacks() &&
                 fixed_pool_reuses_slots_without_growing() &&
                 resource_capacity_failure_retains_existing_ownership()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
