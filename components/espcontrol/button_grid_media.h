#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

enum class MediaControlTab : uint8_t {
  CONTROLS = 0,
  VOLUME = 1,
};

constexpr lv_coord_t MEDIA_CONTROL_VOLUME_VALUE_Y_REF_PX = -8;

struct MediaControlCtx {
  std::string entity_id;
  std::string label;
  std::string friendly_name;
  std::string state_text = "unknown";
  std::string title;
  std::string artist;
  float duration = 0.0f;
  float position_seconds = 0.0f;
  uint32_t position_updated_ms = 0;
  bool position_updated_at_known = false;
  uint32_t position_updated_at_ms = 0;
  bool seek_pending = false;
  float seek_target_seconds = 0.0f;
  uint32_t seek_pending_ms = 0;
  uint32_t track_position_reset_until_ms = 0;
  int current_pct = 0;
  int max_pct = 100;
  int pending_pct = -1;
  uint32_t pending_until_ms = 0;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t secondary_color = DEFAULT_OFF_COLOR;
  uint32_t tertiary_color = DEFAULT_TERTIARY_COLOR;
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *label_lbl = nullptr;
  lv_obj_t *volume_value_lbl = nullptr;
  lv_obj_t *volume_unit_lbl = nullptr;
  lv_obj_t *volume_container = nullptr;
  const lv_font_t *title_font = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *number_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  int width_compensation_percent = 100;
  lv_timer_t *position_timer = nullptr;
  bool available = true;
  bool playing = false;
  bool label_shows_status = false;
  bool top_shows_volume = false;
  bool dragging_progress = false;
  bool dragging_volume = false;
};

struct MediaControlModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *tab_row = nullptr;
  lv_obj_t *controls_tab = nullptr;
  lv_obj_t *volume_tab = nullptr;
  lv_obj_t *controls_box = nullptr;
  lv_obj_t *volume_box = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *artist_lbl = nullptr;
  lv_obj_t *progress_slider = nullptr;
  lv_obj_t *progress_elapsed_lbl = nullptr;
  lv_obj_t *progress_duration_lbl = nullptr;
  lv_obj_t *previous_btn = nullptr;
  lv_obj_t *play_btn = nullptr;
  lv_obj_t *play_icon_lbl = nullptr;
  lv_obj_t *next_btn = nullptr;
  lv_obj_t *volume_arc = nullptr;
  lv_obj_t *volume_pct_lbl = nullptr;
  lv_obj_t *volume_minus_btn = nullptr;
  lv_obj_t *volume_plus_btn = nullptr;
  MediaControlCtx *active = nullptr;
  MediaControlTab tab = MediaControlTab::CONTROLS;
  bool updating_progress = false;
  bool updating_volume = false;
};

inline MediaControlModalUi &media_control_modal_ui() {
  static MediaControlModalUi ui;
  return ui;
}

inline bool media_control_modal_mode(const std::string &mode) {
  return mode == "control_modal";
}

inline std::string media_status_text(const std::string &state) {
  if (state == "playing") return espcontrol_i18n(std::string("Playing"));
  if (state == "paused") return espcontrol_i18n(std::string("Paused"));
  if (state == "idle") return espcontrol_i18n(std::string("Idle"));
  if (state == "off") return espcontrol_i18n(std::string("Off"));
  if (state == "unavailable") return espcontrol_i18n(std::string("Unavailable"));
  if (state == "unknown" || state.empty()) return espcontrol_i18n(std::string("Unknown"));
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

inline void media_refresh_artist_text(lv_obj_t *artist_lbl,
                                      const std::string &entity_id) {
  if (!artist_lbl || entity_id.empty()) return;
  lv_label_set_text(artist_lbl, "");
  ha_get_attribute(
    entity_id, std::string("media_artist"),
    std::function<void(esphome::StringRef)>(
      [artist_lbl](esphome::StringRef artist) {
        media_set_metadata_text(artist_lbl, artist, "");
      })
  );
}

inline bool media_position_timestamp_ms(esphome::StringRef value, uint32_t &updated_ms);
inline bool media_control_seek_pending_active(MediaControlCtx *ctx);
inline bool media_control_track_position_reset_active(MediaControlCtx *ctx);
inline bool media_control_volume_pending_active(MediaControlCtx *ctx);
inline void media_control_hide_modal();
inline void media_control_layout_modal(MediaControlCtx *ctx);
inline void media_control_refresh_modal(MediaControlCtx *ctx);
inline void media_control_refresh_progress(MediaControlCtx *ctx);
inline void media_control_refresh_volume(MediaControlCtx *ctx);
inline void media_control_set_volume_value(MediaControlCtx *ctx, int pct);
inline int media_control_clamp_volume(MediaControlCtx *ctx, int pct);

inline void media_control_refresh_parent_card(MediaControlCtx *ctx) {
  if (!ctx) return;
  if (ctx->label_lbl) {
    std::string label = ctx->label_shows_status
      ? media_status_text(ctx->state_text)
      : ctx->label;
    lv_label_set_text(ctx->label_lbl, label.c_str());
  }
  if (ctx->top_shows_volume && ctx->volume_value_lbl) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", media_control_clamp_volume(ctx, ctx->current_pct));
    lv_label_set_text(ctx->volume_value_lbl, buf);
    if (ctx->volume_unit_lbl) lv_label_set_text(ctx->volume_unit_lbl, "");
  }
}

