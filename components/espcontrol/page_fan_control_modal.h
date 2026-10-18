#pragma once

// Include after the existing fan UI helpers in button_grid.h.
#include <array>

#include "bounded_percent_state.h"
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
  struct State {
    std::array<uint32_t, 10> channels{};
    FanControlVisibleTabs configured;
    uint64_t revision{0};
    bool initialized{false}, repaint{false}, rebuild{false}, pulling{false};
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
    if (state.pulling) return;
    if (graph.dependency_count(card) > state.channels.size()) {
      ctx.available = ctx.light_available = false;
      return;
    }
    const auto revision = graph.revision();
    if (state.initialized && state.revision == revision && !state.repaint) return;
    state.pulling = true;
    struct PullGuard {
      bool& flag;
      ~PullGuard() {
        flag = false;
      }
    } guard{state.pulling};
    constexpr std::array<std::string_view, 9> attributes = {
        "",          "friendly_name", "supported_features", "percentage", "percentage_step", "oscillating",
        "direction", "preset_mode",   "preset_modes"};
    std::array<PageEntityValue, 10> values{};
    for (size_t i = 0; i < attributes.size(); ++i)
      values[i] = graph.read_attribute(card, ctx.entity_id, attributes[i]);
    if (!ctx.light_entity_id.empty()) values[9] = graph.read_attribute(card, ctx.light_entity_id, "");
    // Never expose a partial options list from an exhausted/oversized channel.
    for (const auto& value : values)
      if (value.status == PageEntityStatus::EXHAUSTED) {
        ctx.available = ctx.light_available = false;
        return;
      }
    if (values[8].value.size() > HA_TEXT_SENSOR_STATE_MAX_LEN) {
      ctx.available = ctx.light_available = false;
      return;
    }
    bool changed = false;
    const bool power = fan_power_supported(&ctx), speed = fan_speed_supported(&ctx),
               oscillation = fan_oscillation_supported(&ctx), direction = fan_direction_supported(&ctx),
               preset = fan_preset_supported(&ctx), light = fan_light_supported(&ctx);
    for (size_t i = 0; i < values.size(); ++i) {
      const auto& value = values[i];
      if (state.initialized && state.channels[i] == value.revision) continue;
      changed = true;
      state.channels[i] = value.revision;
      const auto text = value.status == PageEntityStatus::AVAILABLE ? value.value : std::string_view{};
      const auto short_text = text.substr(0, HA_SHORT_STATE_MAX_LEN);
      switch (i) {
        case 0:
          ctx.state = fan_lower(fan_trim(std::string(short_text)));
          ctx.available = !ctx.state.empty() && ctx.state != "unknown" && ctx.state != "unavailable";
          ctx.on = ctx.available && is_entity_on_ref(esphome::StringRef(ctx.state.c_str(), ctx.state.size()));
          break;
        case 1:
          ctx.friendly_name.assign(text.substr(0, HA_FRIENDLY_NAME_MAX_LEN));
          break;
        case 2: {
          ctx.supported_features_known =
              !short_text.empty() && short_text != "unknown" && short_text != "unavailable";
          if (ctx.supported_features_known) {
            char input[HA_SHORT_STATE_MAX_LEN + 1];
            std::memcpy(input, short_text.data(), short_text.size());
            input[short_text.size()] = '\0';
            // Only known positive capability bits are consumed by the fan UI.
            ctx.supported_features = static_cast<int>(std::strtoull(input, nullptr, 10) & 0x7fffffffU);
          }
          break;
        }
        case 3: {
          uint8_t percent;
          if (parse_bounded_percent(short_text, percent)) {
            ctx.percentage = percent;
            ctx.percentage_known = true;
          }
          break;
        }
        case 4: {
          uint8_t percent;
          if (parse_bounded_percent(short_text, percent) && percent > 0) {
            ctx.percentage_step = percent;
            ctx.percentage_step_known = true;
          }
          break;
        }
        case 5: {
          bool parsed = false;
          ctx.oscillation_known = fan_parse_bool_value(std::string(short_text), parsed);
          if (ctx.oscillation_known) ctx.oscillating = parsed;
          break;
        }
        case 6:
          ctx.direction = fan_lower(fan_trim(std::string(short_text)));
          ctx.direction_known = ctx.direction == "forward" || ctx.direction == "reverse";
          break;
        case 7:
          ctx.preset_mode = fan_lower(fan_trim(std::string(short_text)));
          break;
        case 8: {
          const std::string raw(text);
          ctx.preset_modes = fan_parse_options(esphome::StringRef(raw.c_str(), raw.size()));
          state.rebuild = true;
          break;
        }
        case 9: {
          const auto normalized = fan_lower(fan_trim(std::string(short_text)));
          ctx.light_available = !normalized.empty() && normalized != "unknown" && normalized != "unavailable";
          ctx.light_on = ctx.light_available &&
                         is_entity_on_ref(esphome::StringRef(normalized.c_str(), normalized.size()));
          break;
        }
      }
    }
    // Channel revisions preserve optimistic input until that particular report
    // changes. An unrelated attribute cannot replay the previous power/speed.
    state.revision = revision;
    state.initialized = true;
    state.rebuild |= power != fan_power_supported(&ctx) || speed != fan_speed_supported(&ctx) ||
                     oscillation != fan_oscillation_supported(&ctx) ||
                     direction != fan_direction_supported(&ctx) || preset != fan_preset_supported(&ctx) ||
                     light != fan_light_supported(&ctx);
    state.repaint |= changed;
    if (active() != &ctx) {
      state.repaint = state.rebuild = false;
      return;
    }
    if (state.repaint && !page_lvgl_dispatching() && !modal_callback_dispatching()) {
      fan_control_refresh_modal(&ctx);
      state.repaint = state.rebuild = false;
    }
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
