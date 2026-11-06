#include "home_assistant_transport_runtime.h"
#include "esphome/components/api/api_server.h"
#include "external_memory_allocator.h"

#include <cassert>
#include <string>
#include <set>

int main() {
  esphome::api::APIServer server;
  esphome::api::global_api_server = &server;
  for (int i = 0; i < 5; ++i) server.clients.push_back(std::make_unique<esphome::api::APIConnection>());
  server.clients[1]->authenticated = false;
  server.clients[2]->removed = true;
  server.clients[3]->name = "ESPHome Logs";
  server.clients[3]->cursor = 7;
  server.clients[4]->name = nullptr;
  // Model HA's connection-owned listeners: a repeated enumeration adds
  // another listener; only connection teardown removes the old listeners.
  std::multiset<std::string> listeners;
  auto reconnect = [&]() {
    auto &client = *server.clients[0];
    assert(client.removed);
    listeners.clear();
    client.removed = false;
    client.on_subscribe_home_assistant_states_request();
    for (const auto &entry : server.subscriptions)
      listeners.insert(std::string(entry.entity_id) + "/" + (entry.attribute ? entry.attribute : ""));
    client.cursor = -1;
    assert(listeners.size() == server.subscriptions.size());
    for (const auto &pair : listeners) assert(listeners.count(pair) == 1);
  };
  size_t foreign = 0;
  esphome::api::APIServer::HomeAssistantStateSubscription foreign_record{nullptr, nullptr,
                                                                         [&](auto) { ++foreign; }, true};
  foreign_record.entity_id_dynamic_storage = std::make_unique<std::string>("sensor.same");
  foreign_record.attribute_dynamic_storage = std::make_unique<std::string>("unit");
  foreign_record.entity_id = foreign_record.entity_id_dynamic_storage->c_str();
  foreign_record.attribute = foreign_record.attribute_dynamic_storage->c_str();
  const auto* foreign_entity = foreign_record.entity_id;
  const auto* foreign_attribute = foreign_record.attribute;
  server.subscriptions.push_back(std::move(foreign_record));
  size_t delivered = 0;
  auto token = espcontrol::ha::subscribe_owned("sensor.same", "unit", [&](auto) { ++delivered; });
  assert(token != 0 && server.subscriptions.size() == 1);
  espcontrol::ha::maintain_subscriptions();
  assert(server.subscriptions.size() == 2);
  assert(server.clients[0]->disconnects == 1 && server.clients[0]->enumerations == 0);
  assert(server.clients[1]->disconnects == 0 && server.clients[2]->disconnects == 0);
  // The foreign once-read intentionally shares a pair for this ownership check.
  server.clients[0]->removed = false;
  auto stale = server.subscriptions[1].callback;
  stale(esphome::StringRef("on"));
  assert(delivered == 1);
  espcontrol::ha::unsubscribe_owned(token);
  stale(esphome::StringRef("late"));
  assert(delivered == 1 && server.subscriptions.size() == 2);
  espcontrol::ha::maintain_subscriptions();
  assert(server.subscriptions.size() == 1 && server.subscriptions[0].once);
  server.subscriptions[0].callback(esphome::StringRef("C"));
  assert(foreign == 1 && server.clients[0]->disconnects == 2);
  reconnect();
  assert(EspControlExternalAllocatorStats::internal_bytes == 0);
  size_t maintenance = 0;
  static size_t *maintenance_counter;
  maintenance_counter = &maintenance;
  espcontrol::ha::set_subscription_maintenance([]() { ++*maintenance_counter; });
  espcontrol::ha::maintain_subscriptions();
  assert(maintenance == 1);
  assert(server.clients[0]->disconnects == 2 && !server.clients[0]->removed);
  espcontrol::ha::set_subscription_maintenance(nullptr);
  // Exercise actual bridge compaction, including the native capacity bound.
  for (int cycle = 0; cycle < 100; ++cycle) {
    std::vector<uint32_t> tokens;
    for (int i = 0; i < 50; ++i)
      tokens.push_back(espcontrol::ha::subscribe_owned("sensor.distinct_" + std::to_string(i), "", [](auto) {}));
    espcontrol::ha::maintain_subscriptions();
    assert(server.subscriptions.size() == 51);
    assert(server.clients[0]->removed);
    const auto disconnects = server.clients[0]->disconnects;
    espcontrol::ha::maintain_subscriptions();
    assert(server.clients[0]->disconnects == disconnects);
    reconnect();
    for (auto id : tokens) espcontrol::ha::unsubscribe_owned(id);
    espcontrol::ha::maintain_subscriptions();
    assert(server.subscriptions.size() == 1 && server.subscriptions.capacity() <= 8);
    reconnect();
    assert(listeners.size() == 1 && listeners.count("sensor.same/unit") == 1);
    assert(server.subscriptions[0].entity_id == foreign_entity &&
           server.subscriptions[0].attribute == foreign_attribute &&
           *server.subscriptions[0].entity_id_dynamic_storage == "sensor.same" &&
           *server.subscriptions[0].attribute_dynamic_storage == "unit");
    assert(EspControlExternalAllocatorStats::internal_bytes == 0);
  }
  assert(server.clients[0]->disconnects == 202);
  assert(server.clients[1]->disconnects == 0 && server.clients[2]->disconnects == 0);
  assert(!server.clients[3]->removed && server.clients[3]->disconnects == 0 && server.clients[3]->cursor == 7);
  assert(!server.clients[4]->removed && server.clients[4]->disconnects == 0);
  server.clients.push_back(nullptr);
  auto final_token=espcontrol::ha::subscribe_owned("sensor.final","",[](auto) {});
  espcontrol::ha::maintain_subscriptions();
  assert(server.clients[0]->disconnects == 203);
  espcontrol::ha::unsubscribe_owned(final_token);
  espcontrol::ha::maintain_subscriptions();
  assert(server.clients[0]->disconnects == 203); // Already marked; no repeated disconnect.
  esphome::api::global_api_server = nullptr;
}
