#include <cassert>
#include <string>
#include "panel_identity_model.h"

int main() {
  std::string name;
  assert(espcontrol::normalize_panel_name("  Kitchen \t", name) && name == "Kitchen");
  assert(espcontrol::panel_hostname(name, "b2c3") == "kitchen-b2c3");
  assert(espcontrol::normalize_panel_name("", name) && name.empty());
  assert(espcontrol::normalize_panel_name("Küche", name));
  assert(espcontrol::panel_hostname(name, "b2c3") == "k-che-b2c3");
  assert(espcontrol::panel_hostname("東京", "cdef") == "panel-cdef");
  assert(espcontrol::panel_hostname("A long hallway panel name", "cdef").size() <= 31);
  assert(espcontrol::panel_hostname("------------", "cdef") == "panel-cdef");
  assert(!espcontrol::normalize_panel_name(std::string(121, 'x'), name));
  assert(espcontrol::normalize_panel_name(std::string(120, 'x'), name));
  assert(!espcontrol::normalize_panel_name("bad\nname", name));
  assert(!espcontrol::normalize_panel_name(std::string("a\0b", 3), name));
  assert(!espcontrol::normalize_panel_name("a\xc2\x85z", name));
  assert(!espcontrol::normalize_panel_name("\xff", name));
  assert(!espcontrol::normalize_panel_name("\xed\xa0\x80", name));
}
