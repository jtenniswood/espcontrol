#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include "button_grid_media_slider_lifecycle.h"

// Only LVGL scheduling, allocation/rendering and HA subscriptions are simulated.
// Ownership registration, binding, preservation, detach and timer callbacks below
// come from the firmware headers via the generated include.
#define CHECK(condition) do { if (!(condition)) { \
  std::fprintf(stderr, "line %d: %s\n", __LINE__, #condition); std::abort(); \
} } while (false)
struct lv_obj_t;
struct lv_event_t { lv_obj_t *target; void *user_data; };
using EventCallback = void (*)(lv_event_t *);
struct Event { unsigned id; EventCallback callback; int code; void *user_data; };
struct lv_obj_t {
  void *user_data = nullptr;
  lv_obj_t *parent = nullptr;
  std::vector<lv_obj_t *> children;
  std::vector<Event> events;
  int refreshes = 0;
};
struct lv_timer_t { unsigned id; void (*callback)(lv_timer_t *); void *user_data; };
constexpr int LV_EVENT_DELETE = 1, LV_EVENT_SIZE_CHANGED = 2, LV_PART_MAIN = 0;
static unsigned next_id = 0;
static std::vector<lv_timer_t *> timers;
static int live_widgets = 0, live_sliders = 0, live_now_playing = 0;

void *lv_obj_get_user_data(lv_obj_t *obj) { return obj->user_data; }
void lv_obj_set_user_data(lv_obj_t *obj, void *data) { obj->user_data = data; }
void *lv_event_get_user_data(lv_event_t *event) { return event->user_data; }
lv_obj_t *lv_event_get_target(lv_event_t *event) { return event->target; }
void *lv_timer_get_user_data(lv_timer_t *timer) { return timer->user_data; }
void lv_obj_add_event_cb(lv_obj_t *obj, EventCallback callback, int code, void *data) {
  obj->events.push_back({++next_id, callback, code, data});
}
void lv_obj_remove_event_cb_with_user_data(lv_obj_t *obj, EventCallback callback, void *data) {
  auto &events = obj->events;
  events.erase(std::remove_if(events.begin(), events.end(), [=](const Event &event) {
    return event.callback == callback && event.user_data == data;
  }), events.end());
}
void send_event(lv_obj_t *obj, int code) {
  const auto snapshot = obj->events;
  for (const Event &event : snapshot) {
    if (event.code != code) continue;
    if (std::none_of(obj->events.begin(), obj->events.end(), [&](const Event &current) {
          return current.id == event.id;
        })) continue;
    lv_event_t delivered{obj, event.user_data};
    event.callback(&delivered);
  }
}
lv_obj_t *create_widget(lv_obj_t *parent = nullptr) {
  auto *obj = new lv_obj_t;
  obj->parent = parent;
  if (parent) parent->children.push_back(obj);
  ++live_widgets;
  return obj;
}
void delete_widget(lv_obj_t *obj) {
  // LVGL delivers the owner's DELETE event before deleting its children.
  send_event(obj, LV_EVENT_DELETE);
  while (!obj->children.empty()) delete_widget(obj->children.back());
  if (obj->parent) {
    auto &siblings = obj->parent->children;
    siblings.erase(std::find(siblings.begin(), siblings.end(), obj));
  }
  --live_widgets;
  delete obj;
}
lv_timer_t *lv_timer_create(void (*callback)(lv_timer_t *), int, void *data) {
  auto *timer = new lv_timer_t{++next_id, callback, data};
  timers.push_back(timer);
  return timer;
}
void lv_timer_del(lv_timer_t *timer) {
  const auto it = std::find(timers.begin(), timers.end(), timer);
  CHECK(it != timers.end());
  timers.erase(it);
  delete timer;
}
void advance_timers() {
  // Snapshot IDs, not pointers: callbacks may delete or create timers.
  std::vector<unsigned> pending;
  for (const auto *timer : timers) pending.push_back(timer->id);
  for (unsigned id : pending) {
    const auto it = std::find_if(timers.begin(), timers.end(), [=](const lv_timer_t *timer) {
      return timer->id == id;
    });
    if (it != timers.end()) (*it)->callback(*it);
  }
}
struct SliderCtx {
  bool media_position = true;
  lv_obj_t *fill = nullptr, *media_slider = nullptr, *media_track_bg = nullptr;
  lv_obj_t *media_value_lbl = nullptr, *media_status_lbl = nullptr, *geometry_parent = nullptr;
  lv_timer_t *geometry_timer = nullptr, *media_timer = nullptr;
  unsigned char media_position_refresh_remaining = 0;
  SliderCtx() { ++live_sliders; }
  ~SliderCtx() { --live_sliders; }
};
struct MediaNowPlayingCtx {
  lv_obj_t *btn = nullptr, *progress_slider = nullptr, *title_lbl = nullptr, *artist_lbl = nullptr;
  std::function<void()> refresh_entity_route;
  MediaNowPlayingCtx() { ++live_now_playing; }
  ~MediaNowPlayingCtx() { --live_now_playing; }
};
static std::vector<SliderCtx *> playback_sliders;
static std::vector<MediaNowPlayingCtx *> playback_cards;
template<typename T> void detach_consumer(std::vector<T *> &items, T *item) {
  items.erase(std::remove(items.begin(), items.end(), item), items.end());
}
void media_playback_detach_slider(SliderCtx *ctx) { detach_consumer(playback_sliders, ctx); }
void media_playback_detach_now_playing(MediaNowPlayingCtx *ctx) { detach_consumer(playback_cards, ctx); }
void media_playback_detach_button(lv_obj_t *) {}
void clear_media_cover_art(MediaNowPlayingCtx *) {}
void slider_refresh_geometry(lv_obj_t *slider) { ++slider->refreshes; }
void media_apply_position(SliderCtx *ctx) { ++ctx->media_slider->refreshes; }
void media_schedule_position_refresh(SliderCtx *ctx);
void slider_bind_geometry_refresh(lv_obj_t *owner, lv_obj_t *slider);

