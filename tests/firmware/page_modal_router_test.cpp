#include "page_modal_router.h"

#include <array>
#include <cassert>
#include <cstring>
#include <functional>
#include <string>

#include "panel_config_document.h"

using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
namespace {
struct Fixture {
  Fixture(const DeviceUiLimits& limits, uint32_t generation) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string text = "climate.room;Room;Auto;Auto;;;climate;;";
    assert(writer.append_button(1, reinterpret_cast<const uint8_t*>(text.data()), text.size()) ==
           PanelConfigStatus::OK);
    size_t size = 0;
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, generation, arena.data(), arena.size(),
                               &store) == PageStoreStatus::OK);
  }
  std::array<uint8_t, 8192> document{};
  alignas(std::max_align_t) std::array<uint8_t, 32768> arena{};
  PageStore store;
};
struct Origin {
  uint64_t binding{1};
  static bool valid(void* owner, PageBindingToken token) {
    return token.binding == static_cast<Origin*>(owner)->binding;
  }
};
struct Family : PageModalPort {
  ControlModalKind normal{ControlModalKind::NONE}, allowed{ControlModalKind::NONE};
  bool online{true}, local{false}, busy{false}, retired{false}, live{false}, fail_create{false},
      fail_initialize{false};
  uint32_t epoch{1};
  size_t creates{0}, destroys{0}, initializes{0}, updates{0};
  PageModalToken token;
  PageModalClose reason{PageModalClose::SHUTDOWN};
  std::function<void()> on_create, on_initialize, on_destroy;
  bool valid_origin(PageBindingToken) const override { return false; }  // Router uses the scene origin.
  ControlModalKind kind(const PageStore&, ConfigurationHandle) const override { return normal; }
  ControlModalKind kind(const PageStore&, ConfigurationHandle, ControlModalKind requested) const override {
    return requested == allowed                  ? allowed
           : requested == ControlModalKind::NONE ? normal
                                                 : ControlModalKind::NONE;
  }
  ControlModalKind local_kind(ControlModalKind requested) const override {
    return local && requested == allowed ? allowed : ControlModalKind::NONE;
  }
  bool valid_local_origin(PageLocalModalOrigin origin, ControlModalKind kind) const override {
    return local && origin.binding == 7 && kind == allowed;
  }
  bool ready() const override { return !busy; }
  bool connected() const override { return local || online; }
  uint32_t connection_generation() const override { return local ? 0 : epoch; }
  void* create(const PageStore&, ConfigurationHandle, PageModalToken opening) override {
    assert(!live && !busy);
    if (fail_create) return nullptr;
    token = opening;
    live = true;
    retired = false;
    ++creates;
    if (on_create) on_create();
    return this;
  }
  void* create_local(const PageStore& store, PageLocalModalOrigin origin, PageModalToken opening) override {
    assert(local && origin.binding == 7 && opening.card == NO_CARD);
    return create(store, {}, opening);
  }
  bool initialize(void* context, PageModalToken opening) override {
    assert(context == this && live && !retired && opening.opening == token.opening);
    ++initializes;
    if (on_initialize) on_initialize();
    return !fail_initialize;
  }
  bool visible(void* context) const override { return context == this && live && !retired; }
  void update(void* context, PageModalToken opening) override {
    assert(context == this && !retired && opening.opening == token.opening);
    ++updates;
  }
  ControlModalDismissPolicy dismiss_policy(void*) const override {
    return allowed == ControlModalKind::ALARM_CONTROL
               ? ControlModalDismissPolicy::PRESERVE_DURING_DISPLAY_TAKEOVER
               : ControlModalDismissPolicy::DISMISS;
  }
  uint64_t source_revision(void*) const override { return 19; }
  void retire(void* context, PageModalClose why) override {
    assert(context == this && live && !retired);
    retired = true;
    reason = why;
  }
  void destroy(void* context) override {
    assert(context == this && live && retired && !busy);
    if (on_destroy) on_destroy();
    live = false;
    ++destroys;
  }
};
constexpr std::array kinds{
    ControlModalKind::MEDIA_VOLUME,   ControlModalKind::CLIMATE,       ControlModalKind::SWITCH_CONFIRMATION,
    ControlModalKind::OPTION_SELECT,  ControlModalKind::FAN_PRESET,    ControlModalKind::FAN_CONTROL,
    ControlModalKind::NETWORK_STATUS, ControlModalKind::ALARM_PIN,     ControlModalKind::ALARM_CONTROL,
    ControlModalKind::IMAGE_CARD,     ControlModalKind::WIFI_QR,       ControlModalKind::COVER_CONTROL,
    ControlModalKind::LIGHT_CONTROL,  ControlModalKind::MEDIA_CONTROL, ControlModalKind::WEATHER_FORECAST};
