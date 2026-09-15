#pragma once
#include <cstddef>
#include <cstring>
namespace esphome {
class StringRef {
 public:
  StringRef() = default;
  StringRef(const char *value, size_t size) : value_(value), size_(size) {}
  explicit StringRef(const char *value) : StringRef(value, std::strlen(value)) {}
  const char *c_str() const { return value_; }
  size_t size() const { return size_; }
 private:
  const char *value_{""};
  size_t size_{0};
};
}