// Minimal presentation/configuration boundary consumed by the actual driver.
namespace espcontrol::cards {
namespace card_runtime {
enum class CardDriverId { MEDIA, MEDIA_CONTROL, MEDIA_GROUP, MEDIA_PLAY_PAUSE,
  MEDIA_TRANSPORT, MEDIA_VOLUME, MEDIA_POSITION, MEDIA_NOW_PLAYING, MEDIA_COVER_ART, MEDIA_PLAYLIST };
}
enum class Surface { MAIN_GRID, SUBPAGE };
struct Context {
  Surface surface = Surface::MAIN_GRID;
  struct { card_runtime::CardDriverId driver = card_runtime::CardDriverId::MEDIA; } runtime;
};
}
using espcontrol::cards::Context;
using espcontrol::cards::Surface;
struct ParsedCfg { std::string sensor, entity = "media_player.test", label = "Player"; };
struct ConfigText { ParsedCfg state; };
struct BtnSlot {
  lv_obj_t *btn = nullptr, *sensor_container = nullptr, *text_lbl = nullptr;
  lv_obj_t *sensor_lbl = nullptr, *unit_lbl = nullptr;
  ConfigText *config = nullptr;
};
ParsedCfg parse_cfg(const ParsedCfg &config) { return config; }
Context card_runtime_context(const ParsedCfg &) { return {}; }
std::string media_card_mode(const std::string &sensor) { return sensor; }
bool media_playback_button_mode(const std::string &mode) { return mode == "play_pause"; }
bool media_control_modal_mode(const std::string &mode) { return mode == "control_modal"; }
struct lv_font_t {};
constexpr unsigned DEFAULT_SLIDER_COLOR = 0;
struct CardPalette { bool has_on = false; unsigned on_val = 0, off_val = 0, sensor_val = 0; };
enum class DisplayModalLayoutFamily { COMPACT_PORTRAIT };
struct DisplayProfile { struct { DisplayModalLayoutFamily layout_family; } modal{}; };
struct GridConfig {};
bool media_cover_art_uses_screensaver_fonts(int, int) { return false; }
bool media_cover_art_uses_compact_large_fonts(int, int) { return false; }
const lv_font_t *lv_obj_get_style_text_font(lv_obj_t *, int) { return nullptr; }
#define FONT_STUB(name) template<typename... T> const lv_font_t *name(T...) { return nullptr; }
FONT_STUB(display_sensor_font)
FONT_STUB(display_media_control_title_font)
FONT_STUB(display_media_control_artist_font)
FONT_STUB(display_media_cover_art_artist_font)
FONT_STUB(display_media_title_font)
FONT_STUB(display_media_cover_art_title_font)
int display_main_width_percent(const DisplayProfile &) { return 100; }
void setup_media_card(BtnSlot &slot, const ParsedCfg &config, unsigned, unsigned, unsigned,
                      const lv_font_t *, const lv_font_t *, const lv_font_t *, int, int, int) {
  lv_obj_t *slider = nullptr;
  if (config.sensor != "cover_art") {
    slider = create_widget(slot.btn);
    auto *ctx = new SliderCtx;
    ctx->media_slider = slider;
    ctx->fill = create_widget(slider);
    lv_obj_set_user_data(slider, ctx);
    slider_bind_geometry_refresh(slot.btn, slider);
  }
  if (config.sensor == "now_playing" || config.sensor == "cover_art") {
    auto *now = new MediaNowPlayingCtx;
    now->btn = slot.btn;
    now->progress_slider = slider;
    lv_obj_set_user_data(slot.sensor_container, now);
  } else {
    lv_obj_set_user_data(slot.sensor_container, slider);
  }
}

