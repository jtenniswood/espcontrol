#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

static std::vector<std::string> theme_warnings;
#define ESP_LOGW(tag, message, ...) theme_warnings.emplace_back(message)

struct lv_color_t { uint32_t full; };
constexpr lv_color_t lv_color_hex(uint32_t rgb) { return {rgb}; }
struct lv_color32_t { uint8_t blue, green, red, alpha; };
constexpr lv_color32_t lv_color_to_32(lv_color_t color, int alpha) {
  return {static_cast<uint8_t>(color.full), static_cast<uint8_t>(color.full >> 8),
          static_cast<uint8_t>(color.full >> 16), static_cast<uint8_t>(alpha)};
}
constexpr bool lv_color_eq(lv_color_t left, lv_color_t right) { return left.full == right.full; }
constexpr int LV_PART_MAIN = 0;
constexpr int LV_PART_KNOB = 1;
constexpr int LV_STATE_PRESSED = 2;
constexpr int LV_STATE_DISABLED = 4;
constexpr int LV_STATE_CHECKED = 8;
constexpr int LV_STATE_DEFAULT = 0;
constexpr int LV_STYLE_BG_COLOR = 1;
enum lv_style_res_t { LV_STYLE_RES_NOT_FOUND, LV_STYLE_RES_FOUND };
using lv_style_prop_t = uint8_t;
struct lv_style_value_t { lv_color_t color; };
using lv_style_selector_t = int;
constexpr int LV_OPA_TRANSP = 0;
constexpr int LV_OPA_COVER = 255;
enum lv_obj_flag_t : uint32_t {
  LV_OBJ_FLAG_USER_1 = 1u << 27,
  LV_OBJ_FLAG_USER_2 = 1u << 28,
  LV_OBJ_FLAG_USER_3 = 1u << 29,
  LV_OBJ_FLAG_USER_4 = 1u << 30,
};

struct lv_obj_class_t {};
static const lv_obj_class_t lv_obj_class{};
static const lv_obj_class_t lv_label_class{};
static const lv_obj_class_t lv_button_class{};
static const lv_obj_class_t lv_slider_class{};
static const lv_obj_class_t lv_arc_class{};
static const lv_obj_class_t lv_image_class{};
constexpr int LV_EVENT_DELETE = 1;
struct lv_obj_t;
struct lv_event_t { lv_obj_t *target; };
using lv_event_cb_t = void (*)(lv_event_t *);

struct lv_obj_t {
  const lv_obj_class_t *type = &lv_obj_class;
  lv_color_t background{0};
  lv_color_t text{0};
  lv_color_t arc{0};
  lv_color_t knob{0};
  lv_color_t border{0};
  lv_color_t pressed_background{0};
  bool has_pressed_background = false;
  lv_color_t disabled_border{0};
  lv_color_t disabled_text{0};
  lv_color_t checked_text{0};
  lv_color_t pressed_text{0};
  bool has_disabled_text = false;
  bool has_checked_text = false;
  bool has_pressed_text = false;
  int border_width = 0;
  int opacity = LV_OPA_TRANSP;
  int state = 0;
  uint32_t flags = 0;
  lv_obj_t *parent = nullptr;
  std::vector<lv_obj_t *> children;
  lv_event_cb_t delete_callback = nullptr;
};

bool lv_obj_check_type(const lv_obj_t *obj, const lv_obj_class_t *type) {
  return obj->type == type;
}
bool lv_obj_has_flag(const lv_obj_t *obj, lv_obj_flag_t flag) { return (obj->flags & flag) != 0; }
bool lv_obj_has_state(const lv_obj_t *obj, int state) { return (obj->state & state) != 0; }
lv_style_res_t lv_obj_get_local_style_prop(lv_obj_t *obj, lv_style_prop_t prop, lv_style_value_t *value, int selector) {
  assert(prop == LV_STYLE_BG_COLOR && selector == LV_PART_MAIN);
  value->color = obj->background;
  return LV_STYLE_RES_FOUND;
}
void lv_obj_add_flag(lv_obj_t *obj, lv_obj_flag_t flag) { obj->flags |= flag; }
void lv_obj_clear_flag(lv_obj_t *obj, lv_obj_flag_t flag) { obj->flags &= ~flag; }
lv_obj_t *lv_obj_get_parent(const lv_obj_t *obj) { return obj->parent; }
int lv_obj_get_style_bg_opa(const lv_obj_t *obj, int) { return obj->opacity; }
lv_color_t lv_obj_get_style_bg_color(const lv_obj_t *obj, int part) {
  if (part == LV_PART_MAIN && (obj->state & LV_STATE_PRESSED) && obj->has_pressed_background)
    return obj->pressed_background;
  return part == LV_PART_KNOB ? obj->knob : obj->background;
}
lv_color_t lv_obj_get_style_text_color(const lv_obj_t *obj, int) {
  if ((obj->state & LV_STATE_DISABLED) && obj->has_disabled_text) return obj->disabled_text;
  if ((obj->state & LV_STATE_PRESSED) && obj->has_pressed_text) return obj->pressed_text;
  if ((obj->state & LV_STATE_CHECKED) && obj->has_checked_text) return obj->checked_text;
  return obj->text;
}
lv_color_t lv_obj_get_style_arc_color(const lv_obj_t *obj, int) { return obj->arc; }
lv_color_t lv_obj_get_style_border_color(const lv_obj_t *obj, int) { return obj->border; }
int lv_obj_get_style_border_width(const lv_obj_t *obj, int) { return obj->border_width; }
void lv_obj_set_style_bg_color(lv_obj_t *obj, lv_color_t color, int part) {
  if (part == LV_STATE_PRESSED) obj->has_pressed_background = true;
  (part == LV_STATE_PRESSED ? obj->pressed_background :
   part == LV_PART_KNOB ? obj->knob : obj->background) = color;
}
void lv_obj_set_style_bg_opa(lv_obj_t *obj, int opacity, int selector) {
  if (selector == LV_PART_MAIN) obj->opacity = opacity;
}
void apply_push_button_transition(lv_obj_t *) {}
void lv_obj_set_style_text_color(lv_obj_t *obj, lv_color_t color, int selector) {
  if (selector == LV_STATE_DISABLED) { obj->disabled_text = color; obj->has_disabled_text = true; }
  else if (selector == LV_STATE_CHECKED) { obj->checked_text = color; obj->has_checked_text = true; }
  else if (selector == LV_STATE_PRESSED) { obj->pressed_text = color; obj->has_pressed_text = true; }
  else obj->text = color;
}
void lv_obj_set_style_arc_color(lv_obj_t *obj, lv_color_t color, int) { obj->arc = color; }
void lv_obj_set_style_border_color(lv_obj_t *obj, lv_color_t color, int selector) {
  (selector == LV_STATE_DISABLED ? obj->disabled_border : obj->border) = color;
}
uint32_t lv_obj_get_child_cnt(const lv_obj_t *obj) {
  return static_cast<uint32_t>(obj->children.size());
}
lv_obj_t *lv_obj_get_child(const lv_obj_t *obj, uint32_t index) {
  return obj->children[index];
}
void lv_obj_add_event_cb(lv_obj_t *obj, lv_event_cb_t callback, int event, void *) {
  assert(event == LV_EVENT_DELETE);
  obj->delete_callback = callback;
}
lv_obj_t *lv_event_get_target(lv_event_t *event) { return event->target; }

