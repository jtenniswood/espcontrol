#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

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

inline bool event_stream_session_can_send(int fd, bool close_pending) {
  return fd != 0 && !close_pending;
}

inline bool event_stream_session_can_delete(int fd) { return fd == 0; }

// A one-entry, allocation-free queue for coalescible events. Replacing an
// older item is intentional: consumers only need the latest configuration
// revision after a busy socket becomes writable again.
template <size_t MaxMessageBytes, size_t MaxEventBytes>
class EventStreamLatestEvent {
 public:
  bool store(const char *message, size_t message_len, const char *event,
             uint32_t id = 0, uint32_t reconnect = 0) {
    if ((message == nullptr && message_len != 0) ||
        message_len > MaxMessageBytes) {
      return false;
    }
    const size_t event_len = event == nullptr ? 0 : std::strlen(event);
    if (event_len > MaxEventBytes) return false;
    if (message_len != 0) {
      std::memcpy(message_.data(), message, message_len);
    }
    message_[message_len] = '\0';
    if (event_len != 0) std::memcpy(event_.data(), event, event_len);
    event_[event_len] = '\0';
    message_len_ = message_len;
    event_len_ = event_len;
    id_ = id;
    reconnect_ = reconnect;
    pending_ = true;
    return true;
  }

  void clear() { pending_ = false; }
  bool pending() const { return pending_; }
  const char *message() const { return message_.data(); }
  size_t message_len() const { return message_len_; }
  const char *event() const {
    return event_len_ == 0 ? nullptr : event_.data();
  }
  uint32_t id() const { return id_; }
  uint32_t reconnect() const { return reconnect_; }

 private:
  std::array<char, MaxMessageBytes + 1> message_{};
  std::array<char, MaxEventBytes + 1> event_{};
  size_t message_len_{0};
  size_t event_len_{0};
  uint32_t id_{0};
  uint32_t reconnect_{0};
  bool pending_{false};
};

}  // namespace esphome::web_server_idf