void subscribe_media_slider_state(lv_obj_t *, lv_obj_t *slider, const std::string &) {
  auto *ctx = static_cast<SliderCtx *>(lv_obj_get_user_data(slider));
  CHECK(ctx != nullptr);
  if (std::find(playback_sliders.begin(), playback_sliders.end(), ctx) == playback_sliders.end())
    playback_sliders.push_back(ctx);
  media_schedule_position_refresh(ctx);
}
void subscribe_media_now_playing_state(MediaNowPlayingCtx *ctx, const std::string &entity) {
  if (std::find(playback_cards.begin(), playback_cards.end(), ctx) == playback_cards.end())
    playback_cards.push_back(ctx);
  if (ctx->progress_slider) subscribe_media_slider_state(ctx->btn, ctx->progress_slider, entity);
}

// Unexercised modal/playlist/volume/rendering paths fail if entered accidentally.
struct MediaControlCtx { bool highlight_playing = true; };
struct MediaVolumeCtx {};
struct MediaPlaylistCtx {};
struct MediaDriverEnvironment {
  const GridConfig *grid_config = nullptr;
  unsigned accent_color = 0, secondary_color = 0, tertiary_color = 0;
  const lv_font_t *sensor_font = nullptr, *volume_number_font = nullptr, *volume_unit_font = nullptr;
  const lv_font_t *volume_label_font = nullptr, *icon_font = nullptr;
  int volume_width_compensation_percent = 100;
};
#define UNUSED_STUB(name) template<typename... T> void name(T...) { CHECK(false); }
UNUSED_STUB(subscribe_media_playlist_state)
UNUSED_STUB(subscribe_media_state)
UNUSED_STUB(subscribe_media_control_state)
UNUSED_STUB(subscribe_media_volume_state)
UNUSED_STUB(subscribe_friendly_name)
UNUSED_STUB(setup_media_cover_art)
UNUSED_STUB(subscribe_media_cover_art_source_state)
UNUSED_STUB(media_driver_bind_cover_art_route)
UNUSED_STUB(media_playback_detach_control)
UNUSED_STUB(media_playback_detach_volume)
UNUSED_STUB(media_playback_detach_playlist)
void grid_delete_media_control_runtime_ptr(void *) { CHECK(false); }
void grid_delete_media_volume_runtime_ptr(void *) { CHECK(false); }
void grid_delete_media_playlist_runtime_ptr(void *) { CHECK(false); }
bool media_play_pause_show_state(const ParsedCfg &) { CHECK(false); return false; }
std::string media_cover_art_secondary_entity(const ParsedCfg &) { CHECK(false); return {}; }
void subscribe_media_cover_art(MediaNowPlayingCtx *, const std::string &) {}
template<typename... T> MediaPlaylistCtx *create_media_playlist_context(T...) { CHECK(false); return nullptr; }
template<typename... T> MediaVolumeCtx *create_media_volume_context(T...) { CHECK(false); return nullptr; }
template<typename... T> MediaPlaylistCtx *media_driver_track_playlist(T...) { CHECK(false); return nullptr; }
template<typename... T> MediaVolumeCtx *media_driver_track_volume(T...) { CHECK(false); return nullptr; }
template<typename... T> MediaControlCtx *media_driver_create_control(T...) { CHECK(false); return nullptr; }
struct ControlUi { MediaControlCtx *active = nullptr; };
struct VolumeUi { MediaVolumeCtx *active = nullptr; };
ControlUi &media_control_modal_ui() { static ControlUi ui; return ui; }
VolumeUi &media_volume_modal_ui() { static VolumeUi ui; return ui; }
void media_control_hide_modal() { CHECK(false); }
void media_volume_hide_modal() { CHECK(false); }
struct GridRuntimeAllocation { lv_obj_t *owner; void *ptr; void (*deleter)(void *); };

#include "media_lifecycle_production.h"