#include "theme_runtime_static.h"

#include "button_grid_limits.h"
int bounded_grid_slots(int count) {
  assert(count >= 0 && count <= MAX_GRID_SLOTS);
  return count;
}
struct BtnSlot { lv_obj_t *btn = nullptr; };
struct TestSubpage {
  lv_obj_t *screen = nullptr;
  lv_obj_t *back_button = nullptr;
  struct Card { bool neutral_background; lv_obj_t *button; bool sensor_surface = false; bool secondary_surface = false; };
  std::vector<Card> cards;
};
std::vector<TestSubpage> &navigation_subpages() {
  static std::vector<TestSubpage> pages;
  return pages;
}
#include "button_grid_style.h"
void sync_card_checked_text_color(lv_obj_t *button) {
  for (auto *child : button->children) child->text = lv_obj_get_style_text_color(button, LV_PART_MAIN);
}
void set_card_content_disabled(lv_obj_t *, bool) {}
#include "theme_runtime_ui.h"

void register_theme_grid(lv_obj_t *page, BtnSlot *slots, const bool *neutral, int count,
                         int red, int green, int blue, const bool *secondary = nullptr) {
  const bool sensors[MAX_GRID_SLOTS]{};
  register_theme_grid(page, slots, neutral, count, sensors, red, green, blue, secondary);
}
#include "theme_settings.h"
namespace esphome { using StringRef = std::string; }
bool sensor_active_color_state_ref(const std::string &state, bool) { return state == "on"; }
void media_control_apply_availability(lv_obj_t *, lv_obj_t *, bool) {}
#include "theme_modal_adapter.h"

static void test_playback_mode_accent_collision() {
  set_active_theme_palette(DARK_THEME);
  lv_obj_t button, label;
  button.type = &lv_button_class;
  label.type = &lv_label_class;
  button.children = {&label};
  media_control_style_playback_mode_button(&button, true, true, DARK_THEME.surface_primary);
  theme_restyle_tree(&button, DARK_THEME, LIGHT_THEME);
  assert(button.background.full == DARK_THEME.surface_primary);
  set_active_theme_palette(LIGHT_THEME);
  media_control_style_playback_mode_button(&button, true, true, DARK_THEME.surface_primary);
  assert(button.background.full == DARK_THEME.surface_primary);
  assert(label.text.full == CARD_ACCENT_TEXT_COLOR);
  media_control_style_playback_mode_button(&button, false, true, DARK_THEME.surface_primary);
  assert(label.text.full == LIGHT_THEME.text_primary);
  assert(!lv_obj_has_flag(&button, LV_OBJ_FLAG_USER_1));
  theme_restyle_tree(&button, LIGHT_THEME, DARK_THEME);
  assert(button.background.full == DARK_THEME.surface_primary);
  set_active_theme_palette(DARK_THEME);
}

static void test_grid_secondary_surface() {
  set_active_theme_palette(DARK_THEME);
  apply_current_theme();
  lv_obj_t page, sensor;
  sensor.type = &lv_button_class;
  sensor.opacity = LV_OPA_COVER;
  sensor.background = lv_color_hex(DARK_THEME.surface_secondary);
  BtnSlot slots[] = {{&sensor}};
  const bool neutral[] = {true};
  const bool secondary[] = {true};
  register_theme_grid(&page, slots, neutral, 1, 100, 100, 100, secondary);
  set_active_theme_palette(LIGHT_THEME);
  apply_current_theme();
  assert(sensor.background.full == LIGHT_THEME.surface_secondary);
  set_active_theme_palette(DARK_THEME);
  apply_current_theme();
  assert(sensor.background.full == DARK_THEME.surface_secondary);
  lv_event_t deleted{&page};
  page.delete_callback(&deleted);
}

static void test_independent_dark_light_surfaces() {
  set_active_theme_palette(DARK_THEME);
  apply_current_theme();
  lv_obj_t page, card, information, sensor, subpage, sub_card, sub_information, sub_sensor;
  lv_obj_t *buttons[] = {&card, &information, &sensor, &sub_card, &sub_information, &sub_sensor};
  for (auto *button : buttons) {
    button->type = &lv_button_class;
    button->opacity = LV_OPA_COVER;
  }
  card.background = sub_card.background = lv_color_hex(DARK_THEME.surface_card);
  information.background = sub_information.background = lv_color_hex(DARK_THEME.surface_secondary);
  sensor.background = sub_sensor.background = lv_color_hex(DARK_THEME.surface_sensor);
  BtnSlot slots[] = {{&card}, {&information}, {&sensor}};
  const bool neutral[] = {true, true, true};
  const bool sensors[] = {false, false, true};
  const bool secondary[] = {false, true, false};
  register_theme_grid(&page, slots, neutral, 3, sensors, 100, 100, 100, secondary);
  navigation_subpages().push_back({&subpage, nullptr,
      {{true, &sub_card}, {true, &sub_information, false, true}, {true, &sub_sensor, true}}});
  for (const auto *theme : {&LIGHT_THEME, &DARK_THEME, &LIGHT_THEME, &DARK_THEME}) {
    set_active_theme_palette(*theme);
    apply_current_theme();
    assert(page.background.full == theme->background);
    assert(subpage.background.full == theme->background);
    assert(card.background.full == theme->surface_card);
    assert(sub_card.background.full == theme->surface_card);
    assert(information.background.full == theme->surface_secondary);
    assert(sub_information.background.full == theme->surface_secondary);
    assert(sensor.background.full == theme->surface_sensor);
    assert(sub_sensor.background.full == theme->surface_sensor);
  }
  assert(card.background.full == 0x313131);
  assert(sensor.background.full == 0x212121);
  assert(page.background.full == 0x000000);
  navigation_subpages().clear();
  lv_event_t deleted{&page};
  page.delete_callback(&deleted);
}

