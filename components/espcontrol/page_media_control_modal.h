#pragma once

#include "page_modal_admission.h"

// Include after the legacy media panel helpers in button_grid.h.
#include <array>
#include <cmath>
#include <limits>

#include "page_legacy_modal_port.h"
#include "page_media_state_runtime.h"
#include "page_media_volume.h"

namespace espcontrol::pages {
struct PageMediaControlModalEnvironment {
  PageMediaStateRuntime* media{nullptr};
  uint32_t accent_color{DEFAULT_ACCENT_COLOR}, secondary_color{0x212121}, tertiary_color{0x424242};
  const lv_font_t *title_font{nullptr}, *label_font{nullptr}, *number_font{nullptr}, *icon_font{nullptr};
  int width_compensation_percent{100};
};
struct PageMediaControlModalFamily {
  using Context = MediaControlCtx;
  using Environment = PageMediaControlModalEnvironment;
  struct Pending {
    int volume{-1};
    uint32_t volume_until{0}, seek_ms{0}, reset_until{0};
    bool volume_active{false}, seek_active{false}, reset_active{false};
    bool seeking{false}, dragging_progress{false}, dragging_volume{false};
    size_t group_count{0};
    bool operator==(const Pending& other) const {
      return volume == other.volume && volume_until == other.volume_until && seek_ms == other.seek_ms &&
             reset_until == other.reset_until && volume_active == other.volume_active &&
             seek_active == other.seek_active && reset_active == other.reset_active &&
             seeking == other.seeking && dragging_progress == other.dragging_progress &&
             dragging_volume == other.dragging_volume && group_count == other.group_count;
    }
  };
  struct State {
    PageMediaStateRuntime* media{nullptr};
    size_t rows{0};
    std::array<uint32_t, 64> channels{};
    uint64_t graph_revision{0}, media_revision{0};
    uint32_t groups_revision{0}, discovery_revision{0}, display_second{0};
    float duration{0};
    Pending pending{};
    bool initialized{false}, cover{false}, repaint{false}, rebuild_controls{false}, rebuild_content{false},
        pulling{false};
    bool title_current{false}, artist_current{false}, has_position{false};
  };
  static constexpr ControlModalKind kind = ControlModalKind::MEDIA_CONTROL;
  static constexpr const char* tag = "media panel setup";
  static MediaControlModalLifetime& lifetime() {
    return media_control_modal_lifetime();
  }
  static bool matches(const PageStore& store, const CardDefinition& card) {
    if (store.field(card, CardField::TYPE) != "media") return false;
    const auto mode = admission::media_mode(store.field(card, CardField::SENSOR));
    return mode == "control_modal" || mode == "speaker_group" || mode == "cover_art" ||
           (mode == "now_playing" &&
            admission::option_token(store.field(card, CardField::OPTIONS), MEDIA_COVER_ART_OPTION));
  }
  static bool initialize_context(Context& ctx, const ParsedCfg& config, const Environment& environment) {
    if (!environment.media) return false;
    BtnSlot empty{};
    return create_media_control_context(empty, config, environment.accent_color, environment.secondary_color,
                                        environment.tertiary_color, environment.title_font,
                                        environment.label_font, environment.number_font,
                                        environment.icon_font, environment.width_compensation_percent,
                                        &ctx) == &ctx;
  }
  static bool initialize_state(Context&, State& state, const ParsedCfg& config,
                               const Environment& environment, size_t gesture_capacity) {
    state.media = environment.media;
    // Reserve fixed panel controls plus all four possible clickable objects
    // per speaker row before any row widgets or parsed member arrays exist.
    state.rows = gesture_capacity > 32 ? (gesture_capacity - 32) / 4 : 0;
    state.cover = media_card_mode(config.sensor) == "cover_art" ||
                  (media_card_mode(config.sensor) == "now_playing" &&
                   cfg_option_token_present(config.options, MEDIA_COVER_ART_OPTION));
    return state.media != nullptr;
  }
  static void subscribe(Context*) {}
  static Pending pending(Context& ctx) {
    return {ctx.pending_pct,
            ctx.pending_until_ms,
            ctx.seek_pending_ms,
            ctx.track_position_reset_until_ms,
            media_control_volume_pending_active(&ctx),
            media_control_seek_pending_active(&ctx),
            media_control_track_position_reset_active(&ctx),
            ctx.seek_pending,
            ctx.dragging_progress,
            ctx.dragging_volume,
            ctx.group_members.size()};
  }
  static void pull(Context& ctx, PageEntityStateRuntime& graph, ConfigurationHandle card, State& state) {
    if (!state.media || state.pulling) return;
    state.pulling = true;
    struct PullGuard {
      bool& active;
      ~PullGuard() {
        active = false;
      }
    } guard{state.pulling};
    const auto graph_revision = graph.revision(), media_revision = state.media->revision();
    const auto local = pending(ctx);
    if (state.initialized && !state.repaint && state.graph_revision == graph_revision &&
        state.media_revision == media_revision && local == state.pending)
      return;
    const size_t dependencies = graph.dependency_count(card);
    if (dependencies > state.channels.size()) {
      // Fail admission instead of accepting only a prefix of a future schema.
      ctx.available = ctx.power_available = false;
      return;
    }
    bool changed = !state.initialized;
    for (size_t i = 0; i < dependencies; ++i) {
      const auto revision = graph.dependency(card, i).revision;
      changed |= state.channels[i] != revision;
      state.channels[i] = revision;
    }
    state.graph_revision = graph_revision;
    state.media_revision = media_revision;
    std::string_view entity = ctx.entity_id;
    PageMediaRouteSnapshot route;
    if (state.cover) {
      if (!state.media->cover_route(card, route)) {
        ctx.available = ctx.power_available = false;
        return;
      }
      entity = route.entity;
    }
    const bool switched = entity != ctx.entity_id;
    const auto group_revision = state.media->group_revision(card, entity);
    PageMediaPlaybackSnapshot clock;
    state.media->playback(card, entity, clock);
    const bool title_current = state.media->current(card, entity, "media_title");
    const bool artist_current = state.media->current(card, entity, "media_artist");
    const auto second = static_cast<uint32_t>(
        std::clamp(std::isfinite(clock.seconds) ? std::floor(static_cast<double>(clock.seconds) + 0.5) : 0.0,
                   0.0, static_cast<double>(std::numeric_limits<uint32_t>::max())));
    changed |= switched || state.groups_revision != group_revision || state.display_second != second ||
               state.duration != clock.duration || state.has_position != clock.has_position ||
               state.title_current != title_current || state.artist_current != artist_current;
    if (!changed && !state.repaint && local == state.pending) return;
    if (switched) {
      state.rebuild_content = true;
      ctx.entity_id.assign(entity.data(), entity.size());
      cancel(&ctx);
      ctx.group_members.clear();
      ctx.group_volume_states.clear();
      ctx.reported_volume_known = false;
    }
    const auto read = [&](std::string_view attribute) {
      return graph.read_attribute(card, ctx.entity_id, attribute);
    };
    const auto groups = read("group_members");
    const auto discovery = graph.read_attribute(card, ctx.speaker_group_entity,
                                                media_group_discovery_attribute(ctx.speaker_group_entity));
    std::vector<std::string> group_values, helper_members;
    std::vector<MediaGroupDiscoveryItem> helper_items;
    const bool groups_changed = !state.initialized || switched || group_revision != state.groups_revision;
    const bool helper_changed = !state.initialized || discovery.revision != state.discovery_revision;
    if (groups.status == PageEntityStatus::EXHAUSTED || discovery.status == PageEntityStatus::EXHAUSTED ||
        !state.rows || !within_row_budget(groups.value, false, state.rows) ||
        !within_row_budget(discovery.value, true, state.rows)) {
      ctx.available = ctx.power_available = false;
      return;
    }
    if (groups_changed) {
      if (groups.status == PageEntityStatus::AVAILABLE)
        group_values = media_group_parse_entity_list(groups.value.data(), groups.value.size());
      if (group_values.empty()) media_group_append_unique(group_values, ctx.entity_id);
    }
    if (helper_changed && discovery.status == PageEntityStatus::AVAILABLE) {
      if (std::string_view(media_group_discovery_attribute(ctx.speaker_group_entity)) == "data") {
        helper_items = media_group_parse_discovery_items(std::string(discovery.value));
        for (const auto& item : helper_items) media_group_append_unique(helper_members, item.entity_id);
      } else {
        helper_members = media_group_parse_entity_list(discovery.value.data(), discovery.value.size());
      }
    }
    // A service response can confirm membership before its HA attribute
    // arrives. Clock, volume and metadata updates must not overwrite it with
    // the same old raw snapshot. Every live membership report remains authoritative.
    const auto& group_members = groups_changed ? group_values : ctx.group_members;
    const auto& candidates = helper_changed ? helper_members : ctx.speaker_helper_members;
    size_t rows = 1;
    for (const auto& candidate : candidates)
      if (candidate != ctx.entity_id) ++rows;
    for (const auto& member : group_members)
      if (member != ctx.entity_id &&
          std::find(candidates.begin(), candidates.end(), member) == candidates.end())
        ++rows;
    if (rows > state.rows) {
      ctx.available = ctx.power_available = false;
      return;
    }
    char title[HA_STATE_TEXT_MAX_LEN + 1], artist[HA_STATE_TEXT_MAX_LEN + 1];
    const auto metadata = [&](char* output, bool current, std::string_view attribute) {
      const auto value = read(attribute);
      auto text = current && value.status == PageEntityStatus::AVAILABLE
                      ? decode_html_entities_bounded(output, HA_STATE_TEXT_MAX_LEN + 1,
                                                     value.value.substr(0, HA_STATE_TEXT_MAX_LEN))
                      : std::string_view{};
      return text == "unknown" || text == "unavailable" ? std::string_view{} : text;
    };
    const auto source = read("");
    const auto friendly = read("friendly_name");
    const auto volume = read_media_volume(graph, card, ctx.entity_id, media_control_volume_max_pct(&ctx));
    MediaControlStateSnapshot snapshot{source.status == PageEntityStatus::AVAILABLE
                                           ? source.value.substr(0, HA_SHORT_STATE_MAX_LEN)
                                           : std::string_view("unknown"),
                                       metadata(title, title_current, "media_title"),
                                       metadata(artist, artist_current, "media_artist"),
                                       friendly.status == PageEntityStatus::AVAILABLE &&
                                               friendly.value != "unknown" && friendly.value != "unavailable"
                                           ? friendly.value.substr(0, HA_FRIENDLY_NAME_MAX_LEN)
                                           : std::string_view{},
                                       group_members,
                                       helper_changed ? helper_members : ctx.speaker_helper_members,
                                       helper_changed ? helper_items : ctx.speaker_discovery};
    snapshot.speaker_discovery_available = !snapshot.speaker_helper_members.empty();
    snapshot.discovery_changed = helper_changed && helper_items.empty() && !ctx.speaker_discovery.empty();
    snapshot.has_state = source.status == PageEntityStatus::AVAILABLE;
    snapshot.available = available(source);
    snapshot.playing = snapshot.has_state && source.value == "playing";
    snapshot.duration = clock.has_duration ? clock.duration : 0;
    snapshot.volume_known = volume.known;
    snapshot.volume_pct = volume.reported;
    snapshot.volume_control_mode = volume.mode;
    const auto features = read("supported_features");
    char number[HA_SHORT_STATE_MAX_LEN + 1];
    const auto raw = features.status == PageEntityStatus::AVAILABLE
                         ? features.value.substr(0, HA_SHORT_STATE_MAX_LEN)
                         : std::string_view{};
    if (!raw.empty()) std::memcpy(number, raw.data(), raw.size());
    number[raw.size()] = '\0';
    char* end = nullptr;
    const auto mask = std::strtoull(number, &end, 10);
    snapshot.supported_features_known = end != number;
    // Known media capabilities occupy the low 32 bits. Avoid narrowing an
    // overflowing signed parse before applying the shared capability policies.
    snapshot.supported_features = static_cast<int>(mask & 0x7fffffffU);
    const auto shuffle = read("shuffle"), repeat = read("repeat");
    snapshot.shuffle_known =
        shuffle.status == PageEntityStatus::AVAILABLE &&
        media::parse_shuffle_state(std::string(shuffle.value.substr(0, HA_SHORT_STATE_MAX_LEN)),
                                   snapshot.shuffle_enabled);
    snapshot.repeat_mode =
        repeat.status == PageEntityStatus::AVAILABLE
            ? media::parse_repeat_mode(std::string(repeat.value.substr(0, HA_SHORT_STATE_MAX_LEN)))
            : media::RepeatMode::UNKNOWN;
    snapshot.has_position = clock.has_position;
    snapshot.position_seconds = clock.seconds;
    snapshot.position_updated_ms = state.media->now_ms();
    const auto power = graph.read_attribute(card, ctx.power_entity, "");
    ctx.power_state_known = power.status == PageEntityStatus::AVAILABLE;
    ctx.power_available = available(power);
    const auto power_text = power.value.substr(0, HA_SHORT_STATE_MAX_LEN);
    ctx.power_state_text.assign(power_text.data() ? power_text.data() : "", power_text.size());
    const bool update_ui = !page_lvgl_dispatching() && !modal_callback_dispatching();
    if (!update_ui)
      state.rebuild_controls |=
          media_control_shuffle_supported(&ctx) !=
              media::shuffle_supported(snapshot.supported_features_known, snapshot.supported_features) ||
          media_control_repeat_supported(&ctx) !=
              media::repeat_supported(snapshot.supported_features_known, snapshot.supported_features);
    media_control_apply_snapshot(snapshot, &ctx, update_ui, state.repaint || switched, state.rebuild_controls,
                                 state.rebuild_content);
    if (update_ui) state.rebuild_controls = state.rebuild_content = false;
    state.repaint = !update_ui;
    state.initialized = true;
    state.groups_revision = group_revision;
    state.discovery_revision = discovery.revision;
    state.display_second = second;
    state.duration = clock.duration;
    state.has_position = clock.has_position;
    state.title_current = title_current;
    state.artist_current = artist_current;
    state.pending = pending(ctx);
  }
  static bool action_ready(Context&, State& state) {
    return !state.pulling && !state.rebuild_content;
  }
  static bool can_open(Context* ctx) {
    return media_control_can_open_modal(ctx);
  }
  static void open(Context* ctx) {
    media_control_open_modal(ctx);
  }
  static void hide() {
    media_control_destroy_modal();
  }
  static Context* active() {
    return media_control_modal_ui().active;
  }
  static lv_obj_t* overlay() {
    return media_control_modal_ui().overlay;
  }
  static std::string title(Context* ctx) {
    return ctx->label;
  }
  static void cancel(Context* ctx) {
    ctx->pending_pct = -1;
    ctx->pending_until_ms = 0;
    ctx->seek_pending = false;
    ctx->track_position_reset_until_ms = 0;
    ctx->dragging_progress = ctx->dragging_volume = false;
  }

