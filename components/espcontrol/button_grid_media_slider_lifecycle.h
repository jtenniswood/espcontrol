#pragma once

// Internal lifecycle primitives for media sliders. These templates keep timer
// ownership testable without depending on LVGL's concrete object types.
namespace espcontrol::media_slider_lifecycle {

template<typename Context, typename Timer, typename Widget,
         typename DeleteTimer, typename GetUserData, typename SetUserData,
         typename RemoveParentCallback>
inline void detach(
    Context *ctx, Widget *deleting_widget, DeleteTimer delete_timer,
    GetUserData get_user_data, SetUserData set_user_data,
    RemoveParentCallback remove_parent_callback) {
  if (ctx == nullptr) return;

  if (ctx->geometry_timer != nullptr) {
    delete_timer(ctx->geometry_timer);
    ctx->geometry_timer = nullptr;
  }
  if (ctx->media_timer != nullptr) {
    delete_timer(ctx->media_timer);
    ctx->media_timer = nullptr;
  }
  ctx->media_position_refresh_remaining = 0;

  Widget *slider = deleting_widget != nullptr
    ? deleting_widget : ctx->media_slider;
  if (ctx->geometry_parent != nullptr && slider != nullptr) {
    remove_parent_callback(ctx->geometry_parent, slider);
  }
  ctx->geometry_parent = nullptr;

  if (slider != nullptr && get_user_data(slider) == ctx) {
    set_user_data(slider, nullptr);
  }
  ctx->media_slider = nullptr;
  ctx->fill = nullptr;
  ctx->media_track_bg = nullptr;
  ctx->media_value_lbl = nullptr;
  ctx->media_status_lbl = nullptr;
}

template<typename Context, typename Timer, typename GetUserData>
inline bool callback_is_current(
    Context *ctx, Timer *timer, GetUserData get_user_data) {
  return ctx != nullptr && timer != nullptr && ctx->media_timer == timer &&
         ctx->media_slider != nullptr &&
         get_user_data(ctx->media_slider) == ctx;
}

template<typename Context, typename GetUserData>
inline bool can_schedule(Context *ctx, GetUserData get_user_data) {
  return ctx != nullptr && ctx->media_position &&
         ctx->media_slider != nullptr &&
         get_user_data(ctx->media_slider) == ctx;
}

}  // namespace espcontrol::media_slider_lifecycle
