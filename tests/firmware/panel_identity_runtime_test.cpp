#include <cassert>
#include <string>
#include "nvs.h"
#include "esphome/core/application.h"
#include "panel_identity.h"

int main() {
  espcontrol::PanelIdentity first;
  first.setup();
  assert(first.ready());
  assert(!first.restart_required());
  assert(first.target_hostname() == "original-panel-a1b2c3");
  assert(first.save(" Kitchen "));
  assert(first.restart_required());
  assert(first.saved_name() == "Kitchen");
  assert(first.target_hostname() == "espcontrol-kitchen-a1b2c3");
  assert(std::string(esphome::App.get_name().c_str()) == "original-panel-a1b2c3");

  // Simulate entity registration before App.setup(). Entity references must
  // retain their original backing strings after the application name changes.
  auto entity_name = esphome::App.get_friendly_name();
  espcontrol::PanelIdentity rebooted;
  rebooted.setup();
  assert(rebooted.get_setup_priority() > 800);
  assert(!rebooted.restart_required());
  assert(std::string(esphome::App.get_name().c_str()) == "espcontrol-kitchen-a1b2c3");
  assert(std::string(esphome::App.get_friendly_name().c_str()) == "Kitchen");
  assert(std::string(entity_name.c_str()) == "Original panel a1b2c3");
  fail_write = true;
  assert(!rebooted.save("Office"));
  fail_write = false;
  fail_commit = true;
  assert(!rebooted.save("Office"));
  fail_commit = false;
  assert(rebooted.saved_name() == "Kitchen");
  assert(!rebooted.restart_required());
  assert(!rebooted.save("bad\nname"));
  assert(rebooted.save(""));
  assert(rebooted.restart_required());
  esphome::App = esphome::Application{};
  espcontrol::PanelIdentity cleared;
  cleared.setup();
  assert(cleared.target_hostname() == "original-panel-a1b2c3");
  assert(cleared.saved_name().empty());

  identity_flash[0] = 255;
  espcontrol::PanelIdentity corrupt;
  corrupt.setup();
  assert(corrupt.saved_name().empty());
  assert(corrupt.ready());
  fail_open = true;
  espcontrol::PanelIdentity unavailable;
  unavailable.setup();
  assert(!unavailable.ready());
  assert(!unavailable.save("Kitchen"));
}