inline void subscribe_media_control_state(MediaControlCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  register_ha_control_availability(ctx->btn, ctx->btn);
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef state) {
        ctx->state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        ctx->available = !ha_state_unavailable_ref(state);
        ctx->playing = ctx->state_text == "playing";
        apply_control_availability(ctx->btn, ctx->btn, ctx->available);
        set_card_checked_state(ctx->btn, ctx->available && ctx->playing);
        media_control_refresh_parent_card(ctx);
        if (ctx->position_timer) {
          if (ctx->playing) lv_timer_resume(ctx->position_timer);
          else lv_timer_pause(ctx->position_timer);
        }
        MediaControlModalUi &ui = media_control_modal_ui();
        if (ui.active == ctx && !ctx->available) {
          media_control_hide_modal();
        } else if (ui.active == ctx) {
          media_control_refresh_modal(ctx);
        }
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("media_title"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef title) {
        std::string text = string_ref_limited(title, HA_STATE_TEXT_MAX_LEN);
        if (text == "unknown" || text == "unavailable") text.clear();
        ctx->title = text;
        if (media_control_modal_ui().active == ctx) media_control_layout_modal(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("media_artist"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef artist) {
        std::string text = string_ref_limited(artist, HA_STATE_TEXT_MAX_LEN);
        if (text == "unknown" || text == "unavailable") text.clear();
        ctx->artist = text;
        if (media_control_modal_ui().active == ctx) media_control_layout_modal(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("media_duration"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        float duration = 0.0f;
        if (!parse_float_ref(val, duration) || duration < 0.0f) duration = 0.0f;
        ctx->duration = duration;
        if (media_control_modal_ui().active == ctx) media_control_refresh_progress(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("media_position"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        float pos = 0.0f;
        if (!parse_float_ref(val, pos) || pos < 0.0f) pos = 0.0f;
        if (media_control_track_position_reset_active(ctx)) {
          if (pos > MEDIA_SEEK_MATCH_TOLERANCE_SECONDS) {
            media_control_refresh_progress(ctx);
            return;
          }
          ctx->track_position_reset_until_ms = 0;
        }
        if (media_control_seek_pending_active(ctx)) {
          if (std::fabs(pos - ctx->seek_target_seconds) > MEDIA_SEEK_MATCH_TOLERANCE_SECONDS) {
            media_control_refresh_progress(ctx);
            return;
          }
          ctx->seek_pending = false;
        } else {
          ctx->seek_pending = false;
        }
        ctx->position_seconds = pos;
        ctx->position_updated_ms = ctx->position_updated_at_known
          ? ctx->position_updated_at_ms
          : esphome::millis();
        if (media_control_modal_ui().active == ctx) media_control_refresh_progress(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("media_position_updated_at"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        if (media_control_track_position_reset_active(ctx)) {
          media_control_refresh_progress(ctx);
          return;
        }
        if (media_control_seek_pending_active(ctx)) {
          media_control_refresh_progress(ctx);
          return;
        }
        ctx->seek_pending = false;
        uint32_t updated_ms = 0;
        if (media_position_timestamp_ms(val, updated_ms)) {
          ctx->position_updated_at_known = true;
          ctx->position_updated_at_ms = updated_ms;
          ctx->position_updated_ms = updated_ms;
        } else {
          ctx->position_updated_at_known = false;
          ctx->position_updated_at_ms = 0;
          ctx->position_updated_ms = esphome::millis();
        }
        if (media_control_modal_ui().active == ctx) media_control_refresh_progress(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("volume_level"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        float level = 0.0f;
        if (!parse_float_ref(val, level)) return;
        int pct = media_clamp_percent((int)(level * 100.0f + 0.5f));
        if (media_control_volume_pending_active(ctx)) {
          if (pct != ctx->pending_pct) {
            media_control_refresh_volume(ctx);
            return;
          }
          ctx->pending_pct = -1;
          ctx->pending_until_ms = 0;
        } else {
          ctx->pending_pct = -1;
          ctx->pending_until_ms = 0;
        }
        media_control_set_volume_value(ctx, pct);
        media_control_refresh_parent_card(ctx);
      })
  );
  if (ctx->label.empty() || ctx->label == espcontrol_i18n(std::string("Media Control"))) {
    ha_subscribe_attribute(
      ctx->entity_id, std::string("friendly_name"),
      std::function<void(esphome::StringRef value)>(
        [ctx](esphome::StringRef value) {
          ctx->friendly_name = string_ref_limited(value, HA_FRIENDLY_NAME_MAX_LEN);
          if (media_control_modal_ui().active == ctx) media_control_layout_modal(ctx);
        })
    );
  }
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
      ? espcontrol_i18n(std::string("Paused"))
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
                                           lv_coord_t content_inset = 0,
                                           bool reset_text = true) {
  constexpr lv_coord_t TITLE_LINE_SPACE = -1;
  lv_coord_t text_inset = content_inset > 0 ? content_inset : 0;
  lv_coord_t text_width = lv_pct(100);
  if (btn && text_inset > 0) {
    lv_obj_update_layout(btn);
    lv_coord_t available_width = lv_obj_get_width(btn) - text_inset * 2;
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
    lv_obj_align(title_lbl, LV_ALIGN_TOP_LEFT, text_inset, text_inset);
    if (reset_text) lv_label_set_text(title_lbl, "--");
    lv_obj_move_foreground(title_lbl);
  }
  if (artist_lbl) {
    const lv_font_t *font = lv_obj_get_style_text_font(artist_lbl, LV_PART_MAIN);
    if (reset_text) lv_label_set_text(artist_lbl, "");
    lv_label_set_long_mode(artist_lbl, LV_LABEL_LONG_DOT);
    if (font && font->line_height > 0) lv_obj_set_size(artist_lbl, text_width, font->line_height);
    else lv_obj_set_width(artist_lbl, text_width);
    lv_obj_align(artist_lbl, LV_ALIGN_BOTTOM_LEFT, text_inset, -text_inset);
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
      lv_label_set_text(text_lbl, media_position_show_state(p) ? espcontrol_i18n("Paused") : media_action_label(p, mode).c_str());
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
  ctx->content_pad = pad;
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

inline std::string media_control_card_label(const ParsedCfg &p) {
  return p.label.empty() ? espcontrol_i18n(std::string("Media Control")) : p.label;
}

inline void setup_media_control_button(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                       lv_obj_t *sensor_container,
                                       lv_obj_t *sensor_lbl,
                                       lv_obj_t *unit_lbl,
                                       lv_obj_t *text_lbl,
                                       const ParsedCfg &p) {
  bool show_volume = media_control_card_show_volume_number(p);
  if (show_volume) {
    if (icon_lbl) lv_obj_add_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
    if (sensor_container) {
      lv_obj_clear_flag(sensor_container, LV_OBJ_FLAG_HIDDEN);
      lv_obj_align(sensor_container, LV_ALIGN_TOP_LEFT, 0, 0);
      lv_obj_move_foreground(sensor_container);
    }
    if (sensor_lbl) lv_label_set_text(sensor_lbl, "--");
    if (unit_lbl) lv_label_set_text(unit_lbl, "");
  } else if (icon_lbl) {
    lv_obj_clear_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(icon_lbl, media_default_icon("control_modal", p.icon));
    lv_obj_align(icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);
    if (sensor_container) lv_obj_add_flag(sensor_container, LV_OBJ_FLAG_HIDDEN);
  }
  if (text_lbl) {
    std::string label = media_control_card_show_status_label(p)
      ? media_status_text("unknown")
      : media_control_card_label(p);
    lv_label_set_text(text_lbl, label.c_str());
    lv_obj_align(text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    configure_button_label_wrap(text_lbl);
  }
  apply_push_button_transition(btn);
}

inline std::string media_control_title_text(MediaControlCtx *ctx) {
  if (!ctx) return "--";
  if (!ctx->title.empty()) return ctx->title;
  return media_status_text(ctx->state_text);
}

inline std::string media_control_artist_text(MediaControlCtx *ctx) {
  if (!ctx) return "";
  if (!ctx->artist.empty()) return ctx->artist;
  if (!ctx->friendly_name.empty()) return ctx->friendly_name;
  return ctx->label;
}

inline int media_control_volume_max_pct(MediaControlCtx *ctx) {
  if (!ctx) return 100;
  if (ctx->max_pct < 1) return 1;
  if (ctx->max_pct > 100) return 100;
  return ctx->max_pct;
}

inline int media_control_clamp_volume(MediaControlCtx *ctx, int pct) {
  pct = media_clamp_percent(pct);
  int max_pct = media_control_volume_max_pct(ctx);
  return pct > max_pct ? max_pct : pct;
}

inline bool media_control_volume_pending_active(MediaControlCtx *ctx) {
  return ctx && ctx->pending_until_ms != 0 &&
         (int32_t)(ctx->pending_until_ms - esphome::millis()) > 0;
}

inline float media_control_current_position_seconds(MediaControlCtx *ctx) {
  if (!ctx) return 0.0f;
  float seconds = ctx->position_seconds;
  if (ctx->playing && ctx->position_updated_ms > 0) {
    seconds += (esphome::millis() - ctx->position_updated_ms) / 1000.0f;
  }
  if (ctx->duration > 0.0f && seconds > ctx->duration) seconds = ctx->duration;
  if (seconds < 0.0f || !std::isfinite(seconds)) seconds = 0.0f;
  return seconds;
}

inline bool media_control_seek_pending_active(MediaControlCtx *ctx) {
  return ctx && ctx->seek_pending &&
         (esphome::millis() - ctx->seek_pending_ms) < MEDIA_SEEK_PENDING_TIMEOUT_MS;
}

inline bool media_control_track_position_reset_active(MediaControlCtx *ctx) {
  return ctx && ctx->track_position_reset_until_ms != 0 &&
         (int32_t)(ctx->track_position_reset_until_ms - esphome::millis()) > 0;
}

inline void media_control_reset_track_position(MediaControlCtx *ctx) {
  if (!ctx) return;
  uint32_t now = esphome::millis();
  ctx->seek_pending = false;
  ctx->track_position_reset_until_ms = now + MEDIA_SEEK_PENDING_TIMEOUT_MS;
  ctx->position_seconds = 0.0f;
  ctx->position_updated_at_known = false;
  ctx->position_updated_at_ms = 0;
  ctx->position_updated_ms = now;
  media_control_refresh_progress(ctx);
}

inline void media_control_refresh_play_icon(MediaControlCtx *ctx) {
  MediaControlModalUi &ui = media_control_modal_ui();
  if (!ctx || ui.active != ctx || !ui.play_icon_lbl) return;
  lv_label_set_text(ui.play_icon_lbl, ctx->playing ? find_icon("Pause") : find_icon("Play"));
}

inline void media_control_refresh_progress(MediaControlCtx *ctx) {
  MediaControlModalUi &ui = media_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  float seconds = media_control_current_position_seconds(ctx);
  int pct = 0;
  if (ctx->duration > 0.0f) {
    pct = (int)((seconds * 100.0f / ctx->duration) + 0.5f);
  }
  pct = media_clamp_percent(pct);
  if (ui.progress_elapsed_lbl) {
    char buf[16];
    media_format_time(seconds, buf, sizeof(buf));
    lv_label_set_text(ui.progress_elapsed_lbl, buf);
  }
  if (ui.progress_duration_lbl) {
    char buf[16];
    char time_buf[16];
    float remaining_seconds = ctx->duration > 0.0f ? ctx->duration - seconds : 0.0f;
    if (remaining_seconds < 0.0f || !std::isfinite(remaining_seconds)) remaining_seconds = 0.0f;
    media_format_time(remaining_seconds, time_buf, sizeof(time_buf));
    snprintf(buf, sizeof(buf), "-%s", time_buf);
    lv_label_set_text(ui.progress_duration_lbl, buf);
  }
  if (ui.progress_slider && !ctx->dragging_progress) {
    ui.updating_progress = true;
    lv_slider_set_value(ui.progress_slider, pct, LV_ANIM_OFF);
    ui.updating_progress = false;
  }
  if (ui.progress_slider) {
    bool has_duration = ctx->duration > 0.0f && ctx->available;
    apply_control_availability(ui.progress_slider, ui.progress_slider, has_duration);
  }
}

inline void media_control_refresh_volume(MediaControlCtx *ctx) {
  MediaControlModalUi &ui = media_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  int pct = media_control_clamp_volume(ctx, ctx->current_pct);
  if (ui.volume_pct_lbl) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", pct);
    lv_label_set_text(ui.volume_pct_lbl, buf);
  }
  if (ui.volume_arc && !ctx->dragging_volume) {
    ui.updating_volume = true;
    lv_arc_set_range(ui.volume_arc, 0, media_control_volume_max_pct(ctx));
    lv_arc_set_value(ui.volume_arc, pct);
    ui.updating_volume = false;
  }
}

inline void media_control_refresh_modal(MediaControlCtx *ctx) {
  MediaControlModalUi &ui = media_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  std::string title = media_control_title_text(ctx);
  std::string artist = media_control_artist_text(ctx);
  if (ui.title_lbl) lv_label_set_text(ui.title_lbl, title.c_str());
  if (ui.artist_lbl) lv_label_set_text(ui.artist_lbl, artist.c_str());
  apply_control_availability(ui.panel, ui.panel, ctx->available, false);
  media_control_refresh_play_icon(ctx);
  media_control_refresh_progress(ctx);
  media_control_refresh_volume(ctx);
}

inline void media_control_position_timer_cb(lv_timer_t *timer) {
  MediaControlCtx *ctx = static_cast<MediaControlCtx *>(lv_timer_get_user_data(timer));
  if (!ctx || !ctx->playing) return;
  media_control_refresh_progress(ctx);
}

inline void media_control_set_volume_value(MediaControlCtx *ctx, int pct) {
  if (!ctx) return;
  ctx->current_pct = media_control_clamp_volume(ctx, pct);
  media_control_refresh_volume(ctx);
}

inline void media_control_apply_volume_percent(MediaControlCtx *ctx, int pct,
                                               bool from_user, bool send_action) {
  if (!ctx || !ctx->available) return;
  pct = media_control_clamp_volume(ctx, pct);
  ctx->current_pct = pct;
  if (from_user) {
    ctx->pending_pct = pct;
    ctx->pending_until_ms = esphome::millis() + 1500;
  }
  media_control_refresh_volume(ctx);
  if (send_action) send_media_volume_action(ctx->entity_id, pct);
}

inline void media_control_style_tab(lv_obj_t *btn, bool active) {
  if (!btn) return;
  lv_obj_set_style_bg_color(
    btn, lv_color_hex(active ? DARK_TEXT_PRIMARY : DARK_BACKGROUND_TERTIARY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label) {
    lv_obj_set_style_text_color(
      label, lv_color_hex(active ? DEFAULT_TERTIARY_COLOR : DARK_TEXT_PRIMARY), LV_PART_MAIN);
  }
}

inline void media_control_apply_tab_visibility() {
  MediaControlModalUi &ui = media_control_modal_ui();
  bool show_controls = ui.tab == MediaControlTab::CONTROLS;
  bool show_volume = ui.tab == MediaControlTab::VOLUME;
  if (ui.controls_box) {
    if (show_controls) lv_obj_clear_flag(ui.controls_box, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.controls_box, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.volume_box) {
    if (show_volume) lv_obj_clear_flag(ui.volume_box, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.volume_box, LV_OBJ_FLAG_HIDDEN);
  }
  media_control_style_tab(ui.controls_tab, show_controls);
  media_control_style_tab(ui.volume_tab, show_volume);
}

inline void media_control_layout_modal(MediaControlCtx *ctx);

inline lv_obj_t *media_control_create_tab_button(lv_obj_t *parent, const char *icon,
                                                 const lv_font_t *font,
                                                 MediaControlTab tab,
                                                 int width_compensation_percent) {
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  apply_width_compensation(btn, width_compensation_percent);
  lv_obj_set_style_bg_color(btn, lv_color_hex(DARK_BACKGROUND_TERTIARY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
  control_modal_apply_pressed_fill(btn);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *label = lv_label_create(btn);
  if (label) {
    lv_label_set_text(label, icon);
    lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_transform_zoom(label, 180, LV_PART_MAIN);
    light_control_center_icon_label(label);
  }
  lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    MediaControlTab tab = static_cast<MediaControlTab>(
      reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    MediaControlModalUi &ui = media_control_modal_ui();
    ui.tab = tab;
    media_control_apply_tab_visibility();
    media_control_layout_modal(ui.active);
  }, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<uintptr_t>(tab)));
  return btn;
}

inline lv_obj_t *media_control_create_box(lv_obj_t *parent) {
  lv_obj_t *box = lv_obj_create(parent);
  if (!box) return nullptr;
  lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(box, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(box, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
  return box;
}

inline lv_obj_t *media_control_create_icon_button(lv_obj_t *parent, const char *icon,
                                                  const lv_font_t *font) {
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  lv_obj_set_style_bg_color(btn, lv_color_hex(DARK_BACKGROUND_SECONDARY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
  control_modal_apply_pressed_fill(btn);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *label = lv_label_create(btn);
  if (label) {
    lv_label_set_text(label, icon);
    lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_center(label);
  }
  return btn;
}

inline void media_control_style_progress_slider(lv_obj_t *slider, uint32_t background_color,
                                                uint32_t tint_color) {
  if (!slider) return;
  lv_slider_set_range(slider, 0, 100);
  lv_obj_set_style_bg_color(slider, lv_color_hex(background_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lv_color_hex(tint_color), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider, lv_color_hex(tint_color), LV_PART_KNOB);
  lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
  lv_obj_set_style_border_width(slider, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_shadow_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_outline_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_pad_all(slider, 0, LV_PART_KNOB);
}

inline void media_control_layout_modal(MediaControlCtx *ctx) {
  MediaControlModalUi &ui = media_control_modal_ui();
  if (!ctx || !ui.overlay || !ui.panel) return;
  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);
  control_modal_apply_panel_layout(ui.overlay, ui.panel, layout, control_modal_card_radius(ctx->btn));
  control_modal_apply_back_button_layout(ui.back_btn, layout);

  lv_coord_t tab_size = control_modal_prominent_card_tab_size(layout);
  lv_coord_t selected_tab_size = tab_size + tab_size / 8;
  lv_coord_t tab_frame_pad = tab_size / 5;
  lv_coord_t tab_frame_h = tab_size + tab_frame_pad * 2;
  lv_coord_t tab_gap = control_modal_control_tab_gap(layout, tab_size);
  lv_coord_t tabs_total_w = tab_size * 2 + tab_gap;
  lv_coord_t tab_frame_w = tabs_total_w + tab_frame_pad * 2;
  lv_coord_t max_tab_frame_w = layout.panel_w - layout.inset * 3;
  if (tab_frame_w > max_tab_frame_w) tab_frame_w = max_tab_frame_w;
  if (ui.tab_row) {
    lv_obj_set_size(ui.tab_row, tab_frame_w, tab_frame_h);
    lv_obj_set_style_radius(ui.tab_row, tab_frame_h / 2, LV_PART_MAIN);
    lv_obj_align(ui.tab_row, LV_ALIGN_TOP_MID, 0, layout.inset + 2);
  }

  struct MediaControlTabLayout {
    lv_obj_t *btn;
    MediaControlTab tab;
  };
  MediaControlTabLayout tabs[2] = {
    {ui.controls_tab, MediaControlTab::CONTROLS},
    {ui.volume_tab, MediaControlTab::VOLUME},
  };
  lv_coord_t first_tab_x = (tab_frame_w - tabs_total_w) / 2;
  for (int i = 0; i < 2; i++) {
    if (!tabs[i].btn) continue;
    bool active = tabs[i].tab == ui.tab;
    lv_coord_t tab_btn_size = active ? selected_tab_size : tab_size;
    lv_obj_set_size(tabs[i].btn, tab_btn_size, tab_btn_size);
    lv_obj_set_style_radius(tabs[i].btn, tab_btn_size / 2, LV_PART_MAIN);
    lv_coord_t tab_x = first_tab_x + i * (tab_size + tab_gap);
    lv_obj_align(tabs[i].btn, LV_ALIGN_LEFT_MID, tab_x - (tab_btn_size - tab_size) / 2, 0);
    lv_obj_t *label = lv_obj_get_child(tabs[i].btn, 0);
    if (label && control_modal_uses_compact_portrait_tuning(layout)) {
      lv_obj_set_style_transform_zoom(label, 240, LV_PART_MAIN);
    }
    light_control_center_icon_label(label);
  }

  lv_coord_t content_top =
    layout.inset + tab_frame_h + control_modal_prominent_card_tab_content_gap(layout);
  lv_coord_t content_w = layout.panel_w - layout.inset * 2;
  lv_coord_t content_h = layout.panel_h - content_top - layout.inset;
  if (content_h < 180) content_h = layout.panel_h / 2;
  if (ui.controls_box) {
    lv_obj_set_size(ui.controls_box, content_w, content_h);
    lv_obj_align(ui.controls_box, LV_ALIGN_TOP_MID, 0, content_top);
  }
  if (ui.volume_box) {
    lv_obj_set_size(ui.volume_box, content_w, content_h);
    lv_obj_align(ui.volume_box, LV_ALIGN_TOP_MID, 0, content_top);
  }
  if (ui.title_lbl) {
    std::string title = media_control_title_text(ctx);
    lv_label_set_text(ui.title_lbl, title.c_str());
  }
  if (ui.artist_lbl) {
    std::string artist = media_control_artist_text(ctx);
    lv_label_set_text(ui.artist_lbl, artist.c_str());
  }
  const lv_font_t *title_font = ctx->title_font
    ? ctx->title_font
    : (ui.title_lbl ? lv_obj_get_style_text_font(ui.title_lbl, LV_PART_MAIN) : nullptr);
  const lv_font_t *artist_font = ctx->label_font
    ? ctx->label_font
    : (ui.artist_lbl ? lv_obj_get_style_text_font(ui.artist_lbl, LV_PART_MAIN) : nullptr);
  lv_coord_t title_line_h = title_font && title_font->line_height > 0
    ? title_font->line_height : control_modal_scaled_px(28, layout.short_side);
  lv_coord_t artist_h = artist_font && artist_font->line_height > 0
    ? artist_font->line_height : control_modal_scaled_px(24, layout.short_side);
  lv_coord_t title_h = title_line_h;
  lv_coord_t title_max_h = title_line_h * 2;
  lv_coord_t text_w = content_w * 92 / 100;
  lv_coord_t text_gap = control_modal_scaled_px(8, layout.short_side);
  if (text_gap < 6) text_gap = 6;
  lv_coord_t slider_h = control_modal_scaled_px(4, layout.short_side);
  if (slider_h < 3) slider_h = 3;
  lv_coord_t progress_gap = control_modal_scaled_px(12, layout.short_side);
  if (progress_gap < 8) progress_gap = 8;
  lv_coord_t slider_w = content_w * 88 / 100;
  lv_coord_t slider_min_w = control_modal_scaled_px(170, layout.short_side);
  if (slider_w < slider_min_w) slider_w = slider_min_w;
  if (slider_w > content_w) slider_w = content_w;
  const lv_font_t *time_font = ctx->label_font
    ? ctx->label_font
    : (ui.progress_elapsed_lbl ? lv_obj_get_style_text_font(ui.progress_elapsed_lbl, LV_PART_MAIN) : nullptr);
  lv_coord_t time_h = time_font && time_font->line_height > 0
    ? time_font->line_height : control_modal_scaled_px(24, layout.short_side);
  lv_coord_t time_y = content_h - time_h - control_modal_scaled_px(12, layout.short_side);
  lv_coord_t progress_top = time_y - slider_h - progress_gap;
  lv_coord_t btn_gap = control_modal_scaled_px(16, layout.short_side);
  if (btn_gap < 12) btn_gap = 12;
  lv_coord_t btn_size = (content_w - btn_gap * 2) / 3;
  lv_coord_t max_btn = control_modal_scaled_px(86, layout.short_side);
  if (btn_size > max_btn) btn_size = max_btn;
  if (btn_size < 68) btn_size = 68;
  lv_coord_t buttons_total_w = btn_size * 3 + btn_gap * 2;
  lv_coord_t button_start_x = (content_w - buttons_total_w) / 2;
  lv_coord_t button_y = progress_top - btn_size - control_modal_scaled_px(34, layout.short_side);
  if (ui.title_lbl) {
    const char *title_text = lv_label_get_text(ui.title_lbl);
    lv_point_t title_size;
    lv_text_get_size(&title_size, title_text ? title_text : "", title_font, 0, 0,
      text_w, LV_TEXT_FLAG_NONE);
    if (title_size.y > title_h) title_h = title_size.y;
    if (title_h > title_max_h) title_h = title_max_h;
  }
  lv_coord_t base_text_block_h = title_line_h + text_gap + artist_h;
  lv_coord_t title_extra_h = title_h > title_line_h ? title_h - title_line_h : 0;
  lv_coord_t text_block_h = title_h + text_gap + artist_h;
  lv_coord_t text_top = button_y / 2 - base_text_block_h / 2 - title_extra_h;
  lv_coord_t min_text_top = title_extra_h > 0
    ? control_modal_scaled_px(4, layout.short_side)
    : control_modal_scaled_px(22, layout.short_side);
  lv_coord_t button_clearance = control_modal_scaled_px(12, layout.short_side);
  if (button_clearance < 8) button_clearance = 8;
  lv_coord_t max_text_top = button_y - button_clearance - text_block_h;
  if (max_text_top < min_text_top) min_text_top = max_text_top > 0 ? max_text_top : 0;
  if (text_top > max_text_top) text_top = max_text_top;
  if (text_top < min_text_top) text_top = min_text_top;
  if (ui.title_lbl) {
    lv_obj_set_size(ui.title_lbl, text_w, title_h);
    lv_obj_align(ui.title_lbl, LV_ALIGN_TOP_MID, 0, text_top);
  }
  if (ui.artist_lbl) {
    lv_obj_set_size(ui.artist_lbl, text_w, artist_h);
    lv_obj_align(ui.artist_lbl, LV_ALIGN_TOP_MID, 0, text_top + title_h + text_gap);
  }
  if (ui.progress_slider) {
    lv_obj_set_size(ui.progress_slider, slider_w, slider_h);
    lv_obj_set_style_radius(ui.progress_slider, slider_h / 2, LV_PART_MAIN);
    lv_obj_set_style_radius(ui.progress_slider, slider_h / 2, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(ui.progress_slider, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_width(ui.progress_slider, 0, LV_PART_KNOB);
    lv_obj_set_style_height(ui.progress_slider, 0, LV_PART_KNOB);
    lv_obj_align(ui.progress_slider, LV_ALIGN_TOP_MID, 0, progress_top);
  }
  lv_coord_t time_w = slider_w / 2;
  if (ui.progress_elapsed_lbl) {
    lv_obj_set_size(ui.progress_elapsed_lbl, time_w, time_h);
    lv_obj_align(ui.progress_elapsed_lbl, LV_ALIGN_TOP_LEFT, (content_w - slider_w) / 2, time_y);
  }
  if (ui.progress_duration_lbl) {
    lv_obj_set_size(ui.progress_duration_lbl, time_w, time_h);
    lv_obj_align(ui.progress_duration_lbl, LV_ALIGN_TOP_RIGHT, -(content_w - slider_w) / 2, time_y);
  }
  lv_obj_t *buttons[3] = {ui.previous_btn, ui.play_btn, ui.next_btn};
  for (int i = 0; i < 3; i++) {
    if (!buttons[i]) continue;
    lv_obj_set_size(buttons[i], btn_size, btn_size);
    lv_obj_set_style_radius(buttons[i], btn_size / 2, LV_PART_MAIN);
    lv_obj_align(buttons[i], LV_ALIGN_TOP_LEFT, button_start_x + i * (btn_size + btn_gap), button_y);
    lv_obj_t *label = lv_obj_get_child(buttons[i], 0);
    if (label) {
      lv_obj_set_style_transform_zoom(label, 230, LV_PART_MAIN);
      lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }
  }

  if (ui.volume_arc) {
    ControlModalLayout volume_layout = layout;
    volume_layout.panel_w = content_w;
    volume_layout.panel_h = content_h;
    volume_layout.arc_size = content_w < content_h ? content_w : content_h;
    volume_layout.arc_size -= control_modal_scaled_px(18, layout.short_side);
    if (volume_layout.arc_size > layout.arc_size) volume_layout.arc_size = layout.arc_size;
    if (volume_layout.arc_size < 74) volume_layout.arc_size = 74;
    volume_layout.arc_center_x = 0;
    volume_layout.arc_center_y = 0;
    volume_layout.controls_center_y = volume_layout.arc_size / 2 -
      volume_layout.btn_size / 2 - volume_layout.inset +
      control_modal_controls_down_px(volume_layout);
    control_modal_apply_arc_layout(ui.volume_arc, volume_layout, ctx->width_compensation_percent);
    ControlModalLayout volume_buttons_layout = media_volume_step_button_layout(volume_layout);
    volume_buttons_layout.btn_size = volume_buttons_layout.btn_size * 3 / 4;
    if (volume_buttons_layout.btn_size < control_modal_scaled_px(48, layout.short_side)) {
      volume_buttons_layout.btn_size = control_modal_scaled_px(48, layout.short_side);
    }
    volume_buttons_layout.controls_gap = control_modal_scaled_px(12, layout.short_side);
    control_modal_apply_step_buttons_layout(
      ui.volume_minus_btn, ui.volume_plus_btn, volume_buttons_layout);
    if (ui.volume_pct_lbl) {
      apply_width_compensation(ui.volume_pct_lbl, ctx->width_compensation_percent);
      lv_obj_align(ui.volume_pct_lbl, LV_ALIGN_CENTER, 0,
        control_modal_scaled_px(MEDIA_CONTROL_VOLUME_VALUE_Y_REF_PX, volume_layout.short_side));
    }
    lv_obj_update_layout(ui.volume_box);
  }

  media_control_apply_tab_visibility();
  media_control_refresh_modal(ctx);
  lv_obj_move_foreground(ui.back_btn);
}

inline void media_control_hide_modal() {
  MediaControlModalUi &ui = media_control_modal_ui();
  lv_obj_t *overlay = ui.overlay;
  ui = MediaControlModalUi();
  control_modal_delete_overlay(ControlModalKind::MEDIA_CONTROL, overlay);
}

inline MediaControlCtx *create_media_control_context(
    const BtnSlot &s,
    const ParsedCfg &p,
    uint32_t accent_color,
    uint32_t secondary_color,
    uint32_t tertiary_color,
    const lv_font_t *title_font,
    const lv_font_t *label_font,
    const lv_font_t *number_font,
    const lv_font_t *icon_font,
    int width_compensation_percent) {
  MediaControlCtx *ctx = new MediaControlCtx();
  ctx->entity_id = p.entity;
  ctx->label = media_control_card_label(p);
  ctx->max_pct = media_volume_max_percent(p);
  ctx->accent_color = accent_color;
  ctx->secondary_color = secondary_color;
  ctx->tertiary_color = tertiary_color;
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->label_lbl = s.text_lbl;
  ctx->volume_value_lbl = s.sensor_lbl;
  ctx->volume_unit_lbl = s.unit_lbl;
  ctx->volume_container = s.sensor_container;
  ctx->title_font = title_font;
  ctx->label_font = label_font;
  ctx->number_font = number_font;
  ctx->icon_font = icon_font;
  ctx->width_compensation_percent = normalize_width_compensation_percent(width_compensation_percent);
  ctx->label_shows_status = media_control_card_show_status_label(p);
  ctx->top_shows_volume = media_control_card_show_volume_number(p);
  ctx->position_timer = lv_timer_create(media_control_position_timer_cb, 1000, ctx);
  if (ctx->position_timer) lv_timer_pause(ctx->position_timer);
  lv_obj_set_user_data(s.btn, ctx);
  return ctx;
}

inline void media_control_open_modal(MediaControlCtx *ctx) {
  if (!ctx || !ctx->available) return;
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::MEDIA_CONTROL, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, "\U000F0141", false, media_control_hide_modal);
  MediaControlModalUi &ui = media_control_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  ui.tab = MediaControlTab::CONTROLS;
  if (!ui.panel) return;

  ui.tab_row = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_color(ui.tab_row, lv_color_hex(DARK_BACKGROUND_SECONDARY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.tab_row, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.tab_row, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.tab_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.tab_row, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.tab_row, LV_OBJ_FLAG_SCROLLABLE);
  ui.controls_tab = media_control_create_tab_button(
    ui.tab_row, find_icon("Speaker"), ctx->icon_font,
    MediaControlTab::CONTROLS, ctx->width_compensation_percent);
  ui.volume_tab = media_control_create_tab_button(
    ui.tab_row, find_icon("Volume High"), ctx->icon_font,
    MediaControlTab::VOLUME, ctx->width_compensation_percent);

  ui.controls_box = media_control_create_box(ui.panel);
  ui.volume_box = media_control_create_box(ui.panel);

  ui.title_lbl = lv_label_create(ui.controls_box);
  lv_obj_set_style_text_color(ui.title_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.title_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_line_space(ui.title_lbl, 0, LV_PART_MAIN);
  if (ctx->title_font) lv_obj_set_style_text_font(ui.title_lbl, ctx->title_font, LV_PART_MAIN);
  lv_label_set_long_mode(ui.title_lbl, LV_LABEL_LONG_WRAP);
  apply_width_compensation(ui.title_lbl, ctx->width_compensation_percent);

  ui.artist_lbl = lv_label_create(ui.controls_box);
  lv_obj_set_style_text_color(ui.artist_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.artist_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->label_font) lv_obj_set_style_text_font(ui.artist_lbl, ctx->label_font, LV_PART_MAIN);
  lv_label_set_long_mode(ui.artist_lbl, LV_LABEL_LONG_DOT);
  apply_width_compensation(ui.artist_lbl, ctx->width_compensation_percent);

  ui.progress_slider = lv_slider_create(ui.controls_box);
  media_control_style_progress_slider(ui.progress_slider, ctx->secondary_color, ctx->accent_color);
  lv_obj_add_event_cb(ui.progress_slider, [](lv_event_t *e) {
    MediaControlModalUi &ui = media_control_modal_ui();
    if (!ui.active || ui.updating_progress) return;
    if (ui.active) ui.active->dragging_progress = true;
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(ui.progress_slider, [](lv_event_t *) {
    MediaControlModalUi &ui = media_control_modal_ui();
    if (ui.active) ui.active->dragging_progress = true;
  }, LV_EVENT_PRESSED, nullptr);
  lv_obj_add_event_cb(ui.progress_slider, [](lv_event_t *e) {
    MediaControlModalUi &ui = media_control_modal_ui();
    if (!ui.active) return;
    ui.active->dragging_progress = false;
    if (!ui.active->available || ui.active->duration <= 0.0f) return;
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int value = lv_slider_get_value(slider);
    ui.active->seek_pending = true;
    ui.active->seek_target_seconds = ui.active->duration * value / 100.0f;
    ui.active->seek_pending_ms = esphome::millis();
    ui.active->position_seconds = ui.active->seek_target_seconds;
    ui.active->position_updated_ms = ui.active->seek_pending_ms;
    media_control_refresh_progress(ui.active);
    send_media_seek_action(ui.active->entity_id, value, ui.active->duration);
  }, LV_EVENT_RELEASED, nullptr);
  lv_obj_add_event_cb(ui.progress_slider, [](lv_event_t *) {
    MediaControlModalUi &ui = media_control_modal_ui();
    if (ui.active) {
      ui.active->dragging_progress = false;
      media_control_refresh_progress(ui.active);
    }
  }, LV_EVENT_PRESS_LOST, nullptr);

  ui.progress_elapsed_lbl = lv_label_create(ui.controls_box);
  if (ui.progress_elapsed_lbl) {
    lv_label_set_text(ui.progress_elapsed_lbl, "0:00");
    lv_obj_set_style_text_color(ui.progress_elapsed_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui.progress_elapsed_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(ui.progress_elapsed_lbl, ctx->label_font, LV_PART_MAIN);
    lv_label_set_long_mode(ui.progress_elapsed_lbl, LV_LABEL_LONG_CLIP);
    apply_width_compensation(ui.progress_elapsed_lbl, ctx->width_compensation_percent);
  }

  ui.progress_duration_lbl = lv_label_create(ui.controls_box);
  if (ui.progress_duration_lbl) {
    lv_label_set_text(ui.progress_duration_lbl, "0:00");
    lv_obj_set_style_text_color(ui.progress_duration_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui.progress_duration_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(ui.progress_duration_lbl, ctx->label_font, LV_PART_MAIN);
    lv_label_set_long_mode(ui.progress_duration_lbl, LV_LABEL_LONG_CLIP);
    apply_width_compensation(ui.progress_duration_lbl, ctx->width_compensation_percent);
  }

  ui.previous_btn = media_control_create_icon_button(
    ui.controls_box, find_icon("Skip Previous"), ctx->icon_font);
  ui.play_btn = media_control_create_icon_button(
    ui.controls_box, find_icon("Play"), ctx->icon_font);
  ui.play_icon_lbl = ui.play_btn ? lv_obj_get_child(ui.play_btn, 0) : nullptr;
  ui.next_btn = media_control_create_icon_button(
    ui.controls_box, find_icon("Skip Next"), ctx->icon_font);
  lv_obj_add_event_cb(ui.previous_btn, [](lv_event_t *) {
    MediaControlModalUi &ui = media_control_modal_ui();
    if (ui.active && ui.active->available) {
      media_control_reset_track_position(ui.active);
      send_media_playback_action(ui.active->entity_id, "previous");
    }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.play_btn, [](lv_event_t *) {
    MediaControlModalUi &ui = media_control_modal_ui();
    if (ui.active && ui.active->available) send_media_playback_action(ui.active->entity_id, "play_pause");
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.next_btn, [](lv_event_t *) {
    MediaControlModalUi &ui = media_control_modal_ui();
    if (ui.active && ui.active->available) {
      media_control_reset_track_position(ui.active);
      send_media_playback_action(ui.active->entity_id, "next");
    }
  }, LV_EVENT_CLICKED, nullptr);

  ui.volume_arc = lv_arc_create(ui.volume_box);
  if (ui.volume_arc) {
    lv_arc_set_bg_angles(ui.volume_arc, 135, 45);
    lv_arc_set_range(ui.volume_arc, 0, media_control_volume_max_pct(ctx));
    lv_arc_set_value(ui.volume_arc, media_control_clamp_volume(ctx, ctx->current_pct));
    lv_obj_set_style_bg_opa(ui.volume_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui.volume_arc, 0, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ui.volume_arc, lv_color_hex(DARK_TRACK_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_arc_color(ui.volume_arc, lv_color_hex(ctx->accent_color), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(ui.volume_arc, true, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(ui.volume_arc, true, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ui.volume_arc, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_KNOB);
    lv_obj_set_style_border_width(ui.volume_arc, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(ui.volume_arc, 0, LV_PART_KNOB);
    lv_obj_add_flag(ui.volume_arc, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_event_cb(ui.volume_arc, [](lv_event_t *e) {
      MediaControlModalUi &ui = media_control_modal_ui();
      if (!ui.active || ui.updating_volume) return;
      ui.active->dragging_volume = true;
      lv_obj_t *arc = static_cast<lv_obj_t *>(lv_event_get_target(e));
      media_control_apply_volume_percent(ui.active, lv_arc_get_value(arc), true, true);
    }, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(ui.volume_arc, [](lv_event_t *) {
      MediaControlModalUi &ui = media_control_modal_ui();
      if (ui.active) ui.active->dragging_volume = true;
    }, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(ui.volume_arc, [](lv_event_t *) {
      MediaControlModalUi &ui = media_control_modal_ui();
      if (!ui.active) return;
      ui.active->dragging_volume = false;
      media_control_refresh_volume(ui.active);
    }, LV_EVENT_RELEASED, nullptr);
    lv_obj_add_event_cb(ui.volume_arc, [](lv_event_t *) {
      MediaControlModalUi &ui = media_control_modal_ui();
      if (ui.active) {
        ui.active->dragging_volume = false;
        media_control_refresh_volume(ui.active);
      }
    }, LV_EVENT_PRESS_LOST, nullptr);
  }

  ui.volume_pct_lbl = lv_label_create(ui.volume_box);
  if (ui.volume_pct_lbl) {
    lv_label_set_text(ui.volume_pct_lbl, "0");
    lv_obj_set_style_text_color(ui.volume_pct_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui.volume_pct_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (ctx->number_font) lv_obj_set_style_text_font(ui.volume_pct_lbl, ctx->number_font, LV_PART_MAIN);
    apply_width_compensation(ui.volume_pct_lbl, ctx->width_compensation_percent);
  }

  ui.volume_minus_btn = control_modal_create_round_button(
    ui.volume_box, 56, find_icon("Minus"), ctx->icon_font,
    DARK_BORDER, DARK_BACKGROUND_TERTIARY, ctx->width_compensation_percent);
  ui.volume_plus_btn = control_modal_create_round_button(
    ui.volume_box, 56, find_icon("Plus"), ctx->icon_font,
    DARK_BORDER, DARK_BACKGROUND_TERTIARY, ctx->width_compensation_percent);
  if (ui.volume_minus_btn) {
    lv_obj_add_event_cb(ui.volume_minus_btn, [](lv_event_t *) {
      MediaControlModalUi &ui = media_control_modal_ui();
      if (!ui.active) return;
      media_control_apply_volume_percent(ui.active, ui.active->current_pct - 1, true, true);
    }, LV_EVENT_CLICKED, nullptr);
  }
  if (ui.volume_plus_btn) {
    lv_obj_add_event_cb(ui.volume_plus_btn, [](lv_event_t *) {
      MediaControlModalUi &ui = media_control_modal_ui();
      if (!ui.active) return;
      media_control_apply_volume_percent(ui.active, ui.active->current_pct + 1, true, true);
    }, LV_EVENT_CLICKED, nullptr);
  }

  media_control_layout_modal(ctx);
  lv_obj_move_foreground(ui.overlay);
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
  if (mode == "playlist") {
    setup_media_action_layout(s.btn, s.icon_lbl, s.text_lbl, p);
    return;
  }
  if (media_playback_button_mode(mode)) {
    setup_media_action_layout(s.btn, s.icon_lbl, s.text_lbl, p);
    return;
  }
  if (media_control_modal_mode(mode)) {
    setup_media_control_button(
      s.btn, s.icon_lbl, s.sensor_container, s.sensor_lbl, s.unit_lbl, s.text_lbl, p);
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
  register_ha_control_availability(btn_ptr, btn_ptr);
  ha_subscribe_state(
    entity_id,
    std::function<void(esphome::StringRef)>(
      [btn_ptr, status_lbl](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        bool unavailable = ha_state_unavailable_ref(state);
        apply_control_availability(btn_ptr, btn_ptr, !unavailable);
        bool playing = state_text == "playing";
        set_card_checked_state(btn_ptr, playing);
        if (status_lbl) {
          std::string label = media_status_text(state_text);
          lv_label_set_text(status_lbl, label.c_str());
        }
      })
  );
}

inline bool media_playlist_active(const MediaPlaylistCtx *ctx) {
  if (!ctx || !ctx->available || !ctx->playing || ctx->content_id.empty() ||
      !ctx->has_current_content_id) {
    return false;
  }
  if (ctx->current_content_id != ctx->content_id) return false;
  return !ctx->has_current_content_type || ctx->current_content_type.empty() ||
         ctx->current_content_type == ctx->content_type;
}

inline void media_playlist_refresh_checked(MediaPlaylistCtx *ctx) {
  if (!ctx) return;
  set_card_checked_state(ctx->btn, media_playlist_active(ctx));
}

inline MediaPlaylistCtx *create_media_playlist_context(lv_obj_t *btn,
                                                       const ParsedCfg &p) {
  MediaPlaylistCtx *ctx = new MediaPlaylistCtx();
  ctx->btn = btn;
  ctx->entity_id = p.entity;
  ctx->content_id = cfg_option_value(p.options, MEDIA_PLAYLIST_CONTENT_ID_OPTION);
  ctx->content_type = cfg_option_value(p.options, MEDIA_PLAYLIST_CONTENT_TYPE_OPTION);
  if (ctx->content_type.empty()) ctx->content_type = "playlist";
  return ctx;
}

inline void subscribe_media_playlist_state(MediaPlaylistCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  register_ha_control_availability(ctx->btn, ctx->btn);
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        ctx->available = !ha_state_unavailable_ref(state);
        ctx->playing = state_text == "playing";
        apply_control_availability(ctx->btn, ctx->btn, ctx->available);
        media_playlist_refresh_checked(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("media_content_id"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef value) {
        ctx->current_content_id = string_ref_limited(value, HA_STATE_TEXT_MAX_LEN);
        ctx->has_current_content_id =
          !ctx->current_content_id.empty() &&
          ctx->current_content_id != "unknown" &&
          ctx->current_content_id != "unavailable";
        media_playlist_refresh_checked(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("media_content_type"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef value) {
        ctx->current_content_type = string_ref_limited(value, HA_SHORT_STATE_MAX_LEN);
        ctx->has_current_content_type =
          !ctx->current_content_type.empty() &&
          ctx->current_content_type != "unknown" &&
          ctx->current_content_type != "unavailable";
        media_playlist_refresh_checked(ctx);
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
  ha_subscribe_attribute(
    entity_id, std::string("media_title"),
    std::function<void(esphome::StringRef)>(
      [title_lbl, artist_lbl, entity_id](esphome::StringRef title) {
        media_set_metadata_text(title_lbl, title, "--");
        media_refresh_artist_text(artist_lbl, entity_id);
      })
  );
  ha_subscribe_attribute(
    entity_id, std::string("media_artist"),
    std::function<void(esphome::StringRef)>(
      [artist_lbl](esphome::StringRef artist) {
        media_set_metadata_text(artist_lbl, artist, "");
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
                                                   std::function<void()> suspend_display_takeover = nullptr,
                                                   std::function<void()> resume_display_takeover = nullptr) {
  MediaVolumeCtx *ctx = new MediaVolumeCtx();
  ctx->entity_id = p.entity;
  ctx->label = media_label(p);
  ctx->max_pct = media_volume_max_percent(p);
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
  ctx->suspend_display_takeover = suspend_display_takeover;
  ctx->resume_display_takeover = resume_display_takeover;
  if (btn) lv_obj_set_user_data(btn, ctx);
  return ctx;
}

inline void subscribe_media_volume_state(MediaVolumeCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  register_ha_control_availability(ctx->btn, ctx->btn);
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef state) {
        ctx->available = !ha_state_unavailable_ref(state);
        apply_control_availability(ctx->btn, ctx->btn, ctx->available);
        if (!ctx->available) media_volume_hide_modal();
      })
  );
  ha_subscribe_attribute(
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

#ifdef USE_MEDIA_PLAYER
inline void open_device_volume_modal(lv_obj_t *anchor,
                                     esphome::media_player::MediaPlayer *player,
                                     const lv_font_t *value_font,
                                     const lv_font_t *number_font,
                                     const lv_font_t *unit_font,
                                     const lv_font_t *label_font,
                                     const lv_font_t *icon_font,
                                     int width_compensation_percent = 100,
                                     std::function<bool()> mic_muted = nullptr,
                                     std::function<void(bool)> set_mic_muted = nullptr) {
  if (!player) return;
  static MediaVolumeCtx *ctx = nullptr;
  static esphome::media_player::MediaPlayer *subscribed_player = nullptr;
  if (!ctx) {
    ctx = new MediaVolumeCtx();
    ctx->max_pct = 100;
    ctx->accent_color = DEFAULT_SLIDER_COLOR;
    ctx->secondary_color = DEFAULT_OFF_COLOR;
    ctx->tertiary_color = DEFAULT_TERTIARY_COLOR;
  }
  if (subscribed_player != player) {
    subscribed_player = player;
    MediaVolumeCtx *callback_ctx = ctx;
    player->add_on_state_callback([callback_ctx, player](esphome::media_player::MediaPlayerState) {
      int pct = media_clamp_percent((int)(player->volume * 100.0f + 0.5f));
      if (media_volume_pending_active(callback_ctx) && callback_ctx->pending_pct == pct) {
        callback_ctx->pending_pct = -1;
        callback_ctx->pending_until_ms = 0;
      }
      if (!media_volume_pending_active(callback_ctx)) {
        callback_ctx->current_pct = pct;
        media_volume_set_card_value(callback_ctx, pct);
        media_volume_set_modal_value(callback_ctx, pct);
      }
    });
  }
  ctx->entity_id.clear();
  ctx->label = espcontrol_i18n(std::string("Device Volume"));
  ctx->btn = anchor;
  ctx->current_pct = media_clamp_percent((int)(player->volume * 100.0f + 0.5f));
  ctx->pending_pct = -1;
  ctx->pending_until_ms = 0;
  ctx->width_compensation_percent = normalize_width_compensation_percent(width_compensation_percent);
  ctx->value_font = value_font;
  ctx->number_font = number_font ? number_font : value_font;
  ctx->unit_font = unit_font;
  ctx->label_font = label_font;
  ctx->icon_font = icon_font;
  ctx->available = true;
  ctx->mic_muted = mic_muted;
  ctx->set_mic_muted = set_mic_muted;
  ctx->apply_percent = [player](int pct) {
    float volume = media_clamp_percent(pct) / 100.0f;
    player->make_call().set_volume(volume).perform();
  };
  media_volume_open_modal(ctx);
}
#endif

inline void subscribe_media_slider_state(lv_obj_t *btn_ptr,
                                         lv_obj_t *slider,
                                         const std::string &entity_id) {
  SliderCtx *ctx = (SliderCtx *)lv_obj_get_user_data(slider);
  if (!ctx) return;
  register_ha_control_availability(
    btn_ptr, ctx->interactive ? ctx->media_slider : nullptr, ctx->interactive);

  ha_subscribe_state(
    entity_id,
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

  ha_subscribe_attribute(
    entity_id, std::string("media_duration"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        float duration = 0.0f;
        if (!parse_float_ref(val, duration) || duration < 0.0f) duration = 0.0f;
        ctx->media_duration = duration;
        media_apply_position(ctx);
      })
  );

  ha_subscribe_attribute(
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

  ha_subscribe_attribute(
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
