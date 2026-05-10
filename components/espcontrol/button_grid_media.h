#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

inline std::string media_status_text(const std::string &state) {
  if (state == "playing") return "Playing";
  if (state == "paused") return "Paused";
  if (state == "idle") return "Idle";
  if (state == "off") return "Off";
  if (state == "unavailable") return "Unavailable";
  if (state == "unknown" || state.empty()) return "Unknown";
  return sentence_cap_text(state);
}

inline void media_set_metadata_text(lv_obj_t *label, esphome::StringRef value,
                                    const char *fallback) {
  if (!label) return;
  std::string text = string_ref_limited(value, HA_STATE_TEXT_MAX_LEN);
  if (text.empty() || text == "unknown" || text == "unavailable")
    text = fallback ? fallback : "--";
  lv_label_set_text(label, text.c_str());
}

inline bool media_seek_pending_active(SliderCtx *ctx) {
  return ctx && ctx->media_seek_pending &&
         (esphome::millis() - ctx->media_seek_pending_ms) < MEDIA_SEEK_PENDING_TIMEOUT_MS;
}

inline bool media_parse_fixed_int(const char *text, size_t len, size_t pos,
                                  size_t digits, int &out) {
  if (!text || pos + digits > len) return false;
  int value = 0;
  for (size_t i = 0; i < digits; i++) {
    char c = text[pos + i];
    if (c < '0' || c > '9') return false;
    value = value * 10 + (c - '0');
  }
  out = value;
  return true;
}

inline int64_t media_days_from_civil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(year - era * 400);
  const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468;
}

inline bool media_parse_ha_timestamp(esphome::StringRef value, time_t &epoch) {
  std::string text = string_ref_limited(value, 40);
  const char *s = text.c_str();
  size_t len = text.size();
  if (len < 19 || s[4] != '-' || s[7] != '-' || (s[10] != 'T' && s[10] != ' ')) return false;
  int year, month, day, hour, minute, second;
  if (!media_parse_fixed_int(s, len, 0, 4, year) ||
      !media_parse_fixed_int(s, len, 5, 2, month) ||
      !media_parse_fixed_int(s, len, 8, 2, day) ||
      !media_parse_fixed_int(s, len, 11, 2, hour) ||
      !media_parse_fixed_int(s, len, 14, 2, minute) ||
      !media_parse_fixed_int(s, len, 17, 2, second)) {
    return false;
  }
  if (month < 1 || month > 12 || day < 1 || day > 31 ||
      hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
      second < 0 || second > 60) {
    return false;
  }

  size_t tz_pos = 19;
  while (tz_pos < len && s[tz_pos] != 'Z' && s[tz_pos] != '+' && s[tz_pos] != '-') tz_pos++;
  int offset_seconds = 0;
  if (tz_pos < len && (s[tz_pos] == '+' || s[tz_pos] == '-')) {
    int offset_hour, offset_minute;
    if (!media_parse_fixed_int(s, len, tz_pos + 1, 2, offset_hour) ||
        tz_pos + 3 >= len || s[tz_pos + 3] != ':' ||
        !media_parse_fixed_int(s, len, tz_pos + 4, 2, offset_minute) ||
        offset_hour > 23 || offset_minute > 59) {
      return false;
    }
    offset_seconds = (offset_hour * 60 + offset_minute) * 60;
    if (s[tz_pos] == '-') offset_seconds = -offset_seconds;
  }

  int64_t days = media_days_from_civil(year, static_cast<unsigned>(month),
                                       static_cast<unsigned>(day));
  int64_t seconds_since_epoch = days * 86400 + hour * 3600 + minute * 60 + second;
  seconds_since_epoch -= offset_seconds;
  if (seconds_since_epoch < 0) return false;
  epoch = static_cast<time_t>(seconds_since_epoch);
  return true;
}