static void test_restore_grid_theme(ThemeMode mode, EffectiveTheme effective,
                                    bool restore_explicit_light = false) {
  ThemeSettings settings;
  settings.mode = mode;
  settings.auto_method = ThemeAutoMethod::SUNRISE_SUNSET;
  settings.light_start = 20 * 60;
  settings.dark_start = 7 * 60;
  settings.sunrise_offset = -45;
  settings.sunset_offset = 90;
  ThemeResolver resolver;
  resolver.effective = effective;
  ThemeConditions unavailable;
  set_active_theme_palette(effective == EffectiveTheme::LIGHT ? LIGHT_THEME : DARK_THEME);

  int settings_notifications = 0;
  const auto count_settings_refresh = [](void *context, const ThemePalette &) {
    ++*static_cast<int *>(context);
  };
  assert(register_theme_refresh(&settings_notifications, count_settings_refresh, &settings_notifications));
  apply_current_theme();
  const int initial_notifications = settings_notifications;

  lv_obj_t page, button, label, back, back_label, subpage, sub_button, sub_label, accent_card;
  button.type = back.type = sub_button.type = &lv_button_class;
  button.text = back.text = sub_button.text = lv_color_hex(DARK_THEME.text_primary);
  label.type = back_label.type = sub_label.type = &lv_label_class;
  button.children = {&label}; label.parent = &button;
  back.children = {&back_label}; back_label.parent = &back;
  sub_button.children = {&sub_label}; sub_label.parent = &sub_button;
  button.opacity = back.opacity = sub_button.opacity = LV_OPA_COVER;
  BtnSlot slots[] = {{&button}};
  const bool neutral[] = {true};
  register_theme_grid(&page, slots, neutral, 1, 100, 100, 100);
  apply_current_theme();  // The existing owner has already seen this palette.

  // Restore phase 1 reuses the main page/button but recreates descendants with
  // the ESPHome global theme's Dark defaults, then registers the same owner.
  label.text = lv_color_hex(DARK_THEME.text_primary);
  button.background = lv_color_hex(current_theme().surface_card);
  register_theme_grid(&page, slots, neutral, 1, 100, 100, 100);
  assert(label.text.full == DARK_THEME.text_primary);  // Registration is too early.

  // Modern backups post explicit settings independently of the layout. They
  // may arrive before reconciliation finishes; legacy backups post none.
  if (restore_explicit_light) {
    settings.mode = ThemeMode::LIGHT;
    settings.auto_method = ThemeAutoMethod::TIME;
    settings.light_start = 6 * 60 + 30;
    settings.dark_start = 21 * 60;
    settings.sunrise_offset = -20;
    settings.sunset_offset = 45;
  }
  apply_theme_resolution(resolver, settings, unavailable);
  const ThemeSettings expected_settings = settings;
  const EffectiveTheme expected_effective = resolver.effective;
  const ThemePalette *expected_palette = &current_theme();
  const int expected_notifications = initial_notifications + (restore_explicit_light ? 1 : 0);
  assert(settings_notifications == expected_notifications);

  // Phase 2 has retired the old subpage and registered its replacement. Its
  // label and back-button defaults were created after any setting transition.
  back.background = sub_button.background = lv_color_hex(DARK_THEME.surface_primary);
  back_label.text = sub_label.text = lv_color_hex(DARK_THEME.text_primary);
  accent_card.type = &lv_button_class;
  accent_card.opacity = LV_OPA_COVER;
  accent_card.background = lv_color_hex(DARK_THEME.surface_secondary);
  theme_set_content_background(&accent_card);
  navigation_subpages().push_back({&subpage, &back, {{true, &sub_button}, {true, &accent_card}}});
  sub_button.state = LV_STATE_CHECKED;
  const int preserved_subpage_state = sub_button.state;

  refresh_theme_grid_after_rebuild();  // Final boundary: no mode transition.
  assert(settings.mode == expected_settings.mode);
  assert(settings.auto_method == expected_settings.auto_method);
  assert(settings.light_start == expected_settings.light_start);
  assert(settings.dark_start == expected_settings.dark_start);
  assert(settings.sunrise_offset == expected_settings.sunrise_offset);
  assert(settings.sunset_offset == expected_settings.sunset_offset);
  assert(resolver.effective == expected_effective && &current_theme() == expected_palette);
  assert(settings_notifications == expected_notifications);
  assert(page.background.full == current_theme().background);
  assert(button.background.full == current_theme().surface_card);
  assert(label.text.full == current_theme().text_primary);
  assert(subpage.background.full == current_theme().background);
  assert(back.background.full == current_theme().surface_card);
  assert(back_label.text.full == current_theme().text_primary);
  assert(sub_button.background.full == current_theme().surface_card);
  assert(sub_button.state == preserved_subpage_state);
  assert(sub_button.text.full == current_theme().text_primary);
  assert(sub_label.text.full == CARD_ACCENT_TEXT_COLOR);
  assert(accent_card.background.full == DARK_THEME.surface_secondary);
  int grid_bindings = 0;
  for (const auto &binding : theme_refresh_bindings())
    if (binding.owner == &page) ++grid_bindings;
  assert(grid_bindings == 1);
  apply_current_theme();
  assert(settings_notifications == expected_notifications);

  navigation_subpages().clear();
  lv_event_t deleted{&page};
  page.delete_callback(&deleted);
  refresh_theme_grid_after_rebuild();  // Deleted owners must never be touched.
  assert(theme_grid_targets().main_page == nullptr);
  assert(settings_notifications == expected_notifications);
  unregister_theme_refresh(&settings_notifications);
}

