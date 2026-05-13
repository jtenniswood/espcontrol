#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

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
  bool color_correction;
  bool wrap_tall_labels;
  int width_compensation_percent = 100;
  int volume_width_compensation_percent = 100;
  const lv_font_t *icon_font;
  const lv_font_t *sp_sensor_font;
  const lv_font_t *sp_large_sensor_font = nullptr;
  int large_sensor_unit_offset_percent = -10;
  const lv_font_t *media_title_font;
  const lv_font_t *volume_number_font;
  const lv_font_t *volume_label_font = nullptr;
  const lv_font_t *volume_icon_font = nullptr;
  std::string temperature_unit;
  std::string timezone;
  bool developer_experimental_features;
  std::function<void()> pause_home_idle;
  std::function<void()> resume_home_idle;
};

inline bool experimental_card_enabled(const ParsedCfg &p, bool developer_experimental_features) {
  if (p.type == "climate") return developer_experimental_features;
  return true;
}

inline bool experimental_card_enabled(const ParsedCfg &p, const GridConfig &cfg) {
  return experimental_card_enabled(p, cfg.developer_experimental_features);
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
  uint32_t off_val = DEFAULT_OFF_COLOR;
  uint32_t sensor_val = DEFAULT_TERTIARY_COLOR;
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

inline void setup_card_visual(BtnSlot &s, const ParsedCfg &p,
                              const GridConfig &cfg,
                              const CardPalette &palette,
                              int row_span = 1,
                              int col_span = 1) {
  apply_button_colors(s.btn, palette.has_on, palette.on_val,
    palette.has_off, palette.off_val);
  if (s.unit_lbl) lv_obj_set_style_translate_y(s.unit_lbl, 0, LV_PART_MAIN);

  if (!experimental_card_enabled(p, cfg)) {
    setup_toggle_visual(s, ParsedCfg{});
    return;
  }
  if (is_text_sensor_card(p)) {
    setup_text_sensor_card(s, p, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (p.type == "sensor") {
    if (p.sensor.empty()) return;
    setup_sensor_card(s, p, palette.has_sensor_color, palette.sensor_val);
    if (row_span == 2 && col_span == 2 &&
        sensor_large_numbers_enabled(p) && cfg.sp_large_sensor_font) {
      apply_large_sensor_number_style(
        s, cfg.sp_large_sensor_font, cfg.large_sensor_unit_offset_percent);
    }
    return;
  }
  if (p.type == "calendar") {
    setup_calendar_card(s, p, palette.has_sensor_color, palette.sensor_val);
    if (row_span == 2 && col_span == 2 &&
        card_large_numbers_enabled(p) && cfg.sp_large_sensor_font) {
      apply_large_sensor_number_style(
        s, cfg.sp_large_sensor_font, cfg.large_sensor_unit_offset_percent);
    }
    return;
  }
  if (p.type == "timezone") {
    setup_timezone_card(s, p, palette.has_sensor_color, palette.sensor_val);
    if (row_span == 2 && col_span == 2 &&
        card_large_numbers_enabled(p) && cfg.sp_large_sensor_font) {
      apply_large_sensor_number_style(
        s, cfg.sp_large_sensor_font, cfg.large_sensor_unit_offset_percent);
    }
    return;
  }
  if (weather_card_shows_forecast(p)) {
    setup_weather_forecast_card(s, p, palette.has_sensor_color, palette.sensor_val,
      cfg.width_compensation_percent);
    if (row_span == 2 && col_span == 2 &&
        card_large_numbers_enabled(p) && cfg.sp_large_sensor_font) {
      apply_large_sensor_number_style(
        s, cfg.sp_large_sensor_font, cfg.large_sensor_unit_offset_percent);
    }
    return;
  }
  if (p.type == "weather") {
    setup_weather_card(s, palette.has_sensor_color, palette.sensor_val);
    return;
  }
  if (p.type == "garage") {
    setup_garage_card(s, p);
    return;
  }
  if (subpage_parent_sensor_state_enabled(p)) {
    setup_subpage_parent_state_card(s, p, cfg.sp_sensor_font);
    return;
  }
  if (p.type == "lock") {
    setup_lock_card(s, p);
    return;
  }
  if (p.type == "cover" && cover_command_mode(p.sensor)) {
    setup_cover_command_card(s, p);
    return;
  }
  if (p.type == "cover" && cover_toggle_mode(p.sensor)) {
    setup_cover_toggle_card(s, p);
    return;
  }
  if (p.type == "internal") {
    setup_internal_relay_card(s, p);
    return;
  }
  if (p.type == "action") {
    setup_action_card(s, p);
    return;
  }
  if (p.type == "media") {
    setup_media_card(s, p,
      palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR,
      palette.has_off ? palette.off_val : DEFAULT_OFF_COLOR,
      palette.has_sensor_color ? palette.sensor_val : DEFAULT_TERTIARY_COLOR,
      cfg.sp_sensor_font,
      cfg.media_title_font ? cfg.media_title_font : cfg.sp_sensor_font,
      cfg.width_compensation_percent,
      row_span, col_span);
    return;
  }
  if (p.type == "climate") {
    setup_climate_control_button(
      s.btn, s.icon_lbl, s.sensor_container, s.sensor_lbl, s.unit_lbl, s.text_lbl, p);
    return;
  }
  if (brightness_slider_type(p.type) || p.type == "cover") {
    setup_slider_visual(s, p, palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR);
    return;
  }
  if (p.type == "light_temperature") {
    setup_light_temp_visual(s, p, palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR);
    return;
  }
  setup_toggle_visual(s, p);
}

inline void refresh_media_card_layout(BtnSlot &s, const ParsedCfg &p,
                                      const GridConfig &cfg,
                                      int row_span = 1) {
  std::string mode = media_card_mode(p.sensor);
  lv_coord_t pad = lv_obj_get_style_radius(s.btn, LV_PART_MAIN) + 4;

  if (mode == "now_playing") {
    MediaNowPlayingCtx *ctx = (MediaNowPlayingCtx *)lv_obj_get_user_data(s.sensor_container);
    if (!ctx) return;
    if (ctx->title_lbl) apply_width_compensation(ctx->title_lbl, cfg.width_compensation_percent);
    if (ctx->artist_lbl) apply_width_compensation(ctx->artist_lbl, cfg.width_compensation_percent);
    setup_media_now_playing_layout(
      s.btn, s.icon_lbl, ctx->title_lbl, ctx->artist_lbl,
      cfg.media_title_font ? cfg.media_title_font : cfg.sp_sensor_font, pad,
      row_span == 1, ctx->play_pause_background,
      ctx->progress_slider ? pad : 0, false);
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
      apply_width_compensation(ctx->media_value_lbl, cfg.width_compensation_percent);
      lv_obj_align(ctx->media_value_lbl, LV_ALIGN_TOP_LEFT, position_pad, position_pad);
      lv_obj_move_foreground(ctx->media_value_lbl);
    }
    if (s.text_lbl) {
      lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, position_pad, -position_pad);
      configure_button_label_wrap(s.text_lbl);
      lv_obj_move_foreground(s.text_lbl);
    }
    if (slider) slider_refresh_geometry(slider);
    if (ctx) media_apply_position(ctx);
    return;
  }

  if (media_playback_button_mode(mode)) {
    if (s.icon_lbl) lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);
    if (s.text_lbl) lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
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
  if (cfg.wrap_tall_labels && row_span > 1) {
    lv_label_set_long_mode(s.text_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s.text_lbl, lv_pct(100));
  }
  apply_width_compensation(s.icon_lbl, cfg.width_compensation_percent);
  apply_slot_text_width_compensation(s, cfg.width_compensation_percent);

  if (p.type == "media") {
    refresh_media_card_layout(s, p, cfg, row_span);
  } else if (brightness_slider_type(p.type) || p.type == "light_temperature" ||
             (p.type == "cover" && !cover_command_mode(p.sensor) && !cover_toggle_mode(p.sensor))) {
    refresh_slider_card_layout(s);
  }
}

inline void grid_refresh_layout(
    BtnSlot *slots, const GridConfig &cfg,
    const std::string &order_str,
    lv_obj_t *main_page_obj = nullptr) {
  ESP_LOGI("sensors", "Grid refresh: layout start (%lu ms)", esphome::millis());
  set_display_temperature_unit(cfg.temperature_unit, cfg.timezone);
  set_width_compensation_vertical_axis(cfg.width_compensation_vertical);
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
    lv_obj_set_grid_cell(s.btn,
      LV_GRID_ALIGN_STRETCH, col, col_span,
      LV_GRID_ALIGN_STRETCH, row, row_span);
  }

  if (main_page_obj) lv_obj_update_layout(main_page_obj);

  for (int pos = 0; pos < NS; pos++) {
    int idx = order.positions[pos];
    if (idx < 1 || idx > NS) continue;
    auto &s = slots[idx - 1];
    ParsedCfg p = parse_cfg(s.config->state);
    int row_span = order.row_span[idx - 1] > 0 ? order.row_span[idx - 1] : 1;
    refresh_card_layout(s, p, cfg, row_span);
  }
  ESP_LOGI("sensors", "Grid refresh: layout done (%lu ms)", esphome::millis());
}

