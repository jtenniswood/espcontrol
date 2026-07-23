#pragma once

#if defined(USE_ESP32_VARIANT_ESP32P4)

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "esphome/core/component.h"

namespace esphome::c6_recovery {

class C6RecoveryComponent final : public Component {
 public:
  void setup() override;
  void dump_config() override;
  // ESP-Hosted initialises its SDIO transport before ESPHome components run.
  // Connect to that transport immediately before the WiFi component starts.
  float get_setup_priority() const override { return setup_priority::WIFI + 1.0f; }

  void set_firmware_data(const uint8_t *data) { this->firmware_data_ = data; }
  void set_firmware_size(size_t size) { this->firmware_size_ = size; }
  void set_target_version(const std::string &version) { this->target_version_ = version; }
  void set_firmware_sha256(const std::array<uint8_t, 32> &sha256) {
    this->firmware_sha256_ = sha256;
  }

 protected:
  bool should_install_(const std::string &current) const;
  bool verify_firmware_();
  bool install_firmware_();

  const uint8_t *firmware_data_{nullptr};
  size_t firmware_size_{0};
  std::string target_version_;
  std::array<uint8_t, 32> firmware_sha256_{};
};

}  // namespace esphome::c6_recovery

#endif
