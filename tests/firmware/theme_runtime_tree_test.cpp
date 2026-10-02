#include <cassert>
#include <cstdint>
#include <vector>

struct lv_color_t { uint32_t full; };
constexpr lv_color_t lv_color_hex(uint32_t rgb) { return {rgb}; }
struct lv_color32_t { uint32_t full; };
constexpr lv_color32_t lv_color_to_32(lv_color_t color, int) { return {color.full}; }
constexpr bool lv_color_eq(lv_color_t left, lv_color_t right) { return left.full == right.full; }
constexpr int LV_PART_MAIN = 0;
constexpr int LV_PART_KNOB = 1;
constexpr int LV_STATE_PRESSED = 2;
constexpr int LV_STATE_DISABLED = 4;
using lv_style_selector_t = int;
constexpr int LV_OPA_TRANSP = 0;
constexpr int LV_OPA_COVER = 255;

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
  lv_color_t disabled_border{0};
  lv_color_t disabled_text{0};
  int border_width = 0;
  int opacity = LV_OPA_TRANSP;
  int state = 0;
  std::vector<lv_obj_t *> children;
  lv_event_cb_t delete_callback = nullptr;
};

bool lv_obj_check_type(const lv_obj_t *obj, const lv_obj_class_t *type) {
  return obj->type == type;
}
int lv_obj_get_style_bg_opa(const lv_obj_t *obj, int) { return obj->opacity; }
lv_color_t lv_obj_get_style_bg_color(const lv_obj_t *obj, int part) {
  return part == LV_PART_KNOB ? obj->knob : obj->background;
}
lv_color_t lv_obj_get_style_text_color(const lv_obj_t *obj, int) { return obj->text; }
lv_color_t lv_obj_get_style_arc_color(const lv_obj_t *obj, int) { return obj->arc; }
lv_color_t lv_obj_get_style_border_color(const lv_obj_t *obj, int) { return obj->border; }
int lv_obj_get_style_border_width(const lv_obj_t *obj, int) { return obj->border_width; }
void lv_obj_set_style_bg_color(lv_obj_t *obj, lv_color_t color, int part) {
  (part == LV_STATE_PRESSED ? obj->pressed_background :
   part == LV_PART_KNOB ? obj->knob : obj->background) = color;
}
void lv_obj_set_style_text_color(lv_obj_t *obj, lv_color_t color, int selector) {
  (selector == LV_STATE_DISABLED ? obj->disabled_text : obj->text) = color;
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
}