// ── Phase 1: Visual setup ────────────────────────────────────────────

inline void grid_phase1(
    BtnSlot *slots, const GridConfig &cfg,
    const std::string &order_str,
    const std::string &on_hex, const std::string &off_hex,
    const std::string &sensor_hex,
    lv_obj_t *main_page_obj = nullptr) {
  ESP_LOGI("sensors", "Phase 1: visual setup start (%lu ms)", esphome::millis());
  set_display_temperature_unit(cfg.temperature_unit, cfg.timezone);
  set_width_compensation_vertical_axis(cfg.width_compensation_vertical);
  int NS = bounded_grid_slots(cfg.num_slots);
  int COLS = cfg.cols > 0 ? cfg.cols : 1;
  for (int i = 0; i < NS; i++)
    lv_obj_add_flag(slots[i].btn, LV_OBJ_FLAG_HIDDEN);
  configure_grid_layout(main_page_obj, NS, COLS);
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

  bool has_on, has_off, has_sensor_color;
  uint32_t on_val = parse_hex_color(on_hex, has_on);
  uint32_t off_val = parse_hex_color(off_hex, has_off);
  uint32_t sensor_val = parse_hex_color(sensor_hex, has_sensor_color);

  if (cfg.color_correction) {
    if (has_on) on_val = correct_color(on_val);
    if (has_off) off_val = correct_color(off_val);
    if (has_sensor_color) sensor_val = correct_color(sensor_val);
  }

  CardPalette palette;
  palette.has_on = has_on;
  palette.has_off = has_off;
  palette.has_sensor_color = has_sensor_color;
  palette.on_val = has_on ? on_val : DEFAULT_SLIDER_COLOR;
  palette.off_val = has_off ? off_val : DEFAULT_OFF_COLOR;
  palette.sensor_val = has_sensor_color ? sensor_val : DEFAULT_TERTIARY_COLOR;

  reset_calendar_cards();
  reset_timezone_cards();
  reset_weather_forecast_cards();
  reset_climate_control_refs();

  for (int pos = 0; pos < NS; pos++) {
    int idx = order.positions[pos];
    if (idx < 1 || idx > NS) continue;
    auto &s = slots[idx - 1];
    std::string scfg = s.config->state;
    lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_HIDDEN);
    int col = pos % COLS, row = pos / COLS;
    int row_span = order.row_span[idx - 1] > 0 ? order.row_span[idx - 1] : 1;
    int col_span = order.col_span[idx - 1] > 0 ? order.col_span[idx - 1] : 1;
    lv_obj_set_grid_cell(s.btn,
      LV_GRID_ALIGN_STRETCH, col, col_span,
      LV_GRID_ALIGN_STRETCH, row, row_span);

    if (cfg.wrap_tall_labels && row_span > 1) {
      lv_label_set_long_mode(s.text_lbl, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(s.text_lbl, lv_pct(100));
    }

    ParsedCfg p = parse_cfg(scfg);
    apply_width_compensation(s.icon_lbl, cfg.width_compensation_percent);
    apply_slot_text_width_compensation(s, cfg.width_compensation_percent);
    setup_card_visual(s, p, cfg, palette, row_span, col_span);
  }
  ESP_LOGI("sensors", "Phase 1: done (%lu ms)", esphome::millis());
}

