#include "native_ha_state_dispatch.h"
#include "home_assistant_transport_runtime.h"
#include "external_memory_allocator.h"
#include "esphome/components/api/api_connection.h"
#include "esphome/core/hal.h"

#include <cassert>
#include <cstdlib>
#include <new>
#include <string_view>
#include <vector>

namespace {
bool watch_allocations = false;
size_t allocations = 0, original_calls = 0;
}
void *operator new(size_t size) {
  if (watch_allocations) ++allocations;
  if (auto *p = std::malloc(size ? size : 1)) return p;
  throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void *operator new[](size_t size) { return ::operator new(size); }
void operator delete[](void *p) noexcept { ::operator delete(p); }
#ifdef __cpp_sized_deallocation
void operator delete(void *p, size_t) noexcept { std::free(p); }
void operator delete[](void *p, size_t) noexcept { std::free(p); }
#endif

namespace esphome::api {
// Independent stock SDK semantics, including owning server and exact ordering.
void APIConnection::on_home_assistant_state_response(const HomeAssistantStateResponse &message) {
  ++original_calls;
  if (message.entity_id.size() == 0) return;
  if (message.state.size()) const_cast<char *>(message.state.c_str())[message.state.size()] = '\0';
  for (const auto &row : parent->get_state_subs()) {
    if (std::string_view(message.entity_id.c_str(), message.entity_id.size()) != row.entity_id) continue;
    if (row.attribute ? std::string_view(message.attribute.c_str(), message.attribute.size()) != row.attribute
                      : message.attribute.size() != 0) continue;
    row.callback(message.state);
  }
}
}
using Connection = esphome::api::APIConnection;
using Message = esphome::api::HomeAssistantStateResponse;
using Text = esphome::StringRef;
extern "C" void __real__ZN7esphome3api13APIConnection32on_home_assistant_state_responseERKNS0_26HomeAssistantStateResponseE(Connection *self, const Message &message) {
  self->on_home_assistant_state_response(message);
}
extern "C" void __wrap__ZN7esphome3api13APIConnection32on_home_assistant_state_responseERKNS0_26HomeAssistantStateResponseE(Connection *, const Message &);

namespace {
struct Context {
  std::vector<int> calls;
  bool check_guard{false}, invalidate{false};
  size_t expected_native{0}, expected_state_size{4}, maintenance{0};
};
Context *context;
void maintenance() { ++context->maintenance; }
std::function<void(Text)> callback(Context &ctx, int id) {
  return [&ctx, id](Text state) {
    assert(state.size() == ctx.expected_state_size);
    if (state.size()) assert(std::memcmp(state.c_str(), "live", state.size()) == 0 && state.c_str()[state.size()] == '\0');
    ctx.calls.push_back(id);
    if (id == 1 && ctx.check_guard) {
      assert(espcontrol::ha::native_lookup::owner().delivering());
      const auto before = ctx.maintenance;
      espcontrol::ha::maintain_subscriptions();
      assert(ctx.maintenance == before);
      assert(esphome::api::global_api_server->subscriptions.size() == ctx.expected_native);
    }
    if (id == 1 && ctx.invalidate) {
      ctx.invalidate = false;
      espcontrol::ha::native_lookup::owner().changed();
    }
  };
}
void foreign(esphome::api::APIServer &server, Context &ctx, int id,
             const char *entity = "sensor.same", const char *attribute = "unit", bool once = true) {
  esphome::api::APIServer::HomeAssistantStateSubscription row{};
  row.entity_id_dynamic_storage = std::make_unique<std::string>(entity);
  row.entity_id = row.entity_id_dynamic_storage->c_str();
  if (attribute) {
    row.attribute_dynamic_storage = std::make_unique<std::string>(attribute);
    row.attribute = row.attribute_dynamic_storage->c_str();
  }
  row.callback = callback(ctx, id);
  row.once = once;
  server.subscriptions.push_back(std::move(row));
}
Connection *client(esphome::api::APIServer &server) {
  auto value = std::make_unique<Connection>();
  value->parent = &server;
  auto *result = value.get();
  server.clients.push_back(std::move(value));
  return result;
}
void deliver(Connection *connection, Context &ctx, const char *entity = "sensor.same",
             const char *attribute = "unit", bool empty_state = false) {
  ctx.calls.clear();
  ctx.expected_state_size = empty_state ? 0 : 4;
  char state[] = "liveZ";
  Message message{Text(entity), Text(attribute), Text(state, empty_state ? 0 : 4)};
  allocations = 0;
  watch_allocations = true;
  __wrap__ZN7esphome3api13APIConnection32on_home_assistant_state_responseERKNS0_26HomeAssistantStateResponseE(connection, message);
  watch_allocations = false;
  assert(allocations == 0);
  assert(state[4] == ((entity[0] && !empty_state) ? '\0' : 'Z'));
}
}

int main() {
  using namespace espcontrol;
  esphome::api::APIServer server, other_server;
  esphome::api::global_api_server = &server;
  auto *connection = client(server);
  auto *other_connection = client(other_server);
  Context ctx;
  ctx.calls.reserve(128);
  context = &ctx;
  ha::set_subscription_maintenance(maintenance);
  foreign(server, ctx, 1);
  foreign(other_server, ctx, 4);
  const auto *foreign_entity = server.subscriptions[0].entity_id;
  const auto *foreign_attribute = server.subscriptions[0].attribute;
  std::vector<uint32_t> tokens;
  tokens.push_back(ha::subscribe_owned("sensor.same", "unit", callback(ctx, 2)));
  tokens.push_back(ha::subscribe_owned("sensor.same", "unit", callback(ctx, 3)));
  for (int i = 0; i < 62; ++i)
    tokens.push_back(ha::subscribe_owned("sensor.filler_" + std::to_string(i), "unit", callback(ctx, 100 + i)));
  ctx.check_guard = true;
  ctx.expected_native = 1;
  deliver(connection, ctx);  // Stock fallback must not flush pending owned rows from a foreign callback.
  assert((ctx.calls == std::vector<int>{1}) && original_calls == 1);
  ctx.check_guard = false;
  ha::maintain_subscriptions();
  assert(server.subscriptions.size() == 65 && ha::native_lookup::owner().current(&server, server.subscriptions));
  assert(ha::native_lookup::owner().bytes() == 520);
  assert((fake_esphome_allocator::last_allocation_flags & 3) == 3);
  const auto stock_before = original_calls;
  ctx.check_guard = true;
  ctx.expected_native = 65;
  deliver(connection, ctx);
  assert((ctx.calls == std::vector<int>{1, 2, 3}) && original_calls == stock_before);
  ctx.check_guard = false;
  deliver(other_connection, ctx);  // Another owning server must always retain the real handler.
  assert((ctx.calls == std::vector<int>{4}) && original_calls == stock_before + 1);
  foreign(server, ctx, 5);
  deliver(connection, ctx);  // Native append makes the index stale until maintenance.
  assert((ctx.calls == std::vector<int>{1, 2, 3, 5}) && original_calls == stock_before + 2);
  ha::maintain_subscriptions();
  const auto native_size = server.subscriptions.size();
  const auto epoch = ha::native_lookup::owner().epoch();
  ha::unsubscribe_owned(tokens[0]);
  auto replacement = ha::subscribe_owned("sensor.new", "unit", callback(ctx, 6));
  ha::maintain_subscriptions();
  assert(server.subscriptions.size() == native_size && ha::native_lookup::owner().epoch() != epoch);
  deliver(connection, ctx);
  assert((ctx.calls == std::vector<int>{1, 3, 5}));
  deliver(connection, ctx, "sensor.new");
  assert((ctx.calls == std::vector<int>{6}));
  const auto before_interrupt = original_calls;
  ctx.invalidate = true;
  deliver(connection, ctx);
  assert((ctx.calls == std::vector<int>{1}) && original_calls == before_interrupt && ha::native_lookup::interrupted_frames == 1);
  deliver(connection, ctx);
  assert((ctx.calls == std::vector<int>{1, 3, 5}) && original_calls == before_interrupt + 1);
  ha::maintain_subscriptions();
  deliver(connection, ctx, "");
  assert(ctx.calls.empty());
  foreign(server, ctx, 7, "sensor.null", nullptr);
  foreign(server, ctx, 8, "sensor.null", "");
  ha::maintain_subscriptions();
  deliver(connection, ctx, "sensor.null", "", true);
  assert((ctx.calls == std::vector<int>{7, 8}));
  for (auto token : tokens) ha::unsubscribe_owned(token);
  ha::unsubscribe_owned(replacement);
  ha::maintain_subscriptions();
  assert(ha::native_lookup::owner().bytes() == 0 && EspControlExternalAllocatorStats::internal_bytes == 0);
  assert(server.subscriptions.size() == 4 && server.subscriptions[0].once);
  assert(server.subscriptions[0].entity_id == foreign_entity && server.subscriptions[0].attribute == foreign_attribute);
  for (int i = 0; i < 64; ++i) foreign(server, ctx, 100 + i, "sensor.unmatched");
  fake_esphome_allocator::external_available = fake_esphome_allocator::internal_available = false;
  const auto failures = ha::native_lookup::owner().failures();
  ha::maintain_subscriptions();
  assert(ha::native_lookup::owner().bytes() == 0 && ha::native_lookup::owner().failures() == failures + 1);
  deliver(connection, ctx);
  assert((ctx.calls == std::vector<int>{1, 5}));
  fake_esphome_allocator::external_available = fake_esphome_allocator::internal_available = true;
  esphome::owned_ha_test_now += 1000;
  ha::maintain_subscriptions();
  assert(ha::native_lookup::owner().current(&server, server.subscriptions));
  deliver(connection, ctx);
  assert((ctx.calls == std::vector<int>{1, 5}));
  ha::set_subscription_maintenance(nullptr);
  esphome::api::global_api_server = nullptr;
  ha::maintain_subscriptions();
  assert(ha::native_lookup::owner().bytes() == 0 && fake_esphome_allocator::external_pointers.empty());
}
