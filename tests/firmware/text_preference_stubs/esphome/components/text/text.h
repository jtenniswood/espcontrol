#pragma once
#include <cstdint>
#include <string>
namespace esphome::text {
struct Traits {
  uint16_t minimum{0}, maximum{255};
  uint16_t get_min_length() const { return minimum; }
  uint16_t get_max_length() const { return maximum; }
  const char *get_pattern_c_str() const { return ""; }
};
class Text {
public:
  std::string state;
  Traits traits;
  size_t controls{0}, publications{0};
  uint32_t preference_hash{0x12345678};
  uint32_t get_preference_hash() const { return preference_hash; }
  void publish_state(const std::string &value) {
    state = value;
    ++publications;
  }
  struct Call {
    Text &text;
    std::string value;
    Call &set_value(const char *data, size_t size) {
      value.assign(data, size);
      return *this;
    }
    void perform() {
      ++text.controls;
      text.publish_state(value);
    }
  };
  Call make_call() { return {*this, {}}; }
};
} // namespace esphome::text
