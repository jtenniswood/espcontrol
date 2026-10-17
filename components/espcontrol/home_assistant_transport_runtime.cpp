#include "home_assistant_transport_runtime.h"

#include "esphome/core/defines.h"

#ifdef USE_API_HOMEASSISTANT_STATES
#include "esphome/components/api/api_server.h"
#include "esphome/core/version.h"
#include "esphome/core/log.h"
#include "external_memory_allocator.h"
#include "owned_ha_subscriptions.h"
#ifdef USE_ESP32
#include "esp_heap_caps.h"
#endif

// This bridge deliberately uses only public SDK record definitions. ESPHome
// exposes a const view but no unsubscribe API. Audit enumeration and dispatch
// again when changing the pinned SDK, rather than assuming its contract holds.
static_assert(ESPHOME_VERSION_CODE == VERSION_CODE(2026, 9, 1),
              "Audit owned HA subscription removal for the new ESPHome version");

namespace espcontrol::ha {
namespace {
struct NativeBackend {
  using State = esphome::StringRef;
  using Callback = StateCallback;
  static bool available() { return esphome::api::global_api_server != nullptr; }
  static size_t internal_free() {
#ifdef USE_ESP32
    return heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
#else
    return 0;
#endif
  }
  static size_t internal_largest() {
#ifdef USE_ESP32
    return heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
#else
    return 0;
#endif
  }
  static size_t psram_free() {
#ifdef USE_ESP32
    return heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
#else
    return 0;
#endif
  }
  static void subscribe(const char *entity, const char *attribute, Callback callback) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
        entity, attribute, std::move(callback));
  }
  template<typename Predicate>
  static bool remove_if(Predicate predicate) {
    auto &server = *esphome::api::global_api_server;
    // APIServer is mutable; its public getter's const qualification protects
    // normal readers. Only this loop-owned bridge mutates the underlying list,
    // after native dispatch has returned. No offset/private-layout assumptions.
    using Subscriptions = std::vector<esphome::api::APIServer::HomeAssistantStateSubscription>;
    auto &subscriptions = const_cast<Subscriptions &>(server.get_state_subs());
    const size_t before = subscriptions.size();
    subscriptions.erase(std::remove_if(subscriptions.begin(), subscriptions.end(),
        [&](const auto &entry) { return predicate(entry.entity_id, entry.attribute, entry.once); }), subscriptions.end());
    if (subscriptions.size() == before) return false;
    if (subscriptions.capacity() > std::max(size_t{8}, subscriptions.size() * 2)) {
      Subscriptions replacement;
      replacement.reserve(subscriptions.size());
      for (auto &entry : subscriptions) replacement.push_back(std::move(entry));
      replacement.swap(subscriptions);
    }
    return true;
  }
  static void reannounce() {
    // Compaction changes positional cursors. Restart enumeration for every
    // authenticated connection so another component's subscriptions cannot be
    // skipped, including non-HA clients that requested the state list.
    for (const auto &client : esphome::api::global_api_server->active_clients()) {
      if (client && !client->is_marked_for_removal() && client->is_authenticated())
        client->on_subscribe_home_assistant_states_request();
    }
  }
};
using Registry = OwnedHaSubscriptions<NativeBackend, EspControlExternalAllocator<std::byte>>;
void (*maintenance_callback)() = nullptr;
}  // namespace

uint32_t subscribe_owned(const std::string &entity, const std::string &attribute,
                         StateCallback callback) {
  return Registry::instance().subscribe(entity, attribute, std::move(callback));
}
void unsubscribe_owned(uint32_t token) { Registry::instance().unsubscribe(token); }
void set_subscription_maintenance(void (*maintenance)()) { maintenance_callback = maintenance; }
void maintain_subscriptions() {
  if (maintenance_callback != nullptr) maintenance_callback();
  auto &registry = Registry::instance();
  if (registry.flush()) {
    ESP_LOGD("ha_transport", "Native subscriptions: owned=%u pending=%u registry_bytes=%u upstream=%u capacity=%u heap_internal=%u heap_largest=%u psram_free=%u",
             static_cast<unsigned>(registry.size()), static_cast<unsigned>(registry.pending_count()),
             static_cast<unsigned>(registry.storage_bytes()),
             static_cast<unsigned>(esphome::api::global_api_server->get_state_subs().size()),
             static_cast<unsigned>(esphome::api::global_api_server->get_state_subs().capacity()),
             static_cast<unsigned>(NativeBackend::internal_free()),
             static_cast<unsigned>(NativeBackend::internal_largest()),
             static_cast<unsigned>(NativeBackend::psram_free()));
  }
}
}  // namespace espcontrol::ha
#else
namespace espcontrol::ha {
uint32_t subscribe_owned(const std::string &, const std::string &, StateCallback) { return 0; }
void unsubscribe_owned(uint32_t) {}
void set_subscription_maintenance(void (*)()) {}
void maintain_subscriptions() {}
}  // namespace espcontrol::ha
#endif
