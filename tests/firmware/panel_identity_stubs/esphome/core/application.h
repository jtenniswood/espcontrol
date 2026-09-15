#pragma once
#include "string_ref.h"
namespace esphome {
struct Application {
  StringRef name{"original-panel-a1b2c3"};
  StringRef friendly{"Original panel a1b2c3"};
  const StringRef &get_name() const { return name; }
  const StringRef &get_friendly_name() const { return friendly; }
};
inline Application App;
}
