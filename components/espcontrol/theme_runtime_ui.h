#pragma once

#include "theme_runtime_static.h"

// Runtime LVGL targets for the palette refresh boundary. Dark -> Dark leaves
// existing styles, state, and Home Assistant bindings alone.

struct ThemeGridTargets {
  lv_obj_t *main_page = nullptr;
  lv_obj_t *buttons[MAX_GRID_SLOTS]{};
  bool neutral_buttons[MAX_GRID_SLOTS]{};
  bool sensor_surfaces[MAX_GRID_SLOTS]{};
  bool secondary_surfaces[MAX_GRID_SLOTS]{};
  int count = 0;
  int red_percent = 100;
  int green_percent = 100;
  int blue_percent = 100;
};

inline ThemeGridTargets &theme_grid_targets() {
  static ThemeGridTargets targets;
  return targets;
}

inline uint32_t theme_grid_correct_color(uint32_t raw_rgb,
                                         const ThemeGridTargets &targets) {
  return correct_display_color(raw_rgb, targets.red_percent,
                               targets.green_percent, targets.blue_percent);
}

inline uint32_t current_grid_sensor_color() {
  return theme_grid_correct_color(current_theme().surface_secondary, theme_grid_targets());
}

inline uint32_t current_grid_sensor_surface_color() {
  return theme_grid_correct_color(current_theme().surface_sensor, theme_grid_targets());
}

