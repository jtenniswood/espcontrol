#pragma once

namespace espcontrol::reset {
bool begin_configuration_write();
void finish_configuration_write();

// Share the reset/OTA interlock while repairing persistent compatibility data.
// No reset intent can be accepted between a pending check and the final write.
class ConfigurationWriteLease {
public:
  ConfigurationWriteLease() : held_(begin_configuration_write()) {}
  ~ConfigurationWriteLease() { if (held_) finish_configuration_write(); }
  ConfigurationWriteLease(const ConfigurationWriteLease &) = delete;
  ConfigurationWriteLease &operator=(const ConfigurationWriteLease &) = delete;
  bool ready() const { return held_; }
private:
  bool held_;
};
} // namespace espcontrol::reset
