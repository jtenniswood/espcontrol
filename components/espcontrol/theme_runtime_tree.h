#pragma once

#include "theme_palette.h"

// Restyle only neutral application chrome. This adapter is called by owners
// already registered with the theme refresh boundary. Artwork/screensaver trees
// are never passed here. It does not change values, states, or object lifetimes.
inline bool theme_color_matches(lv_color_t color, uint32_t rgb) {
  return lv_color_eq(color, lv_color_hex(rgb));
}

struct ThemeTreeCorrection {
  int red = 100;
  int green = 100;
  int blue = 100;
};

inline uint32_t theme_tree_corrected(uint32_t rgb, ThemeTreeCorrection correction) {
  return correct_display_color(rgb, correction.red, correction.green, correction.blue);
}

inline bool theme_neutral_background(lv_color_t color, const ThemePalette &theme,
                                     ThemeTreeCorrection correction = {}) {
  return theme_color_matches(color, theme.background) ||
         theme_color_matches(color, theme_display_color(theme.surface_primary)) ||
         theme_color_matches(color, theme_display_color(theme.surface_secondary)) ||
         theme_color_matches(color, theme_display_color(theme.control_neutral)) ||
         theme_color_matches(color, theme_display_color(theme.track_background)) ||
         theme_color_matches(color, theme_tree_corrected(theme.surface_primary, correction)) ||
         theme_color_matches(color, theme_tree_corrected(theme.surface_secondary, correction)) ||
         theme_color_matches(color, theme_tree_corrected(theme.track_background, correction)) ||
         theme_color_matches(color, theme.overlay);
}

inline void theme_restyle_tree(lv_obj_t *obj, const ThemePalette &previous,
                               const ThemePalette &theme, bool content_contrast = false,
                               ThemeTreeCorrection correction = {}) {
  if (!obj || lv_obj_check_type(obj, &lv_image_class)) return;

  const bool opaque = lv_obj_get_style_bg_opa(obj, LV_PART_MAIN) != LV_OPA_TRANSP;
  const lv_color_t background = lv_obj_get_style_bg_color(obj, LV_PART_MAIN);
  const bool own_content_background = opaque &&
      !theme_neutral_background(background, previous, correction) &&
      !theme_neutral_background(background, theme, correction);
  const bool preserve_foreground = content_contrast || own_content_background;
  const bool slider = lv_obj_check_type(obj, &lv_slider_class);
  const bool arc = lv_obj_check_type(obj, &lv_arc_class);

  if (opaque && !own_content_background && !slider) {
    if (theme_color_matches(background, theme_tree_corrected(previous.surface_secondary, correction))) {
      lv_obj_set_style_bg_color(obj, lv_color_hex(theme_tree_corrected(theme.surface_secondary, correction)), LV_PART_MAIN);
    } else if (theme_color_matches(background, theme_tree_corrected(previous.surface_primary, correction))) {
      lv_obj_set_style_bg_color(obj, lv_color_hex(theme_tree_corrected(theme.surface_primary, correction)), LV_PART_MAIN);
    } else if (theme_color_matches(background, theme_display_color(previous.surface_secondary))) {
      lv_obj_set_style_bg_color(obj, lv_color_hex(theme_display_color(theme.surface_secondary)), LV_PART_MAIN);
    } else if (theme_color_matches(background, theme_display_color(previous.surface_primary))) {
      lv_obj_set_style_bg_color(obj, lv_color_hex(theme_display_color(theme.surface_primary)), LV_PART_MAIN);
    } else if (theme_color_matches(background, previous.background)) {
      lv_obj_set_style_bg_color(obj, lv_color_hex(theme.background), LV_PART_MAIN);
    }
  }

  if (lv_obj_check_type(obj, &lv_label_class) && !preserve_foreground) {
    const lv_color_t text = lv_obj_get_style_text_color(obj, LV_PART_MAIN);
    if (theme_color_matches(text, previous.text_muted)) {
      lv_obj_set_style_text_color(obj, lv_color_hex(theme.text_muted), LV_PART_MAIN);
    } else if (theme_color_matches(text, previous.text_disabled)) {
      lv_obj_set_style_text_color(obj, lv_color_hex(theme.text_disabled), LV_PART_MAIN);
    } else if (theme_color_matches(text, previous.text_primary)) {
      lv_obj_set_style_text_color(obj, lv_color_hex(theme.text_primary), LV_PART_MAIN);
    }
  }

  if (lv_obj_get_style_border_width(obj, LV_PART_MAIN) > 0) {
    const lv_color_t border = lv_obj_get_style_border_color(obj, LV_PART_MAIN);
    if (theme_color_matches(border, theme_tree_corrected(previous.border, correction)))
      lv_obj_set_style_border_color(obj,
          lv_color_hex(theme_tree_corrected(theme.border, correction)), LV_PART_MAIN);
    else if (theme_color_matches(border, theme_display_color(previous.border)))
      lv_obj_set_style_border_color(obj, lv_color_hex(theme_display_color(theme.border)), LV_PART_MAIN);
    else if (!preserve_foreground && theme_color_matches(border, previous.text_primary))
      lv_obj_set_style_border_color(obj, lv_color_hex(theme.text_primary), LV_PART_MAIN);
  }

  if (slider || arc) {
    const lv_color_t track = arc
        ? lv_obj_get_style_arc_color(obj, LV_PART_MAIN)
        : lv_obj_get_style_bg_color(obj, LV_PART_MAIN);
    if (theme_color_matches(track, theme_tree_corrected(previous.track_background, correction)) ||
        theme_color_matches(track, theme_tree_corrected(previous.surface_primary, correction))) {
      if (arc)
        lv_obj_set_style_arc_color(obj, lv_color_hex(theme_tree_corrected(theme.track_background, correction)), LV_PART_MAIN);
      else
        lv_obj_set_style_bg_color(obj, lv_color_hex(theme_tree_corrected(theme.track_background, correction)), LV_PART_MAIN);
    } else if (theme_color_matches(track, theme_display_color(previous.track_background)) ||
        theme_color_matches(track, theme_display_color(previous.surface_primary))) {
      if (arc)
        lv_obj_set_style_arc_color(obj, lv_color_hex(theme_display_color(theme.track_background)), LV_PART_MAIN);
      else
        lv_obj_set_style_bg_color(obj, lv_color_hex(theme_display_color(theme.track_background)), LV_PART_MAIN);
    }
    const lv_color_t knob = lv_obj_get_style_bg_color(obj, LV_PART_KNOB);
    if (theme_color_matches(knob, previous.text_primary))
      lv_obj_set_style_bg_color(obj, lv_color_hex(theme.text_primary), LV_PART_KNOB);
  }

  const uint32_t count = lv_obj_get_child_cnt(obj);
  for (uint32_t i = 0; i < count; ++i)
    theme_restyle_tree(lv_obj_get_child(obj, i), previous, theme, preserve_foreground, correction);
}

