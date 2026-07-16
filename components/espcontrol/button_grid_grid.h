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
  // The device's default grid (in the active orientation). Icons render at
  // full size for this grid and scale down as the user makes the grid denser.
  // 0 means "unknown" -> fall back to the active grid (no scaling).
  int base_cols = 0;
  int base_rows = 0;
  // Total compiled button slots (the ceiling the grid can grow to). All of
  // them are built once at boot so the grid can grow live without a restart.
  int compiled_slots = 0;
  bool width_compensation_vertical = false;
  bool wrap_tall_labels;
  bool info_only = false;
  bool subpage_chevrons_enabled = true;
  int width_compensation_percent = 100;
  int volume_width_compensation_percent = 100;
  int media_artwork_width_compensation_percent = 100;
  DisplayModalProfile modal_profile;
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
  std::function<void(espcontrol::DisplayTakeoverKind)> begin_display_takeover;
  std::function<void(espcontrol::DisplayTakeoverKind)> end_display_takeover;
  esphome::artwork_image::ArtworkImage **image_card_images = nullptr;
  esphome::artwork_image::ArtworkImage *image_card_modal_image = nullptr;
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
  profile.modal = cfg.modal_profile;
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

template<typename T>
inline T *grid_track_runtime_allocation(lv_obj_t *owner, T *ptr);

template<typename T>
inline T *grid_delete_with_owner(lv_obj_t *owner, T *ptr);

inline AlarmActionCtx *grid_delete_alarm_action_with_owner(
    lv_obj_t *owner, AlarmActionCtx *ctx);
inline AlarmActionCtx *grid_track_alarm_action_runtime(
    lv_obj_t *owner, AlarmActionCtx *ctx);
inline FanCardCtx *grid_delete_fan_card_with_owner(
    lv_obj_t *owner, FanCardCtx *ctx);
inline FanCardCtx *grid_track_fan_card_runtime(
    lv_obj_t *owner, FanCardCtx *ctx);
inline void refresh_slider_card_layout(BtnSlot &slot);
inline bool bind_garage_status_card(
    BtnSlot &slot, const ParsedCfg &config,
    TransientStatusLabel **status_label_out);
inline bool bind_gate_status_card(
    BtnSlot &slot, const ParsedCfg &config,
    TransientStatusLabel **status_label_out);
inline LockCardCtx *bind_lock_status_card(
    BtnSlot &slot, const ParsedCfg &config,
    TransientStatusLabel **status_label_out);
inline TransientStatusLabel *grid_track_transient_status_label_runtime(
    lv_obj_t *owner, TransientStatusLabel *ctx);
inline TransientStatusLabel *grid_delete_transient_status_label_with_owner(
    lv_obj_t *owner, TransientStatusLabel *ctx);

#include "button_grid_status_entity_driver.h"

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

#include "button_grid_date_time_driver.h"
#include "button_grid_sensor_driver.h"
#include "button_grid_weather_driver.h"
#include "button_grid_basic_action_driver.h"
#include "button_grid_numeric_selectable_driver.h"
#include "button_grid_cleaning_driver.h"
#include "button_grid_access_cover_driver.h"
#include "button_grid_navigation_driver.h"
#include "button_grid_image_driver.h"

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

inline bool info_only_hidden_card_type(const espcontrol::cards::Context &context) {
  return !card_runtime_information_only(context);
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
    ctx->cover_art->begin_display_takeover = nullptr;
    ctx->cover_art->end_display_takeover = nullptr;
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
  ImageCardCtx *art = acquire_image_card_context(cfg, p.entity);
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
  art->begin_display_takeover = cfg.begin_display_takeover;
  art->end_display_takeover = cfg.end_display_takeover;
  art->modal_fit = false;
  art->media_artwork = true;
  art->media_overlay = overlay;
  art->pending_fallback_picture.clear();
  art->media_artwork_retry_mask = 0;
  art->diagnostics_enabled = cfg.image_card_diagnostics;
  art->retry_deadline_ms = esphome::millis() + IMAGE_CARD_STARTUP_RETRY_MS;
  art->width_compensation_percent = cfg.width_compensation_percent;
  art->media_artwork_width_compensation_percent = cfg.media_artwork_width_compensation_percent;
  media_ctx->cover_art = art;
  media_ctx->cover_overlay = overlay;
  if (image_only && media_ctx->btn) lv_obj_set_user_data(media_ctx->btn, art);
  if (art->image_ready) image_card_set_widget_source(img, art->image);
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
  image_card_request_media_artwork(art);
}