static void test_sensor_state_refresh() {
  set_active_theme_palette(DARK_THEME);
  apply_current_theme();
  lv_obj_t page, sensor, label, subpage, sub_sensor;
  sensor.type = &lv_button_class;
  sensor.opacity = LV_OPA_COVER;
  sensor.children = {&label};
  label.type = &lv_label_class;
  label.parent = &sensor;
  BtnSlot slots[] = {{&sensor}};
  const bool neutral[] = {true};
  const bool secondary[] = {true};
  register_theme_grid(&page, slots, neutral, 1, 50, 100, 100, secondary);
  // The active accent deliberately collides with a semantic neutral RGB.
  const uint32_t accent = DARK_THEME.surface_secondary;
  apply_sensor_active_color(&sensor, true, "on", accent, current_grid_sensor_color(), false);
  set_active_theme_palette(LIGHT_THEME);
  apply_current_theme();
  assert(sensor.background.full == accent && label.text.full == CARD_ACCENT_TEXT_COLOR);
  apply_sensor_active_color(&sensor, true, "off", accent, current_grid_sensor_color(), false);
  assert(sensor.background.full == correct_display_color(LIGHT_THEME.surface_secondary, 50, 100, 100));
  assert(label.text.full == LIGHT_THEME.text_primary);
  sub_sensor = sensor;
  sub_sensor.children.clear();
  navigation_subpages().push_back({&subpage, nullptr, {{true, &sub_sensor, false, true}}});
  sensor.state = LV_STATE_CHECKED;
  set_active_theme_palette(DARK_THEME);
  apply_current_theme();
  assert(sensor.state == LV_STATE_CHECKED);
  assert(sensor.background.full == correct_display_color(DARK_THEME.surface_secondary, 50, 100, 100));
  assert(sub_sensor.background.full == sensor.background.full);
  apply_sensor_active_color(&sensor, true, "off", accent, current_grid_sensor_color(), true);
  assert(sensor.background.full == correct_display_color(DARK_THEME.surface_secondary, 50, 100, 100));
  navigation_subpages().clear();
  lv_event_t deleted{&page};
  page.delete_callback(&deleted);
}

static_assert(CONTROL_MODAL_THEME_PRESSED_CAPACITY == 64,
              "All 64 supported select rows must fit in one modal owner");

static void test_supported_pressed_lists() {
  // Production demand: fan (20 fixed + 32 presets), climate (19 + 32 options),
  // select (64 rows). Hidden controls still belong to their modal owner.
  for (int demand : {52, 51, 64}) {
    set_active_theme_palette(DARK_THEME);
    apply_current_theme();
    theme_warnings.clear();
    lv_obj_t overlay, panel, rows[64];
    panel.background = lv_color_hex(DARK_THEME.surface_secondary);
    panel.opacity = LV_OPA_COVER;
    control_modal_register_theme({&overlay, &panel});
    for (int i = 0; i < demand; ++i) {
      auto &row = rows[i];
      row.type = &lv_button_class;
      row.parent = &panel;
      row.background = lv_color_hex(DARK_THEME.surface_primary);
      row.opacity = LV_OPA_COVER;
      panel.children.push_back(&row);
      control_modal_apply_pressed_fill(&row);
    }
    set_active_theme_palette(LIGHT_THEME);
    apply_current_theme();
    for (int i = 0; i < demand; ++i) {
      auto &row = rows[i];
      assert(lv_obj_get_style_bg_color(&row, LV_PART_MAIN).full == LIGHT_THEME.surface_primary);
      row.state = LV_STATE_PRESSED;
      assert(lv_obj_get_style_bg_color(&row, LV_PART_MAIN).full == LIGHT_THEME.surface_primary);
      row.state = 0;
    }
    rows[demand - 1].state = LV_STATE_PRESSED;
    set_active_theme_palette(DARK_THEME);
    apply_current_theme();
    assert(rows[demand - 1].state == LV_STATE_PRESSED);
    for (int i = 0; i < demand; ++i) {
      assert(rows[i].background.full == DARK_THEME.surface_primary);
      assert(rows[i].pressed_background.full == DARK_THEME.surface_primary);
    }
    assert(theme_warnings.empty());
    lv_event_t deleted{&overlay};
    overlay.delete_callback(&deleted);
  }
}

static void test_pressed_overflow_recovery() {
  set_active_theme_palette(DARK_THEME);
  apply_current_theme();
  theme_warnings.clear();
  lv_obj_t overlay, panel, rows[CONTROL_MODAL_THEME_PRESSED_CAPACITY + 1];
  const auto open = [&]() {
    panel.children.clear();
    panel.background = lv_color_hex(current_theme().surface_secondary);
    panel.opacity = LV_OPA_COVER;
    control_modal_register_theme({&overlay, &panel});
    for (auto &row : rows) {
      row = {};
      row.type = &lv_button_class;
      row.parent = &panel;
      row.background = lv_color_hex(current_theme().surface_primary);
      row.opacity = LV_OPA_COVER;
      panel.children.push_back(&row);
      control_modal_apply_pressed_fill(&row);
    }
  };
  open();
  auto &omitted = rows[CONTROL_MODAL_THEME_PRESSED_CAPACITY];
  assert(theme_warnings.size() == 1);
  assert(omitted.delete_callback == nullptr);
  set_active_theme_palette(LIGHT_THEME);
  apply_current_theme();
  assert(lv_obj_get_style_bg_color(&omitted, LV_PART_MAIN).full == LIGHT_THEME.surface_primary);
  omitted.state = LV_STATE_PRESSED;
  assert(lv_obj_get_style_bg_color(&omitted, LV_PART_MAIN).full == DARK_THEME.surface_primary);
  omitted.state = 0;
  assert(lv_obj_get_style_bg_color(&omitted, LV_PART_MAIN).full == LIGHT_THEME.surface_primary);
  assert(omitted.pressed_background.full == DARK_THEME.surface_primary);
  // Reopening writes current-theme pressed styles, even for the omitted row.
  lv_event_t deleted{&overlay};
  overlay.delete_callback(&deleted);
  open();
  assert(theme_warnings.size() == 2);
  omitted.state = LV_STATE_PRESSED;
  assert(lv_obj_get_style_bg_color(&omitted, LV_PART_MAIN).full == LIGHT_THEME.surface_primary);
  omitted.state = 0;
  set_active_theme_palette(DARK_THEME);
  apply_current_theme();
  assert(omitted.background.full == DARK_THEME.surface_primary);
  assert(omitted.pressed_background.full == LIGHT_THEME.surface_primary);
  // A freed slot is usable on an explicit registration retry, not on a press.
  deleted.target = &rows[0];
  rows[0].delete_callback(&deleted);
  panel.children.erase(panel.children.begin());
  control_modal_apply_pressed_fill(&omitted);
  assert(omitted.delete_callback != nullptr);
  assert(theme_warnings.size() == 2);
  set_active_theme_palette(LIGHT_THEME);
  apply_current_theme();
  assert(omitted.pressed_background.full == LIGHT_THEME.surface_primary);
  deleted.target = &overlay;
  overlay.delete_callback(&deleted);
  set_active_theme_palette(DARK_THEME);
  apply_current_theme();
}