inline void theme_restyle_tab(lv_obj_t *tab, const ThemePalette &theme) {
  if (!tab) return;
  const bool active = lv_obj_get_style_bg_opa(tab, LV_PART_MAIN) == LV_OPA_COVER;
  lv_obj_set_style_bg_color(tab, lv_color_hex(active ? theme.text_primary :
                             theme_display_color(theme.surface_primary)), LV_PART_MAIN);
  lv_obj_t *label = lv_obj_get_child(tab, 0);
  if (label) lv_obj_set_style_text_color(label,
      lv_color_hex(active ? theme_display_color(theme.surface_secondary) :
                             theme.text_primary), LV_PART_MAIN);
}

inline void theme_restyle_pressed_fill(lv_obj_t *button, const ThemePalette &theme) {
  if (!button) return;
  lv_obj_set_style_bg_color(button, lv_color_hex(theme_display_color(theme.surface_primary)),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) |
      static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
}

inline void theme_restyle_disabled_step(lv_obj_t *button, const ThemePalette &theme) {
  if (!button) return;
  const auto selector = static_cast<lv_style_selector_t>(LV_PART_MAIN) |
                        static_cast<lv_style_selector_t>(LV_STATE_DISABLED);
  lv_obj_set_style_border_color(button,
      lv_color_hex(theme_display_color(theme.control_neutral)), LV_PART_MAIN);
  lv_obj_set_style_border_color(button,
      lv_color_hex(theme_display_color(theme.track_background)), selector);
  lv_obj_t *label = lv_obj_get_child(button, 0);
  if (label) {
    lv_obj_set_style_text_color(label, lv_color_hex(theme.text_primary), LV_PART_MAIN);
    lv_obj_set_style_text_color(label,
        lv_color_hex(theme_display_color(theme.track_background)), selector);
  }
}