inline bool media_position_timestamp_ms(esphome::StringRef value, uint32_t &updated_ms) {
  time_t updated_epoch;
  if (!media_parse_ha_timestamp(value, updated_epoch)) return false;
  time_t now_epoch = std::time(nullptr);
  if (now_epoch <= 0 || updated_epoch <= 0 || updated_epoch > now_epoch) return false;
  uint64_t elapsed_ms = static_cast<uint64_t>(now_epoch - updated_epoch) * 1000ULL;
  if (elapsed_ms > 0xFFFFFFFFULL) elapsed_ms = 0xFFFFFFFFULL;
  updated_ms = esphome::millis() - static_cast<uint32_t>(elapsed_ms);
  return true;
}

inline void media_apply_position(SliderCtx *ctx) {
  if (!ctx) return;
  float seconds = ctx->media_position_seconds;
  if (ctx->media_playing && ctx->media_position_updated_ms > 0) {
    uint32_t elapsed_ms = esphome::millis() - ctx->media_position_updated_ms;
    seconds += elapsed_ms / 1000.0f;
  }
  if (ctx->media_duration > 0.0f && seconds > ctx->media_duration) {
    seconds = ctx->media_duration;
  }

  if (ctx->media_value_lbl) {
    char time_buf[16];
    media_format_time(seconds, time_buf, sizeof(time_buf));
    lv_label_set_text(ctx->media_value_lbl, time_buf);
  }

  int pct = 0;
  if (ctx->media_duration > 0.0f) {
    pct = (int)((seconds * 100.0f / ctx->media_duration) + 0.5f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
  }
  if (ctx->media_slider) lv_slider_set_value(ctx->media_slider, pct, LV_ANIM_OFF);
  if (ctx->media_slider && ctx->fill) {
    lv_obj_t *btn = lv_obj_get_parent(ctx->media_slider);
    int fill_pct = ctx->inverted ? 100 - pct : pct;
    slider_update_ctx_fill(ctx, btn, fill_pct);
  }
}

inline void media_set_pending_seek_position(SliderCtx *ctx, int value) {
  if (!ctx || ctx->media_duration <= 0.0f) return;
  if (value < 0) value = 0;
  if (value > 100) value = 100;
  float seconds = ctx->media_duration * value / 100.0f;
  ctx->media_seek_pending = true;
  ctx->media_seek_target_seconds = seconds;
  ctx->media_seek_pending_ms = esphome::millis();
  ctx->media_position_seconds = seconds;
  ctx->media_position_updated_ms = ctx->media_seek_pending_ms;
  media_apply_position(ctx);
}

inline void media_position_timer_cb(lv_timer_t *timer) {
  SliderCtx *ctx = static_cast<SliderCtx *>(lv_timer_get_user_data(timer));
  if (!ctx || !ctx->media_position || !ctx->media_playing) return;
  media_apply_position(ctx);
}

inline void setup_media_action_layout(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                      lv_obj_t *text_lbl,
                                      const ParsedCfg &p) {
  std::string mode = media_card_mode(p.sensor);
  if (icon_lbl) {
    lv_obj_clear_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(icon_lbl, media_default_icon(mode, p.icon));
    lv_obj_align(icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);
  }
  if (text_lbl) {
    std::string label = media_play_pause_show_state(p)
      ? std::string("Paused")
      : media_action_label(p, mode);
    lv_label_set_text(text_lbl, label.c_str());
    lv_obj_align(text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    configure_button_label_wrap(text_lbl);
  }
  apply_push_button_transition(btn);
}

inline void setup_media_now_playing_layout(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                           lv_obj_t *title_lbl,
                                           lv_obj_t *artist_lbl,
                                           const lv_font_t *title_font,
                                           lv_coord_t pad,
                                           bool limit_title_lines,
                                           bool tappable,
                                           lv_coord_t content_inset = 0) {
  constexpr lv_coord_t TITLE_LINE_SPACE = -1;
  lv_coord_t text_width = lv_pct(100);
  if (btn && content_inset > 0) {
    lv_obj_update_layout(btn);
    lv_coord_t available_width = lv_obj_get_width(btn) - content_inset * 2;
    if (available_width > 1) text_width = available_width;
  }
  if (tappable) {
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    apply_push_button_transition(btn);
  } else {
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  }
  if (icon_lbl) lv_obj_add_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
  if (title_lbl) {
    if (title_font) lv_obj_set_style_text_font(title_lbl, title_font, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(title_lbl, TITLE_LINE_SPACE, LV_PART_MAIN);
    if (limit_title_lines) {
      const lv_font_t *font = title_font ? title_font : lv_obj_get_style_text_font(title_lbl, LV_PART_MAIN);
      lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_DOT);
      if (font && font->line_height > 0) {
        lv_obj_set_size(title_lbl, text_width, font->line_height * 2 + TITLE_LINE_SPACE);
      }
      else lv_obj_set_width(title_lbl, text_width);
    } else {
      lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(title_lbl, text_width);
    }
    lv_obj_align(title_lbl, LV_ALIGN_TOP_LEFT, content_inset, content_inset);
    lv_label_set_text(title_lbl, "--");
    lv_obj_move_foreground(title_lbl);
  }
  if (artist_lbl) {
    const lv_font_t *font = lv_obj_get_style_text_font(artist_lbl, LV_PART_MAIN);
    lv_label_set_text(artist_lbl, "--");
    lv_label_set_long_mode(artist_lbl, LV_LABEL_LONG_DOT);
    if (font && font->line_height > 0) lv_obj_set_size(artist_lbl, text_width, font->line_height);
    else lv_obj_set_width(artist_lbl, text_width);
    lv_obj_align(artist_lbl, LV_ALIGN_BOTTOM_LEFT, content_inset, -content_inset);
    lv_obj_move_foreground(artist_lbl);
  }
}

inline lv_obj_t *setup_media_progress_background(lv_obj_t *btn,
                                                 uint32_t progress_color,
                                                 uint32_t background_color,
                                                 const std::string &entity_id) {
  lv_obj_set_style_bg_color(btn, lv_color_hex(background_color), LV_PART_MAIN);
  lv_obj_set_style_bg_color(
    btn, lv_color_hex(background_color),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) |
      static_cast<lv_style_selector_t>(LV_STATE_CHECKED));

  lv_obj_t *slider = setup_slider_widget(btn, progress_color, true);
  lv_obj_t *fill = lv_obj_get_child(btn, 0);

  SliderCtx *ctx = new SliderCtx();
  ctx->entity_id = entity_id;
  ctx->fill = fill;
  ctx->horizontal = true;
  ctx->cover_tilt = false;
  ctx->inverted = false;
  ctx->radius = lv_obj_get_style_radius(btn, LV_PART_MAIN);
  ctx->media_position = true;
  ctx->media_slider = slider;
  lv_obj_set_user_data(slider, (void *)ctx);
  slider_bind_geometry_refresh(btn, slider);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *ctx = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!ctx) return;
    int val = lv_slider_get_value(sl);
    slider_update_ctx_fill(ctx, lv_obj_get_parent(sl), ctx->inverted ? 100 - val : val);
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *ctx = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!ctx || ctx->entity_id.empty() || !ctx->available) return;
    int val = lv_slider_get_value(sl);
    media_set_pending_seek_position(ctx, val);
    send_media_seek_action(ctx->entity_id, val, ctx->media_duration);
  }, LV_EVENT_RELEASED, nullptr);

  ctx->media_timer = lv_timer_create(media_position_timer_cb, 1000, ctx);
  if (ctx->media_timer) lv_timer_pause(ctx->media_timer);
  return slider;
}

