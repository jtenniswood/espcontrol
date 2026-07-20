#pragma once

#include <cstddef>

namespace esphome::web_server_idf {

constexpr size_t EVENT_STREAM_HEAP_SAFETY_BYTES = 8 * 1024;

inline bool event_stream_allocation_available(size_t free_bytes,
                                              size_t largest_block,
                                              size_t allocation_bytes) {
  return largest_block >= allocation_bytes &&
         free_bytes >= allocation_bytes + EVENT_STREAM_HEAP_SAFETY_BYTES;
}

inline bool event_stream_should_reconnect_after_allocation_failure(bool allocation_available) {
  return !allocation_available;
}

}  // namespace esphome::web_server_idf
