#include <cassert>
#include <string>
#include "panel_identity_model.h"

int main() {
  std::string name;
  assert(espcontrol::normalize_panel_name("  Kitchen \t", name) && name == "Kitchen");
  assert(espcontrol::panel_hostname(name, "a1b2c3") == "espcontrol-kitchen-a1b2c3");
  assert(espcontrol::normalize_panel_name("", name) && name.empty());
  assert(espcontrol::normalize_panel_name("Küche", name));
  assert(espcontrol::panel_hostname(name, "a1b2c3") == "espcontrol-k-che-a1b2c3");
  assert(espcontrol::panel_hostname("東京", "abcdef") == "espcontrol-panel-abcdef");
  assert(espcontrol::panel_hostname("A long hallway panel name", "abcdef").size() <= 31);
  assert(espcontrol::panel_hostname("------------", "abcdef") == "espcontrol-panel-abcdef");
  assert(!espcontrol::normalize_panel_name(std::string(121, 'x'), name));
  assert(espcontrol::normalize_panel_name(std::string(120, 'x'), name));
  assert(!espcontrol::normalize_panel_name("bad\nname", name));
  assert(!espcontrol::normalize_panel_name(std::string("a\0b", 3), name));
  assert(!espcontrol::normalize_panel_name("a\xc2\x85z", name));
  assert(!espcontrol::normalize_panel_name("\xff", name));
  assert(!espcontrol::normalize_panel_name("\xed\xa0\x80", name));
}