inline void setup_media_volume_button(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                      lv_obj_t *sensor_container,
                                      lv_obj_t *sensor_lbl,
                                      lv_obj_t *unit_lbl,
                                      lv_obj_t *text_lbl,
                                      const ParsedCfg &p) {
  if (icon_lbl) {
    lv_obj_add_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
  }
  if (sensor_container) {
    lv_obj_clear_flag(sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(sensor_container, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_move_foreground(sensor_container);
  }
  if (sensor_lbl) {
    lv_label_set_text(sensor_lbl, "--");
  }
  if (unit_lbl) {
    lv_label_set_text(unit_lbl, "");
  }
  if (text_lbl) {
    lv_label_set_text(text_lbl, media_label(p).c_str());
    lv_obj_align(text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    configure_button_label_wrap(text_lbl);
    lv_obj_move_foreground(text_lbl);
  }
  apply_push_button_transition(btn);
}

inline lv_obj_t *setup_media_slider_layout(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                           lv_obj_t *text_lbl, lv_obj_t *value_lbl,
                                           const ParsedCfg &p,
                                           uint32_t on_color,
                                           uint32_t /*track_color*/,
                                           lv_coord_t pad) {
  std::string mode = media_card_mode(p.sensor);
  bool position = mode == "position";
  bool horizontal = true;

  if (position) {
    if (icon_lbl) lv_obj_add_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
    if (value_lbl) {
      lv_label_set_text(value_lbl, "0:00");
      lv_obj_move_foreground(value_lbl);
    }
    if (text_lbl) {
      lv_obj_clear_flag(text_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(text_lbl, media_position_show_state(p) ? "Paused" : media_action_label(p, mode).c_str());
      lv_obj_align(text_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);
      configure_button_label_wrap(text_lbl);
      lv_obj_move_foreground(text_lbl);
    }
  } else {
    if (icon_lbl) {
      lv_obj_clear_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(icon_lbl, media_default_icon(mode, p.icon));
      lv_obj_align(icon_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
      lv_obj_move_foreground(icon_lbl);
    }
    if (text_lbl) {
      lv_label_set_text(text_lbl, media_label(p).c_str());
      lv_obj_align(text_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);
      configure_button_label_wrap(text_lbl);
      lv_obj_move_foreground(text_lbl);
    }
  }

  lv_obj_t *slider = setup_slider_widget(btn, on_color, horizontal);
  lv_obj_t *fill = lv_obj_get_child(btn, 0);
  lv_obj_t *track = nullptr;
  if (position) {
    if (value_lbl) lv_obj_move_foreground(value_lbl);
    if (text_lbl) lv_obj_move_foreground(text_lbl);
  }

  SliderCtx *ctx = new SliderCtx();
  ctx->entity_id = p.entity;
  ctx->fill = fill;
  ctx->horizontal = horizontal;
  ctx->cover_tilt = false;
  ctx->inverted = false;
  ctx->radius = lv_obj_get_style_radius(btn, LV_PART_MAIN);
  ctx->media_position = position;
  ctx->media_slider = slider;
  ctx->media_track_bg = track;
  ctx->media_value_lbl = value_lbl;
  ctx->media_status_lbl = position && media_position_show_state(p) ? text_lbl : nullptr;
  lv_obj_set_user_data(slider, (void *)ctx);
  slider_bind_geometry_refresh(btn, slider);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *ctx = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!ctx) return;
    int val = lv_slider_get_value(sl);
    int fill_val = ctx->inverted ? 100 - val : val;
    slider_update_ctx_fill(ctx, lv_obj_get_parent(sl), fill_val);
    if (ctx->media_position && ctx->media_duration > 0.0f && ctx->media_value_lbl) {
      char time_buf[16];
      media_format_time(ctx->media_duration * val / 100.0f, time_buf, sizeof(time_buf));
      lv_label_set_text(ctx->media_value_lbl, time_buf);
    }
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *ctx = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!ctx || ctx->entity_id.empty()) return;
    if (!ctx->available) return;
    int val = lv_slider_get_value(sl);
    if (ctx->media_position) {
      media_set_pending_seek_position(ctx, val);
      send_media_seek_action(ctx->entity_id, val, ctx->media_duration);
    }
  }, LV_EVENT_RELEASED, nullptr);

  if (position) {
    ctx->media_timer = lv_timer_create(media_position_timer_cb, 1000, ctx);
    if (ctx->media_timer) lv_timer_pause(ctx->media_timer);
  }
  return slider;
}

inline lv_obj_t *setup_media_position_layout(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                             lv_obj_t *text_lbl,
                                             const ParsedCfg &p,
                                             uint32_t progress_color,
                                             uint32_t background_color,
                                             const lv_font_t *value_font,
                                             lv_color_t text_color,
                                             lv_coord_t pad,
                                             int width_compensation_percent = 100) {
  lv_obj_t *value_lbl = lv_label_create(btn);
  if (value_font) lv_obj_set_style_text_font(value_lbl, value_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(value_lbl, text_color, LV_PART_MAIN);
  apply_width_compensation(value_lbl, width_compensation_percent);
  lv_label_set_text(value_lbl, "0:00");
  lv_obj_align(value_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
  lv_obj_set_style_bg_color(btn, lv_color_hex(background_color), LV_PART_MAIN);
  lv_obj_set_style_bg_color(
    btn, lv_color_hex(background_color),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) |
      static_cast<lv_style_selector_t>(LV_STATE_CHECKED));
  return setup_media_slider_layout(
    btn, icon_lbl, text_lbl, value_lbl, p, progress_color, background_color, pad);
}

inline void setup_media_card(BtnSlot &s, const ParsedCfg &p, uint32_t on_color,
                             uint32_t secondary_color,
                             uint32_t tertiary_color,
                             const lv_font_t *sensor_font,
                             const lv_font_t *media_title_font,
                             int width_compensation_percent = 100,
                             int row_span = 1,
                             int col_span = 1) {
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_coord_t pad = lv_obj_get_style_radius(s.btn, LV_PART_MAIN) + 4;
  std::string mode = media_card_mode(p.sensor);
  if (media_playback_button_mode(mode)) {
    setup_media_action_layout(s.btn, s.icon_lbl, s.text_lbl, p);
    return;
  }
  if (mode == "volume") {
    setup_media_volume_button(
      s.btn, s.icon_lbl, s.sensor_container, s.sensor_lbl, s.unit_lbl, s.text_lbl, p);
    return;
  }
  if (mode == "now_playing") {
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_color_t text_color = lv_obj_get_style_text_color(s.sensor_lbl, LV_PART_MAIN);
    MediaNowPlayingCtx *ctx = new MediaNowPlayingCtx();
    ctx->btn = s.btn;
    ctx->play_pause_background = media_now_playing_play_pause_enabled(p);
    if (media_now_playing_progress_enabled(p)) {
      ctx->progress_slider = setup_media_progress_background(s.btn, secondary_color, tertiary_color, p.entity);
    }
    lv_obj_t *title_lbl = lv_label_create(s.btn);
    lv_obj_set_style_text_color(title_lbl, text_color, LV_PART_MAIN);
    apply_width_compensation(title_lbl, width_compensation_percent);
    s.sensor_lbl = title_lbl;
    ctx->title_lbl = title_lbl;
    ctx->artist_lbl = s.text_lbl;
    lv_obj_set_user_data(s.sensor_container, (void *)ctx);
    setup_media_now_playing_layout(
      s.btn, s.icon_lbl, s.sensor_lbl, s.text_lbl, media_title_font, pad,
      row_span == 1, ctx->play_pause_background,
      media_now_playing_progress_enabled(p) ? pad : 0);
    return;
  }
  if (mode == "position") {
    lv_coord_t position_pad = lv_obj_get_style_pad_top(s.btn, LV_PART_MAIN);
    lv_color_t text_color = lv_obj_get_style_text_color(s.sensor_lbl, LV_PART_MAIN);
    lv_obj_t *slider = setup_media_position_layout(
      s.btn, s.icon_lbl, s.text_lbl, p, secondary_color, tertiary_color,
      sensor_font, text_color, position_pad, width_compensation_percent);
    lv_obj_set_user_data(s.sensor_container, (void *)slider);
    return;
  }
  lv_obj_t *slider = setup_media_slider_layout(s.btn, s.icon_lbl, s.text_lbl,
    nullptr, p, on_color, tertiary_color, pad);
  lv_obj_set_user_data(s.sensor_container, (void *)slider);
}

inline void subscribe_media_state(lv_obj_t *btn_ptr,
                                  lv_obj_t *status_lbl,
                                  const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [btn_ptr, status_lbl](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        bool unavailable = ha_state_unavailable_ref(state);
        apply_control_availability(btn_ptr, btn_ptr, !unavailable);
        bool playing = state_text == "playing";
        if (playing) lv_obj_add_state(btn_ptr, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn_ptr, LV_STATE_CHECKED);
        if (status_lbl) {
          std::string label = media_status_text(state_text);
          lv_label_set_text(status_lbl, label.c_str());
        }
      })
  );
}

