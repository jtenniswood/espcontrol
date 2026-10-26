#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "esphome/core/string_ref.h"

namespace esphome::api {
struct APIConnection {
  bool authenticated{true};
  bool removed{false};
  int cursor{-1};
  size_t enumerations{0};
  bool home_assistant_states{false};
  bool is_marked_for_removal() const { return removed; }
  bool is_authenticated() const { return authenticated; }
  void on_subscribe_home_assistant_states_request() {
    cursor = 0;
    home_assistant_states = true;
    ++enumerations;
  }
};
struct APIServer {
  struct HomeAssistantStateSubscription {
    const char *entity_id;
    const char *attribute;
    std::function<void(StringRef)> callback;
    bool once;
    std::unique_ptr<std::string> entity_id_dynamic_storage{};
    std::unique_ptr<std::string> attribute_dynamic_storage{};
  };
  std::vector<HomeAssistantStateSubscription> subscriptions;
  std::vector<std::unique_ptr<APIConnection>> clients;
  const auto &get_state_subs() const { return subscriptions; }
  const auto &active_clients() const { return clients; }
  void subscribe_home_assistant_state(const char *entity, const char *attribute,
                                    std::function<void(StringRef)> &&callback) {
    subscriptions.push_back({entity, attribute, std::move(callback), false});
  }
};
inline APIServer *global_api_server = nullptr;
}  // namespace esphome::api
