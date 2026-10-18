#pragma once

#include "page_modal_admission.h"

// Include after the option-select helpers in button_grid.h.
#include <array>

#include "page_legacy_modal_port.h"
namespace espcontrol::pages {
struct PageOptionSelectModalEnvironment {
  uint32_t accent_color{DEFAULT_ACCENT_COLOR}, secondary_color{0x212121}, tertiary_color{0x424242};
  int width_compensation_percent{100};
  const lv_font_t* value_font{nullptr};
  const lv_font_t* label_font{nullptr};
  const lv_font_t* icon_font{nullptr};
};
struct PageOptionSelectModalFamily {
  using Context = OptionSelectCtx;
  using Environment = PageOptionSelectModalEnvironment;
  struct State {
    std::array<uint32_t, 3> channels{};
    uint64_t revision{0};
    bool initialized{false}, invalidated{false}, pulling{false}, repaint{false};
  };
  static constexpr ControlModalKind kind = ControlModalKind::OPTION_SELECT;
  static constexpr const char* tag = "option select panel setup";
  static OptionSelectModalLifetime& lifetime() {
    return option_select_modal_lifetime();
  }
  static bool matches(const PageStore& store, const CardDefinition& card) {
    const auto type = store.field(card, CardField::TYPE), sensor = store.field(card, CardField::SENSOR);
    return type == "option_select" || (type == "action" && admission::option_select_action(sensor));
  }
  static bool initialize_context(Context& ctx, const ParsedCfg& config, const Environment& env) {
    BtnSlot empty{};
    if (create_option_select_context(empty, config, env.accent_color, env.secondary_color, env.tertiary_color,
                                     env.width_compensation_percent, &ctx) != &ctx)
      return false;
    ctx.value_font = env.value_font;
    ctx.label_font = env.label_font;
    ctx.icon_font = env.icon_font;
    return true;
  }
  static void subscribe(Context*) {}
  static void pull(Context& ctx, PageEntityStateRuntime& graph, ConfigurationHandle card, State& state) {
    if (state.pulling) return;
    const auto revision = graph.revision();
    if (state.initialized && state.revision == revision && !state.repaint) return;
    state.pulling = true;
    struct Guard {
      bool& flag;
      ~Guard() {
        flag = false;
      }
    } guard{state.pulling};
    std::array<PageEntityValue, 3> values = {graph.read_attribute(card, ctx.entity_id, ""),
                                             graph.read_attribute(card, ctx.entity_id, "friendly_name"),
                                             graph.read_attribute(card, ctx.entity_id, "options")};
    for (const auto& value : values)
      if (value.status == PageEntityStatus::EXHAUSTED) {
        ctx.available = false;
        state.invalidated = true;
        return;
      }
    if (values[2].value.size() > OPTION_SELECT_OPTIONS_TEXT_MAX_LEN) {
      ctx.available = false;
      state.invalidated = true;
      return;
    }
    for (size_t i = 0; i < values.size(); ++i) {
      const auto& value = values[i];
      if (state.initialized && state.channels[i] == value.revision) continue;
      state.channels[i] = value.revision;
      const auto text = value.status == PageEntityStatus::AVAILABLE ? value.value : std::string_view{};
      if (i == 0) {
        const auto raw = text.substr(0, HA_STATE_TEXT_MAX_LEN);
        const esphome::StringRef ref(raw.data(), raw.size());
        ctx.available = value.status == PageEntityStatus::AVAILABLE &&
                        !ha_entity_state_unavailable_ref(ctx.entity_id, ref);
        ctx.current_option =
            ctx.available && normalized_state_text(ref) != "unknown" ? std::string(raw) : std::string{};
        state.repaint = true;
      } else if (i == 1)
        ctx.friendly_name.assign(text.substr(0, HA_FRIENDLY_NAME_MAX_LEN));
      else {
        auto options = option_select_parse_options(std::string(text));
        if (ctx.options != options) {
          ctx.options = std::move(options);
          if (active() == &ctx) state.invalidated = true;
        }
      }
    }
    state.initialized = true;
    state.revision = revision;
    if (active() != &ctx) {
      state.repaint = false;
      return;
    }
    if (!state.invalidated && state.repaint && !page_lvgl_dispatching() && !modal_callback_dispatching()) {
      option_select_refresh_modal_rows(&ctx);
      state.repaint = false;
    }
  }
  static bool action_ready(Context&, State& state) {
    return !state.pulling && !state.invalidated;
  }
  static bool can_open(Context* ctx, State& state) {
    return !state.invalidated && option_select_can_open_modal(ctx);
  }
  static void open(Context* ctx) {
    option_select_open_modal(ctx);
  }
  static void hide() {
    option_select_hide_modal();
  }
  static Context* active() {
    return option_select_modal_ui().active;
  }
  static lv_obj_t* overlay() {
    return option_select_modal_ui().overlay;
  }
  static std::string title(Context* ctx) {
    return option_select_label(ctx);
  }
  static void cancel(Context*) {}
};
using PageOptionSelectModalPort = PageLegacyModalPort<PageOptionSelectModalFamily>;
using PageOptionSelectModalStyle = PageLegacyModalStyle<PageOptionSelectModalFamily>;
}  // namespace espcontrol::pages