struct Card {
  ConfigText config;
  BtnSlot slot;
  Context context;
  Card(const std::string &mode, Surface surface) {
    config.state.sensor = mode;
    context.surface = surface;
    slot.config = &config;
    slot.btn = create_widget();
    slot.sensor_container = create_widget(slot.btn);
    setup();
  }
  void setup() {
    CHECK(espcontrol::cards::media_driver_setup_visual(
      slot, config.state, context, CardPalette{}, DisplayProfile{}));
  }
  SliderCtx *slider() {
    auto *widget = config.state.sensor == "now_playing" || config.state.sensor == "cover_art"
      ? static_cast<MediaNowPlayingCtx *>(lv_obj_get_user_data(slot.sensor_container))->progress_slider
      : static_cast<lv_obj_t *>(lv_obj_get_user_data(slot.sensor_container));
    return static_cast<SliderCtx *>(lv_obj_get_user_data(widget));
  }
  void bind() {
    if (context.surface == Surface::MAIN_GRID) grid_release_main_runtime_allocations(&slot, 1);
    CHECK(espcontrol::cards::media_driver_bind_data(slot, config.state, context, MediaDriverEnvironment{}));
  }
  void rebuild() {
    CHECK(context.surface == Surface::MAIN_GRID);
    CHECK(espcontrol::cards::media_driver_cleanup(slot, config.state, context));
    while (!slot.btn->children.empty()) delete_widget(slot.btn->children.back());
    // Fire delayed work after freeing the visuals, before Phase 2 releases contexts.
    advance_timers();
    send_event(slot.btn, LV_EVENT_SIZE_CHANGED);
    slot.sensor_container = create_widget(slot.btn);
    setup();
  }
  ~Card() {
    if (context.surface == Surface::MAIN_GRID) {
      grid_prepare_media_runtime_for_visual_reset(slot.btn);
      grid_release_runtime_allocations(slot.btn);
    }
    delete_widget(slot.btn);
  }
};
void check_retired() {
  // Dispatch before checking queues so stale callbacks dereference freed objects
  // under ASan. Also check counts explicitly on hosts without the sanitizer.
  advance_timers();
  CHECK(timers.empty());
  CHECK(playback_sliders.empty());
  CHECK(playback_cards.empty());
  CHECK(grid_runtime_allocations().empty());
  CHECK(live_sliders == 0 && live_now_playing == 0 && live_widgets == 0);
}
void test_rebuild_before_binding(const std::string &mode) {
  {
    Card card(mode, Surface::MAIN_GRID);
    CHECK(!grid_runtime_allocations().empty());
    media_schedule_position_refresh(card.slider()); // Layout refresh before HA exists.
    card.rebuild();
    card.bind();
    CHECK(live_sliders == 1);
    CHECK(live_now_playing == (mode == "now_playing" ? 1 : 0));
    advance_timers();
    CHECK(card.slider()->media_slider->refreshes > 0);
  }
  check_retired();
}
void test_repeated_binding(const std::string &mode, Surface surface) {
  {
    Card card(mode, surface);
    card.bind();
    SliderCtx *original = card.slider();
    lv_timer_t *pending = original->media_timer;
    const size_t owners = grid_runtime_allocations().size();
    const size_t callbacks = card.slot.btn->events.size();
    for (int i = 0; i < 5; ++i) {
      card.bind();
      CHECK(card.slider() == original);
      CHECK(original->media_timer == pending);
      CHECK(grid_runtime_allocations().size() == owners);
      CHECK(card.slot.btn->events.size() == callbacks);
    }
    advance_timers();
    CHECK(original->media_slider->refreshes > 0);
    CHECK(playback_sliders.size() == 1);
  }
  check_retired();
}
void test_subpage_delete_before_binding(const std::string &mode) {
  {
    Card card(mode, Surface::SUBPAGE);
    if (mode != "cover_art") media_schedule_position_refresh(card.slider());
    CHECK(timers.size() == (mode == "cover_art" ? 0 : 2));
  }
  check_retired();
}
void test_bound_rebuild(const std::string &mode) {
  {
    Card card(mode, Surface::MAIN_GRID);
    for (int i = 0; i < 5; ++i) {
      card.bind();
      card.rebuild();
      CHECK(playback_sliders.empty());
      CHECK(playback_cards.empty());
    }
    card.bind();
    CHECK(live_sliders == 1);
    advance_timers();
  }
  check_retired();
}
void test_slider_deleted_before_owner(const std::string &mode, Surface surface) {
  {
    Card card(mode, surface);
    card.bind();
    SliderCtx *ctx = card.slider();
    delete_widget(ctx->media_slider);
    CHECK(ctx->media_slider == nullptr);
    CHECK(ctx->geometry_timer == nullptr && ctx->media_timer == nullptr);
    advance_timers();
    send_event(card.slot.btn, LV_EVENT_SIZE_CHANGED);
  }
  check_retired();
}
int main() {
  for (const std::string mode : {"position", "now_playing"}) {
    test_rebuild_before_binding(mode);
    test_repeated_binding(mode, Surface::MAIN_GRID);
    test_repeated_binding(mode, Surface::SUBPAGE);
    test_subpage_delete_before_binding(mode);
    test_bound_rebuild(mode);
    test_slider_deleted_before_owner(mode, Surface::MAIN_GRID);
    test_slider_deleted_before_owner(mode, Surface::SUBPAGE);
  }
  // Cover-art ownership before binding shares the Now Playing context path.
  test_subpage_delete_before_binding("cover_art");
  return EXIT_SUCCESS;
}
