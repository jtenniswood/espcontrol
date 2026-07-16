#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

#ifdef ESP_PLATFORM
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

#include "esphome/core/version.h"
#include "artwork_controller.h"
#include "../artwork_image/image_pipeline_policy.h"
#include <cstring>

constexpr uint32_t IMAGE_CARD_STARTUP_RETRY_MS = 45000;
constexpr uint32_t IMAGE_CARD_RETRY_INTERVAL_MS = 2000;
constexpr uint32_t IMAGE_CARD_API_RETRY_INTERVAL_MS = 250;
constexpr uint32_t IMAGE_CARD_MIN_REPEAT_REFRESH_MS = 30000;
constexpr uint32_t IMAGE_CARD_MODAL_REFRESH_DELAY_MS = 1000;
constexpr uint32_t IMAGE_CARD_MODAL_REQUEST_DELAY_MS = 100;
constexpr uint32_t IMAGE_CARD_MODAL_CLEANUP_DELAY_MS = 100;
constexpr uint32_t IMAGE_CARD_MODAL_CLOSE_GUARD_MS = 350;
constexpr uint32_t IMAGE_CARD_MEDIA_ARTWORK_DEBOUNCE_MS = 300;
constexpr uint8_t IMAGE_CARD_STARTUP_DOWNLOAD_RETRIES = 10;
constexpr int IMAGE_CARD_MAX_CONTEXTS = 6;
constexpr int IMAGE_CARD_MODAL_MAX_TARGET_SIDE_PX = 800;
constexpr size_t IMAGE_CARD_MEMORY_HEADROOM_BYTES = 96 * 1024;
constexpr lv_coord_t IMAGE_CARD_COMPACT_PORTRAIT_MODAL_BACK_BUTTON_REF_PX = 58;
constexpr const char *IMAGE_CARD_LOADING_ICON = "\U000F02E9";

struct ImageCardCtx {
  lv_obj_t *widget = nullptr;
  lv_obj_t *btn = nullptr;
  lv_obj_t *loading_widget = nullptr;
  lv_obj_t *loading_label = nullptr;
  const lv_font_t *icon_font = nullptr;
  const lv_font_t *label_font = nullptr;
  esphome::artwork_image::ArtworkImage *image = nullptr;
  esphome::artwork_image::ArtworkImage *modal_image = nullptr;
  std::string entity_id;
  std::string base_url;
  std::function<std::string()> base_url_provider;
  std::string source_url;
  std::string url;
  std::string modal_url;
  std::string cached_entity_id;
  std::string modal_source_url;
  std::string access_token;
  std::function<void(espcontrol::DisplayTakeoverKind)> begin_display_takeover;
  std::function<void(espcontrol::DisplayTakeoverKind)> end_display_takeover;
  uint32_t retry_deadline_ms = 0;
  uint32_t next_picture_retry_ms = 0;
  uint32_t next_download_retry_ms = 0;
  uint32_t last_download_completed_ms = 0;
  uint32_t modal_open_started_ms = 0;
  uint32_t last_tile_request_started_ms = 0;
  uint32_t last_modal_request_started_ms = 0;
  int width_compensation_percent = 100;
  int media_artwork_width_compensation_percent = 100;
  bool active = false;
  bool callbacks_bound = false;
  bool requested_once = false;
  bool image_ready = false;
  bool download_active = false;
  bool download_queued = false;
  bool modal_fit = false;
  bool diagnostics_enabled = false;
  bool access_token_request_pending = false;
  bool media_artwork = false;
  lv_obj_t *media_overlay = nullptr;
  std::string pending_fallback_picture;
  espcontrol::artwork::SourceCandidates media_artwork_sources;
  uint8_t media_artwork_retry_mask = 0;
  lv_timer_t *media_artwork_timer = nullptr;
  lv_timer_t *modal_cleanup_timer = nullptr;
  uint8_t startup_download_errors = 0;
};

struct ImageCardModalUi {
  ImageCardCtx *active = nullptr;
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *image_widget = nullptr;
  lv_obj_t *loading_widget = nullptr;
  lv_timer_t *request_timer = nullptr;
};

struct ImageCardModalCache {
  esphome::artwork_image::ArtworkImage *image = nullptr;
  std::string entity_id;
  std::string source_url;
  bool ready = false;
};

inline ImageCardCtx *image_card_contexts() {
  static ImageCardCtx contexts[IMAGE_CARD_MAX_CONTEXTS];
  return contexts;
}

inline void image_card_schedule_source_refresh(ImageCardCtx *ctx, uint32_t delay_ms,
                                               const char *reason);
inline void image_card_request_source_url(ImageCardCtx *ctx, bool source_changed = false);

inline bool image_card_uses_background_pipeline(
    esphome::artwork_image::ArtworkImage *image, const std::string &url) {
  return image && image->can_use_p4_pipeline(url);
}

inline ImageCardCtx *&image_card_active_download_context() {
  static ImageCardCtx *ctx = nullptr;
  return ctx;
}

inline void image_card_start_next_queued_download(ImageCardCtx *finished_ctx) {
  ImageCardCtx *contexts = image_card_contexts();
  for (int i = 0; i < IMAGE_CARD_MAX_CONTEXTS; i++) {
    ImageCardCtx *next = &contexts[i];
    if (!next->active || !next->download_queued || next == finished_ctx) continue;
    next->download_queued = false;
    if (esphome::artwork_image::image_pipeline_can_start_followup_inline(
          image_card_uses_background_pipeline(next->image, next->source_url))) {
      image_card_request_source_url(next);
    } else {
      image_card_schedule_source_refresh(next, IMAGE_CARD_API_RETRY_INTERVAL_MS,
                                         "image download queue");
    }
    return;
  }
}
inline void image_card_release_download_slot(ImageCardCtx *ctx, bool start_next = true) {
  if (!ctx) return;
  ctx->download_active = false;
  ctx->download_queued = false;
  ImageCardCtx *&active = image_card_active_download_context();
  if (active == ctx) {
    active = nullptr;
    if (start_next) image_card_start_next_queued_download(ctx);
  }
}

inline void image_card_prioritize_modal_download(ImageCardCtx *ctx) {
  ImageCardCtx *active = image_card_active_download_context();
  if (active && active->image) {
    bool requeue_preempted_tile =
      esphome::artwork_image::image_pipeline_should_requeue_interrupted_tile(
        true, active->active, !active->source_url.empty());
    active->image->cancel_update();
    image_card_release_download_slot(active, false);
    if (requeue_preempted_tile) {
      active->download_queued = true;
      active->next_download_retry_ms =
          esphome::millis() + IMAGE_CARD_MODAL_REFRESH_DELAY_MS;
    }
  }
  if (ctx && ctx != active) {
    bool requeue_selected_tile =
      esphome::artwork_image::image_pipeline_should_requeue_interrupted_tile(
        ctx->download_queued, ctx->active, !ctx->source_url.empty());
    if (ctx->image) ctx->image->cancel_update();
    image_card_release_download_slot(ctx, false);
    if (requeue_selected_tile) {
      ctx->download_queued = true;
      ctx->next_download_retry_ms =
          esphome::millis() + IMAGE_CARD_MODAL_REFRESH_DELAY_MS;
    }
  }
}

inline ImageCardModalUi &image_card_modal_ui() {
  static ImageCardModalUi ui;
  return ui;
}

inline ImageCardModalCache &image_card_modal_cache() {
  static ImageCardModalCache cache;
  return cache;
}

inline void image_card_log_diagnostics(ImageCardCtx *ctx, const char *stage,
                                       lv_coord_t target_width = 0,
                                       lv_coord_t target_height = 0) {
  if (!ctx || !ctx->diagnostics_enabled) return;
  ImageCardModalUi &ui = image_card_modal_ui();
  uint32_t now = esphome::millis();
  uint32_t modal_age = ctx->modal_open_started_ms == 0 ? 0 : now - ctx->modal_open_started_ms;
  uint32_t tile_age = ctx->last_tile_request_started_ms == 0 ? 0 : now - ctx->last_tile_request_started_ms;
  uint32_t modal_request_age = ctx->last_modal_request_started_ms == 0 ? 0 : now - ctx->last_modal_request_started_ms;
#ifdef ESP_PLATFORM
  size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  size_t internal_min = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  size_t internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
  size_t psram_largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
  UBaseType_t stack_high_water = uxTaskGetStackHighWaterMark(nullptr);
  int core = xPortGetCoreID();
  ESP_LOGI("image_card_diag",
           "%s entity=%s modal=%s image_ready=%s tile_dl=%s queued=%s requested=%s target=%dx%d "
           "src_len=%u url_len=%u modal_url_len=%u modal_age=%lu tile_age=%lu modal_req_age=%lu "
           "internal_free=%u internal_min=%u internal_largest=%u psram_free=%u psram_largest=%u "
           "task_stack_free=%u core=%d",
           stage ? stage : "event", ctx->entity_id.c_str(),
           ui.active == ctx && ui.overlay ? "open" : "closed",
           ctx->image_ready ? "yes" : "no",
           ctx->download_active ? "yes" : "no",
           ctx->download_queued ? "yes" : "no",
           ctx->requested_once ? "yes" : "no",
           static_cast<int>(target_width), static_cast<int>(target_height),
           static_cast<unsigned>(ctx->source_url.size()),
           static_cast<unsigned>(ctx->url.size()),
           static_cast<unsigned>(ctx->modal_url.size()),
           static_cast<unsigned long>(modal_age),
           static_cast<unsigned long>(tile_age),
           static_cast<unsigned long>(modal_request_age),
           static_cast<unsigned>(internal_free),
           static_cast<unsigned>(internal_min),
           static_cast<unsigned>(internal_largest),
           static_cast<unsigned>(psram_free),
           static_cast<unsigned>(psram_largest),
           static_cast<unsigned>(stack_high_water),
           core);
#else
  ESP_LOGI("image_card_diag",
           "%s entity=%s modal=%s image_ready=%s tile_dl=%s queued=%s requested=%s target=%dx%d "
           "src_len=%u url_len=%u modal_url_len=%u modal_age=%lu tile_age=%lu modal_req_age=%lu",
           stage ? stage : "event", ctx->entity_id.c_str(),
           ui.active == ctx && ui.overlay ? "open" : "closed",
           ctx->image_ready ? "yes" : "no",
           ctx->download_active ? "yes" : "no",
           ctx->download_queued ? "yes" : "no",
           ctx->requested_once ? "yes" : "no",
           static_cast<int>(target_width), static_cast<int>(target_height),
           static_cast<unsigned>(ctx->source_url.size()),
           static_cast<unsigned>(ctx->url.size()),
           static_cast<unsigned>(ctx->modal_url.size()),
           static_cast<unsigned long>(modal_age),
           static_cast<unsigned long>(tile_age),
           static_cast<unsigned long>(modal_request_age));
#endif
}

inline void image_card_style_modal_back_button(lv_obj_t *btn,
                                               const ControlModalLayout &layout) {
  if (!btn) return;
  control_modal_style_translucent_chrome_button(btn);
  if (!control_modal_uses_compact_portrait_tuning(layout)) return;

  lv_coord_t size = control_modal_scaled_px(
    IMAGE_CARD_COMPACT_PORTRAIT_MODAL_BACK_BUTTON_REF_PX, layout.short_side);
  if (size <= layout.back_size) return;
  lv_obj_set_size(btn, size, size);
  lv_obj_set_style_radius(btn, size / 2, LV_PART_MAIN);
  lv_obj_align(btn, LV_ALIGN_TOP_LEFT, layout.back_inset_x, layout.back_inset_y);
}

inline bool image_card_modal_active_for(ImageCardCtx *ctx) {
  ImageCardModalUi &ui = image_card_modal_ui();
  return ctx && ui.active == ctx && ui.overlay && ui.image_widget;
}

inline lv_obj_t *image_card_loading_widget(lv_obj_t *widget);
inline bool image_card_position_widget(lv_obj_t *btn, lv_obj_t *widget,
                                       lv_coord_t *target_width,
                                       lv_coord_t *target_height);
inline void image_card_move_label_foreground(lv_obj_t *loading_widget);
inline void image_card_align_label(lv_obj_t *label, lv_obj_t *btn,
                                   lv_coord_t x_offset = 0,
                                   lv_coord_t y_offset = 0);
inline void image_card_align_icon(lv_obj_t *icon, lv_obj_t *btn);
inline bool image_card_apply_modal_geometry(
  ImageCardCtx *ctx,
  esphome::artwork_image::ArtworkImage *image,
  lv_coord_t *target_width = nullptr,
  lv_coord_t *target_height = nullptr);
