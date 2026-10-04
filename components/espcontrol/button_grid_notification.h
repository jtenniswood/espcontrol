#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Notification cards ────────────────────────────────────────────────
//
// A notification card shows one Home Assistant message on a single line,
// vertically centred, under an ordinary card icon that follows the severity.
// The severity attribute only changes the colours and that icon, never the
// wording; a message too wide for the card scrolls as a marquee. Tapping the
// card opens the full text, and an optional Home Assistant action
// acknowledges it.

// NotificationLevel and its alias matching live in button_grid_config_parser.h
// so the browser and firmware can be compared without LVGL.

struct NotificationPalette {
  uint32_t background;
  uint32_t text;
};

inline NotificationPalette notification_level_palette(NotificationLevel level) {
  switch (level) {
    case NotificationLevel::WARNING:
      return {NOTIFICATION_WARNING_BG, NOTIFICATION_WARNING_TEXT};
    case NotificationLevel::ALERT:
      return {NOTIFICATION_ALERT_BG, NOTIFICATION_ALERT_TEXT};
    case NotificationLevel::INFORMATION:
    default:
      return {NOTIFICATION_INFORMATION_BG, NOTIFICATION_INFORMATION_TEXT};
  }
}

// The modal names the level in a coloured pill. The modal panel already uses
// the information background, so that level swaps in the neutral card grey and
// the other two keep their severity colours.
inline NotificationPalette notification_level_pill_palette(NotificationLevel level) {
  if (level == NotificationLevel::INFORMATION) {
    return {SECONDARY_GREY, DARK_TEXT_PRIMARY};
  }
  return notification_level_palette(level);
}

inline std::string notification_level_title(NotificationLevel level) {
  switch (level) {
    case NotificationLevel::WARNING:
      return espcontrol_i18n(std::string("Warning"));
    case NotificationLevel::ALERT:
      return espcontrol_i18n(std::string("Alert"));
    case NotificationLevel::INFORMATION:
    default:
      return espcontrol_i18n(std::string("Information"));
  }
}

struct NotificationCardCtx {
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  lv_obj_t *message_lbl = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  std::string entity_id;
  std::string ack_action;
  std::string label;
  std::string message;
  NotificationLevel level = NotificationLevel::INFORMATION;
  int width_compensation_percent = 100;
};

struct NotificationModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *close_btn = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *message_lbl = nullptr;
  lv_obj_t *ack_btn = nullptr;
  std::string entity_id;
  std::string ack_action;
};

inline NotificationModalUi &notification_modal_ui() {
  static NotificationModalUi ui;
  return ui;
}

inline void notification_card_hide_modal() {
  NotificationModalUi &ui = notification_modal_ui();
  control_modal_delete_overlay(ControlModalKind::NOTIFICATION, ui.overlay);
  ui = NotificationModalUi();
}

inline void notification_card_acknowledge() {
  NotificationModalUi &ui = notification_modal_ui();
  const std::string entity_id = ui.entity_id;
  const std::string action = ui.ack_action;
  notification_card_hide_modal();
  if (entity_id.empty() || action.empty()) return;
  if (!ha_send_entity_action(entity_id, action.c_str())) {
    ESP_LOGW("notification", "Acknowledge action %s failed for %s",
             action.c_str(), entity_id.c_str());
  }
}

