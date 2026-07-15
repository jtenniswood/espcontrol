#pragma once

#include <cstddef>
#include <cstring>

namespace esphome {

class StringRef {
 public:
  explicit StringRef(const char *value)
      : value_(value ? value : ""), size_(value ? std::strlen(value) : 0) {}
  StringRef(const char *value, size_t size) : value_(value ? value : ""), size_(size) {}

  const char *c_str() const { return value_; }
  size_t size() const { return size_; }

 private:
  const char *value_;
  size_t size_;
};

}  // namespace esphome
