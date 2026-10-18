#include "page_image_tile_runtime.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>

#include "button_grid_subpage_parser.h"
#include "page_card_dependencies.h"
#include "page_entity_observer_set.h"
#include "page_expanded_image_source.h"
#include "panel_config_document.h"

using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
static size_t allocations = 0;
void* operator new(size_t n) {
  ++allocations;
  if (auto* p = std::malloc(n ? n : 1)) return p;
  std::abort();
}
void* operator new[](size_t n) {
  return ::operator new(n);
}
void operator delete(void* p) noexcept {
  std::free(p);
}
void operator delete[](void* p) noexcept {
  std::free(p);
}
void operator delete(void* p, size_t) noexcept {
  std::free(p);
}
void operator delete[](void* p, size_t) noexcept {
  std::free(p);
}
namespace {
uint32_t now = 100;
uint32_t clock_now() {
  return now;
}
struct Arena final : PageArenaAllocator {
  size_t live{0};
  void* allocate(size_t n) override {
    ++live;
    return std::malloc(n);
  }
  void release(void* p, size_t) override {
    --live;
    std::free(p);
  }
  ~Arena() {
    assert(!live);
  }
};
struct Subscriptions final : PageEntitySubscriptions {
  struct Entry {
    const char *entity, *attribute;
    void* owner;
    uint32_t revision, channel;
    Deliver deliver;
  };
  std::array<Entry, 128> entries{};
  size_t used{0};
  uint32_t generation{1};
  bool subscribe(const char* entity, const char* attribute, void* owner, uint32_t revision, uint32_t channel,
                 Deliver deliver) override {
    assert(used < entries.size());
    entries[used++] = {entity, attribute, owner, revision, channel, deliver};
    return true;
  }
  void release(void* owner) override {
    for (auto& e : entries)
      if (e.owner == owner) e.owner = nullptr;
  }
  uint32_t connection_generation() const override {
    return generation;
  }
  void send(const char* entity, const char* attribute, std::string_view value) {
    const auto count = allocations;
    size_t delivered = 0;
    for (size_t i = 0; i < used; ++i) {
      const auto e = entries[i];
      if (e.owner && !std::strcmp(entity, e.entity) && !std::strcmp(attribute, e.attribute)) {
        e.deliver(e.owner, e.revision, e.channel, value);
        ++delivered;
      }
    }
    if (!delivered || allocations != count)
      std::fprintf(stderr, "delivery %s/%s: %zu allocation delta %zu channels %zu\n", entity, attribute,
                   delivered, allocations - count, used);
    assert(delivered && allocations == count);
  }
};
struct Requests final : PageImageRequests {
  struct Delivery {
    void* owner{nullptr};
    Receive receive{nullptr};
    size_t slot{0};
    uint64_t request{0};
    void reply(bool success = true, bool cached = false) const {
      if (receive) receive(owner, slot, request, success, cached);
    }
  };
  std::array<Delivery, 6> jobs{};
  std::array<std::string, 6> urls{};
  std::array<PageImageOptions, 6> options{};
  size_t starts{0}, cancels{0}, releases{0};
  bool accept{true};
  std::function<void()> on_start, on_cancel;
  bool start(size_t slot, uint64_t request, std::string_view url, PageImageOptions opt, void* owner,
             Receive receive) override {
    ++starts;
    urls[slot] = url;
    options[slot] = opt;
    jobs[slot] = {owner, receive, slot, request};
    if (on_start) on_start();
    return accept;
  }
  void cancel(size_t slot) override {
    ++cancels;
    const auto old = jobs[slot];
    if (on_cancel) on_cancel();
    old.reply();
  }
  void release_surface(size_t) override {
    ++releases;
  }
};
struct Origin final : PageImageOrigin {
  Subscriptions& subscriptions;
  std::function<bool(PageBindingToken)> bound;
  bool online{true};
  explicit Origin(Subscriptions& value) : subscriptions(value) {}
  bool valid(PageBindingToken t) const override {
    return bound && bound(t);
  }
  bool connected() const override {
    return online;
  }
  uint32_t connection_generation() const override {
    return subscriptions.generation;
  }
};
struct Environment final : PageImageTileEnvironment {
  std::string base{"https://ha"};
  uint32_t generation{1};
  bool allowed{true}, shown{true}, modal{false};
  mutable size_t probes{0};
  std::string_view base_url() const override {
    return base;
  }
  uint32_t endpoint_generation() const override {
    return generation;
  }
  bool request_allowed(uint16_t w, uint16_t h) const override {
    assert(w && h);
    ++probes;
    return allowed;
  }
  bool visible(PageBindingToken) const override {
    return shown;
  }
  bool expanded(PageBindingToken) const override {
    return modal;
  }
  uint16_t maximum_source_side() const override {
    return 400;
  }
};
template <size_t Shells, size_t Images>
struct Port final : PageRendererPort {
  const DeviceUiLimits& limits;
  PageImageRuntime<Images>& images;
  PageImageTileRuntime<Images>& tiles;
  std::array<PageBindingToken, Shells> tokens{};
  std::array<size_t, Shells> slots{};
  camera::RefreshMode mode{camera::RefreshMode::OFF};
  bool fit{false};
  std::function<void(PageBindingToken)> on_bind;
  Port(const DeviceUiLimits& l, PageImageRuntime<Images>& i, PageImageTileRuntime<Images>& t)
      : limits(l), images(i), tiles(t) {
    slots.fill(Images);
  }
  PageViewGeometry geometry(const PageStore&) const override {
    return {limits.slot_count, limits.grid_columns};
  }
  bool prepare(const PageStore&, size_t n) override {
    return n == Shells;
  }
  void discard_prepared() override {}
  void activate_prepared(const PageStore&) override {}
  void retire_active() override {}
  void unbind(size_t shell, PageBindingToken token) override {
    if (slots[shell] < Images) {
      if (tiles.find(token) < Images)
        tiles.unbind(token);
      else
        images.unbind(slots[shell]);
    }
    slots[shell] = Images;
    tokens[shell] = {};
  }
  void bind(size_t shell, const PageStore& store, const PageCellLayout&, PageBackStyleView,
            PageBindingToken token) override {
    tokens[shell] = token;
    if (token.back) return;
    const auto entity = store.field(*store.card(token.card), CardField::ENTITY);
    if (entity == "media_player.primary")
      slots[shell] = images.acquire(token);
    else if (page_image_entity_kind(entity) != PageImageEntityKind::UNSUPPORTED) {
      slots[shell] = tiles.bind(token, {entity, "binary_sensor.motion", mode, 5000, fit});
      tiles.size(token, 800, 200, 0x123456);
    }
    if (on_bind) on_bind(token);
  }
  void update(size_t, PageBindingToken) override {}
  void suspend() override {
    for (size_t i = 0; i < Shells; ++i)
      if (slots[i] < Images) {
        if (tiles.find(tokens[i]) < Images)
          tiles.unbind(tokens[i]);
        else
          images.unbind(slots[i]);
        slots[i] = Images;
      }
  }
  bool state_changed() const override {
    return true;
  }
};
template <size_t Shells, size_t Images>
struct Fixture {
  std::array<uint8_t, 8192> document{};
  size_t bytes{0};
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageEntityObserverSet<2> observers;
  PageEntityStateRuntime state{dependencies, arena, subscriptions, {256, 128, 32768}, &observers};
  Requests requests;
  Origin origin{subscriptions};
  PageImageRuntime<Images> images;
  Environment environment;
  PageImageTileRuntime<Images> tiles{state, images, environment, clock_now};
  Port<Shells, Images> port;
  PageRendererRuntime<Shells> renderer;
  alignas(std::max_align_t) std::array<uint8_t, 32768> storage{};
  PageStore store;
  Fixture(const DeviceUiLimits& limits, bool peer = false,
          camera::RefreshMode mode = camera::RefreshMode::OFF, uint64_t nonce = 1,
          uint64_t first_transfer = 1)
      : images(requests, origin, nonce),
        tiles(state, images, environment, clock_now, first_transfer),
        port(limits, images, tiles),
        renderer(limits, arena, port) {
    now = 100;
    PanelConfigWriter w(document.data(), document.size());
    assert(w.begin() == PanelConfigStatus::OK);
    assert(w.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                   std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const auto button = [&](uint8_t slot, const std::string& text) {
      assert(w.append_button(slot, reinterpret_cast<const uint8_t*>(text.data()), text.size()) ==
             PanelConfigStatus::OK);
    };
    button(1, peer ? "media_player.primary;;;;cover_art;;media;;"
                   : "camera.front;Front;Auto;Auto;;;image;;image_modal_refresh_mode=activity,"
                     "image_modal_refresh_trigger=binary_sensor.motion");
    button(2, "image.still;Still;Auto;Auto;;;image;;");
    button(3, "camera.other;Other;Auto;Auto;;;image;;");
    button(4, ";Rooms;;;indicator;;subpage;;");
    const std::string child = "~B,1|image,camera.hidden,Hidden,Auto,Auto,,,,";
    assert(w.append_subpage(4, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    std::string order;
    for (size_t i = 1; i <= limits.slot_count; ++i) {
      if (i > 1) order += ",";
      order += std::to_string(i);
    }
    const std::string key = "button_order";
    assert(w.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                            reinterpret_cast<const uint8_t*>(order.data()),
                            order.size()) == PanelConfigStatus::OK);
    assert(w.finish(&bytes) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), bytes, limits, 17, storage.data(), storage.size(), &store) ==
           PageStoreStatus::OK);
    assert(observers.add(tiles));
    assert(state.prepare(store));
    assert(renderer.prepare(store));
    origin.bound = [&](PageBindingToken t) { return renderer.bound(t); };
    port.mode = mode;
    port.on_bind = [&](PageBindingToken t) { assert(renderer.bound(t) && !renderer.valid(t)); };
    renderer.activate_prepared(store);
    state.activate_prepared(store);
  }
  ~Fixture() {
    renderer.retire_active();
    tiles.retire();
    observers.remove(tiles);
  }
  PageBindingToken token(size_t shell = 0) const {
    return renderer.binding(shell);
  }
  size_t slot(size_t shell = 0) const {
    return tiles.find(token(shell));
  }
  void send(const char* attr, std::string_view value, const char* entity = "camera.front") {
    subscriptions.send(entity, attr, value);
  }
  void tick(uint32_t advance = 0) {
    now += advance;
    images.loop();
    tiles.loop();
    renderer.update_visible();
  }
  void preload(const char* entity = "camera.front") {
    send("access_token", "secret", entity);
    send("", "idle", entity);
  }
  void ready(size_t shell = 0) {
    requests.jobs[slot(shell)].reply();
    tick();
  }
};
using F = Fixture<9, 6>;
template <size_t Shells, size_t Images>
struct ExpandedObserver final : PageEntityObserver, PageModalImageOrigin {
  Fixture<Shells, Images>& f;
  PageImageRefreshState policy;
  PageImageRefreshTransfer id;
  PageModalToken token;
  const char* entity;
  PageImageEntityKind kind;
  bool active{false};
  PageModalRuntime* modal_runtime{nullptr};
  ExpandedObserver(Fixture<Shells, Images>& value, size_t shell = 0, const char* source = "camera.front")
      : f(value),
        token{f.token(shell).configuration, 1, f.token(shell).card, ControlModalKind::IMAGE_CARD},
        entity(source),
        kind(page_image_entity_kind(source)) {
    assert(f.observers.add(*this));
  }
  ~ExpandedObserver() {
    active = false;
    f.tiles.end_expanded(id, policy);
    f.observers.remove(*this);
  }
  bool valid(PageModalToken value) const override {
    return active && PageModalImageIdentity::same(value, token) &&
           (!modal_runtime || modal_runtime->valid(value));
  }
  bool connected() const override {
    return f.origin.online;
  }
  uint32_t connection_generation() const override {
    return f.subscriptions.generation;
  }
  PageImageRefreshTransfer begin() {
    active = true;
    id = f.tiles.begin_expanded(token, *this, policy);
    return id;
  }
  bool end() {
    active = false;
    return f.tiles.end_expanded(id, policy);
  }
  void observed(uint32_t generation, std::string_view source, std::string_view attribute,
                std::string_view value, bool copied) override {
    observed_update(generation, source, attribute, value, copied, {});
  }
  void observed_update(uint32_t generation, std::string_view source, std::string_view attribute,
                       std::string_view value, bool copied, std::string_view previous) override {
    if (copied || !valid(token) || generation != token.configuration) return;
    policy.synchronize(connection_generation());
    if (source == entity) {
      if (attribute.empty())
        policy.primary(kind, value, now);
      else if (attribute == "entity_picture")
        policy.picture_update(now);
      else if (attribute == "access_token")
        policy.token_update(value, previous, now);
    } else if (source == "binary_sensor.motion" && attribute.empty())
      policy.trigger(value, true, now, true);
  }
};
void refresh_handoff() {
  {
    F f(DEVICE_UI_PROFILES[8], false, camera::RefreshMode::PERIODIC);
    f.preload();
    f.tick();
    f.ready();
    f.ready();
    assert(f.requests.starts == 2);
    f.tick(2000);
    ExpandedObserver expanded(f);
    assert(expanded.begin().transfer && expanded.policy.schedule.next_due == 5100);
    const auto count = f.requests.starts;
    f.tick(4000);
    assert(f.requests.starts == count && expanded.policy.schedule.due(now, true));
    expanded.policy.schedule.started();
    f.tick(10000);
    assert(f.requests.starts == count && !expanded.policy.schedule.due(now, true));
    expanded.policy.expanded_finished(now, true);
    assert(expanded.end());
    f.tick();
    f.tick(4999);
    assert(f.requests.starts == count);
    f.tick(1);
    assert(f.requests.starts == count + 1);
  }
  {
    F f(DEVICE_UI_PROFILES[8], false, camera::RefreshMode::ACTIVITY);
    f.preload();
    f.send("", "on", "binary_sensor.motion");
    f.tick();
    f.ready();
    ExpandedObserver expanded(f);
    assert(expanded.begin().transfer && expanded.policy.activity.on(expanded.policy.connection) &&
           expanded.policy.schedule.due(now, true));
    expanded.policy.schedule.started();
    expanded.policy.expanded_finished(now, true);
    f.tick(1000);
    f.send("", "off", "binary_sensor.motion");
    f.send("", "on", "binary_sensor.motion");
    assert(expanded.policy.schedule.window_end == now + 30000);
    assert(expanded.end());
    f.tick(3999);
    assert(f.requests.starts == 1);
    f.tick(1);
    assert(f.requests.starts == 2);
    f.ready();
    f.environment.shown = false;
    f.tick();
    f.send("", "off", "binary_sensor.motion");
    f.send("", "on", "binary_sensor.motion");
    f.environment.shown = true;
    const auto count = f.requests.starts;
    f.tick(60000);
    assert(f.requests.starts == count);
  }
  {
    F f(DEVICE_UI_PROFILES[8]);
    f.preload("image.still");
    f.tick();
    f.ready(1);
    const auto slot = f.slot(1);
    const auto count = f.requests.starts;
    ExpandedObserver expanded(f, 1, "image.still");
    assert(expanded.begin().transfer);
    expanded.policy.picture.modal_requested = expanded.policy.picture.latest;
    f.send("", "revision2", "image.still");
    f.tick();
    assert(f.requests.starts == count && expanded.policy.picture.tile_dirty());
    expanded.policy.picture.modal_applied = expanded.policy.picture.modal_requested;
    assert(expanded.policy.picture.modal_dirty());
    expanded.policy.picture.modal_requested = expanded.policy.picture.latest;
    expanded.policy.picture.modal_applied = expanded.policy.picture.modal_requested;
    assert(!expanded.policy.picture.modal_dirty() && expanded.policy.picture.tile_dirty());
    f.send("", "unavailable", "image.still");
    assert(expanded.policy.unavailable);
    f.send("", "None", "image.still");
    assert(expanded.policy.unavailable);
    f.send("", "revision3", "image.still");
    assert(!expanded.policy.unavailable && expanded.policy.picture.modal_dirty());
    assert(expanded.policy.tile_invalidated);
    assert(expanded.end());
    assert(f.tiles.snapshot(f.token(1)).status == PageImageTileStatus::UNAVAILABLE);
    f.tick(999);
    assert(f.requests.starts == count);
    f.tick(1);
    assert(f.requests.starts == count + 1 && f.images.snapshot(slot, f.token(1)).pending);
    assert(f.tiles.snapshot(f.token(1)).status == PageImageTileStatus::UNAVAILABLE);
    f.ready(1);
    assert(f.tiles.snapshot(f.token(1)).status == PageImageTileStatus::READY);
  }
}
void handoff_retry_and_source_timing() {
  {
    F f(DEVICE_UI_PROFILES[8]);
    f.preload();
    f.tick();
    f.ready();
    f.tick(60000);
    ExpandedObserver expanded(f);
    assert(expanded.begin().transfer && expanded.policy.last_tile_completed == 100);
    expanded.policy.expanded_finished(now, true);
    assert(expanded.end());
    f.tick();
    assert(f.requests.starts == 1);  // No unsolicited off-mode refresh on close.
    f.send("", "idle");
    f.tick();
    assert(f.requests.starts == 2);  // The actual tile completion still owns its 30s guard.
  }
  {
    F f(DEVICE_UI_PROFILES[8]);
    f.preload();
    f.tick();
    ExpandedObserver expanded(f);
    assert(expanded.begin().transfer);
    expanded.policy.expanded_finished(now, true);
    assert(expanded.end());
    f.tick(999);
    assert(f.requests.starts == 1);
    f.tick(1);
    assert(f.requests.starts == 2);
  }
  {
    F f(DEVICE_UI_PROFILES[8]);
    f.preload();
    f.tick();
    f.ready();
    ExpandedObserver expanded(f);
    assert(expanded.begin().transfer);
    f.send("entity_picture", "https://remote/new");
    assert(expanded.end());
    f.tick(999);
    assert(f.requests.starts == 1);
    f.tick(1);
    assert(f.requests.starts == 2 && f.requests.urls[0] == "https://remote/new");
  }
  {
    F f(DEVICE_UI_PROFILES[8]);
    f.preload();
    f.tick();
    f.ready();
    ExpandedObserver expanded(f);
    assert(expanded.begin().transfer);
    expanded.policy.expanded_finished(now, false);
    assert(expanded.policy.camera_errors == 1 && expanded.policy.camera_retry_after == now + 2000);
    assert(expanded.end());
    assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::UNAVAILABLE);
    f.tick(1999);
    assert(f.requests.starts == 1);
    f.tick(1);
    assert(f.requests.starts == 2);
    f.ready();
    assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::READY);
  }
  {
    F f(DEVICE_UI_PROFILES[8]);
    f.preload();
    f.tick();
    f.requests.jobs[0].reply(false);
    f.tick();
    ExpandedObserver expanded(f);
    assert(expanded.begin().transfer);
    assert(expanded.policy.camera_errors == 1 && expanded.policy.camera_retry_after == now + 2000);
    expanded.policy.expanded_finished(now, false);
    assert(expanded.policy.camera_errors == 2 && expanded.policy.camera_retry_after == now + 4000);
    assert(expanded.end());
    f.tick(3999);
    assert(f.requests.starts == 1);
    f.tick(1);
    assert(f.requests.starts == 2);
  }
}
void handoff_late_source_delay() {
  for (const uint32_t start : {uint32_t{100}, UINT32_MAX-2000}) {
    F f(DEVICE_UI_PROFILES[8]);
    now=start; f.preload(); f.tick();
    ExpandedObserver expanded(f); assert(expanded.begin().transfer);
    now+=5000;
    expanded.policy.expanded_finished(now,true);
    now+=500;
    f.send("access_token","late-attribute");
    assert(expanded.end());
    f.tick(999); assert(f.requests.starts==1);
    f.tick(1); assert(f.requests.starts==2);
  }
}
void handoff_recovers_only_current_pixels() {
  F f(DEVICE_UI_PROFILES[8]);
  f.preload();
  f.tick();
  f.ready();
  ExpandedObserver expanded(f);
  assert(expanded.begin().transfer);
  expanded.policy.expanded_finished(now, true);
  f.send("", "unavailable");
  f.send("", "idle");
  assert(expanded.policy.tile_invalidated && !expanded.policy.expanded_success);
  expanded.policy.expanded_finished(now, true);
  assert(expanded.policy.tile_invalidated && expanded.policy.expanded_success);
  assert(expanded.end());
  assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::UNAVAILABLE);
  f.tick(999);
  assert(f.requests.starts == 1);
  f.tick(1);
  assert(f.requests.starts == 2);
  f.ready();
  assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::READY);
}
void handoff_staging_and_admission() {
  {
    F f(DEVICE_UI_PROFILES[8]);
    f.preload();
    f.tick();
    f.ready();
    ExpandedObserver expanded(f);
    assert(expanded.begin().transfer);
    const auto starts = f.requests.starts;
    assert(f.renderer.prepare(f.store) && f.state.prepare(f.store));
    f.renderer.discard_prepared();
    f.state.discard_prepared();
    assert(f.renderer.valid(f.token()) && !expanded.policy.source_dirty && expanded.policy.primary_seeded);
    assert(expanded.end());
    f.tick();
    assert(f.requests.starts == starts);
  }
  {
    F f(DEVICE_UI_PROFILES[8]);
    f.environment.allowed = false;
    f.preload();
    f.tick();
    assert(f.requests.starts == 0 && f.environment.probes == 1);
    ExpandedObserver expanded(f);
    assert(expanded.begin().transfer);
    expanded.policy.expanded_finished(now, true);
    assert(expanded.end());
    f.environment.allowed = true;
    f.tick(1999);
    assert(f.requests.starts == 0 && f.environment.probes == 1);
    f.tick(1);
    assert(f.requests.starts == 1 && f.environment.probes == 2);
  }
}
void handoff_clock_wrap() {
  {
    F f(DEVICE_UI_PROFILES[8]);
    now = UINT32_MAX - 100;
    f.preload();
    f.tick();
    f.ready();
    ExpandedObserver expanded(f);
    assert(expanded.begin().transfer);
    f.send("entity_picture", "https://remote/wrapped");
    assert(expanded.end());
    f.tick(999);
    assert(f.requests.starts == 1);
    f.tick(1);
    assert(f.requests.starts == 2);
  }
  {
    F f(DEVICE_UI_PROFILES[8]);
    now = UINT32_MAX - 100;
    f.preload();
    f.tick();
    f.ready();
    ExpandedObserver expanded(f);
    assert(expanded.begin().transfer);
    expanded.policy.expanded_finished(now, false);
    assert(expanded.end());
    f.tick(1999);
    assert(f.requests.starts == 1);
    f.tick(1);
    assert(f.requests.starts == 2);
  }
}
void handoff_cancel_and_replacement() {
  {
    F f(DEVICE_UI_PROFILES[8]);
    f.preload();
    f.tick();
    const auto old = f.requests.jobs[0];
    ExpandedObserver expanded(f);
    bool entered = false;
    f.requests.on_cancel = [&] {
      if (!entered) {
        entered = true;
        f.send("", "unavailable");
      }
    };
    assert(expanded.begin().transfer && entered && expanded.policy.unavailable);
    f.requests.on_cancel = nullptr;
    assert(expanded.end());
    f.tick();
    assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::UNAVAILABLE);
    old.reply();
    assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::UNAVAILABLE);
    f.send("", "idle");
    f.tick();
    assert(f.requests.starts == 2);
  }
  {
    F f(DEVICE_UI_PROFILES[8]);
    f.preload();
    f.tick();
    ExpandedObserver expanded(f);
    bool entered = false;
    f.requests.on_cancel = [&] {
      if (!entered) {
        entered = true;
        assert(f.renderer.suspend());
        assert(f.renderer.resume());
      }
    };
    assert(!expanded.begin().transfer && entered);
    f.requests.on_cancel = nullptr;
    f.tick();
    assert(f.tiles.find(f.token()) < 6);
  }
  {
    F f(DEVICE_UI_PROFILES[8]);
    f.preload();
    f.tick();
    ExpandedObserver expanded(f);
    alignas(std::max_align_t) std::array<uint8_t, 32768> bytes{};
    PageStore staged, published;
    assert(PageStore::stage_v1(f.document.data(), f.bytes, DEVICE_UI_PROFILES[8], UINT32_MAX, bytes.data(),
                               bytes.size(), &staged) == PageStoreStatus::OK);
    assert(f.renderer.prepare(staged) && f.state.prepare(staged));
    assert(PageStore::stage_v1(f.document.data(), f.bytes, DEVICE_UI_PROFILES[8], 0, bytes.data(),
                               bytes.size(), &published) == PageStoreStatus::OK);
    bool entered = false;
    f.requests.on_cancel = [&] {
      if (!entered) {
        entered = true;
        expanded.active = false;
        f.renderer.activate_prepared(published);
        f.state.activate_prepared(published);
        std::memset(f.storage.data(), 0xA5, f.storage.size());
      }
    };
    assert(!expanded.begin().transfer && entered && f.token().configuration == 0);
    f.requests.on_cancel = nullptr;
    f.tick();
    f.renderer.retire_active();
    f.state.retire_active();
  }
}
void actual_modal_handoff_lifetime() {
  F f(DEVICE_UI_PROFILES[8], false, camera::RefreshMode::PERIODIC);
  f.preload();
  f.tick();
  f.ready();
  f.ready();
  ExpandedObserver expanded(f);
  struct Port final : PageModalPort {
    F& f;
    ExpandedObserver<9, 6>& expanded;
    PageModalRuntime* runtime{nullptr};
    bool used{false}, shown{false}, returned{false};
    Port(F& fixture, ExpandedObserver<9, 6>& observer) : f(fixture), expanded(observer) {}
    bool valid_origin(PageBindingToken t) const override {
      return f.renderer.valid(t);
    }
    ControlModalKind kind(const PageStore& store, ConfigurationHandle card) const override {
      return store.card(card) && store.field(*store.card(card), CardField::TYPE) == "image"
                 ? ControlModalKind::IMAGE_CARD
                 : ControlModalKind::NONE;
    }
    bool ready() const override {
      return true;
    }
    bool connected() const override {
      return f.origin.online;
    }
    uint32_t connection_generation() const override {
      return f.subscriptions.generation;
    }
    void* create(const PageStore&, ConfigurationHandle, PageModalToken token) override {
      assert(!used);
      used = true;
      expanded.token = token;
      return this;
    }
    bool initialize(void*, PageModalToken token) override {
      assert(runtime->valid(token) && !runtime->interactive(token));
      shown = expanded.begin().transfer != 0;
      return shown;
    }
    bool visible(void* p) const override {
      return p == this && used && shown;
    }
    void update(void*, PageModalToken) override {}
    ControlModalDismissPolicy dismiss_policy(void*) const override {
      return ControlModalDismissPolicy::DISMISS;
    }
    void retire(void*, PageModalClose) override {
      assert(!runtime->valid(expanded.token));
      returned = expanded.end();
      shown = false;
    }
    void destroy(void* p) override {
      assert(p == this && !shown);
      used = false;
    }
  } port(f, expanded);
  PageModalRuntime modal(port);
  port.runtime = &modal;
  expanded.modal_runtime = &modal;
  assert(modal.prepare(f.store));
  modal.activate_prepared(f.store);
  assert(modal.request_card({17, 0}));
  modal.loop();
  assert(modal.interactive(modal.token()) && expanded.id.transfer);
  const auto count = f.requests.starts;
  f.tick(6000);
  assert(f.requests.starts == count);
  expanded.policy.expanded_finished(now, true);
  modal.dismiss();
  assert(port.returned);
  modal.loop();
  f.tick(4999);
  assert(f.requests.starts == count);
  f.tick(1);
  assert(f.requests.starts == count + 1);
  assert(modal.request_card({17, 0}));
  modal.loop();
  assert(expanded.id.transfer);
  ++f.subscriptions.generation;
  assert(!modal.valid(modal.token()));
  modal.loop();
  assert(port.returned && !port.used);
  f.tick();
  // A close during transfer cancellation invalidates initialization; fresh
  // tile policy must resume without a live modal or a dangling transfer.
  f.preload();
  f.tick();
  bool closed = false;
  f.requests.on_cancel = [&] {
    if (!closed) {
      closed = true;
      assert(modal.notify_closed(modal.token()));
    }
  };
  assert(modal.request_card({17, 0}));
  modal.loop();
  f.requests.on_cancel = nullptr;
  assert(closed && !port.used && !modal.token().opening);
  f.tick();
  modal.retire_active();
  modal.loop();
  expanded.modal_runtime = nullptr;
}
void handoff_expiry_and_identity() {
  F f(DEVICE_UI_PROFILES[8], false, camera::RefreshMode::ACTIVITY);
  f.preload();
  f.send("", "on", "binary_sensor.motion");
  f.tick();
  f.ready();
  ExpandedObserver expanded(f);
  const auto old = expanded.begin();
  assert(old.transfer);
  for (unsigned field = 0; field < 5; ++field) {
    auto wrong = old;
    switch (field) {
      case 0:
        ++wrong.transfer;
        break;
      case 1:
        ++wrong.modal.configuration;
        break;
      case 2:
        ++wrong.modal.opening;
        break;
      case 3:
        ++wrong.modal.card;
        break;
      case 4:
        wrong.modal.kind = ControlModalKind::CLIMATE;
        break;
    }
    assert(!f.tiles.end_expanded(wrong, expanded.policy));
  }
  assert(!f.tiles.begin_expanded(expanded.token, expanded, expanded.policy).transfer);
  expanded.active = false;
  f.tick();
  assert(!f.tiles.end_expanded(old, expanded.policy));
  f.ready();
  const auto count = f.requests.starts;
  f.tick(60000);
  assert(f.requests.starts == count);  // No replay of delegated activity.
  ++expanded.token.opening;
  const auto next = expanded.begin();
  assert(next.transfer != old.transfer && next.transfer);
  assert(!f.tiles.end_expanded(old, expanded.policy));
  assert(expanded.end());
  // A hidden source has no tile lease; the demanded owner must seed its own state.
  ++expanded.token.opening;
  expanded.token.card = f.store.page(1)->first_card;
  expanded.active = true;
  assert(!f.tiles.has_tile({expanded.token.configuration, expanded.token.card}));
  assert(!f.tiles.begin_expanded(expanded.token, expanded, expanded.policy).transfer);
}
template <size_t Shells, size_t Images>
void handoff_cycles(const DeviceUiLimits& limits) {
  Fixture<Shells, Images> f(limits);
  f.preload();
  f.tick();
  f.ready();
  ExpandedObserver expanded(f);
  const auto count = allocations, starts = f.requests.starts, channels = f.subscriptions.used;
  for (size_t n = 0; n < 1000; ++n) {
    ++expanded.token.opening;
    const auto id = expanded.begin();
    assert(id.transfer);
    assert(expanded.end());
  }
  assert(allocations == count && f.requests.starts == starts && f.subscriptions.used == channels);
}
void final_transfer_nonce() {
  F f(DEVICE_UI_PROFILES[8], false, camera::RefreshMode::OFF, 1, UINT64_MAX);
  ExpandedObserver expanded(f);
  assert(f.tiles.has_tile({expanded.token.configuration, expanded.token.card}));
  assert(expanded.begin().transfer == UINT64_MAX);
  assert(expanded.end());
  ++expanded.token.opening;
  assert(!expanded.begin().transfer);
}
template <size_t Shells, size_t Images>
struct ExpandedPipeline {
  Fixture<Shells, Images>& f;
  struct Port final : PageModalPort, PageModalImageOrigin, PageExpandedImageEnvironment {
    ExpandedPipeline& h;
    std::string entity, trigger, base{"https://ha"};
    PageModalToken opening;
    camera::RefreshMode mode{camera::RefreshMode::OFF};
    uint32_t interval{10000}, endpoint{1};
    bool used{false}, shown{false}, busy{false}, allowed{true}, force_fit{false};
    size_t probes{0}, creates{0}, destroys{0};
    explicit Port(ExpandedPipeline& value) : h(value) {}
    bool valid_origin(PageBindingToken token) const override {
      return h.f.renderer.valid(token);
    }
    ControlModalKind kind(const PageStore& store, ConfigurationHandle card) const override {
      return store.card(card) && store.field(*store.card(card), CardField::TYPE) == "image" &&
                     page_image_entity_kind(store.field(*store.card(card), CardField::ENTITY)) !=
                         PageImageEntityKind::UNSUPPORTED
                 ? ControlModalKind::IMAGE_CARD
                 : ControlModalKind::NONE;
    }
    bool ready() const override {
      return !busy;
    }
    bool connected() const override {
      return h.f.origin.online;
    }
    uint32_t connection_generation() const override {
      return h.f.subscriptions.generation;
    }
    bool valid(PageModalToken token) const override {
      return h.modal.valid(token);
    }
    bool visible(PageModalToken token) const override {
      return h.modal.interactive(token);
    }
    std::string_view base_url() const override {
      return base;
    }
    uint32_t endpoint_generation() const override {
      return endpoint;
    }
    uint16_t maximum_source_side() const override {
      return 400;
    }
    bool request_allowed(uint16_t w, uint16_t height) const override {
      assert(w && height && w <= 400 && height <= 400);
      ++const_cast<Port*>(this)->probes;
      return allowed;
    }
    void* create(const PageStore& store, ConfigurationHandle card, PageModalToken token) override {
      assert(!used);
      used = true;
      opening = token;
      ++creates;
      const auto* row = store.card(card);
      entity = store.field(*row, CardField::ENTITY);
      const std::string options(store.field(*row, CardField::OPTIONS));
      trigger = cfg_option_value(options, "image_modal_refresh_trigger");
      mode = camera::refresh_mode(cfg_option_value(options, "image_modal_refresh_mode"));
      interval = camera::refresh_interval_ms(cfg_option_value(options, "image_modal_refresh_interval"));
      return this;
    }
    bool initialize(void*, PageModalToken token) override {
      h.f.environment.modal = true;
      h.f.environment.shown = false;
      const auto count = allocations;
      const bool bound = h.source.bind(token, {entity, trigger, mode, interval, force_fit});
      assert(allocations == count);
      if (!bound) {
        h.f.environment.modal = false;
        h.f.environment.shown = true;
        return false;
      }
      h.source.size(token, 800, 600, 0x123456);
      shown = true;
      return true;
    }
    bool visible(void* p) const override {
      return p == this && used && shown;
    }
    void update(void*, PageModalToken) override {}
    ControlModalDismissPolicy dismiss_policy(void*) const override {
      return ControlModalDismissPolicy::DISMISS;
    }
    void retire(void*, PageModalClose) override {
      shown = false;
      h.source.unbind(opening);
      h.f.environment.modal = false;
      h.f.environment.shown = true;
    }
    void destroy(void* p) override {
      assert(p == this && !shown && !busy);
      used = false;
      entity.clear();
      trigger.clear();
      ++destroys;
    }
  } port;
  PageModalRuntime modal;
  Requests requests;
  PageModalImageRuntime images;
  PageExpandedImageSource<Images> source;
  explicit ExpandedPipeline(Fixture<Shells, Images>& fixture, uint64_t first_request = 1)
      : f(fixture),
        port(*this),
        modal(port),
        images(requests, port, first_request),
        source(f.state, f.tiles, images, port, port, clock_now) {
    assert(f.observers.add(source));
    assert(modal.prepare(f.store));
    modal.activate_prepared(f.store);
  }
  ~ExpandedPipeline() {
    modal.retire_active();
    modal.loop();
    source.retire();
    images.loop();
    f.observers.remove(source);
    assert(port.creates == port.destroys);
  }
  PageModalToken open(CardId card = 0) {
    assert(modal.request_card({f.token().configuration, card}));
    modal.loop();
    assert(port.used && modal.interactive(modal.token()));
    return modal.token();
  }
  void tick(uint32_t delta = 0) {
    now += delta;
    f.images.loop();
    images.loop();
    source.loop();
    modal.loop();
    f.tiles.loop();
    f.renderer.update_visible();
  }
  void ready() {
    requests.jobs[0].reply();
    tick();
  }
  void close() {
    modal.dismiss();
    modal.loop();
    images.loop();
  }
};
void expanded_source_lifecycle() {
  F f(DEVICE_UI_PROFILES[8], false, camera::RefreshMode::ACTIVITY);
  f.preload();
  f.tick();
  f.ready();
  ExpandedPipeline pipeline(f);
  const auto token = pipeline.open();
  auto snapshot = pipeline.source.snapshot(token);
  assert(snapshot.bound && snapshot.preview.binding && snapshot.asset == 0 &&
         snapshot.status == PageImageTileStatus::READY);
  pipeline.tick();
  assert(pipeline.requests.starts == 0 && pipeline.source.snapshot(token).pending);
  pipeline.tick(99);
  assert(pipeline.requests.starts == 0);
  pipeline.tick(1);
  assert(pipeline.requests.starts == 1);
  assert(pipeline.requests.urls[0] ==
         "https://ha/api/camera_proxy/camera.front?token=secret&width=400&height=300");
  const auto options = pipeline.requests.options[0];
  assert(options.width == 400 && options.height == 300 && options.priority == PageImagePriority::MODAL &&
         options.max_source_dimension == 400 && options.cover && options.retain_completed);
  const auto old = pipeline.requests.jobs[0];
  pipeline.ready();
  snapshot = pipeline.source.snapshot(token);
  assert(snapshot.asset && snapshot.status == PageImageTileStatus::READY && !snapshot.preview.binding);
  const auto content = pipeline.source.content(token);
  assert(content.token.opening == token.opening && content.asset == snapshot.asset &&
         content.entity == "camera.front" && content.width == 400 && content.height == 300 &&
         content.url.find("token=secret") != std::string_view::npos);
  const auto requests = f.requests.starts;
  pipeline.close();
  assert(!pipeline.source.snapshot(token).bound && f.requests.starts == requests);
  const auto next = pipeline.open();
  assert(next.opening != token.opening);
  old.reply();
  assert(!pipeline.images.snapshot(0, next).ready);
  pipeline.tick();
  pipeline.tick(100);
  pipeline.ready();
  // A new source URI cannot bypass an activity window. Fresh motion can.
  f.send("access_token", "rotated");
  pipeline.tick();
  assert(!pipeline.source.content(next).token.opening);
  pipeline.tick(10000);
  assert(pipeline.requests.starts == 2);
  f.send("", "off", "binary_sensor.motion");
  f.send("", "on", "binary_sensor.motion");
  pipeline.tick();
  pipeline.tick(100);
  assert(pipeline.requests.starts == 3 &&
         pipeline.requests.urls[0].find("token=rotated") != std::string::npos);
  pipeline.ready();
  pipeline.close();
}
void expanded_source_revisions_and_recovery() {
  F f(DEVICE_UI_PROFILES[8]);
  f.preload("image.still");
  f.tick();
  f.ready(1);
  ExpandedPipeline pipeline(f);
  const auto token = pipeline.open(f.token(1).card);
  pipeline.tick();
  pipeline.tick(100);
  const auto starts = pipeline.requests.starts;
  f.send("", "revision2", "image.still");
  f.send("", "revision3", "image.still");
  pipeline.tick();
  assert(pipeline.requests.starts == starts);
  pipeline.ready();
  assert(pipeline.source.snapshot(token).pending);
  pipeline.tick(100);
  assert(pipeline.requests.starts == starts + 1);
  pipeline.ready();
  f.send("", "revision3", "image.still");
  pipeline.tick(1000);
  assert(pipeline.requests.starts == starts + 1);
  f.send("", "unavailable", "image.still");
  assert(pipeline.source.snapshot(token).status == PageImageTileStatus::UNAVAILABLE);
  pipeline.tick();
  f.send("", "None", "image.still");
  pipeline.tick();
  assert(pipeline.source.snapshot(token).status == PageImageTileStatus::UNAVAILABLE);
  f.send("", "revision3", "image.still");
  assert(pipeline.source.snapshot(token).asset == 0 && !pipeline.source.snapshot(token).preview.binding);
  pipeline.tick();
  pipeline.tick(100);
  pipeline.ready();
  assert(pipeline.source.snapshot(token).asset);
  pipeline.close();
  assert(f.tiles.snapshot(f.token(1)).status == PageImageTileStatus::UNAVAILABLE);
  pipeline.tick(1000);
  f.ready(1);
  assert(f.tiles.snapshot(f.token(1)).status == PageImageTileStatus::READY);
}
void expanded_source_retry_and_geometry() {
  F f(DEVICE_UI_PROFILES[8]);
  f.preload();
  f.tick();
  f.ready();
  ExpandedPipeline pipeline(f);
  pipeline.port.force_fit = true;
  const auto token = pipeline.open();
  pipeline.port.allowed = false;
  pipeline.tick();
  pipeline.tick(100);
  assert(pipeline.requests.starts == 0 && pipeline.port.probes == 1 &&
         pipeline.source.snapshot(token).status == PageImageTileStatus::UNAVAILABLE);
  for (size_t n = 0; n < 10; ++n) {
    f.send("entity_picture", "/api/camera_proxy/camera.front");
    pipeline.tick(100);
  }
  assert(pipeline.port.probes == 1);
  pipeline.port.allowed = true;
  pipeline.tick(999);
  assert(pipeline.requests.starts == 0);
  pipeline.tick(1);
  assert(pipeline.source.snapshot(token).pending);
  pipeline.tick(100);
  assert(pipeline.requests.starts == 1);
  assert(!pipeline.requests.options[0].cover && pipeline.requests.options[0].background == 0x123456);
  pipeline.requests.jobs[0].reply(false);
  pipeline.tick();
  assert(pipeline.source.snapshot(token).status == PageImageTileStatus::UNAVAILABLE);
  pipeline.tick(3999);
  assert(pipeline.requests.starts == 1);
  pipeline.tick(1);
  pipeline.tick(100);
  assert(pipeline.requests.starts == 2);
  pipeline.ready();
  auto asset = pipeline.source.snapshot(token).asset;
  assert(asset);
  pipeline.source.size(token, 1200, 600, 0xabcdef);
  assert(pipeline.source.snapshot(token).asset == 0);
  pipeline.tick();
  pipeline.tick(100);
  assert(pipeline.requests.starts == 3 && pipeline.requests.options[0].width == 400 &&
         pipeline.requests.options[0].height == 200 && pipeline.requests.options[0].background == 0xabcdef);
  pipeline.ready();
  assert(pipeline.source.snapshot(token).asset != asset);
  ++pipeline.port.endpoint;
  assert(pipeline.source.snapshot(token).asset == 0);
  pipeline.tick();
  pipeline.tick(100);
  assert(pipeline.requests.starts == 4);
  pipeline.ready();
  ++f.subscriptions.generation;
  assert(!pipeline.source.snapshot(token).bound);
  pipeline.tick();
  assert(!pipeline.port.used);
  pipeline.close();
}
void expanded_source_preemption_and_hidden() {
  {
    F f(DEVICE_UI_PROFILES[8]);
    f.preload();
    f.preload("camera.other");
    f.tick();
    assert(f.requests.starts == 2);
    ExpandedPipeline pipeline(f);
    pipeline.open();
    assert(!f.images.snapshot(f.slot(2), f.token(2)).pending);
    pipeline.tick();
    pipeline.tick(100);
    pipeline.ready();
    pipeline.close();
    pipeline.tick(1000);
    assert(f.requests.starts >= 4);
  }
  {
    F f(DEVICE_UI_PROFILES[8]);
    ExpandedPipeline pipeline(f);
    const auto child = f.store.page(1)->first_card;
    const auto token = pipeline.open(child);
    assert(pipeline.source.snapshot(token).status == PageImageTileStatus::LOADING &&
           !pipeline.source.snapshot(token).preview.binding);
    pipeline.tick();
    pipeline.tick(44999);
    assert(pipeline.requests.starts == 0 &&
           pipeline.source.snapshot(token).status == PageImageTileStatus::LOADING);
    pipeline.tick(1);
    assert(pipeline.source.snapshot(token).status == PageImageTileStatus::UNAVAILABLE);
    f.send("access_token", "hidden-token", "camera.hidden");
    pipeline.tick();
    pipeline.tick(100);
    assert(pipeline.requests.starts == 1);
    pipeline.ready();
    assert(pipeline.source.snapshot(token).asset);
  }
}
void expanded_preemption_keeps_schedule() {
  for (const auto mode : {camera::RefreshMode::PERIODIC, camera::RefreshMode::ACTIVITY}) {
    F f(DEVICE_UI_PROFILES[8], false, mode);
    f.preload();
    f.preload("camera.other");
    f.tick();
    f.ready();
    f.ready(2);
    if (mode == camera::RefreshMode::ACTIVITY) {
      // All fake image tiles use the same declared trigger in this fixture.
      f.send("", "off", "binary_sensor.motion");
      f.send("", "on", "binary_sensor.motion");
    }
    f.tick(10000);
    assert(f.images.snapshot(f.slot(2), f.token(2)).pending);
    ExpandedPipeline pipeline(f);
    pipeline.open();
    pipeline.tick();
    pipeline.tick(100);
    pipeline.ready();
    pipeline.close();
    pipeline.tick(1000);
    assert(f.images.snapshot(f.slot(2), f.token(2)).pending);
    const auto request = f.requests.jobs[f.slot(2)].request;
    f.ready(2);
    pipeline.tick();
    assert(f.requests.jobs[f.slot(2)].request == request);
    pipeline.tick(4999);
    assert(f.requests.jobs[f.slot(2)].request == request);
    pipeline.tick(1);
    assert(f.requests.jobs[f.slot(2)].request != request);
  }
}
void expanded_source_reentrant_delivery() {
  F f(DEVICE_UI_PROFILES[8]);
  f.preload();
  f.tick();
  f.ready();
  ExpandedPipeline pipeline(f);
  auto token = pipeline.open();
  pipeline.tick();
  bool closed = false;
  pipeline.requests.on_start = [&] {
    if (!closed) {
      closed = true;
      pipeline.port.busy = true;
      assert(pipeline.modal.notify_closed(token));
      assert(pipeline.port.used && !pipeline.port.entity.empty());
      pipeline.modal.loop();
      assert(pipeline.port.used);
      pipeline.port.busy = false;
    }
  };
  pipeline.tick(100);
  pipeline.requests.on_start = nullptr;
  assert(closed && !pipeline.port.used && !pipeline.source.snapshot(token).bound);
  token = pipeline.open();
  pipeline.tick();
  pipeline.requests.on_start = [&] { pipeline.requests.jobs[0].reply(true, true); };
  pipeline.tick(100);
  pipeline.requests.on_start = nullptr;
  pipeline.tick();
  assert(pipeline.source.snapshot(token).asset);
  const auto old = pipeline.requests.jobs[0];
  pipeline.close();
  token = pipeline.open();
  pipeline.tick();
  pipeline.tick(100);
  old.reply();
  assert(pipeline.source.snapshot(token).pending);
  pipeline.ready();
}
void expanded_source_cancel_replacement() {
  F f(DEVICE_UI_PROFILES[8]);
  f.preload();
  f.tick();
  f.ready();
  ExpandedPipeline pipeline(f);
  const auto token = pipeline.open();
  pipeline.tick();
  pipeline.tick(100);
  pipeline.ready();
  pipeline.source.size(token, 600, 800, 0);
  bool closed = false;
  pipeline.requests.on_cancel = [&] {
    if (closed) return;
    closed = true;
    assert(pipeline.modal.notify_closed(token));
  };
  pipeline.tick();
  pipeline.requests.on_cancel = nullptr;
  assert(closed && !pipeline.source.snapshot(token).bound && !pipeline.port.used);
  const auto fresh = pipeline.open();
  pipeline.tick();
  pipeline.tick(100);
  pipeline.ready();
  assert(pipeline.source.snapshot(fresh).asset);
  // A source report immediately revokes cache eligibility, before maintenance.
  f.send("access_token", "new-token");
  assert(!pipeline.source.content(fresh).token.opening);
  pipeline.close();
}
void expanded_source_configuration_replacement() {
  F f(DEVICE_UI_PROFILES[8]);
  f.preload();
  f.tick();
  f.ready();
  ExpandedPipeline pipeline(f);
  const auto token = pipeline.open();
  pipeline.tick();
  alignas(std::max_align_t) std::array<uint8_t, 32768> replacement_bytes{};
  PageStore staged, published;
  assert(PageStore::stage_v1(f.document.data(), f.bytes, DEVICE_UI_PROFILES[8], UINT32_MAX,
                             replacement_bytes.data(), replacement_bytes.size(),
                             &staged) == PageStoreStatus::OK);
  assert(f.state.prepare(staged) && f.renderer.prepare(staged) && pipeline.modal.prepare(staged));
  f.state.discard_prepared();
  f.renderer.discard_prepared();
  pipeline.modal.discard_prepared();
  assert(pipeline.modal.valid(token));
  assert(f.state.prepare(staged) && f.renderer.prepare(staged) && pipeline.modal.prepare(staged));
  assert(PageStore::stage_v1(f.document.data(), f.bytes, DEVICE_UI_PROFILES[8], 0, replacement_bytes.data(),
                             replacement_bytes.size(), &published) == PageStoreStatus::OK);
  bool replaced = false;
  pipeline.requests.on_start = [&] {
    if (replaced) return;
    replaced = true;
    const auto count = allocations;
    pipeline.modal.activate_prepared(published);
    f.renderer.activate_prepared(published);
    f.state.activate_prepared(published);
    assert(count == allocations);
    std::memset(f.storage.data(), 0xa5, f.storage.size());
  };
  pipeline.tick(100);
  pipeline.requests.on_start = nullptr;
  assert(replaced && f.token().configuration == 0 && !pipeline.source.snapshot(token).bound);
  pipeline.requests.jobs[0].reply();
  assert(!pipeline.source.snapshot(token).bound);
  pipeline.modal.retire_active();
  pipeline.modal.loop();
  f.renderer.retire_active();
  f.state.retire_active();
}
void expanded_source_exhaustion() {
  F f(DEVICE_UI_PROFILES[8]);
  f.preload();
  f.tick();
  f.ready();
  ExpandedPipeline pipeline(f, UINT64_MAX);
  auto token = pipeline.open();
  pipeline.tick();
  pipeline.tick(100);
  pipeline.ready();
  assert(pipeline.source.snapshot(token).asset == UINT64_MAX);
  pipeline.close();
  token = pipeline.open();
  pipeline.tick();
  pipeline.tick(100);
  assert(pipeline.requests.starts == 1 && !pipeline.source.snapshot(token).asset &&
         pipeline.source.snapshot(token).status == PageImageTileStatus::UNAVAILABLE);
}
template <size_t Shells, size_t Images>
void expanded_source_profile(const DeviceUiLimits& limits) {
  Fixture<Shells, Images> f(limits);
  f.preload();
  f.tick();
  f.ready();
  ExpandedPipeline pipeline(f);
  const auto channels = f.subscriptions.used;
  for (size_t n = 0; n < 1000; ++n) {
    const auto token = pipeline.open();
    pipeline.tick();
    pipeline.tick(100);
    pipeline.ready();
    assert(pipeline.source.snapshot(token).asset);
    const auto before = allocations;
    pipeline.close();
    assert(allocations == before && !pipeline.source.snapshot(token).bound);
  }
  assert(f.subscriptions.used == channels);
}

