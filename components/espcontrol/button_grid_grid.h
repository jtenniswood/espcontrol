#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif

// =============================================================================
// GRID BOOT PHASES - Consolidated on_boot logic for all devices
// =============================================================================
// Each sensors.yaml builds id()-based arrays and calls these three functions.
// Device-specific behavior is controlled by GridConfig fields.
// =============================================================================

struct GridConfig {
  int num_slots;
  int cols;
  bool width_compensation_vertical = false;
  bool wrap_tall_labels;
  bool info_only = false;
  bool subpage_chevrons_enabled = true;
  int width_compensation_percent = 100;
  int volume_width_compensation_percent = 100;
  int media_artwork_width_compensation_percent = 100;
  int label_lines = 0;
  int label_lines_tall = 0;
  int color_correction_red_percent = COLOR_CORRECTION_RED_PERCENT;
  int color_correction_green_percent = COLOR_CORRECTION_GREEN_PERCENT;
  int color_correction_blue_percent = COLOR_CORRECTION_BLUE_PERCENT;
  const lv_font_t *icon_font;
  const lv_font_t *sp_sensor_font;
  const lv_font_t *sp_large_sensor_font = nullptr;
  int large_sensor_unit_offset_percent = -10;
  const lv_font_t *media_title_font;
  const lv_font_t *media_control_title_font = nullptr;
  const lv_font_t *media_control_artist_font = nullptr;
  const lv_font_t *option_select_value_font = nullptr;
  const lv_font_t *volume_number_font;
  const lv_font_t *volume_label_font = nullptr;
  const lv_font_t *climate_card_icon_font = nullptr;
  const lv_font_t *climate_option_title_font = nullptr;
  const lv_font_t *climate_option_value_font = nullptr;
  const lv_font_t *volume_icon_font = nullptr;
  const lv_font_t *subpage_chevron_font = nullptr;
  int subpage_chevron_x = 0;
  int subpage_chevron_y = 2;
  int subpage_chevron_text_width_percent = 94;
  std::string temperature_unit;
  std::string timezone;
  std::function<void()> suspend_display_takeover;
  std::function<void()> resume_display_takeover;
  esphome::artwork_image::ArtworkImage **image_card_images = nullptr;
  esphome::artwork_image::ArtworkImage **image_card_modal_images = nullptr;
  int image_card_image_count = 0;
  bool image_card_diagnostics = false;
  std::function<std::string()> home_assistant_base_url;
};

#include "button_grid_image.h"

