#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome::http_request {

class HttpContainer {
 public:
  virtual ~HttpContainer() = default;
  virtual int read(uint8_t *buffer, size_t max_length) = 0;
  virtual void end() = 0;

  int status_code{0};
  size_t content_length{0};

 protected:
  size_t bytes_read_{0};
};

}  // namespace esphome::http_request
