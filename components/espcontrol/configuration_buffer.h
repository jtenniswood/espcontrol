#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace espcontrol::configuration {

// A serialized configuration operation may borrow RAM, but keeps none at rest.
// Firmware supplies a PSRAM-only allocator; allocation failure is explicit.
class ConfigurationBuffer {
public:
  using Allocate = uint8_t *(*)(size_t);
  using Release = void (*)(uint8_t *);

  static uint8_t *allocate_host(size_t size) {
    return static_cast<uint8_t *>(std::malloc(size));
  }
  static void release_host(uint8_t *buffer) { std::free(buffer); }

  explicit ConfigurationBuffer(Allocate allocate = allocate_host,
                               Release release = release_host)
      : allocate_(allocate), release_(release) {}
  ~ConfigurationBuffer() { reset(); }
  ConfigurationBuffer(const ConfigurationBuffer &) = delete;
  ConfigurationBuffer &operator=(const ConfigurationBuffer &) = delete;

  uint8_t *allocate(size_t size) {
    reset();
    if (size && allocate_ && release_)
      data_ = allocate_(size);
    return data_;
  }
  void reset() {
    if (data_)
      release_(data_);
    data_ = nullptr;
  }

private:
  Allocate allocate_;
  Release release_;
  uint8_t *data_{nullptr};
};

// Bound deferred memory retries without allocating scheduler entries or
// repeatedly attempting a full document on every application-loop iteration.
class ConfigurationBufferRetry {
public:
  bool ready(uint32_t now) const {
    return !pending_ || static_cast<uint32_t>(now - failed_at_) >= 1000;
  }
  void failed(uint32_t now) {
    failed_at_ = now;
    pending_ = true;
  }
  void clear() { pending_ = false; }

private:
  uint32_t failed_at_{0};
  bool pending_{false};
};

} // namespace espcontrol::configuration
