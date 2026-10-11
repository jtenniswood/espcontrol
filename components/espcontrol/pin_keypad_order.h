#pragma once
#include <array>
#include <cstdint>

template<typename Random>
std::array<uint8_t, 10> scrambled_pin_digits(Random random) {
  std::array<uint8_t, 10> digits{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}};
  for (uint32_t i = 9; i > 0; --i) {
    const uint32_t j = random() % (i + 1);
    const auto value = digits[i];
    digits[i] = digits[j];
    digits[j] = value;
  }
  return digits;
}
