#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// Shared randomized PIN keypad modal used by alarm actions and screensaver unlock.

enum class PinKeypadKind {
  NONE,
  ALARM,
  SCREENSAVER,
};

using PinKeypadSubmitCallback = bool (*)(const std::string &, void *);
using PinKeypadCloseCallback = void (*)();

struct PinKeypadUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *pin_lbl = nullptr;
  PinKeypadKind kind = PinKeypadKind::NONE;
  PinKeypadSubmitCallback submit_callback = nullptr;
  PinKeypadCloseCallback close_callback = nullptr;
  void *user_data = nullptr;
  const char *empty_label = nullptr;
  std::string pin;
  std::string key_data[12];
};

inline PinKeypadUi &pin_keypad_ui() {
  static PinKeypadUi ui;
  return ui;
}

inline ControlModalKind pin_keypad_modal_kind(PinKeypadKind kind) {
  return kind == PinKeypadKind::ALARM
    ? ControlModalKind::ALARM_PIN
    : ControlModalKind::SCREENSAVER_PIN;
}

inline void pin_keypad_hide_modal() {
  PinKeypadUi &ui = pin_keypad_ui();
  PinKeypadCloseCallback close_callback = ui.close_callback;
  ControlModalKind kind = pin_keypad_modal_kind(ui.kind);
  ui.pin.clear();
  control_modal_delete_overlay(kind, ui.overlay);
  ui = PinKeypadUi();
  if (close_callback) close_callback();
}

inline void pin_keypad_update_display() {
  PinKeypadUi &ui = pin_keypad_ui();
  if (!ui.pin_lbl) return;
  if (ui.pin.empty()) {
    lv_label_set_text(ui.pin_lbl, espcontrol_i18n(ui.empty_label ? ui.empty_label : "Enter Pin"));
    return;
  }
  std::string masked(ui.pin.size(), '*');
  lv_label_set_text(ui.pin_lbl, masked.c_str());
}

inline void pin_keypad_submit() {
  PinKeypadUi &ui = pin_keypad_ui();
  if (!ui.submit_callback || ui.pin.empty()) return;
  std::string code = ui.pin;
  bool accepted = ui.submit_callback(code, ui.user_data);
  if (accepted) {
    pin_keypad_hide_modal();
    return;
  }
  ui.pin.clear();
  pin_keypad_update_display();
}

inline void pin_keypad_key_cb(lv_event_t *e) {
  const std::string *key = static_cast<const std::string *>(lv_event_get_user_data(e));
  if (!key) return;
  PinKeypadUi &ui = pin_keypad_ui();
  if (*key == "back") {
    ui.pin.clear();
    pin_keypad_update_display();
    return;
  }
  if (*key == "submit") {
    pin_keypad_submit();
    return;
  }
  if (ui.pin.size() < 16 && key->size() == 1 &&
      (*key)[0] >= '0' && (*key)[0] <= '9') {
    ui.pin.push_back((*key)[0]);
    pin_keypad_update_display();
  }
}

inline lv_obj_t *pin_keypad_create_key_button(lv_obj_t *parent, lv_coord_t width,
                                              lv_coord_t height,
                                              const char *text,
                                              const lv_font_t *font,
                                              int width_compensation_percent,
                                              uint16_t label_zoom = 256) {
  lv_coord_t radius = width < height ? width / 2 : height / 2;
  lv_obj_t *btn = control_modal_create_round_button(
    parent, width, text, font, DARK_BORDER, TERTIARY_GREY,
    width_compensation_percent);
  lv_obj_set_size(btn, width, height);
  lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label && label_zoom != 256) {
    lv_obj_update_layout(label);
    lv_coord_t offset_x = lv_obj_get_width(label) * (256 - label_zoom) / 512;
    lv_coord_t offset_y = lv_obj_get_height(label) * (256 - label_zoom) / 512;
    lv_obj_set_style_transform_zoom(label, label_zoom, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, offset_x, offset_y);
  }
  return btn;
}

inline lv_coord_t pin_keypad_gap(lv_coord_t keypad_w, lv_coord_t keypad_h,
                                 lv_coord_t short_side) {
  lv_coord_t gap = control_modal_scaled_px(14, short_side);
  lv_coord_t max_gap_w = keypad_w > 0 ? keypad_w / 16 : 0;
  lv_coord_t max_gap_h = keypad_h > 0 ? keypad_h / 20 : 0;
  if (max_gap_w > 0 && gap > max_gap_w) gap = max_gap_w;
  if (max_gap_h > 0 && gap > max_gap_h) gap = max_gap_h;
  if (gap < 4) gap = 4;
  return gap;
}

inline lv_coord_t pin_keypad_key_size(lv_coord_t keypad_w, lv_coord_t keypad_h,
                                      lv_coord_t gap, lv_coord_t short_side) {
  lv_coord_t key_size_w = (keypad_w - gap * 2) / 3;
  lv_coord_t key_size_h = (keypad_h - gap * 3) / 4;
  lv_coord_t key_size = key_size_w < key_size_h ? key_size_w : key_size_h;
  lv_coord_t max_key_size = control_modal_scaled_px(112, short_side);
  if (max_key_size < 64) max_key_size = 64;
  if (key_size > max_key_size) key_size = max_key_size;
  if (key_size < 24) key_size = 24;
  return key_size;
}

