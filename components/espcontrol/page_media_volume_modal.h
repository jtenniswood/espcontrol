#pragma once

// Include after the existing media UI/context helpers in button_grid.h.
#include <cctype>

#include "page_legacy_modal_port.h"
#include "page_media_volume.h"

namespace espcontrol::pages {
struct PageMediaVolumeModalEnvironment {
  uint32_t accent_color{DEFAULT_ACCENT_COLOR}, secondary_color{0x212121}, tertiary_color{0x424242};
  const lv_font_t *value_font{nullptr}, *number_font{nullptr}, *unit_font{nullptr};
  const lv_font_t *label_font{nullptr}, *icon_font{nullptr};
  int width_compensation_percent{100};
};
struct PageMediaVolumeModalFamily {
  using Context = MediaVolumeCtx;
  using Environment = PageMediaVolumeModalEnvironment;
  struct State {
    uint64_t revision{0};
    PageMediaVolumeSnapshot volume{};
    int pending_pct{-1};
    uint32_t pending_until{0};
    bool initialized{false}, available{false}, pending_active{false};
  };
  static constexpr ControlModalKind kind = ControlModalKind::MEDIA_VOLUME;
  static constexpr const char* tag = "volume panel setup";
  static MediaVolumeModalLifetime& lifetime() {
    return media_volume_modal_lifetime();
  }
  static bool matches(const PageStore& store, const CardDefinition& card) {
    return store.field(card, CardField::TYPE) == "media" &&
           card_runtime_media_mode(std::string(store.field(card, CardField::SENSOR))) == "volume";
  }
  static bool initialize_context(Context& ctx, const ParsedCfg& config, const Environment& environment) {
    return create_media_volume_context(
               nullptr, nullptr, config, environment.accent_color, environment.secondary_color,
               environment.tertiary_color, environment.value_font, environment.number_font,
               environment.unit_font, environment.label_font, environment.icon_font,
               environment.width_compensation_percent, nullptr, nullptr, &ctx) == &ctx;
  }
  // The configuration-owned graph already owns every required channel.
  static void subscribe(Context*) {}
  static void pull(Context& ctx, PageEntityStateRuntime& graph, ConfigurationHandle card, State& state) {
    const auto revision = graph.revision();
    const bool pending = media_volume_pending_active(&ctx);
    const bool pending_changed = state.pending_pct != ctx.pending_pct ||
                                 state.pending_until != ctx.pending_until_ms ||
                                 state.pending_active != pending;
    if (state.initialized && state.revision == revision && !pending_changed) return;
    const auto source = graph.read_attribute(card, ctx.entity_id, "");
    const bool available = source.status == PageEntityStatus::AVAILABLE && source_available(source.value);
    const auto volume = read_media_volume(graph, card, ctx.entity_id, media_volume_max_pct(&ctx));
    state.revision = revision;
    if (state.initialized && !pending_changed && state.available == available &&
        state.volume.known == volume.known && state.volume.reported == volume.reported &&
        state.volume.mode == volume.mode)
      return;
    state.initialized = true;
    state.available = available;
    state.volume = volume;
    media_volume_apply_snapshot(&ctx, {available, volume.known, volume.reported, volume.mode});
    state.pending_pct = ctx.pending_pct;
    state.pending_until = ctx.pending_until_ms;
    state.pending_active = media_volume_pending_active(&ctx);
  }
  static void open(Context* ctx) {
    media_volume_open_modal(ctx);
  }
  static void hide() {
    media_volume_hide_modal();
  }
  static Context* active() {
    return media_volume_modal_ui().active;
  }
  static lv_obj_t* overlay() {
    return media_volume_modal_ui().overlay;
  }
  static std::string title(Context* ctx) {
    return ctx->label;
  }
  static void cancel(Context* ctx) {
    ctx->pending_pct = -1;
    ctx->pending_until_ms = 0;
    if (active() == ctx) media_volume_modal_ui().updating_arc = false;
  }

 private:
  static bool source_available(std::string_view value) {
    value = value.substr(0, HA_SHORT_STATE_MAX_LEN);
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.remove_prefix(1);
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.remove_suffix(1);
    if (value.empty()) return false;
    const auto equals = [value](std::string_view expected) {
      if (value.size() != expected.size()) return false;
      for (size_t i = 0; i < value.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(value[i])) != expected[i]) return false;
      return true;
    };
    return !equals("unknown") && !equals("unavailable");
  }
};
using PageMediaVolumeModalPort = PageLegacyModalPort<PageMediaVolumeModalFamily>;
using PageMediaVolumeModalStyle = PageLegacyModalStyle<PageMediaVolumeModalFamily>;
}  // namespace espcontrol::pages