void sources_and_revisions() {
  F f(DEVICE_UI_PROFILES[8]);
  const auto token = f.token();
  assert(f.slot() == 0 && f.requests.starts == 0);
  f.tick();
  assert(f.requests.starts == 0 && f.tiles.snapshot(token).status == PageImageTileStatus::LOADING);
  f.preload();
  f.tick();
  assert(f.requests.starts == 1);
  assert(f.requests.urls[0] == "https://ha/api/camera_proxy/camera.front?token=secret&width=400&height=100");
  const auto opt = f.requests.options[0];
  assert(opt.width == 800 && opt.height == 200 && opt.max_source_dimension == 400 &&
         opt.priority == PageImagePriority::TILE && opt.cover && opt.retain_completed);
  const auto initial = f.requests.jobs[0];
  f.ready();
  assert(f.tiles.snapshot(token).status == PageImageTileStatus::READY);
  f.send("", "idle");
  f.tick(100);
  assert(f.requests.starts == 1);
  f.send("entity_picture", "https://remote/image?width=123#fragment");
  f.tick();
  assert(f.requests.starts == 2 && f.requests.urls[0] == "https://remote/image?width=123#fragment");
  f.ready();
  f.send("entity_picture", "/api/camera_proxy/camera.front?height=99#fragment");
  f.tick();
  assert(f.requests.starts == 3 &&
         f.requests.urls[0] ==
             "https://ha/api/camera_proxy/camera.front?height=99&token=secret&width=400#fragment");
  f.ready();
  f.send("", "unavailable");
  f.tick();
  assert(f.tiles.snapshot(token).status == PageImageTileStatus::UNAVAILABLE &&
         f.images.snapshot(0, token).ready);
  f.send("", "idle");
  f.tick();
  assert(f.requests.starts == 4 && f.tiles.snapshot(token).status == PageImageTileStatus::UNAVAILABLE);
  initial.reply();
  assert(f.tiles.snapshot(token).status == PageImageTileStatus::UNAVAILABLE);
  f.ready();
  assert(f.tiles.snapshot(token).status == PageImageTileStatus::READY);
  // Revisions received during a transfer coalesce, and an old completion cannot acknowledge them.
  f.preload("image.still");
  f.tick();
  const auto i = f.slot(1);
  assert(i < 6);
  const auto starts = f.requests.starts;
  f.send("", "2026-10-18T12:00", "image.still");
  f.send("", "2026-10-18T12:01", "image.still");
  f.tick();
  assert(f.requests.starts == starts);
  f.requests.jobs[i].reply();
  f.tick();
  assert(f.requests.starts == starts + 1);
  f.requests.jobs[i].reply(true, true);
  f.tick();
  f.send("", "2026-10-18T12:01", "image.still");
  f.tick();
  assert(f.requests.starts == starts + 1 &&
         f.tiles.snapshot(f.token(1)).status == PageImageTileStatus::READY);
  f.send("access_token", "rotated", "image.still");
  f.tick();
  assert(f.requests.starts == starts + 1);
  f.send("", "2026-10-18T12:02", "image.still");
  f.tick();
  assert(f.requests.starts == starts + 2 && f.requests.urls[i].find("token=rotated") != std::string::npos);
}
void retry_and_admission() {
  F f(DEVICE_UI_PROFILES[8]);
  f.environment.allowed = false;
  f.preload();
  f.tick();
  assert(f.requests.starts == 0 && f.environment.probes == 1 &&
         f.tiles.snapshot(f.token()).status == PageImageTileStatus::UNAVAILABLE);
  for (size_t n = 0; n < 100; ++n) {
    f.send("", "idle");
    f.tick(10);
  }
  assert(f.environment.probes == 1);
  f.environment.allowed = true;
  f.tick(1000);
  assert(f.requests.starts == 1);
  f.requests.jobs[0].reply(false);
  f.tick();
  assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::UNAVAILABLE);
  f.tick(1999);
  assert(f.requests.starts == 1);
  f.tick(1);
  assert(f.requests.starts == 2);
  f.requests.jobs[0].reply(false);
  f.tick();
  f.tick(3999);
  assert(f.requests.starts == 2);
  f.tick(1);
  assert(f.requests.starts == 3);
  f.ready();
  f.tick(30000);
  assert(f.requests.starts == 3);
  f.send("", "idle");
  f.tick();
  assert(f.requests.starts == 4);
}
void periodic_and_activity() {
  {
    F f(DEVICE_UI_PROFILES[8], false, camera::RefreshMode::PERIODIC);
    f.preload();
    f.tick();
    f.ready();
    // Legacy initial fetch is separate from its periodic cadence.
    assert(f.requests.starts == 2);
    f.ready();
    f.tick(4999);
    assert(f.requests.starts == 2);
    f.tick(1);
    assert(f.requests.starts == 3);
    f.requests.jobs[0].reply(false);
    f.tick();
    assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::READY);
    f.tick(4999);
    assert(f.requests.starts == 3);
    f.tick(1);
    assert(f.requests.starts == 4);
    f.environment.shown = false;
    f.tick();
    const auto count = f.requests.starts;
    f.tick(60000);
    assert(f.requests.starts == count);
  }
  {
    F f(DEVICE_UI_PROFILES[8], false, camera::RefreshMode::ACTIVITY);
    f.preload();
    f.send("", "off", "binary_sensor.motion");
    f.tick();
    f.ready();
    assert(f.requests.starts == 1);
    f.send("", "on", "binary_sensor.motion");
    f.tick();
    assert(f.requests.starts == 2);
    f.ready();
    f.tick(4999);
    assert(f.requests.starts == 2);
    f.tick(1);
    assert(f.requests.starts == 3);
    f.ready();
    f.environment.modal = true;
    f.tick(5000);
    assert(f.requests.starts == 3);
    f.environment.modal = false;
    f.tick();
    assert(f.requests.starts == 4);
    f.ready();
    f.environment.shown = false;
    f.tick();
    f.send("", "off", "binary_sensor.motion");
    f.send("", "on", "binary_sensor.motion");
    f.environment.shown = true;
    f.tick(60000);
    assert(f.requests.starts == 4);
  }
}
void connection_geometry_and_reentrancy() {
  F f(DEVICE_UI_PROFILES[8]);
  f.preload();
  f.tick();
  f.ready();
  const auto old = f.requests.jobs[0];
  ++f.subscriptions.generation;
  f.send("access_token", "new");
  f.tick();
  assert(f.requests.starts == 2 && f.requests.urls[0].find("token=new") != std::string::npos);
  old.reply();
  assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::LOADING);
  f.ready();
  const auto released = f.requests.releases;
  f.tiles.size(f.token(), 200, 800);
  assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::LOADING);
  f.tick();
  assert(f.requests.starts == 3 && f.requests.releases == released + 1 &&
         f.requests.options[0].max_source_dimension == 400 &&
         f.requests.urls[0].find("width=100&height=400") != std::string::npos);
  f.ready();
  ++f.environment.generation;
  f.tick();
  assert(f.requests.starts == 4);
  f.ready();
  bool entered = false;
  f.requests.on_start = [&] {
    if (!entered) {
      entered = true;
      assert(f.renderer.suspend());
    }
  };
  f.send("access_token", "replace");
  f.tick();
  assert(entered && f.tiles.find(f.token()) == 6);
  f.requests.on_start = nullptr;
  assert(f.renderer.resume());
  f.tick();
  assert(f.requests.starts >= 6);
  // Cancellation may rebind the whole scene; preserve the replacement lease.
  entered = false;
  f.requests.on_cancel = [&] {
    if (!entered) {
      entered = true;
      assert(f.renderer.suspend());
      assert(f.renderer.resume());
    }
  };
  ++f.environment.generation;
  f.tick();
  f.requests.on_cancel = nullptr;
  assert(entered);
  f.tick();
  assert(f.tiles.find(f.token()) < 6);
}
template <size_t Shells, size_t Images>
void navigation(const DeviceUiLimits& limits) {
  Fixture<Shells, Images> f(limits, true);
  assert(f.port.slots[0] == 0 && f.slot(1) == 1);
  if (Images == 2)
    assert(f.slot(2) == Images);
  else
    assert(f.slot(2) == 2);
  const auto peer = f.token();
  const auto child = f.store.card(f.token(3).card)->target;
  assert(child != NO_PAGE);
  const auto count = allocations, channels = f.subscriptions.used;
  const auto starts = f.requests.starts;
  const auto releases = f.requests.releases;
  for (size_t n = 0; n < 1000; ++n) {
    assert(f.renderer.open_page(child));
    assert(f.tiles.find(f.token(1)) < Images);
    assert(!f.renderer.bound(peer));
    assert(f.renderer.back());
    f.renderer.update_visible();
  }
  assert(allocations == count && f.subscriptions.used == channels && f.requests.starts == starts &&
         f.requests.releases == releases);
  f.images.loop();
  assert(f.requests.releases > releases);
}
void replacement_and_synchronous_delivery() {
  F f(DEVICE_UI_PROFILES[8]);
  f.preload();
  f.tick();
  f.ready();
  const auto old_token = f.token();
  const auto old_job = f.requests.jobs[0];
  alignas(std::max_align_t) std::array<uint8_t, 32768> replacement_bytes{};
  PageStore staged, published;
  assert(PageStore::stage_v1(f.document.data(), f.bytes, DEVICE_UI_PROFILES[8], UINT32_MAX,
                             replacement_bytes.data(), replacement_bytes.size(),
                             &staged) == PageStoreStatus::OK);
  assert(f.state.prepare(staged) && f.renderer.prepare(staged));
  assert(f.renderer.valid(old_token));
  // Failed/discarded staging must not cancel a live image.
  f.state.discard_prepared();
  f.renderer.discard_prepared();
  assert(f.renderer.valid(old_token));
  assert(f.state.prepare(staged) && f.renderer.prepare(staged));
  assert(PageStore::stage_v1(f.document.data(), f.bytes, DEVICE_UI_PROFILES[8], 0, replacement_bytes.data(),
                             replacement_bytes.size(), &published) == PageStoreStatus::OK);
  bool replaced = false;
  f.requests.on_start = [&] {
    if (!replaced) {
      replaced = true;
      const auto before = allocations;
      f.renderer.activate_prepared(published);
      f.state.activate_prepared(published);
      assert(allocations == before);
    }
  };
  f.send("access_token", "replace-scene");
  f.tick();
  f.requests.on_start = nullptr;
  assert(replaced && !f.renderer.bound(old_token) && f.token().configuration == 0);
  old_job.reply();
  f.preload();
  f.tick();
  f.ready();
  assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::READY);
  f.requests.on_start = [&] { f.requests.jobs[f.slot()].reply(true, true); };
  f.send("access_token", "synchronous");
  f.tick();
  f.tick();
  assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::READY);
  f.requests.on_start = nullptr;
  f.requests.accept = false;
  f.send("access_token", "rejected");
  f.tick();
  f.tick();
  assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::UNAVAILABLE);
  // Retire borrowed metadata before the local replacement bank leaves scope.
  f.renderer.retire_active();
  f.state.retire_active();
}
void public_fallback_and_fit() {
  F f(DEVICE_UI_PROFILES[8]);
  // Rebind from already-prepared metadata; bind/size must remain allocation-free.
  const auto token = f.token();
  f.tiles.unbind(token);
  const auto count = allocations;
  assert(f.tiles.bind(token, {"camera.front", {}, camera::RefreshMode::ACTIVITY, 10000, true}) < 6);
  f.tiles.size(token, 200, 800, 0x123456);
  assert(allocations == count);
  f.environment.base.clear();
  f.send("entity_picture", "https://public/picture");
  f.tick();
  assert(f.requests.starts == 1 && !f.requests.options[0].cover &&
         f.requests.options[0].background == 0x123456 && f.requests.options[0].max_source_dimension == 800);
  f.ready();
  f.tick(60000);
  assert(f.requests.starts == 1);  // Invalid activity trigger does not poll.
  f.tiles.size(token, 200, 800, 0xabcdef);
  f.tick();
  assert(f.requests.starts == 2 && f.requests.options[0].background == 0xabcdef);
}
void startup_and_exhaustion() {
  {
    F f(DEVICE_UI_PROFILES[8]);
    f.tick(44999);
    assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::LOADING);
    f.tick(1);
    assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::UNAVAILABLE);
    f.preload();
    f.tick(2000);
    assert(f.requests.starts == 1 && f.tiles.snapshot(f.token()).status == PageImageTileStatus::LOADING);
    f.tick(60000);
    assert(f.tiles.snapshot(f.token()).status == PageImageTileStatus::LOADING);
    f.ready();
  }
  {
    F f(DEVICE_UI_PROFILES[8], false, camera::RefreshMode::OFF, UINT64_MAX);
    f.preload();
    f.tick();
    f.ready();
    f.send("access_token", "last");
    f.tick();
    assert(f.requests.starts == 1 && f.tiles.snapshot(f.token()).status == PageImageTileStatus::UNAVAILABLE);
  }
}
}  // namespace
int main() {
  expanded_source_lifecycle();
  expanded_source_revisions_and_recovery();
  expanded_source_retry_and_geometry();
  expanded_source_preemption_and_hidden();
  expanded_source_reentrant_delivery();
  expanded_preemption_keeps_schedule();
  expanded_source_cancel_replacement();
  expanded_source_configuration_replacement();
  expanded_source_exhaustion();
  refresh_handoff();
  handoff_retry_and_source_timing();
  handoff_recovers_only_current_pixels();
  handoff_late_source_delay();
  handoff_staging_and_admission();
  handoff_clock_wrap();
  handoff_cancel_and_replacement();
  actual_modal_handoff_lifetime();
  handoff_expiry_and_identity();
  final_transfer_nonce();
  sources_and_revisions();
  retry_and_admission();
  periodic_and_activity();
  connection_geometry_and_reentrancy();
  startup_and_exhaustion();
  replacement_and_synchronous_delivery();
  public_fallback_and_fit();
  for (size_t i = 0; i < std::size(DEVICE_UI_PROFILES); ++i) {
    const auto& l = DEVICE_UI_PROFILES[i];
    switch (l.slot_count) {
      case 6:
        navigation<6, 6>(l);
        handoff_cycles<6, 6>(l);
        expanded_source_profile<6, 6>(l);
        break;
      case 9:
        if (i == 8) {
          navigation<9, 2>(l);
          handoff_cycles<9, 2>(l);
          expanded_source_profile<9, 2>(l);
        } else {
          navigation<9, 6>(l);
          handoff_cycles<9, 6>(l);
          expanded_source_profile<9, 6>(l);
        }
        break;
      case 15:
        navigation<15, 6>(l);
        handoff_cycles<15, 6>(l);
        expanded_source_profile<15, 6>(l);
        break;
      case 20:
        navigation<20, 6>(l);
        handoff_cycles<20, 6>(l);
        expanded_source_profile<20, 6>(l);
        break;
      default:
        assert(false);
    }
  }
  std::puts(
      "image tiles: sources, revision coalescing, retry/refresh, shared leases, reconnect and 1000 "
      "allocation-free navigation, expanded handoff and demanded image download cycles per profile passed");
}
