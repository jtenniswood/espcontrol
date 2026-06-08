#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

#include "esphome/core/version.h"

struct ImageCardCtx {
  lv_obj_t *widget = nullptr;
  lv_obj_t *btn = nullptr;
  const lv_font_t *icon_font = nullptr;
  esphome::artwork_image::ArtworkImage *image = nullptr;
  std::string entity_id;
  std::string source_url;
  std::string url;
  uint32_t refresh_interval_ms = 0;
  uint32_t next_refresh_ms = 0;
  int width_compensation_percent = 100;
  bool active = false;
  bool callbacks_bound = false;
  bool requested_once = false;
  bool timer_only = false;
  bool modal_fit = false;
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

inline void image_card_hide(ImageCardCtx *ctx) {
  if (!ctx) return;
  if (ctx->widget) lv_obj_add_flag(ctx->widget, LV_OBJ_FLAG_HIDDEN);
}

inline void image_card_apply_downloaded(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !ctx->widget || !ctx->image) return;
  if (ctx->image->get_url() != ctx->url) return;
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
  if (image_card_modal_active_for(ctx)) {
    return;
  } else {
    image_card_hide(ctx);
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
    contexts[i].icon_font = nullptr;
    contexts[i].entity_id.clear();
    contexts[i].source_url.clear();
    contexts[i].url.clear();
    contexts[i].refresh_interval_ms = 0;
    contexts[i].next_refresh_ms = 0;
    contexts[i].width_compensation_percent = 100;
    contexts[i].requested_once = false;
    contexts[i].timer_only = false;
    contexts[i].modal_fit = false;
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
  lv_obj_set_pos(ui.image_widget, 0, 0);
  lv_obj_set_size(ui.image_widget, width, height);
  lv_obj_set_style_radius(
    ui.image_widget, lv_obj_get_style_radius(ui.panel, LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_clip_corner(ui.image_widget, true, LV_PART_MAIN);
  ctx->image->set_target_size(width, height);
  ctx->image->set_resize_mode(ctx->modal_fit
    ? esphome::artwork_image::ImageResizeMode::FIT
    : esphome::artwork_image::ImageResizeMode::COVER);
  if (target_width) *target_width = width;
  if (target_height) *target_height = height;
  return true;
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
  lv_obj_set_style_radius(img, lv_obj_get_style_radius(s.btn, LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_clip_corner(img, true, LV_PART_MAIN);
  lv_obj_set_user_data(s.sensor_container, img);
}

inline void image_card_align_label(lv_obj_t *label, lv_obj_t *btn) {
  if (!label || !btn) return;
  lv_coord_t pad = lv_obj_get_style_radius(btn, LV_PART_MAIN) + 4;
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, pad, pad);
  lv_obj_move_foreground(label);
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

inline std::string image_card_cache_bust_url(const std::string &url) {
  if (url.empty()) return "";
  std::string next = url;
  next += (next.find('?') == std::string::npos) ? "?time=" : "&time=";
  next += std::to_string(esphome::millis());
  return next;
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
  ctx->url = image_card_cache_bust_url(ctx->source_url);
  ctx->requested_once = true;
  image_card_schedule_next_refresh(ctx, now);
  image_card_apply_active_geometry(ctx);
  ESP_LOGI("image_card", "Downloading camera image for %s", ctx->entity_id.c_str());
  ctx->image->request_update_url(ctx->url);
}

inline void image_card_hide_modal() {
  ImageCardModalUi &ui = image_card_modal_ui();
  ImageCardCtx *ctx = ui.active;
  control_modal_delete_overlay(ControlModalKind::IMAGE_CARD, ui.overlay);
  ui = ImageCardModalUi();
  if (ctx && ctx->active && ctx->image) {
    image_card_apply_widget_geometry(ctx->btn, ctx->widget, ctx->image);
    if (!ctx->source_url.empty()) image_card_request_source_url(ctx);
  }
}

inline void image_card_open_modal(ImageCardCtx *ctx) {
  if (!ctx || !ctx->active || !ctx->image || ctx->source_url.empty()) {
    ESP_LOGW("image_card", "No camera image is ready to open");
    return;
  }

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
  image_card_apply_modal_geometry(ctx);
  image_card_set_widget_source(ui.image_widget, ctx->image);
  lv_obj_move_background(ui.image_widget);
  lv_obj_move_foreground(ui.back_btn);
  lv_obj_move_foreground(ui.overlay);
  image_card_request_source_url(ctx);
}

inline void image_card_handle_picture(ImageCardCtx *ctx, const GridConfig &cfg,
                                      esphome::StringRef picture) {
  if (!ctx || !ctx->active || !ctx->image) return;
  std::string base = cfg.home_assistant_base_url ? cfg.home_assistant_base_url() : "";
  std::string raw = string_ref_limited(picture, 4096);
  std::string url = image_card_join_url(base, raw);
  if (url.empty()) {
    ESP_LOGW("image_card", "No usable entity_picture URL for %s", ctx->entity_id.c_str());
    image_card_hide(ctx);
    return;
  }
  ctx->source_url = url;
  if (!ctx->requested_once || !ctx->timer_only || ctx->refresh_interval_ms == 0) {
    image_card_request_source_url(ctx);
  } else if (ctx->next_refresh_ms == 0) {
    image_card_schedule_next_refresh(ctx);
  }
}

inline void image_card_refresh_due() {
  ImageCardCtx *contexts = image_card_contexts();
  uint32_t now = esphome::millis();
  for (int i = 0; i < 4; i++) {
    ImageCardCtx *ctx = &contexts[i];
    if (!ctx->active || ctx->refresh_interval_ms == 0 ||
        ctx->source_url.empty() || !ctx->requested_once) {
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
  if (p.entity.empty()) return true;
  if (p.entity.rfind("camera.", 0) != 0) {
    ESP_LOGW("image_card", "Image card only supports camera entities: %s", p.entity.c_str());
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
  ctx->icon_font = s.icon_lbl ? lv_obj_get_style_text_font(s.icon_lbl, LV_PART_MAIN) : nullptr;
  ctx->entity_id = p.entity;
  ctx->refresh_interval_ms = image_card_refresh_interval_ms(p);
  ctx->timer_only = image_card_timer_only_refresh(p);
  ctx->modal_fit = image_card_modal_fit_enabled(p);
  ctx->width_compensation_percent = cfg.width_compensation_percent;
  image_card_apply_widget_geometry(ctx->btn, ctx->widget, ctx->image);
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
      [ctx, cfg](esphome::StringRef picture) {
        image_card_handle_picture(ctx, cfg, picture);
      })
  );
  ha_get_attribute(
    p.entity,
    std::string("entity_picture"),
    std::function<void(esphome::StringRef)>(
      [ctx, cfg](esphome::StringRef picture) {
        image_card_handle_picture(ctx, cfg, picture);
      })
  );
  return true;
}
