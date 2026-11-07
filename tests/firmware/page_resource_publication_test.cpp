#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "page_card_dependencies.h"
#include "page_forecast_runtime.h"
#include "page_local_relay_runtime.h"
#include "page_local_sensor_runtime.h"
#include "page_media_state_runtime.h"
#include "page_modal_runtime.h"
#include "page_runtime_resources_group.h"
#include "panel_config_document.h"
using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
namespace {
struct Arena : PageArenaAllocator {
  std::unordered_map<void*, size_t> live;
  bool fail{false};
  void* allocate(size_t size) override {
    if (fail) return nullptr;
    auto* pointer = std::malloc(size);
    assert(pointer);
    live[pointer] = size;
    return pointer;
  }
  void release(void* pointer, size_t size) override {
    assert(live.at(pointer) == size);
    live.erase(pointer);
    std::free(pointer);
  }
};
struct Document {
  explicit Document(const std::string& card) {
    PanelConfigWriter writer(data.data(), data.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    const auto* profile = DEVICE_UI_PROFILES[8].device_profile;
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(profile), std::strlen(profile)) ==
           PanelConfigStatus::OK);
    assert(writer.append_button(1, reinterpret_cast<const uint8_t*>(card.data()), card.size()) ==
           PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
  }
  bool prepare(PageStoreRuntime& runtime) const {
    return runtime.prepare(PANEL_CONFIG_DOCUMENT_VERSION, data.data(), size);
  }
  bool activate(PageStoreRuntime& runtime, uint32_t generation) const {
    return runtime.activate_prepared(generation, PANEL_CONFIG_DOCUMENT_VERSION, data.data(), size);
  }
  std::array<uint8_t, 4096> data{};
  size_t size{0};
};
struct Subscriptions : PageEntitySubscriptions {
  struct Binding {
    const char* entity;
    const char* attribute;
    void* owner;
    uint32_t revision, channel;
    Deliver deliver;
  };
  std::vector<Binding> bindings;
  bool subscribe(const char* entity, const char* attribute, void* owner, uint32_t revision, uint32_t channel,
                 Deliver deliver) override {
    bindings.push_back({entity, attribute, owner, revision, channel, deliver});
    return true;
  }
  void release(void* owner) override {
    bindings.erase(std::remove_if(bindings.begin(), bindings.end(),
                                  [&](const auto& binding) { return binding.owner == owner; }),
                   bindings.end());
  }
  uint32_t connection_generation() const override {
    return 1;
  }
  void send(const char* entity, const char* attribute, std::string_view value) {
    for (const auto& binding : bindings)
      if (entity == std::string_view(binding.entity) && attribute == std::string_view(binding.attribute))
        binding.deliver(binding.owner, binding.revision, binding.channel, value);
  }
};
void relay() {
  Arena arena;
  struct Provider : PageLocalRelayProvider {
    bool value{false};
    PageLocalRelaySnapshot read(std::string_view) const override {
      return {value, "Relay"};
    }
  } provider;
  {
    PageLocalRelayRuntime relay(arena, provider);
    PageStoreRuntime runtime(DEVICE_UI_PROFILES[8], arena, 32768, &relay);
    Document document("relay_one;;Auto;Auto;switch;;internal;;");
    assert(document.prepare(runtime));
    provider.value = true;
    assert(document.activate(runtime, 0));
    assert(relay.on(0, "relay_one") && !relay.on(UINT32_MAX, "relay_one") &&
           relay.memory().staged_bytes == 0);
    provider.value = false;
    assert(document.prepare(runtime));
    assert(relay.on(0, "relay_one"));
    runtime.discard_prepared();
    assert(relay.on(0, "relay_one"));
    assert(document.prepare(runtime));
    provider.value = true;
    assert(document.activate(runtime, 2));
    assert(relay.on(2, "relay_one") && !relay.on(0, "relay_one") && relay.memory().staged_bytes == 0);
  }
  assert(arena.live.empty());
}
void media_publication() {
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  struct Transport : PageMediaTransport {
    bool connected() const override {
      return true;
    }
    uint32_t connection_generation() const override {
      return 1;
    }
    uint32_t now_ms() const override {
      return 100;
    }
    int64_t epoch_seconds() const override {
      return 1000;
    }
    void reannounce() override {}
  } transport;
  {
    PageMediaStateRuntime media(arena, transport);
    PageEntityStateRuntime state(dependencies, arena, subscriptions, {64, 64, 2048}, &media);
    PageRuntimeResourcesGroup<2> resources({&media, &state});
    PageStoreRuntime runtime(DEVICE_UI_PROFILES[8], arena, 32768, &resources);
    Document document("media_player.room;;Auto;Auto;play_pause;;media;;");
    assert(document.prepare(runtime));
    subscriptions.send("media_player.room", "", "playing");
    assert(document.activate(runtime, 0));
    PageMediaSourceSnapshot snapshot;
    assert(media.source_snapshot({0, 0}, "media_player.room", snapshot) && snapshot.available);
    assert(!media.source_snapshot({UINT32_MAX, 0}, "media_player.room", snapshot));
    assert(document.prepare(runtime));
    assert(document.activate(runtime, 18));
    assert(media.source_snapshot({18, 0}, "media_player.room", snapshot) && snapshot.available);
    assert(!media.source_snapshot({0, 0}, "media_player.room", snapshot) &&
           media.memory().staged_bytes == 0);
  }
  assert(arena.live.empty() && subscriptions.bindings.empty());
}
void forecast() {
  Arena arena;
  struct Requests : PageForecastRequests {
    uint32_t time{0};
    PageForecastToken token;
    bool connected() const override {
      return true;
    }
    uint32_t connection_generation() const override {
      return 1;
    }
    uint32_t now_ms() const override {
      return time;
    }
    bool start(void*, PageForecastToken value, std::string_view, Receive) override {
      token = value;
      return true;
    }
    void cancel(void*, PageForecastToken) override {}
  } requests;
  {
    PageForecastRuntime forecast(arena, requests);
    PageStoreRuntime runtime(DEVICE_UI_PROFILES[8], arena, 32768, &forecast);
    Document document("weather.home;;Auto;Auto;;;weather;tomorrow;");
    assert(document.prepare(runtime));
    assert(document.activate(runtime, 0));
    PageForecastSnapshot snapshot;
    assert(forecast.snapshot({0, 0}, "weather.home", snapshot));
    assert(!forecast.snapshot({UINT32_MAX, 0}, "weather.home", snapshot));
    requests.time += PageForecastRuntime::READY_DELAY_MS + 1;
    forecast.loop();
    assert(requests.token.configuration == 0);
    assert(document.prepare(runtime));
    assert(document.activate(runtime, 32));
    assert(forecast.snapshot({32, 0}, "weather.home", snapshot) &&
           !forecast.snapshot({0, 0}, "weather.home", snapshot));
    requests.time += PageForecastRuntime::READY_DELAY_MS + 1;
    forecast.loop();
    assert(requests.token.configuration == 32 && forecast.memory().staged_bytes == 0);
  }
  assert(arena.live.empty());
}
void modal() {
  Arena arena;
  struct Port : PageModalPort {
    bool shown{false}, alive{false};
    int context{0};
    std::string entity;
    bool valid_origin(PageBindingToken) const override {
      return true;
    }
    ControlModalKind kind(const PageStore& store, ConfigurationHandle card) const override {
      const auto* definition = store.card(card);
      return definition && store.field(*definition, CardField::TYPE) == "light_control"
                 ? ControlModalKind::LIGHT_CONTROL
                 : ControlModalKind::NONE;
    }
    bool ready() const override {
      return true;
    }
    bool connected() const override {
      return true;
    }
    uint32_t connection_generation() const override {
      return 1;
    }
    void* create(const PageStore& store, ConfigurationHandle card, PageModalToken) override {
      assert(!alive);
      alive = true;
      entity = store.field(*store.card(card), CardField::ENTITY);
      return &context;
    }
    bool initialize(void*, PageModalToken) override {
      shown = true;
      return true;
    }
    bool visible(void*) const override {
      return shown;
    }
    void update(void*, PageModalToken) override {}
    ControlModalDismissPolicy dismiss_policy(void*) const override {
      return ControlModalDismissPolicy::DISMISS;
    }
    void retire(void*, PageModalClose) override {
      shown = false;
      assert(entity == "light.room");
    }
    void destroy(void*) override {
      assert(alive && !shown);
      alive = false;
      entity.clear();
    }
  } port;
  {
    PageModalRuntime modal(port);
    PageStoreRuntime runtime(DEVICE_UI_PROFILES[8], arena, 32768, &modal);
    Document document("light.room;Room;Auto;Auto;;;light_control;;");
    assert(document.prepare(runtime));
    assert(!modal.request_card({UINT32_MAX, 0}));
    assert(document.activate(runtime, 0));
    assert(modal.request_card({0, 0}));
    modal.loop();
    const auto token = modal.token();
    assert(modal.valid(token) && token.configuration == 0);
    assert(document.prepare(runtime));
    runtime.discard_prepared();
    assert(modal.valid(token));
    assert(document.prepare(runtime));
    assert(document.activate(runtime, 2));
    assert(!modal.valid(token));
    modal.loop();
    assert(!modal.request_card({0, 0}) && modal.request_card({2, 0}));
    modal.loop();
    assert(modal.valid(modal.token()) && modal.token().configuration == 2);
  }
  assert(arena.live.empty() && !port.alive);
}
void local() {
  Arena arena;
  struct Provider : PageLocalSensorProvider {
    size_t capacity() const override {
      return 0;
    }
    uint32_t resolve(std::string_view, bool) const override {
      return NO_LOCAL_SENSOR;
    }
    bool watch(uint32_t) override {
      return false;
    }
    PageLocalSensorSample read(uint32_t) const override {
      return {};
    }
  } provider;
  {
    PageLocalSensorRuntime local(arena, provider, {16, 16, 1024});
    PageStoreRuntime runtime(DEVICE_UI_PROFILES[8], arena, 32768, &local);
    Document value("computed;Value;Auto;Auto;local;;sensor;1;");
    Document empty(";;;;sensor.room;;sensor;1;");
    assert(local.install_manual_feed(true));
    assert(value.prepare(runtime) && value.activate(runtime, 0));
    assert(page_local_sensor_feed().apply_number("computed", 31.5f));
    assert(local.read({0, 0}, "computed", false).status == PageEntityStatus::AVAILABLE);
    assert(empty.prepare(runtime));
    runtime.discard_prepared();
    assert(local.read({0, 0}, "computed", false).status == PageEntityStatus::AVAILABLE);
    assert(empty.prepare(runtime) && empty.activate(runtime, 42));
    assert(local.memory().state.active_channels == 0 && local.memory().binding_bytes == 0);
    assert(local.read({0, 0}, "computed", false).status == PageEntityStatus::STALE);
  }
  assert(arena.live.empty());
}
}  // namespace
int main() {
  relay();
  media_publication();
  forecast();
  local();
  modal();
}
