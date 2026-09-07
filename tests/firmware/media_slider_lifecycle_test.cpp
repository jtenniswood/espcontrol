#include <cstdlib>

#include "button_grid_media_slider_lifecycle.h"

namespace {

struct FakeTimer {
  int delete_count = 0;
};

struct FakeWidget {
  void *user_data = nullptr;
};

struct FakeContext {
  bool media_position = true;
  FakeWidget *fill = nullptr;
  FakeWidget *media_slider = nullptr;
  FakeTimer *media_timer = nullptr;
  unsigned char media_position_refresh_remaining = 0;
  FakeWidget *media_track_bg = nullptr;
  FakeWidget *geometry_parent = nullptr;
  FakeTimer *geometry_timer = nullptr;
  FakeWidget *media_value_lbl = nullptr;
  FakeWidget *media_status_lbl = nullptr;
};

struct LifecycleHarness {
  int removed_parent_callbacks = 0;

  void detach(FakeContext *ctx, FakeWidget *deleting_widget = nullptr) {
    espcontrol::media_slider_lifecycle::detach<
      FakeContext, FakeTimer, FakeWidget>(
        ctx, deleting_widget,
        [](FakeTimer *timer) { timer->delete_count++; },
        [](FakeWidget *widget) { return widget->user_data; },
        [](FakeWidget *widget, void *user_data) {
          widget->user_data = user_data;
        },
        [this](FakeWidget *, FakeWidget *) {
          removed_parent_callbacks++;
        });
  }
};

bool callback_is_current(FakeContext *ctx, FakeTimer *timer) {
  return espcontrol::media_slider_lifecycle::callback_is_current(
    ctx, timer, [](FakeWidget *widget) { return widget->user_data; });
}

bool can_schedule(FakeContext *ctx) {
  return espcontrol::media_slider_lifecycle::can_schedule(
    ctx, [](FakeWidget *widget) { return widget->user_data; });
}

bool test_delete_before_first_timer() {
  FakeTimer geometry_timer;
  FakeTimer media_timer;
  FakeWidget parent;
  FakeWidget slider;
  FakeWidget fill;
  FakeWidget track;
  FakeWidget value;
  FakeWidget status;
  FakeContext ctx;
  ctx.fill = &fill;
  ctx.media_slider = &slider;
  ctx.media_timer = &media_timer;
  ctx.media_position_refresh_remaining = 10;
  ctx.media_track_bg = &track;
  ctx.geometry_parent = &parent;
  ctx.geometry_timer = &geometry_timer;
  ctx.media_value_lbl = &value;
  ctx.media_status_lbl = &status;
  slider.user_data = &ctx;

  LifecycleHarness harness;
  if (!callback_is_current(&ctx, &media_timer) || !can_schedule(&ctx)) {
    return false;
  }
  harness.detach(&ctx, &slider);

  return geometry_timer.delete_count == 1 && media_timer.delete_count == 1 &&
         harness.removed_parent_callbacks == 1 &&
         slider.user_data == nullptr && ctx.media_slider == nullptr &&
         ctx.media_timer == nullptr && ctx.geometry_timer == nullptr &&
         ctx.geometry_parent == nullptr && ctx.fill == nullptr &&
         ctx.media_track_bg == nullptr && ctx.media_value_lbl == nullptr &&
         ctx.media_status_lbl == nullptr &&
         ctx.media_position_refresh_remaining == 0 &&
         !callback_is_current(&ctx, &media_timer) && !can_schedule(&ctx);
}

bool test_cleanup_is_idempotent() {
  FakeTimer geometry_timer;
  FakeTimer media_timer;
  FakeWidget parent;
  FakeWidget slider;
  FakeContext ctx;
  ctx.media_slider = &slider;
  ctx.media_timer = &media_timer;
  ctx.geometry_parent = &parent;
  ctx.geometry_timer = &geometry_timer;
  slider.user_data = &ctx;

  LifecycleHarness harness;
  harness.detach(&ctx);
  harness.detach(&ctx);
  return geometry_timer.delete_count == 1 && media_timer.delete_count == 1 &&
         harness.removed_parent_callbacks == 1;
}

bool test_replaced_or_rebound_slider() {
  FakeTimer stale_timer;
  FakeTimer current_timer;
  FakeWidget old_slider;
  FakeWidget new_slider;
  FakeContext old_ctx;
  FakeContext new_ctx;
  old_ctx.media_slider = &old_slider;
  old_ctx.media_timer = &stale_timer;
  old_slider.user_data = &old_ctx;
  new_ctx.media_slider = &new_slider;
  new_ctx.media_timer = &current_timer;
  new_slider.user_data = &new_ctx;

  LifecycleHarness harness;
  harness.detach(&old_ctx);
  if (callback_is_current(&old_ctx, &stale_timer)) return false;
  if (!callback_is_current(&new_ctx, &current_timer)) return false;

  // Rebinding data does not detach the visual context or duplicate timers.
  return can_schedule(&new_ctx) && new_ctx.media_timer == &current_timer &&
         current_timer.delete_count == 0;
}

bool test_shared_non_media_slider_cleanup() {
  FakeTimer geometry_timer;
  FakeWidget parent;
  FakeWidget slider;
  FakeContext ctx;
  ctx.media_position = false;
  ctx.media_slider = &slider;
  ctx.geometry_parent = &parent;
  ctx.geometry_timer = &geometry_timer;
  slider.user_data = &ctx;

  LifecycleHarness harness;
  harness.detach(&ctx, &slider);
  return geometry_timer.delete_count == 1 &&
         harness.removed_parent_callbacks == 1 &&
         slider.user_data == nullptr && !can_schedule(&ctx);
}

}  // namespace

int main() {
  if (!test_delete_before_first_timer()) return EXIT_FAILURE;
  if (!test_cleanup_is_idempotent()) return EXIT_FAILURE;
  if (!test_replaced_or_rebound_slider()) return EXIT_FAILURE;
  if (!test_shared_non_media_slider_cleanup()) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
