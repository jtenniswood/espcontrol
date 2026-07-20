#include "event_stream_policy.h"

#include <cstdlib>

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
  return EXIT_SUCCESS;
}