// Layout is a pure function of the slot and display, so the generic card
// layout pass can reapply it without the runtime context.
//
// The card is laid out like the Date & Time cards: the message takes the same
// large value font they use for the time, and the severity reads on the bottom
// label line where they put the date. Nothing is vertically centred, so the
// card sits on the same baselines as the rest of the grid.
inline void notification_card_apply_card_layout(
    const BtnSlot &s, const DisplayProfile &display) {
  // The value font fills the space the icon would occupy, so the icon is
  // hidden exactly as the Date & Time cards hide theirs.
  if (s.icon_lbl) lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  if (s.sensor_container) {
    lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    // The container sizes to its content by default, which would let a long
    // message grow past the card instead of scrolling inside it.
    lv_obj_set_width(s.sensor_container, lv_pct(100));
    lv_obj_align(s.sensor_container, LV_ALIGN_TOP_LEFT, 0, 0);
    // A scrolling label reports its whole unwrapped text as its size, so with
    // the container now a fixed width its content overflows and LVGL draws a
    // scrollbar along the bottom - a stray line under the message.
    lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(s.sensor_container, LV_SCROLLBAR_MODE_OFF);
  }
  if (s.sensor_lbl) {
    // Not the value font the Date & Time cards use: that one is subset to
    // digits and a few symbols, so a sentence renders as empty boxes. The
    // title role is the largest font carrying the full text glyph set on
    // every panel.
    const lv_font_t *font = display_media_title_font(display);
    if (font) lv_obj_set_style_text_font(s.sensor_lbl, font, LV_PART_MAIN);
    lv_obj_set_width(s.sensor_lbl, lv_pct(100));
    // A circular scroll only animates once the text overflows, so a short
    // message simply sits still.
    lv_label_set_long_mode(s.sensor_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
  }
  // The value row carries no unit; the severity goes on the label line.
  if (s.unit_lbl) lv_label_set_display_text(s.unit_lbl, "");
  if (s.text_lbl) {
    lv_obj_clear_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_max_height(s.text_lbl, LV_COORD_MAX, LV_PART_MAIN);
    lv_label_set_long_mode(s.text_lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(s.text_lbl, lv_pct(100));
    lv_obj_set_height(s.text_lbl, LV_SIZE_CONTENT);
    lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  }
}

// The card face is the message, so with nothing to show it says so rather
// than standing in the card name. The name still titles the modal.
inline std::string notification_card_display_text(const NotificationCardCtx *ctx) {
  if (!ctx->message.empty()) return ctx->message;
  return espcontrol_i18n(std::string("No notifications"));
}

// Scroll speed for the message. Raise to scroll faster, lower to slow it.
constexpr int NOTIFICATION_SCROLL_PX_PER_SEC = 90;

// LVGL's speed-based label scrolling resolves to
// LV_CLAMP(min_time, distance * 100 / speed, max_time) with max_time capped at
// 10 s, so any message wider than 100 * speed pixels crosses the card in
// exactly 10 s however the speed is set - which is every message long enough
// to scroll at all. That makes the effective speed rise with the message
// length: a 1200 px scroll runs at 120 px/sec and a 2000 px one at 200 px/sec,
// which is why a long message races past. Setting the duration explicitly from
// the text width gives a real constant speed at any length, and 90 px/sec is
// about three quarters of what the clamp produced for a message long enough
// to scroll.
inline void notification_card_apply_scroll_speed(lv_obj_t *label) {
  if (label == nullptr) return;
  const lv_font_t *font = lv_obj_get_style_text_font(label, LV_PART_MAIN);
  const char *text = lv_label_get_text(label);
  if (font == nullptr || text == nullptr) return;
  lv_point_t size;
  lv_text_get_size(
    &size, text, font,
    lv_obj_get_style_text_letter_space(label, LV_PART_MAIN),
    lv_obj_get_style_text_line_space(label, LV_PART_MAIN),
    LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  if (size.x <= 0) return;
  uint32_t ms = static_cast<uint32_t>(
    (static_cast<int64_t>(size.x) * 1000) / NOTIFICATION_SCROLL_PX_PER_SEC);
  if (ms < 300) ms = 300;
  lv_obj_set_style_anim_duration(label, ms, LV_PART_MAIN);
}

inline void notification_card_apply_message(NotificationCardCtx *ctx) {
  if (ctx == nullptr || ctx->message_lbl == nullptr) return;
  lv_label_set_text(ctx->message_lbl, notification_card_display_text(ctx).c_str());
  notification_card_apply_scroll_speed(ctx->message_lbl);
}

inline void notification_card_apply_level(NotificationCardCtx *ctx) {
  if (ctx == nullptr) return;
  const NotificationPalette palette = notification_level_palette(ctx->level);
  if (ctx->btn != nullptr) {
    lv_obj_set_style_bg_color(
      ctx->btn, lv_color_hex(palette.background),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) |
        static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  if (ctx->message_lbl != nullptr) {
    lv_obj_set_style_text_color(ctx->message_lbl, lv_color_hex(palette.text), LV_PART_MAIN);
  }
  if (ctx->text_lbl != nullptr) {
    lv_obj_set_style_text_color(ctx->text_lbl, lv_color_hex(palette.text), LV_PART_MAIN);
    lv_label_set_text(ctx->text_lbl, notification_level_title(ctx->level).c_str());
  }
}

inline void notification_card_refresh(NotificationCardCtx *ctx) {
  notification_card_apply_level(ctx);
  notification_card_apply_message(ctx);
}

inline void setup_notification_card(
    BtnSlot &s, const ParsedCfg &p, const DisplayProfile &display) {
  notification_card_apply_card_layout(s, display);
  if (s.btn) {
    lv_obj_set_style_bg_color(
      s.btn, lv_color_hex(NOTIFICATION_INFORMATION_BG),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) |
        static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  if (s.sensor_lbl) {
    lv_obj_set_style_text_color(
      s.sensor_lbl, lv_color_hex(NOTIFICATION_INFORMATION_TEXT), LV_PART_MAIN);
    lv_label_set_text(s.sensor_lbl, espcontrol_i18n("No notifications"));
  }
  if (s.text_lbl) {
    lv_obj_set_style_text_color(
      s.text_lbl, lv_color_hex(NOTIFICATION_INFORMATION_TEXT), LV_PART_MAIN);
    lv_label_set_text(
      s.text_lbl,
      notification_level_title(NotificationLevel::INFORMATION).c_str());
  }
}

inline void notification_card_open_modal(NotificationCardCtx *ctx) {
  if (ctx == nullptr) return;
  const lv_font_t *label_font = ctx->label_font;
  const lv_font_t *icon_font = ctx->icon_font ? ctx->icon_font : label_font;

  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::NOTIFICATION, ctx->btn, ctx->width_compensation_percent,
    icon_font, notification_card_hide_modal);
  if (shell.overlay == nullptr) return;
  // The shell seeds the clock bar from the card label widget, which on this
  // card holds the message; name the card instead, as the camera, media, and
  // Wifi Sharing cards do.
  set_clock_bar_modal_label(
    ctx->label.empty() ? espcontrol_i18n(std::string("Notification")) : ctx->label);
  NotificationModalUi &ui = notification_modal_ui();
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.close_btn = shell.close_btn;
  ui.entity_id = ctx->entity_id;
  ui.ack_action = ctx->ack_action;

  ControlModalLayout &layout = shell.layout;
  const lv_coord_t content_w = shell.content_w;
  lv_coord_t gap = control_modal_scaled_px(16, layout.short_side);
  if (gap < 10) gap = 10;

  const NotificationPalette pill = notification_level_pill_palette(ctx->level);
  lv_coord_t pill_pad_x = control_modal_scaled_px(14, layout.short_side);
  if (pill_pad_x < 10) pill_pad_x = 10;
  lv_coord_t pill_pad_y = control_modal_scaled_px(6, layout.short_side);
  if (pill_pad_y < 4) pill_pad_y = 4;
  ui.title_lbl = lv_label_create(ui.panel);
  lv_label_set_text(ui.title_lbl, notification_level_title(ctx->level).c_str());
  lv_label_set_long_mode(ui.title_lbl, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(ui.title_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_color(ui.title_lbl, lv_color_hex(pill.text), LV_PART_MAIN);
  lv_obj_set_style_bg_color(ui.title_lbl, lv_color_hex(pill.background), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.title_lbl, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_pad_left(ui.title_lbl, pill_pad_x, LV_PART_MAIN);
  lv_obj_set_style_pad_right(ui.title_lbl, pill_pad_x, LV_PART_MAIN);
  lv_obj_set_style_pad_top(ui.title_lbl, pill_pad_y, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(ui.title_lbl, pill_pad_y, LV_PART_MAIN);
  if (label_font) lv_obj_set_style_text_font(ui.title_lbl, label_font, LV_PART_MAIN);
  lv_obj_update_layout(ui.title_lbl);
  lv_obj_set_style_radius(
    ui.title_lbl, lv_obj_get_height(ui.title_lbl) / 2, LV_PART_MAIN);

  ui.message_lbl = lv_label_create(ui.panel);
  lv_label_set_text(
    ui.message_lbl,
    ctx->message.empty() ? espcontrol_i18n("No notifications") : ctx->message.c_str());
  lv_label_set_long_mode(ui.message_lbl, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(ui.message_lbl, content_w);
  lv_obj_set_style_text_align(ui.message_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_color(ui.message_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  if (label_font) lv_obj_set_style_text_font(ui.message_lbl, label_font, LV_PART_MAIN);

  lv_coord_t ack_height = 0;
  if (!ui.ack_action.empty() && !ui.entity_id.empty()) {
    lv_coord_t button_h = control_modal_scaled_px(52, layout.short_side);
    if (button_h < 36) button_h = 36;
    lv_coord_t button_min_w = button_h + control_modal_scaled_px(16, layout.short_side);
    if (button_min_w < 56) button_min_w = 56;
    if (button_min_w > content_w) button_min_w = content_w;
    const uint32_t ack_color = current_button_primary_color();
    ui.ack_btn = control_modal_create_text_button(
      ui.panel, espcontrol_i18n(std::string("Acknowledge")), content_w,
      button_min_w, button_h, button_h / 2, ack_color, label_font);
    if (ui.ack_btn != nullptr) {
      lv_obj_t *ack_label = lv_obj_get_child(ui.ack_btn, 0);
      if (ack_label) {
        lv_obj_set_style_text_color(
          ack_label, lv_color_hex(readable_text_color_for_bg(ack_color)), LV_PART_MAIN);
      }
      lv_obj_add_event_cb(ui.ack_btn, [](lv_event_t *) {
        notification_card_acknowledge();
      }, LV_EVENT_CLICKED, nullptr);
      lv_obj_update_layout(ui.ack_btn);
      ack_height = lv_obj_get_height(ui.ack_btn) + gap;
    }
  }

  // Keep the whole stack inside the panel, leaving room for the close button.
  const lv_coord_t top_limit = layout.inset + layout.back_size + gap;
  lv_coord_t message_max_h = layout.panel_h - top_limit - layout.inset - ack_height;
  if (message_max_h < layout.short_side / 6) message_max_h = layout.short_side / 6;
  lv_obj_set_style_max_height(ui.message_lbl, message_max_h, LV_PART_MAIN);

  lv_obj_update_layout(ui.title_lbl);
  lv_obj_update_layout(ui.message_lbl);
  const lv_coord_t title_h = lv_obj_get_height(ui.title_lbl);
  const lv_coord_t message_h = lv_obj_get_height(ui.message_lbl);
  const lv_coord_t group_h = title_h + gap + message_h + ack_height;
  lv_coord_t group_top = (layout.panel_h - group_h) / 2;
  if (group_top < top_limit) group_top = top_limit;

  lv_obj_align(ui.title_lbl, LV_ALIGN_CENTER, 0,
               group_top + title_h / 2 - layout.panel_h / 2);
  lv_obj_align(ui.message_lbl, LV_ALIGN_CENTER, 0,
               group_top + title_h + gap + message_h / 2 - layout.panel_h / 2);
  if (ui.ack_btn != nullptr) {
    const lv_coord_t ack_h = lv_obj_get_height(ui.ack_btn);
    lv_obj_align(ui.ack_btn, LV_ALIGN_CENTER, 0,
                 group_top + group_h - ack_h / 2 - layout.panel_h / 2);
  }

  lv_obj_move_foreground(ui.overlay);
}

// The message can arrive as either the entity state or one of its attributes;
// both land here so the wrap-or-scroll decision is made in one place.
inline std::function<void(esphome::StringRef)> notification_card_message_callback(
    NotificationCardCtx *ctx) {
  return [ctx](esphome::StringRef state) {
    // A trigger-based template sensor reads unknown until it first fires;
    // that is no message rather than a message reading "unknown".
    ctx->message = ha_state_unavailable_ref(state)
      ? std::string()
      : string_ref_limited(state, NOTIFICATION_MAX_MESSAGE_LENGTH);
    notification_card_apply_message(ctx);
  };
}

inline void notification_card_bind_runtime(
    BtnSlot &s, const ParsedCfg &p, int width_compensation_percent,
    bool is_subpage) {
  if (s.btn == nullptr) return;
  NotificationCardCtx *ctx = is_subpage
    ? grid_delete_with_owner(s.btn, new NotificationCardCtx())
    : grid_track_runtime_allocation(s.btn, new NotificationCardCtx());
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->text_lbl = s.text_lbl;
  ctx->message_lbl = s.sensor_lbl;
  // The card message uses the enlarged title font; the modal wraps a paragraph,
  // so it keeps the ordinary card label size from the button itself.
  ctx->label_font = s.btn
    ? lv_obj_get_style_text_font(s.btn, LV_PART_MAIN) : nullptr;
  ctx->icon_font = s.icon_lbl
    ? lv_obj_get_style_text_font(s.icon_lbl, LV_PART_MAIN) : nullptr;
  ctx->entity_id = p.entity;
  ctx->ack_action = notification_card_ack_action(p.options);
  if (!notification_card_ack_action_valid(ctx->ack_action)) ctx->ack_action.clear();
  ctx->label = p.label;
  ctx->width_compensation_percent = width_compensation_percent;
  lv_obj_set_user_data(s.btn, ctx);
  lv_obj_add_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  if (is_subpage) {
    lv_obj_add_event_cb(s.btn, [](lv_event_t *e) {
      lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
      notification_card_open_modal(
        target ? static_cast<NotificationCardCtx *>(lv_obj_get_user_data(target))
               : nullptr);
    }, LV_EVENT_CLICKED, nullptr);
  }
  if (p.entity.empty()) {
    notification_card_refresh(ctx);
    return;
  }

  const std::string message_attribute = notification_card_message_attribute(p.options);
  if (message_attribute.empty()) {
    ha_subscribe_state(p.entity, notification_card_message_callback(ctx));
  } else {
    ha_subscribe_attribute(
      p.entity, message_attribute, notification_card_message_callback(ctx),
      HA_SUBSCRIPTION_SCOPE_DEFAULT, true);
  }

  ha_subscribe_attribute(
    p.entity, notification_card_level_attribute(p.options),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      ctx->level = notification_level_from_text(
        string_ref_limited(state, NOTIFICATION_MAX_LEVEL_LENGTH));
      notification_card_apply_level(ctx);
    }),
    HA_SUBSCRIPTION_SCOPE_DEFAULT,
    true);
  notification_card_refresh(ctx);
}