inline lv_coord_t pin_keypad_label_y(const ControlModalLayout &layout, lv_coord_t pin_h) {
  lv_coord_t pin_y = layout.inset + (layout.back_size - pin_h) / 2;
  if (control_modal_uses_compact_portrait_tuning(layout)) {
    pin_y += control_modal_scaled_px(16, layout.short_side);
  }
  if (pin_y < layout.inset) pin_y = layout.inset;
  return pin_y;
}

inline uint32_t pin_keypad_random_seed() {
  uint32_t seed = lv_tick_get();
#ifdef ESP_PLATFORM
  seed ^= (uint32_t) esp_random();
#endif
  return seed;
}

inline void pin_keypad_shuffle_digits(std::string *digits, int count) {
  if (!digits || count <= 1) return;
  uint32_t seed = pin_keypad_random_seed();
  for (int i = count - 1; i > 0; i--) {
    seed = seed * 1664525u + 1013904223u;
    int j = seed % (i + 1);
    std::swap(digits[i], digits[j]);
  }
}

inline void pin_keypad_prepare_keys(PinKeypadUi &ui) {
  std::string digits[10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
  pin_keypad_shuffle_digits(digits, 10);
  for (int i = 0; i < 9; i++) ui.key_data[i] = digits[i];
  ui.key_data[9] = "back";
  ui.key_data[10] = digits[9];
  ui.key_data[11] = "submit";
}

inline bool pin_keypad_open_modal(
    PinKeypadKind kind,
    lv_obj_t *source_btn,
    int width_compensation_percent,
    const lv_font_t *pin_label_font,
    const lv_font_t *key_label_font,
    const lv_font_t *icon_font,
    const char *empty_label,
    bool closeable,
    PinKeypadSubmitCallback submit_callback,
    void *user_data,
    PinKeypadCloseCallback close_callback = nullptr) {
  if (!submit_callback) return false;

  ControlModalShell shell = control_modal_open_shell(
    pin_keypad_modal_kind(kind), source_btn, width_compensation_percent, icon_font,
    closeable ? "\U000F0141" : nullptr, closeable, pin_keypad_hide_modal);

  PinKeypadUi &ui = pin_keypad_ui();
  ui = PinKeypadUi();
  ui.kind = kind;
  ui.submit_callback = submit_callback;
  ui.close_callback = close_callback;
  ui.user_data = user_data;
  ui.empty_label = empty_label;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  pin_keypad_prepare_keys(ui);

  ControlModalLayout &layout = shell.layout;

  ui.pin_lbl = lv_label_create(ui.panel);
  lv_obj_set_style_text_color(ui.pin_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.pin_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (pin_label_font) lv_obj_set_style_text_font(ui.pin_lbl, pin_label_font, LV_PART_MAIN);
  apply_width_compensation(ui.pin_lbl, width_compensation_percent);
  lv_coord_t pin_w = layout.panel_w - (layout.inset + layout.back_size) * 2;
  if (pin_w < 60) pin_w = layout.panel_w - layout.inset * 2;
  lv_obj_set_width(ui.pin_lbl, pin_w);
  pin_keypad_update_display();
  lv_obj_update_layout(ui.pin_lbl);
  lv_coord_t pin_h = lv_obj_get_height(ui.pin_lbl);
  lv_coord_t pin_y = pin_keypad_label_y(layout, pin_h);
  lv_obj_align(ui.pin_lbl, LV_ALIGN_TOP_MID, 0, pin_y);

  lv_coord_t pin_button_gap = control_modal_scaled_px(24, layout.short_side);
  if (pin_button_gap < 10) pin_button_gap = 10;
  lv_coord_t keypad_top = layout.inset + layout.back_size + pin_button_gap;
  lv_coord_t keypad_bottom = layout.panel_h - layout.inset;
  lv_coord_t keypad_w = layout.panel_w - layout.inset * 2;
  lv_coord_t keypad_h = keypad_bottom - keypad_top;
  lv_coord_t gap = pin_keypad_gap(keypad_w, keypad_h, layout.short_side);
  lv_coord_t key_size = pin_keypad_key_size(keypad_w, keypad_h, gap, layout.short_side);
  lv_coord_t total_w = key_size * 3 + gap * 2;
  lv_coord_t total_h = key_size * 4 + gap * 3;
  lv_coord_t start_x = (layout.panel_w - total_w) / 2;
  lv_coord_t start_y = keypad_top + (keypad_h - total_h) / 2;
  if (start_y < keypad_top) start_y = keypad_top;

  for (int i = 0; i < 12; i++) {
    const std::string &key = ui.key_data[i];
    const char *text = key.c_str();
    const lv_font_t *key_font = key_label_font;
    uint16_t key_zoom = 256;
    if (key == "back") {
      text = "\U000F0156";
      key_font = icon_font;
      key_zoom = 170;
    } else if (key == "submit") {
      text = find_icon("Check");
      key_font = icon_font;
      key_zoom = 170;
    }

    lv_obj_t *key_btn = pin_keypad_create_key_button(
      ui.panel, key_size, key_size, text, key_font,
      width_compensation_percent, key_zoom);
    if (key == "submit") {
      lv_obj_set_style_bg_color(key_btn, lv_color_hex(DEFAULT_SLIDER_COLOR), LV_PART_MAIN);
      lv_obj_set_style_border_color(key_btn, lv_color_hex(DEFAULT_SLIDER_COLOR), LV_PART_MAIN);
      lv_obj_t *key_lbl = lv_obj_get_child(key_btn, 0);
      if (key_lbl) lv_obj_set_style_text_color(key_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    }
    int row = i / 3;
    int col = i % 3;
    lv_coord_t x = start_x + col * (key_size + gap);
    lv_coord_t y = start_y + row * (key_size + gap);
    lv_obj_set_pos(key_btn, x, y);
    lv_obj_add_event_cb(key_btn, pin_keypad_key_cb, LV_EVENT_CLICKED, &ui.key_data[i]);
  }

  if (ui.back_btn) lv_obj_move_foreground(ui.back_btn);
  lv_obj_move_foreground(ui.overlay);
  return true;
}

inline bool screensaver_pin_keypad_submit(const std::string &code, void *user_data) {
  (void) user_data;
  if (normalize_numeric_pin(code) != screensaver_pin_unlock_code()) return false;
  screensaver_pin_locked() = false;
  screensaver_pin_unlock_code().clear();
  screen_lock_apply();
  return true;
}

struct ScreensaverPinKeypadOpenArgs {
  lv_obj_t *source_btn = nullptr;
  int width_compensation_percent = 0;
  const lv_font_t *pin_label_font = nullptr;
  const lv_font_t *key_label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  lv_timer_t *reopen_timer = nullptr;
};

inline ScreensaverPinKeypadOpenArgs &screensaver_pin_keypad_open_args() {
  static ScreensaverPinKeypadOpenArgs args;
  return args;
}

inline void screensaver_pin_reopen_if_locked();

inline void screensaver_pin_reopen_timer_cb(lv_timer_t *timer) {
  ScreensaverPinKeypadOpenArgs &args = screensaver_pin_keypad_open_args();
  if (args.reopen_timer == timer) args.reopen_timer = nullptr;
  lv_timer_del(timer);
  if (!screensaver_pin_locked() || screensaver_pin_unlock_code().empty()) return;
  pin_keypad_open_modal(
    PinKeypadKind::SCREENSAVER,
    nullptr,
    args.width_compensation_percent,
    args.pin_label_font,
    args.key_label_font,
    args.icon_font,
    "Unlock Screen",
    false,
    screensaver_pin_keypad_submit,
    nullptr,
    screensaver_pin_reopen_if_locked);
}

inline void screensaver_pin_reopen_if_locked() {
  if (!screensaver_pin_locked() || screensaver_pin_unlock_code().empty()) return;
  ScreensaverPinKeypadOpenArgs &args = screensaver_pin_keypad_open_args();
  if (args.reopen_timer) lv_timer_del(args.reopen_timer);
  args.reopen_timer = lv_timer_create(screensaver_pin_reopen_timer_cb, 1, nullptr);
}

inline bool screensaver_pin_active(bool required, const std::string &pin) {
  return required && !normalize_numeric_pin(pin).empty();
}

inline bool screensaver_pin_lock_and_open(
    bool required,
    const std::string &pin,
    lv_obj_t *source_btn,
    int width_compensation_percent,
    const lv_font_t *pin_label_font,
    const lv_font_t *key_label_font,
    const lv_font_t *icon_font) {
  std::string normalized_pin = normalize_numeric_pin(pin);
  if (!required || normalized_pin.empty()) {
    screensaver_pin_locked() = false;
    screensaver_pin_unlock_code().clear();
    screen_lock_apply();
    return false;
  }
  if (control_modal_active().kind == ControlModalKind::SCREENSAVER_PIN) {
    pin_keypad_ui().close_callback = nullptr;
  }
  control_modal_force_close_active();
  ScreensaverPinKeypadOpenArgs &args = screensaver_pin_keypad_open_args();
  args.source_btn = source_btn;
  args.width_compensation_percent = width_compensation_percent;
  args.pin_label_font = pin_label_font;
  args.key_label_font = key_label_font;
  args.icon_font = icon_font;
  screensaver_pin_unlock_code() = normalized_pin;
  screensaver_pin_locked() = true;
  screen_lock_apply();
  return pin_keypad_open_modal(
    PinKeypadKind::SCREENSAVER,
    source_btn,
    width_compensation_percent,
    pin_label_font,
    key_label_font,
    icon_font,
    "Unlock Screen",
    false,
    screensaver_pin_keypad_submit,
    nullptr,
    screensaver_pin_reopen_if_locked);
}
