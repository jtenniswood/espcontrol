#pragma once

#include <cstdint>

// Pure modal geometry. This file deliberately has no ESPHome or LVGL
// dependencies so every supported display profile can be exercised by host
// tests without compiling firmware.

namespace espcontrol::modal {

enum class LayoutFamily : uint8_t {
  COMPACT_SQUARE,
  LARGE_SQUARE,
  COMPACT_PORTRAIT,
  WIDE_LANDSCAPE,
  LARGE_LANDSCAPE,
};

enum class Density : uint8_t {
  COMPACT,
  COMFORTABLE,
  SPACIOUS,
};

enum class MemoryTier : uint8_t {
  STANDARD,
  CONSTRAINED,
};

struct DeviceProfile {
  LayoutFamily layout_family = LayoutFamily::COMPACT_SQUARE;
  Density density = Density::COMPACT;
  MemoryTier memory_tier = MemoryTier::STANDARD;
  int base_touch_target = 46;
};

struct DesignTokens {
  int reference_side = 480;
  int arc_stroke = 17;
  int back_button = 46;
  int control_button = 80;
  int inset = 18;
  int controls_gap = 24;
  int controls_down = 22;
  int title_gap = 10;
};

constexpr DesignTokens DEFAULT_DESIGN_TOKENS{};

struct FrameRequest {
  int screen_width = 480;
  int screen_height = 480;
  int panel_left = 4;
  int panel_top = 0;
  int panel_right = 4;
  int panel_bottom = 0;
  int width_compensation_percent = 100;
  bool width_compensation_vertical = false;
  bool allow_compact_portrait_tuning = true;
};

struct FrameLayout {
  int screen_width = 480;
  int screen_height = 480;
  int short_side = 480;
  int panel_x = 4;
  int panel_y = 0;
  int panel_width = 472;
  int panel_height = 480;
  int inset = 18;
  int back_inset_x = 18;
  int back_inset_y = 18;
  int back_size = 46;
  int button_size = 80;
  int arc_stroke = 17;
  int controls_gap = 24;
  int arc_size = 320;
  int arc_center_x = 0;
  int arc_center_y = 0;
  int value_center_y = 0;
  int title_gap = 10;
  int controls_center_y = 0;
};

struct TabRequest {
  int tab_count = 1;
  bool show_tab_bar = false;
  bool avoid_back_button = true;
};

struct TabLayout {
  int tab_count = 1;
  bool show_tab_bar = false;
  int tab_size = 48;
  int selected_tab_size = 54;
  int frame_padding = 9;
  int tab_gap = 12;
  int tabs_total_width = 48;
  int frame_width = 66;
  int frame_height = 66;
  int safe_left = 0;
  int centered_left = 0;
  int row_left = 0;
  int content_gap = 16;
};

struct ContentRequest {
  bool show_tab_bar = false;
  int tab_frame_height = 0;
  int tab_content_gap = 0;
  int top_without_tabs = 0;
  int safe_top = 0;
  int minimum_height = 0;
  int fallback_height = 0;
};

struct ContentLayout {
  int top = 0;
  int bottom = 0;
  int width = 0;
  int height = 0;
  int center_y = 0;
};

constexpr bool uses_family(const DeviceProfile &profile, LayoutFamily family) {
  return profile.layout_family == family;
}

constexpr bool is_square_family(const DeviceProfile &profile) {
  return uses_family(profile, LayoutFamily::COMPACT_SQUARE) ||
         uses_family(profile, LayoutFamily::LARGE_SQUARE);
}

constexpr bool is_constrained(const DeviceProfile &profile) {
  return profile.memory_tier == MemoryTier::CONSTRAINED;
}

constexpr int normalized_width_percent(int percent) {
  if (percent <= 0) return 100;
  if (percent < 50) return 50;
  if (percent > 150) return 150;
  return percent;
}

constexpr int scaled_px(int px, int short_side,
                        const DesignTokens &tokens = DEFAULT_DESIGN_TOKENS) {
  return px * short_side / tokens.reference_side;
}

constexpr FrameLayout calculate_frame(
    const DeviceProfile &profile,
    const FrameRequest &request,
    const DesignTokens &tokens = DEFAULT_DESIGN_TOKENS) {
  FrameLayout layout;
  layout.screen_width = request.screen_width;
  layout.screen_height = request.screen_height;
  layout.short_side = request.screen_width < request.screen_height
    ? request.screen_width
    : request.screen_height;
  layout.panel_x = request.panel_left;
  layout.panel_y = request.panel_top;
  layout.panel_width = request.screen_width - request.panel_left - request.panel_right;
  layout.panel_height = request.screen_height - request.panel_top - request.panel_bottom;

  layout.back_size = scaled_px(tokens.back_button, layout.short_side, tokens);
  layout.button_size = scaled_px(tokens.control_button, layout.short_side, tokens);
  layout.inset = scaled_px(tokens.inset, layout.short_side, tokens);
  if (layout.inset < 8) layout.inset = 8;
  layout.back_inset_x = layout.inset;
  layout.back_inset_y = layout.inset;
  if (request.allow_compact_portrait_tuning &&
      uses_family(profile, LayoutFamily::COMPACT_PORTRAIT)) {
    int back_offset = scaled_px(12, layout.short_side, tokens);
    layout.back_inset_x += back_offset;
    layout.back_inset_y += back_offset;
  }
  layout.arc_stroke = scaled_px(tokens.arc_stroke, layout.short_side, tokens);
  layout.controls_gap = scaled_px(tokens.controls_gap, layout.short_side, tokens);
  layout.title_gap = scaled_px(tokens.title_gap, layout.short_side, tokens);

  layout.arc_size = layout.panel_width < layout.panel_height
    ? layout.panel_width
    : layout.panel_height;
  layout.arc_size -= layout.inset * 2;
  int reserved_bottom = layout.button_size / 3 + layout.inset;
  int available_height = layout.panel_height - layout.inset * 2;
  if (available_height > reserved_bottom) {
    int fit_height = available_height - reserved_bottom + layout.arc_stroke;
    if (layout.arc_size > fit_height) layout.arc_size = fit_height;
  }
  if (layout.arc_size < 74) layout.arc_size = 74;

  int width_percent = normalized_width_percent(request.width_compensation_percent);
  int visible_arc_width = request.width_compensation_vertical
    ? layout.arc_size
    : layout.arc_size * width_percent / 100;
  if (visible_arc_width > layout.panel_width - layout.inset * 2) {
    layout.arc_size = (layout.panel_width - layout.inset * 2) * 100 / width_percent;
    visible_arc_width = request.width_compensation_vertical
      ? layout.arc_size
      : layout.arc_size * width_percent / 100;
  }

  layout.arc_center_x = (layout.arc_size - visible_arc_width) / 2;
  layout.arc_center_y = 0;
  layout.value_center_y = layout.arc_stroke / 2;
  layout.controls_center_y = layout.arc_size / 2 - layout.button_size / 2 -
    layout.inset + scaled_px(tokens.controls_down, layout.short_side, tokens);
  return layout;
}

constexpr int control_tab_min_size(const DeviceProfile &profile) {
  if (uses_family(profile, LayoutFamily::LARGE_LANDSCAPE)) return 64;
  if (uses_family(profile, LayoutFamily::COMPACT_PORTRAIT)) return 72;
  return 48;
}

constexpr int control_tab_size(const DeviceProfile &profile,
                               const FrameLayout &frame,
                               const DesignTokens &tokens = DEFAULT_DESIGN_TOKENS) {
  if (uses_family(profile, LayoutFamily::COMPACT_PORTRAIT)) {
    int size = scaled_px(72, frame.short_side, tokens);
    if (size < control_tab_min_size(profile)) size = control_tab_min_size(profile);
    if (size > 76) size = 76;
    return size;
  }
  const bool large_landscape = uses_family(profile, LayoutFamily::LARGE_LANDSCAPE);
  int size = frame.back_size * (large_landscape ? 4 : 7) / (large_landscape ? 5 : 10);
  if (size < control_tab_min_size(profile)) size = control_tab_min_size(profile);
  const int max_size = large_landscape ? 88 : 68;
  if (size > max_size) size = max_size;
  return size;
}

constexpr int shared_tab_size(const DeviceProfile &profile,
                              const FrameLayout &frame,
                              const DesignTokens &tokens = DEFAULT_DESIGN_TOKENS) {
  if (uses_family(profile, LayoutFamily::LARGE_SQUARE))
    return scaled_px(50, frame.short_side, tokens);
  if (uses_family(profile, LayoutFamily::COMPACT_PORTRAIT))
    return scaled_px(58, frame.short_side, tokens);
  if (uses_family(profile, LayoutFamily::WIDE_LANDSCAPE))
    return scaled_px(54, frame.short_side, tokens);
  return control_tab_size(profile, frame, tokens);
}

constexpr int tab_gap(const DeviceProfile &profile, int tab_size) {
  const bool large_landscape = uses_family(profile, LayoutFamily::LARGE_LANDSCAPE);
  int gap = large_landscape ? tab_size * 2 / 5 : tab_size / 4;
  const int minimum_gap = large_landscape ? 24 : 12;
  if (gap < minimum_gap) gap = minimum_gap;
  return gap;
}

constexpr int shared_tab_content_gap(
    const DeviceProfile &profile,
    const FrameLayout &frame,
    const DesignTokens &tokens = DEFAULT_DESIGN_TOKENS) {
  if (uses_family(profile, LayoutFamily::LARGE_SQUARE))
    return scaled_px(30, frame.short_side, tokens);
  if (uses_family(profile, LayoutFamily::COMPACT_PORTRAIT))
    return scaled_px(12, frame.short_side, tokens);
  if (uses_family(profile, LayoutFamily::WIDE_LANDSCAPE))
    return scaled_px(28, frame.short_side, tokens);
  if (uses_family(profile, LayoutFamily::LARGE_LANDSCAPE)) {
    int gap = scaled_px(22, frame.short_side, tokens);
    if (gap < 28) gap = 28;
    return gap;
  }
  return 16;
}

constexpr TabLayout calculate_tabs(
    const DeviceProfile &profile,
    const FrameLayout &frame,
    const TabRequest &request,
    const DesignTokens &tokens = DEFAULT_DESIGN_TOKENS) {
  TabLayout layout;
  layout.tab_count = request.tab_count < 1 ? 1 : request.tab_count;
  layout.show_tab_bar = request.show_tab_bar && layout.tab_count > 1;
  layout.tab_size = shared_tab_size(profile, frame, tokens);
  layout.content_gap = shared_tab_content_gap(profile, frame, tokens);

  const int minimum_tab_size = control_tab_min_size(profile);
  while (true) {
    layout.selected_tab_size = layout.tab_size + layout.tab_size / 8;
    layout.frame_padding = layout.tab_size / 5;
    layout.tab_gap = tab_gap(profile, layout.tab_size);
    layout.tabs_total_width = layout.tab_size * layout.tab_count +
      layout.tab_gap * (layout.tab_count - 1);
    layout.frame_width = layout.tabs_total_width + layout.frame_padding * 2;
    layout.frame_height = layout.tab_size + layout.frame_padding * 2;
    layout.safe_left = frame.back_inset_x + frame.back_size + frame.inset / 2;
    layout.centered_left = (frame.panel_width - layout.frame_width) / 2;
    layout.row_left = layout.centered_left;

    const bool can_shrink = layout.show_tab_bar && request.avoid_back_button &&
      !uses_family(profile, LayoutFamily::COMPACT_PORTRAIT) &&
      !uses_family(profile, LayoutFamily::LARGE_SQUARE) &&
      layout.row_left < layout.safe_left && layout.tab_size > minimum_tab_size;
    if (!can_shrink) break;
    layout.tab_size--;
  }

  if (request.avoid_back_button && layout.row_left < layout.safe_left)
    layout.row_left = layout.safe_left;
  if (!layout.show_tab_bar) layout.frame_height = 0;
  return layout;
}

constexpr ContentLayout calculate_content(const FrameLayout &frame,
                                           const ContentRequest &request) {
  ContentLayout layout;
  layout.top = request.show_tab_bar
    ? frame.inset + request.tab_frame_height + request.tab_content_gap
    : request.top_without_tabs;
  if (layout.top < request.safe_top) layout.top = request.safe_top;
  layout.bottom = frame.panel_height - frame.inset;
  layout.width = frame.panel_width - frame.inset * 2;
  layout.height = layout.bottom - layout.top;
  if (request.minimum_height > 0 && layout.height < request.minimum_height) {
    layout.height = request.fallback_height > 0
      ? request.fallback_height
      : frame.panel_height / 2;
  }
  layout.center_y = layout.top + layout.height / 2 - frame.panel_height / 2;
  return layout;
}

}  // namespace espcontrol::modal