inline void subscribe_media_slider_state(lv_obj_t *btn_ptr,
                                         lv_obj_t *slider,
                                         const std::string &entity_id);

inline void subscribe_media_now_playing_state(MediaNowPlayingCtx *ctx,
                                              const std::string &entity_id) {
  if (entity_id.empty()) return;
  lv_obj_t *title_lbl = ctx ? ctx->title_lbl : nullptr;
  lv_obj_t *artist_lbl = ctx ? ctx->artist_lbl : nullptr;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("media_title"),
    std::function<void(esphome::StringRef)>(
      [title_lbl](esphome::StringRef title) {
        media_set_metadata_text(title_lbl, title, "--");
      })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("media_artist"),
    std::function<void(esphome::StringRef)>(
      [artist_lbl](esphome::StringRef artist) {
        media_set_metadata_text(artist_lbl, artist, "--");
      })
  );
  if (ctx && ctx->progress_slider) {
    subscribe_media_slider_state(lv_obj_get_parent(ctx->progress_slider), ctx->progress_slider, entity_id);
  }
  if (ctx && ctx->play_pause_background && ctx->btn) {
    subscribe_media_state(ctx->btn, nullptr, entity_id);
  }
}

inline MediaVolumeCtx *create_media_volume_context(lv_obj_t *btn,
                                                   lv_obj_t *label_lbl,
                                                   const ParsedCfg &p,
                                                   uint32_t accent_color,
                                                   uint32_t secondary_color,
                                                   uint32_t tertiary_color,
                                                   const lv_font_t *value_font,
                                                   const lv_font_t *number_font,
                                                   const lv_font_t *unit_font,
                                                   const lv_font_t *label_font,
                                                   const lv_font_t *icon_font,
                                                   int width_compensation_percent = 100,
                                                   lv_obj_t *pct_lbl = nullptr,
                                                   lv_obj_t *unit_lbl = nullptr,
                                                   std::function<void()> pause_home_idle = nullptr,
                                                   std::function<void()> resume_home_idle = nullptr) {
  MediaVolumeCtx *ctx = new MediaVolumeCtx();
  ctx->entity_id = p.entity;
  ctx->label = media_label(p);
  ctx->accent_color = accent_color;
  ctx->secondary_color = secondary_color;
  ctx->tertiary_color = tertiary_color;
  ctx->btn = btn;
  ctx->label_lbl = label_lbl;
  ctx->pct_lbl = pct_lbl;
  ctx->unit_lbl = unit_lbl;
  ctx->width_compensation_percent = normalize_width_compensation_percent(width_compensation_percent);
  ctx->value_font = value_font;
  ctx->number_font = number_font ? number_font : value_font;
  ctx->unit_font = unit_font;
  ctx->label_font = label_font;
  ctx->icon_font = icon_font;
  ctx->pause_home_idle = pause_home_idle;
  ctx->resume_home_idle = resume_home_idle;
  if (btn) lv_obj_set_user_data(btn, ctx);
  return ctx;
}

