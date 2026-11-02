#pragma once
#include <array>
#include <vector>
#include <utility>
using gpio_num_t = int;
constexpr int GPIO_MODE_OUTPUT = 1;
inline std::array<int, 4> levels{}, directions{};
inline std::vector<std::pair<char, int>> gpio_calls;
inline bool gpio_failure = false;
inline int gpio_set_level(int pin, int level) { gpio_calls.push_back({'L', pin}); if (gpio_failure) return 1; levels[pin] = level; return 0; }
inline int gpio_set_direction(int pin, int direction) { gpio_calls.push_back({'D', pin}); if (gpio_failure) return 1; directions[pin] = direction; return 0; }
inline int gpio_reset_pin(int pin) { gpio_calls.push_back({'R', pin}); directions[pin] = 0; return 0; }
