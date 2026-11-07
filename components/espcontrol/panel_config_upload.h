#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "configuration_buffer.h"

namespace espcontrol::configuration {

// One request owns one bounded, exact-length body. No handler state survives
// a completed request or a client that disconnects before the final chunk.
class PanelConfigUpload {
public:
  PanelConfigUpload(size_t total, ConfigurationBuffer::Allocate allocate,
                    ConfigurationBuffer::Release release)
      : buffer_(allocate, release), data_(buffer_.allocate(total)),
        expected_(total), valid_(total != 0 && data_ != nullptr) {}

  bool ready() const { return valid_; }
  bool append(const uint8_t *data, size_t size, size_t index, size_t total) {
    if (!valid_ || total != expected_ || index != received_ ||
        size > expected_ - received_ || (size != 0 && data == nullptr)) {
      valid_ = false;
      return false;
    }
    if (size)
      std::memcpy(data_ + received_, data, size);
    received_ += size;
    return true;
  }
  bool complete() const { return valid_ && received_ == expected_; }
  const uint8_t *data() const { return data_; }
  size_t size() const { return received_; }

private:
  ConfigurationBuffer buffer_;
  uint8_t *data_;
  size_t expected_;
  size_t received_{0};
  bool valid_;
};

} // namespace espcontrol::configuration