inline void subscribe_media_volume_state(MediaVolumeCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, {},
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef state) {
        ctx->available = !ha_state_unavailable_ref(state);
        apply_control_availability(ctx->btn, ctx->btn, ctx->available);
        if (!ctx->available) media_volume_hide_modal();
      })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("volume_level"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        float level = 0.0f;
        if (!parse_float_ref(val, level)) return;
        int pct = media_clamp_percent((int)(level * 100.0f + 0.5f));
        if (media_volume_pending_active(ctx)) {
          if (pct != ctx->pending_pct) {
            media_volume_set_modal_value(ctx, ctx->pending_pct);
            return;
          }
          ctx->pending_pct = -1;
          ctx->pending_until_ms = 0;
        } else {
          ctx->pending_pct = -1;
          ctx->pending_until_ms = 0;
        }
        ctx->current_pct = pct;
        media_volume_set_card_value(ctx, pct);
        media_volume_set_modal_value(ctx, pct);
      })
  );
}

inline void subscribe_media_slider_state(lv_obj_t *btn_ptr,
                                         lv_obj_t *slider,
                                         const std::string &entity_id) {
  SliderCtx *ctx = (SliderCtx *)lv_obj_get_user_data(slider);
  if (!ctx) return;

  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [btn_ptr, ctx](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        bool unavailable = ha_state_unavailable_ref(state);
        ctx->available = !unavailable;
        apply_control_availability(
          btn_ptr, ctx->interactive ? ctx->media_slider : nullptr,
          ctx->available, ctx->interactive);
        ctx->media_playing = state_text == "playing";
        if (ctx->media_status_lbl) {
          std::string label = media_status_text(state_text);
          lv_label_set_text(ctx->media_status_lbl, label.c_str());
        }
        if (ctx->media_timer) {
          if (ctx->media_playing) lv_timer_resume(ctx->media_timer);
          else lv_timer_pause(ctx->media_timer);
        }
      })
  );

  if (!ctx->media_position) return;

  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("media_duration"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        float duration = 0.0f;
        if (!parse_float_ref(val, duration) || duration < 0.0f) duration = 0.0f;
        ctx->media_duration = duration;
        media_apply_position(ctx);
      })
  );

  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("media_position"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        float pos = 0.0f;
        if (!parse_float_ref(val, pos) || pos < 0.0f) pos = 0.0f;
        if (media_seek_pending_active(ctx)) {
          if (std::fabs(pos - ctx->media_seek_target_seconds) > MEDIA_SEEK_MATCH_TOLERANCE_SECONDS) {
            media_apply_position(ctx);
            return;
          }
          ctx->media_seek_pending = false;
        } else {
          ctx->media_seek_pending = false;
        }
        ctx->media_position_seconds = pos;
        ctx->media_position_updated_ms = ctx->media_position_updated_at_known
          ? ctx->media_position_updated_at_ms
          : esphome::millis();
        media_apply_position(ctx);
      })
  );

  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("media_position_updated_at"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        if (media_seek_pending_active(ctx)) {
          media_apply_position(ctx);
          return;
        }
        ctx->media_seek_pending = false;
        uint32_t updated_ms = 0;
        if (media_position_timestamp_ms(val, updated_ms)) {
          ctx->media_position_updated_at_known = true;
          ctx->media_position_updated_at_ms = updated_ms;
          ctx->media_position_updated_ms = updated_ms;
        } else {
          ctx->media_position_updated_at_known = false;
          ctx->media_position_updated_at_ms = 0;
          ctx->media_position_updated_ms = esphome::millis();
        }
        media_apply_position(ctx);
      })
  );
}
