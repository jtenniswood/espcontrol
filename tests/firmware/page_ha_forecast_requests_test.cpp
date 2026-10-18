#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

#include "page_forecast_runtime.h"
using namespace espcontrol::pages;
namespace esphome {
uint32_t millis() {
  return 100;
}
namespace api {
struct Json {
  const char* payload;
  Json operator[](const char*) const {
    return *this;
  }
  template <class T>
  T as() const {
    return payload;
  }
};
struct ActionResponse {
  bool success;
  const char* payload;
  bool is_success() const {
    return success;
  }
  Json get_json() const {
    return {payload};
  }
};
struct HomeassistantActionRequest {
  uint32_t call_id = 0;
  bool wants_response = false;
  std::string response_template, entity, type;
};
}  // namespace api
}  // namespace esphome
using Response = esphome::api::ActionResponse;
using Request = esphome::api::HomeassistantActionRequest;
std::unordered_map<uint32_t, std::function<void(const Response&)>> callbacks;
bool online = true, heap = true, register_ok = true, begin_ok = true, send_ok = true;
size_t begins = 0, sends = 0;
uint32_t next_id = 1;
std::function<void()> during_send;
constexpr size_t HA_ACTION_INTERNAL_FREE_MIN_BYTES = 12000, HA_ACTION_INTERNAL_LARGEST_MIN_BYTES = 4000;
bool ha_api_state_connected() {
  return online;
}
struct Coordinator {
  uint32_t connection_generation() const {
    return 7;
  }
} coordinator;
auto& ha_read_coordinator() {
  return coordinator;
}
bool ha_internal_heap_available(const char*, size_t free, size_t largest) {
  assert(free == HA_ACTION_INTERNAL_FREE_MIN_BYTES && largest == HA_ACTION_INTERNAL_LARGEST_MIN_BYTES);
  return heap;
}
uint32_t next_weather_forecast_call_id() {
  return next_id++;
}
bool ha_action_begin(Request& request, const char* service, bool event, size_t count, uint32_t id) {
  ++begins;
  assert(std::string(service) == "weather.get_forecasts" && !event && count == 2);
  request.call_id = id;
  return begin_ok;
}
std::string weather_daily_response_template(const std::string& entity) {
  return "DAILY:" + entity;
}
void ha_action_add_entity(Request& request, const std::string& entity) {
  request.entity = entity;
}
void ha_action_add_data(Request& request, const char* key, const char* value) {
  assert(std::string(key) == "type");
  request.type = value;
}
bool ha_register_action_response_callback(uint32_t id, std::function<void(const Response&)> callback) {
  if (!register_ok) return false;
  callbacks[id] = std::move(callback);
  return true;
}
void reply(uint32_t id, const Response& response) {
  auto it = callbacks.find(id);
  assert(it != callbacks.end());
  auto callback = std::move(it->second);
  callbacks.erase(it);
  callback(response);
}
bool ha_cancel_action_response_callback(uint32_t id, const char*) {
  if (callbacks.find(id) == callbacks.end()) return false;
  reply(id, {false, nullptr});
  return true;
}
bool ha_action_send(Request& request) {
  ++sends;
  assert(request.wants_response && request.type == "daily" &&
         request.response_template == "DAILY:" + request.entity);
  const bool result = send_ok;
  if (during_send) {
    auto call = std::move(during_send);
    during_send = {};
    call();
  }
  return result;
}
#include "page_ha_forecast_requests.h"
struct Owner {
  size_t calls = 0, bytes = 0;
  bool success = false;
  PageForecastToken token;
  std::function<void()> hook;
  static void receive(void* pointer, PageForecastToken token, std::string_view payload, bool success) {
    auto& owner = *static_cast<Owner*>(pointer);
    ++owner.calls;
    owner.bytes = payload.size();
    owner.success = success;
    owner.token = token;
    if (owner.hook) {
      auto call = std::move(owner.hook);
      owner.hook = {};
      call();
    }
  }
};
int main() {
  PageHaForecastRequests transport;
  Owner owner;
  PageForecastToken token{17, 7, 1, 0};
  assert(transport.connected() && transport.connection_generation() == 7 && transport.now_ms() == 100);
#if defined(ESPCONTROL_DISABLE_WEATHER_FORECAST) && ESPCONTROL_DISABLE_WEATHER_FORECAST
  assert(!transport.start(&owner, token, "weather.home", Owner::receive) && callbacks.empty() && begins == 0);
  return 0;
#else
  heap = false;
  assert(!transport.start(&owner, token, "weather.home", Owner::receive) && begins == 0);
  heap = true;
  online = false;
  assert(!transport.start(&owner, token, "weather.home", Owner::receive));
  online = true;
  assert(!transport.start(&owner, token, "sensor.home", Owner::receive));
  begin_ok = false;
  assert(!transport.start(&owner, token, "weather.home", Owner::receive));
  begin_ok = true;
  register_ok = false;
  assert(!transport.start(&owner, token, "weather.home", Owner::receive) && callbacks.empty());
  register_ok = true;
  send_ok = false;
  assert(!transport.start(&owner, token, "weather.home", Owner::receive) && callbacks.empty() &&
         owner.calls == 0);
  send_ok = true;
  assert(transport.start(&owner, token, "weather.home", Owner::receive) && callbacks.size() == 1);
  const auto first = callbacks.begin()->first;
  const auto late = callbacks.begin()->second;
  assert(!transport.start(&owner, token, "weather.home", Owner::receive));
  auto wrong = token;
  ++wrong.source;
  transport.cancel(&owner, wrong);
  assert(callbacks.size() == 1);
  Owner other;
  transport.cancel(&other, token);
  assert(callbacks.size() == 1);
  reply(first, {true, "payload"});
  assert(owner.calls == 1 && owner.success && owner.bytes == 7 && callbacks.empty());
  ++token.request;
  assert(transport.start(&owner, token, "weather.home", Owner::receive));
  late({true, "OLD"});
  assert(owner.calls == 1 && callbacks.size() == 1);
  transport.cancel(&owner, token);
  assert(callbacks.empty() && owner.calls == 1);
  ++token.request;
  assert(transport.start(&owner, token, "weather.home", Owner::receive));
  reply(callbacks.begin()->first, {false, nullptr});
  assert(owner.calls == 2 && !owner.success);
  ++token.request;
  assert(transport.start(&owner, token, "weather.home", Owner::receive));
  reply(callbacks.begin()->first, {true, nullptr});
  assert(owner.calls == 3 && !owner.success);
  ++token.request;
  assert(transport.start(&owner, token, "weather.home", Owner::receive));
  const std::string oversized(10000, 'x');
  reply(callbacks.begin()->first, {true, oversized.c_str()});
  assert(owner.calls == 4 && owner.success && owner.bytes == espcontrol::weather::MAX_PAYLOAD_BYTES + 1);
  // A response may start another request before the original send returns.
  // A failed outer send must not clear that replacement's record/callback.
  ++token.request;
  const auto original = next_id;
  owner.hook = [&]() {
    ++token.request;
    send_ok = true;
    assert(transport.start(&owner, token, "weather.after", Owner::receive));
  };
  during_send = [&]() { reply(original, {true, "sync"}); };
  send_ok = false;
  assert(!transport.start(&owner, token, "weather.home", Owner::receive));
  assert(callbacks.size() == 1 && owner.calls == 5);
  transport.cancel(&owner, token);
  assert(callbacks.empty());
#endif
}
