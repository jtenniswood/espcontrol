#pragma once

#include <utility>

#include "esphome/components/network/ip_address.h"
#include "esphome/components/network/util.h"
#include "page_network_status_modal.h"
#include "screensaver_pin.h"

namespace espcontrol::pages {
// One application-owned adapter. Sampling reads ESPHome's cached local values;
// it neither queries HA nor starts a transfer/subscription or per-modal timer.
class PageLocalNetworkStatusSource final : public PageNetworkStatusSource {
 public:
  PageLocalNetworkStatusSource(std::string name, std::string version, void (&lock)(),
                               float (*wifi)() = nullptr)
      : name_(std::move(name)), version_(std::move(version)), lock_(lock), wifi_(wifi) {}
  std::string_view device_name() const override {
    return name_;
  }
  std::string_view firmware_version() const override {
    return version_;
  }
  bool wifi_available() const override {
    return wifi_ != nullptr;
  }
  bool has_pin() const override {
    return espcontrol::screensaver_pin().has_pin();
  }
  void sample(PageNetworkStatusSample& result) override {
    result = {};
    static_assert(esphome::network::IP_ADDRESS_BUFFER_SIZE <= sizeof(result.ip));
    const auto ips = esphome::network::get_ip_addresses();
    if (!ips.empty()) ips[0].str_to(result.ip.data());
    if (wifi_) result.wifi_percent = wifi_();
  }
  void activate_lock() override {
    lock_();
  }

 private:
  std::string name_, version_;
  void (&lock_)();
  float (*wifi_)();
};
}  // namespace espcontrol::pages