inline void grid_log_memory(const char *stage) {
#ifdef ESP_PLATFORM
  ESP_LOGI("sensors", "Phase 2 %s heap: internal=%u psram=%u",
    stage,
    (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
    (unsigned) heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
#else
  (void) stage;
#endif
}

inline DisplayProfile display_profile_from_grid_config(const GridConfig &cfg) {
  DisplayProfile profile;
  profile.fonts.icon = cfg.icon_font;
  profile.fonts.sensor = cfg.sp_sensor_font;
  profile.fonts.large_sensor = cfg.sp_large_sensor_font;
  profile.fonts.media_title = cfg.media_title_font;
  profile.fonts.media_control_title = cfg.media_control_title_font;
  profile.fonts.media_control_artist = cfg.media_control_artist_font;
  profile.fonts.option_select_value = cfg.option_select_value_font;
  profile.fonts.volume_number = cfg.volume_number_font;
  profile.fonts.volume_label = cfg.volume_label_font;
  profile.fonts.climate_card_icon = cfg.climate_card_icon_font;
  profile.fonts.climate_option_title = cfg.climate_option_title_font;
  profile.fonts.climate_option_value = cfg.climate_option_value_font;
  profile.fonts.volume_icon = cfg.volume_icon_font;
  profile.width.vertical_axis = cfg.width_compensation_vertical;
  profile.width.main_percent = cfg.width_compensation_percent;
  profile.width.volume_percent = cfg.volume_width_compensation_percent;
  profile.large_numbers.font = cfg.sp_large_sensor_font;
  profile.large_numbers.unit_offset_percent = cfg.large_sensor_unit_offset_percent;
  profile.color.red_percent = cfg.color_correction_red_percent;
  profile.color.green_percent = cfg.color_correction_green_percent;
  profile.color.blue_percent = cfg.color_correction_blue_percent;
  return profile;
}

inline void configure_grid_layout(lv_obj_t *page, int num_slots, int cols) {
  if (!page) return;
  int slot_count = bounded_grid_slots(num_slots);
  int col_count = cols > 0 ? cols : 1;
  if (col_count > MAX_GRID_SLOTS) col_count = MAX_GRID_SLOTS;
  int row_count = (slot_count + col_count - 1) / col_count;
  if (row_count < 1) row_count = 1;
  if (row_count > MAX_GRID_SLOTS) row_count = MAX_GRID_SLOTS;

  static lv_coord_t col_dsc[MAX_GRID_SLOTS + 1];
  static lv_coord_t row_dsc[MAX_GRID_SLOTS + 1];
  for (int i = 0; i < col_count; i++) col_dsc[i] = LV_GRID_FR(1);
  col_dsc[col_count] = LV_GRID_TEMPLATE_LAST;
  for (int i = 0; i < row_count; i++) row_dsc[i] = LV_GRID_FR(1);
  row_dsc[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_grid_dsc_array(page, col_dsc, row_dsc);
  lv_obj_update_layout(page);
}

struct CardPalette {
  bool has_on = false;
  bool has_off = false;
  bool has_sensor_color = false;
  uint32_t on_val = DEFAULT_SLIDER_COLOR;
  uint32_t off_val = SECONDARY_GREY;
  uint32_t sensor_val = TERTIARY_GREY;
};

inline lv_coord_t large_sensor_unit_offset_px(const lv_font_t *large_font, int percent) {
  if (!large_font || large_font->line_height <= 0) return 0;
  return large_font->line_height * percent / 100;
}

inline void apply_large_sensor_number_style(const BtnSlot &s, const lv_font_t *large_font,
                                            int unit_offset_percent) {
  if (s.sensor_lbl && large_font) {
    lv_obj_set_style_text_font(s.sensor_lbl, large_font, LV_PART_MAIN);
  }
  if (s.unit_lbl) {
    lv_obj_set_style_translate_y(
      s.unit_lbl, large_sensor_unit_offset_px(large_font, unit_offset_percent), LV_PART_MAIN);
  }
}

inline bool large_number_square_card_layout(int row_span, int col_span) {
  return card_span_is_large(row_span, col_span);
}

inline bool card_large_date_time_layout(const ParsedCfg &p, int row_span, int col_span) {
  if (p.type == "clock") {
    return large_number_square_card_layout(row_span, col_span) ||
           card_span_is_wide(row_span, col_span);
  }
  return large_number_square_card_layout(row_span, col_span);
}

inline bool card_large_numbers_active_for_layout(const ParsedCfg &p, int row_span, int col_span) {
  return card_large_numbers_supported(p) && !card_large_numbers_disabled(p) && (
    large_number_square_card_layout(row_span, col_span) ||
    card_large_numbers_enabled(p));
}

inline bool wide_large_date_time_card_layout(int row_span, int col_span) {
  return card_span_is_wide(row_span, col_span);
}

inline void apply_wide_large_date_time_card_layout(const BtnSlot &s,
                                                   lv_align_t align = LV_ALIGN_CENTER) {
  if (s.text_lbl) lv_obj_add_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);
  if (s.sensor_container) lv_obj_align(s.sensor_container, align, 0, 0);
}

inline lv_align_t wide_large_date_time_card_align(const ParsedCfg &p) {
  return (p.type == "clock" || (p.type == "calendar" && calendar_card_shows_time(p)))
    ? LV_ALIGN_LEFT_MID
    : LV_ALIGN_CENTER;
}

inline void apply_card_label_line_clamp(lv_obj_t *label, const GridConfig &cfg,
                                        int row_span = 1) {
  if (!label || cfg.label_lines <= 0) return;
  int lines = (row_span > 1 && cfg.label_lines_tall > 0)
    ? cfg.label_lines_tall
    : cfg.label_lines;
  if (lines <= 0) return;
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label, lv_pct(100));
  lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

inline bool card_slot_static_child(const BtnSlot &s, lv_obj_t *child) {
  return child == s.icon_lbl || child == s.sensor_container ||
         child == s.text_lbl || child == s.subpage_lbl;
}

inline void reset_card_slot_dynamic_children(BtnSlot &s) {
  if (!s.btn) return;
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_state(s.btn, LV_STATE_CHECKED);
  sync_card_checked_text_color(s.btn);
  lv_obj_clear_state(s.btn, LV_STATE_DISABLED);
  lv_obj_set_style_opa(s.btn, LV_OPA_COVER, LV_PART_MAIN);
  if (s.sensor_container) lv_obj_set_user_data(s.sensor_container, nullptr);
  if (s.text_lbl) {
    lv_obj_set_style_bg_opa(s.text_lbl, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s.text_lbl, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s.text_lbl, 0, LV_PART_MAIN);
  }
  int32_t count = static_cast<int32_t>(lv_obj_get_child_cnt(s.btn));
  for (int32_t i = count - 1; i >= 0; i--) {
    lv_obj_t *child = lv_obj_get_child(s.btn, i);
    if (!child || card_slot_static_child(s, child)) continue;
    lv_obj_del(child);
  }
}

inline bool info_only_hidden_card_type(const ParsedCfg &p) {
  if (p.type == "sensor" || p.type == "text_sensor" ||
      p.type == "door_window" || p.type == "presence" ||
      p.type == "calendar" || p.type == "clock" || p.type == "timezone" ||
      p.type == "weather" || p.type == "weather_forecast" || p.type == "image") {
    return false;
  }
  return true;
}

inline void media_cover_art_refresh_geometry(MediaNowPlayingCtx *ctx) {
  if (!ctx || !ctx->cover_art) return;
  image_card_refresh_tile_geometry(ctx->cover_art);
  if (ctx->cover_overlay) image_card_position_widget(ctx->cover_art->btn, ctx->cover_overlay);
  if (ctx->cover_art->widget) lv_obj_move_background(ctx->cover_art->widget);
  if (ctx->cover_overlay) lv_obj_move_foreground(ctx->cover_overlay);
  if (ctx->progress_slider) lv_obj_move_foreground(ctx->progress_slider);
  if (ctx->title_lbl) lv_obj_move_foreground(ctx->title_lbl);
  if (ctx->artist_lbl) lv_obj_move_foreground(ctx->artist_lbl);
}

inline void clear_media_cover_art(MediaNowPlayingCtx *ctx) {
  if (!ctx) return;
  if (ctx->cover_art) {
    lv_obj_t *widget = ctx->cover_art->widget;
    image_card_clear_media_artwork(ctx->cover_art);
    ctx->cover_art->active = false;
    ctx->cover_art->widget = nullptr;
    ctx->cover_art->btn = nullptr;
    ctx->cover_art->entity_id.clear();
    ctx->cover_art->base_url.clear();
    ctx->cover_art->base_url_provider = nullptr;
    ctx->cover_art->suspend_display_takeover = nullptr;
    ctx->cover_art->resume_display_takeover = nullptr;
    ctx->cover_art->diagnostics_enabled = false;
    ctx->cover_art->media_artwork = false;
    ctx->cover_art->media_overlay = nullptr;
    if (widget) lv_obj_del(widget);
    ctx->cover_art = nullptr;
  }
  if (ctx->cover_overlay) {
    lv_obj_del(ctx->cover_overlay);
    ctx->cover_overlay = nullptr;
  }
}

inline void setup_media_cover_art(BtnSlot &s, const ParsedCfg &p,
                                  const GridConfig &cfg) {
  if (!s.sensor_container) return;
  MediaNowPlayingCtx *media_ctx =
    static_cast<MediaNowPlayingCtx *>(lv_obj_get_user_data(s.sensor_container));
  if (!media_ctx || !media_ctx->btn) return;
  clear_media_cover_art(media_ctx);
  if (!media_cover_art_enabled(p) || p.entity.empty()) return;
  ImageCardCtx *art = acquire_image_card_context(cfg);
  if (!art) {
    ESP_LOGW("media_card", "No image downloader available for media cover art: %s",
             p.entity.c_str());
    return;
  }
  const bool image_only = card_runtime_media_mode(p.sensor) == "cover_art";
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  lv_obj_t *img = lv_image_create(media_ctx->btn);
#else
  lv_obj_t *img = lv_img_create(media_ctx->btn);
#endif
  lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(img, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(img, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(img, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(img, LV_OPA_TRANSP, LV_PART_MAIN);
  image_card_apply_tile_image_align(img);

  lv_obj_t *overlay = nullptr;
  if (!image_only) {
    overlay = lv_obj_create(media_ctx->btn);
    lv_obj_remove_style_all(overlay);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
  }

  art->widget = img;
  art->btn = media_ctx->btn;
  art->loading_widget = nullptr;
  art->loading_label = nullptr;
  art->icon_font = nullptr;
  art->label_font = nullptr;
  art->entity_id = p.entity;
  art->base_url = cfg.home_assistant_base_url ? cfg.home_assistant_base_url() : "";
  art->base_url_provider = cfg.home_assistant_base_url;
  art->suspend_display_takeover = cfg.suspend_display_takeover;
  art->resume_display_takeover = cfg.resume_display_takeover;
  art->refresh_interval_ms = 0;
  art->timer_only = false;
  art->modal_fit = false;
  art->media_artwork = true;
  art->media_overlay = overlay;
  art->pending_fallback_picture.clear();
  art->diagnostics_enabled = cfg.image_card_diagnostics;
  art->retry_deadline_ms = esphome::millis() + IMAGE_CARD_STARTUP_RETRY_MS;
  art->width_compensation_percent = cfg.width_compensation_percent;
  art->media_artwork_width_compensation_percent = cfg.media_artwork_width_compensation_percent;
  media_ctx->cover_art = art;
  media_ctx->cover_overlay = overlay;
  if (image_only && media_ctx->btn) lv_obj_set_user_data(media_ctx->btn, art);
  media_cover_art_refresh_geometry(media_ctx);
  image_card_log_diagnostics(art, "bind-media-artwork");
}

inline void subscribe_media_cover_art(MediaNowPlayingCtx *ctx,
                                      const std::string &entity_id) {
  if (!ctx || !ctx->cover_art || entity_id.empty()) return;
  ImageCardCtx *art = ctx->cover_art;
  const uint32_t generation = ha_subscription_generation();
  ha_subscribe_attribute(
    entity_id,
    std::string("entity_picture"),
    std::function<void(esphome::StringRef)>(
      [art, entity_id, generation](esphome::StringRef picture) {
        if (!image_card_context_current(art, entity_id, generation)) return;
        image_card_handle_media_artwork_picture(art, picture, false);
      })
  );
  ha_subscribe_attribute(
    entity_id,
    std::string("entity_picture_local"),
    std::function<void(esphome::StringRef)>(
      [art, entity_id, generation](esphome::StringRef picture) {
        if (!image_card_context_current(art, entity_id, generation)) return;
        image_card_handle_media_artwork_picture(art, picture, true);
      })
  );
  subscribe_image_card_access_token(art, entity_id);
  subscribe_image_card_entity_state(art, entity_id);
  image_card_request_media_artwork(art);
}

inline void setup_card_visual(BtnSlot &s, const ParsedCfg &p,
                              const GridConfig &cfg,
                              const CardPalette &palette,
                              int row_span = 1,
                              int col_span = 1) {
  const DisplayProfile display = display_profile_from_grid_config(cfg);
  const auto family = card_runtime_family(p.type);
  reset_card_slot_dynamic_children(s);
  apply_button_colors(s.btn, palette.has_on, palette.on_val,
    palette.has_off, palette.off_val);
  apply_button_on_pattern(s.btn, p.options, palette.has_on, palette.on_val);
  if (s.sensor_lbl && display_sensor_font(display)) {
    lv_obj_set_style_text_font(s.sensor_lbl, display_sensor_font(display), LV_PART_MAIN);
  }
  if (s.unit_lbl) lv_obj_set_style_translate_y(s.unit_lbl, 0, LV_PART_MAIN);
  if (s.unit_lbl) lv_obj_clear_flag(s.unit_lbl, LV_OBJ_FLAG_HIDDEN);
  if (s.text_lbl) lv_obj_clear_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);
  if (s.icon_lbl) lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);
  if (s.sensor_container) lv_obj_align(s.sensor_container, LV_ALIGN_TOP_LEFT, 0, 0);
  if (s.text_lbl) lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  set_subpage_chevron_visible(
    s, family == espcontrol::cards::Family::SUBPAGE && cfg.subpage_chevrons_enabled,
    cfg.subpage_chevron_x, cfg.subpage_chevron_y,
    cfg.subpage_chevron_text_width_percent);

  if (cfg.info_only && info_only_hidden_card_type(p)) {
    lv_obj_add_flag(s.btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
    return;
  }

  screen_lock_register_controlled_button(s.btn);

  if (family == espcontrol::cards::Family::IMAGE) {
    setup_image_card(s);
    return;
  }
  if (family == espcontrol::cards::Family::SCREEN_LOCK) {
    setup_screen_lock_card(s, p);
    return;
  }

  if (sensor_card_local_sensor(p)) {
    if (p.entity.empty()) return;
    setup_local_sensor_card(s, p, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (is_text_sensor_card(p)) {
    setup_text_sensor_card(s, p, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (family == espcontrol::cards::Family::SENSOR) {
    if (p.sensor.empty()) return;
    setup_sensor_card(s, p, palette.has_sensor_color, palette.sensor_val);
    if (large_number_square_card_layout(row_span, col_span) &&
        card_large_numbers_active_for_layout(p, row_span, col_span) &&
        display_large_sensor_font(display)) {
      apply_large_sensor_number_style(
        s, display_large_sensor_font(display), display_large_sensor_unit_offset_percent(display));
    }
    return;
  }
  if (p.type == "door_window") {
    if (p.sensor.empty()) return;
    setup_door_window_card(s, p, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (p.type == "presence") {
    if (p.sensor.empty()) return;
    setup_presence_card(s, p, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (p.type == "calendar") {
    setup_calendar_card(s, p, palette.has_sensor_color, palette.sensor_val);
    if (card_large_date_time_layout(p, row_span, col_span) &&
        card_large_numbers_active_for_layout(p, row_span, col_span) &&
        display_large_sensor_font(display)) {
      apply_large_sensor_number_style(
        s, display_large_sensor_font(display), display_large_sensor_unit_offset_percent(display));
      if (wide_large_date_time_card_layout(row_span, col_span)) {
        apply_wide_large_date_time_card_layout(s, wide_large_date_time_card_align(p));
      }
    }
    return;
  }
  if (p.type == "clock") {
    setup_clock_card(s, p, palette.has_sensor_color, palette.sensor_val);
    if (card_large_date_time_layout(p, row_span, col_span) &&
        card_large_numbers_active_for_layout(p, row_span, col_span) &&
        display_large_sensor_font(display)) {
      apply_large_sensor_number_style(
        s, display_large_sensor_font(display), display_large_sensor_unit_offset_percent(display));
      if (wide_large_date_time_card_layout(row_span, col_span)) {
        apply_wide_large_date_time_card_layout(s, wide_large_date_time_card_align(p));
      }
    }
    return;
  }
  if (p.type == "timezone") {
    setup_timezone_card(s, p, palette.has_sensor_color, palette.sensor_val);
    if (card_large_date_time_layout(p, row_span, col_span) &&
        card_large_numbers_active_for_layout(p, row_span, col_span) &&
        display_large_sensor_font(display)) {
      apply_large_sensor_number_style(
        s, display_large_sensor_font(display), display_large_sensor_unit_offset_percent(display));
      if (wide_large_date_time_card_layout(row_span, col_span)) {
        apply_wide_large_date_time_card_layout(s);
      }
    }
    return;
  }
  if (weather_card_shows_forecast(p)) {
    setup_weather_forecast_card(s, p, palette.has_sensor_color, palette.sensor_val,
      display_main_width_percent(display));
    if (large_number_square_card_layout(row_span, col_span) &&
        card_large_numbers_active_for_layout(p, row_span, col_span) &&
        display_large_sensor_font(display)) {
      apply_large_sensor_number_style(
        s, display_large_sensor_font(display), display_large_sensor_unit_offset_percent(display));
    }
    return;
  }
  if (family == espcontrol::cards::Family::WEATHER) {
    setup_weather_card(s, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (p.type == "garage") {
    setup_garage_card(s, p);
    return;
  }
  if (p.type == "gate") {
    setup_gate_card(s, p);
    return;
  }
  if (subpage_parent_sensor_state_enabled(p)) {
    setup_subpage_parent_state_card(
      s, p, display_sensor_font(display), cfg.subpage_chevrons_enabled,
      cfg.subpage_chevron_x, cfg.subpage_chevron_y,
      cfg.subpage_chevron_text_width_percent);
    return;
  }
  if (p.type == "lock") {
    setup_lock_card(s, p);
    return;
  }
  if (family == espcontrol::cards::Family::ALARM) {
    setup_alarm_card(s, p);
    return;
  }
  if (family == espcontrol::cards::Family::ALARM_ACTION) {
    setup_alarm_action_card(s, p);
    return;
  }
  if (fan_non_speed_card_type(p.type)) {
    setup_fan_card(s, p);
    return;
  }
  if (p.type == "fan_control") {
    setup_fan_control_card(s, p);
    return;
  }
  if (family == espcontrol::cards::Family::COVER && cover_modal_mode(p.sensor)) {
    setup_cover_modal_card(s, p);
    return;
  }
  if (family == espcontrol::cards::Family::COVER && cover_command_mode(p.sensor)) {
    setup_cover_command_card(s, p);
    return;
  }
  if (family == espcontrol::cards::Family::COVER && cover_toggle_mode(p.sensor)) {
    setup_cover_toggle_card(s, p);
    return;
  }
  if (family == espcontrol::cards::Family::INTERNAL) {
    setup_internal_relay_card(s, p);
    return;
  }
  if (family == espcontrol::cards::Family::ACTION &&
      (p.type == "local" || action_card_local_action(p))) {
    setup_local_action_card(s, p);
    return;
  }
  if (family == espcontrol::cards::Family::LOCAL_SENSOR || sensor_card_local_sensor(p)) {
    if (p.entity.empty()) return;
    setup_local_sensor_card(s, p, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (family == espcontrol::cards::Family::ACTION) {
    if (action_card_option_select(p)) {
      setup_option_select_card(
        s, p, palette.has_sensor_color, palette.sensor_val,
        display_option_select_value_font_or(
          display, s.text_lbl ? lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN) : nullptr));
      return;
    }
    setup_action_card(s, p);
    return;
  }
  if (family == espcontrol::cards::Family::VACUUM) {
    setup_vacuum_card(s, p);
    return;
  }
  if (family == espcontrol::cards::Family::MOWER) {
    setup_lawn_mower_card(s, p);
    return;
  }
  if (family == espcontrol::cards::Family::OPTION_SELECT) {
    setup_option_select_card(
      s, p, palette.has_sensor_color, palette.sensor_val,
      display_option_select_value_font_or(
        display, s.text_lbl ? lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN) : nullptr));
    return;
  }
  if (family == espcontrol::cards::Family::TODO) {
    setup_todo_card(s, p, palette.off_val);
    if (large_number_square_card_layout(row_span, col_span) &&
        card_large_numbers_active_for_layout(p, row_span, col_span) &&
        display_large_sensor_font(display)) {
      apply_large_sensor_number_style(
        s, display_large_sensor_font(display), display_large_sensor_unit_offset_percent(display));
    }
    return;
  }
  if (family == espcontrol::cards::Family::MEDIA) {
    setup_media_card(s, p,
      palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR,
      palette.off_val,
      palette.sensor_val,
      display_sensor_font(display),
      display_media_title_font(display),
      display_main_width_percent(display),
      row_span, col_span);
    return;
  }
  if (family == espcontrol::cards::Family::CLIMATE) {
    setup_climate_control_button(
      s.btn, s.icon_lbl, s.sensor_container, s.sensor_lbl, s.unit_lbl,
      s.text_lbl, p, display_icon_font(display));
    return;
  }
  if (family == espcontrol::cards::Family::LIGHT_CONTROL) {
    setup_light_control_card(s, p);
    return;
  }
  if (espcontrol::cards::uses_slider_visual(p.type)) {
    setup_slider_visual(s, p, palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR);
    return;
  }
  if (family == espcontrol::cards::Family::LIGHT_TEMPERATURE) {
    setup_light_temp_visual(s, p, palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR);
    return;
  }
  setup_toggle_visual(s, p);
}

inline bool bind_basic_sensor_card(BtnSlot &s, const ParsedCfg &p,
                                   const CardPalette &palette) {
  if (sensor_card_local_sensor(p)) return false;
  if (is_text_sensor_card(p)) {
    if (!p.sensor.empty())
      subscribe_sensor_text_card_value(s.text_lbl, p, s.btn,
        sensor_active_color_enabled(p), palette.on_val, palette.sensor_val);
    return true;
  }
  if (p.type == "sensor") {
    if (!p.sensor.empty()) {
      if (p.precision == "icon") {
        subscribe_sensor_icon_state(s.btn, s.icon_lbl, p);
      } else {
        subscribe_sensor_value(s.sensor_lbl, p.sensor, parse_precision(p.precision),
          s.unit_lbl, p.unit, s.btn,
          sensor_active_color_enabled(p), palette.on_val, palette.sensor_val);
      }
      if (p.label.empty())
        subscribe_friendly_name(s.text_lbl, p.sensor);
    }
    return true;
  }
  if (p.type == "door_window") {
    if (!p.sensor.empty()) {
      subscribe_door_window_state(s.btn, s.icon_lbl, p.sensor,
        door_window_closed_icon(p), door_window_open_icon(p),
        door_window_active_color_enabled(p), palette.on_val, palette.sensor_val);
      if (p.label.empty())
        subscribe_friendly_name(s.text_lbl, p.sensor);
    }
    return true;
  }
  if (p.type == "presence") {
    if (!p.sensor.empty()) {
      subscribe_presence_state(s.btn, s.icon_lbl, p.sensor,
        presence_clear_icon(p), presence_detected_icon(p),
        presence_active_color_enabled(p), palette.on_val, palette.sensor_val);
      if (p.label.empty())
        subscribe_friendly_name(s.text_lbl, p.sensor);
    }
    return true;
  }
  return false;
}

inline bool bind_passive_card_sources(BtnSlot &s, const ParsedCfg &p) {
  if (p.type == "calendar") {
    subscribe_calendar_date_source(p.entity);
    return true;
  }
  if (p.type == "clock" || p.type == "timezone" || weather_card_shows_forecast(p)) {
    return true;
  }
  if (p.type == "weather") {
    if (!p.entity.empty())
      subscribe_weather_state(s.icon_lbl, s.text_lbl, p.entity);
    return true;
  }
  return false;
}

inline bool bind_garage_status_card(BtnSlot &s, const ParsedCfg &p,
                                    TransientStatusLabel **status_label_out = nullptr) {
  if (p.type != "garage" || p.entity.empty()) {
    return false;
  }
  bool show_status = garage_card_show_status(p);
  std::string fallback_label = p.label.empty() ? espcontrol_i18n(std::string("Garage Door")) : p.label;
  TransientStatusLabel *status_label = create_transient_status_label(
    s.text_lbl, show_status ? "--" : fallback_label);
  if (status_label_out != nullptr) *status_label_out = status_label;
  subscribe_garage_state(s.btn, s.icon_lbl, status_label,
    garage_closed_icon(p.icon), garage_open_icon(p.icon_on), p.entity, show_status);
  if (!show_status && p.label.empty())
    subscribe_friendly_name(status_label, p.entity);
  return true;
}

inline bool bind_gate_status_card(BtnSlot &s, const ParsedCfg &p,
                                  TransientStatusLabel **status_label_out = nullptr) {
  if (p.type != "gate" || p.entity.empty()) {
    return false;
  }
  bool show_status = gate_card_show_status(p);
  std::string fallback_label = p.label.empty() ? espcontrol_i18n(std::string("Gate")) : p.label;
  TransientStatusLabel *status_label = create_transient_status_label(
    s.text_lbl, show_status ? "--" : fallback_label);
  if (status_label_out != nullptr) *status_label_out = status_label;
  subscribe_gate_state(s.btn, s.icon_lbl, status_label,
    gate_closed_icon(p.icon), gate_open_icon(p.icon_on), p.entity, show_status);
  if (!show_status && p.label.empty())
    subscribe_friendly_name(status_label, p.entity);
  return true;
}

inline LockCardCtx *bind_lock_status_card(BtnSlot &s, const ParsedCfg &p,
                                          TransientStatusLabel **status_label_out = nullptr) {
  if (p.type != "lock" || p.entity.empty() || lock_command_mode(p.sensor)) {
    return nullptr;
  }
  LockCardCtx *ctx = new LockCardCtx();
  ctx->entity_id = p.entity;
  lv_obj_set_user_data(s.btn, ctx);
  std::string fallback_label = p.label.empty() ? espcontrol_i18n(std::string("Lock")) : p.label;
  TransientStatusLabel *status_label = create_transient_status_label(
    s.text_lbl, fallback_label);
  if (status_label_out != nullptr) *status_label_out = status_label;
  subscribe_lock_state(s.btn, s.icon_lbl, status_label,
    lock_locked_icon(p.icon), lock_unlocked_icon(p.icon_on), ctx);
  if (p.label.empty())
    subscribe_friendly_name(status_label, p.entity);
  return ctx;
}

inline MediaControlCtx *grid_media_control_runtime_for_owner(lv_obj_t *owner);

inline void refresh_media_card_layout(BtnSlot &s, const ParsedCfg &p,
                                      const GridConfig &cfg,
                                      int row_span = 1) {
  const DisplayProfile display = display_profile_from_grid_config(cfg);
  std::string mode = media_card_mode(p.sensor);
  lv_coord_t pad = lv_obj_get_style_radius(s.btn, LV_PART_MAIN) + 4;

  if (mode == "cover_art") {
    MediaNowPlayingCtx *ctx = (MediaNowPlayingCtx *)lv_obj_get_user_data(s.sensor_container);
    if (!ctx) return;
    if (s.icon_lbl) lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    if (s.text_lbl) {
      lv_label_set_text(s.text_lbl, "");
      lv_obj_add_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    if (ctx->title_lbl) lv_obj_add_flag(ctx->title_lbl, LV_OBJ_FLAG_HIDDEN);
    if (ctx->artist_lbl) lv_obj_add_flag(ctx->artist_lbl, LV_OBJ_FLAG_HIDDEN);
    media_cover_art_refresh_geometry(ctx);
    return;
  }

  if (mode == "now_playing") {
    MediaNowPlayingCtx *ctx = (MediaNowPlayingCtx *)lv_obj_get_user_data(s.sensor_container);
    if (!ctx) return;
    if (ctx->title_lbl) display_apply_main_width(ctx->title_lbl, display);
    if (ctx->artist_lbl) display_apply_main_width(ctx->artist_lbl, display);
    setup_media_now_playing_layout(
      s.btn, s.icon_lbl, ctx->title_lbl, ctx->artist_lbl,
      display_media_title_font(display), pad,
      row_span == 1, ctx->play_pause_background,
      ctx->progress_slider ? pad : 0, false);
    media_cover_art_refresh_geometry(ctx);
    if (ctx->progress_slider) slider_refresh_geometry(ctx->progress_slider);
    return;
  }

  if (mode == "position") {
    lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
    SliderCtx *ctx = slider ? (SliderCtx *)lv_obj_get_user_data(slider) : nullptr;
    lv_coord_t position_pad = ctx && ctx->content_pad > 0
      ? ctx->content_pad
      : lv_obj_get_style_pad_top(s.btn, LV_PART_MAIN);
    if (ctx && ctx->media_value_lbl) {
      display_apply_main_width(ctx->media_value_lbl, display);
      lv_obj_align(ctx->media_value_lbl, LV_ALIGN_TOP_LEFT, position_pad, position_pad);
      lv_obj_move_foreground(ctx->media_value_lbl);
    }
    if (s.text_lbl) {
      lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, position_pad, -position_pad);
      configure_button_label_wrap(s.text_lbl);
      lv_obj_move_foreground(s.text_lbl);
    }
    if (slider) slider_refresh_geometry(slider);
    if (ctx) {
      media_apply_position(ctx);
      media_schedule_position_refresh(ctx);
    }
    return;
  }

  if (media_playback_button_mode(mode)) {
    if (s.icon_lbl) lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);
    if (s.text_lbl) lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    return;
  }
  if (mode == "control_modal") {
    MediaControlCtx *ctx = grid_media_control_runtime_for_owner(s.btn);
    setup_media_control_button(
      s.btn, s.icon_lbl, s.sensor_container, s.sensor_lbl, s.unit_lbl, s.text_lbl, p);
    if (s.btn) lv_obj_set_user_data(s.btn, ctx);
    if (ctx) media_control_refresh_parent_card(ctx);
    return;
  }
  if (mode == "volume") return;

  lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
  if (slider) slider_refresh_geometry(slider);
}

inline void refresh_slider_card_layout(BtnSlot &s) {
  lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
  lv_coord_t pad = lv_obj_get_style_radius(s.btn, LV_PART_MAIN) + 4;
  if (s.icon_lbl) lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
  if (s.text_lbl) lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);
  if (slider) slider_refresh_geometry(slider);
}

inline void refresh_card_layout(BtnSlot &s, const ParsedCfg &p,
                                const GridConfig &cfg,
                                int row_span = 1) {
  const DisplayProfile display = display_profile_from_grid_config(cfg);
  if (cfg.label_lines > 0) {
    apply_card_label_line_clamp(s.text_lbl, cfg, row_span);
  } else if (cfg.wrap_tall_labels && row_span > 1) {
    lv_label_set_long_mode(s.text_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s.text_lbl, lv_pct(100));
  }
  display_apply_main_width(s.icon_lbl, display);
  display_apply_slot_text_width(s, display);
  if (p.type == "subpage") {
    set_subpage_chevron_visible(
      s, cfg.subpage_chevrons_enabled, cfg.subpage_chevron_x,
      cfg.subpage_chevron_y, cfg.subpage_chevron_text_width_percent);
  }

  if (p.type == "image") {
    ImageCardCtx *ctx = s.btn
      ? static_cast<ImageCardCtx *>(lv_obj_get_user_data(s.btn))
      : nullptr;
    if (ctx && ctx->active) {
      image_card_refresh_tile_geometry(ctx);
    } else {
      lv_obj_t *widget = s.sensor_container
        ? static_cast<lv_obj_t *>(lv_obj_get_user_data(s.sensor_container))
        : nullptr;
      if (widget) {
        image_card_position_widget(s.btn, widget);
        lv_obj_t *loading = image_card_loading_widget(widget);
        image_card_position_widget(s.btn, loading);
        image_card_refresh_loading_layout(loading);
      }
    }
    if (s.text_lbl && !lv_obj_has_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN)) {
      image_card_align_label_stack(s.text_lbl, s.btn);
    }
    if (s.icon_lbl && !lv_obj_has_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN)) {
      image_card_align_icon(s.icon_lbl, s.btn);
    }
  } else if (p.type == "media") {
    refresh_media_card_layout(s, p, cfg, row_span);
  } else if (brightness_slider_type(p.type) || p.type == "light_temperature" ||
             (p.type == "cover" && !cover_modal_mode(p.sensor) &&
              !cover_command_mode(p.sensor) && !cover_toggle_mode(p.sensor))) {
    refresh_slider_card_layout(s);
  }
}

inline void grid_refresh_layout(
    BtnSlot *slots, const GridConfig &cfg,
    const std::string &order_str,
    lv_obj_t *main_page_obj = nullptr) {
  ESP_LOGI("sensors", "Grid refresh: layout start (%lu ms)", esphome::millis());
  set_display_temperature_unit(cfg.temperature_unit, cfg.timezone);
  const DisplayProfile display = display_profile_from_grid_config(cfg);
  display_set_width_axis(display);
  int NS = bounded_grid_slots(cfg.num_slots);
  int COLS = cfg.cols > 0 ? cfg.cols : 1;
  // When the grid shape changes, LVGL can otherwise lay out children that
  // still point at now-invalid cells from the previous descriptor.
  for (int i = 0; i < NS; i++)
    lv_obj_add_flag(slots[i].btn, LV_OBJ_FLAG_HIDDEN);
  configure_grid_layout(main_page_obj, NS, COLS);
  int ROWS = (NS + COLS - 1) / COLS;

  OrderResult parsed, order;
  parse_order_string(order_str, NS, parsed);
  clear_spanned_cells(parsed, NS, COLS, order);
  clock_bar_clear_responsive_grid_cards(main_page_obj);
  navigation_clear_home_targets();

  lv_obj_t *first_card = nullptr;
  if (parsed.positions[0] >= 1 && parsed.positions[0] <= NS) {
    first_card = slots[parsed.positions[0] - 1].btn;
  } else if (NS > 0) {
    first_card = slots[0].btn;
  }
  set_media_home_grid_metrics(main_page_obj, COLS, ROWS, first_card);

  for (int pos = 0; pos < NS; pos++) {
    int idx = order.positions[pos];
    if (idx < 1 || idx > NS) continue;
    auto &s = slots[idx - 1];
    lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_HIDDEN);
    int col = pos % COLS, row = pos / COLS;
    int row_span = order.row_span[idx - 1] > 0 ? order.row_span[idx - 1] : 1;
    int col_span = order.col_span[idx - 1] > 0 ? order.col_span[idx - 1] : 1;
    set_grid_card_cell(s.btn, main_page_obj, col, row, col_span, row_span, COLS, ROWS);
  }

  if (main_page_obj) lv_obj_update_layout(main_page_obj);

  for (int pos = 0; pos < NS; pos++) {
    int idx = order.positions[pos];
    if (idx < 1 || idx > NS) continue;
    auto &s = slots[idx - 1];
    ParsedCfg p = parse_cfg(s.config->state);
    navigation_register_home_target(idx, pos, p.label, s.config->state, s.btn);
    int row_span = order.row_span[idx - 1] > 0 ? order.row_span[idx - 1] : 1;
    refresh_card_layout(s, p, cfg, row_span);
    if (p.type == "vacuum") {
      refresh_vacuum_card_translated_text(
        s.text_lbl, static_cast<VacuumCardCtx *>(lv_obj_get_user_data(s.btn)), p);
    }
  }
  refresh_subpage_vacuum_card_translated_text();
  ESP_LOGI("sensors", "Grid refresh: layout done (%lu ms)", esphome::millis());
}

// ── Phase 1: Visual setup ────────────────────────────────────────────

inline void grid_phase1(
    BtnSlot *slots, const GridConfig &cfg,
    const std::string &order_str,
    const std::string &on_hex,
    lv_obj_t *main_page_obj = nullptr) {
  ESP_LOGI("sensors", "Phase 1: visual setup start (%lu ms)", esphome::millis());
  set_backlight_display_takeover_callback(navigation_close_modals_for_display_takeover);
  set_display_temperature_unit(cfg.temperature_unit, cfg.timezone);
  const DisplayProfile display = display_profile_from_grid_config(cfg);
  display_set_width_axis(display);
  // Clear image references before visual setup removes their old LVGL widgets.
  reset_image_card_pool(cfg);
  int NS = bounded_grid_slots(cfg.num_slots);
  int COLS = cfg.cols > 0 ? cfg.cols : 1;
  if (COLS > MAX_GRID_SLOTS) COLS = MAX_GRID_SLOTS;
  for (int i = 0; i < NS; i++)
    lv_obj_add_flag(slots[i].btn, LV_OBJ_FLAG_HIDDEN);
  configure_grid_layout(main_page_obj, NS, COLS);
  int ROWS = (NS + COLS - 1) / COLS;
  if (NS != cfg.num_slots) {
    ESP_LOGW("sensors", "Grid slot count %d exceeds max %d; ignoring extra slots",
      cfg.num_slots, MAX_GRID_SLOTS);
  }

  if (!order_str.empty()) {
    bool all_empty = true;
    for (int i = 0; i < NS; i++) {
      if (!slots[i].config->state.empty()) { all_empty = false; break; }
    }
    if (all_empty) {
      ESP_LOGW("sensors", "Button order is set but all configs are empty. "
        "If upgrading from the old per-field format, export your config "
        "from the old firmware's web UI and import it after upgrading.");
    }
  }

  OrderResult parsed, order;
  parse_order_string(order_str, NS, parsed);
  clear_spanned_cells(parsed, NS, COLS, order);
  clock_bar_clear_responsive_grid_cards(main_page_obj);

  bool has_on;
  uint32_t on_val = parse_hex_color(on_hex, has_on);
  uint32_t off_val = display_correct_color(DEFAULT_SECONDARY_COLOR_RAW, display);
  uint32_t sensor_val = display_correct_color(DEFAULT_TERTIARY_COLOR_RAW, display);
  if (has_on) on_val = display_correct_color(on_val, display);

  CardPalette palette;
  palette.has_on = has_on;
  palette.has_off = true;
  palette.has_sensor_color = true;
  palette.on_val = has_on ? on_val : DEFAULT_SLIDER_COLOR;
  palette.off_val = off_val;
  palette.sensor_val = sensor_val;
  set_current_button_primary_color(palette.on_val);

  bump_ha_subscription_generation();
  reset_calendar_cards();
  reset_timezone_cards();
  weather_forecast_cancel_pending_requests();
  reset_weather_forecast_cards();
  reset_climate_control_refs();
  screen_lock_reset_registry();

  for (int pos = 0; pos < NS; pos++) {
    int idx = order.positions[pos];
    if (idx < 1 || idx > NS) continue;
    auto &s = slots[idx - 1];
    std::string scfg = s.config->state;
    lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_HIDDEN);
    int col = pos % COLS, row = pos / COLS;
    int row_span = order.row_span[idx - 1] > 0 ? order.row_span[idx - 1] : 1;
    int col_span = order.col_span[idx - 1] > 0 ? order.col_span[idx - 1] : 1;
    set_grid_card_cell(s.btn, main_page_obj, col, row, col_span, row_span, COLS, ROWS);

    if (cfg.wrap_tall_labels && row_span > 1) {
      lv_label_set_long_mode(s.text_lbl, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(s.text_lbl, lv_pct(100));
    }

    ParsedCfg p = parse_cfg(scfg);
    display_apply_main_width(s.icon_lbl, display);
    display_apply_slot_text_width(s, display);
    setup_card_visual(s, p, cfg, palette, row_span, col_span);
    refresh_card_layout(s, p, cfg, row_span);
  }
  screen_lock_apply();
  ESP_LOGI("sensors", "Phase 1: done (%lu ms)", esphome::millis());
}

// ── Phase 2: HA subscriptions + subpage creation ─────────────────────

inline std::string optional_text_state(esphome::text::Text **configs, int index) {
  return (configs != nullptr && configs[index] != nullptr) ? configs[index]->state : "";
}

template<typename T>
inline T *grid_delete_with_owner(lv_obj_t *owner, T *ptr) {
  if (owner != nullptr && ptr != nullptr) {
    lv_obj_add_event_cb(owner, [](lv_event_t *e) {
      delete static_cast<T *>(lv_event_get_user_data(e));
    }, LV_EVENT_DELETE, ptr);
  }
  return ptr;
}

inline void grid_delete_alarm_card_runtime_ptr(void *ptr);

inline AlarmActionCtx *grid_delete_alarm_action_with_owner(lv_obj_t *owner,
                                                           AlarmActionCtx *ctx) {
  if (owner != nullptr && ctx != nullptr) {
    lv_obj_add_event_cb(owner, [](lv_event_t *e) {
      AlarmActionCtx *action = static_cast<AlarmActionCtx *>(lv_event_get_user_data(e));
      if (action != nullptr) {
        grid_delete_alarm_card_runtime_ptr(action->card);
        delete action;
      }
    }, LV_EVENT_DELETE, ctx);
  }
  return ctx;
}

inline AlarmCardCtx *grid_delete_alarm_card_with_owner(lv_obj_t *owner,
                                                       AlarmCardCtx *ctx) {
  if (owner != nullptr && ctx != nullptr) {
    lv_obj_add_event_cb(owner, [](lv_event_t *e) {
      grid_delete_alarm_card_runtime_ptr(lv_event_get_user_data(e));
    }, LV_EVENT_DELETE, ctx);
  }
  return ctx;
}

struct GridRuntimeAllocation {
  lv_obj_t *owner = nullptr;
  void *ptr = nullptr;
  void (*deleter)(void *) = nullptr;
};

inline std::vector<GridRuntimeAllocation> &grid_runtime_allocations() {
  static std::vector<GridRuntimeAllocation> allocations;
  return allocations;
}

template<typename T>
inline void grid_delete_runtime_ptr(void *ptr) {
  delete static_cast<T *>(ptr);
}

inline void grid_delete_transient_status_label(TransientStatusLabel *ctx) {
  if (ctx != nullptr) {
    if (ctx->revert_timer != nullptr) {
      lv_timer_del(ctx->revert_timer);
      ctx->revert_timer = nullptr;
    }
    delete ctx;
  }
}

inline void grid_delete_transient_status_label_runtime_ptr(void *ptr) {
  grid_delete_transient_status_label(static_cast<TransientStatusLabel *>(ptr));
}

inline void grid_delete_alarm_card_runtime_ptr(void *ptr) {
  AlarmCardCtx *ctx = static_cast<AlarmCardCtx *>(ptr);
  if (ctx != nullptr) {
    alarm_release_arming_takeover(ctx);
    if (ctx->arm_delay_timer != nullptr) {
      lv_timer_del(ctx->arm_delay_timer);
      ctx->arm_delay_timer = nullptr;
    }
    if (ctx->pending_action_timer != nullptr) {
      lv_timer_del(ctx->pending_action_timer);
      ctx->pending_action_timer = nullptr;
    }
    grid_delete_transient_status_label(ctx->status_label);
    ctx->status_label = nullptr;
    delete ctx;
  }
}

inline void grid_delete_alarm_action_runtime_ptr(void *ptr) {
  AlarmActionCtx *action = static_cast<AlarmActionCtx *>(ptr);
  if (action != nullptr) {
    grid_delete_alarm_card_runtime_ptr(action->card);
    delete action;
  }
}

inline void grid_delete_fan_card_runtime_ptr(void *ptr) {
  FanCardCtx *ctx = static_cast<FanCardCtx *>(ptr);
  if (ctx != nullptr) {
    grid_delete_transient_status_label(ctx->status_label);
    ctx->status_label = nullptr;
    delete ctx;
  }
}

inline void grid_delete_media_control_runtime_ptr(void *ptr) {
  delete_media_control_context(static_cast<MediaControlCtx *>(ptr));
}

inline void grid_release_runtime_allocations(lv_obj_t *owner) {
  if (owner == nullptr) return;
  std::vector<GridRuntimeAllocation> &allocations = grid_runtime_allocations();
  size_t write_index = 0;
  for (size_t read_index = 0; read_index < allocations.size(); read_index++) {
    GridRuntimeAllocation &allocation = allocations[read_index];
    if (allocation.owner == owner) {
      if (allocation.deleter != nullptr && allocation.ptr != nullptr) {
        allocation.deleter(allocation.ptr);
      }
      continue;
    }
    if (write_index != read_index) allocations[write_index] = allocation;
    write_index++;
  }
  allocations.resize(write_index);
  if (allocations.empty()) std::vector<GridRuntimeAllocation>().swap(allocations);
}

template<typename T>
inline T *grid_track_runtime_allocation(lv_obj_t *owner, T *ptr) {
  if (owner != nullptr && ptr != nullptr) {
    grid_runtime_allocations().push_back({
      owner,
      ptr,
      grid_delete_runtime_ptr<T>,
    });
  }
  return ptr;
}

inline AlarmActionCtx *grid_track_alarm_action_runtime(lv_obj_t *owner,
                                                       AlarmActionCtx *ctx) {
  if (owner != nullptr && ctx != nullptr) {
    grid_runtime_allocations().push_back({
      owner,
      ctx,
      grid_delete_alarm_action_runtime_ptr,
    });
  }
  return ctx;
}

inline MediaControlCtx *grid_track_media_control_runtime(lv_obj_t *owner,
                                                         MediaControlCtx *ctx) {
  if (owner != nullptr && ctx != nullptr) {
    grid_runtime_allocations().push_back({
      owner,
      ctx,
      grid_delete_media_control_runtime_ptr,
    });
  }
  return ctx;
}

inline MediaControlCtx *grid_media_control_runtime_for_owner(lv_obj_t *owner) {
  if (owner == nullptr) return nullptr;
  for (const GridRuntimeAllocation &allocation : grid_runtime_allocations()) {
    if (allocation.owner == owner &&
        allocation.deleter == grid_delete_media_control_runtime_ptr) {
      return static_cast<MediaControlCtx *>(allocation.ptr);
    }
  }
  return nullptr;
}

inline MediaControlCtx *grid_delete_media_control_with_owner(lv_obj_t *owner,
                                                             MediaControlCtx *ctx) {
  if (owner != nullptr && ctx != nullptr) {
    lv_obj_add_event_cb(owner, [](lv_event_t *e) {
      delete_media_control_context(static_cast<MediaControlCtx *>(lv_event_get_user_data(e)));
    }, LV_EVENT_DELETE, ctx);
  }
  return ctx;
}

inline AlarmCardCtx *grid_track_alarm_card_runtime(lv_obj_t *owner,
                                                   AlarmCardCtx *ctx) {
  if (owner != nullptr && ctx != nullptr) {
    grid_runtime_allocations().push_back({
      owner,
      ctx,
      grid_delete_alarm_card_runtime_ptr,
    });
  }
  return ctx;
}

inline FanCardCtx *grid_track_fan_card_runtime(lv_obj_t *owner, FanCardCtx *ctx) {
  if (owner != nullptr && ctx != nullptr) {
    grid_runtime_allocations().push_back({
      owner,
      ctx,
      grid_delete_fan_card_runtime_ptr,
    });
  }
  return ctx;
}

inline TransientStatusLabel *grid_track_transient_status_label_runtime(
    lv_obj_t *owner, TransientStatusLabel *ctx) {
  if (owner != nullptr && ctx != nullptr) {
    grid_runtime_allocations().push_back({
      owner,
      ctx,
      grid_delete_transient_status_label_runtime_ptr,
    });
  }
  return ctx;
}

inline void grid_release_main_runtime_allocations(BtnSlot *slots, int slot_count) {
  if (slots == nullptr) return;
  for (int i = 0; i < slot_count; i++) {
    grid_release_runtime_allocations(slots[i].btn);
  }
}

inline void grid_clear_subpage_parent_targets(BtnSlot *slots, int slot_count) {
  if (slots == nullptr) return;
  for (int i = 0; i < slot_count; i++) {
    ParsedCfg p = parse_cfg(slots[i].config->state);
    if (p.type == "subpage") lv_obj_set_user_data(slots[i].btn, nullptr);
  }
}

inline void grid_phase2(
    BtnSlot *slots, const GridConfig &cfg,
    esphome::text::Text **sp_configs,
    esphome::text::Text **sp_ext_configs,
    esphome::text::Text **sp_ext2_configs,
    esphome::text::Text **sp_ext3_configs,
    esphome::text::Text **sp_ext4_configs,
    esphome::text::Text **sp_ext5_configs,
    esphome::text::Text **sp_ext6_configs,
    esphome::text::Text **sp_ext7_configs,
    const std::string &order_str,
    const std::string &on_hex,
    lv_obj_t *main_page_obj) {
  ESP_LOGI("sensors", "Phase 2: subscriptions + subpages start (%lu ms)", esphome::millis());
  grid_log_memory("start");
  set_display_temperature_unit(cfg.temperature_unit, cfg.timezone);
  const DisplayProfile display = display_profile_from_grid_config(cfg);
  display_set_width_axis(display);
  set_switch_confirmation_message_font(display_switch_confirmation_message_font(display));
  set_switch_confirmation_icon_font(display_icon_font(display));
  int NS = bounded_grid_slots(cfg.num_slots);
  int COLS = cfg.cols > 0 ? cfg.cols : 1;
  configure_grid_layout(main_page_obj, NS, COLS);
  if (NS != cfg.num_slots) {
    ESP_LOGW("sensors", "Grid slot count %d exceeds max %d; ignoring extra slots",
      cfg.num_slots, MAX_GRID_SLOTS);
  }
  int ROWS = (NS + COLS - 1) / COLS;

  static bool has_sensor[MAX_GRID_SLOTS] = {};
  static bool sensor_text_mode[MAX_GRID_SLOTS] = {};
  static bool has_icon_on[MAX_GRID_SLOTS] = {};
  static const char* icon_off_cp[MAX_GRID_SLOTS] = {};
  static const char* icon_on_cp[MAX_GRID_SLOTS] = {};

  static bool sp_child_was_on[MAX_SUBPAGE_ITEMS] = {};
  static std::string sp_entity_ids[MAX_SUBPAGE_ITEMS];
  static int sp_child_alloc_idx = 0;
  static int sp_entity_alloc_idx = 0;
  sp_child_alloc_idx = 0;
  sp_entity_alloc_idx = 0;
  memset(has_sensor, 0, sizeof(has_sensor));
  memset(sensor_text_mode, 0, sizeof(sensor_text_mode));
  memset(has_icon_on, 0, sizeof(has_icon_on));
  bump_ha_subscription_generation();
  weather_forecast_cancel_pending_requests();
  reset_climate_control_refs();
  clear_internal_relay_watchers();
  grid_release_main_runtime_allocations(slots, NS);
  grid_clear_subpage_parent_targets(slots, NS);
  navigation_clear_home_targets();
  // Image-card contexts may still point at widgets inside subpage screens.
  reset_image_card_pool(cfg);
  navigation_clear_subpages();
  clear_subpage_vacuum_card_text_refs();

  bool has_on;
  uint32_t on_val = parse_hex_color(on_hex, has_on);
  uint32_t off_val = display_correct_color(DEFAULT_SECONDARY_COLOR_RAW, display);
  uint32_t sensor_val = display_correct_color(DEFAULT_TERTIARY_COLOR_RAW, display);
  if (has_on) on_val = display_correct_color(on_val, display);

  CardPalette palette;
  palette.has_on = has_on;
  palette.has_off = true;
  palette.has_sensor_color = true;
  palette.on_val = has_on ? on_val : DEFAULT_SLIDER_COLOR;
  palette.off_val = off_val;
  palette.sensor_val = sensor_val;
  set_current_button_primary_color(palette.on_val);

  OrderResult parsed, order;
  parse_order_string(order_str, NS, parsed);
  clear_spanned_cells(parsed, NS, COLS, order);
  lv_obj_t *first_card = nullptr;
  if (order.positions[0] >= 1 && order.positions[0] <= NS) {
    first_card = slots[order.positions[0] - 1].btn;
  } else if (NS > 0) {
    first_card = slots[0].btn;
  }
  set_media_home_grid_metrics(main_page_obj, COLS, ROWS, first_card);

  for (int pos = 0; pos < NS; pos++) {
    int idx = order.positions[pos];
    if (idx < 1 || idx > NS) continue;
    auto &s = slots[idx - 1];
    std::string scfg = s.config->state;

    ParsedCfg p = parse_cfg(scfg);
    const auto family = card_runtime_family(p.type);
    int row_span = order.row_span[idx - 1] > 0 ? order.row_span[idx - 1] : 1;
    int col_span = order.col_span[idx - 1] > 0 ? order.col_span[idx - 1] : 1;
    bool is_1x1_card = card_span_is_single(row_span, col_span);
    if (cfg.info_only && info_only_hidden_card_type(p)) continue;
    navigation_register_home_target(idx, pos, p.label, scfg, s.btn);
    if (family == espcontrol::cards::Family::PUSH) continue;
    if (bind_image_card(s, p, cfg)) continue;
    if (family == espcontrol::cards::Family::LOCAL_SENSOR || sensor_card_local_sensor(p)) continue;
    if (bind_basic_sensor_card(s, p, palette)) continue;
    if (bind_passive_card_sources(s, p)) continue;
    if (p.type == "garage") {
      if (!garage_command_mode(p.sensor) || garage_card_show_status(p)) {
        TransientStatusLabel *status_label = nullptr;
        bind_garage_status_card(s, p, &status_label);
        grid_track_transient_status_label_runtime(s.btn, status_label);
      }
      continue;
    }
    if (p.type == "gate") {
      if (!p.entity.empty()) {
        if (gate_command_mode(p.sensor)) {
          subscribe_control_availability(s.btn, s.btn, p.entity);
        }
      }
      if (!gate_command_mode(p.sensor) || gate_card_show_status(p)) {
        TransientStatusLabel *status_label = nullptr;
        bind_gate_status_card(s, p, &status_label);
        grid_track_transient_status_label_runtime(s.btn, status_label);
      }
      continue;
    }
    if (subpage_parent_sensor_state_enabled(p)) {
      if (subpage_parent_text_state_enabled(p)) {
        subscribe_text_sensor_value(s.text_lbl, p.sensor);
      } else {
        subscribe_sensor_value(s.sensor_lbl, p.sensor, parse_precision(p.precision),
          s.unit_lbl, p.unit);
        if (p.label.empty())
          subscribe_friendly_name(s.text_lbl, p.sensor);
      }
      continue;
    }
    if (subpage_parent_icon_entity_state_enabled(p)) {
      has_sensor[idx - 1] = false;
      sensor_text_mode[idx - 1] = false;
      has_icon_on[idx - 1] = !p.icon_on.empty() && p.icon_on != "Auto";
      if (has_icon_on[idx - 1])
        icon_on_cp[idx - 1] = find_icon(p.icon_on.c_str());

      if (p.icon.empty() || p.icon == "Auto") {
        icon_off_cp[idx - 1] = domain_default_icon(p.entity.substr(0, p.entity.find('.')));
      } else {
        icon_off_cp[idx - 1] = find_icon(p.icon.c_str());
      }

      if (p.label.empty())
        subscribe_friendly_name(s.text_lbl, p.entity);

      std::string parent_subpage_kind = normalize_subpage_kind(cfg_option_value(p.options, "subpage_kind"));
      if (parent_subpage_kind == "climate") {
        subscribe_climate_subpage_parent_indicator(
          p.entity, s.btn, s.icon_lbl, has_icon_on[idx - 1],
          icon_off_cp[idx - 1], icon_on_cp[idx - 1]);
      } else if (parent_subpage_kind == "lawn_mower") {
        subscribe_toggle_state(s.btn, s.icon_lbl, s.sensor_container,
          &has_sensor[idx - 1], &sensor_text_mode[idx - 1],
          &has_icon_on[idx - 1], &icon_off_cp[idx - 1], &icon_on_cp[idx - 1],
          nullptr, p.entity, false, lawn_mower_state_active_ref);
      } else {
        subscribe_toggle_state(s.btn, s.icon_lbl, s.sensor_container,
          &has_sensor[idx - 1], &sensor_text_mode[idx - 1],
          &has_icon_on[idx - 1], &icon_off_cp[idx - 1], &icon_on_cp[idx - 1],
          nullptr, p.entity, false);
      }
      continue;
    }
    if (p.type == "lock") {
      if (!p.entity.empty()) {
        if (!lock_command_mode(p.sensor)) {
          TransientStatusLabel *status_label = nullptr;
          grid_track_runtime_allocation(s.btn, bind_lock_status_card(s, p, &status_label));
          grid_track_transient_status_label_runtime(s.btn, status_label);
        }
      }
      continue;
    }
    if (family == espcontrol::cards::Family::ALARM) {
      if (!p.entity.empty()) {
        AlarmCardCtx *ctx = create_alarm_card_context(
          s, p, main_page_obj, NS, COLS,
          has_on ? on_val : DEFAULT_SLIDER_COLOR,
          off_val,
          sensor_val,
          display_icon_font(display),
          display_media_title_font_or(display, lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN)),
          display_sensor_font(display),
          display_optional_media_title_font(display),
          lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
          lv_obj_get_style_text_color(s.text_lbl, LV_PART_MAIN),
          display_main_width_percent(display),
          false,
          cfg.suspend_display_takeover,
          cfg.resume_display_takeover);
        grid_track_alarm_card_runtime(s.btn, ctx);
        lv_obj_set_user_data(s.btn, ctx);
        subscribe_alarm_state(ctx);
        if (p.label.empty() && !ctx->show_status_label)
          subscribe_friendly_name(ctx->status_label, p.entity);
      }
      continue;
    }
    if (family == espcontrol::cards::Family::ALARM_ACTION) {
      if (!p.entity.empty()) {
        AlarmCardCtx *alarm_action_card = new AlarmCardCtx();
        alarm_action_card->entity_id = p.entity;
        alarm_action_card->label = p.label.empty()
          ? alarm_action_label(p.sensor) : p.label;
        alarm_action_card->options = p.options;
        alarm_action_card->btn = s.btn;
        alarm_action_card->icon_lbl = s.icon_lbl;
        alarm_action_card->grid_page = main_page_obj;
        alarm_action_card->label_font = lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN);
        alarm_action_card->key_label_font =
          display_media_title_font_or(display, alarm_action_card->label_font);
        alarm_action_card->pin_label_font = alarm_action_card->key_label_font;
        alarm_action_card->icon_font = display_icon_font(display);
        alarm_action_card->arming_title_font = alarm_action_card->key_label_font;
        alarm_action_card->on_color = has_on ? on_val : DEFAULT_SLIDER_COLOR;
        alarm_action_card->off_color = off_val;
        alarm_action_card->tertiary_color = sensor_val;
        alarm_action_card->width_compensation_percent = display_main_width_percent(display);
        alarm_action_card->grid_cols = COLS;
        alarm_action_card->suspend_display_takeover = cfg.suspend_display_takeover;
        alarm_action_card->resume_display_takeover = cfg.resume_display_takeover;
        alarm_set_card_state_colors(alarm_action_card, alarm_action_card->on_color);

        AlarmActionCtx *action_ctx = grid_track_alarm_action_runtime(s.btn, new AlarmActionCtx());
        action_ctx->card = alarm_action_card;
        action_ctx->mode = alarm_action_valid(p.sensor) ? p.sensor : "away";
        action_ctx->requires_pin =
          alarm_action_requires_pin(alarm_action_card->options, action_ctx->mode);
        subscribe_alarm_action_state(alarm_action_card, action_ctx->mode);
        lv_obj_set_user_data(s.btn, action_ctx);
      }
      continue;
    }
    if (fan_non_speed_card_type(p.type)) {
      if (!p.entity.empty()) {
        FanCardCtx *ctx = create_fan_card_context(
          s, p,
          has_on ? on_val : DEFAULT_SLIDER_COLOR,
          off_val,
          sensor_val,
          lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
          display_icon_font(display),
          display_main_width_percent(display));
        grid_track_fan_card_runtime(s.btn, ctx);
        subscribe_fan_card_state(ctx);
      }
      continue;
    }
    if (p.type == "fan_control") {
      if (!p.entity.empty()) {
        FanCardCtx *ctx = create_fan_card_context(
          s, p,
          has_on ? on_val : DEFAULT_SLIDER_COLOR,
          palette.has_off ? palette.off_val : SECONDARY_GREY,
          palette.has_sensor_color ? palette.sensor_val : TERTIARY_GREY,
          lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
          display_icon_font(display),
          display_main_width_percent(display));
        subscribe_fan_card_state(ctx);
      }
      continue;
    }
    if (p.type == "cover" && cover_command_mode(p.sensor)) {
      lv_obj_set_user_data(s.btn, nullptr);
      if (!p.entity.empty()) {
        if (p.label.empty())
          subscribe_friendly_name(s.text_lbl, p.entity);
        CoverCommandCtx *ctx = create_cover_command_context(p);
        grid_track_runtime_allocation(s.btn, ctx);
        lv_obj_set_user_data(s.btn, ctx);
        subscribe_cover_command_features(ctx);
      }
      continue;
    }
    if (p.type == "cover" && cover_toggle_mode(p.sensor)) {
      if (!p.entity.empty()) {
        TransientStatusLabel *status_label = create_transient_status_label(
          s.text_lbl, p.label.empty() ? espcontrol_i18n(std::string("Cover")) : p.label);
        grid_track_transient_status_label_runtime(s.btn, status_label);
        subscribe_cover_toggle_state(s.btn, s.icon_lbl, status_label,
          slider_icon_off(p.type, p.entity, p.icon), slider_icon_on(p.type, p.entity, p.icon, p.icon_on), p.entity);
        if (p.label.empty())
          subscribe_friendly_name(status_label, p.entity);
      }
      continue;
    }
    if (family == espcontrol::cards::Family::INTERNAL) {
      if (!p.entity.empty() && !internal_relay_push_mode(p)) {
        bool internal_has_icon_on = !p.icon_on.empty() && p.icon_on != "Auto";
        const char *internal_icon_on = internal_has_icon_on ? find_icon(p.icon_on.c_str()) : nullptr;
        watch_internal_relay_state(p.entity, s.btn, s.icon_lbl,
          internal_has_icon_on, internal_relay_icon(p, false), internal_icon_on);
      }
      continue;
    }
    if (family == espcontrol::cards::Family::ACTION && p.type == "action") {
      if (action_card_option_select(p)) {
        if (!p.entity.empty()) {
          OptionSelectCtx *ctx = create_option_select_context(
            s, p,
            has_on ? on_val : DEFAULT_SLIDER_COLOR,
            off_val,
            sensor_val,
            display_main_width_percent(display));
          grid_track_runtime_allocation(s.btn, ctx);
          subscribe_option_select_state(ctx);
          subscribe_option_select_friendly_name(ctx);
        }
        continue;
      }
      std::string state_entity = action_card_state_entity(p);
      if (!state_entity.empty()) {
        ActionCardStateCtx *ctx = create_action_card_state_context(s, p);
        grid_track_runtime_allocation(s.btn, ctx);
        subscribe_action_card_display_state(ctx, state_entity);
      }
      continue;
    }
    if (family == espcontrol::cards::Family::VACUUM) {
      lv_obj_set_user_data(s.btn, nullptr);
      if (!p.entity.empty() && vacuum_card_mode_needs_state(p.sensor)) {
        VacuumCardCtx *ctx = create_vacuum_card_context(s, p);
        grid_track_runtime_allocation(s.btn, ctx);
        subscribe_vacuum_card_state(ctx);
        lv_obj_set_user_data(s.btn, ctx);
      }
      continue;
    }
    if (family == espcontrol::cards::Family::MOWER) {
      lv_obj_set_user_data(s.btn, nullptr);
      if (!p.entity.empty() && lawn_mower_card_mode_needs_state(p.sensor)) {
        LawnMowerCardCtx *ctx = create_lawn_mower_card_context(s, p);
        grid_track_runtime_allocation(s.btn, ctx);
        subscribe_lawn_mower_card_state(ctx);
        lv_obj_set_user_data(s.btn, ctx);
      }
      continue;
    }
    if (family == espcontrol::cards::Family::OPTION_SELECT) {
      if (!p.entity.empty()) {
        OptionSelectCtx *ctx = create_option_select_context(
          s, p,
          has_on ? on_val : DEFAULT_SLIDER_COLOR,
          off_val,
          sensor_val,
          display_main_width_percent(display));
        grid_track_runtime_allocation(s.btn, ctx);
        subscribe_option_select_state(ctx);
        subscribe_option_select_friendly_name(ctx);
      }
      continue;
    }
    if (family == espcontrol::cards::Family::TODO) {
      if (!p.entity.empty()) {
        TodoCardCtx *ctx = create_todo_card_context(
          s, p,
          has_on ? on_val : DEFAULT_SLIDER_COLOR,
          off_val,
          large_number_square_card_layout(row_span, col_span) &&
              card_large_numbers_active_for_layout(p, row_span, col_span) &&
              display_large_sensor_font(display)
            ? display_large_sensor_font(display) : display_sensor_font(display),
          lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
          display_media_title_font_or(
            display, lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN)),
          display_icon_font(display),
          display_main_width_percent(display),
          is_1x1_card);
        grid_track_runtime_allocation(s.btn, ctx);
        subscribe_todo_state(ctx);
        subscribe_todo_friendly_name(ctx);
      }
      continue;
    }
    if (p.type == "local") {
      continue;
    }
    if (family == espcontrol::cards::Family::MEDIA) {
      if (!p.entity.empty()) {
        std::string mode = media_card_mode(p.sensor);
        if (mode == "play_pause") {
          subscribe_media_state(s.btn, media_play_pause_show_state(p) ? s.text_lbl : nullptr, p.entity);
        } else if (mode == "playlist") {
          MediaPlaylistCtx *ctx = grid_track_runtime_allocation(
            s.btn, create_media_playlist_context(s.btn, p));
          subscribe_media_playlist_state(ctx);
        } else if (media_playback_button_mode(mode)) {
          // Previous/next are momentary actions and do not reflect player state.
        } else if (mode == "control_modal") {
          MediaControlCtx *ctx = grid_track_media_control_runtime(s.btn, create_media_control_context(
            s, p,
            has_on ? on_val : DEFAULT_SLIDER_COLOR,
            palette.has_off ? palette.off_val : SECONDARY_GREY,
            palette.has_sensor_color ? palette.sensor_val : TERTIARY_GREY,
            display_media_control_title_font(display),
            display_media_control_artist_font(
              display, display_volume_label_font(
                display, lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN))),
            display_volume_number_font(display),
            display_icon_font(display),
            display_volume_width_percent(display)));
          subscribe_media_control_state(ctx);
        } else if (mode == "volume") {
          MediaVolumeCtx *ctx = create_media_volume_context(
            s.btn, s.text_lbl, p, has_on ? on_val : DEFAULT_SLIDER_COLOR,
            off_val,
            sensor_val,
            display_sensor_font(display),
            display_volume_number_font(display),
            display_volume_label_font(display)
              ? display_volume_label_font(display)
              : lv_obj_get_style_text_font(s.unit_lbl, LV_PART_MAIN),
            display_volume_label_font(display)
              ? display_volume_label_font(display)
              : lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
            display_icon_font(display),
            display_volume_width_percent(display),
            s.sensor_lbl, s.unit_lbl,
            cfg.suspend_display_takeover, cfg.resume_display_takeover);
          grid_track_runtime_allocation(s.btn, ctx);
          subscribe_media_volume_state(ctx);
          if (p.label.empty()) subscribe_friendly_name(s.text_lbl, p.entity);
        } else if (mode == "now_playing" || mode == "cover_art") {
          MediaNowPlayingCtx *ctx = (MediaNowPlayingCtx *)lv_obj_get_user_data(s.sensor_container);
          setup_media_cover_art(s, p, cfg);
          if (mode == "now_playing") subscribe_media_now_playing_state(ctx, p.entity);
          subscribe_media_cover_art(ctx, p.entity);
          if (mode == "cover_art" && media_cover_art_press_action(p) == "control_modal") {
            MediaControlCtx *control_ctx = grid_track_media_control_runtime(s.btn, create_media_control_context(
              s, p,
              has_on ? on_val : DEFAULT_SLIDER_COLOR,
              palette.has_off ? palette.off_val : SECONDARY_GREY,
              palette.has_sensor_color ? palette.sensor_val : TERTIARY_GREY,
              display_media_control_title_font(display),
              display_media_control_artist_font(
                display, display_volume_label_font(
                  display, lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN))),
              display_volume_number_font(display),
              display_icon_font(display),
              display_volume_width_percent(display)));
            subscribe_media_control_state(control_ctx);
            if (ctx && ctx->cover_art) lv_obj_set_user_data(s.btn, ctx->cover_art);
          }
        } else {
          lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
          if (slider) subscribe_media_slider_state(s.btn, slider, p.entity);
          if (p.label.empty() && mode != "position")
            subscribe_friendly_name(s.text_lbl, p.entity);
        }
      }
      continue;
    }
    if (family == espcontrol::cards::Family::CLIMATE) {
      if (!p.entity.empty()) {
        ClimateControlCtx *ctx = create_climate_control_context(
          s.btn, s.icon_lbl, s.text_lbl, p,
          has_on ? on_val : DEFAULT_SLIDER_COLOR,
          off_val,
          sensor_val,
          display_volume_number_font(display),
          display_volume_label_font(display)
            ? display_volume_label_font(display)
            : lv_obj_get_style_text_font(s.unit_lbl, LV_PART_MAIN),
          display_volume_label_font(display)
            ? display_volume_label_font(display)
            : lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
          display_climate_option_title_font(display)
            ? display_climate_option_title_font(display)
            : lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
          display_climate_option_value_font(display)
            ? display_climate_option_value_font(display)
            : lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
          display_climate_option_title_font(display)
            ? display_climate_option_title_font(display)
            : lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
          display_climate_card_icon_font(display),
          display_icon_font(display),
          display_volume_width_percent(display),
          s.sensor_container, s.sensor_lbl, s.unit_lbl);
        grid_track_runtime_allocation(s.btn, ctx);
        subscribe_climate_control_state(ctx);
      }
      continue;
    }
    if (family == espcontrol::cards::Family::LIGHT_CONTROL) {
      if (!p.entity.empty()) {
        LightControlCtx *ctx = create_light_control_context(
          s, p,
          has_on ? on_val : DEFAULT_SLIDER_COLOR,
          display_volume_number_font(display),
          display_volume_label_font(display)
            ? display_volume_label_font(display)
            : lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
          display_icon_font(display),
          display_volume_width_percent(display));
        grid_track_runtime_allocation(s.btn, ctx);
        subscribe_light_control_state(ctx);
      }
      continue;
    }

    if (p.entity.empty()) continue;

    if (p.type == "cover" && cover_modal_mode(p.sensor)) {
      CoverControlCtx *ctx = create_cover_control_context(
        s, p,
        has_on ? on_val : DEFAULT_SLIDER_COLOR,
        off_val,
        display_climate_option_title_font(display)
          ? display_climate_option_title_font(display)
          : lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
        display_climate_option_value_font(display)
          ? display_climate_option_value_font(display)
          : lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
        display_climate_option_title_font(display)
          ? display_climate_option_title_font(display)
          : lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
        display_climate_card_icon_font(display),
        display_icon_font(display),
        display_volume_width_percent(display));
      grid_track_runtime_allocation(s.btn, ctx);
      subscribe_cover_control_state(ctx);
      continue;
    }

    if (brightness_slider_type(p.type) || p.type == "cover") {
      lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
      bool sl_has_icon_on = slider_has_alt_icon(p.type, p.icon_on);
      const char *sl_icon_on_cp = sl_has_icon_on ? slider_icon_on(p.type, p.entity, p.icon, p.icon_on) : nullptr;
      const char *sl_icon_off_cp = sl_has_icon_on ? slider_icon_off(p.type, p.entity, p.icon) : nullptr;
      subscribe_slider_state(s.btn, s.icon_lbl, slider,
        sl_has_icon_on, sl_icon_off_cp, sl_icon_on_cp, p.entity,
        p.type == "cover" && cover_tilt_mode(p.sensor));
      if (p.label.empty())
        subscribe_friendly_name(s.text_lbl, p.entity);
      continue;
    }
    if (p.type == "light_temperature") {
      lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
      if (slider) {
        int min_k = 2000, max_k = 6500;
        parse_kelvin_range(p.unit, min_k, max_k);
        subscribe_light_temp_state(s.btn, slider, p.entity, min_k, max_k, p.precision == "color");
      }
      if (p.label.empty()) {
        SliderCtx *lctx = slider ? (SliderCtx *)lv_obj_get_user_data(slider) : nullptr;
        subscribe_friendly_name_for_light_temp(s.text_lbl, lctx, p.entity);
      }
      continue;
    }

    has_sensor[idx - 1] = !p.sensor.empty();
    sensor_text_mode[idx - 1] = has_sensor[idx - 1] && p.precision == "text";

    has_icon_on[idx - 1] = !p.icon_on.empty() && p.icon_on != "Auto";
    if (has_icon_on[idx - 1])
      icon_on_cp[idx - 1] = find_icon(p.icon_on.c_str());

    const char* icon_cp = "\U000F0493";
    if (p.icon.empty() || p.icon == "Auto") {
      icon_cp = domain_default_icon(p.entity.substr(0, p.entity.find('.')));
    } else {
      icon_cp = find_icon(p.icon.c_str());
    }
    icon_off_cp[idx - 1] = icon_cp;

    ToggleTextSensorCtx *text_sensor_ctx = nullptr;
    if (sensor_text_mode[idx - 1]) {
      text_sensor_ctx = grid_track_runtime_allocation(s.btn, new ToggleTextSensorCtx());
      text_sensor_ctx->text_lbl = s.text_lbl;
      text_sensor_ctx->steady_text = label_text_or_empty(s.text_lbl);
    }

    if (p.label.empty()) {
      if (text_sensor_ctx)
        subscribe_friendly_name(text_sensor_ctx, p.entity);
      else
        subscribe_friendly_name(s.text_lbl, p.entity);
    }

    subscribe_toggle_state(s.btn, s.icon_lbl, s.sensor_container,
      &has_sensor[idx - 1], &sensor_text_mode[idx - 1],
      &has_icon_on[idx - 1], &icon_off_cp[idx - 1], &icon_on_cp[idx - 1],
      text_sensor_ctx, p.entity);

    if (has_sensor[idx - 1]) {
      if (sensor_text_mode[idx - 1])
        subscribe_toggle_text_sensor_value(text_sensor_ctx, p.sensor);
      else
        subscribe_sensor_value(s.sensor_lbl, p.sensor, parse_precision(p.precision),
          s.unit_lbl, p.unit);
    }
  }

  if (cfg.info_only) return;

  // --- Subpage creation ---
  static lv_coord_t sp_col_dsc[MAX_GRID_SLOTS + 1];
  for (int i = 0; i < COLS; i++) sp_col_dsc[i] = LV_GRID_FR(1);
  sp_col_dsc[COLS] = LV_GRID_TEMPLATE_LAST;
  static lv_coord_t sp_row_dsc[MAX_GRID_SLOTS + 1];
  for (int i = 0; i < ROWS; i++) sp_row_dsc[i] = LV_GRID_FR(1);
  sp_row_dsc[ROWS] = LV_GRID_TEMPLATE_LAST;

  const lv_font_t *sp_icon_fnt = lv_obj_get_style_text_font(slots[0].icon_lbl, LV_PART_MAIN);

  lv_obj_t *ref_btn = slots[0].btn;
  for (int i = 0; i < NS; i++) {
    ParsedCfg pc = parse_cfg(slots[i].config->state);
    if (!brightness_slider_type(pc.type) && pc.type != "cover") {
      ref_btn = slots[i].btn;
      break;
    }
  }
  lv_coord_t sp_radius = lv_obj_get_style_radius(ref_btn, LV_PART_MAIN);
  lv_coord_t sp_pad = lv_obj_get_style_pad_top(ref_btn, LV_PART_MAIN);
  const lv_font_t *sp_btn_fnt = lv_obj_get_style_text_font(ref_btn, LV_PART_MAIN);
  lv_color_t sp_txt_color = lv_obj_get_style_text_color(ref_btn, LV_PART_MAIN);

  lv_coord_t mp_pad_top = lv_obj_get_style_pad_top(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_bottom = lv_obj_get_style_pad_bottom(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_left = lv_obj_get_style_pad_left(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_right = lv_obj_get_style_pad_right(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_row = lv_obj_get_style_pad_row(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_col = lv_obj_get_style_pad_column(main_page_obj, LV_PART_MAIN);

  static int sp_on_count[MAX_GRID_SLOTS] = {};
  memset(sp_on_count, 0, sizeof(sp_on_count));

  for (int si = 0; si < NS; si++) {
    ParsedCfg p = parse_cfg(slots[si].config->state);
    if (p.type != "subpage") continue;
    bool sp_indicator = p.sensor == "indicator" && p.entity.empty();

    bool sp_has_icon_on = !p.icon_on.empty() && p.icon_on != "Auto";
    const char* sp_icon_on_glyph = sp_has_icon_on ? find_icon(p.icon_on.c_str()) : nullptr;
    const char* sp_icon_off_glyph = nullptr;
    if (sp_has_icon_on) {
      sp_icon_off_glyph = (p.icon.empty() || p.icon == "Auto")
        ? "\U000F024B" : find_icon(p.icon.c_str());
    }

    std::string sp_cfg = optional_text_state(sp_configs, si) +
      optional_text_state(sp_ext_configs, si) +
      optional_text_state(sp_ext2_configs, si) +
      optional_text_state(sp_ext3_configs, si) +
      optional_text_state(sp_ext4_configs, si) +
      optional_text_state(sp_ext5_configs, si) +
      optional_text_state(sp_ext6_configs, si) +
      optional_text_state(sp_ext7_configs, si);
    if (sp_cfg.empty()) continue;

    auto sp_btns = parse_subpage_config(sp_cfg);
    std::string sp_order_str = get_subpage_order(sp_cfg);
    std::string sp_back_label = get_subpage_back_label(sp_order_str);

    SubpageOrder sp_ord;
    parse_subpage_order(sp_order_str, NS, sp_btns.size(), sp_ord);

    lv_obj_t *sub_scr = lv_obj_create(NULL);
    int display_order = NS;
    for (int pos = 0; pos < NS; pos++) {
      if (parsed.positions[pos] == si + 1) {
        display_order = pos;
        break;
      }
    }
    navigation_register_subpage(
      si + 1, display_order,
      normalize_subpage_kind(cfg_option_value(p.options, "subpage_kind")),
      sub_scr);
    lv_obj_set_style_bg_color(sub_scr, lv_obj_get_style_bg_color(main_page_obj, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sub_scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_layout(sub_scr, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(sub_scr, sp_col_dsc, sp_row_dsc);
    lv_obj_set_style_pad_top(sub_scr, mp_pad_top, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(sub_scr, mp_pad_bottom, LV_PART_MAIN);
    lv_obj_set_style_pad_left(sub_scr, mp_pad_left, LV_PART_MAIN);
    lv_obj_set_style_pad_right(sub_scr, mp_pad_right, LV_PART_MAIN);
    lv_obj_set_style_pad_row(sub_scr, mp_pad_row, LV_PART_MAIN);
    lv_obj_set_style_pad_column(sub_scr, mp_pad_col, LV_PART_MAIN);
    lv_obj_clear_flag(sub_scr, LV_OBJ_FLAG_SCROLLABLE);
    clock_bar_clear_responsive_grid_cards(sub_scr);

    lv_obj_t *back_btn = create_grid_card_button(
      sub_scr, sp_radius, sp_pad, sp_btn_fnt, sp_txt_color);
    apply_button_colors(back_btn, false, DEFAULT_SLIDER_COLOR, true, off_val);
    set_grid_card_cell(
      back_btn, sub_scr,
      sp_ord.back_pos % COLS, sp_ord.back_pos / COLS,
      sp_ord.back_col_span, sp_ord.back_row_span,
      COLS, ROWS);
    BtnSlot back_slot = create_dynamic_card_slot(
      back_btn, sp_icon_fnt, display_sensor_font(display), sp_btn_fnt, sp_txt_color,
      cfg.subpage_chevron_font);
    display_apply_main_width(back_slot.icon_lbl, display);
    display_apply_slot_text_width(back_slot, display);
    lv_label_set_text(back_slot.icon_lbl, "\U000F0141");
    lv_label_set_text(back_slot.text_lbl, sp_back_label.c_str());

    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
      lv_scr_load_anim((lv_obj_t *)lv_event_get_user_data(e), LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }, LV_EVENT_CLICKED, main_page_obj);
    screen_lock_register_controlled_button(back_btn);

    auto add_parent_indicator = [&](const std::string &entity_id,
                                    bool (*is_active_state)(esphome::StringRef) = is_entity_on_ref) {
      if (!sp_indicator || entity_id.empty()) return;
      lv_obj_t *parent_btn = slots[si].btn;
      lv_obj_t *parent_icon = slots[si].icon_lbl;
      int parent_idx = si;
      int cwi = sp_child_alloc_idx++;
      if (cwi >= MAX_SUBPAGE_ITEMS) {
        ESP_LOGW("sensors", "Too many subpage state indicators; skipping %s", entity_id.c_str());
        return;
      }
      sp_child_was_on[cwi] = false;
      subscribe_subpage_parent_indicator(
        entity_id, parent_btn, parent_icon, parent_idx,
        &sp_child_was_on[cwi], sp_has_icon_on,
        sp_icon_off_glyph, sp_icon_on_glyph, sp_on_count, is_active_state);
    };

    auto add_subpage_toggle_click = [&](lv_obj_t *btn, const std::string &entity_id, bool set_checked) {
      if (entity_id.empty()) return;
      int eid_idx = sp_entity_alloc_idx++;
      if (eid_idx >= MAX_SUBPAGE_ITEMS) {
        ESP_LOGW("sensors", "Too many subpage click handlers; skipping %s", entity_id.c_str());
        return;
      }
      sp_entity_ids[eid_idx] = entity_id;
      if (set_checked) {
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
          lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
          set_card_checked_state(target, true);
          std::string *en = (std::string *)lv_event_get_user_data(e);
          if (en && !en->empty()) send_toggle_action(*en);
        }, LV_EVENT_CLICKED, &sp_entity_ids[eid_idx]);
      } else {
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
          std::string *en = (std::string *)lv_event_get_user_data(e);
          if (en && !en->empty()) send_toggle_action(*en);
        }, LV_EVENT_CLICKED, &sp_entity_ids[eid_idx]);
      }
    };

    for (int gp = 0; gp < NS; gp++) {
      int bn = sp_ord.positions[gp];
      if (bn < 1 || bn > (int)sp_btns.size()) continue;
      auto &sb = sp_btns[bn - 1];
      ParsedCfg sb_cfg = parsed_cfg_from_subpage_btn(sb);
      const auto family = card_runtime_family(sb_cfg.type);
      int col, row;
      if (sp_ord.has_back_token) { col = gp % COLS; row = gp / COLS; }
      else { int op = gp + 1; col = op % COLS; row = op / COLS; }
      int rs = sp_ord.row_span[bn - 1] > 0 ? sp_ord.row_span[bn - 1] : 1;

      lv_obj_t *sb_btn = create_grid_card_button(
        sub_scr, sp_radius, sp_pad, sp_btn_fnt, sp_txt_color);
      int cs = sp_ord.col_span[bn - 1] > 0 ? sp_ord.col_span[bn - 1] : 1;
      set_grid_card_cell(sb_btn, sub_scr, col, row, cs, rs, COLS, ROWS);
      BtnSlot sub_slot = create_dynamic_card_slot(
        sb_btn, sp_icon_fnt, display_sensor_font(display), sp_btn_fnt, sp_txt_color,
        cfg.subpage_chevron_font);
      display_apply_main_width(sub_slot.icon_lbl, display);
      display_apply_slot_text_width(sub_slot, display);
      setup_card_visual(sub_slot, sb_cfg, cfg, palette, rs, cs);

      if (family == espcontrol::cards::Family::SCREEN_LOCK) {
        lv_obj_add_event_cb(sb_btn, [](lv_event_t *) {
          screen_lock_toggle();
        }, LV_EVENT_CLICKED, nullptr);
        continue;
      }
      if (bind_image_card(sub_slot, sb_cfg, cfg, true)) continue;
      if (family == espcontrol::cards::Family::LOCAL_SENSOR || sensor_card_local_sensor(sb_cfg)) continue;
      if (bind_basic_sensor_card(sub_slot, sb_cfg, palette)) continue;
      if (bind_passive_card_sources(sub_slot, sb_cfg)) continue;
      if (sb_cfg.type == "cover" && cover_modal_mode(sb_cfg.sensor)) {
        if (!sb_cfg.entity.empty()) {
          CoverControlCtx *ctx = create_cover_control_context(
            sub_slot, sb_cfg,
            has_on ? on_val : DEFAULT_SLIDER_COLOR,
            off_val,
            display_climate_option_title_font(display)
              ? display_climate_option_title_font(display)
              : lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
            display_climate_option_value_font(display)
              ? display_climate_option_value_font(display)
              : lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
            display_climate_option_title_font(display)
              ? display_climate_option_title_font(display)
              : lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
            display_climate_card_icon_font(display),
            display_icon_font(display),
            display_volume_width_percent(display));
          grid_delete_with_owner(sb_btn, ctx);
          subscribe_cover_control_state(ctx);
          add_parent_indicator(sb_cfg.entity);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
            CoverControlCtx *ctx = (CoverControlCtx *)lv_obj_get_user_data(target);
            if (ctx) cover_control_open_modal(ctx);
          }, LV_EVENT_CLICKED, nullptr);
        }
        continue;
      }
      if (sb_cfg.type == "cover" && cover_command_mode(sb_cfg.sensor)) {
        if (!sb_cfg.entity.empty()) {
          if (sb_cfg.label.empty())
            subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.entity);
          CoverCommandCtx *ctx = grid_delete_with_owner(sb_btn, create_cover_command_context(sb_cfg));
          subscribe_cover_command_features(ctx);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            CoverCommandCtx *c = (CoverCommandCtx *)lv_event_get_user_data(e);
            if (c) send_cover_command_action(*c);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (sb_cfg.type == "cover" && cover_toggle_mode(sb_cfg.sensor)) {
        if (!sb_cfg.entity.empty()) {
          TransientStatusLabel *status_label = create_transient_status_label(
            sub_slot.text_lbl, sb_cfg.label.empty() ? espcontrol_i18n(std::string("Cover")) : sb_cfg.label);
          subscribe_cover_toggle_state(sub_slot.btn, sub_slot.icon_lbl, status_label,
            slider_icon_off(sb_cfg.type, sb_cfg.entity, sb_cfg.icon),
            slider_icon_on(sb_cfg.type, sb_cfg.entity, sb_cfg.icon, sb_cfg.icon_on),
            sb_cfg.entity);
          if (sb_cfg.label.empty())
            subscribe_friendly_name(status_label, sb_cfg.entity);
          add_parent_indicator(sb_cfg.entity);
          add_subpage_toggle_click(sb_btn, sb_cfg.entity, true);
        }
        continue;
      }
      if (sb_cfg.type == "garage") {
        if (!sb_cfg.entity.empty()) {
          if (garage_command_mode(sb_cfg.sensor)) {
            if (garage_card_show_status(sb_cfg)) {
              bind_garage_status_card(sub_slot, sb_cfg);
              add_parent_indicator(sb_cfg.entity);
            }
            ParsedCfg *ctx = grid_delete_with_owner(sb_btn, new ParsedCfg(sb_cfg));
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
              if (c) send_cover_command_action(*c);
            }, LV_EVENT_CLICKED, ctx);
          } else {
            if (bind_garage_status_card(sub_slot, sb_cfg)) {
              add_parent_indicator(sb_cfg.entity);
              add_subpage_toggle_click(sb_btn, sb_cfg.entity, true);
            }
          }
        }
        continue;
      }
      if (sb_cfg.type == "gate") {
        if (!sb_cfg.entity.empty()) {
          if (gate_command_mode(sb_cfg.sensor)) {
            subscribe_control_availability(sub_slot.btn, sub_slot.btn, sb_cfg.entity);
            if (gate_card_show_status(sb_cfg)) {
              bind_gate_status_card(sub_slot, sb_cfg);
              add_parent_indicator(sb_cfg.entity);
            }
            ParsedCfg *ctx = grid_delete_with_owner(sb_btn, new ParsedCfg(sb_cfg));
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
              if (c) send_cover_command_action(*c);
            }, LV_EVENT_CLICKED, ctx);
          } else {
            if (bind_gate_status_card(sub_slot, sb_cfg)) {
              add_parent_indicator(sb_cfg.entity);
              add_subpage_toggle_click(sb_btn, sb_cfg.entity, true);
            }
          }
        }
        continue;
      }
      if (sb_cfg.type == "lock") {
        if (!sb_cfg.entity.empty()) {
          if (lock_command_mode(sb_cfg.sensor)) {
            ParsedCfg *ctx = grid_delete_with_owner(sb_btn, new ParsedCfg(sb_cfg));
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
              if (c) send_lock_command_action(*c);
            }, LV_EVENT_CLICKED, ctx);
          } else {
            LockCardCtx *lock_ctx = bind_lock_status_card(sub_slot, sb_cfg);
            if (lock_ctx) {
              grid_delete_with_owner(sb_btn, lock_ctx);
              add_parent_indicator(sb_cfg.entity);
              lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
                LockCardCtx *ctx = (LockCardCtx *)lv_event_get_user_data(e);
                if (ctx) send_lock_action(ctx);
              }, LV_EVENT_CLICKED, lock_ctx);
            }
          }
        }
        continue;
      }
      if (family == espcontrol::cards::Family::ALARM) {
        ParsedCfg alarm_cfg = sb_cfg;
        if (alarm_cfg.entity.empty()) alarm_cfg.entity = p.entity;
        if (alarm_cfg.options.empty()) alarm_cfg.options = p.options;
        if (!alarm_cfg.entity.empty()) {
          AlarmCardCtx *ctx = create_alarm_card_context(
            sub_slot, alarm_cfg, sub_scr, NS, COLS,
            palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR,
            palette.off_val,
            palette.sensor_val,
            display_icon_font(display),
            display_media_title_font_or(display, lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN)),
            display_sensor_font(display),
            display_optional_media_title_font(display),
            lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
            lv_obj_get_style_text_color(sub_slot.text_lbl, LV_PART_MAIN),
            display_main_width_percent(display),
            false,
            cfg.suspend_display_takeover,
            cfg.resume_display_takeover);
          grid_delete_alarm_card_with_owner(sb_btn, ctx);
          ctx->grid_page = sub_scr;
          lv_obj_set_user_data(sb_btn, ctx);
          subscribe_alarm_state(ctx);
          if (alarm_cfg.label.empty() && !ctx->show_status_label)
            subscribe_friendly_name(ctx->status_label, alarm_cfg.entity);
          add_parent_indicator(alarm_cfg.entity);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            AlarmCardCtx *ctx = (AlarmCardCtx *)lv_event_get_user_data(e);
            if (ctx) alarm_card_open_page(ctx);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (family == espcontrol::cards::Family::ALARM_ACTION) {
        std::string action_entity = sb_cfg.entity.empty() ? p.entity : sb_cfg.entity;
        if (!action_entity.empty()) {
          AlarmCardCtx *alarm_action_card = new AlarmCardCtx();
          alarm_action_card->entity_id = action_entity;
          alarm_action_card->label = sb_cfg.label.empty()
            ? alarm_action_label(sb_cfg.sensor) : sb_cfg.label;
          alarm_action_card->options = sb_cfg.options.empty() ? p.options : sb_cfg.options;
          alarm_action_card->btn = sub_slot.btn;
          alarm_action_card->icon_lbl = sub_slot.icon_lbl;
          alarm_action_card->grid_page = sub_scr;
          alarm_action_card->label_font = lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN);
          alarm_action_card->key_label_font =
            display_media_title_font_or(display, alarm_action_card->label_font);
          alarm_action_card->pin_label_font = alarm_action_card->key_label_font;
          alarm_action_card->icon_font = display_icon_font(display);
          alarm_action_card->arming_title_font = alarm_action_card->key_label_font;
          alarm_action_card->on_color = has_on ? on_val : DEFAULT_SLIDER_COLOR;
          alarm_action_card->off_color = off_val;
          alarm_action_card->tertiary_color = sensor_val;
          alarm_action_card->width_compensation_percent = display_main_width_percent(display);
          alarm_action_card->grid_cols = COLS;
          alarm_action_card->suspend_display_takeover = cfg.suspend_display_takeover;
          alarm_action_card->resume_display_takeover = cfg.resume_display_takeover;
          AlarmActionCtx *action_ctx = grid_delete_alarm_action_with_owner(sb_btn, new AlarmActionCtx());
          action_ctx->card = alarm_action_card;
          action_ctx->mode = alarm_action_valid(sb_cfg.sensor) ? sb_cfg.sensor : "away";
          action_ctx->requires_pin =
            alarm_action_requires_pin(alarm_action_card->options, action_ctx->mode);
          alarm_set_card_state_colors(alarm_action_card, alarm_action_card->on_color);
          subscribe_alarm_action_state(alarm_action_card, action_ctx->mode);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            AlarmActionCtx *action = static_cast<AlarmActionCtx *>(lv_event_get_user_data(e));
            alarm_action_activate(action);
          }, LV_EVENT_CLICKED, action_ctx);
        }
        continue;
      }
      if (fan_non_speed_card_type(sb_cfg.type)) {
        if (!sb_cfg.entity.empty()) {
          FanCardCtx *ctx = create_fan_card_context(
            sub_slot, sb_cfg,
            has_on ? on_val : DEFAULT_SLIDER_COLOR,
            off_val,
            sensor_val,
            lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
            display_icon_font(display),
            display_main_width_percent(display));
          grid_delete_with_owner(sb_btn, ctx);
          subscribe_fan_card_state(ctx);
          add_parent_indicator(sb_cfg.entity);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            FanCardCtx *ctx = (FanCardCtx *)lv_event_get_user_data(e);
            if (ctx) fan_card_handle_click(ctx);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (sb_cfg.type == "fan_control") {
        if (!sb_cfg.entity.empty()) {
          FanCardCtx *ctx = create_fan_card_context(
            sub_slot, sb_cfg,
            has_on ? on_val : DEFAULT_SLIDER_COLOR,
            palette.has_off ? palette.off_val : SECONDARY_GREY,
            palette.has_sensor_color ? palette.sensor_val : TERTIARY_GREY,
            lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
            display_icon_font(display),
            display_main_width_percent(display));
          subscribe_fan_card_state(ctx);
          add_parent_indicator(sb_cfg.entity);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            FanCardCtx *ctx = (FanCardCtx *)lv_event_get_user_data(e);
            if (ctx) fan_control_open_modal(ctx);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (family == espcontrol::cards::Family::PUSH) {
        std::string push_label = sb_cfg.label.empty() ? espcontrol_i18n(std::string("Push")) : sb_cfg.label;
        std::string *label = grid_delete_with_owner(sb_btn, new std::string(push_label));
        lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
          std::string *label = (std::string *)lv_event_get_user_data(e);
          esphome::api::HomeassistantActionRequest req;
          if (!ha_action_begin(req, "esphome.push_button_pressed", true, 1)) return;
          ha_action_add_data(req, "label", label->c_str());
          ha_action_send(req);
        }, LV_EVENT_CLICKED, label);
        continue;
      }
      if (family == espcontrol::cards::Family::ACTION && sb_cfg.type == "action") {
        if (!sb_cfg.entity.empty() && !sb_cfg.sensor.empty()) {
          if (action_card_local_action(sb_cfg)) {
            std::string *key = grid_delete_with_owner(sb_btn, new std::string(sb_cfg.entity));
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              std::string *k = (std::string *)lv_event_get_user_data(e);
              if (k) send_local_action(*k);
            }, LV_EVENT_CLICKED, key);
            continue;
          }
          if (action_card_option_select(sb_cfg)) {
            OptionSelectCtx *ctx = create_option_select_context(
              sub_slot, sb_cfg,
              has_on ? on_val : DEFAULT_SLIDER_COLOR,
              off_val,
              sensor_val,
              display_main_width_percent(display));
            grid_delete_with_owner(sb_btn, ctx);
            subscribe_option_select_state(ctx);
            subscribe_option_select_friendly_name(ctx);
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              OptionSelectCtx *ctx = (OptionSelectCtx *)lv_event_get_user_data(e);
              if (ctx) option_select_open_modal(ctx);
            }, LV_EVENT_CLICKED, ctx);
            continue;
          }
          std::string state_entity = action_card_state_entity(sb_cfg);
          if (!state_entity.empty()) {
            ActionCardStateCtx *action_ctx = create_action_card_state_context(sub_slot, sb_cfg);
            grid_delete_with_owner(sb_btn, action_ctx);
            subscribe_action_card_display_state(action_ctx, state_entity);
          }
          ParsedCfg *ctx = grid_delete_with_owner(sb_btn, new ParsedCfg(sb_cfg));
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
            lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
            if (!c) return;
            if (action_script_confirmation_enabled(*c) && target) {
              switch_confirmation_open_modal(*c, target, false);
            } else {
              send_action_card_action(*c);
            }
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (family == espcontrol::cards::Family::VACUUM) {
        if (!sb_cfg.entity.empty()) {
          VacuumCardCtx *ctx = create_vacuum_card_context(sub_slot, sb_cfg);
          grid_delete_with_owner(sb_btn, ctx);
          register_subpage_vacuum_card_text(sub_slot.text_lbl, ctx, sb_cfg);
          if (vacuum_card_mode_needs_state(sb_cfg.sensor)) {
            subscribe_vacuum_card_state(ctx);
          }
          add_parent_indicator(sb_cfg.entity);
          if (!vacuum_card_read_only(sb_cfg)) {
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              VacuumCardCtx *ctx = (VacuumCardCtx *)lv_event_get_user_data(e);
              send_vacuum_card_action(ctx);
            }, LV_EVENT_CLICKED, ctx);
          }
        }
        continue;
      }
      if (family == espcontrol::cards::Family::MOWER) {
        if (!sb_cfg.entity.empty()) {
          LawnMowerCardCtx *ctx = create_lawn_mower_card_context(sub_slot, sb_cfg);
          grid_delete_with_owner(sb_btn, ctx);
          if (lawn_mower_card_mode_needs_state(sb_cfg.sensor)) {
            subscribe_lawn_mower_card_state(ctx);
          }
          add_parent_indicator(sb_cfg.entity, lawn_mower_state_active_ref);
          if (!lawn_mower_card_read_only(sb_cfg)) {
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              LawnMowerCardCtx *ctx = (LawnMowerCardCtx *)lv_event_get_user_data(e);
              send_lawn_mower_card_action(ctx);
            }, LV_EVENT_CLICKED, ctx);
          }
        }
        continue;
      }
      if (family == espcontrol::cards::Family::WEBHOOK) {
        ParsedCfg *ctx = grid_delete_with_owner(sb_btn, new ParsedCfg(sb_cfg));
        lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
          ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
          if (c) send_webhook_action(*c);
        }, LV_EVENT_CLICKED, ctx);
        continue;
      }
      if (family == espcontrol::cards::Family::TODO) {
        if (!sb_cfg.entity.empty()) {
          TodoCardCtx *ctx = create_todo_card_context(
            sub_slot, sb_cfg,
            has_on ? on_val : DEFAULT_SLIDER_COLOR,
            off_val,
            large_number_square_card_layout(rs, cs) &&
                card_large_numbers_active_for_layout(sb_cfg, rs, cs) &&
                display_large_sensor_font(display)
              ? display_large_sensor_font(display) : display_sensor_font(display),
            lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
            display_media_title_font_or(
              display, lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN)),
            display_icon_font(display),
            display_main_width_percent(display),
            card_span_is_single(rs, cs));
          grid_delete_with_owner(sb_btn, ctx);
          subscribe_todo_state(ctx);
          subscribe_todo_friendly_name(ctx);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            TodoCardCtx *ctx = (TodoCardCtx *)lv_event_get_user_data(e);
            if (todo_card_context_valid(ctx)) todo_card_open_modal(ctx);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (family == espcontrol::cards::Family::OPTION_SELECT) {
        if (!sb_cfg.entity.empty()) {
          OptionSelectCtx *ctx = create_option_select_context(
            sub_slot, sb_cfg,
            has_on ? on_val : DEFAULT_SLIDER_COLOR,
            off_val,
            sensor_val,
            display_main_width_percent(display));
          grid_delete_with_owner(sb_btn, ctx);
          subscribe_option_select_state(ctx);
          subscribe_option_select_friendly_name(ctx);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            OptionSelectCtx *ctx = (OptionSelectCtx *)lv_event_get_user_data(e);
            if (ctx) option_select_open_modal(ctx);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (family == espcontrol::cards::Family::MEDIA) {
        std::string mode = media_card_mode(sb_cfg.sensor);
        if (!sb_cfg.entity.empty()) {
          if (mode == "playlist") {
            ParsedCfg *ctx = grid_delete_with_owner(sb_btn, new ParsedCfg(sb_cfg));
            MediaPlaylistCtx *playlist_ctx = grid_delete_with_owner(
              sb_btn, create_media_playlist_context(sub_slot.btn, sb_cfg));
            subscribe_media_playlist_state(playlist_ctx);
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
              if (c) {
                ESP_LOGI("button", "Subpage playlist clicked: entity=%s label=%s mode=%s options=%s",
                         c->entity.c_str(), c->label.c_str(), c->sensor.c_str(), c->options.c_str());
                send_media_playlist_action(*c);
              }
            }, LV_EVENT_CLICKED, ctx);
          } else if (media_playback_button_mode(mode)) {
            ParsedCfg *ctx = grid_delete_with_owner(sb_btn, new ParsedCfg(sb_cfg));
            ctx->sensor = mode;
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
              if (c) send_media_playback_action(c->entity, media_card_mode(c->sensor));
            }, media_fast_press_mode(mode) ? LV_EVENT_PRESSED : LV_EVENT_CLICKED, ctx);
            if (mode == "play_pause")
              subscribe_media_state(sub_slot.btn,
                media_play_pause_show_state(sb_cfg) ? sub_slot.text_lbl : nullptr,
                sb_cfg.entity);
          } else if (mode == "control_modal") {
            MediaControlCtx *ctx = grid_delete_media_control_with_owner(sb_btn, create_media_control_context(
              sub_slot, sb_cfg,
              has_on ? on_val : DEFAULT_SLIDER_COLOR,
              palette.has_off ? palette.off_val : SECONDARY_GREY,
              palette.has_sensor_color ? palette.sensor_val : TERTIARY_GREY,
              display_media_control_title_font(display),
              display_media_control_artist_font(
                display, display_volume_label_font(
                  display, lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN))),
              display_volume_number_font(display),
              display_icon_font(display),
              display_volume_width_percent(display)));
            subscribe_media_control_state(ctx);
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              MediaControlCtx *ctx = (MediaControlCtx *)lv_event_get_user_data(e);
              if (ctx) media_control_open_modal(ctx);
            }, LV_EVENT_CLICKED, ctx);
          } else if (mode == "volume") {
            MediaVolumeCtx *ctx = create_media_volume_context(
              sub_slot.btn, sub_slot.text_lbl, sb_cfg,
              has_on ? on_val : DEFAULT_SLIDER_COLOR,
              off_val,
              sensor_val,
              display_sensor_font(display),
              display_volume_number_font(display),
              display_volume_label_font(display)
                ? display_volume_label_font(display)
                : lv_obj_get_style_text_font(sub_slot.unit_lbl, LV_PART_MAIN),
              display_volume_label_font(display)
                ? display_volume_label_font(display)
                : lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
              display_icon_font(display),
              display_volume_width_percent(display),
              sub_slot.sensor_lbl, sub_slot.unit_lbl,
              cfg.suspend_display_takeover, cfg.resume_display_takeover);
            grid_delete_with_owner(sb_btn, ctx);
            subscribe_media_volume_state(ctx);
            if (sb_cfg.label.empty()) subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.entity);
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              MediaVolumeCtx *ctx = (MediaVolumeCtx *)lv_event_get_user_data(e);
              if (ctx) media_volume_open_modal(ctx);
            }, LV_EVENT_CLICKED, ctx);
          } else if (mode == "now_playing" || mode == "cover_art") {
            MediaNowPlayingCtx *ctx = (MediaNowPlayingCtx *)lv_obj_get_user_data(sub_slot.sensor_container);
            setup_media_cover_art(sub_slot, sb_cfg, cfg);
            if (mode == "now_playing") subscribe_media_now_playing_state(ctx, sb_cfg.entity);
            subscribe_media_cover_art(ctx, sb_cfg.entity);
            if (mode == "cover_art" && media_cover_art_press_action(sb_cfg) == "control_modal") {
              MediaControlCtx *control_ctx = grid_delete_media_control_with_owner(sb_btn, create_media_control_context(
                sub_slot, sb_cfg,
                has_on ? on_val : DEFAULT_SLIDER_COLOR,
                palette.has_off ? palette.off_val : SECONDARY_GREY,
                palette.has_sensor_color ? palette.sensor_val : TERTIARY_GREY,
                display_media_control_title_font(display),
                display_media_control_artist_font(
                  display, display_volume_label_font(
                    display, lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN))),
                display_volume_number_font(display),
                display_icon_font(display),
                display_volume_width_percent(display)));
              subscribe_media_control_state(control_ctx);
              if (ctx && ctx->cover_art) lv_obj_set_user_data(sb_btn, ctx->cover_art);
              lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
                MediaControlCtx *ctx = (MediaControlCtx *)lv_event_get_user_data(e);
                if (ctx) media_control_open_modal(ctx);
              }, LV_EVENT_CLICKED, control_ctx);
            } else if (mode == "cover_art" || media_now_playing_play_pause_enabled(sb_cfg)) {
              ParsedCfg *click_ctx = grid_delete_with_owner(sb_btn, new ParsedCfg(sb_cfg));
              lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
                ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
                if (c) send_media_playback_action(c->entity, "play_pause");
              }, LV_EVENT_CLICKED, click_ctx);
            }
          } else {
            lv_obj_t *media_slider = (lv_obj_t *)lv_obj_get_user_data(sub_slot.sensor_container);
            if (media_slider) subscribe_media_slider_state(sub_slot.btn, media_slider, sb_cfg.entity);
            if (sb_cfg.label.empty() && mode != "position")
              subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.entity);
          }
          add_parent_indicator(sb_cfg.entity);
        }
        continue;
      }
      if (family == espcontrol::cards::Family::CLIMATE) {
        if (!sb_cfg.entity.empty()) {
          ClimateControlCtx *ctx = create_climate_control_context(
            sub_slot.btn, sub_slot.icon_lbl, sub_slot.text_lbl, sb_cfg,
            has_on ? on_val : DEFAULT_SLIDER_COLOR,
            off_val,
            sensor_val,
            display_volume_number_font(display),
            display_volume_label_font(display)
              ? display_volume_label_font(display)
              : lv_obj_get_style_text_font(sub_slot.unit_lbl, LV_PART_MAIN),
            display_volume_label_font(display)
              ? display_volume_label_font(display)
              : lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
            display_climate_option_title_font(display)
              ? display_climate_option_title_font(display)
              : lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
            display_climate_option_value_font(display)
              ? display_climate_option_value_font(display)
              : lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
            display_climate_option_title_font(display)
              ? display_climate_option_title_font(display)
              : lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
            display_climate_card_icon_font(display),
            display_icon_font(display),
            display_volume_width_percent(display),
            sub_slot.sensor_container, sub_slot.sensor_lbl, sub_slot.unit_lbl);
          grid_delete_with_owner(sb_btn, ctx);
          subscribe_climate_control_state(ctx);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            ClimateControlCtx *ctx = (ClimateControlCtx *)lv_event_get_user_data(e);
            if (ctx) climate_control_open_modal(ctx);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (sb_cfg.type == "local") {
        if (!sb_cfg.entity.empty()) {
          std::string *key = grid_delete_with_owner(sb_btn, new std::string(sb_cfg.entity));
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            std::string *k = (std::string *)lv_event_get_user_data(e);
            if (k) send_local_action(*k);
          }, LV_EVENT_CLICKED, key);
        }
        continue;
      }
      if (sb_cfg.type == "light_control") {
        if (!sb_cfg.entity.empty()) {
          LightControlCtx *ctx = create_light_control_context(
            sub_slot, sb_cfg,
            has_on ? on_val : DEFAULT_SLIDER_COLOR,
            display_volume_number_font(display),
            display_volume_label_font(display)
              ? display_volume_label_font(display)
              : lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
            display_icon_font(display),
            display_volume_width_percent(display));
          grid_delete_with_owner(sb_btn, ctx);
          subscribe_light_control_state(ctx);
          add_parent_indicator(sb_cfg.entity);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            LightControlCtx *ctx = (LightControlCtx *)lv_event_get_user_data(e);
            if (ctx) light_control_open_modal(ctx);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (family == espcontrol::cards::Family::INTERNAL) {
        bool push_mode = internal_relay_push_mode(sb_cfg);
        if (!push_mode && !sb_cfg.entity.empty()) {
          bool internal_has_icon_on = !sb_cfg.icon_on.empty() && sb_cfg.icon_on != "Auto";
          const char *internal_icon_on = internal_has_icon_on ? find_icon(sb_cfg.icon_on.c_str()) : nullptr;
          bool *child_was_on = nullptr;
          lv_obj_t *parent_btn = nullptr;
          lv_obj_t *parent_icon = nullptr;
          if (sp_indicator) {
            int cwi = sp_child_alloc_idx++;
            if (cwi >= MAX_SUBPAGE_ITEMS) {
              ESP_LOGW("sensors", "Too many subpage state indicators; skipping %s", sb_cfg.entity.c_str());
            } else {
              sp_child_was_on[cwi] = false;
              child_was_on = &sp_child_was_on[cwi];
              parent_btn = slots[si].btn;
              parent_icon = slots[si].icon_lbl;
            }
          }
          watch_internal_relay_state(
            sb_cfg.entity, sub_slot.btn, sub_slot.icon_lbl,
            internal_has_icon_on, internal_relay_icon(sb_cfg, push_mode), internal_icon_on,
            child_was_on, parent_btn, parent_icon, si,
            sp_has_icon_on, sp_icon_off_glyph, sp_icon_on_glyph, sp_on_count);
        }
        if (!sb_cfg.entity.empty()) {
          InternalRelayClickCtx *ctx = grid_delete_with_owner(sb_btn, new InternalRelayClickCtx());
          ctx->key = sb_cfg.entity;
          ctx->push_mode = push_mode;
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            InternalRelayClickCtx *c = (InternalRelayClickCtx *)lv_event_get_user_data(e);
            if (c && !c->key.empty()) send_internal_relay_action(c->key, c->push_mode);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (family == espcontrol::cards::Family::LIGHT_TEMPERATURE) {
        if (!sb_cfg.entity.empty()) {
          lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(sub_slot.sensor_container);
          if (slider) {
            int min_k = 2000, max_k = 6500;
            parse_kelvin_range(sb_cfg.unit, min_k, max_k);
            subscribe_light_temp_state(sub_slot.btn, slider, sb_cfg.entity,
              min_k, max_k, sb_cfg.precision == "color");
            if (sb_cfg.label.empty()) {
              SliderCtx *lctx = (SliderCtx *)lv_obj_get_user_data(slider);
              subscribe_friendly_name_for_light_temp(sub_slot.text_lbl, lctx, sb_cfg.entity);
            }
          }
          add_parent_indicator(sb_cfg.entity);
        }
        continue;
      }
      if (brightness_slider_type(sb_cfg.type) || sb_cfg.type == "cover") {
        if (!sb_cfg.entity.empty()) {
          lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(sub_slot.sensor_container);
          bool sl_has_icon_on = slider_has_alt_icon(sb_cfg.type, sb_cfg.icon_on);
          const char *sl_icon_on = sl_has_icon_on
            ? slider_icon_on(sb_cfg.type, sb_cfg.entity, sb_cfg.icon, sb_cfg.icon_on) : nullptr;
          const char *sl_icon_off = sl_has_icon_on
            ? slider_icon_off(sb_cfg.type, sb_cfg.entity, sb_cfg.icon) : nullptr;
          if (slider) {
            subscribe_slider_state(sub_slot.btn, sub_slot.icon_lbl, slider,
              sl_has_icon_on, sl_icon_off, sl_icon_on, sb_cfg.entity,
              sb_cfg.type == "cover" && cover_tilt_mode(sb_cfg.sensor));
          }
          if (sb_cfg.label.empty())
            subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.entity);
          add_parent_indicator(sb_cfg.entity);
          std::string *eid = grid_delete_with_owner(sb_btn, new std::string(sb_cfg.entity));
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            std::string *en = (std::string *)lv_event_get_user_data(e);
            if (en && !en->empty()) send_slider_action(*en, -1);
          }, LV_EVENT_CLICKED, eid);
        }
        continue;
      }
      if (!sb_cfg.entity.empty()) {
        bool switch_has_sensor = !sb_cfg.sensor.empty();
        bool switch_sensor_text_mode = switch_has_sensor && sb_cfg.precision == "text";
        bool switch_has_icon_on = !sb_cfg.icon_on.empty() && sb_cfg.icon_on != "Auto";
        const char *switch_icon_on = switch_has_icon_on ? find_icon(sb_cfg.icon_on.c_str()) : nullptr;
        const char *switch_icon_off = (sb_cfg.icon.empty() || sb_cfg.icon == "Auto")
          ? domain_default_icon(sb_cfg.entity.substr(0, sb_cfg.entity.find('.')))
          : find_icon(sb_cfg.icon.c_str());

        ToggleTextSensorCtx *switch_text_ctx = nullptr;
        if (switch_sensor_text_mode) {
          switch_text_ctx = grid_delete_with_owner(sb_btn, new ToggleTextSensorCtx());
          switch_text_ctx->text_lbl = sub_slot.text_lbl;
          switch_text_ctx->steady_text = label_text_or_empty(sub_slot.text_lbl);
        }

        if (sb_cfg.label.empty()) {
          if (switch_text_ctx)
            subscribe_friendly_name(switch_text_ctx, sb_cfg.entity);
          else
            subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.entity);
        }

        bool *switch_has_sensor_ptr = grid_delete_with_owner(sb_btn, new bool(switch_has_sensor));
        bool *switch_sensor_text_ptr = grid_delete_with_owner(sb_btn, new bool(switch_sensor_text_mode));
        bool *switch_has_icon_on_ptr = grid_delete_with_owner(sb_btn, new bool(switch_has_icon_on));
        const char **switch_icon_off_ptr = grid_delete_with_owner(sb_btn, new const char*(switch_icon_off));
        const char **switch_icon_on_ptr = grid_delete_with_owner(sb_btn, new const char*(switch_icon_on));
        subscribe_toggle_state(sub_slot.btn, sub_slot.icon_lbl, sub_slot.sensor_container,
          switch_has_sensor_ptr, switch_sensor_text_ptr, switch_has_icon_on_ptr,
          switch_icon_off_ptr, switch_icon_on_ptr, switch_text_ctx, sb_cfg.entity);

        if (switch_has_sensor) {
          if (switch_sensor_text_mode)
            subscribe_toggle_text_sensor_value(switch_text_ctx, sb_cfg.sensor);
          else
            subscribe_sensor_value(sub_slot.sensor_lbl, sb_cfg.sensor, parse_precision(sb_cfg.precision),
              sub_slot.unit_lbl, sb_cfg.unit);
        }

        add_parent_indicator(sb_cfg.entity);
        ParsedCfg *click_ctx = grid_delete_with_owner(sb_btn, new ParsedCfg(sb_cfg));
        lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
          ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
          lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
          if (!c || c->entity.empty()) return;
          bool currently_on = target && lv_obj_has_state(target, LV_STATE_CHECKED);
          if (switch_confirmation_required(*c, currently_on) && target &&
              !is_button_entity(c->entity)) {
            switch_confirmation_open_modal(*c, target, !currently_on);
          } else {
            send_toggle_action(c->entity);
          }
        }, LV_EVENT_CLICKED, click_ctx);
      }
    }

    lv_obj_set_user_data(slots[si].btn, (void *)sub_scr);
  }
  screen_lock_apply();
  refresh_weather_forecast_cards();
  grid_log_memory("end");
  ESP_LOGI("sensors", "Phase 2: done (%lu ms)", esphome::millis());
}