void cycles(const DeviceUiLimits& limits, uint32_t generation) {
  Fixture fixture(limits, generation);
  Origin origin;
  std::array<Family, kinds.size()> families;
  std::array<PageModalRoute, kinds.size()> routes;
  for (size_t i = 0; i < kinds.size(); ++i) {
    families[i].allowed = kinds[i];
    families[i].local = kinds[i] == ControlModalKind::NETWORK_STATUS ||
                        kinds[i] == ControlModalKind::WIFI_QR ||
                        kinds[i] == ControlModalKind::WEATHER_FORECAST;
    routes[i] = {kinds[i], &families[i]};
  }
  families[1].normal = ControlModalKind::CLIMATE;
  PageModalRouter<kinds.size()> router(routes, &origin, Origin::valid);
  PageModalRuntime runtime(router);
  assert(runtime.prepare(fixture.store));
  runtime.activate_prepared(fixture.store);
  const PageBindingToken binding{generation, 1, 0, 0, 0, false};
  assert(runtime.request(binding));
  runtime.loop();
  assert(runtime.token().kind == ControlModalKind::CLIMATE && runtime.valid(runtime.token()));
  runtime.dismiss();
  runtime.loop();
  for (int cycle = 0; cycle < 1000; ++cycle) {
    for (size_t i = 0; i < kinds.size(); ++i) {
      const auto kind = kinds[i];
      if (kind == ControlModalKind::NETWORK_STATUS)
        assert(runtime.request_local({generation, 7, 0}, kind));
      else
        assert(runtime.request(binding, kind));
      runtime.loop();
      const auto token = runtime.token();
      assert(token.kind == kind && runtime.valid(token) && runtime.interactive(token));
      assert(runtime.memory().active_contexts == 1 && runtime.memory().retired_contexts == 0);
      size_t live = 0;
      for (auto& family : families) live += family.live;
      assert(live == 1);
      assert(router.source_revision(&families[i]) == 19);
    }
  }
  runtime.dismiss();
  runtime.loop();
  for (const auto& family : families) assert(family.creates == family.destroys && !family.live);

  // The original binding and admitted family survive queueing, never a newly
  // sampled origin/connection. Local panels keep their own stable offline epoch.
  assert(runtime.request(binding, ControlModalKind::CLIMATE));
  ++families[1].epoch;
  runtime.loop();
  assert(!runtime.token().opening);
  assert(runtime.request(binding, ControlModalKind::CLIMATE));
  ++origin.binding;
  runtime.loop();
  assert(!runtime.token().opening);
  origin.binding = 1;
  for (auto& family : families) family.online = false;
  assert(!runtime.request(binding, ControlModalKind::CLIMATE));
  for (auto kind : {ControlModalKind::WIFI_QR, ControlModalKind::WEATHER_FORECAST}) {
    assert(runtime.request(binding, kind));
    runtime.loop();
    assert(runtime.valid(runtime.token()));
  }
  runtime.dismiss();
  runtime.loop();
}
void failures_and_retirement() {
  Fixture fixture(DEVICE_UI_PROFILES[8], 0);
  Family climate, alarm;
  climate.normal = climate.allowed = ControlModalKind::CLIMATE;
  alarm.allowed = ControlModalKind::ALARM_CONTROL;
  Origin origin;
  PageModalRouter<2> router({{{climate.allowed, &climate}, {alarm.allowed, &alarm}}}, &origin, Origin::valid);
  PageModalRuntime runtime(router);
  assert(runtime.prepare(fixture.store));
  runtime.activate_prepared(fixture.store);
  const ConfigurationHandle card{0, 0};
  assert(!router.create(fixture.store, card, {0, 0, 0, climate.allowed}));
  assert(!router.create(fixture.store, card, {1, 1, 0, climate.allowed}));
  assert(!router.create(fixture.store, card, {0, 1, 1, climate.allowed}));
  climate.busy = true;
  assert(!router.create(fixture.store, card, {0, 1, 0, climate.allowed}));
  climate.busy = false;
  climate.fail_create = true;
  assert(runtime.request_card(card));
  runtime.loop();
  assert(!runtime.token().opening);
  climate.fail_create = false;
  climate.fail_initialize = true;
  assert(runtime.request_card(card));
  runtime.loop();
  assert(climate.creates == climate.destroys && climate.reason == PageModalClose::FAILED);
  climate.fail_initialize = false;
  climate.on_create = [&] { runtime.dismiss(PageModalClose::REPLACED); };
  assert(runtime.request_card(card));
  runtime.loop();
  assert(!runtime.token().opening && climate.reason == PageModalClose::REPLACED);
  climate.on_create = {};
  climate.on_initialize = [&] { runtime.notify_closed(climate.token); };
  assert(runtime.request_card(card));
  runtime.loop();
  assert(!runtime.token().opening && climate.creates == climate.destroys);
  climate.on_initialize = {};
  assert(runtime.request_card(card));
  runtime.loop();
  const auto closing = runtime.token();
  auto stale = closing;
  ++stale.opening;
  assert(!router.initialize(&climate, stale));
  const auto updates = climate.updates;
  router.update(&climate, stale);
  assert(climate.updates == updates);
  climate.busy = true;  // Decoder/draw lease keeps the old family allocated.
  runtime.dismiss();
  assert(runtime.request_card(card, ControlModalKind::ALARM_CONTROL));
  runtime.loop();
  assert(climate.live && climate.retired && !alarm.live);
  bool finished = false;
  climate.on_destroy = [&] {
    assert(runtime.can_finish_retired(closing));
    runtime.loop();
    assert(!alarm.live);
    finished = true;
  };
  climate.busy = false;
  runtime.loop();
  assert(finished && !climate.live && alarm.live);
  assert(runtime.display_takeover(true));  // Preserved alarm control keeps its full family identity.
  assert(runtime.valid(runtime.token()));
  runtime.display_takeover(false);
  runtime.loop();
  assert(!alarm.live && alarm.creates == alarm.destroys);

  alarm.normal = ControlModalKind::ALARM_CONTROL;
  assert(router.kind(fixture.store, card) == ControlModalKind::NONE);  // Ambiguous defaults fail closed.
  assert(router.kind(fixture.store, card, ControlModalKind::CLIMATE) == ControlModalKind::CLIMATE);
  assert(router.kind(fixture.store, card, ControlModalKind::MEDIA_CONTROL) == ControlModalKind::NONE);
  PageModalRouter<2> duplicate({{{climate.allowed, &climate}, {climate.allowed, &alarm}}}, &origin,
                               Origin::valid);
  PageModalRouter<1> missing({{{ControlModalKind::CLIMATE, nullptr}}}, &origin, Origin::valid);
  assert(!duplicate.ready() && !missing.ready() && !missing.valid_origin({}));
}
}  // namespace
int main() {
  for (const auto& limits : DEVICE_UI_PROFILES) cycles(limits, 0);
  failures_and_retirement();
}
