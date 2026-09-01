#include <cassert>
#include <cstring>
#include <string>

#include "icons.h"

int main() {
  assert(std::strcmp(find_icon("Wi-Fi Startup"), find_icon("Wifi")) == 0);
  assert(std::strcmp(find_icon("Wi-Fi"), find_icon("Wifi Setup")) == 0);
  assert(std::strcmp(find_icon("Wi-Fi Strength Outline"),
                     find_icon("Wifi Strength Outline")) == 0);
  assert(std::strcmp(find_icon("Wi-Fi Strength 1"),
                     find_icon("Wifi Strength 1")) == 0);
  assert(std::strcmp(find_icon("Wi-Fi Strength 2"),
                     find_icon("Wifi Strength 2")) == 0);
  assert(std::strcmp(find_icon("Wi-Fi Strength 3"),
                     find_icon("Wifi Strength 3")) == 0);
  assert(std::strcmp(find_icon("Wi-Fi Strength Off Outline"),
                     find_icon("Wifi Strength Off Outline")) == 0);
}
