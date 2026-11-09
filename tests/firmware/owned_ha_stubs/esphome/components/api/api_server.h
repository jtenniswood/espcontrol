#pragma once
#include <cstdlib>
#include <functional>
#include <memory>
#include <vector>
#include "esphome/core/string_ref.h"

namespace esphome::api {
struct APIServer;
struct HomeAssistantStateResponse;
struct SubscribeHomeAssistantStateResponse {
  StringRef entity_id{""}, attribute{""};
  bool once{false};
};
struct APIConnection {
  APIServer *parent{nullptr};
  bool authenticated{true};
  bool removed{false};
  int cursor{-1};
  size_t enumerations{0};
  size_t disconnects{0};
  const char *name{"Home Assistant 2026.11.0"};
  bool home_assistant_states{false};
  bool writable{true};
  size_t send_attempts{0};
  std::vector<std::pair<std::string,std::string>> requests;
  bool send_message(const SubscribeHomeAssistantStateResponse &request) {
    ++send_attempts;
    if (!writable) return false;
    if (!request.once) std::abort();
    requests.emplace_back(std::string(request.entity_id.c_str(),request.entity_id.size()),
                          std::string(request.attribute.c_str(),request.attribute.size()));
    return true;
  }
  const char *get_name() const { return name; }
  void on_disconnect_response() { removed = true; ++disconnects; }
  bool is_marked_for_removal() const { return removed; }
  bool is_authenticated() const { return authenticated; }
  void on_home_assistant_state_response(const HomeAssistantStateResponse &message);
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