inline void grid_phase2(
    BtnSlot *slots, const GridConfig &cfg,
    esphome::text::Text **sp_configs,
    esphome::text::Text **sp_ext_configs,
    esphome::text::Text **sp_ext2_configs,
    esphome::text::Text **sp_ext3_configs,
    const std::string &order_str,
    const std::string &on_hex,
    lv_obj_t *main_page_obj) {
  grid_phase2(slots, cfg, sp_configs, sp_ext_configs, sp_ext2_configs, sp_ext3_configs,
    nullptr, nullptr, nullptr, nullptr,
    order_str, on_hex, main_page_obj);
}

inline void grid_phase2(
    BtnSlot *slots, const GridConfig &cfg,
    esphome::text::Text **sp_configs,
    esphome::text::Text **sp_ext_configs,
    const std::string &order_str,
    const std::string &on_hex,
    lv_obj_t *main_page_obj) {
  grid_phase2(slots, cfg, sp_configs, sp_ext_configs, nullptr, nullptr,
    order_str, on_hex, main_page_obj);
}

// ── Phase 3: Temperature + presence/media subscriptions ───────────────

inline uint32_t &clock_bar_temperature_subscription_generation() {
  static uint32_t generation = 0;
  return generation;
}

inline bool configure_clock_bar_temperature_entities(
    const std::string &temperature_entities,
    lv_obj_t **temperature_labels,
    size_t temperature_label_count,
    lv_obj_t *main_page_obj,
    std::function<bool()> clock_bar_visible_callback = nullptr,
    std::function<bool()> clock_bar_temperature_visible_callback = nullptr) {
  set_clock_bar_temperature_labels(temperature_labels, temperature_label_count);

  std::vector<std::string> clock_bar_entities =
      parse_clock_bar_temperature_entities(temperature_entities);
  uint32_t generation = ++clock_bar_temperature_subscription_generation();

  if (clock_bar_entities.empty()) {
    set_clock_bar_temperature_value_count(0);
    return false;
  }

  set_clock_bar_temperature_value_count(clock_bar_entities.size());
  refresh_clock_bar_temperature_label_values(
      main_page_obj,
      clock_bar_visible_callback ? clock_bar_visible_callback() : true,
      false,
      clock_bar_temperature_visible_callback
          ? clock_bar_temperature_visible_callback()
          : true,
      NAN, NAN);

  for (size_t i = 0; i < clock_bar_entities.size(); i++) {
    ha_subscribe_state(
      clock_bar_entities[i],
      std::function<void(esphome::StringRef)>(
        [i, generation, main_page_obj, clock_bar_visible_callback,
         clock_bar_temperature_visible_callback](esphome::StringRef state) {
          if (generation != clock_bar_temperature_subscription_generation()) return;
          float val = 0.0f;
          if (parse_float_ref(state, val)) {
            std::vector<float> &values = clock_bar_temperature_values();
            if (i < values.size()) values[i] = val;
            refresh_clock_bar_temperature_label_values(
                main_page_obj,
                clock_bar_visible_callback ? clock_bar_visible_callback() : true,
                false,
                clock_bar_temperature_visible_callback
                    ? clock_bar_temperature_visible_callback()
                    : true,
                NAN, NAN);
          }
        }),
      HA_SUBSCRIPTION_SCOPE_PHASE3
    );
  }

  return true;
}

