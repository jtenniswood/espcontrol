#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

#include "esphome/core/version.h"
#include <cstring>

constexpr uint32_t IMAGE_CARD_STARTUP_RETRY_MS = 45000;
constexpr uint32_t IMAGE_CARD_RETRY_INTERVAL_MS = 2000;
constexpr uint32_t IMAGE_CARD_MODAL_REFRESH_DELAY_MS = 1000;
constexpr uint8_t IMAGE_CARD_STARTUP_DOWNLOAD_RETRIES = 10;
constexpr int IMAGE_CARD_MODAL_MAX_TARGET_SIDE_PX = 800;
constexpr const char *IMAGE_CARD_LOADING_ICON = "\U000F02E9";

struct ImageCardCtx {
  lv_obj_t *widget = nullptr;
  lv_obj_t *btn = nullptr;
  lv_obj_t *loading_widget = nullptr;
  lv_obj_t *loading_label = nullptr;
  const lv_font_t *icon_font = nullptr;
  esphome::artwork_image::ArtworkImage *image = nullptr;
  std::string entity_id;
  std::string base_url;
  std::function<std::string()> base_url_provider;
  std::string source_url;
  std::string url;
  uint32_t refresh_interval_ms = 0;
  uint32_t next_refresh_ms = 0;
  uint32_t retry_deadline_ms = 0;
  uint32_t next_picture_retry_ms = 0;
  uint32_t next_download_retry_ms = 0;
  int width_compensation_percent = 100;
  bool active = false;
  bool callbacks_bound = false;
  bool requested_once = false;
  bool image_ready = false;
  bool timer_only = false;
  bool modal_fit = false;
  uint8_t startup_download_errors = 0;
};

struct ImageCardModalUi {
  ImageCardCtx *active = nullptr;
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *image_widget = nullptr;
};

inline ImageCardCtx *image_card_contexts() {
  static ImageCardCtx contexts[4];
  return contexts;
}

inline ImageCardModalUi &image_card_modal_ui() {
  static ImageCardModalUi ui;
  return ui;
}

inline bool image_card_modal_active_for(ImageCardCtx *ctx) {
  ImageCardModalUi &ui = image_card_modal_ui();
  return ctx && ui.active == ctx && ui.overlay && ui.image_widget;
}

inline lv_obj_t *image_card_loading_widget(lv_obj_t *widget);

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

