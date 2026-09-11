#pragma once
namespace esphome {
class Component {
 public:
  virtual ~Component() = default;
  virtual void setup() {}
  virtual float get_setup_priority() const { return 0; }
};
}
