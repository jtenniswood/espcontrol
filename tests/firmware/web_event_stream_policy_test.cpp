#include "event_stream_policy.h"

#include <cstdlib>
#include <cstring>

namespace {

[[noreturn]] void fail() { std::abort(); }

void require(bool condition) {
  if (!condition) fail();
}

}  // namespace

int main() {
  using namespace esphome::web_server_idf;
  constexpr size_t allocation = 2048;
  require(event_stream_allocation_available(
      allocation + EVENT_STREAM_HEAP_SAFETY_BYTES, allocation, allocation));
  require(!event_stream_allocation_available(
      allocation + EVENT_STREAM_HEAP_SAFETY_BYTES - 1, allocation, allocation));
  require(!event_stream_allocation_available(
      allocation + EVENT_STREAM_HEAP_SAFETY_BYTES, allocation - 1, allocation));
  require(!event_stream_should_reconnect_after_allocation_failure(true));
  require(event_stream_should_reconnect_after_allocation_failure(false));
  require(event_stream_session_can_send(4, false));
  require(!event_stream_session_can_send(4, true));
  require(!event_stream_session_can_delete(4));
  require(event_stream_session_can_delete(0));

  EventStreamLatestEvent<32, 32> pending;
  require(pending.store("{\"revision\":1}", 14,
                        "espcontrol_configuration"));
  require(pending.pending());
  require(pending.store("{\"revision\":2}", 14,
                        "espcontrol_configuration"));
  require(std::strcmp(pending.message(), "{\"revision\":2}") == 0);
  require(std::strcmp(pending.event(), "espcontrol_configuration") == 0);
  pending.clear();
  require(!pending.pending());
  require(!pending.store("012345678901234567890123456789012", 33, "event"));
  return EXIT_SUCCESS;
}