inline void image_card_sync_tile_corners(lv_obj_t *btn, lv_obj_t *widget) {
  if (!btn) return;
  lv_coord_t radius = lv_obj_get_style_radius(btn, LV_PART_MAIN);
  image_card_apply_corner_clip(btn, radius);
  image_card_apply_corner_clip(widget, radius);
  image_card_apply_corner_clip(image_card_loading_widget(widget), radius);
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

inline void image_card_refresh_loading_layout(lv_obj_t *loading_widget) {
  if (!loading_widget) return;
  lv_obj_set_layout(loading_widget, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(loading_widget, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(loading_widget, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(loading_widget, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_pad_row(loading_widget, 16, LV_PART_MAIN);
  lv_obj_t *icon = image_card_loading_icon(loading_widget);
  if (icon) {
    lv_obj_set_width(icon, lv_pct(100));
    lv_obj_set_style_text_align(icon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  }
  lv_obj_t *label = image_card_loading_label(loading_widget);
  if (label) {
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  }
  lv_obj_update_layout(loading_widget);
}

inline void image_card_set_loading_state(lv_obj_t *loading_widget, const char *text) {
  if (!loading_widget) return;
  image_card_refresh_loading_layout(loading_widget);
  lv_obj_t *icon = image_card_loading_icon(loading_widget);
  if (icon) lv_label_set_text(icon, IMAGE_CARD_LOADING_ICON);
  lv_obj_t *label = image_card_loading_label(loading_widget);
  if (label) lv_label_set_text(label, espcontrol_i18n(text));
  lv_obj_clear_flag(loading_widget, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(loading_widget);
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

inline bool image_card_startup_retry_active(ImageCardCtx *ctx,
                                            uint32_t now = esphome::millis()) {
  return ctx && ctx->retry_deadline_ms != 0 &&
         (int32_t)(now - ctx->retry_deadline_ms) < 0;
}

inline void image_card_apply_downloaded(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !ctx->widget || !ctx->image) return;
  if (ctx->image->get_url() != ctx->url) return;
  ctx->image_ready = true;
  ctx->startup_download_errors = 0;
  ctx->next_download_retry_ms = 0;
  image_card_hide_loading(ctx);
  if (image_card_modal_active_for(ctx)) {
    ImageCardModalUi &ui = image_card_modal_ui();
    image_card_set_widget_source(ui.image_widget, ctx->image);
    lv_obj_move_foreground(ui.back_btn);
    notify_dashboard_content_changed();
    return;
  }
  image_card_set_widget_source(ctx->widget, ctx->image);
  lv_obj_clear_flag(ctx->widget, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_background(ctx->widget);
  lv_obj_invalidate(ctx->widget);
  if (ctx->btn) lv_obj_invalidate(ctx->btn);
  notify_dashboard_content_changed();
}

inline void image_card_handle_download_error(ImageCardCtx *ctx) {
  if (!ctx) return;
  ESP_LOGW("image_card", "Image download failed for %s", ctx->entity_id.c_str());
  uint32_t now = esphome::millis();
  if (!ctx->image_ready && image_card_startup_retry_active(ctx, now) &&
      ctx->startup_download_errors < IMAGE_CARD_STARTUP_DOWNLOAD_RETRIES) {
    ctx->startup_download_errors++;
    ctx->next_download_retry_ms = now + IMAGE_CARD_RETRY_INTERVAL_MS;
    image_card_hide(ctx);
    image_card_set_loading_state(ctx, "Loading", true);
    return;
  }
  if (image_card_modal_active_for(ctx)) {
    return;
  } else {
    image_card_hide(ctx);
    image_card_set_loading_state(ctx, "Unavailable", true);
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

inline void image_card_hide_modal();

inline void reset_image_card_pool(const GridConfig &cfg) {
  if (control_modal_active().kind == ControlModalKind::IMAGE_CARD) {
    control_modal_close_active();
  }
  ImageCardCtx *contexts = image_card_contexts();
  int count = cfg.image_card_image_count;
  if (count > 4) count = 4;
  for (int i = 0; i < count; i++) {
    contexts[i].active = false;
    contexts[i].widget = nullptr;
    contexts[i].btn = nullptr;
    contexts[i].loading_widget = nullptr;
    contexts[i].loading_label = nullptr;
    contexts[i].icon_font = nullptr;
    contexts[i].entity_id.clear();
    contexts[i].base_url.clear();
    contexts[i].base_url_provider = nullptr;
    contexts[i].source_url.clear();
    contexts[i].url.clear();
    contexts[i].refresh_interval_ms = 0;
    contexts[i].next_refresh_ms = 0;
    contexts[i].retry_deadline_ms = 0;
    contexts[i].next_picture_retry_ms = 0;
    contexts[i].next_download_retry_ms = 0;
    contexts[i].width_compensation_percent = 100;
    contexts[i].requested_once = false;
    contexts[i].image_ready = false;
    contexts[i].timer_only = false;
    contexts[i].modal_fit = false;
    contexts[i].startup_download_errors = 0;
    contexts[i].image = cfg.image_card_images ? cfg.image_card_images[i] : nullptr;
    if (contexts[i].image) contexts[i].image->release();
  }
}

inline ImageCardCtx *acquire_image_card_context(const GridConfig &cfg) {
  ImageCardCtx *contexts = image_card_contexts();
  int count = cfg.image_card_image_count;
  if (count > 4) count = 4;
  for (int i = 0; i < count; i++) {
    if (!contexts[i].active && contexts[i].image) {
      contexts[i].active = true;
      image_card_bind_callbacks(&contexts[i]);
      return &contexts[i];
    }
  }
  return nullptr;
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
                                             esphome::artwork_image::ArtworkImage *image) {
  if (!image) return;
  lv_coord_t width = 0;
  lv_coord_t height = 0;
  if (!image_card_position_widget(btn, widget, &width, &height)) return;
  lv_obj_t *loading = image_card_loading_widget(widget);
  image_card_position_widget(btn, loading);
  image_card_refresh_loading_layout(loading);
  image->set_target_size(width, height);
  image->set_resize_mode(esphome::artwork_image::ImageResizeMode::COVER);
}

inline bool image_card_apply_modal_geometry(ImageCardCtx *ctx,
                                            lv_coord_t *target_width = nullptr,
                                            lv_coord_t *target_height = nullptr) {
  ImageCardModalUi &ui = image_card_modal_ui();
  if (!ctx || ui.active != ctx || !ui.panel || !ui.image_widget || !ctx->image) return false;
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
  int source_width = ctx->image->get_fixed_width();
  int source_height = ctx->image->get_fixed_height();
  uint32_t zoom = image_card_scale_for_size(width, height, source_width, source_height, ctx->modal_fit);
  lv_coord_t scaled_width = static_cast<lv_coord_t>(static_cast<int64_t>(source_width) * zoom / 256);
  lv_coord_t scaled_height = static_cast<lv_coord_t>(static_cast<int64_t>(source_height) * zoom / 256);
  lv_img_set_pivot(ui.image_widget, 0, 0);
  lv_img_set_zoom(ui.image_widget, zoom);
  lv_obj_set_pos(ui.image_widget, (width - scaled_width) / 2, (height - scaled_height) / 2);
  lv_obj_set_size(ui.image_widget, source_width, source_height);
#endif
  if (target_width) *target_width = width;
  if (target_height) *target_height = height;
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

inline void image_card_apply_active_geometry(ImageCardCtx *ctx) {
  if (!ctx || !ctx->image) return;
  if (image_card_modal_active_for(ctx) && image_card_apply_modal_geometry(ctx)) return;
  image_card_apply_widget_geometry(ctx->btn, ctx->widget, ctx->image);
}

inline void setup_image_card(BtnSlot &s) {
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
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

  lv_obj_t *loading = lv_obj_create(s.btn);
  lv_obj_set_size(loading, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(loading, lv_color_hex(DARK_BACKGROUND_TERTIARY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(loading, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(loading, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(loading, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(loading, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(loading, 16, LV_PART_MAIN);
  lv_obj_set_layout(loading, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(loading, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(loading, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(loading, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_clear_flag(loading, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(loading, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *loading_icon = lv_label_create(loading);
  if (s.icon_lbl) {
    const lv_font_t *font = lv_obj_get_style_text_font(s.icon_lbl, LV_PART_MAIN);
    if (font) lv_obj_set_style_text_font(loading_icon, font, LV_PART_MAIN);
  }
  lv_obj_set_style_text_color(loading_icon, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_opa(loading_icon, LV_OPA_70, LV_PART_MAIN);
  lv_label_set_text(loading_icon, IMAGE_CARD_LOADING_ICON);

  lv_obj_t *loading_label = lv_label_create(loading);
  if (s.text_lbl) {
    const lv_font_t *font = lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN);
    if (font) lv_obj_set_style_text_font(loading_label, font, LV_PART_MAIN);
  }
  lv_obj_set_style_text_color(loading_label, lv_color_hex(DARK_TEXT_SOFT), LV_PART_MAIN);
  lv_obj_set_style_text_opa(loading_label, LV_OPA_80, LV_PART_MAIN);
  lv_obj_set_style_text_align(loading_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_label_set_text(loading_label, espcontrol_i18n("Loading"));

  lv_obj_set_user_data(img, loading);
  lv_obj_set_user_data(s.sensor_container, img);
  image_card_sync_tile_corners(s.btn, img);
}

inline void image_card_align_label(lv_obj_t *label, lv_obj_t *btn) {
  if (!label || !btn) return;
  lv_coord_t pad = lv_obj_get_style_radius(btn, LV_PART_MAIN) + 4;
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, pad, pad);
  lv_obj_move_foreground(label);
}

inline bool image_card_entity_supported(const std::string &entity_id) {
  return entity_id.rfind("camera.", 0) == 0 || entity_id.rfind("image.", 0) == 0;
}

inline void subscribe_image_card_label(lv_obj_t *label, lv_obj_t *btn,
                                       const std::string &entity_id) {
  ha_subscribe_attribute(
    entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([label, btn](esphome::StringRef name) {
      lv_label_set_text(label, string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN).c_str());
      image_card_align_label(label, btn);
    })
  );
}

inline void image_card_configure_label(BtnSlot &s, const ParsedCfg &p) {
  if (!s.text_lbl) return;
  if (!image_card_label_enabled(p)) {
    lv_obj_add_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(s.text_lbl, lv_color_hex(DARK_OVERLAY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(s.text_lbl, LV_OPA_50, LV_PART_MAIN);
  lv_obj_set_style_radius(s.text_lbl, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_left(s.text_lbl, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_right(s.text_lbl, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_top(s.text_lbl, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(s.text_lbl, 4, LV_PART_MAIN);
  lv_label_set_long_mode(s.text_lbl, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(s.text_lbl, lv_pct(92));
  lv_label_set_text(s.text_lbl, (p.label.empty() ? p.entity : p.label).c_str());
  image_card_align_label(s.text_lbl, s.btn);
  if (p.label.empty() && !p.entity.empty()) {
    subscribe_image_card_label(s.text_lbl, s.btn, p.entity);
  }
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

inline std::string image_card_cache_bust_url(const std::string &url) {
  if (url.empty()) return "";
  std::string next = url;
  next += (next.find('?') == std::string::npos) ? "?time=" : "&time=";
  next += std::to_string(esphome::millis());
  return next;
}

inline bool image_card_query_has_param(const std::string &url, const std::string &param) {
  size_t query = url.find('?');
  if (query == std::string::npos) return false;
  std::string token = param + "=";
  size_t pos = url.find(token, query + 1);
  while (pos != std::string::npos) {
    if ((pos == query + 1 || url[pos - 1] == '&') &&
        (url.find('#') == std::string::npos || pos < url.find('#'))) {
      return true;
    }
    pos = url.find(token, pos + 1);
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
         url.find("/api/image_proxy/") != std::string::npos;
}

inline std::string image_card_sized_url(const std::string &url,
                                        lv_coord_t width,
                                        lv_coord_t height) {
  if (!image_card_home_assistant_proxy_url(url) || width <= 0 || height <= 0) {
    return url;
  }
  std::string next = image_card_append_query_param(url, "width", static_cast<int>(width));
  return image_card_append_query_param(next, "height", static_cast<int>(height));
}

inline void image_card_handle_picture(ImageCardCtx *ctx, esphome::StringRef picture);

inline void image_card_request_picture(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || ctx->entity_id.empty()) return;
  if (!ha_api_state_connected()) {
    if (image_card_startup_retry_active(ctx)) {
      ctx->next_picture_retry_ms = esphome::millis() + IMAGE_CARD_RETRY_INTERVAL_MS;
      image_card_set_loading_state(ctx, "Loading", true);
    }
    return;
  }
  bool requested = ha_get_attribute(
    ctx->entity_id,
    std::string("entity_picture"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef picture) {
        image_card_handle_picture(ctx, picture);
      })
  );
  if (!requested && image_card_startup_retry_active(ctx)) {
    ctx->next_picture_retry_ms = esphome::millis() + IMAGE_CARD_RETRY_INTERVAL_MS;
  }
}

inline void image_card_schedule_next_refresh(ImageCardCtx *ctx, uint32_t now = esphome::millis()) {
  if (!ctx || ctx->refresh_interval_ms == 0) {
    if (ctx) ctx->next_refresh_ms = 0;
    return;
  }
  ctx->next_refresh_ms = now + ctx->refresh_interval_ms;
}

inline void image_card_request_source_url(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !ctx->image || ctx->source_url.empty()) return;
  uint32_t now = esphome::millis();
  if (image_card_modal_active_for(ctx)) {
    ctx->next_download_retry_ms = now + IMAGE_CARD_MODAL_REFRESH_DELAY_MS;
    ESP_LOGD("image_card", "Deferring image refresh while modal is open for %s", ctx->entity_id.c_str());
    return;
  }
  lv_coord_t width = 0;
  lv_coord_t height = 0;
  esphome::artwork_image::ImageResizeMode resize_mode =
    esphome::artwork_image::ImageResizeMode::COVER;
  if (!image_card_position_widget(ctx->btn, ctx->widget, &width, &height)) return;
  lv_obj_t *loading = image_card_loading_widget(ctx->widget);
  image_card_position_widget(ctx->btn, loading);
  image_card_refresh_loading_layout(loading);
  ctx->url = image_card_cache_bust_url(image_card_sized_url(ctx->source_url, width, height));
  ctx->requested_once = true;
  ctx->next_download_retry_ms = 0;
  image_card_schedule_next_refresh(ctx, now);
  ctx->image->set_target_size(width, height);
  ctx->image->set_resize_mode(resize_mode);
  ESP_LOGI("image_card", "Downloading camera image for %s", ctx->entity_id.c_str());
  ctx->image->request_update_url(ctx->url);
}

inline void image_card_schedule_source_refresh(ImageCardCtx *ctx, uint32_t delay_ms,
                                               const char *reason) {
  if (!ctx || !ctx->active || ctx->source_url.empty()) return;
  ctx->next_download_retry_ms = esphome::millis() + delay_ms;
  ESP_LOGI("image_card", "Queued %s image refresh for %s in %lu ms",
           reason ? reason : "camera", ctx->entity_id.c_str(),
           static_cast<unsigned long>(delay_ms));
}

inline void image_card_hide_modal() {
  ImageCardModalUi &ui = image_card_modal_ui();
  ImageCardCtx *ctx = ui.active;
  image_card_clear_widget_source(ui.image_widget);
  control_modal_delete_overlay(ControlModalKind::IMAGE_CARD, ui.overlay);
  ui = ImageCardModalUi();
  if (ctx && ctx->active && ctx->image) {
    image_card_apply_widget_geometry(ctx->btn, ctx->widget, ctx->image);
    if (!ctx->source_url.empty()) image_card_schedule_source_refresh(ctx, IMAGE_CARD_MODAL_REFRESH_DELAY_MS, "tile");
  }
}

inline void image_card_open_modal(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !ctx->image || !ctx->image_ready) {
    ESP_LOGW("image_card", "No camera image is ready to open");
    return;
  }
  ESP_LOGI("image_card", "Opening image modal for %s", ctx->entity_id.c_str());
  ctx->next_download_retry_ms = 0;
  ctx->image->cancel_update();

  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::IMAGE_CARD, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, "\U000F0141", false, image_card_hide_modal);

  ImageCardModalUi &ui = image_card_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  control_modal_style_translucent_chrome_button(ui.back_btn);

  lv_obj_set_style_bg_color(ui.panel, lv_color_hex(DARK_OVERLAY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.panel, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_clip_corner(ui.panel, true, LV_PART_MAIN);

#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  ui.image_widget = lv_image_create(ui.panel);
#else
  ui.image_widget = lv_img_create(ui.panel);
#endif
  lv_obj_clear_flag(ui.image_widget, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.image_widget, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(ui.image_widget, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.image_widget, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.image_widget, LV_OPA_TRANSP, LV_PART_MAIN);
  image_card_set_widget_source(ui.image_widget, ctx->image);
  image_card_apply_modal_geometry(ctx);
  lv_obj_move_background(ui.image_widget);
  lv_obj_move_foreground(ui.back_btn);
  lv_obj_move_foreground(ui.overlay);
}

inline void image_card_handle_picture(ImageCardCtx *ctx, esphome::StringRef picture) {
  if (!ctx || !ctx->active || !ctx->image) return;
  std::string raw = string_ref_limited(picture, 4096);
  std::string url = image_card_join_url(image_card_base_url(ctx), raw);
  if (url.empty()) {
    ESP_LOGW("image_card", "No usable entity_picture URL for %s", ctx->entity_id.c_str());
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
  ctx->next_picture_retry_ms = 0;
  ctx->source_url = url;
  if (image_card_modal_active_for(ctx)) {
    image_card_schedule_source_refresh(ctx, IMAGE_CARD_MODAL_REFRESH_DELAY_MS, "deferred");
    return;
  }
  if (!ctx->requested_once || !ctx->timer_only || ctx->refresh_interval_ms == 0) {
    image_card_request_source_url(ctx);
  } else if (ctx->next_refresh_ms == 0) {
    image_card_schedule_next_refresh(ctx);
  }
}

inline void refresh_image_cards() {
  if (!ha_api_state_connected()) return;
  ImageCardCtx *contexts = image_card_contexts();
  uint32_t now = esphome::millis();
  for (int i = 0; i < 4; i++) {
    ImageCardCtx *ctx = &contexts[i];
    if (!ctx->active) continue;
    if (!ctx->image_ready) {
      ctx->retry_deadline_ms = now + IMAGE_CARD_STARTUP_RETRY_MS;
      image_card_set_loading_state(ctx, "Loading", true);
    }
    ctx->next_picture_retry_ms = 0;
    image_card_request_picture(ctx);
  }
}

inline void image_card_refresh_due() {
  ImageCardCtx *contexts = image_card_contexts();
  uint32_t now = esphome::millis();
  for (int i = 0; i < 4; i++) {
    ImageCardCtx *ctx = &contexts[i];
    if (!ctx->active || ctx->refresh_interval_ms == 0 ||
        ctx->source_url.empty() || !ctx->requested_once) {
      if (ctx->active && ctx->next_picture_retry_ms != 0 &&
          (int32_t)(now - ctx->next_picture_retry_ms) >= 0) {
        ctx->next_picture_retry_ms = 0;
        image_card_request_picture(ctx);
      }
      if (ctx->active && ctx->next_download_retry_ms != 0 &&
          (int32_t)(now - ctx->next_download_retry_ms) >= 0) {
        ctx->next_download_retry_ms = 0;
        image_card_request_source_url(ctx);
      }
      continue;
    }
    if (ctx->next_picture_retry_ms != 0 &&
        (int32_t)(now - ctx->next_picture_retry_ms) >= 0) {
      ctx->next_picture_retry_ms = 0;
      image_card_request_picture(ctx);
    }
    if (ctx->next_download_retry_ms != 0 &&
        (int32_t)(now - ctx->next_download_retry_ms) >= 0) {
      ctx->next_download_retry_ms = 0;
      image_card_request_source_url(ctx);
      continue;
    }
    if ((int32_t)(now - ctx->next_refresh_ms) >= 0) {
      image_card_request_source_url(ctx);
    }
  }
}

inline bool bind_image_card(BtnSlot &s, const ParsedCfg &p, const GridConfig &cfg,
                            bool bind_click_handler = false) {
  if (p.type != "image") return false;
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
  ImageCardCtx *ctx = acquire_image_card_context(cfg);
  if (!ctx) {
    ESP_LOGW("image_card", "No image card downloader available for %s", p.entity.c_str());
    return true;
  }
  ctx->widget = widget;
  ctx->btn = s.btn;
  ctx->loading_widget = loading;
  ctx->loading_label = image_card_loading_label(loading);
  ctx->icon_font = s.icon_lbl ? lv_obj_get_style_text_font(s.icon_lbl, LV_PART_MAIN) : nullptr;
  ctx->entity_id = p.entity;
  ctx->base_url = cfg.home_assistant_base_url ? cfg.home_assistant_base_url() : "";
  ctx->base_url_provider = cfg.home_assistant_base_url;
  ctx->refresh_interval_ms = image_card_refresh_interval_ms(p);
  ctx->timer_only = image_card_timer_only_refresh(p);
  ctx->modal_fit = image_card_modal_fit_enabled(p);
  ctx->retry_deadline_ms = esphome::millis() + IMAGE_CARD_STARTUP_RETRY_MS;
  ctx->width_compensation_percent = cfg.width_compensation_percent;
  image_card_apply_widget_geometry(ctx->btn, ctx->widget, ctx->image);
  image_card_set_loading_state(ctx, "Loading", true);
  lv_obj_set_user_data(s.btn, ctx);
  lv_obj_add_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  apply_push_button_transition(s.btn);
  if (bind_click_handler) {
    lv_obj_add_event_cb(s.btn, [](lv_event_t *e) {
      ImageCardCtx *ctx = static_cast<ImageCardCtx *>(lv_event_get_user_data(e));
      image_card_open_modal(ctx);
    }, LV_EVENT_CLICKED, ctx);
  }

  ha_subscribe_attribute(
    p.entity,
    std::string("entity_picture"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef picture) {
        image_card_handle_picture(ctx, picture);
      })
  );
  image_card_request_picture(ctx);
  return true;
}