static void test_content_collisions() {
  // Swatches, HA-derived light fills and selected rows may be exactly neutral.
  // A selected row must still refresh the neutral controls below it.
  const uint32_t colors[] = {0x000000, 0x313131, 0x212121, 0xFFFFFF,
                            LIGHT_THEME.background, LIGHT_THEME.text_primary};
  for (uint32_t rgb : colors) {
    lv_obj_t selected, content_label, neutral_child, handle, accent_label, slider;
    selected.opacity = LV_OPA_COVER;
    selected.background = lv_color_hex(rgb);
    selected.state = LV_STATE_PRESSED;
    theme_set_content_background(&selected);
    content_label.type = &lv_label_class;
    content_label.text = lv_color_hex(0xFFFFFF);
    neutral_child.opacity = LV_OPA_COVER;
    neutral_child.background = lv_color_hex(DARK_THEME.surface_secondary);
    handle.opacity = LV_OPA_COVER;
    handle.background = lv_color_hex(DARK_THEME.text_primary);
    theme_set_primary_foreground_fill(&handle);
    accent_label.type = &lv_label_class;
    accent_label.text = lv_color_hex(rgb);
    theme_set_content_foreground(&accent_label);
    slider.type = &lv_slider_class;
    slider.opacity = LV_OPA_COVER;
    slider.background = lv_color_hex(DARK_THEME.track_background);
    slider.knob = lv_color_hex(rgb);
    theme_set_content_foreground(&slider);
    selected.children = {&content_label, &neutral_child, &handle, &accent_label, &slider};
    for (int i = 0; i < 3; ++i) {
      theme_restyle_tree(&selected, DARK_THEME, LIGHT_THEME);
      assert(selected.background.full == rgb);
      assert(content_label.text.full == 0xFFFFFF);
      assert(neutral_child.background.full == LIGHT_THEME.surface_secondary);
      assert(handle.background.full == LIGHT_THEME.text_primary);
      assert(accent_label.text.full == rgb);
      assert(slider.knob.full == rgb);
      assert(slider.background.full == LIGHT_THEME.track_background);
      theme_restyle_tree(&selected, LIGHT_THEME, DARK_THEME);
      assert(selected.background.full == rgb);
      assert(handle.background.full == DARK_THEME.text_primary);
      assert(neutral_child.background.full == DARK_THEME.surface_secondary);
      assert(selected.state == LV_STATE_PRESSED);
    }
    // On deselection the state writer returns the fill to theme ownership.
    theme_set_content_background(&selected, false);
    selected.background = lv_color_hex(DARK_THEME.surface_primary);
    theme_restyle_tree(&selected, DARK_THEME, LIGHT_THEME);
    assert(selected.background.full == LIGHT_THEME.surface_primary);
  }
  // Test foreground ownership outside a selected/content-background ancestor.
  lv_obj_t accent;
  accent.type = &lv_label_class;
  accent.text = lv_color_hex(DARK_THEME.text_primary);
  theme_set_content_foreground(&accent);
  theme_restyle_tree(&accent, DARK_THEME, LIGHT_THEME);
  assert(accent.text.full == DARK_THEME.text_primary);
  lv_obj_t pressed;
  pressed.pressed_background = lv_color_hex(0x313131);
  theme_set_content_pressed_fill(&pressed);
  theme_restyle_pressed_fill(&pressed, LIGHT_THEME);
  assert(pressed.pressed_background.full == 0x313131);
  theme_set_content_pressed_fill(&pressed, false);
  theme_restyle_pressed_fill(&pressed, LIGHT_THEME);
  assert(pressed.pressed_background.full == LIGHT_THEME.surface_primary);
  lv_obj_t off_button, inverted_label;
  off_button.opacity = LV_OPA_COVER;
  off_button.background = lv_color_hex(DARK_THEME.text_primary);
  theme_set_primary_foreground_fill(&off_button);
  inverted_label.type = &lv_label_class;
  inverted_label.text = lv_color_hex(DARK_THEME.text_inverted);
  inverted_label.parent = &off_button;
  off_button.children = {&inverted_label};
  theme_restyle_tree(&off_button, DARK_THEME, LIGHT_THEME);
  assert(off_button.background.full == LIGHT_THEME.text_primary);
  assert(inverted_label.text.full == LIGHT_THEME.text_inverted);
  theme_restyle_tree(&off_button, LIGHT_THEME, DARK_THEME);
  assert(off_button.background.full == DARK_THEME.text_primary);
  assert(inverted_label.text.full == DARK_THEME.text_inverted);
  // The climate current-temperature dot shares Dark RGB with surface_primary,
  // but must resolve to control_neutral, not surface_primary, in Light.
  lv_obj_t current_dot;
  current_dot.opacity = LV_OPA_COVER;
  current_dot.background = lv_color_hex(DARK_THEME.control_neutral);
  theme_set_control_neutral_fill(&current_dot);
  theme_restyle_tree(&current_dot, DARK_THEME, LIGHT_THEME);
  assert(current_dot.background.full == LIGHT_THEME.control_neutral);
  theme_restyle_tree(&current_dot, LIGHT_THEME, DARK_THEME);
  assert(current_dot.background.full == DARK_THEME.control_neutral);
  // Corrected neutral RGB can collide too; an explicit fill remains raw content.
  const ThemeTreeCorrection correction{80, 90, 100};
  lv_obj_t fill;
  fill.opacity = LV_OPA_COVER;
  fill.background = lv_color_hex(theme_tree_corrected(DARK_THEME.surface_primary, correction));
  const uint32_t original = fill.background.full;
  theme_set_content_background(&fill);
  theme_restyle_tree(&fill, DARK_THEME, LIGHT_THEME, false, correction);
  assert(fill.background.full == original);
}