 private:
  // A preflight upper bound, not a second discovery parser. Quoted commas and
  // escaped quotes cannot conceal extra v2 rows; malformed data still goes
  // through the existing parser after this reservation check.
  static bool within_row_budget(std::string_view raw, bool discovery, size_t maximum) {
    bool quote = false, escaped = false;
    char delimiter = 0;
    size_t count = 0, depth = 0;
    if (discovery && raw.substr(0, 3) == "v2|") {
      for (char ch : raw.substr(3)) {
        if (escaped) {
          escaped = false;
          continue;
        }
        if (quote && ch == '\\') {
          escaped = true;
          continue;
        }
        if (ch == '"') {
          quote = !quote;
          continue;
        }
        if (quote) continue;
        if (ch == '[' && ++depth == 2 && ++count > maximum) return false;
        if (ch == ']' && depth) --depth;
      }
      return true;
    }
    if (discovery) raw = raw.substr(0, raw.find('|'));
    bool token = false;
    for (char ch : raw) {
      if (escaped) {
        escaped = false;
        token = true;
        continue;
      }
      if (quote && ch == '\\') {
        escaped = true;
        continue;
      }
      if (ch == '\'' || ch == '"') {
        if (!quote) {
          quote = true;
          delimiter = ch;
        } else if (ch == delimiter)
          quote = false;
        token = true;
        continue;
      }
      if (!quote && (ch == ',' || ch == '[' || ch == ']')) {
        if (token && ++count > maximum) return false;
        token = false;
      } else if (quote || !std::isspace(static_cast<unsigned char>(ch)))
        token = true;
    }
    return !token || count < maximum;
  }
  static bool available(const PageEntityValue& value) {
    return value.status == PageEntityStatus::AVAILABLE &&
           !ha_state_unavailable_ref(esphome::StringRef(value.value.data(), value.value.size()));
  }
};
using PageMediaControlModalPort = PageLegacyModalPort<PageMediaControlModalFamily>;
using PageMediaControlModalStyle = PageLegacyModalStyle<PageMediaControlModalFamily>;
}  // namespace espcontrol::pages