inline void image_card_show_modal_image(
  ImageCardCtx *ctx,
  esphome::artwork_image::ArtworkImage *image);
inline bool image_card_queue_modal_source_request(ImageCardCtx *ctx);
inline void image_card_schedule_source_refresh(ImageCardCtx *ctx, uint32_t delay_ms,
                                               const char *reason);

inline uint32_t image_card_scale_for_size(lv_coord_t target_width, lv_coord_t target_height,
                                          int source_width, int source_height, bool fit) {
  if (target_width <= 0 || target_height <= 0 || source_width <= 0 || source_height <= 0) {
    return 256;
  }
  uint32_t scale_x = static_cast<uint32_t>(target_width) * 256u / static_cast<uint32_t>(source_width);
  uint32_t scale_y = static_cast<uint32_t>(target_height) * 256u / static_cast<uint32_t>(source_height);
  uint32_t scale = fit
    ? (scale_x < scale_y ? scale_x : scale_y)
    : (scale_x > scale_y ? scale_x : scale_y);
  return scale == 0 ? 1 : scale;
}

inline void image_card_set_widget_source(lv_obj_t *widget,
                                         esphome::artwork_image::ArtworkImage *image) {
  if (!widget || !image) return;
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  lv_image_set_src(widget, image->get_lv_image_dsc());
#else
  lv_img_set_src(widget, image->get_lv_img_dsc());
#endif
  lv_obj_clear_flag(widget, LV_OBJ_FLAG_HIDDEN);
  lv_obj_invalidate(widget);
}

inline void image_card_clear_widget_source(lv_obj_t *widget) {
  if (!widget) return;
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  lv_image_set_src(widget, nullptr);
#else
  lv_img_set_src(widget, nullptr);
#endif
  lv_obj_add_flag(widget, LV_OBJ_FLAG_HIDDEN);
  lv_obj_invalidate(widget);
}

inline lv_style_selector_t image_card_pressed_selector() {
  return static_cast<lv_style_selector_t>(LV_PART_MAIN) |
         static_cast<lv_style_selector_t>(LV_STATE_PRESSED);
}

inline void image_card_apply_corner_clip(lv_obj_t *obj, lv_coord_t radius) {
  if (!obj) return;
  lv_obj_set_style_radius(obj, radius, LV_PART_MAIN);
  lv_obj_set_style_radius(obj, radius, image_card_pressed_selector());
  lv_obj_set_style_clip_corner(obj, true, LV_PART_MAIN);
  lv_obj_set_style_clip_corner(obj, true, image_card_pressed_selector());
}

inline void image_card_apply_tile_image_align(lv_obj_t *widget) {
  if (!widget) return;
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  lv_image_set_inner_align(widget, LV_IMAGE_ALIGN_COVER);
#else
  (void) widget;
#endif
}

inline void image_card_sync_tile_corners(lv_obj_t *btn, lv_obj_t *widget) {
  if (!btn) return;
  lv_coord_t radius = lv_obj_get_style_radius(btn, LV_PART_MAIN);
  image_card_apply_corner_clip(btn, radius);
  image_card_apply_corner_clip(widget, radius);
  lv_obj_t *loading = image_card_loading_widget(widget);
  image_card_apply_corner_clip(loading, radius);
  if (loading) {
    lv_obj_set_style_clip_corner(loading, false, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(loading, false, image_card_pressed_selector());
  }
}

inline lv_obj_t *image_card_loading_widget(lv_obj_t *widget) {
  return widget ? static_cast<lv_obj_t *>(lv_obj_get_user_data(widget)) : nullptr;
}

inline lv_obj_t *image_card_loading_icon(lv_obj_t *loading_widget) {
  if (!loading_widget || lv_obj_get_child_cnt(loading_widget) < 1) return nullptr;
  return lv_obj_get_child(loading_widget, 0);
}

inline lv_obj_t *image_card_loading_label(lv_obj_t *loading_widget) {
  if (!loading_widget || lv_obj_get_child_cnt(loading_widget) < 2) return nullptr;
  return lv_obj_get_child(loading_widget, 1);
}

inline const lv_font_t *image_card_label_font_for_slot(const BtnSlot &s) {
  const lv_font_t *font = s.text_lbl
    ? lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN)
    : nullptr;
  if (!font && s.btn) font = lv_obj_get_style_text_font(s.btn, LV_PART_MAIN);
  return font;
}

inline const lv_font_t *image_card_icon_font_for_slot(const BtnSlot &s) {
  const lv_font_t *font = s.icon_lbl
    ? lv_obj_get_style_text_font(s.icon_lbl, LV_PART_MAIN)
    : nullptr;
  if (!font && s.btn) font = lv_obj_get_style_text_font(s.btn, LV_PART_MAIN);
  return font;
}

inline void image_card_apply_loading_fonts(lv_obj_t *loading_widget,
                                           const lv_font_t *icon_font,
                                           const lv_font_t *label_font) {
  if (!loading_widget) return;
  if (label_font) lv_obj_set_style_text_font(loading_widget, label_font, LV_PART_MAIN);
  lv_obj_t *icon = image_card_loading_icon(loading_widget);
  if (icon && icon_font) lv_obj_set_style_text_font(icon, icon_font, LV_PART_MAIN);
  lv_obj_t *label = image_card_loading_label(loading_widget);
  if (label && label_font) lv_obj_set_style_text_font(label, label_font, LV_PART_MAIN);
}