static void test_registry_exhaustion() {
  theme_warnings.clear();
  assert(!register_theme_static_page(nullptr));
  assert(theme_warnings.size() == 1);
  theme_warnings.clear();
  lv_obj_t pages[9];
  for (int i = 0; i < 8; ++i) assert(register_theme_static_page(&pages[i]));
  assert(register_theme_static_page(&pages[0]));  // Duplicate succeeds even at capacity.
  assert(!register_theme_static_page(&pages[8]));
  assert(theme_warnings.size() == 1);
  assert(theme_warnings.back().find("Static page targets full") != std::string::npos);
  assert(pages[8].delete_callback == nullptr);
  for (int i = 0; i < 8; ++i) {
    lv_event_t deleted{&pages[i]};
    pages[i].delete_callback(&deleted);
  }
  assert(register_theme_static_page(&pages[8]));  // Freed slots are reusable.
  lv_event_t deleted{&pages[8]};
  pages[8].delete_callback(&deleted);

  lv_obj_t collected, labels[13], actions[3];
  for (auto &label : labels) {
    label.type = &lv_label_class;
    label.text = lv_color_hex(DARK_THEME.text_primary);
    collected.children.push_back(&label);
  }
  for (auto &action : actions) {
    action.type = &lv_button_class;
    action.background = lv_color_hex(DARK_THEME.setup_action);
    collected.children.push_back(&action);
  }
  theme_warnings.clear();
  assert(register_theme_static_page(&collected));
  assert(theme_static_pages()[0].label_count == 12);
  assert(theme_static_pages()[0].action_count == 2);
  assert(theme_warnings.size() == 2);
  set_active_theme_palette(LIGHT_THEME);
  apply_current_theme();
  assert(labels[11].text.full == LIGHT_THEME.text_primary);
  assert(labels[12].text.full == DARK_THEME.text_primary);  // Overflow is explicit.
  assert(actions[1].background.full == LIGHT_THEME.setup_action);
  assert(actions[2].background.full == DARK_THEME.setup_action);
  set_active_theme_palette(DARK_THEME);
  apply_current_theme();
  deleted.target = &collected;
  collected.delete_callback(&deleted);

  // A failed outer registry reservation must release its static-page slot.
  int owners[16];
  const auto noop = [](void *, const ThemePalette &) {};
  for (auto &owner : owners) assert(register_theme_refresh(&owner, noop, nullptr));
  theme_warnings.clear();
  lv_obj_t rejected;
  assert(!register_theme_static_page(&rejected));
  assert(theme_static_pages()[0].page == nullptr);
  assert(rejected.delete_callback == nullptr);
  assert(theme_warnings.size() == 1);
  assert(theme_warnings.back().find("Refresh registry full") != std::string::npos);
  for (auto &owner : owners) unregister_theme_refresh(&owner);
  assert(register_theme_static_page(&rejected));
  deleted.target = &rejected;
  rejected.delete_callback(&deleted);

  // Exercise the actual modal tracking functions extracted from production.
  lv_obj_t panel, tabs[11], pressed[CONTROL_MODAL_THEME_PRESSED_CAPACITY + 1], disabled[9];
  auto &modal = control_modal_theme_targets()[0];
  modal.panel = &panel;
  theme_warnings.clear();
  for (auto &tab : tabs) { tab.parent = &panel; control_modal_track_theme_tab(&tab); }
  for (auto &button : pressed) { button.parent = &panel; control_modal_track_theme_pressed(&button); }
  for (auto &button : disabled) { button.parent = &panel; control_modal_track_theme_disabled(&button); }
  assert(modal.theme_tab_count == 10 && modal.theme_pressed_count == CONTROL_MODAL_THEME_PRESSED_CAPACITY && modal.theme_disabled_count == 8);
  assert(theme_warnings.size() == 3);
  assert(theme_warnings[0].find("Modal tab targets full") != std::string::npos);
  assert(theme_warnings[1].find("Modal pressed targets full") != std::string::npos);
  assert(theme_warnings[2].find("Modal disabled targets full") != std::string::npos);
  assert(tabs[10].delete_callback == nullptr && pressed[CONTROL_MODAL_THEME_PRESSED_CAPACITY].delete_callback == nullptr &&
         disabled[8].delete_callback == nullptr);
  control_modal_track_theme_tab(&tabs[0]);
  control_modal_track_theme_pressed(&pressed[0]);
  control_modal_track_theme_disabled(&disabled[0]);
  assert(theme_warnings.size() == 3);  // Existing entries never consume another slot.
  deleted.target = &tabs[0]; tabs[0].delete_callback(&deleted);
  deleted.target = &pressed[0]; pressed[0].delete_callback(&deleted);
  deleted.target = &disabled[0]; disabled[0].delete_callback(&deleted);
  control_modal_track_theme_tab(&tabs[10]);
  control_modal_track_theme_pressed(&pressed[CONTROL_MODAL_THEME_PRESSED_CAPACITY]);
  control_modal_track_theme_disabled(&disabled[8]);
  assert(modal.theme_tab_count == 10 && modal.theme_pressed_count == CONTROL_MODAL_THEME_PRESSED_CAPACITY && modal.theme_disabled_count == 8);
  assert(theme_warnings.size() == 3);
  modal = {};
  // Image modal content intentionally has no neutral state targets.
  modal.panel = &panel; modal.content_owned = true;
  control_modal_track_theme_tab(&tabs[0]);
  assert(modal.theme_tab_count == 0 && theme_warnings.size() == 3);
  modal = {};
  for (auto &owner : owners) assert(register_theme_refresh(&owner, noop, nullptr));
  lv_obj_t overlay;
  ControlModalThemeTargets targets;
  targets.overlay = &overlay;
  control_modal_register_theme(targets);
  assert(theme_warnings.size() == 4);
  assert(theme_warnings.back().find("Refresh registry full") != std::string::npos);
  deleted.target = &overlay; overlay.delete_callback(&deleted);
  assert(modal.overlay == nullptr);
  for (auto &owner : owners) unregister_theme_refresh(&owner);
}