inline void theme_apply_grid_button(lv_obj_t *button, uint32_t neutral,
                                    const ThemePalette &theme,
                                    bool secondary_surface = false) {
  if (!button) return;
  // Change only the neutral/default fill. Checked and pressed backgrounds are
  // owned by the persisted user accent and the card's state callbacks.
  const bool content_fill = lv_obj_has_flag(button, LV_OBJ_FLAG_USER_1) &&
                            !lv_obj_has_flag(button, LV_OBJ_FLAG_USER_3);
  if (!content_fill) {
    // Store the surface role explicitly: cards and information panels are
    // both white in Light, but must return to different Dark colors.
    if (secondary_surface) neutral = current_grid_sensor_color();
    lv_obj_set_style_bg_color(button, lv_color_hex(neutral), LV_PART_MAIN);
  }
  lv_obj_set_style_text_color(button,
      lv_color_hex(content_fill ? CARD_ACCENT_TEXT_COLOR : theme.text_primary), LV_PART_MAIN);
  // Accent/checked foreground stays white for contrast even when the neutral
  // palette uses dark text. The accent itself remains card/user-owned.
  lv_obj_set_style_text_color(button, lv_color_hex(CARD_ACCENT_TEXT_COLOR),
                              static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(button, lv_color_hex(CARD_ACCENT_TEXT_COLOR),
                              static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_PRESSED);
  sync_card_checked_text_color(button);
  set_card_content_disabled(button, lv_obj_has_state(button, LV_STATE_DISABLED));
}

inline void theme_apply_grid(void *context, const ThemePalette &theme) {
  auto &targets = *static_cast<ThemeGridTargets *>(context);
  if (!targets.main_page) return;
  lv_obj_set_style_bg_color(targets.main_page, lv_color_hex(theme.background), LV_PART_MAIN);
  const uint32_t neutral = theme_grid_correct_color(theme.surface_card, targets);
  const ThemeTreeCorrection correction = {targets.red_percent, targets.green_percent,
                                          targets.blue_percent};
  for (int i = 0; i < targets.count; ++i) {
    const bool sensor_surface = targets.sensor_surfaces[i];
    if ((targets.neutral_buttons[i] || sensor_surface) && targets.buttons[i]) {
      const bool accent_state = lv_obj_has_state(targets.buttons[i], LV_STATE_CHECKED) ||
                                lv_obj_has_state(targets.buttons[i], LV_STATE_PRESSED);
      theme_restyle_tree(targets.buttons[i], theme_refresh_previous(), theme, accent_state,
                         correction, sensor_surface);
      if (!sensor_surface) theme_apply_grid_button(
          targets.buttons[i], neutral, theme, targets.secondary_surfaces[i]);
    }
  }
  for (auto &entry : navigation_subpages()) {
    if (!entry.screen) continue;
    lv_obj_set_style_bg_color(entry.screen, lv_color_hex(theme.background), LV_PART_MAIN);
    theme_restyle_tree(entry.back_button, theme_refresh_previous(), theme, false, correction);
    theme_apply_grid_button(entry.back_button, neutral, theme);
    for (auto &card : entry.cards) {
      const bool sensor_surface = card.sensor_surface;
      if ((card.neutral_background || sensor_surface) && card.button) {
        const bool accent_state = lv_obj_has_state(card.button, LV_STATE_CHECKED) ||
                                  lv_obj_has_state(card.button, LV_STATE_PRESSED);
        theme_restyle_tree(card.button, theme_refresh_previous(), theme, accent_state,
                           correction, sensor_surface);
        if (!sensor_surface) theme_apply_grid_button(
            card.button, neutral, theme, card.secondary_surface);
      }
    }
  }
}

inline void register_theme_grid(lv_obj_t *main_page, BtnSlot *slots,
                                const bool *neutral_buttons, int count,
                                const bool *sensor_surfaces,
                                int red_percent, int green_percent,
                                int blue_percent,
                                const bool *secondary_surfaces = nullptr) {
  if (!main_page || !slots || !neutral_buttons || !sensor_surfaces) return;
  auto &targets = theme_grid_targets();
  const bool new_owner = targets.main_page != main_page;
  targets.main_page = main_page;
  targets.count = bounded_grid_slots(count);
  targets.red_percent = red_percent;
  targets.green_percent = green_percent;
  targets.blue_percent = blue_percent;
  for (int i = 0; i < targets.count; ++i) {
    targets.buttons[i] = slots[i].btn;
    targets.neutral_buttons[i] = neutral_buttons[i];
    targets.sensor_surfaces[i] = sensor_surfaces[i];
    targets.secondary_surfaces[i] = secondary_surfaces && secondary_surfaces[i];
  }
  for (int i = targets.count; i < MAX_GRID_SLOTS; ++i) {
    targets.buttons[i] = nullptr;
    targets.sensor_surfaces[i] = false;
    targets.secondary_surfaces[i] = false;
  }
  if (!register_theme_refresh(main_page, theme_apply_grid, &targets))
    ESP_LOGW("theme", "Refresh registry full; grid will not follow theme changes");
  if (new_owner) {
    lv_obj_add_event_cb(main_page, [](lv_event_t *event) {
      unregister_theme_refresh(lv_event_get_target(event));
      theme_grid_targets().main_page = nullptr;
    }, LV_EVENT_DELETE, nullptr);
  }
  // Phase 2 still has to bind cards and replace subpages. Apply once that
  // reconstruction is complete, rather than caching an early partial refresh.
}

inline void refresh_theme_grid_after_rebuild() {
  auto &targets = theme_grid_targets();
  if (!targets.main_page) return;
  // The owner survives a rebuild, but its new descendants can inherit the
  // compile-time Dark styles. Reset only this binding's applied palette by
  // re-registering it, then use the normal refresh path without changing mode.
  unregister_theme_refresh(targets.main_page);
  if (!register_theme_refresh(targets.main_page, theme_apply_grid, &targets))
    ESP_LOGW("theme", "Refresh registry full; rebuilt grid will not follow theme changes");
  apply_current_theme();
}

struct ThemeHudTargets {
  lv_obj_t *temperature = nullptr;
  lv_obj_t *time = nullptr;
  lv_obj_t *network = nullptr;
  lv_obj_t *night = nullptr;
};

inline ThemeHudTargets &theme_hud_targets() {
  static ThemeHudTargets targets;
  return targets;
}

inline void theme_apply_hud(void *context, const ThemePalette &theme) {
  const auto &targets = *static_cast<ThemeHudTargets *>(context);
  lv_obj_t *labels[] = {targets.temperature, targets.time, targets.network, targets.night};
  for (lv_obj_t *label : labels) {
    if (label) lv_obj_set_style_text_color(label, lv_color_hex(theme.clockbar_text), LV_PART_MAIN);
  }
}

inline void register_theme_hud(lv_obj_t *temperature, lv_obj_t *time,
                               lv_obj_t *network, lv_obj_t *night) {
  auto &targets = theme_hud_targets();
  const bool new_owner = targets.time != time;
  targets = {temperature, time, network, night};
  if (!time) return;
  if (!register_theme_refresh(time, theme_apply_hud, &targets))
    ESP_LOGW("theme", "Refresh registry full; HUD will not follow theme changes");
  if (new_owner) {
    lv_obj_add_event_cb(time, [](lv_event_t *event) {
      unregister_theme_refresh(lv_event_get_target(event));
      theme_hud_targets() = {};
    }, LV_EVENT_DELETE, nullptr);
  }
  apply_current_theme();
}
