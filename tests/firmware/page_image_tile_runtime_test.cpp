#include "page_image_tile_runtime.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>

#include "page_card_dependencies.h"
#include "page_entity_observer_set.h"
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
  PageEntityObserverSet<1> observers;
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
          camera::RefreshMode mode = camera::RefreshMode::OFF, uint64_t nonce = 1)
      : images(requests, origin, nonce), port(limits, images, tiles), renderer(limits, arena, port) {
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
        break;
      case 9:
        if (i == 8)
          navigation<9, 2>(l);
        else
          navigation<9, 6>(l);
        break;
      case 15:
        navigation<15, 6>(l);
        break;
      case 20:
        navigation<20, 6>(l);
        break;
      default:
        assert(false);
    }
  }
  std::puts(
      "image tiles: sources, revision coalescing, retry/refresh, shared leases, reconnect and 1000 "
      "allocation-free navigation cycles per profile passed");
}