inline void setup_card_visual(BtnSlot &s, const ParsedCfg &p,
                              const espcontrol::cards::Context &context,
                              const GridConfig &cfg,
                              const CardPalette &palette,
                              int row_span = 1,
                              int col_span = 1) {
  const DisplayProfile display = display_profile_from_grid_config(cfg);
  const auto family = context.family;
  if (context.legacy_dispatch) {
    ESP_LOGD("card_runtime", "Legacy setup fallback: surface=%s type=%s driver=%u",
             context.surface == espcontrol::cards::Surface::SUBPAGE ? "subpage" : "main",
             p.type.c_str(), static_cast<unsigned>(context.runtime.driver));
  }
  espcontrol::cards::status_entity_driver_cleanup(s, p, context);
  espcontrol::cards::date_time_driver_cleanup(s, p, context);
  espcontrol::cards::sensor_driver_cleanup(s, p, context);
  espcontrol::cards::weather_driver_cleanup(s, p, context);
  espcontrol::cards::basic_action_driver_cleanup(s, p, context);
  espcontrol::cards::numeric_selectable_driver_cleanup(s, p, context);
  espcontrol::cards::cleaning_driver_cleanup(s, p, context);
  espcontrol::cards::access_cover_driver_cleanup(s, p, context);
  espcontrol::cards::navigation_driver_cleanup(s, p, context);
  espcontrol::cards::image_driver_cleanup(s, p, context);
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

  if (cfg.info_only && info_only_hidden_card_type(context)) {
    lv_obj_add_flag(s.btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
    return;
  }

  screen_lock_register_controlled_button(s.btn);

  if (espcontrol::cards::image_driver_setup_visual(s, p, context)) {
    espcontrol::cards::image_driver_attach_interaction(s, p, context);
    espcontrol::cards::image_driver_refresh_layout(s, p, context);
    return;
  }
  if (espcontrol::cards::sensor_driver_setup_visual(
        s, p, context, palette)) {
    espcontrol::cards::sensor_driver_attach_interaction(s, p, context);
    espcontrol::cards::sensor_driver_refresh_layout(
      s, p, context, display, row_span, col_span);
    return;
  }
  if (espcontrol::cards::status_entity_driver_setup_visual(
        s, p, context, palette)) {
    espcontrol::cards::status_entity_driver_attach_interaction(s, p, context);
    espcontrol::cards::status_entity_driver_refresh_layout(
      s, p, context, row_span, col_span);
    return;
  }
  if (espcontrol::cards::date_time_driver_setup_visual(
        s, p, context, palette)) {
    espcontrol::cards::date_time_driver_attach_interaction(s, p, context);
    espcontrol::cards::date_time_driver_refresh_layout(
      s, p, context, display, row_span, col_span);
    return;
  }
  if (espcontrol::cards::weather_driver_setup_visual(
        s, p, context, palette, display)) {
    espcontrol::cards::weather_driver_attach_interaction(s, p, context);
    espcontrol::cards::weather_driver_refresh_layout(
      s, p, context, display, row_span, col_span);
    return;
  }
  if (espcontrol::cards::basic_action_driver_setup_visual(s, p, context)) {
    espcontrol::cards::basic_action_driver_attach_interaction(s, p, context);
    espcontrol::cards::basic_action_driver_refresh_layout(
      s, p, context, display, row_span, col_span);
    return;
  }
  if (espcontrol::cards::numeric_selectable_driver_setup_visual(
        s, p, context, palette, display)) {
    espcontrol::cards::numeric_selectable_driver_attach_interaction(
      s, p, context);
    espcontrol::cards::numeric_selectable_driver_refresh_layout(
      s, p, context);
    return;
  }
  if (espcontrol::cards::cleaning_driver_setup_visual(s, p, context)) {
    espcontrol::cards::cleaning_driver_attach_interaction(s, p, context);
    espcontrol::cards::cleaning_driver_refresh_layout(s, p, context);
    return;
  }
  if (espcontrol::cards::access_cover_driver_setup_visual(
        s, p, context, palette)) {
    espcontrol::cards::access_cover_driver_attach_interaction(
      s, p, context);
    espcontrol::cards::access_cover_driver_refresh_layout(
      s, p, context);
    return;
  }
  if (espcontrol::cards::navigation_driver_setup_visual(
        s, p, context, cfg, display)) {
    espcontrol::cards::navigation_driver_attach_interaction(s, p, context);
    espcontrol::cards::navigation_driver_refresh_layout(s, p, context, cfg);
    return;
  }
  if (family == espcontrol::cards::Family::ALARM) {
    setup_alarm_card(s, p);
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
  if (card_runtime_uses_slider_visual(context)) {
    setup_slider_visual(s, p, palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR);
    return;
  }
  setup_toggle_visual(s, p);
}

inline bool bind_basic_sensor_card(
    BtnSlot &s, const ParsedCfg &p,
    const espcontrol::cards::Context &context, const CardPalette &palette) {
  if (espcontrol::cards::status_entity_driver_bind_data(
        s, p, context, palette)) return true;
  if (espcontrol::cards::date_time_driver_bind_data(s, p, context)) return true;
  if (espcontrol::cards::sensor_driver_bind_data(
        s, p, context, palette)) return true;
  if (espcontrol::cards::weather_driver_bind_data(s, p, context)) return true;
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

// Minimum icon zoom (256 = 100%) so glyphs stay legible in the densest grids.
constexpr int32_t GRID_ICON_MIN_ZOOM = 96;  // ~37.5%

inline void apply_density_scaled_transform(lv_obj_t *obj, int32_t zoom, int32_t comp,
                                           bool vertical) {
  if (!obj) return;
  int32_t comp_axis = comp * zoom / 256;
  lv_obj_set_style_transform_scale_x(obj, vertical ? zoom : comp_axis, LV_PART_MAIN);
  lv_obj_set_style_transform_scale_y(obj, vertical ? comp_axis : zoom, LV_PART_MAIN);
}

// Scale a card's icon and label to match its cell size: 100% at the device's
// default grid, smaller as the grid gets denser, and larger (up to 100%) for
// cards that span multiple cells. Glyphs are only ever downscaled, so they
// stay sharp. The density factor is folded into the width-compensation
// transform already applied to these labels, so call this after
// display_apply_slot_text_width — density wins for the icon and label while
// the other slot elements keep plain width compensation.
inline void apply_card_content_density(BtnSlot &s, const GridConfig &cfg,
                                       const DisplayProfile &display,
                                       int col_span, int row_span) {
  int cols = cfg.cols > 0 ? cfg.cols : 1;
  int slots = bounded_grid_slots(cfg.num_slots);
  int rows = (slots + cols - 1) / cols;
  if (rows < 1) rows = 1;
  int base_cols = cfg.base_cols > 0 ? cfg.base_cols : cols;
  int base_rows = cfg.base_rows > 0 ? cfg.base_rows : rows;
  if (col_span < 1) col_span = 1;
  if (row_span < 1) row_span = 1;
  int32_t zoom_w = (int32_t) base_cols * col_span * 256 / cols;
  int32_t zoom_h = (int32_t) base_rows * row_span * 256 / rows;
  int32_t zoom = zoom_w < zoom_h ? zoom_w : zoom_h;
  if (zoom > 256) zoom = 256;
  if (zoom < GRID_ICON_MIN_ZOOM) zoom = GRID_ICON_MIN_ZOOM;
  int32_t comp = (int32_t) width_compensation_scale(display_main_width_percent(display));
  bool vertical = width_compensation_vertical_axis();
  apply_density_scaled_transform(s.icon_lbl, zoom, comp, vertical);
  // Text legibility degrades faster than icon clarity, so labels shrink at
  // half the icon's rate: blended midway toward full size (icon at 60% ->
  // label at 80%). Spanning cards still cap at 100%.
  int32_t label_zoom = (256 + zoom) / 2;
  apply_density_scaled_transform(s.text_lbl, label_zoom, comp, vertical);
}

inline void refresh_card_layout(BtnSlot &s, const ParsedCfg &p,
                                const GridConfig &cfg,
                                int row_span = 1, int col_span = 1) {
  const DisplayProfile display = display_profile_from_grid_config(cfg);
  const auto context = card_runtime_context(p);
  if (cfg.label_lines > 0) {
    apply_card_label_line_clamp(s.text_lbl, cfg, row_span);
  } else if (cfg.wrap_tall_labels && row_span > 1) {
    lv_label_set_long_mode(s.text_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s.text_lbl, lv_pct(100));
  }
  display_apply_slot_text_width(s, display);
  // Density scaling runs before the driver dispatch below so cards handled by
  // an early-returning driver (e.g. subpage navigation) are scaled too.
  apply_card_content_density(s, cfg, display, col_span, row_span);
  if (espcontrol::cards::navigation_driver_refresh_layout(
        s, p, context, cfg)) return;

  if (espcontrol::cards::numeric_selectable_driver_refresh_layout(
        s, p, context)) return;

  if (espcontrol::cards::image_driver_refresh_layout(
        s, p, context)) {
    return;
  } else if (p.type == "media") {
    refresh_media_card_layout(s, p, cfg, row_span);
  } else {
    espcontrol::cards::access_cover_driver_refresh_layout(
      s, p, card_runtime_context(p));
  }
}

inline void grid_refresh_layout(
    BtnSlot *slots, const GridConfig &cfg,
    const std::string &order_str,
    lv_obj_t *main_page_obj = nullptr) {
  ESP_LOGI("sensors", "Grid refresh: layout start (%lu ms)", esphome::millis());
  set_display_temperature_unit(cfg.temperature_unit, cfg.timezone);
  const DisplayProfile display = display_profile_from_grid_config(cfg);
  display_activate_profile(display);
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
    int col_span = order.col_span[idx - 1] > 0 ? order.col_span[idx - 1] : 1;
    refresh_card_layout(s, p, cfg, row_span, col_span);
    espcontrol::cards::cleaning_driver_refresh_translated_text(
      s, p, card_runtime_context(p));
  }
  espcontrol::cards::cleaning_driver_refresh_subpage_translated_text();
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
  display_activate_profile(display);
  // Clear image references before visual setup removes their old LVGL widgets.
  espcontrol::cards::image_driver_reset_pool(cfg);
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
    const auto context = card_runtime_context(p);
    display_apply_main_width(s.icon_lbl, display);
    display_apply_slot_text_width(s, display);
    setup_card_visual(s, p, context, cfg, palette, row_span, col_span);
    refresh_card_layout(s, p, cfg, row_span, col_span);
  }
  // Build the remaining compiled slots once, kept hidden. Card widgets are only
  // ever constructed here (at boot), so pre-building the whole pool lets the
  // user grow the grid later through the live refresh path with no restart.
  int compiled = cfg.compiled_slots > NS ? cfg.compiled_slots : NS;
  if (compiled > MAX_GRID_SLOTS) compiled = MAX_GRID_SLOTS;
  bool built[MAX_GRID_SLOTS] = {};
  for (int pos = 0; pos < NS; pos++) {
    int idx = order.positions[pos];
    if (idx >= 1 && idx <= compiled) built[idx - 1] = true;
  }
  for (int i = 0; i < compiled; i++) {
    if (built[i]) continue;
    auto &s = slots[i];
    ParsedCfg p = parse_cfg(s.config->state);
    const auto context = card_runtime_context(p);
    setup_card_visual(s, p, context, cfg, palette, 1, 1);
    // setup_card_visual un-hides the button (reset_card_slot_dynamic_children);
    // keep spare slots hidden until the grid grows to activate them. A hidden
    // object takes no input and is skipped by layout, so this alone suffices.
    lv_obj_add_flag(s.btn, LV_OBJ_FLAG_HIDDEN);
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
inline void grid_delete_transient_status_label(TransientStatusLabel *ctx);

inline TransientStatusLabel *grid_delete_transient_status_label_with_owner(
    lv_obj_t *owner, TransientStatusLabel *ctx) {
  if (owner != nullptr && ctx != nullptr) {
    lv_obj_add_event_cb(owner, [](lv_event_t *event) {
      grid_delete_transient_status_label(
        static_cast<TransientStatusLabel *>(
          lv_event_get_user_data(event)));
    }, LV_EVENT_DELETE, ctx);
  }
  return ctx;
}

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

inline FanCardCtx *grid_delete_fan_card_with_owner(lv_obj_t *owner,
                                                   FanCardCtx *ctx) {
  if (owner != nullptr && ctx != nullptr) {
    lv_obj_add_event_cb(owner, [](lv_event_t *e) {
      FanCardCtx *fan = static_cast<FanCardCtx *>(lv_event_get_user_data(e));
      if (fan != nullptr) {
        grid_delete_transient_status_label(fan->status_label);
        fan->status_label = nullptr;
        delete fan;
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

inline void grid_clear_navigation_targets(BtnSlot *slots, int slot_count) {
  if (slots == nullptr) return;
  for (int i = 0; i < slot_count; i++) {
    ParsedCfg p = parse_cfg(slots[i].config->state);
    const auto context = card_runtime_context(p);
    espcontrol::cards::navigation_driver_cleanup(slots[i], p, context);
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
  display_activate_profile(display);
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

  static std::string sp_entity_ids[MAX_SUBPAGE_ITEMS];
  static int sp_entity_alloc_idx = 0;
  static espcontrol::cards::NavigationDriverChildIndicators
    navigation_child_indicators;
  espcontrol::cards::navigation_driver_reset_child_indicators(
    navigation_child_indicators);
  sp_entity_alloc_idx = 0;
  memset(has_sensor, 0, sizeof(has_sensor));
  memset(sensor_text_mode, 0, sizeof(sensor_text_mode));
  memset(has_icon_on, 0, sizeof(has_icon_on));
  bump_ha_subscription_generation();
  weather_forecast_cancel_pending_requests();
  reset_climate_control_refs();
  clear_internal_relay_watchers();
  grid_release_main_runtime_allocations(slots, NS);
  grid_clear_navigation_targets(slots, NS);
  navigation_clear_home_targets();
  // Image-card contexts may still point at widgets inside subpage screens.
  espcontrol::cards::image_driver_reset_pool(cfg);
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
    const auto context = card_runtime_context(p);
    const auto family = context.family;
    int row_span = order.row_span[idx - 1] > 0 ? order.row_span[idx - 1] : 1;
    int col_span = order.col_span[idx - 1] > 0 ? order.col_span[idx - 1] : 1;
    bool is_1x1_card = card_span_is_single(row_span, col_span);
    if (cfg.info_only && info_only_hidden_card_type(context)) continue;
    navigation_register_home_target(idx, pos, p.label, scfg, s.btn);
    if (espcontrol::cards::image_driver_bind_main(
          s, p, context, cfg)) continue;
    if (bind_basic_sensor_card(s, p, context, palette)) continue;
    espcontrol::cards::ToggleDriverState toggle_state;
    toggle_state.has_sensor = &has_sensor[idx - 1];
    toggle_state.sensor_text_mode = &sensor_text_mode[idx - 1];
    toggle_state.has_icon_on = &has_icon_on[idx - 1];
    toggle_state.icon_off = &icon_off_cp[idx - 1];
    toggle_state.icon_on = &icon_on_cp[idx - 1];
    if (espcontrol::cards::basic_action_driver_bind_main(
          s, p, context, cfg, palette, display, main_page_obj, COLS,
          toggle_state)) continue;
    if (espcontrol::cards::numeric_selectable_driver_bind_main(
          s, p, context, palette, display)) continue;
    if (espcontrol::cards::cleaning_driver_bind_main(
          s, p, context)) continue;
    if (espcontrol::cards::access_cover_driver_bind_main(
          s, p, context)) continue;
    espcontrol::cards::NavigationDriverParentState navigation_state;
    navigation_state.has_sensor = &has_sensor[idx - 1];
    navigation_state.sensor_text_mode = &sensor_text_mode[idx - 1];
    navigation_state.has_icon_on = &has_icon_on[idx - 1];
    navigation_state.icon_off = &icon_off_cp[idx - 1];
    navigation_state.icon_on = &icon_on_cp[idx - 1];
    if (espcontrol::cards::navigation_driver_bind_main(
          s, p, context, navigation_state)) continue;
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
          cfg.begin_display_takeover,
          cfg.end_display_takeover);
        grid_track_alarm_card_runtime(s.btn, ctx);
        lv_obj_set_user_data(s.btn, ctx);
        subscribe_alarm_state(ctx);
        if (p.label.empty() && !ctx->show_status_label)
          subscribe_friendly_name(ctx->status_label, p.entity);
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
            s.sensor_lbl, s.unit_lbl);
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

  for (int si = 0; si < NS; si++) {
    ParsedCfg p = parse_cfg(slots[si].config->state);
    const auto parent_context = card_runtime_context(p);
    if (!espcontrol::cards::navigation_driver_matches(parent_context)) continue;

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
    espcontrol::cards::navigation_driver_own_subpage(
      slots[si], p, parent_context, si + 1, display_order, sub_scr);
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
    display_apply_slot_text_width(back_slot, display);
    apply_card_content_density(back_slot, cfg, display, sp_ord.back_col_span, sp_ord.back_row_span);
    lv_label_set_text(back_slot.icon_lbl, "\U000F0141");
    lv_label_set_text(back_slot.text_lbl, sp_back_label.c_str());

    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
      lv_scr_load_anim((lv_obj_t *)lv_event_get_user_data(e), LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }, LV_EVENT_CLICKED, main_page_obj);
    screen_lock_register_controlled_button(back_btn);

    auto add_parent_indicator = [&](const std::string &entity_id,
                                    bool (*is_active_state)(esphome::StringRef) = is_entity_on_ref) {
      espcontrol::cards::navigation_driver_add_child_indicator(
        navigation_child_indicators, slots[si], si, p, parent_context,
        entity_id, is_active_state);
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
      const auto context = card_runtime_context(
          sb_cfg, espcontrol::cards::Surface::SUBPAGE);
      const auto family = context.family;
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
      display_apply_slot_text_width(sub_slot, display);
      apply_card_content_density(sub_slot, cfg, display, cs, rs);
      setup_card_visual(sub_slot, sb_cfg, context, cfg, palette, rs, cs);

      if (espcontrol::cards::image_driver_bind_subpage(
            sub_slot, sb_cfg, context, cfg)) continue;
      if (bind_basic_sensor_card(sub_slot, sb_cfg, context, palette)) continue;
      espcontrol::cards::BasicActionSubpageEnvironment action_environment;
      action_environment.grid_config = &cfg;
      action_environment.parent_config = &p;
      action_environment.palette = palette;
      action_environment.display = display;
      action_environment.grid_page = sub_scr;
      action_environment.grid_cols = COLS;
      action_environment.add_parent_indicator =
        [&](const std::string &entity_id) { add_parent_indicator(entity_id); };
      action_environment.parent_indicator_enabled =
        espcontrol::cards::navigation_driver_aggregates_child_state(
          p, parent_context);
      action_environment.child_allocation_index =
        &navigation_child_indicators.next_child;
      action_environment.child_capacity = MAX_SUBPAGE_ITEMS;
      action_environment.child_was_on =
        navigation_child_indicators.child_was_on;
      action_environment.parent_btn = slots[si].btn;
      action_environment.parent_icon = slots[si].icon_lbl;
      action_environment.parent_index = si;
      action_environment.parent_has_icon_on =
        espcontrol::cards::navigation_driver_parent_has_alt_icon(
          p, parent_context);
      action_environment.parent_icon_off =
        espcontrol::cards::navigation_driver_parent_icon_off(
          p, parent_context);
      action_environment.parent_icon_on =
        espcontrol::cards::navigation_driver_parent_icon_on(
          p, parent_context);
      action_environment.parent_on_count =
        navigation_child_indicators.parent_on_count;
      if (espcontrol::cards::basic_action_driver_bind_subpage(
            sub_slot, sb_cfg, context, action_environment)) continue;
      espcontrol::cards::NumericSelectableSubpageEnvironment
        numeric_environment;
      numeric_environment.palette = palette;
      numeric_environment.display = display;
      numeric_environment.add_parent_indicator =
        [&](const std::string &entity_id) { add_parent_indicator(entity_id); };
      if (espcontrol::cards::numeric_selectable_driver_bind_subpage(
            sub_slot, sb_cfg, context, numeric_environment)) continue;
      espcontrol::cards::CleaningDriverSubpageEnvironment
        cleaning_environment;
      cleaning_environment.add_parent_indicator =
        [&](const std::string &entity_id) { add_parent_indicator(entity_id); };
      cleaning_environment.add_mower_parent_indicator =
        [&](const std::string &entity_id) {
          add_parent_indicator(entity_id, lawn_mower_state_active_ref);
        };
      if (espcontrol::cards::cleaning_driver_bind_subpage(
            sub_slot, sb_cfg, context, cleaning_environment)) continue;
      espcontrol::cards::AccessCoverDriverSubpageEnvironment
        access_cover_environment;
      access_cover_environment.add_parent_indicator =
        [&](const std::string &entity_id) { add_parent_indicator(entity_id); };
      access_cover_environment.add_toggle_click =
        [&](lv_obj_t *button, const std::string &entity_id, bool checked) {
          add_subpage_toggle_click(button, entity_id, checked);
        };
      if (espcontrol::cards::access_cover_driver_bind_subpage(
            sub_slot, sb_cfg, context, access_cover_environment)) continue;
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
            cfg.begin_display_takeover,
            cfg.end_display_takeover);
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
              sub_slot.sensor_lbl, sub_slot.unit_lbl);
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
