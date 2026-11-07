#pragma once

// Include after the existing fan helpers and PageFanControlModalEnvironment.
#include "page_fan_control_modal.h"

namespace espcontrol::pages {
struct PageFanPresetModalFamily {
  using Context = FanCardCtx;
  using Environment = PageFanControlModalEnvironment;
  using State = PageFanModalState;
  static constexpr ControlModalKind kind = ControlModalKind::FAN_PRESET;
  static constexpr const char* tag = "fan preset panel setup";
  static FanControlModalLifetime& lifetime() {
    return fan_preset_modal_lifetime();
  }
  static bool matches(const PageStore& store, const CardDefinition& card) {
    return store.field(card, CardField::TYPE) == "fan_preset";
  }
  static bool initialize_context(Context& ctx, const ParsedCfg& config, const Environment& environment) {
    return PageFanControlModalFamily::initialize_context(ctx, config, environment);
  }
  static void subscribe(Context*) {}
  static void pull(Context& ctx, PageEntityStateRuntime& graph, ConfigurationHandle card, State& state) {
    pull_fan_modal_context(ctx, graph, card, state, active, fan_preset_rebuild_list);
  }
  static bool action_ready(Context&, State& state) {
    return !state.pulling && !state.rebuild;
  }
  static bool can_open(Context* ctx) {
    return fan_modal_context_current(ctx) && fan_control_supported(ctx);
  }
  static void open(Context* ctx) {
    fan_preset_open(ctx);
  }
  static void hide() {
    fan_preset_close();
  }
  static Context* active() {
    return fan_preset_ui().active;
  }
  static lv_obj_t* overlay() {
    return fan_preset_ui().overlay;
  }
  static std::string title(Context*) {
    return espcontrol_i18n(std::string("Preset"));
  }
  static void cancel(Context*) {}
};
using PageFanPresetModalPort = PageLegacyModalPort<PageFanPresetModalFamily>;
using PageFanPresetModalStyle = PageLegacyModalStyle<PageFanPresetModalFamily>;
}  // namespace espcontrol::pages