// ── Phase 2: HA subscriptions + subpage creation ─────────────────────

inline std::string optional_text_state(esphome::text::Text **configs, int index) {
  return (configs != nullptr && configs[index] != nullptr) ? configs[index]->state : "";
}

inline void grid_phase2(
    BtnSlot *slots, const GridConfig &cfg,
    esphome::text::Text **sp_configs,
    esphome::text::Text **sp_ext_configs,
    esphome::text::Text **sp_ext2_configs,
    esphome::text::Text **sp_ext3_configs,
    const std::string &order_str,
    const std::string &on_hex, const std::string &off_hex,
    const std::string &sensor_hex,
    lv_obj_t *main_page_obj) {
  ESP_LOGI("sensors", "Phase 2: subscriptions + subpages start (%lu ms)", esphome::millis());
  set_display_temperature_unit(cfg.temperature_unit, cfg.timezone);
  set_width_compensation_vertical_axis(cfg.width_compensation_vertical);
  set_switch_confirmation_message_font(
    cfg.media_title_font ? cfg.media_title_font : cfg.volume_label_font);
  set_switch_confirmation_icon_font(cfg.icon_font);
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
  clear_internal_relay_watchers();

  bool has_on, has_off, has_sensor_color;
  uint32_t on_val = parse_hex_color(on_hex, has_on);
  uint32_t off_val = parse_hex_color(off_hex, has_off);
  uint32_t sensor_val = parse_hex_color(sensor_hex, has_sensor_color);

  if (cfg.color_correction) {
    if (has_on) on_val = correct_color(on_val);
    if (has_off) off_val = correct_color(off_val);
    if (has_sensor_color) sensor_val = correct_color(sensor_val);
  }

  CardPalette palette;
  palette.has_on = has_on;
  palette.has_off = has_off;
  palette.has_sensor_color = has_sensor_color;
  palette.on_val = has_on ? on_val : DEFAULT_SLIDER_COLOR;
  palette.off_val = has_off ? off_val : DEFAULT_OFF_COLOR;
  palette.sensor_val = has_sensor_color ? sensor_val : DEFAULT_TERTIARY_COLOR;

  OrderResult parsed;
  parse_order_string(order_str, NS, parsed);
  lv_obj_t *first_card = nullptr;
  if (parsed.positions[0] >= 1 && parsed.positions[0] <= NS) {
    first_card = slots[parsed.positions[0] - 1].btn;
  } else if (NS > 0) {
    first_card = slots[0].btn;
  }
  set_media_home_grid_metrics(main_page_obj, COLS, ROWS, first_card);

  for (int pos = 0; pos < NS; pos++) {
    int idx = parsed.positions[pos];
    if (idx < 1 || idx > NS) continue;
    auto &s = slots[idx - 1];
    std::string scfg = s.config->state;

    ParsedCfg p = parse_cfg(scfg);
    if (!experimental_card_enabled(p, cfg)) continue;
    if (is_text_sensor_card(p)) {
      if (!p.sensor.empty())
        subscribe_text_sensor_value(s.text_lbl, p.sensor);
      continue;
    }
    if (p.type == "sensor") {
      if (p.sensor.empty()) continue;
      subscribe_sensor_value(s.sensor_lbl, p.sensor, parse_precision(p.precision),
        s.unit_lbl, p.unit, s.btn);
      if (p.label.empty())
        subscribe_friendly_name(s.text_lbl, p.sensor);
      continue;
    }
    if (p.type == "calendar") {
      subscribe_calendar_date_source(p.entity);
      continue;
    }
    if (p.type == "timezone") {
      continue;
    }
    if (weather_card_shows_forecast(p)) {
      continue;
    }
    if (p.type == "weather") {
      if (!p.entity.empty())
        subscribe_weather_state(s.icon_lbl, s.text_lbl, p.entity);
      continue;
    }
    if (p.type == "garage") {
      if (!p.entity.empty()) {
        if (garage_command_mode(p.sensor)) {
          subscribe_control_availability(s.btn, s.btn, p.entity);
        } else {
          TransientStatusLabel *status_label = create_transient_status_label(
            s.text_lbl, p.label.empty() ? "Garage Door" : p.label);
          subscribe_garage_state(s.btn, s.icon_lbl, status_label,
            garage_closed_icon(p.icon), garage_open_icon(p.icon_on), p.entity);
          if (p.label.empty())
            subscribe_friendly_name(status_label, p.entity);
        }
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

      subscribe_toggle_state(s.btn, s.icon_lbl, s.sensor_container,
        &has_sensor[idx - 1], &sensor_text_mode[idx - 1],
        &has_icon_on[idx - 1], &icon_off_cp[idx - 1], &icon_on_cp[idx - 1],
        nullptr, p.entity, false);
      continue;
    }
    if (p.type == "lock") {
      if (!p.entity.empty()) {
        if (lock_command_mode(p.sensor)) {
          subscribe_control_availability(s.btn, s.btn, p.entity);
        } else {
          LockCardCtx *ctx = new LockCardCtx();
          ctx->entity_id = p.entity;
          lv_obj_set_user_data(s.btn, ctx);
          TransientStatusLabel *status_label = create_transient_status_label(
            s.text_lbl, p.label.empty() ? "Lock" : p.label);
          subscribe_lock_state(s.btn, s.icon_lbl, status_label,
            lock_locked_icon(p.icon), lock_unlocked_icon(p.icon_on), ctx);
          if (p.label.empty())
            subscribe_friendly_name(status_label, p.entity);
        }
      }
      continue;
    }
    if (p.type == "cover" && cover_command_mode(p.sensor)) {
      if (!p.entity.empty()) {
        if (p.label.empty())
          subscribe_friendly_name(s.text_lbl, p.entity);
        subscribe_control_availability(s.btn, s.btn, p.entity);
      }
      continue;
    }
    if (p.type == "cover" && cover_toggle_mode(p.sensor)) {
      if (!p.entity.empty()) {
        TransientStatusLabel *status_label = create_transient_status_label(
          s.text_lbl, p.label.empty() ? "Cover" : p.label);
        subscribe_cover_toggle_state(s.btn, s.icon_lbl, status_label,
          slider_icon_off(p.type, p.entity, p.icon), slider_icon_on(p.type, p.entity, p.icon, p.icon_on), p.entity);
        if (p.label.empty())
          subscribe_friendly_name(status_label, p.entity);
      }
      continue;
    }
    if (p.type == "internal") {
      if (!p.entity.empty() && !internal_relay_push_mode(p)) {
        bool internal_has_icon_on = !p.icon_on.empty() && p.icon_on != "Auto";
        const char *internal_icon_on = internal_has_icon_on ? find_icon(p.icon_on.c_str()) : nullptr;
        watch_internal_relay_state(p.entity, s.btn, s.icon_lbl,
          internal_has_icon_on, internal_relay_icon(p, false), internal_icon_on);
      }
      continue;
    }
    if (p.type == "action") {
      subscribe_control_availability(s.btn, s.btn, p.entity);
      continue;
    }
    if (p.type == "media") {
      if (!p.entity.empty()) {
        std::string mode = media_card_mode(p.sensor);
        if (mode == "play_pause") {
          subscribe_media_state(s.btn, media_play_pause_show_state(p) ? s.text_lbl : nullptr, p.entity);
        } else if (media_playback_button_mode(mode)) {
          subscribe_control_availability(s.btn, s.btn, p.entity);
          // Previous/next are momentary actions and do not reflect player state.
        } else if (mode == "volume") {
          MediaVolumeCtx *ctx = create_media_volume_context(
            s.btn, s.text_lbl, p, has_on ? on_val : DEFAULT_SLIDER_COLOR,
            has_off ? off_val : DEFAULT_OFF_COLOR,
            has_sensor_color ? sensor_val : DEFAULT_TERTIARY_COLOR,
            cfg.sp_sensor_font,
            cfg.volume_number_font ? cfg.volume_number_font : cfg.sp_sensor_font,
            cfg.volume_label_font
              ? cfg.volume_label_font
              : lv_obj_get_style_text_font(s.unit_lbl, LV_PART_MAIN),
            cfg.volume_label_font
              ? cfg.volume_label_font
              : lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
            cfg.icon_font,
            cfg.volume_width_compensation_percent,
            s.sensor_lbl, s.unit_lbl,
            cfg.pause_home_idle, cfg.resume_home_idle);
          subscribe_media_volume_state(ctx);
          if (p.label.empty()) subscribe_friendly_name(s.text_lbl, p.entity);
        } else if (mode == "now_playing") {
          MediaNowPlayingCtx *ctx = (MediaNowPlayingCtx *)lv_obj_get_user_data(s.sensor_container);
          subscribe_media_now_playing_state(ctx, p.entity);
        } else {
          lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
          if (slider) subscribe_media_slider_state(s.btn, slider, p.entity);
          if (p.label.empty() && mode != "position")
            subscribe_friendly_name(s.text_lbl, p.entity);
        }
      }
      continue;
    }
    if (p.type == "climate") {
      if (!p.entity.empty()) {
        ClimateControlCtx *ctx = create_climate_control_context(
          s.btn, s.icon_lbl, s.text_lbl, p,
          has_on ? on_val : DEFAULT_SLIDER_COLOR,
          has_off ? off_val : DEFAULT_OFF_COLOR,
          has_sensor_color ? sensor_val : DEFAULT_TERTIARY_COLOR,
          cfg.volume_number_font ? cfg.volume_number_font : cfg.sp_sensor_font,
          cfg.volume_label_font
            ? cfg.volume_label_font
            : lv_obj_get_style_text_font(s.unit_lbl, LV_PART_MAIN),
          cfg.volume_label_font
            ? cfg.volume_label_font
            : lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN),
          cfg.icon_font,
          cfg.volume_width_compensation_percent,
          s.sensor_container, s.sensor_lbl, s.unit_lbl);
        subscribe_climate_control_state(ctx);
      }
      continue;
    }

    if (p.entity.empty()) continue;

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
      text_sensor_ctx = new ToggleTextSensorCtx();
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

  // --- Subpage creation ---
  // Heap-allocated grid descriptors (never freed -- display lifetime)
  lv_coord_t *sp_col_dsc = new lv_coord_t[COLS + 1];
  for (int i = 0; i < COLS; i++) sp_col_dsc[i] = LV_GRID_FR(1);
  sp_col_dsc[COLS] = LV_GRID_TEMPLATE_LAST;
  lv_coord_t *sp_row_dsc = new lv_coord_t[ROWS + 1];
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
      optional_text_state(sp_ext3_configs, si);
    if (sp_cfg.empty()) continue;

    auto sp_btns = parse_subpage_config(sp_cfg);
    if (sp_btns.empty()) continue;
    std::string sp_order_str = get_subpage_order(sp_cfg);
    std::string sp_back_label = get_subpage_back_label(sp_order_str);

    SubpageOrder sp_ord;
    parse_subpage_order(sp_order_str, NS, sp_btns.size(), sp_ord);

    lv_obj_t *sub_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(sub_scr, lv_color_black(), LV_PART_MAIN);
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

    lv_obj_t *back_btn = create_grid_card_button(
      sub_scr, sp_radius, sp_pad, sp_btn_fnt, sp_txt_color);
    apply_button_colors(back_btn, false, DEFAULT_SLIDER_COLOR, has_off, off_val);
    lv_obj_set_grid_cell(back_btn, LV_GRID_ALIGN_STRETCH, sp_ord.back_pos % COLS, sp_ord.back_col_span,
      LV_GRID_ALIGN_STRETCH, sp_ord.back_pos / COLS, sp_ord.back_row_span);
    BtnSlot back_slot = create_dynamic_card_slot(
      back_btn, sp_icon_fnt, cfg.sp_sensor_font, sp_btn_fnt, sp_txt_color);
    apply_width_compensation(back_slot.icon_lbl, cfg.width_compensation_percent);
    apply_slot_text_width_compensation(back_slot, cfg.width_compensation_percent);
    lv_label_set_text(back_slot.icon_lbl, "\U000F0141");
    lv_label_set_text(back_slot.text_lbl, sp_back_label.c_str());

    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
      lv_scr_load_anim((lv_obj_t *)lv_event_get_user_data(e), LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }, LV_EVENT_CLICKED, main_page_obj);

    auto add_parent_indicator = [&](const std::string &entity_id) {
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
        sp_icon_off_glyph, sp_icon_on_glyph, sp_on_count);
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
          lv_obj_add_state(target, LV_STATE_CHECKED);
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
      if (!experimental_card_enabled(sb_cfg, cfg)) continue;
      int col, row;
      if (sp_ord.has_back_token) { col = gp % COLS; row = gp / COLS; }
      else { int op = gp + 1; col = op % COLS; row = op / COLS; }
      int rs = sp_ord.row_span[bn - 1] > 0 ? sp_ord.row_span[bn - 1] : 1;

      lv_obj_t *sb_btn = create_grid_card_button(
        sub_scr, sp_radius, sp_pad, sp_btn_fnt, sp_txt_color);
      int cs = sp_ord.col_span[bn - 1] > 0 ? sp_ord.col_span[bn - 1] : 1;
      lv_obj_set_grid_cell(sb_btn, LV_GRID_ALIGN_STRETCH, col, cs, LV_GRID_ALIGN_STRETCH, row, rs);
      BtnSlot sub_slot = create_dynamic_card_slot(
        sb_btn, sp_icon_fnt, cfg.sp_sensor_font, sp_btn_fnt, sp_txt_color);
      apply_width_compensation(sub_slot.icon_lbl, cfg.width_compensation_percent);
      apply_slot_text_width_compensation(sub_slot, cfg.width_compensation_percent);
      setup_card_visual(sub_slot, sb_cfg, cfg, palette, rs, cs);

      if (is_text_sensor_card(sb_cfg)) {
        if (!sb_cfg.sensor.empty())
          subscribe_text_sensor_value(sub_slot.text_lbl, sb_cfg.sensor);
        continue;
      }
      if (sb_cfg.type == "sensor") {
        if (sb_cfg.sensor.empty()) continue;
        subscribe_sensor_value(sub_slot.sensor_lbl, sb_cfg.sensor, parse_precision(sb_cfg.precision),
          sub_slot.unit_lbl, sb_cfg.unit, sub_slot.btn);
        if (sb_cfg.label.empty())
          subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.sensor);
        continue;
      }
      if (sb_cfg.type == "calendar") {
        subscribe_calendar_date_source(sb_cfg.entity);
        continue;
      }
      if (sb_cfg.type == "timezone" || weather_card_shows_forecast(sb_cfg)) {
        continue;
      }
      if (sb_cfg.type == "weather") {
        if (!sb_cfg.entity.empty())
          subscribe_weather_state(sub_slot.icon_lbl, sub_slot.text_lbl, sb_cfg.entity);
        continue;
      }
      if (sb_cfg.type == "cover" && cover_command_mode(sb_cfg.sensor)) {
        if (!sb_cfg.entity.empty()) {
          if (sb_cfg.label.empty())
            subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.entity);
          subscribe_control_availability(sub_slot.btn, sub_slot.btn, sb_cfg.entity);
          ParsedCfg *ctx = new ParsedCfg(sb_cfg);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
            if (c) send_cover_command_action(*c);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (sb_cfg.type == "cover" && cover_toggle_mode(sb_cfg.sensor)) {
        if (!sb_cfg.entity.empty()) {
          TransientStatusLabel *status_label = create_transient_status_label(
            sub_slot.text_lbl, sb_cfg.label.empty() ? "Cover" : sb_cfg.label);
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
            subscribe_control_availability(sub_slot.btn, sub_slot.btn, sb_cfg.entity);
            ParsedCfg *ctx = new ParsedCfg(sb_cfg);
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
              if (c) send_cover_command_action(*c);
            }, LV_EVENT_CLICKED, ctx);
          } else {
            TransientStatusLabel *status_label = create_transient_status_label(
              sub_slot.text_lbl, sb_cfg.label.empty() ? "Garage Door" : sb_cfg.label);
            subscribe_garage_state(sub_slot.btn, sub_slot.icon_lbl, status_label,
              garage_closed_icon(sb_cfg.icon), garage_open_icon(sb_cfg.icon_on), sb_cfg.entity);
            if (sb_cfg.label.empty())
              subscribe_friendly_name(status_label, sb_cfg.entity);
            add_parent_indicator(sb_cfg.entity);
            add_subpage_toggle_click(sb_btn, sb_cfg.entity, true);
          }
        }
        continue;
      }
      if (sb_cfg.type == "lock") {
        if (!sb_cfg.entity.empty()) {
          if (lock_command_mode(sb_cfg.sensor)) {
            subscribe_control_availability(sub_slot.btn, sub_slot.btn, sb_cfg.entity);
            ParsedCfg *ctx = new ParsedCfg(sb_cfg);
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
              if (c) send_lock_command_action(*c);
            }, LV_EVENT_CLICKED, ctx);
          } else {
            LockCardCtx *lock_ctx = new LockCardCtx();
            lock_ctx->entity_id = sb_cfg.entity;
            lv_obj_set_user_data(sb_btn, lock_ctx);
            TransientStatusLabel *status_label = create_transient_status_label(
              sub_slot.text_lbl, sb_cfg.label.empty() ? "Lock" : sb_cfg.label);
            subscribe_lock_state(sub_slot.btn, sub_slot.icon_lbl, status_label,
              lock_locked_icon(sb_cfg.icon), lock_unlocked_icon(sb_cfg.icon_on), lock_ctx);
            if (sb_cfg.label.empty())
              subscribe_friendly_name(status_label, sb_cfg.entity);
            add_parent_indicator(sb_cfg.entity);
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              LockCardCtx *ctx = (LockCardCtx *)lv_event_get_user_data(e);
              if (ctx) send_lock_action(ctx);
            }, LV_EVENT_CLICKED, lock_ctx);
          }
        }
        continue;
      }
      if (sb_cfg.type == "push") {
        std::string push_label = sb_cfg.label.empty() ? "Push" : sb_cfg.label;
        std::string *label = new std::string(push_label);
        lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
          std::string *label = (std::string *)lv_event_get_user_data(e);
          esphome::api::HomeassistantActionRequest req;
          req.service = decltype(req.service)("esphome.push_button_pressed");
          req.is_event = true;
          req.data.init(1);
          auto &kv = req.data.emplace_back();
          kv.key = decltype(kv.key)("label");
          kv.value = decltype(kv.value)(label->c_str());
          esphome::api::global_api_server->send_homeassistant_action(req);
        }, LV_EVENT_CLICKED, label);
        continue;
      }
      if (sb_cfg.type == "action") {
        if (!sb_cfg.entity.empty() && !sb_cfg.sensor.empty()) {
          subscribe_control_availability(sub_slot.btn, sub_slot.btn, sb_cfg.entity);
          ParsedCfg *ctx = new ParsedCfg(sb_cfg);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
            if (c) send_action_card_action(*c);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (sb_cfg.type == "media") {
        std::string mode = media_card_mode(sb_cfg.sensor);
        if (!sb_cfg.entity.empty()) {
          if (media_playback_button_mode(mode)) {
            ParsedCfg *ctx = new ParsedCfg(sb_cfg);
            ctx->sensor = mode;
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
              if (c) send_media_playback_action(c->entity, media_card_mode(c->sensor));
            }, LV_EVENT_CLICKED, ctx);
            if (mode == "play_pause")
              subscribe_media_state(sub_slot.btn,
                media_play_pause_show_state(sb_cfg) ? sub_slot.text_lbl : nullptr,
                sb_cfg.entity);
            else
              subscribe_control_availability(sub_slot.btn, sub_slot.btn, sb_cfg.entity);
          } else if (mode == "volume") {
            MediaVolumeCtx *ctx = create_media_volume_context(
              sub_slot.btn, sub_slot.text_lbl, sb_cfg,
              has_on ? on_val : DEFAULT_SLIDER_COLOR,
              has_off ? off_val : DEFAULT_OFF_COLOR,
              has_sensor_color ? sensor_val : DEFAULT_TERTIARY_COLOR,
              cfg.sp_sensor_font,
              cfg.volume_number_font ? cfg.volume_number_font : cfg.sp_sensor_font,
              cfg.volume_label_font
                ? cfg.volume_label_font
                : lv_obj_get_style_text_font(sub_slot.unit_lbl, LV_PART_MAIN),
              cfg.volume_label_font
                ? cfg.volume_label_font
                : lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
              cfg.icon_font,
              cfg.volume_width_compensation_percent,
              sub_slot.sensor_lbl, sub_slot.unit_lbl,
              cfg.pause_home_idle, cfg.resume_home_idle);
            subscribe_media_volume_state(ctx);
            if (sb_cfg.label.empty()) subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.entity);
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              MediaVolumeCtx *ctx = (MediaVolumeCtx *)lv_event_get_user_data(e);
              if (ctx) media_volume_open_modal(ctx);
            }, LV_EVENT_CLICKED, ctx);
          } else if (mode == "now_playing") {
            MediaNowPlayingCtx *ctx = (MediaNowPlayingCtx *)lv_obj_get_user_data(sub_slot.sensor_container);
            subscribe_media_now_playing_state(ctx, sb_cfg.entity);
            if (media_now_playing_play_pause_enabled(sb_cfg)) {
              ParsedCfg *click_ctx = new ParsedCfg(sb_cfg);
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
      if (sb_cfg.type == "climate") {
        if (!sb_cfg.entity.empty()) {
          ClimateControlCtx *ctx = create_climate_control_context(
            sub_slot.btn, sub_slot.icon_lbl, sub_slot.text_lbl, sb_cfg,
            has_on ? on_val : DEFAULT_SLIDER_COLOR,
            has_off ? off_val : DEFAULT_OFF_COLOR,
            has_sensor_color ? sensor_val : DEFAULT_TERTIARY_COLOR,
            cfg.volume_number_font ? cfg.volume_number_font : cfg.sp_sensor_font,
            cfg.volume_label_font
              ? cfg.volume_label_font
              : lv_obj_get_style_text_font(sub_slot.unit_lbl, LV_PART_MAIN),
            cfg.volume_label_font
              ? cfg.volume_label_font
              : lv_obj_get_style_text_font(sub_slot.text_lbl, LV_PART_MAIN),
            cfg.icon_font,
            cfg.volume_width_compensation_percent,
            sub_slot.sensor_container, sub_slot.sensor_lbl, sub_slot.unit_lbl);
          subscribe_climate_control_state(ctx);
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            ClimateControlCtx *ctx = (ClimateControlCtx *)lv_event_get_user_data(e);
            if (ctx) climate_control_open_modal(ctx);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (sb_cfg.type == "internal") {
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
          InternalRelayClickCtx *ctx = new InternalRelayClickCtx();
          ctx->key = sb_cfg.entity;
          ctx->push_mode = push_mode;
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            InternalRelayClickCtx *c = (InternalRelayClickCtx *)lv_event_get_user_data(e);
            if (c && !c->key.empty()) send_internal_relay_action(c->key, c->push_mode);
          }, LV_EVENT_CLICKED, ctx);
        }
        continue;
      }
      if (sb_cfg.type == "light_temperature") {
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
          std::string *eid = new std::string(sb_cfg.entity);
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
          switch_text_ctx = new ToggleTextSensorCtx();
          switch_text_ctx->text_lbl = sub_slot.text_lbl;
          switch_text_ctx->steady_text = label_text_or_empty(sub_slot.text_lbl);
        }

        if (sb_cfg.label.empty()) {
          if (switch_text_ctx)
            subscribe_friendly_name(switch_text_ctx, sb_cfg.entity);
          else
            subscribe_friendly_name(sub_slot.text_lbl, sb_cfg.entity);
        }

        bool *switch_has_sensor_ptr = new bool(switch_has_sensor);
        bool *switch_sensor_text_ptr = new bool(switch_sensor_text_mode);
        bool *switch_has_icon_on_ptr = new bool(switch_has_icon_on);
        const char **switch_icon_off_ptr = new const char*(switch_icon_off);
        const char **switch_icon_on_ptr = new const char*(switch_icon_on);
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
        ParsedCfg *click_ctx = new ParsedCfg(sb_cfg);
        lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
          ParsedCfg *c = (ParsedCfg *)lv_event_get_user_data(e);
          lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
          if (!c || c->entity.empty()) return;
          if (switch_confirmation_enabled(*c) && target &&
              lv_obj_has_state(target, LV_STATE_CHECKED) &&
              !is_button_entity(c->entity)) {
            switch_confirmation_open_modal(*c, target);
          } else {
            send_toggle_action(c->entity);
          }
        }, LV_EVENT_CLICKED, click_ctx);
      }
    }

    lv_obj_set_user_data(slots[si].btn, (void *)sub_scr);
  }
  refresh_weather_forecast_cards();
  ESP_LOGI("sensors", "Phase 2: done (%lu ms)", esphome::millis());
}

