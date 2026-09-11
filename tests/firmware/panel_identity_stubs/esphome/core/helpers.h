#pragma once
#include <cstring>
namespace esphome {
constexpr size_t MAC_ADDRESS_BUFFER_SIZE = 13;
inline void get_mac_address_into_buffer(char (&buffer)[MAC_ADDRESS_BUFFER_SIZE]) { std::strcpy(buffer, "001122A1B2C3"); }
}