inline void image_card_refresh_loading_layout(lv_obj_t *loading_widget) {
  if (!loading_widget) return;
  lv_obj_set_layout(loading_widget, 0);
  lv_obj_t *btn = lv_obj_get_parent(loading_widget);
  image_card_position_widget(btn, loading_widget, nullptr, nullptr);
  lv_obj_set_style_pad_all(loading_widget, 0, LV_PART_MAIN);
  lv_coord_t pad_left = btn ? lv_obj_get_style_pad_left(btn, LV_PART_MAIN) : 0;
  lv_coord_t pad_right = btn ? lv_obj_get_style_pad_right(btn, LV_PART_MAIN) : 0;
  lv_coord_t pad_top = btn ? lv_obj_get_style_pad_top(btn, LV_PART_MAIN) : 0;
  lv_coord_t pad_bottom = btn ? lv_obj_get_style_pad_bottom(btn, LV_PART_MAIN) : 0;
  lv_coord_t width = btn ? lv_obj_get_width(btn) : lv_obj_get_width(loading_widget);
  lv_coord_t height = btn ? lv_obj_get_height(btn) : lv_obj_get_height(loading_widget);
  lv_obj_t *card_label = static_cast<lv_obj_t *>(lv_obj_get_user_data(loading_widget));
  if (card_label && !lv_obj_has_flag(card_label, LV_OBJ_FLAG_HIDDEN)) {
    const lv_font_t *font = lv_obj_get_style_text_font(card_label, LV_PART_MAIN);
    lv_coord_t reserve = font && font->line_height > 0 ? font->line_height * 2 : 40;
    reserve += pad_bottom;
    if (reserve > height / 2) reserve = height / 2;
    if (height > reserve) lv_obj_set_height(loading_widget, height - reserve);
  }
  lv_obj_t *icon = image_card_loading_icon(loading_widget);
  lv_obj_t *label = image_card_loading_label(loading_widget);
  if (icon) {
    lv_obj_set_width(icon, LV_SIZE_CONTENT);
    lv_obj_align(icon, LV_ALIGN_TOP_LEFT, pad_left, pad_top);
    lv_obj_move_foreground(icon);
  }
  if (label) {
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_pad_all(label, 0, LV_PART_MAIN);
    if (width > pad_left + pad_right) {
      lv_obj_set_width(label, width - pad_left - pad_right);
    }
    if (icon) {
      lv_obj_align_to(label, icon, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
    } else {
      lv_obj_align(label, LV_ALIGN_TOP_LEFT, pad_left, pad_top);
    }
    lv_obj_move_foreground(label);
  }
  lv_obj_update_layout(loading_widget);
}

inline void image_card_set_loading_state(lv_obj_t *loading_widget, const char *text) {
  if (!loading_widget) return;
  lv_obj_t *btn = lv_obj_get_parent(loading_widget);
  image_card_position_widget(btn, loading_widget, nullptr, nullptr);
  lv_obj_t *icon = image_card_loading_icon(loading_widget);
  if (icon) lv_label_set_text(icon, IMAGE_CARD_LOADING_ICON);
  lv_obj_t *label = image_card_loading_label(loading_widget);
  if (label) lv_label_set_text(label, espcontrol_i18n(text));
  image_card_refresh_loading_layout(loading_widget);
  lv_obj_clear_flag(loading_widget, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(loading_widget);
  image_card_move_label_foreground(loading_widget);
  lv_obj_invalidate(loading_widget);
}

inline void image_card_set_loading_state(ImageCardCtx *ctx, const char *text,
                                         bool force = false) {
  if (!ctx || !ctx->loading_widget) return;
  bool image_visible = ctx->widget && !lv_obj_has_flag(ctx->widget, LV_OBJ_FLAG_HIDDEN);
  if (image_visible && !force) return;
  image_card_set_loading_state(ctx->loading_widget, text);
}

inline void image_card_hide_loading(ImageCardCtx *ctx) {
  if (!ctx || !ctx->loading_widget) return;
  lv_obj_add_flag(ctx->loading_widget, LV_OBJ_FLAG_HIDDEN);
}

inline void image_card_hide(ImageCardCtx *ctx) {
  if (!ctx) return;
  if (ctx->widget) lv_obj_add_flag(ctx->widget, LV_OBJ_FLAG_HIDDEN);
}

inline void image_card_clear_media_artwork(ImageCardCtx *ctx) {
  if (!ctx || !ctx->media_artwork) return;
  image_card_release_download_slot(ctx);
  image_card_clear_widget_source(ctx->widget);
  if (ctx->image) ctx->image->release();
  ctx->image_ready = false;
  ctx->requested_once = false;
  ctx->source_url.clear();
  ctx->url.clear();
  ctx->pending_fallback_picture.clear();
  ctx->media_artwork_sources.clear();
  ctx->media_artwork_retry_mask = 0;
  if (ctx->media_artwork_timer) {
    lv_timer_del(ctx->media_artwork_timer);
    ctx->media_artwork_timer = nullptr;
  }
  ctx->next_picture_retry_ms = 0;
  ctx->next_download_retry_ms = 0;
  ctx->last_download_completed_ms = 0;
  image_card_hide(ctx);
  if (ctx->media_overlay) lv_obj_add_flag(ctx->media_overlay, LV_OBJ_FLAG_HIDDEN);
}

inline void image_card_layout_modal_loading(ImageCardCtx *ctx) {
  ImageCardModalUi &ui = image_card_modal_ui();
  if (!ctx || ui.active != ctx || !ui.panel || !ui.loading_widget) return;
  lv_obj_update_layout(ui.panel);
  lv_coord_t width = lv_obj_get_width(ui.panel);
  lv_coord_t height = lv_obj_get_height(ui.panel);
  if (width <= 0 || height <= 0) return;
  lv_obj_set_pos(ui.loading_widget, 0, 0);
  lv_obj_set_size(ui.loading_widget, width, height);
  lv_obj_set_style_radius(
    ui.loading_widget, lv_obj_get_style_radius(ui.panel, LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_clip_corner(ui.loading_widget, true, LV_PART_MAIN);
  if (lv_obj_get_child_cnt(ui.loading_widget) < 2) return;
  lv_obj_t *icon = lv_obj_get_child(ui.loading_widget, 0);
  lv_obj_t *label = lv_obj_get_child(ui.loading_widget, 1);
  lv_obj_set_width(label, width);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_align(icon, LV_ALIGN_CENTER, 0, -18);
  lv_obj_align_to(label, icon, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
}

inline void image_card_show_modal_loading(ImageCardCtx *ctx, const char *text) {
  ImageCardModalUi &ui = image_card_modal_ui();
  if (!ctx || ui.active != ctx || !ui.loading_widget) return;
  image_card_layout_modal_loading(ctx);
  if (lv_obj_get_child_cnt(ui.loading_widget) >= 2) {
    lv_obj_t *icon = lv_obj_get_child(ui.loading_widget, 0);
    lv_obj_t *label = lv_obj_get_child(ui.loading_widget, 1);
    lv_label_set_text(icon, IMAGE_CARD_LOADING_ICON);
    lv_label_set_text(label, espcontrol_i18n(text));
  }
  lv_obj_clear_flag(ui.loading_widget, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(ui.loading_widget);
  if (ui.back_btn) lv_obj_move_foreground(ui.back_btn);
  lv_obj_invalidate(ui.loading_widget);
}

inline void image_card_hide_modal_loading(ImageCardCtx *ctx) {
  ImageCardModalUi &ui = image_card_modal_ui();
  if (!ctx || ui.active != ctx || !ui.loading_widget) return;
  lv_obj_add_flag(ui.loading_widget, LV_OBJ_FLAG_HIDDEN);
  if (ui.back_btn) lv_obj_move_foreground(ui.back_btn);
}

inline bool image_card_startup_retry_active(ImageCardCtx *ctx,
                                            uint32_t now = esphome::millis()) {
  return ctx && ctx->retry_deadline_ms != 0 &&
         (int32_t)(now - ctx->retry_deadline_ms) < 0;
}

inline size_t image_card_estimated_buffer_bytes(int width, int height) {
  if (width <= 0 || height <= 0) return 0;
  return static_cast<size_t>(width) * static_cast<size_t>(height) * 2u;
}

inline size_t image_card_estimated_pipeline_bytes(int width, int height) {
  size_t frame_bytes = image_card_estimated_buffer_bytes(width, height);
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  // Active image, replacement image, P4 JPEG output/scaling workspace and a
  // conservative compressed-transfer allowance can coexist during refresh.
  return frame_bytes * 3u + 256u * 1024u;
#else
  return frame_bytes * 2u + 128u * 1024u;
#endif
}

inline bool image_card_memory_available(ImageCardCtx *ctx, const char *stage,
                                        int width, int height) {
#ifdef ESP_PLATFORM
  size_t image_bytes = image_card_estimated_buffer_bytes(width, height);
  size_t pipeline_bytes = image_card_estimated_pipeline_bytes(width, height);
  size_t needed_free = pipeline_bytes + IMAGE_CARD_MEMORY_HEADROOM_BYTES;
  size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  size_t internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  size_t external_free = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
  size_t external_largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
  size_t heap_free = internal_free + external_free;
  size_t heap_largest = std::max(internal_largest, external_largest);
  if (image_bytes > 0 && (heap_free < needed_free || heap_largest < image_bytes)) {
    ESP_LOGW("image_card",
             "Skipping %s image refresh for %s: need=%u largest=%u free=%u internal=%u psram=%u target=%dx%d",
             stage ? stage : "camera", ctx ? ctx->entity_id.c_str() : "(unknown)",
             (unsigned) needed_free, (unsigned) heap_largest, (unsigned) heap_free,
             (unsigned) internal_free, (unsigned) external_free, width, height);
    return false;
  }
  image_card_log_diagnostics(ctx, stage ? stage : "memory-ok", width, height);
#else
  (void) ctx;
  (void) stage;
  (void) width;
  (void) height;
#endif
  return true;
}

inline void image_card_apply_downloaded(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !ctx->widget || !ctx->image) return;
  if (ctx->image->get_url() != ctx->url) {
    image_card_release_download_slot(ctx);
    return;
  }
  ctx->image_ready = true;
  image_card_release_download_slot(ctx);
  ctx->last_download_completed_ms = esphome::millis();
  ctx->startup_download_errors = 0;
  ctx->next_download_retry_ms = 0;
  if (ctx->diagnostics_enabled && ctx->last_tile_request_started_ms != 0) {
    ESP_LOGI("image_card_diag", "Tile image applied for %s after %lu ms",
             ctx->entity_id.c_str(),
             static_cast<unsigned long>(esphome::millis() - ctx->last_tile_request_started_ms));
  }
  image_card_log_diagnostics(ctx, "tile-download-applied");
  image_card_hide_loading(ctx);
  image_card_set_widget_source(ctx->widget, ctx->image);
  lv_obj_clear_flag(ctx->widget, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_background(ctx->widget);
  if (ctx->media_overlay) {
    lv_obj_clear_flag(ctx->media_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ctx->media_overlay);
  }
  lv_obj_invalidate(ctx->widget);
  if (ctx->btn) lv_obj_invalidate(ctx->btn);
  notify_dashboard_content_changed();
}

inline void image_card_handle_download_error(ImageCardCtx *ctx) {
  if (!ctx) return;
  image_card_release_download_slot(ctx);
  ESP_LOGW("image_card", "Image download failed for %s", ctx->entity_id.c_str());
  image_card_log_diagnostics(ctx, "tile-download-error");
  uint32_t now = esphome::millis();
  if (!ctx->image_ready && image_card_startup_retry_active(ctx, now) &&
      ctx->startup_download_errors < IMAGE_CARD_STARTUP_DOWNLOAD_RETRIES) {
    ctx->startup_download_errors++;
    ctx->next_download_retry_ms = now + IMAGE_CARD_RETRY_INTERVAL_MS;
    image_card_hide(ctx);
    image_card_set_loading_state(ctx, "Loading", true);
    return;
  }
  if (ctx->image_ready) {
    image_card_hide_loading(ctx);
    if (!ctx->source_url.empty()) {
      ctx->next_download_retry_ms = now + IMAGE_CARD_RETRY_INTERVAL_MS;
    }
    return;
  }
  if (image_card_modal_active_for(ctx)) {
    return;
  } else {
    image_card_hide(ctx);
    image_card_set_loading_state(ctx, "Unavailable", true);
  }
}

inline bool image_card_has_separate_modal_image(ImageCardCtx *ctx) {
  return ctx && ctx->modal_image && ctx->modal_image != ctx->image;
}

inline void image_card_cancel_stale_modal_download(ImageCardCtx *ctx) {
  if (!image_card_has_separate_modal_image(ctx)) return;
  ImageCardCtx *contexts = image_card_contexts();
  for (int i = 0; i < IMAGE_CARD_MAX_CONTEXTS; i++) {
    ImageCardCtx *previous = &contexts[i];
    if (!esphome::artwork_image::image_pipeline_should_preempt_stale_modal(
          previous != ctx, previous->active, previous->modal_cleanup_timer != nullptr,
          previous->modal_image == ctx->modal_image)) {
      continue;
    }
    ctx->modal_image->cancel_update();
    image_card_log_diagnostics(ctx, "stale-modal-download-cancelled");
    return;
  }
}

inline bool image_card_modal_has_preview(ImageCardCtx *ctx);

inline void image_card_show_modal_download_failure(ImageCardCtx *ctx) {
  if (image_card_modal_has_preview(ctx)) {
    image_card_hide_modal_loading(ctx);
  } else {
    image_card_show_modal_loading(ctx, "Unavailable");
  }
}

inline void image_card_apply_modal_downloaded(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !image_card_has_separate_modal_image(ctx)) return;
  if (!image_card_modal_active_for(ctx)) return;
  if (ctx->modal_image->get_url() != ctx->modal_url) return;
  ImageCardModalUi &ui = image_card_modal_ui();
  if (!image_card_apply_modal_geometry(ctx, ctx->modal_image)) return;
  ImageCardModalCache &cache = image_card_modal_cache();
  cache.image = ctx->modal_image;
  cache.entity_id = ctx->entity_id;
  cache.source_url = ctx->modal_source_url;
  cache.ready = true;
  if (ctx->diagnostics_enabled && ctx->last_modal_request_started_ms != 0) {
    ESP_LOGI("image_card_diag", "Modal image applied for %s after %lu ms",
             ctx->entity_id.c_str(),
             static_cast<unsigned long>(esphome::millis() - ctx->last_modal_request_started_ms));
  }
  image_card_log_diagnostics(ctx, "modal-download-applied");
  image_card_set_widget_source(ui.image_widget, ctx->modal_image);
  lv_obj_move_background(ui.image_widget);
  image_card_hide_modal_loading(ctx);
  if (ui.back_btn) lv_obj_move_foreground(ui.back_btn);
  if (ui.panel) lv_obj_invalidate(ui.panel);
  notify_dashboard_content_changed();
}

inline void image_card_handle_modal_download_error(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !image_card_has_separate_modal_image(ctx)) return;
  ESP_LOGW("image_card", "Modal image download failed for %s", ctx->entity_id.c_str());
  image_card_log_diagnostics(ctx, "modal-download-error");
  if (image_card_modal_active_for(ctx)) {
    ImageCardModalUi &ui = image_card_modal_ui();
    image_card_show_modal_download_failure(ctx);
    if (ui.back_btn) lv_obj_move_foreground(ui.back_btn);
  }
}

inline void image_card_bind_callbacks(ImageCardCtx *ctx) {
  if (!ctx || !ctx->image || ctx->callbacks_bound) return;
  ctx->callbacks_bound = true;
  ctx->image->add_on_finished_callback([ctx](bool) {
    image_card_apply_downloaded(ctx);
  });
  ctx->image->add_on_error_callback([ctx]() {
    image_card_handle_download_error(ctx);
  });
}

inline void image_card_bind_modal_callbacks(
    esphome::artwork_image::ArtworkImage *modal_image) {
  static esphome::artwork_image::ArtworkImage *bound_image = nullptr;
  if (!modal_image || bound_image == modal_image) return;
  bound_image = modal_image;
  modal_image->add_on_finished_callback([modal_image](bool) {
    ImageCardCtx *ctx = image_card_modal_ui().active;
    if (ctx && ctx->modal_image == modal_image) image_card_apply_modal_downloaded(ctx);
  });
  modal_image->add_on_error_callback([modal_image]() {
    ImageCardCtx *ctx = image_card_modal_ui().active;
    if (ctx && ctx->modal_image == modal_image) image_card_handle_modal_download_error(ctx);
  });
}

inline void image_card_hide_modal();

inline void reset_image_card_pool(const GridConfig &cfg) {
  if (control_modal_active().kind == ControlModalKind::IMAGE_CARD) {
    control_modal_close_active();
  }
  ImageCardCtx *contexts = image_card_contexts();
  int count = cfg.image_card_image_count;
  if (count > IMAGE_CARD_MAX_CONTEXTS) count = IMAGE_CARD_MAX_CONTEXTS;
  if (count < 0) count = 0;
  if (cfg.image_card_modal_image) cfg.image_card_modal_image->cancel_update();
  image_card_bind_modal_callbacks(cfg.image_card_modal_image);
  for (int i = 0; i < IMAGE_CARD_MAX_CONTEXTS; i++) {
    esphome::artwork_image::ArtworkImage *next_image =
        (i < count && cfg.image_card_images) ? cfg.image_card_images[i] : nullptr;
    if (contexts[i].image) contexts[i].image->cancel_update();
    if (next_image && next_image == contexts[i].image && contexts[i].image_ready &&
        !contexts[i].entity_id.empty()) {
      contexts[i].cached_entity_id = contexts[i].entity_id;
    } else {
      contexts[i].cached_entity_id.clear();
      contexts[i].source_url.clear();
      contexts[i].url.clear();
      contexts[i].modal_url.clear();
      contexts[i].modal_source_url.clear();
      contexts[i].image_ready = false;
      if (contexts[i].image) contexts[i].image->release();
    }
    image_card_clear_widget_source(contexts[i].widget);
    contexts[i].active = false;
    contexts[i].widget = nullptr;
    contexts[i].btn = nullptr;
    contexts[i].loading_widget = nullptr;
    contexts[i].loading_label = nullptr;
    contexts[i].icon_font = nullptr;
    contexts[i].label_font = nullptr;
    contexts[i].entity_id.clear();
    contexts[i].base_url.clear();
    contexts[i].base_url_provider = nullptr;
    contexts[i].access_token.clear();
    contexts[i].begin_display_takeover = nullptr;
    contexts[i].end_display_takeover = nullptr;
    contexts[i].retry_deadline_ms = 0;
    contexts[i].next_picture_retry_ms = 0;
    contexts[i].next_download_retry_ms = 0;
    contexts[i].modal_open_started_ms = 0;
    contexts[i].last_tile_request_started_ms = 0;
    contexts[i].last_modal_request_started_ms = 0;
    contexts[i].width_compensation_percent = 100;
    contexts[i].media_artwork_width_compensation_percent = 100;
    if (contexts[i].modal_cleanup_timer) {
      lv_timer_del(contexts[i].modal_cleanup_timer);
      contexts[i].modal_cleanup_timer = nullptr;
    }
    image_card_release_download_slot(&contexts[i], false);
    contexts[i].modal_fit = false;
    contexts[i].diagnostics_enabled = false;
    contexts[i].access_token_request_pending = false;
    contexts[i].media_artwork = false;
    contexts[i].media_overlay = nullptr;
    contexts[i].pending_fallback_picture.clear();
    contexts[i].media_artwork_sources.clear();
    contexts[i].media_artwork_retry_mask = 0;
    if (contexts[i].media_artwork_timer) {
      lv_timer_del(contexts[i].media_artwork_timer);
      contexts[i].media_artwork_timer = nullptr;
    }
    contexts[i].startup_download_errors = 0;
    contexts[i].image = next_image;
    contexts[i].modal_image = cfg.image_card_modal_image;
  }
}

inline ImageCardCtx *acquire_image_card_context(const GridConfig &cfg,
                                                const std::string &entity_id) {
  ImageCardCtx *contexts = image_card_contexts();
  int count = cfg.image_card_image_count;
  if (count > IMAGE_CARD_MAX_CONTEXTS) count = IMAGE_CARD_MAX_CONTEXTS;
  ImageCardCtx *selected = nullptr;
  for (int i = 0; i < count; i++) {
    if (!contexts[i].active && contexts[i].image && contexts[i].image_ready &&
        contexts[i].cached_entity_id == entity_id) {
      selected = &contexts[i];
      break;
    }
  }
  if (!selected) {
    for (int i = 0; i < count; i++) {
      if (!contexts[i].active && contexts[i].image) {
        selected = &contexts[i];
        break;
      }
    }
  }
  if (!selected) return nullptr;
  if (selected->cached_entity_id != entity_id) {
    selected->image->release();
    selected->source_url.clear();
    selected->url.clear();
    selected->modal_url.clear();
    selected->modal_source_url.clear();
    selected->requested_once = false;
    selected->image_ready = false;
    selected->last_download_completed_ms = 0;
  }
  selected->cached_entity_id = entity_id;
  selected->active = true;
  selected->modal_image = cfg.image_card_modal_image;
  image_card_bind_callbacks(selected);
  image_card_bind_modal_callbacks(selected->modal_image);
  return selected;
}

inline bool image_card_position_widget(lv_obj_t *btn, lv_obj_t *widget,
                                       lv_coord_t *target_width = nullptr,
                                       lv_coord_t *target_height = nullptr) {
  if (!btn || !widget) return false;
  lv_obj_update_layout(btn);
  lv_coord_t width = lv_obj_get_width(btn);
  lv_coord_t height = lv_obj_get_height(btn);
  if (width <= 0 || height <= 0) return false;
  lv_coord_t pad_left = lv_obj_get_style_pad_left(btn, LV_PART_MAIN);
  lv_coord_t pad_top = lv_obj_get_style_pad_top(btn, LV_PART_MAIN);
  lv_obj_set_pos(widget, -pad_left, -pad_top);
  lv_obj_set_size(widget, width, height);
  image_card_sync_tile_corners(btn, widget);
  if (target_width) *target_width = width;
  if (target_height) *target_height = height;
  return true;
}

inline void image_card_apply_widget_geometry(lv_obj_t *btn, lv_obj_t *widget,
                                             esphome::artwork_image::ArtworkImage *image,
                                             lv_coord_t target_width_override = 0) {
  if (!image) return;
  lv_coord_t width = 0;
  lv_coord_t height = 0;
  if (!image_card_position_widget(btn, widget, &width, &height)) return;
  lv_coord_t target_width = target_width_override > 0 ? target_width_override : width;
  image_card_apply_tile_image_align(widget);
  lv_obj_t *loading = image_card_loading_widget(widget);
  image_card_position_widget(btn, loading);
  image_card_refresh_loading_layout(loading);
  image->set_target_size(target_width, height);
  image->set_resize_mode(esphome::artwork_image::ImageResizeMode::COVER);
}

inline lv_coord_t image_card_media_artwork_target_width(ImageCardCtx *ctx, lv_coord_t width) {
  if (!ctx || !ctx->media_artwork || width <= 0) return width;
  int percent = normalize_width_compensation_percent(ctx->media_artwork_width_compensation_percent);
  return std::max<lv_coord_t>(1, static_cast<lv_coord_t>(
    (static_cast<int64_t>(width) * percent + 50) / 100));
}

inline void image_card_apply_context_widget_geometry(ImageCardCtx *ctx) {
  if (!ctx || !ctx->image) return;
  lv_coord_t width = 0;
  lv_coord_t height = 0;
  if (!image_card_position_widget(ctx->btn, ctx->widget, &width, &height)) return;
  image_card_apply_widget_geometry(
    ctx->btn, ctx->widget, ctx->image, image_card_media_artwork_target_width(ctx, width));
}

inline void image_card_reset_resized_tile(ImageCardCtx *ctx) {
  if (!ctx || !ctx->image) return;
  image_card_clear_widget_source(ctx->widget);
  ctx->image->release();
  ctx->url.clear();
  ctx->requested_once = false;
  ctx->image_ready = false;
  image_card_set_loading_state(ctx, "Loading", true);
}

inline void image_card_refresh_tile_geometry(ImageCardCtx *ctx) {
  if (!ctx || !ctx->image) return;
  int previous_width = ctx->image->get_fixed_width();
  int previous_height = ctx->image->get_fixed_height();
  image_card_apply_context_widget_geometry(ctx);
  int current_width = ctx->image->get_fixed_width();
  int current_height = ctx->image->get_fixed_height();
  if (current_width <= 0 || current_height <= 0 ||
      (current_width == previous_width && current_height == previous_height) ||
      ctx->source_url.empty()) {
    return;
  }
  if (image_card_modal_active_for(ctx)) {
    image_card_schedule_source_refresh(ctx, IMAGE_CARD_MODAL_REFRESH_DELAY_MS, "resized tile");
  } else {
    image_card_reset_resized_tile(ctx);
    image_card_schedule_source_refresh(ctx, 1, "resized tile");
  }
}

inline bool image_card_apply_modal_geometry(ImageCardCtx *ctx,
                                            esphome::artwork_image::ArtworkImage *image,
                                            lv_coord_t *target_width,
                                            lv_coord_t *target_height) {
  ImageCardModalUi &ui = image_card_modal_ui();
  if (!ctx || ui.active != ctx || !ui.panel || !ui.image_widget || !image) return false;
  lv_obj_update_layout(ui.panel);
  lv_coord_t width = lv_obj_get_width(ui.panel);
  lv_coord_t height = lv_obj_get_height(ui.panel);
  if (width <= 0 || height <= 0) return false;
  lv_obj_set_style_clip_corner(ui.panel, true, LV_PART_MAIN);
  lv_obj_set_pos(ui.image_widget, 0, 0);
  lv_obj_set_size(ui.image_widget, width, height);
  lv_obj_set_style_radius(
    ui.image_widget, lv_obj_get_style_radius(ui.panel, LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_clip_corner(ui.image_widget, true, LV_PART_MAIN);
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  lv_image_set_inner_align(
    ui.image_widget, ctx->modal_fit ? LV_IMAGE_ALIGN_CONTAIN : LV_IMAGE_ALIGN_COVER);
#else
  int source_width = image->get_fixed_width();
  int source_height = image->get_fixed_height();
  uint32_t zoom = image_card_scale_for_size(width, height, source_width, source_height, ctx->modal_fit);
  lv_coord_t scaled_width = static_cast<lv_coord_t>(static_cast<int64_t>(source_width) * zoom / 256);
  lv_coord_t scaled_height = static_cast<lv_coord_t>(static_cast<int64_t>(source_height) * zoom / 256);
  lv_img_set_pivot(ui.image_widget, 0, 0);
  lv_img_set_zoom(ui.image_widget, zoom);
  lv_obj_set_pos(ui.image_widget, (width - scaled_width) / 2, (height - scaled_height) / 2);
  lv_obj_set_size(ui.image_widget, source_width, source_height);
#endif
  image_card_layout_modal_loading(ctx);
  if (target_width) *target_width = width;
  if (target_height) *target_height = height;
  return true;
}

inline bool image_card_modal_has_tile_fallback(ImageCardCtx *ctx) {
  return ctx && ctx->image && ctx->image_ready;
}

inline bool image_card_modal_cache_matches(ImageCardCtx *ctx) {
  if (!ctx) return false;
  ImageCardModalCache &cache = image_card_modal_cache();
  return esphome::artwork_image::image_pipeline_modal_cache_matches(
      cache.ready, cache.image == ctx->modal_image,
      cache.entity_id == ctx->entity_id, cache.source_url == ctx->source_url);
}

inline bool image_card_modal_has_preview(ImageCardCtx *ctx) {
  return image_card_modal_cache_matches(ctx) || image_card_modal_has_tile_fallback(ctx);
}

inline void image_card_show_modal_image(ImageCardCtx *ctx,
                                        esphome::artwork_image::ArtworkImage *image) {
  ImageCardModalUi &ui = image_card_modal_ui();
  if (!ctx || !image_card_modal_active_for(ctx) || !image) return;
  if (!image_card_apply_modal_geometry(ctx, image)) return;
  image_card_set_widget_source(ui.image_widget, image);
  lv_obj_move_background(ui.image_widget);
  if (ui.loading_widget && !lv_obj_has_flag(ui.loading_widget, LV_OBJ_FLAG_HIDDEN)) {
    lv_obj_move_foreground(ui.loading_widget);
  }
  if (ui.back_btn) lv_obj_move_foreground(ui.back_btn);
  if (ui.panel) lv_obj_invalidate(ui.panel);
}

inline bool image_card_modal_refresh_supported() {
  return true;
}

inline void image_card_limit_target_size(lv_coord_t source_width, lv_coord_t source_height,
                                         int *target_width, int *target_height) {
  int width = source_width > 0 ? static_cast<int>(source_width) : 1;
  int height = source_height > 0 ? static_cast<int>(source_height) : 1;
  int long_side = width > height ? width : height;
  if (long_side > IMAGE_CARD_MODAL_MAX_TARGET_SIDE_PX) {
    width = std::max(1, width * IMAGE_CARD_MODAL_MAX_TARGET_SIDE_PX / long_side);
    height = std::max(1, height * IMAGE_CARD_MODAL_MAX_TARGET_SIDE_PX / long_side);
  }
  if (target_width) *target_width = width;
  if (target_height) *target_height = height;
}

inline void image_card_tile_request_size(lv_coord_t target_width, lv_coord_t target_height,
                                         int *request_width, int *request_height) {
  image_card_limit_target_size(target_width, target_height, request_width, request_height);
}

inline void image_card_apply_active_geometry(ImageCardCtx *ctx) {
  if (!ctx || !ctx->image) return;
  if (image_card_modal_active_for(ctx) && image_card_apply_modal_geometry(ctx, ctx->image)) return;
  image_card_apply_context_widget_geometry(ctx);
}

inline void setup_image_card(BtnSlot &s) {
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  clear_push_button_transition(s.btn);
  if (s.icon_lbl) lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  if (s.sensor_container) lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  if (s.text_lbl) lv_obj_add_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);
  if (s.subpage_lbl) lv_obj_add_flag(s.subpage_lbl, LV_OBJ_FLAG_HIDDEN);
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  lv_obj_t *img = lv_image_create(s.btn);
#else
  lv_obj_t *img = lv_img_create(s.btn);
#endif
  lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(img, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(img, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(img, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(img, LV_OPA_TRANSP, LV_PART_MAIN);
  image_card_apply_tile_image_align(img);

  lv_obj_t *loading = lv_obj_create(s.btn);
  lv_obj_set_size(loading, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(loading, lv_color_hex(TERTIARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(loading, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(loading, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(loading, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(loading, 0, LV_PART_MAIN);
  lv_obj_set_layout(loading, 0);
  lv_obj_clear_flag(loading, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(loading, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *loading_icon = lv_label_create(loading);
  const lv_font_t *loading_icon_font = image_card_icon_font_for_slot(s);
  const lv_font_t *loading_label_font = image_card_label_font_for_slot(s);
  image_card_apply_loading_fonts(loading, loading_icon_font, loading_label_font);
  lv_obj_set_style_text_color(loading_icon, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_opa(loading_icon, LV_OPA_COVER, LV_PART_MAIN);
  lv_label_set_text(loading_icon, IMAGE_CARD_LOADING_ICON);

  lv_obj_t *loading_label = lv_label_create(loading);
  image_card_apply_loading_fonts(loading, loading_icon_font, loading_label_font);
  lv_obj_set_style_text_color(loading_label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_opa(loading_label, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_text_align(loading_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  lv_label_set_text(loading_label, espcontrol_i18n("Loading"));

  lv_obj_set_user_data(img, loading);
  lv_obj_set_user_data(loading, s.text_lbl);
  lv_obj_set_user_data(s.sensor_container, img);
  image_card_sync_tile_corners(s.btn, img);
}

inline lv_obj_t *image_card_label_shadow(lv_obj_t *label, lv_obj_t *btn) {
  if (!label || !btn) return nullptr;
  int32_t count = static_cast<int32_t>(lv_obj_get_child_cnt(btn));
  for (int32_t i = 0; i < count; i++) {
    lv_obj_t *child = lv_obj_get_child(btn, i);
    if (child && child != label && lv_obj_check_type(child, &lv_label_class) &&
        lv_obj_get_user_data(child) == label) {
      return child;
    }
  }
  return nullptr;
}

inline void image_card_delete_label_shadow(lv_obj_t *label, lv_obj_t *btn) {
  lv_obj_t *shadow = image_card_label_shadow(label, btn);
  if (shadow) lv_obj_del(shadow);
}

inline void image_card_parent_offset_from_button(lv_obj_t *obj, lv_obj_t *btn,
                                                 lv_coord_t &x, lv_coord_t &y,
                                                 lv_coord_t &height) {
  x = 0;
  y = 0;
  height = btn ? lv_obj_get_height(btn) : 0;
  if (!obj || !btn) return;
  lv_obj_t *parent = lv_obj_get_parent(obj);
  while (parent && parent != btn) {
    x += lv_obj_get_x(parent);
    y += lv_obj_get_y(parent);
    height = lv_obj_get_height(parent);
    parent = lv_obj_get_parent(parent);
  }
}

inline void image_card_align_label(lv_obj_t *label, lv_obj_t *btn,
                                   lv_coord_t x_offset,
                                   lv_coord_t y_offset) {
  if (!label || !btn) return;
  lv_obj_update_layout(btn);
  lv_coord_t width = lv_obj_get_width(btn);
  lv_coord_t height = lv_obj_get_height(btn);
  lv_coord_t pad_left = lv_obj_get_style_pad_left(btn, LV_PART_MAIN);
  lv_coord_t pad_right = lv_obj_get_style_pad_right(btn, LV_PART_MAIN);
  lv_coord_t pad_top = lv_obj_get_style_pad_top(btn, LV_PART_MAIN);
  lv_coord_t pad_bottom = lv_obj_get_style_pad_bottom(btn, LV_PART_MAIN);
  if (width > 0) lv_obj_set_width(label, width);
  lv_obj_set_style_pad_left(label, pad_left, LV_PART_MAIN);
  lv_obj_set_style_pad_right(label, pad_right, LV_PART_MAIN);
  lv_obj_set_style_pad_top(label, pad_top, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(label, pad_bottom, LV_PART_MAIN);
  lv_coord_t parent_x = 0;
  lv_coord_t parent_y = 0;
  lv_coord_t parent_height = height;
  image_card_parent_offset_from_button(label, btn, parent_x, parent_y, parent_height);
  lv_obj_align(
    label, LV_ALIGN_BOTTOM_LEFT,
    -pad_left - parent_x + x_offset,
    pad_bottom - parent_y + (height - parent_height) + y_offset);
  lv_obj_move_foreground(label);
}

inline void image_card_align_label_stack(lv_obj_t *label, lv_obj_t *btn) {
  if (!label || !btn) return;
  lv_obj_t *shadow = image_card_label_shadow(label, btn);
  if (shadow) image_card_align_label(shadow, btn, 1, 1);
  image_card_align_label(label, btn);
}

inline void image_card_move_label_foreground(lv_obj_t *loading_widget) {
  if (!loading_widget) return;
  lv_obj_t *label = static_cast<lv_obj_t *>(lv_obj_get_user_data(loading_widget));
  lv_obj_t *btn = lv_obj_get_parent(loading_widget);
  if (!label || !btn || lv_obj_has_flag(label, LV_OBJ_FLAG_HIDDEN)) return;
  lv_obj_t *shadow = image_card_label_shadow(label, btn);
  if (shadow) lv_obj_move_foreground(shadow);
  lv_obj_move_foreground(label);
  image_card_align_label_stack(label, btn);
}

inline void image_card_align_icon(lv_obj_t *icon, lv_obj_t *btn) {
  if (!icon || !btn) return;
  lv_coord_t parent_x = 0;
  lv_coord_t parent_y = 0;
  lv_coord_t parent_height = 0;
  image_card_parent_offset_from_button(icon, btn, parent_x, parent_y, parent_height);
  lv_obj_align(icon, LV_ALIGN_TOP_LEFT, -parent_x, -parent_y);
  lv_obj_move_foreground(icon);
}

inline bool image_card_entity_supported(const std::string &entity_id) {
  return entity_id.rfind("camera.", 0) == 0 || entity_id.rfind("image.", 0) == 0;
}

inline void image_card_set_label_text(lv_obj_t *label, lv_obj_t *btn,
                                      const char *text) {
  if (!label) return;
  const char *safe_text = text ? text : "";
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_label_set_text(label, safe_text);
  lv_obj_t *shadow = image_card_label_shadow(label, btn);
  if (shadow) {
    lv_label_set_long_mode(shadow, LV_LABEL_LONG_WRAP);
    lv_label_set_text(shadow, safe_text);
  }
  image_card_align_label_stack(label, btn);
}

inline void subscribe_image_card_label(lv_obj_t *label, lv_obj_t *btn,
                                       const std::string &entity_id) {
  const uint32_t generation = ha_subscription_generation();
  ha_subscribe_attribute(
    entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([label, btn, generation](esphome::StringRef name) {
      if (generation != ha_subscription_generation()) return;
      image_card_set_label_text(
        label, btn, string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN).c_str());
    })
  );
}

inline void image_card_configure_label(BtnSlot &s, const ParsedCfg &p) {
  if (!s.text_lbl) return;
  if (!image_card_label_enabled(p)) {
    image_card_delete_label_shadow(s.text_lbl, s.btn);
    lv_obj_add_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_opa(s.text_lbl, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_radius(s.text_lbl, 0, LV_PART_MAIN);
  lv_label_set_long_mode(s.text_lbl, LV_LABEL_LONG_WRAP);

  lv_obj_t *shadow = image_card_label_shadow(s.text_lbl, s.btn);
  if (!shadow) {
    shadow = lv_label_create(s.btn);
    lv_obj_set_user_data(shadow, s.text_lbl);
  }
  const lv_font_t *font = lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(shadow, font, LV_PART_MAIN);
  lv_obj_set_style_text_color(shadow, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_text_opa(shadow, LV_OPA_50, LV_PART_MAIN);
  lv_obj_set_style_text_align(shadow, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(shadow, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_radius(shadow, 0, LV_PART_MAIN);
  lv_label_set_long_mode(shadow, LV_LABEL_LONG_WRAP);
  image_card_set_label_text(s.text_lbl, s.btn, (p.label.empty() ? p.entity : p.label).c_str());
  if (p.label.empty() && !p.entity.empty()) {
    subscribe_image_card_label(s.text_lbl, s.btn, p.entity);
  }
}

inline void image_card_configure_icon(BtnSlot &s, const ParsedCfg &p) {
  if (!s.icon_lbl) return;
  if (!image_card_icon_enabled(p)) {
    lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_label_set_text(s.icon_lbl, find_icon(
    p.icon.empty() || p.icon == "Auto" ? "Camera" : p.icon.c_str()));
  lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  image_card_align_icon(s.icon_lbl, s.btn);
}

inline std::string image_card_join_url(const std::string &base, const std::string &path) {
  if (path.empty() || path == "unknown" || path == "unavailable") return "";
  if (path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0) return path;
  if (base.empty() || path[0] != '/') return "";
  return base + path;
}

inline std::string image_card_base_url(ImageCardCtx *ctx) {
  if (!ctx) return "";
  return ctx->base_url_provider ? ctx->base_url_provider() : ctx->base_url;
}

inline std::string image_card_entity_proxy_path(const std::string &entity_id) {
  if (entity_id.rfind("camera.", 0) == 0) return "/api/camera_proxy/" + entity_id;
  if (entity_id.rfind("image.", 0) == 0) return "/api/image_proxy/" + entity_id;
  return "";
}

inline bool image_card_prefer_local_picture(ImageCardCtx *ctx) {
  return ctx && ctx->media_artwork;
}

inline std::string image_card_entity_proxy_url(ImageCardCtx *ctx) {
  if (!ctx) return "";
  return image_card_join_url(image_card_base_url(ctx), image_card_entity_proxy_path(ctx->entity_id));
}

inline std::string image_card_proxy_path_with_token(const std::string &proxy_path,
                                                    const std::string &token) {
  if (proxy_path.empty() || token.empty()) return proxy_path;
  std::string path = proxy_path;
  path += (path.find('?') == std::string::npos) ? "?token=" : "&token=";
  path += token;
  return path;
}

inline bool image_card_valid_access_token(const std::string &token) {
  return !token.empty() && token != "unknown" && token != "unavailable";
}

inline bool image_card_query_has_param(const std::string &url, const std::string &param) {
  size_t query = url.find('?');
  if (query == std::string::npos) return false;
  size_t fragment = url.find('#', query + 1);
  if (fragment == std::string::npos) fragment = url.size();
  size_t pos = query + 1;
  while (pos < fragment) {
    size_t next = url.find('&', pos);
    if (next == std::string::npos || next > fragment) next = fragment;
    size_t eq = url.find('=', pos);
    if (eq != std::string::npos && eq < next &&
        eq - pos == param.size() &&
        url.compare(pos, param.size(), param) == 0) {
      return true;
    }
    pos = next + 1;
  }
  return false;
}

inline std::string image_card_append_query_param(const std::string &url,
                                                const std::string &param,
                                                int value) {
  if (image_card_query_has_param(url, param)) return url;
  std::string next = url;
  std::string fragment;
  size_t fragment_pos = next.find('#');
  if (fragment_pos != std::string::npos) {
    fragment = next.substr(fragment_pos);
    next.erase(fragment_pos);
  }
  next += (next.find('?') == std::string::npos) ? "?" : "&";
  next += param;
  next += "=";
  next += std::to_string(value);
  next += fragment;
  return next;
}

inline bool image_card_home_assistant_proxy_url(const std::string &url) {
  return url.find("/api/camera_proxy/") != std::string::npos ||
         url.find("/api/image_proxy/") != std::string::npos ||
         url.find("/api/media_player_proxy/") != std::string::npos;
}

inline bool image_card_protected_home_assistant_proxy_url(const std::string &url) {
  return url.find("/api/camera_proxy/") != std::string::npos ||
         url.find("/api/image_proxy/") != std::string::npos;
}

inline bool image_card_home_assistant_proxy_authed(const std::string &url) {
  return !image_card_protected_home_assistant_proxy_url(url) || image_card_query_has_param(url, "token");
}

inline std::string image_card_sized_url(const std::string &url,
                                        lv_coord_t width,
                                        lv_coord_t height) {
  if (!image_card_protected_home_assistant_proxy_url(url) || width <= 0 || height <= 0) {
    return url;
  }
  std::string next = image_card_append_query_param(url, "width", static_cast<int>(width));
  return image_card_append_query_param(next, "height", static_cast<int>(height));
}

inline void image_card_handle_picture(ImageCardCtx *ctx, esphome::StringRef picture);
inline void image_card_handle_media_artwork_picture(ImageCardCtx *ctx,
                                                    esphome::StringRef picture,
                                                    bool local);
inline void image_card_request_picture(ImageCardCtx *ctx);
inline void image_card_request_media_artwork(ImageCardCtx *ctx);
inline bool image_card_context_current(ImageCardCtx *ctx,
                                       const std::string &entity_id,
                                       uint32_t generation);

inline void image_card_request_current_picture(ImageCardCtx *ctx) {
  if (!ctx) return;
  if (ctx->media_artwork) {
    image_card_request_media_artwork(ctx);
  } else {
    image_card_request_picture(ctx);
  }
}

// Explicit refreshes (for example after Home Assistant reconnects) must read
// both artwork attributes. Timed retries keep the mask so they only resubmit
// the source that previously failed to queue.
inline void image_card_refresh_current_picture(ImageCardCtx *ctx) {
  if (!ctx) return;
  if (ctx->media_artwork) {
    ctx->media_artwork_retry_mask = 0;
    ctx->media_artwork_sources.clear();
    ctx->pending_fallback_picture.clear();
    if (ctx->media_artwork_timer) {
      lv_timer_del(ctx->media_artwork_timer);
      ctx->media_artwork_timer = nullptr;
    }
  }
  image_card_request_current_picture(ctx);
}

inline void image_card_schedule_picture_retry(ImageCardCtx *ctx, uint32_t delay_ms) {
  if (!ctx || !ctx->active) return;
  ctx->next_picture_retry_ms = esphome::millis() + delay_ms;
}

inline void image_card_request_picture(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || ctx->entity_id.empty()) return;
  image_card_log_diagnostics(ctx, "picture-request");
  if (!ha_api_connected()) {
    image_card_log_diagnostics(ctx, "picture-waiting-ha-api");
    if (image_card_startup_retry_active(ctx)) {
      image_card_schedule_picture_retry(ctx, IMAGE_CARD_RETRY_INTERVAL_MS);
      image_card_set_loading_state(ctx, "Loading", true);
    }
    return;
  }
  const std::string entity_id = ctx->entity_id;
  std::string proxy_path = image_card_entity_proxy_path(entity_id);
  if (!proxy_path.empty()) {
    if (image_card_base_url(ctx).empty()) {
      image_card_log_diagnostics(ctx, "picture-waiting-base-url");
      image_card_schedule_picture_retry(ctx, IMAGE_CARD_RETRY_INTERVAL_MS);
      image_card_set_loading_state(ctx, "Loading", true);
      return;
    }
    if (image_card_valid_access_token(ctx->access_token)) {
      std::string authed_path = image_card_proxy_path_with_token(proxy_path, ctx->access_token);
      image_card_handle_picture(ctx, esphome::StringRef(authed_path));
      return;
    }
    if (ctx->access_token_request_pending) {
      image_card_log_diagnostics(ctx, "picture-waiting-token-request");
      image_card_schedule_picture_retry(ctx, IMAGE_CARD_RETRY_INTERVAL_MS);
      image_card_set_loading_state(ctx, "Loading", true);
      return;
    }
    const uint32_t generation = ha_subscription_generation();
    ctx->access_token_request_pending = true;
    bool requested = ha_get_attribute(
      entity_id,
      std::string("access_token"),
      std::function<void(esphome::StringRef)>(
        [ctx, entity_id, generation, proxy_path](esphome::StringRef token_ref) {
          if (!image_card_context_current(ctx, entity_id, generation)) return;
          ctx->access_token_request_pending = false;
          std::string token = string_ref_limited(token_ref, 512);
          if (!image_card_valid_access_token(token)) {
            ctx->access_token.clear();
            image_card_log_diagnostics(ctx, "picture-waiting-token");
            image_card_schedule_picture_retry(ctx, IMAGE_CARD_RETRY_INTERVAL_MS);
            image_card_set_loading_state(ctx, "Loading", true);
            return;
          }
          ctx->access_token = token;
          std::string authed_path = image_card_proxy_path_with_token(proxy_path, token);
          image_card_handle_picture(ctx, esphome::StringRef(authed_path));
        })
    );
    if (!requested) {
      ctx->access_token_request_pending = false;
      image_card_log_diagnostics(ctx, "picture-attribute-request-queued");
      image_card_schedule_picture_retry(
        ctx,
        ha_api_connected() ? IMAGE_CARD_API_RETRY_INTERVAL_MS : IMAGE_CARD_RETRY_INTERVAL_MS);
      image_card_set_loading_state(ctx, "Loading", true);
    }
    return;
  }
  if (image_card_prefer_local_picture(ctx)) {
    const uint32_t generation = ha_subscription_generation();
    bool requested_local = ha_get_attribute(
      entity_id,
      std::string("entity_picture_local"),
      std::function<void(esphome::StringRef)>(
        [ctx, entity_id, generation](esphome::StringRef picture) {
          if (!image_card_context_current(ctx, entity_id, generation)) return;
          std::string local = string_ref_limited(picture, 4096);
          if (!local.empty() && local != "unknown" && local != "unavailable") {
            ctx->pending_fallback_picture.clear();
            image_card_handle_picture(ctx, picture);
            return;
          }
          if (!ctx->pending_fallback_picture.empty()) {
            std::string fallback = ctx->pending_fallback_picture;
            ctx->pending_fallback_picture.clear();
            image_card_handle_picture(ctx, esphome::StringRef(fallback));
            return;
          }
          bool fallback_requested = ha_get_attribute(
            entity_id,
            std::string("entity_picture"),
            std::function<void(esphome::StringRef)>(
              [ctx, entity_id, generation](esphome::StringRef fallback_picture) {
                if (!image_card_context_current(ctx, entity_id, generation)) return;
                image_card_handle_picture(ctx, fallback_picture);
              })
          );
          if (!fallback_requested) image_card_handle_picture(ctx, picture);
        })
    );
    if (requested_local) return;
  }
  const uint32_t generation = ha_subscription_generation();
  bool requested = ha_get_attribute(
    entity_id,
    std::string("entity_picture"),
    std::function<void(esphome::StringRef)>(
      [ctx, entity_id, generation](esphome::StringRef picture) {
        if (!image_card_context_current(ctx, entity_id, generation)) return;
        if (image_card_prefer_local_picture(ctx)) {
          ctx->pending_fallback_picture = string_ref_limited(picture, 4096);
          image_card_request_picture(ctx);
          return;
        }
        image_card_handle_picture(ctx, picture);
      })
  );
  if (!requested && (ha_api_connected() || image_card_startup_retry_active(ctx))) {
    ESP_LOGD("image_card", "Queued entity_picture retry for %s: connected=%d state_connected=%d",
             entity_id.c_str(), ha_api_connected(), ha_api_state_connected());
    image_card_log_diagnostics(ctx, "picture-retry-queued");
    image_card_schedule_picture_retry(
      ctx,
      ha_api_connected() ? IMAGE_CARD_API_RETRY_INTERVAL_MS : IMAGE_CARD_RETRY_INTERVAL_MS);
  }
}

inline void subscribe_image_card_access_token(ImageCardCtx *ctx,
                                              const std::string &entity_id) {
  if (!ctx || image_card_entity_proxy_path(entity_id).empty()) return;
  const uint32_t generation = ha_subscription_generation();
  ha_subscribe_attribute(
    entity_id,
    std::string("access_token"),
    std::function<void(esphome::StringRef)>(
      [ctx, entity_id, generation](esphome::StringRef token_ref) {
        if (!image_card_context_current(ctx, entity_id, generation)) return;
        ctx->access_token_request_pending = false;
        std::string token = string_ref_limited(token_ref, 512);
        if (!image_card_valid_access_token(token)) {
          ctx->access_token.clear();
          return;
        }
        if (token == ctx->access_token) return;
        ctx->access_token = token;
        image_card_request_picture(ctx);
      })
  );
}

inline void subscribe_image_card_entity_state(ImageCardCtx *ctx,
                                              const std::string &entity_id) {
  if (!ctx || entity_id.empty()) return;
  const uint32_t generation = ha_subscription_generation();
  ha_subscribe_state(
    entity_id,
    std::function<void(esphome::StringRef)>(
      [ctx, entity_id, generation](esphome::StringRef) {
        if (!image_card_context_current(ctx, entity_id, generation)) return;
        image_card_request_picture(ctx);
      })
  );
}

inline bool image_card_context_current(ImageCardCtx *ctx,
                                       const std::string &entity_id,
                                       uint32_t generation) {
  return ctx && ctx->active &&
         generation == ha_subscription_generation() &&
         ctx->entity_id == entity_id;
}

inline void image_card_request_source_url(ImageCardCtx *ctx, bool source_changed) {
  if (!ctx || !ctx->active || !ctx->image || ctx->source_url.empty()) return;
  uint32_t now = esphome::millis();
  if (image_card_modal_active_for(ctx)) {
    ctx->next_download_retry_ms = now + IMAGE_CARD_MODAL_REFRESH_DELAY_MS;
    ESP_LOGD("image_card", "Deferring image refresh while modal is open for %s", ctx->entity_id.c_str());
    image_card_log_diagnostics(ctx, "tile-refresh-deferred-modal");
    return;
  }
  lv_coord_t width = 0;
  lv_coord_t height = 0;
  esphome::artwork_image::ImageResizeMode resize_mode =
    esphome::artwork_image::ImageResizeMode::COVER;
  if (!image_card_position_widget(ctx->btn, ctx->widget, &width, &height)) return;
  lv_coord_t decode_width = image_card_media_artwork_target_width(ctx, width);
  lv_coord_t decode_height = height;
  lv_obj_t *loading = image_card_loading_widget(ctx->widget);
  image_card_position_widget(ctx->btn, loading);
  image_card_refresh_loading_layout(loading);
  bool replace_pending_request = ctx->download_active && source_changed;
  if (ctx->download_active && !source_changed) {
    ESP_LOGD("image_card", "Skipping duplicate image refresh while download is active for %s",
             ctx->entity_id.c_str());
    image_card_log_diagnostics(ctx, "tile-refresh-duplicate", width, height);
    return;
  }
  ImageCardCtx *active_download = image_card_active_download_context();
  if (!replace_pending_request && active_download && active_download != ctx) {
    ctx->download_queued = true;
    ctx->next_download_retry_ms = now + IMAGE_CARD_API_RETRY_INTERVAL_MS;
    ESP_LOGD("image_card", "Deferring image refresh for %s while %s is downloading",
             ctx->entity_id.c_str(), active_download->entity_id.c_str());
    image_card_log_diagnostics(ctx, "tile-refresh-queued", width, height);
    return;
  }
  if (!replace_pending_request && !image_card_memory_available(ctx, "tile", decode_width, decode_height)) {
    ctx->next_download_retry_ms = now + IMAGE_CARD_RETRY_INTERVAL_MS;
    if (!ctx->image_ready) {
      image_card_hide(ctx);
      image_card_set_loading_state(ctx, "Unavailable", true);
    }
    return;
  }
  int request_width = 0;
  int request_height = 0;
  image_card_tile_request_size(decode_width, decode_height, &request_width, &request_height);
  ctx->url = image_card_sized_url(ctx->source_url, request_width, request_height);
  ctx->requested_once = true;
  ctx->download_active = true;
  ctx->download_queued = false;
  image_card_active_download_context() = ctx;
  ctx->next_download_retry_ms = 0;
  ctx->last_tile_request_started_ms = now;
  ctx->image->set_target_size(decode_width, decode_height);
  ctx->image->set_resize_mode(resize_mode);
  ESP_LOGI("image_card", "%s camera image for %s",
           replace_pending_request ? "Updating queued" : "Downloading", ctx->entity_id.c_str());
  image_card_log_diagnostics(ctx, "tile-download-start", request_width, request_height);
  int max_source_dim = request_width > request_height ? request_width : request_height;
  std::string effective_url = ctx->image->request_update_url(ctx->url, max_source_dim);
  if (effective_url.empty()) image_card_release_download_slot(ctx);
  if (!effective_url.empty()) {
    ctx->url = effective_url;
  }
}

inline bool image_card_request_modal_source_url(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !image_card_has_separate_modal_image(ctx) ||
      ctx->source_url.empty() || !image_card_modal_active_for(ctx)) {
    return false;
  }
  if (!image_card_modal_refresh_supported()) {
    ESP_LOGI("image_card", "Using cached tile image in modal for small P4 screen: %s",
             ctx->entity_id.c_str());
    image_card_log_diagnostics(ctx, "modal-refresh-unsupported");
    return false;
  }
  ImageCardModalUi &ui = image_card_modal_ui();
  lv_obj_update_layout(ui.panel);
  lv_coord_t panel_width = lv_obj_get_width(ui.panel);
  lv_coord_t panel_height = lv_obj_get_height(ui.panel);
  int width = 0;
  int height = 0;
  image_card_limit_target_size(panel_width, panel_height, &width, &height);
  if (width <= 0 || height <= 0) return false;
  if (!image_card_memory_available(ctx, "modal", width, height)) return false;
  ctx->modal_url = image_card_sized_url(ctx->source_url, width, height);
  ctx->modal_source_url = ctx->source_url;
  ctx->modal_image->set_target_size(width, height);
  ctx->modal_image->set_resize_mode(
    ctx->modal_fit ? esphome::artwork_image::ImageResizeMode::FIT
                   : esphome::artwork_image::ImageResizeMode::COVER);
  if (!image_card_modal_has_preview(ctx)) image_card_show_modal_loading(ctx, "Loading");
  ESP_LOGI("image_card", "Downloading modal camera image for %s", ctx->entity_id.c_str());
  ctx->last_modal_request_started_ms = esphome::millis();
  image_card_log_diagnostics(ctx, "modal-download-start", width, height);
  int max_source_dim = width > height ? width : height;
  std::string effective_url = ctx->modal_image->request_update_url(ctx->modal_url, max_source_dim);
  if (effective_url.empty()) return false;
  ctx->modal_url = effective_url;
  return true;
}

inline void image_card_modal_request_timer_cb(lv_timer_t *timer) {
  ImageCardModalUi &ui = image_card_modal_ui();
  ImageCardCtx *ctx = static_cast<ImageCardCtx *>(lv_timer_get_user_data(timer));
  if (ui.request_timer == timer) ui.request_timer = nullptr;
  lv_timer_del(timer);
  if (!ctx || !image_card_modal_active_for(ctx)) return;
  if (!image_card_request_modal_source_url(ctx)) {
    image_card_show_modal_download_failure(ctx);
  }
}

inline void image_card_cancel_modal_request_timer() {
  ImageCardModalUi &ui = image_card_modal_ui();
  if (!ui.request_timer) return;
  lv_timer_del(ui.request_timer);
  ui.request_timer = nullptr;
}

inline bool image_card_queue_modal_source_request(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !image_card_has_separate_modal_image(ctx) ||
      ctx->source_url.empty() || !image_card_modal_active_for(ctx) ||
      !image_card_modal_refresh_supported()) {
    return false;
  }
  ImageCardModalUi &ui = image_card_modal_ui();
  if (!image_card_modal_has_preview(ctx)) image_card_show_modal_loading(ctx, "Loading");
  image_card_cancel_modal_request_timer();
  ui.request_timer = lv_timer_create(
    image_card_modal_request_timer_cb, IMAGE_CARD_MODAL_REQUEST_DELAY_MS, ctx);
  if (!ui.request_timer) {
    image_card_log_diagnostics(ctx, "modal-request-immediate");
    bool requested = image_card_request_modal_source_url(ctx);
    if (!requested) image_card_show_modal_download_failure(ctx);
    return requested;
  }
  image_card_log_diagnostics(ctx, "modal-request-timer-created");
  return true;
}

inline void image_card_schedule_source_refresh(ImageCardCtx *ctx, uint32_t delay_ms,
                                               const char *reason) {
  if (!ctx || !ctx->active || ctx->source_url.empty()) return;
  ctx->next_download_retry_ms = esphome::millis() + delay_ms;
  ESP_LOGI("image_card", "Queued %s image refresh for %s in %lu ms",
           reason ? reason : "camera", ctx->entity_id.c_str(),
           static_cast<unsigned long>(delay_ms));
}

inline void image_card_finish_modal_cleanup(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !ctx->image || image_card_modal_active_for(ctx)) return;
  image_card_log_diagnostics(ctx, "modal-cleanup");
  ImageCardCtx *active_modal = image_card_modal_ui().active;
  bool shared_modal_in_use = active_modal && active_modal->modal_image == ctx->modal_image;
  if (esphome::artwork_image::image_pipeline_should_cancel_modal_cleanup(
        image_card_has_separate_modal_image(ctx), shared_modal_in_use)) {
    ctx->modal_image->cancel_update();
  }
  image_card_apply_context_widget_geometry(ctx);
}

inline void image_card_modal_cleanup_timer_cb(lv_timer_t *timer) {
  ImageCardCtx *ctx = static_cast<ImageCardCtx *>(lv_timer_get_user_data(timer));
  if (ctx && ctx->modal_cleanup_timer == timer) ctx->modal_cleanup_timer = nullptr;
  lv_timer_del(timer);
  image_card_finish_modal_cleanup(ctx);
}

inline void image_card_schedule_modal_cleanup(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active) return;
  if (ctx->modal_cleanup_timer) {
    lv_timer_del(ctx->modal_cleanup_timer);
    ctx->modal_cleanup_timer = nullptr;
  }
  ctx->modal_cleanup_timer = lv_timer_create(
    image_card_modal_cleanup_timer_cb, IMAGE_CARD_MODAL_CLEANUP_DELAY_MS, ctx);
  if (!ctx->modal_cleanup_timer) image_card_finish_modal_cleanup(ctx);
}

inline void image_card_abort_modal_open(ImageCardCtx *ctx, const char *reason) {
  ESP_LOGW("image_card", "Unable to open image modal for %s: %s",
           ctx ? ctx->entity_id.c_str() : "(unknown)",
           reason ? reason : "modal setup failed");
  image_card_log_diagnostics(ctx, "modal-open-abort");
  ImageCardModalUi &ui = image_card_modal_ui();
  image_card_cancel_modal_request_timer();
  image_card_clear_widget_source(ui.image_widget);
  control_modal_delete_overlay(ControlModalKind::IMAGE_CARD, ui.overlay);
  ui = ImageCardModalUi();
  if (ctx && ctx->end_display_takeover) {
    ctx->end_display_takeover(espcontrol::DisplayTakeoverKind::INTERACTIVE);
  }
  image_card_schedule_modal_cleanup(ctx);
}

inline void image_card_hide_modal() {
  ImageCardModalUi &ui = image_card_modal_ui();
  ImageCardCtx *ctx = ui.active;
  if (ctx) ESP_LOGI("image_card", "Closing image modal for %s", ctx->entity_id.c_str());
  image_card_log_diagnostics(ctx, "modal-close");
  image_card_cancel_modal_request_timer();
  image_card_clear_widget_source(ui.image_widget);
  control_modal_delete_overlay(ControlModalKind::IMAGE_CARD, ui.overlay);
  ui = ImageCardModalUi();
  if (ctx && ctx->end_display_takeover) {
    ctx->end_display_takeover(espcontrol::DisplayTakeoverKind::INTERACTIVE);
  }
  image_card_schedule_modal_cleanup(ctx);
}

inline void image_card_open_modal(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !ctx->image ||
      !esphome::artwork_image::image_pipeline_modal_can_open(
        ctx->image_ready, !ctx->source_url.empty())) {
    ESP_LOGW("image_card", "No camera card is available to open");
    image_card_log_diagnostics(ctx, "modal-open-not-ready");
    return;
  }
  ESP_LOGI("image_card", "Opening image modal for %s", ctx->entity_id.c_str());
  ctx->modal_open_started_ms = esphome::millis();
  image_card_log_diagnostics(ctx, "modal-open-start");
  if (ctx->modal_cleanup_timer) {
    lv_timer_del(ctx->modal_cleanup_timer);
    ctx->modal_cleanup_timer = nullptr;
    image_card_finish_modal_cleanup(ctx);
  }
  image_card_cancel_stale_modal_download(ctx);
  // Keep any scheduled tile retry alive. While the modal is open the normal
  // refresh path defers it, then starts it shortly after the modal closes.
  image_card_prioritize_modal_download(ctx);

  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::IMAGE_CARD, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, image_card_hide_modal);
  if (!shell.overlay || !shell.panel || !shell.close_btn) {
    ESP_LOGW("image_card", "Unable to open image modal for %s: modal shell setup failed",
             ctx->entity_id.c_str());
    return;
  }
  control_modal_block_close_for(IMAGE_CARD_MODAL_CLOSE_GUARD_MS);

  ImageCardModalUi &ui = image_card_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  if (ctx->begin_display_takeover) {
    ctx->begin_display_takeover(espcontrol::DisplayTakeoverKind::INTERACTIVE);
  }
  image_card_log_diagnostics(ctx, "modal-display-takeover-began");
  image_card_style_modal_back_button(ui.back_btn, shell.layout);

  lv_obj_set_style_bg_color(ui.panel, lv_color_hex(DARK_OVERLAY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.panel, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_clip_corner(ui.panel, true, LV_PART_MAIN);

#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  ui.image_widget = lv_image_create(ui.panel);
#else
  ui.image_widget = lv_img_create(ui.panel);
#endif
  if (!ui.image_widget) {
    image_card_abort_modal_open(ctx, "image widget setup failed");
    return;
  }
  lv_obj_clear_flag(ui.image_widget, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.image_widget, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(ui.image_widget, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.image_widget, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.image_widget, LV_OPA_TRANSP, LV_PART_MAIN);

  ui.loading_widget = lv_obj_create(ui.panel);
  if (!ui.loading_widget) {
    image_card_abort_modal_open(ctx, "loading widget setup failed");
    return;
  }
  lv_obj_set_style_bg_color(ui.loading_widget, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.loading_widget, LV_OPA_70, LV_PART_MAIN);
  lv_obj_set_style_border_color(ui.loading_widget, lv_color_hex(DARK_BORDER), LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.loading_widget, 1, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.loading_widget, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.loading_widget, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.loading_widget, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.loading_widget, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ui.loading_widget, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *loading_icon = lv_label_create(ui.loading_widget);
  if (!loading_icon) {
    image_card_abort_modal_open(ctx, "loading icon setup failed");
    return;
  }
  if (ctx->icon_font) lv_obj_set_style_text_font(loading_icon, ctx->icon_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(loading_icon, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_opa(loading_icon, LV_OPA_COVER, LV_PART_MAIN);
  lv_label_set_text(loading_icon, IMAGE_CARD_LOADING_ICON);

  lv_obj_t *loading_label = lv_label_create(ui.loading_widget);
  if (!loading_label) {
    image_card_abort_modal_open(ctx, "loading label setup failed");
    return;
  }
  if (ctx->label_font) lv_obj_set_style_text_font(loading_label, ctx->label_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(loading_label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_opa(loading_label, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_text_align(loading_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  lv_label_set_long_mode(loading_label, LV_LABEL_LONG_DOT);
  lv_label_set_text(loading_label, espcontrol_i18n("Loading"));

  ImageCardModalCache &modal_cache = image_card_modal_cache();
  if (image_card_modal_cache_matches(ctx)) {
    image_card_show_modal_image(ctx, modal_cache.image);
    image_card_log_diagnostics(ctx, "modal-cache-shown");
  } else if (ctx->image_ready) {
    image_card_show_modal_image(ctx, ctx->image);
    image_card_log_diagnostics(ctx, "modal-tile-fallback-shown");
  } else {
    image_card_show_modal_loading(ctx, "Loading");
    image_card_log_diagnostics(ctx, "modal-open-before-tile-ready");
  }
  image_card_queue_modal_source_request(ctx);
  lv_obj_move_foreground(ui.back_btn);
  lv_obj_move_foreground(ui.overlay);
}

inline void image_card_handle_picture(ImageCardCtx *ctx, esphome::StringRef picture) {
  if (!ctx || !ctx->active || !ctx->image) return;
  std::string raw = string_ref_limited(picture, 4096);
  std::string base_url = image_card_base_url(ctx);
  std::string url = image_card_join_url(base_url, raw);
  if (url.empty()) {
    std::string proxy_path = image_card_entity_proxy_path(ctx->entity_id);
    if (!proxy_path.empty() && base_url.empty()) {
      ESP_LOGD("image_card", "Waiting for Home Assistant base URL before loading %s",
               ctx->entity_id.c_str());
      image_card_log_diagnostics(ctx, "picture-empty-base-url");
      image_card_hide(ctx);
      image_card_set_loading_state(ctx, "Loading", true);
      image_card_schedule_picture_retry(ctx, IMAGE_CARD_RETRY_INTERVAL_MS);
      return;
    }
  }
  if (url.empty()) {
    ESP_LOGW("image_card", "No usable image URL for %s", ctx->entity_id.c_str());
    image_card_log_diagnostics(ctx, "picture-no-url");
    if (ctx->media_artwork) {
      image_card_clear_media_artwork(ctx);
      return;
    }
    if (ctx->image_ready) return;
    image_card_hide(ctx);
    if (image_card_startup_retry_active(ctx)) {
      ctx->next_picture_retry_ms = esphome::millis() + IMAGE_CARD_RETRY_INTERVAL_MS;
      image_card_set_loading_state(ctx, "Loading", true);
    } else {
      image_card_set_loading_state(ctx, "Unavailable", true);
    }
    return;
  }
  if (!image_card_home_assistant_proxy_authed(url) &&
      image_card_valid_access_token(ctx->access_token)) {
    url = image_card_proxy_path_with_token(url, ctx->access_token);
  }
  if (!image_card_home_assistant_proxy_authed(url) &&
      !ctx->access_token_request_pending) {
    const std::string entity_id = ctx->entity_id;
    const std::string retry_picture = raw;
    const uint32_t generation = ha_subscription_generation();
    ctx->access_token_request_pending = true;
    bool requested = ha_get_attribute(
      entity_id,
      std::string("access_token"),
      std::function<void(esphome::StringRef)>(
        [ctx, entity_id, retry_picture, generation](esphome::StringRef token_ref) {
          if (!image_card_context_current(ctx, entity_id, generation)) return;
          ctx->access_token_request_pending = false;
          std::string token = string_ref_limited(token_ref, 512);
          if (!image_card_valid_access_token(token)) {
            ctx->access_token.clear();
            image_card_schedule_picture_retry(ctx, IMAGE_CARD_RETRY_INTERVAL_MS);
            image_card_set_loading_state(ctx, "Loading", true);
            return;
          }
          ctx->access_token = token;
          image_card_handle_picture(ctx, esphome::StringRef(retry_picture));
        })
    );
    if (requested) {
      image_card_schedule_picture_retry(ctx, IMAGE_CARD_RETRY_INTERVAL_MS);
      image_card_set_loading_state(ctx, "Loading", true);
      return;
    }
    ctx->access_token_request_pending = false;
  }
  if (!image_card_home_assistant_proxy_authed(url)) {
    ESP_LOGW("image_card", "Skipping unauthenticated Home Assistant image proxy for %s",
             ctx->entity_id.c_str());
    image_card_log_diagnostics(ctx, "picture-unauthenticated-proxy");
    if (ctx->image_ready) return;
    image_card_hide(ctx);
    if (image_card_startup_retry_active(ctx)) {
      image_card_schedule_picture_retry(ctx, IMAGE_CARD_RETRY_INTERVAL_MS);
      image_card_set_loading_state(ctx, "Loading", true);
    } else {
      image_card_set_loading_state(ctx, "Unavailable", true);
    }
    return;
  }
  if (espcontrol::artwork::artwork_picture_response_clears_retry(
        ctx->media_artwork, ctx->media_artwork_retry_mask)) {
    ctx->next_picture_retry_ms = 0;
  }
  uint32_t now = esphome::millis();
  bool source_changed = ctx->source_url != url;
  if (ctx->image_ready && ctx->source_url == url && ctx->last_download_completed_ms != 0 &&
      (uint32_t)(now - ctx->last_download_completed_ms) < IMAGE_CARD_MIN_REPEAT_REFRESH_MS) {
    ESP_LOGD("image_card", "Skipping recent image refresh for %s", ctx->entity_id.c_str());
    image_card_log_diagnostics(ctx, "picture-recent-refresh-skipped");
    return;
  }
  ctx->source_url = url;
  image_card_log_diagnostics(ctx, "picture-url-ready");
  if (image_card_modal_active_for(ctx)) {
    image_card_queue_modal_source_request(ctx);
    image_card_schedule_source_refresh(ctx, IMAGE_CARD_MODAL_REFRESH_DELAY_MS, "tile");
    return;
  }
  image_card_request_source_url(ctx, source_changed);
}

inline void image_card_process_media_artwork(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !ctx->media_artwork) return;
  const espcontrol::artwork::SourceSelection selection =
      ctx->media_artwork_sources.select(ctx->source_url, false);
  const std::string &chosen = selection.primary;
  if (chosen.empty()) {
    image_card_clear_media_artwork(ctx);
    return;
  }
  image_card_handle_picture(ctx, esphome::StringRef(chosen));
}

inline void image_card_media_artwork_timer_cb(lv_timer_t *timer) {
  ImageCardCtx *ctx = static_cast<ImageCardCtx *>(lv_timer_get_user_data(timer));
  if (ctx && ctx->media_artwork_timer == timer) ctx->media_artwork_timer = nullptr;
  lv_timer_del(timer);
  image_card_process_media_artwork(ctx);
}

inline void image_card_schedule_media_artwork_process(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !ctx->media_artwork) return;
  if (ctx->media_artwork_timer) lv_timer_del(ctx->media_artwork_timer);
  ctx->media_artwork_timer = lv_timer_create(
    image_card_media_artwork_timer_cb, IMAGE_CARD_MEDIA_ARTWORK_DEBOUNCE_MS, ctx);
  if (!ctx->media_artwork_timer) image_card_process_media_artwork(ctx);
}

inline void image_card_handle_media_artwork_picture(ImageCardCtx *ctx,
                                                    esphome::StringRef picture,
                                                    bool local) {
  if (!ctx || !ctx->active || !ctx->media_artwork) return;
  ctx->media_artwork_retry_mask = espcontrol::artwork::artwork_source_mark_received(
    ctx->media_artwork_retry_mask, local);
  std::string raw = string_ref_limited(picture, 4096);
  std::string url = image_card_join_url(image_card_base_url(ctx), raw);
  // These two attribute requests run independently. A delayed remote callback
  // must not discard a newer local proxy URL that has already arrived.
  ctx->media_artwork_sources.update(
      local, url,
      espcontrol::artwork::RemoteUpdatePolicy::PRESERVE_LOCAL);
  if (local) {
    if (!url.empty() && url != ctx->source_url) ctx->startup_download_errors = 0;
    if (!url.empty()) ctx->pending_fallback_picture.clear();
  } else {
    ctx->pending_fallback_picture = raw;
    if (!url.empty() && url != ctx->source_url) {
      ctx->startup_download_errors = 0;
    }
  }
  if (espcontrol::artwork::source_response_can_apply_immediately(local, !url.empty())) {
    if (ctx->media_artwork_timer) {
      lv_timer_del(ctx->media_artwork_timer);
      ctx->media_artwork_timer = nullptr;
    }
    image_card_log_diagnostics(ctx, "media-artwork-local-immediate");
    image_card_process_media_artwork(ctx);
    return;
  }
  image_card_schedule_media_artwork_process(ctx);
}

inline void image_card_request_media_artwork(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !ctx->media_artwork || ctx->entity_id.empty()) return;
  const std::string entity_id = ctx->entity_id;
  const uint32_t generation = ha_subscription_generation();
  uint8_t request_mask = espcontrol::artwork::artwork_source_request_mask(
    ctx->media_artwork_retry_mask);
  bool remote_queued = true;
  if ((request_mask & espcontrol::artwork::ARTWORK_SOURCE_REMOTE) != 0) {
    remote_queued = ha_get_attribute(
      entity_id,
      std::string("entity_picture"),
      std::function<void(esphome::StringRef)>(
        [ctx, entity_id, generation](esphome::StringRef picture) {
          if (!image_card_context_current(ctx, entity_id, generation)) return;
          image_card_handle_media_artwork_picture(ctx, picture, false);
        })
    );
  }
  bool local_queued = true;
  if ((request_mask & espcontrol::artwork::ARTWORK_SOURCE_LOCAL) != 0) {
    local_queued = ha_get_attribute(
      entity_id,
      std::string("entity_picture_local"),
      std::function<void(esphome::StringRef)>(
        [ctx, entity_id, generation](esphome::StringRef picture) {
          if (!image_card_context_current(ctx, entity_id, generation)) return;
          image_card_handle_media_artwork_picture(ctx, picture, true);
        })
    );
  }
  ctx->media_artwork_retry_mask = espcontrol::artwork::artwork_source_failed_mask(
    request_mask, remote_queued, local_queued);
  if (ctx->media_artwork_retry_mask != 0) {
    image_card_log_diagnostics(ctx, "media-artwork-retry-queued");
    image_card_schedule_picture_retry(
      ctx,
      ha_api_connected() ? IMAGE_CARD_API_RETRY_INTERVAL_MS : IMAGE_CARD_RETRY_INTERVAL_MS);
    if (!ctx->image_ready) image_card_set_loading_state(ctx, "Loading", true);
  }
}

inline void refresh_image_cards() {
  if (!ha_api_connected()) return;
  ImageCardCtx *contexts = image_card_contexts();
  uint32_t now = esphome::millis();
  for (int i = 0; i < IMAGE_CARD_MAX_CONTEXTS; i++) {
    ImageCardCtx *ctx = &contexts[i];
    if (!ctx->active) continue;
    if (!ctx->image_ready) {
      ctx->retry_deadline_ms = now + IMAGE_CARD_STARTUP_RETRY_MS;
      image_card_set_loading_state(ctx, "Loading", true);
    }
    ctx->next_picture_retry_ms = 0;
    image_card_refresh_current_picture(ctx);
  }
}

inline void image_card_refresh_due() {
  ImageCardCtx *contexts = image_card_contexts();
  uint32_t now = esphome::millis();
  for (int i = 0; i < IMAGE_CARD_MAX_CONTEXTS; i++) {
    ImageCardCtx *ctx = &contexts[i];
    if (!ctx->active) continue;
    if (ctx->next_picture_retry_ms != 0 &&
        (int32_t)(now - ctx->next_picture_retry_ms) >= 0) {
      ctx->next_picture_retry_ms = 0;
      image_card_request_current_picture(ctx);
    }
    if (ctx->next_download_retry_ms != 0 &&
        (int32_t)(now - ctx->next_download_retry_ms) >= 0) {
      ctx->next_download_retry_ms = 0;
      image_card_request_source_url(ctx);
    }
  }
}

inline bool image_card_bind_runtime(BtnSlot &s, const ParsedCfg &p,
                                    const GridConfig &cfg,
                                    bool bind_click_handler = false) {
  lv_obj_t *widget = s.sensor_container
    ? static_cast<lv_obj_t *>(lv_obj_get_user_data(s.sensor_container))
    : nullptr;
  lv_obj_t *loading = image_card_loading_widget(widget);
  if (p.entity.empty()) {
    image_card_set_loading_state(loading, "Configure");
    return true;
  }
  if (!image_card_entity_supported(p.entity)) {
    ESP_LOGW("image_card", "Image card only supports camera and image entities: %s", p.entity.c_str());
    image_card_set_loading_state(loading, "Unavailable");
    return true;
  }
  image_card_configure_label(s, p);
  image_card_configure_icon(s, p);
  ImageCardCtx *ctx = acquire_image_card_context(cfg, p.entity);
  if (!ctx) {
    ESP_LOGW("image_card", "No image card downloader available for %s", p.entity.c_str());
    image_card_set_loading_state(loading, "Too many");
    return true;
  }
  ctx->widget = widget;
  ctx->btn = s.btn;
  ctx->loading_widget = loading;
  ctx->loading_label = image_card_loading_label(loading);
  ctx->icon_font = image_card_icon_font_for_slot(s);
  ctx->label_font = image_card_label_font_for_slot(s);
  image_card_apply_loading_fonts(loading, ctx->icon_font, ctx->label_font);
  ctx->entity_id = p.entity;
  ctx->base_url = cfg.home_assistant_base_url ? cfg.home_assistant_base_url() : "";
  ctx->base_url_provider = cfg.home_assistant_base_url;
  ctx->begin_display_takeover = cfg.begin_display_takeover;
  ctx->end_display_takeover = cfg.end_display_takeover;
  ctx->modal_fit = image_card_modal_fit_enabled(p);
  ctx->media_artwork = false;
  ctx->media_overlay = nullptr;
  ctx->pending_fallback_picture.clear();
  ctx->media_artwork_retry_mask = 0;
  ctx->diagnostics_enabled = cfg.image_card_diagnostics;
  ctx->retry_deadline_ms = esphome::millis() + IMAGE_CARD_STARTUP_RETRY_MS;
  ctx->width_compensation_percent = cfg.width_compensation_percent;
  ctx->media_artwork_width_compensation_percent = 100;
  image_card_log_diagnostics(ctx, "bind-card");
  int cached_target_width = ctx->image->get_fixed_width();
  int cached_target_height = ctx->image->get_fixed_height();
  image_card_apply_widget_geometry(ctx->btn, ctx->widget, ctx->image);
  if (esphome::artwork_image::image_pipeline_cached_target_changed(
        ctx->image_ready, cached_target_width, cached_target_height,
        ctx->image->get_fixed_width(), ctx->image->get_fixed_height())) {
    ctx->last_download_completed_ms = 0;
    image_card_log_diagnostics(ctx, "cached-tile-resize-refresh");
  }
  if (ctx->image_ready) {
    image_card_hide_loading(ctx);
    image_card_set_widget_source(ctx->widget, ctx->image);
    lv_obj_clear_flag(ctx->widget, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_background(ctx->widget);
  } else {
    image_card_set_loading_state(ctx, "Loading", true);
  }
  lv_obj_set_user_data(s.btn, ctx);
  lv_obj_add_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  if (bind_click_handler) {
    lv_obj_add_event_cb(s.btn, [](lv_event_t *e) {
      ImageCardCtx *ctx = static_cast<ImageCardCtx *>(lv_event_get_user_data(e));
      image_card_open_modal(ctx);
    }, LV_EVENT_CLICKED, ctx);
  }

  const std::string image_card_entity_id = p.entity;
  const uint32_t image_card_generation = ha_subscription_generation();
  ha_subscribe_attribute(
    image_card_entity_id,
    std::string("entity_picture"),
    std::function<void(esphome::StringRef)>(
      [ctx, image_card_entity_id, image_card_generation](esphome::StringRef picture) {
        if (!image_card_context_current(ctx, image_card_entity_id, image_card_generation)) return;
        image_card_handle_picture(ctx, picture);
      })
  );
  subscribe_image_card_access_token(ctx, image_card_entity_id);
  subscribe_image_card_entity_state(ctx, p.entity);
  image_card_request_picture(ctx);
  return true;
}
