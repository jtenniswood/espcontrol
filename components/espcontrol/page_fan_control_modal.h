#pragma once

// Include after the existing fan UI helpers in button_grid.h.
#include <array>

#include "page_fan_modal_state.h"
#include "page_legacy_modal_port.h"

namespace espcontrol::pages {
struct PageFanControlModalEnvironment {
  uint32_t accent_color{DEFAULT_ACCENT_COLOR}, secondary_color{0x212121}, tertiary_color{0x424242};
  const lv_font_t *label_font{nullptr}, *icon_font{nullptr};
  int width_compensation_percent{100};
};
struct PageFanControlModalFamily {
  using Context = FanCardCtx;
  using Environment = PageFanControlModalEnvironment;
  struct State : PageFanModalState {
    FanControlVisibleTabs configured;
  };
  static constexpr ControlModalKind kind = ControlModalKind::FAN_CONTROL;
  static constexpr const char* tag = "fan panel setup";
  static FanControlModalLifetime& lifetime() {
    return fan_control_modal_lifetime();
  }
  static bool matches(const PageStore& store, const CardDefinition& card) {
    return store.field(card, CardField::TYPE) == "fan_control";
  }
  static bool initialize_context(Context& ctx, const ParsedCfg& config, const Environment& environment) {
    BtnSlot empty{};
    return create_fan_card_context(empty, config, environment.accent_color, environment.secondary_color,
                                   environment.tertiary_color, environment.label_font, environment.icon_font,
                                   environment.width_compensation_percent, &ctx) == &ctx;
  }
  static bool initialize_state(Context& ctx, State& state, const ParsedCfg&, const Environment&, size_t) {
    auto value = cfg_option_value(ctx.options, FAN_CONTROL_TABS_OPTION);
    if (value.empty()) {
      value = FAN_CONTROL_DEFAULT_TABS_VALUE;
      if (!ctx.light_entity_id.empty()) value += "|light";
    }
    for (size_t start = 0; start <= value.size();) {
      const auto end = value.find('|', start);
      FanControlTab tab;
      if (fan_control_tab_from_token(
              value.substr(start, end == std::string::npos ? std::string::npos : end - start), tab))
        state.configured.add(tab);
      if (end == std::string::npos) break;
      start = end + 1;
    }
    return true;
  }
  // Configuration owns every channel. Opening adds no HA callback owner.
  static void subscribe(Context*) {}
  static void pull(Context& ctx, PageEntityStateRuntime& graph, ConfigurationHandle card, State& state) {
    pull_fan_modal_context(ctx, graph, card, state, active, fan_control_refresh_modal);
  }

  static bool action_ready(Context&, State& state) {
    return !state.pulling && !state.rebuild;
  }
  static bool can_open(Context* ctx, State& state) {
    // A usable primary always has the legacy Power fallback. Offline primary
    // admission requires a configured, usable Light tab. No per-frame parsing.
    return fan_modal_context_current(ctx) &&
           (ctx->available || (fan_light_supported(ctx) && state.configured.contains(FanControlTab::LIGHT)));
  }
  static void open(Context* ctx) {
    fan_control_open_modal(ctx);
  }
  static void hide() {
    fan_control_hide_modal();
  }
  static Context* active() {
    return fan_control_modal_ui().active;
  }
  static lv_obj_t* overlay() {
    return fan_control_modal_ui().overlay;
  }
  static std::string title(Context* ctx) {
    return fan_control_card_title(ctx);
  }
  static void cancel(Context* ctx) {
    ctx->dragging_speed = ctx->updating_speed = false;
  }
};
using PageFanControlModalPort = PageLegacyModalPort<PageFanControlModalFamily>;
using PageFanControlModalStyle = PageLegacyModalStyle<PageFanControlModalFamily>;
}  // namespace espcontrol::pages
