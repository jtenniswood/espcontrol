#include "home_assistant_transport_runtime.h"

#include <cstring>

#include "esphome/core/defines.h"

#ifdef USE_API_HOMEASSISTANT_STATES
#include "esphome/components/api/api_server.h"
#include "esphome/core/version.h"
#include "esphome/core/log.h"
#include "external_memory_allocator.h"
#include "owned_ha_subscriptions.h"
#include "native_ha_state_dispatch.h"
#include "esphome/core/hal.h"
#ifdef USE_ESP32
#include "esp_heap_caps.h"
#endif

// This bridge deliberately uses only public SDK record definitions. ESPHome
// exposes a const view but no unsubscribe API. Audit enumeration and dispatch
// again when changing the pinned SDK, rather than assuming its contract holds.
// The API server/connection sources are byte-identical in these two patches.
static_assert(ESPHOME_VERSION_CODE == VERSION_CODE(2026, 10, 1) ||
                  ESPHOME_VERSION_CODE == VERSION_CODE(2026, 10, 2),
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
    // Replaying a live HA session adds duplicate remote listeners. The native
    // protocol has no remote unsubscribe; reconnect HA so its peer removes the
    // old listeners before enumerating the changed list. This public hook only
    // marks removal: APIServer closes it before advancing an obsolete cursor.
    // Diagnostic clients keep their sessions; never mutate active_clients here.
    for (const auto &client : esphome::api::global_api_server->active_clients()) {
      if (!client || client->is_marked_for_removal() || !client->is_authenticated()) continue;
      const char *name = client->get_name();
      if (name != nullptr && std::strstr(name, "Home Assistant") != nullptr)
        client->on_disconnect_response();
    }
  }
};
using Registry = OwnedHaSubscriptions<NativeBackend, EspControlExternalAllocator<std::byte>>;
void (*maintenance_callback)() = nullptr;
bool refresh_pending = false;
bool refresh_again = false;
size_t refresh_cursor = 0;
void finish_state_refresh() {
  refresh_pending = refresh_again;
  refresh_again = false;
  refresh_cursor = 0;
}
void refresh_one_state() {
  if (!refresh_pending || !NativeBackend::available()) return;
  const auto &subscriptions = esphome::api::global_api_server->get_state_subs();
  if (refresh_cursor >= subscriptions.size()) { finish_state_refresh(); return; }
  const auto &entry = subscriptions[refresh_cursor];
  if (entry.entity_id == nullptr || *entry.entity_id == '\0') {
    if (++refresh_cursor == subscriptions.size()) finish_state_refresh();
    return; // Match native enumeration's handling of unconfigured wizard IDs.
  }
  esphome::api::SubscribeHomeAssistantStateResponse request;
  request.entity_id = esphome::StringRef(entry.entity_id ? entry.entity_id : "");
  request.attribute = esphome::StringRef(entry.attribute ? entry.attribute : "");
  request.once = true;
  bool sent = false, accepted = true;
  for (const auto &client : esphome::api::global_api_server->active_clients()) {
    if (!client || client->is_marked_for_removal() || !client->is_authenticated()) continue;
    const char *name = client->get_name();
    if (name == nullptr || std::strstr(name, "Home Assistant") == nullptr) continue;
    sent = true;
    accepted = client->send_message(request) && accepted;
  }
  // Backpressure retries the same one-shot pair; it never adds remote listeners.
  if (sent && accepted && ++refresh_cursor == subscriptions.size()) finish_state_refresh();
}
}  // namespace

uint32_t subscribe_owned(const std::string &entity, const std::string &attribute,
                         StateCallback callback) {
  return Registry::instance().subscribe(entity, attribute, std::move(callback));
}
void unsubscribe_owned(uint32_t token) { Registry::instance().unsubscribe(token); }
void set_subscription_maintenance(void (*maintenance)()) { maintenance_callback = maintenance; }
void request_state_refresh() {
  // Coalesce requests before the sweep starts. Once it has visited a pair,
  // new consumers may have missed that pair's response: retain one follow-up
  // sweep without restarting the cursor or starving the remaining pairs.
  if (refresh_pending && refresh_cursor > 0) refresh_again = true;
  if (!refresh_pending) refresh_cursor = 0;
  refresh_pending = true;
}
void maintain_subscriptions() {
  if (native_lookup::owner().delivering()) return;
  if (maintenance_callback != nullptr) maintenance_callback();
  auto &registry = Registry::instance();
  const bool changed = registry.flush();
  if (changed) {
    refresh_cursor = 0; // Compaction can move the pair at the current cursor.
    refresh_again = false; // This complete sweep also covers queued requests.
    native_lookup::owner().changed();
  }
  native_lookup::maintain(esphome::millis());
  if (changed) {
    ESP_LOGD("ha_transport", "Native subscriptions: owned=%u pending=%u registry_bytes=%u lookup_bytes=%u upstream=%u capacity=%u heap_internal=%u heap_largest=%u psram_free=%u",
             static_cast<unsigned>(registry.size()), static_cast<unsigned>(registry.pending_count()),
             static_cast<unsigned>(registry.storage_bytes()),
             static_cast<unsigned>(native_lookup::owner().bytes()),
             static_cast<unsigned>(esphome::api::global_api_server->get_state_subs().size()),
             static_cast<unsigned>(esphome::api::global_api_server->get_state_subs().capacity()),
             static_cast<unsigned>(NativeBackend::internal_free()),
             static_cast<unsigned>(NativeBackend::internal_largest()),
             static_cast<unsigned>(NativeBackend::psram_free()));
  }
  refresh_one_state();
}
}  // namespace espcontrol::ha
#else
namespace espcontrol::ha {
uint32_t subscribe_owned(const std::string &, const std::string &, StateCallback) { return 0; }
void unsubscribe_owned(uint32_t) {}
void set_subscription_maintenance(void (*)()) {}
void request_state_refresh() {}
void maintain_subscriptions() {}
}  // namespace espcontrol::ha
#endif