inline void grid_phase2(
    BtnSlot *slots, const GridConfig &cfg,
    esphome::text::Text **sp_configs,
    esphome::text::Text **sp_ext_configs,
    const std::string &order_str,
    const std::string &on_hex, const std::string &off_hex,
    const std::string &sensor_hex,
    lv_obj_t *main_page_obj) {
  grid_phase2(slots, cfg, sp_configs, sp_ext_configs, nullptr, nullptr,
    order_str, on_hex, off_hex, sensor_hex, main_page_obj);
}

// ── Phase 3: Temperature + presence/media subscriptions ───────────────

inline void grid_phase3(
    bool indoor_on, bool outdoor_on,
    const std::string &indoor_entity, const std::string &outdoor_entity,
    float *indoor_temp_ptr, float *outdoor_temp_ptr,
    lv_obj_t *temp_label,
    const std::string &presence_entity,
    bool *presence_detected_ptr,
    const std::string &media_player_entity,
    bool *media_player_playing_ptr,
    std::function<void()> wake_callback,
    std::function<void()> sleep_callback) {
  ESP_LOGI("sensors", "Phase 3: temp/presence/media subscriptions start (%lu ms)", esphome::millis());

  if (indoor_on && outdoor_on) {
    char buf[32];
    format_clock_bar_temperature_pair(buf, sizeof(buf), "-", "-");
    lv_label_set_text(temp_label, buf);
  } else if (indoor_on || outdoor_on) {
    char buf[16];
    format_clock_bar_temperature_single(buf, sizeof(buf), "-");
    lv_label_set_text(temp_label, buf);
  }

  if (indoor_on && !indoor_entity.empty()) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      indoor_entity, {},
      std::function<void(esphome::StringRef)>(
        [indoor_temp_ptr, outdoor_temp_ptr, temp_label](esphome::StringRef state) {
          float val = 0.0f;
          if (parse_float_ref(state, val)) {
            *indoor_temp_ptr = val;
            float outdoor = *outdoor_temp_ptr;
            char buf[40];
            if (std::isnan(outdoor)) {
              char indoor_buf[16];
              format_fixed_decimal(indoor_buf, sizeof(indoor_buf), val, 0);
              format_clock_bar_temperature_single(buf, sizeof(buf), indoor_buf);
            } else {
              char outdoor_buf[16];
              char indoor_buf[16];
              format_fixed_decimal(outdoor_buf, sizeof(outdoor_buf), outdoor, 0);
              format_fixed_decimal(indoor_buf, sizeof(indoor_buf), val, 0);
              format_clock_bar_temperature_pair(buf, sizeof(buf), outdoor_buf, indoor_buf);
            }
            lv_label_set_text(temp_label, buf);
          }
        })
    );
  }

  if (outdoor_on && !outdoor_entity.empty()) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      outdoor_entity, {},
      std::function<void(esphome::StringRef)>(
        [indoor_temp_ptr, outdoor_temp_ptr, temp_label](esphome::StringRef state) {
          float val = 0.0f;
          if (parse_float_ref(state, val)) {
            *outdoor_temp_ptr = val;
            float indoor = *indoor_temp_ptr;
            char buf[40];
            if (std::isnan(indoor)) {
              char outdoor_buf[16];
              format_fixed_decimal(outdoor_buf, sizeof(outdoor_buf), val, 0);
              format_clock_bar_temperature_single(buf, sizeof(buf), outdoor_buf);
            } else {
              char outdoor_buf[16];
              char indoor_buf[16];
              format_fixed_decimal(outdoor_buf, sizeof(outdoor_buf), val, 0);
              format_fixed_decimal(indoor_buf, sizeof(indoor_buf), indoor, 0);
              format_clock_bar_temperature_pair(buf, sizeof(buf), outdoor_buf, indoor_buf);
            }
            lv_label_set_text(temp_label, buf);
          }
        })
    );
  }

  if (!presence_entity.empty()) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      presence_entity, {},
      std::function<void(esphome::StringRef)>(
        [presence_detected_ptr, wake_callback, sleep_callback](esphome::StringRef state) {
          if (state == "on") {
            *presence_detected_ptr = true;
            lv_disp_trig_activity(NULL);
            wake_callback();
          } else if (state == "off") {
            *presence_detected_ptr = false;
            sleep_callback();
          }
        })
    );
  }

  if (!media_player_entity.empty() && media_player_playing_ptr) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      media_player_entity, {},
      std::function<void(esphome::StringRef)>(
        [media_player_playing_ptr](esphome::StringRef state) {
          *media_player_playing_ptr = state == "playing";
        })
    );
  }
  ESP_LOGI("sensors", "Phase 3: done (%lu ms)", esphome::millis());
}