inline void grid_phase3(
    bool indoor_on, bool outdoor_on,
    const std::string &indoor_entity, const std::string &outdoor_entity,
    const std::string &temperature_entities,
    float *indoor_temp_ptr, float *outdoor_temp_ptr,
    lv_obj_t **temperature_labels,
    size_t temperature_label_count,
    lv_obj_t *main_page_obj,
    const std::string &presence_entity,
    bool *presence_detected_ptr,
    const std::string &media_player_entity,
    bool *media_player_playing_ptr,
    std::function<bool()> clock_bar_visible_callback,
    std::function<void()> wake_callback,
    std::function<void()> sleep_callback,
    std::function<bool()> clock_bar_temperature_visible_callback = nullptr) {
  ESP_LOGI("sensors", "Phase 3: temp/presence/media subscriptions start (%lu ms)", esphome::millis());
  bool has_clock_bar_entities = configure_clock_bar_temperature_entities(
      temperature_entities, temperature_labels, temperature_label_count,
      main_page_obj, clock_bar_visible_callback,
      clock_bar_temperature_visible_callback);
  if (has_clock_bar_entities) {
    indoor_on = false;
  }

  refresh_clock_bar_temperature_label_values(
      main_page_obj,
      clock_bar_visible_callback ? clock_bar_visible_callback() : true,
      indoor_on, outdoor_on,
      indoor_temp_ptr ? *indoor_temp_ptr : NAN,
      outdoor_temp_ptr ? *outdoor_temp_ptr : NAN);

  if (indoor_on && !indoor_entity.empty()) {
    ha_subscribe_state(
      indoor_entity,
      std::function<void(esphome::StringRef)>(
        [indoor_on, outdoor_on, indoor_temp_ptr, outdoor_temp_ptr,
         main_page_obj, clock_bar_visible_callback](esphome::StringRef state) {
          float val = 0.0f;
          if (parse_float_ref(state, val)) {
            *indoor_temp_ptr = val;
            refresh_clock_bar_temperature_label_values(
                main_page_obj,
                clock_bar_visible_callback ? clock_bar_visible_callback() : true,
                indoor_on, outdoor_on, *indoor_temp_ptr, *outdoor_temp_ptr);
          }
        }),
      HA_SUBSCRIPTION_SCOPE_PHASE3
    );
  }

  if (outdoor_on && !outdoor_entity.empty()) {
    ha_subscribe_state(
      outdoor_entity,
      std::function<void(esphome::StringRef)>(
        [indoor_on, outdoor_on, indoor_temp_ptr, outdoor_temp_ptr,
         main_page_obj, clock_bar_visible_callback](esphome::StringRef state) {
          float val = 0.0f;
          if (parse_float_ref(state, val)) {
            *outdoor_temp_ptr = val;
            refresh_clock_bar_temperature_label_values(
                main_page_obj,
                clock_bar_visible_callback ? clock_bar_visible_callback() : true,
                indoor_on, outdoor_on, *indoor_temp_ptr, *outdoor_temp_ptr);
          }
        }),
      HA_SUBSCRIPTION_SCOPE_PHASE3
    );
  }

  if (!presence_entity.empty()) {
    ha_subscribe_state(
      presence_entity,
      std::function<void(esphome::StringRef)>(
        [presence_detected_ptr, wake_callback, sleep_callback](esphome::StringRef state) {
          if (state == "on") {
            *presence_detected_ptr = true;
            lv_disp_trig_activity(NULL);
            if (wake_callback) wake_callback();
          } else if (state == "off") {
            *presence_detected_ptr = false;
            if (sleep_callback) sleep_callback();
          }
        }),
      HA_SUBSCRIPTION_SCOPE_PHASE3
    );
  }

  if (!media_player_entity.empty() && media_player_playing_ptr) {
    ha_subscribe_state(
      media_player_entity,
      std::function<void(esphome::StringRef)>(
        [media_player_playing_ptr](esphome::StringRef state) {
          *media_player_playing_ptr = state == "playing";
        }),
      HA_SUBSCRIPTION_SCOPE_PHASE3
    );
  }
  ESP_LOGI("sensors", "Phase 3: done (%lu ms)", esphome::millis());
}
