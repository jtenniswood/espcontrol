#pragma once
#include <lwip/inet.h>
#include <cassert>

namespace esphome::network {
constexpr size_t IP_ADDRESS_BUFFER_SIZE = 48;
class IPAddress {
 public:
  IPAddress() = default;
  explicit IPAddress(const char *text) { assert(inet_pton(AF_INET, text, &this->address_.addr) == 1); }
  operator ip4_addr_t() const { return this->address_; }
  const char *str_to(char *buffer) const { return inet_ntop(AF_INET, &this->address_.addr, buffer, IP_ADDRESS_BUFFER_SIZE); }
 private:
  ip4_addr_t address_{};
};
}
