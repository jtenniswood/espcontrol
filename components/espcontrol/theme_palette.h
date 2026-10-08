#pragma once

#include <cstdint>

#include "display_color.h"

// Raw RGB values. Device/profile correction belongs at the LVGL call site,
// just as it did before the palette was introduced.
struct ThemePalette {
  uint32_t background;
  uint32_t surface_primary;
  uint32_t surface_secondary;
  uint32_t text_primary;
  uint32_t clockbar_text;
  uint32_t text_muted;
  uint32_t text_inverted;
  uint32_t text_disabled;
  uint32_t border;
  uint32_t control_neutral;
  uint32_t track_background;
  uint32_t overlay;
  uint32_t setup_action;
};

constexpr ThemePalette make_dark_theme() {
  ThemePalette theme{};
  theme.background = 0x303030;
  theme.surface_primary = 0x303030;
  theme.surface_secondary = 0x404040;
  theme.text_primary = 0xFFFFFF;
  theme.clockbar_text = 0xFFFFFF;
  theme.text_muted = 0xB0B0B0;
  theme.text_inverted = 0x000000;
  theme.text_disabled = 0x707070;
  theme.border = 0x303030;
  theme.control_neutral = 0x303030;
  theme.track_background = 0x313131;
  theme.overlay = 0x000000;
  theme.setup_action = 0x333333;
  return theme;
}

inline constexpr ThemePalette DARK_THEME = make_dark_theme();

// Raw RGB values for the first Light presentation. The separate setup action
// role keeps the existing Dark #333333 distinct from other neutral controls.
inline constexpr ThemePalette LIGHT_THEME = {
    0x404040,  // background
    0xFFFFFF,  // surface_primary
    0xE8E8E8,  // surface_secondary
    0x333333,  // text_primary
    0xFFFFFF,  // clockbar_text
    0x606060,  // text_muted
    0xFFFFFF,  // text_inverted
    0x9A9A9A,  // text_disabled
    0xD0D0D0,  // border
    0xE0E0E0,  // control_neutral
    0xD0D0D0,  // track_background
    0x000000,  // overlay
    0xE0E0E0,  // setup_action
};

// Palette instances must outlive their use by the UI. Firmware installs only
// the two static palettes; a host test may install a temporary palette in scope.
inline const ThemePalette *&active_theme_palette_ref() {
  static const ThemePalette *palette = &DARK_THEME;
  return palette;
}

inline const ThemePalette &current_theme() {
  return *active_theme_palette_ref();
}

inline void set_active_theme_palette(const ThemePalette &palette) {
  active_theme_palette_ref() = &palette;
}

inline constexpr uint32_t theme_display_color(uint32_t raw_rgb) {
  return correct_display_color(raw_rgb);
}

// A small, allocation-free dispatch boundary for live LVGL surfaces. Owners
// register after construction and unregister before deletion; callbacks apply
// the active palette in place without changing LVGL state or rebuilding cards.
using ThemeRefreshCallback = void (*)(void *, const ThemePalette &);

struct ThemeRefreshBinding {
  void *owner = nullptr;
  ThemeRefreshCallback callback = nullptr;
  void *context = nullptr;
  const ThemePalette *applied = nullptr;
};

inline ThemeRefreshBinding (&theme_refresh_bindings())[16] {
  static ThemeRefreshBinding bindings[16]{};
  return bindings;
}

inline const ThemePalette *&theme_refresh_previous_ref() {
  static const ThemePalette *previous = &DARK_THEME;
  return previous;
}

inline const ThemePalette &theme_refresh_previous() {
  return *theme_refresh_previous_ref();
}

inline bool register_theme_refresh(void *owner, ThemeRefreshCallback callback,
                                   void *context) {
  if (!owner || !callback) return false;
  for (auto &binding : theme_refresh_bindings()) {
    if (binding.owner == owner) {
      binding.callback = callback;
      binding.context = context;
      return true;
    }
  }
  for (auto &binding : theme_refresh_bindings()) {
    if (!binding.owner) {
      // YAML screens start with Dark compile-time styles. Runtime owners can
      // already use the active palette; the first refresh handles both cases.
      binding = {owner, callback, context, &DARK_THEME};
      return true;
    }
  }
  return false;
}

inline void unregister_theme_refresh(void *owner) {
  for (auto &binding : theme_refresh_bindings()) {
    if (binding.owner == owner) binding = {};
  }
}

inline void apply_current_theme() {
  for (auto &binding : theme_refresh_bindings()) {
    if (!binding.callback || binding.applied == &current_theme()) continue;
    void *owner = binding.owner;
    theme_refresh_previous_ref() = binding.applied;
    binding.callback(binding.context, current_theme());
    if (binding.owner == owner) binding.applied = &current_theme();
  }
  theme_refresh_previous_ref() = &current_theme();
}