int main() {
  ThemePalette alternate = DARK_THEME;
  alternate.background = 0x101112;
  alternate.surface_primary = 0x202122;
  alternate.surface_secondary = 0x303132;
  alternate.text_primary = 0x404142;
  alternate.text_muted = 0x505152;
  alternate.text_disabled = 0x606162;
  alternate.track_background = 0x707172;
  alternate.border = 0x909192;
  alternate.control_neutral = 0xA0A1A2;

  lv_obj_t panel;
  panel.background = lv_color_hex(DARK_THEME.surface_secondary);
  panel.opacity = LV_OPA_COVER;
  lv_obj_t title;
  title.type = &lv_label_class;
  title.text = lv_color_hex(DARK_THEME.text_primary);
  lv_obj_t metadata;
  metadata.type = &lv_label_class;
  metadata.text = lv_color_hex(DARK_THEME.text_muted);
  lv_obj_t unavailable;
  unavailable.type = &lv_label_class;
  unavailable.text = lv_color_hex(DARK_THEME.text_disabled);
  lv_obj_t slider;
  slider.type = &lv_slider_class;
  slider.background = lv_color_hex(DARK_THEME.track_background);
  slider.opacity = LV_OPA_COVER;
  slider.knob = lv_color_hex(DARK_THEME.text_primary);
  slider.border = lv_color_hex(DARK_THEME.border);
  slider.border_width = 1;
  lv_obj_t arc;
  arc.type = &lv_arc_class;
  arc.arc = lv_color_hex(DARK_THEME.track_background);
  arc.knob = lv_color_hex(DARK_THEME.text_primary);
  lv_obj_t selected;
  selected.background = lv_color_hex(0xAABBCC);
  selected.opacity = LV_OPA_COVER;
  lv_obj_t selected_label;
  selected_label.type = &lv_label_class;
  selected_label.text = lv_color_hex(DARK_THEME.text_primary);
  selected.children.push_back(&selected_label);
  lv_obj_t error;
  error.background = lv_color_hex(0xB00020);
  error.opacity = LV_OPA_COVER;
  lv_obj_t error_label;
  error_label.type = &lv_label_class;
  error_label.text = lv_color_hex(DARK_THEME.text_primary);
  error.children.push_back(&error_label);
  lv_obj_t artwork;
  artwork.type = &lv_image_class;
  lv_obj_t artwork_label;
  artwork_label.type = &lv_label_class;
  artwork_label.text = lv_color_hex(DARK_THEME.text_primary);
  artwork.children.push_back(&artwork_label);
  lv_obj_t active_tab;
  active_tab.background = lv_color_hex(DARK_THEME.text_primary);
  active_tab.opacity = LV_OPA_COVER;
  lv_obj_t active_tab_label;
  active_tab_label.type = &lv_label_class;
  active_tab_label.text = lv_color_hex(DARK_THEME.surface_secondary);
  active_tab.children.push_back(&active_tab_label);
  panel.children = {&title, &metadata, &unavailable, &slider, &arc,
                    &selected, &error, &artwork, &active_tab};
  panel.state = LV_STATE_PRESSED;  // An open control keeps its interaction state.
  selected.state = LV_STATE_DISABLED;
  lv_obj_t *active_page = &panel;

  theme_restyle_tree(&panel, DARK_THEME, alternate);
  assert(panel.background.full == alternate.surface_secondary);
  assert(title.text.full == alternate.text_primary);
  assert(metadata.text.full == alternate.text_muted);
  assert(unavailable.text.full == alternate.text_disabled);
  assert(slider.background.full == alternate.track_background);
  assert(slider.knob.full == alternate.text_primary);
  assert(slider.border.full == alternate.border);
  assert(arc.arc.full == alternate.track_background);
  assert(arc.knob.full == alternate.text_primary);
  assert(selected.background.full == 0xAABBCC);
  assert(selected_label.text.full == DARK_THEME.text_primary);
  assert(error.background.full == 0xB00020);
  assert(error_label.text.full == DARK_THEME.text_primary);
  assert(artwork_label.text.full == DARK_THEME.text_primary);
  assert(panel.state == LV_STATE_PRESSED);
  assert(selected.state == LV_STATE_DISABLED);
  assert(active_page == &panel);
  theme_restyle_tab(&active_tab, alternate);
  assert(active_tab.background.full == alternate.text_primary);
  assert(active_tab_label.text.full == alternate.surface_secondary);
  theme_restyle_pressed_fill(&active_tab, alternate);
  assert(active_tab.pressed_background.full == alternate.surface_primary);
  lv_obj_t disabled_step;
  lv_obj_t disabled_step_label;
  disabled_step.children.push_back(&disabled_step_label);
  theme_restyle_disabled_step(&disabled_step, alternate);
  assert(disabled_step.border.full == alternate.control_neutral);
  assert(disabled_step.disabled_border.full == alternate.track_background);
  assert(disabled_step_label.text.full == alternate.text_primary);
  assert(disabled_step_label.disabled_text.full == alternate.track_background);

  const ThemeTreeCorrection correction{80, 90, 100};
  lv_obj_t corrected_card;
  corrected_card.opacity = LV_OPA_COVER;
  corrected_card.background = lv_color_hex(
      theme_tree_corrected(DARK_THEME.surface_primary, correction));
  lv_obj_t corrected_track;
  corrected_track.type = &lv_slider_class;
  corrected_track.opacity = LV_OPA_COVER;
  corrected_track.background = lv_color_hex(
      theme_tree_corrected(DARK_THEME.track_background, correction));
  corrected_card.children.push_back(&corrected_track);
  theme_restyle_tree(&corrected_card, DARK_THEME, alternate, false, correction);
  assert(corrected_card.background.full ==
         theme_tree_corrected(alternate.surface_primary, correction));
  assert(corrected_track.background.full ==
         theme_tree_corrected(alternate.track_background, correction));
  theme_restyle_tree(&corrected_card, alternate, DARK_THEME, false, correction);
  assert(corrected_card.background.full ==
         theme_tree_corrected(DARK_THEME.surface_primary, correction));

  theme_restyle_tree(&panel, alternate, DARK_THEME);
  assert(panel.background.full == DARK_THEME.surface_secondary);
  assert(title.text.full == DARK_THEME.text_primary);
  assert(metadata.text.full == DARK_THEME.text_muted);
  assert(slider.background.full == DARK_THEME.track_background);
  assert(slider.border.full == DARK_THEME.border);
  assert(arc.arc.full == DARK_THEME.track_background);
  assert(selected_label.text.full == DARK_THEME.text_primary);
  assert(error_label.text.full == DARK_THEME.text_primary);
  assert(artwork_label.text.full == DARK_THEME.text_primary);
  theme_restyle_tab(&active_tab, DARK_THEME);
  assert(active_tab.background.full == DARK_THEME.text_primary);
  assert(active_tab_label.text.full == DARK_THEME.surface_secondary);
  theme_restyle_pressed_fill(&active_tab, DARK_THEME);
  assert(active_tab.pressed_background.full == DARK_THEME.surface_primary);
  theme_restyle_disabled_step(&disabled_step, DARK_THEME);
  assert(disabled_step.disabled_border.full == DARK_THEME.track_background);

  // Exercise the production palettes through the same in-place tree adapter.
  theme_restyle_tree(&panel, DARK_THEME, LIGHT_THEME);
  assert(panel.background.full == LIGHT_THEME.surface_secondary);
  assert(title.text.full == LIGHT_THEME.text_primary);
  assert(metadata.text.full == LIGHT_THEME.text_muted);
  assert(unavailable.text.full == LIGHT_THEME.text_disabled);
  assert(slider.background.full == LIGHT_THEME.track_background);
  assert(slider.knob.full == LIGHT_THEME.text_primary);
  assert(selected.background.full == 0xAABBCC);
  assert(selected_label.text.full == DARK_THEME.text_primary);
  assert(error.background.full == 0xB00020);
  assert(artwork_label.text.full == DARK_THEME.text_primary);
  assert(panel.state == LV_STATE_PRESSED && selected.state == LV_STATE_DISABLED);
  assert(active_page == &panel);
  theme_restyle_tree(&panel, LIGHT_THEME, DARK_THEME);
  assert(panel.background.full == DARK_THEME.surface_secondary);
  assert(title.text.full == DARK_THEME.text_primary);
  assert(slider.background.full == DARK_THEME.track_background);

  // A YAML-created setup page starts with compile-time Dark styles. The
  // registered object references must refresh and clean up on deletion.
  lv_obj_t setup_page;
  setup_page.background = lv_color_hex(DARK_THEME.background);
  setup_page.opacity = LV_OPA_COVER;
  lv_obj_t setup_title;
  setup_title.type = &lv_label_class;
  setup_title.text = lv_color_hex(DARK_THEME.text_primary);
  lv_obj_t setup_hint;
  setup_hint.type = &lv_label_class;
  setup_hint.text = lv_color_hex(DARK_THEME.text_muted);
  lv_obj_t setup_action;
  setup_action.type = &lv_button_class;
  setup_action.background = lv_color_hex(DARK_THEME.setup_action);
  setup_action.opacity = LV_OPA_COVER;
  setup_page.children = {&setup_title, &setup_hint, &setup_action};
  assert(register_theme_static_page(&setup_page));
  assert(register_theme_static_page(&setup_page));
  alternate.setup_action = 0x808182;
  set_active_theme_palette(alternate);
  apply_current_theme();
  assert(setup_page.background.full == alternate.background);
  assert(setup_title.text.full == alternate.text_primary);
  assert(setup_hint.text.full == alternate.text_muted);
  assert(setup_action.background.full == alternate.setup_action);
  set_active_theme_palette(DARK_THEME);
  apply_current_theme();
  assert(setup_page.background.full == DARK_THEME.background);
  assert(setup_title.text.full == DARK_THEME.text_primary);
  assert(setup_action.background.full == DARK_THEME.setup_action);
  lv_event_t deleted{&setup_page};
  setup_page.delete_callback(&deleted);
  assert(theme_static_pages()[0].page == nullptr);
  for (const auto &binding : theme_refresh_bindings())
    assert(binding.owner != &setup_page);

  lv_obj_t clock_page;
  clock_page.background = lv_color_hex(DARK_THEME.background);
  lv_obj_t user_clock_text;
  user_clock_text.type = &lv_label_class;
  user_clock_text.text = lv_color_hex(DARK_THEME.text_primary);
  clock_page.children.push_back(&user_clock_text);
  assert(register_theme_static_page(&clock_page, false));
  set_active_theme_palette(alternate);
  apply_current_theme();
  assert(clock_page.background.full == alternate.background);
  assert(user_clock_text.text.full == DARK_THEME.text_primary);
  lv_event_t clock_deleted{&clock_page};
  clock_page.delete_callback(&clock_deleted);
  lv_obj_t late_setup_page;
  late_setup_page.background = lv_color_hex(DARK_THEME.background);
  lv_obj_t late_title;
  late_title.type = &lv_label_class;
  late_title.text = lv_color_hex(DARK_THEME.text_primary);
  late_setup_page.children.push_back(&late_title);
  assert(register_theme_static_page(&late_setup_page));
  assert(late_setup_page.background.full == alternate.background);
  assert(late_title.text.full == alternate.text_primary);
  lv_event_t late_deleted{&late_setup_page};
  late_setup_page.delete_callback(&late_deleted);
  set_active_theme_palette(DARK_THEME);
  apply_current_theme();

  // A forced-Light build creates YAML pages with Light colors already in place.
  // They still need role discovery so a later Light -> Dark switch works.
  set_active_theme_palette(LIGHT_THEME);
  lv_obj_t light_page;
  light_page.background = lv_color_hex(LIGHT_THEME.background);
  light_page.opacity = LV_OPA_COVER;
  lv_obj_t light_title;
  light_title.type = &lv_label_class;
  light_title.text = lv_color_hex(LIGHT_THEME.text_primary);
  lv_obj_t light_hint;
  light_hint.type = &lv_label_class;
  light_hint.text = lv_color_hex(LIGHT_THEME.text_muted);
  lv_obj_t light_action;
  light_action.type = &lv_button_class;
  light_action.background = lv_color_hex(LIGHT_THEME.setup_action);
  light_page.children = {&light_title, &light_hint, &light_action};
  assert(register_theme_static_page(&light_page));
  assert(light_title.text.full == LIGHT_THEME.text_primary);
  set_active_theme_palette(DARK_THEME);
  apply_current_theme();
  assert(light_page.background.full == DARK_THEME.background);
  assert(light_title.text.full == DARK_THEME.text_primary);
  assert(light_hint.text.full == DARK_THEME.text_muted);
  assert(light_action.background.full == DARK_THEME.setup_action);
  lv_event_t light_deleted{&light_page};
  light_page.delete_callback(&light_deleted);

  lv_obj_t late_light_page;
  late_light_page.background = lv_color_hex(LIGHT_THEME.background);
  lv_obj_t late_light_title;
  late_light_title.type = &lv_label_class;
  late_light_title.text = lv_color_hex(LIGHT_THEME.text_primary);
  late_light_page.children.push_back(&late_light_title);
  assert(register_theme_static_page(&late_light_page));
  assert(late_light_page.background.full == DARK_THEME.background);
  assert(late_light_title.text.full == DARK_THEME.text_primary);
  lv_event_t late_light_deleted{&late_light_page};
  late_light_page.delete_callback(&late_light_deleted);
  test_supported_pressed_lists();
  test_grid_secondary_surface();
  test_independent_dark_light_surfaces();
  test_restore_grid_theme(ThemeMode::LIGHT, EffectiveTheme::LIGHT);
  test_restore_grid_theme(ThemeMode::DARK, EffectiveTheme::DARK);
  test_restore_grid_theme(ThemeMode::DARK, EffectiveTheme::DARK, true);
  test_restore_grid_theme(ThemeMode::AUTO, EffectiveTheme::LIGHT);
  test_restore_grid_theme(ThemeMode::AUTO, EffectiveTheme::DARK);
  test_playback_mode_accent_collision();
  test_sensor_state_refresh();
  test_pressed_overflow_recovery();
  test_content_collisions();
  test_registry_exhaustion();
}
