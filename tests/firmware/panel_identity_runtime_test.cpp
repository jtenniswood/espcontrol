#include <cassert>
#include <string>
#include "nvs.h"
#include "nvs_flash.h"
#include "panel_flash_layout.h"
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
  assert(first.target_hostname() == "kitchen-a1b2c3");
  assert(std::string(esphome::App.get_name().c_str()) == "original-panel-a1b2c3");

  // Simulate entity registration before App.setup(). Entity references must
  // retain their original backing strings after the application name changes.
  auto entity_name = esphome::App.get_friendly_name();
  espcontrol::PanelIdentity rebooted;
  rebooted.setup();
  assert(rebooted.get_setup_priority() > 800);
  assert(!rebooted.restart_required());
  assert(std::string(esphome::App.get_name().c_str()) == "kitchen-a1b2c3");
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

  // A full settings store must not prevent identity writes on deployed layouts.
  fail_open = false;
  identity_flash.clear();
  has_data_partition = true;
  shared_full = true;
  esphome::App = esphome::Application{};
  espcontrol::PanelIdentity dedicated;
  dedicated.setup();
  assert(dedicated.ready());
  assert(initialized_partition.address == 0xffc000);
  assert(initialized_partition.size == 16384);
  assert(dedicated.save("Office"));
  assert(identity_flash.empty());
  esphome::App = esphome::Application{};
  espcontrol::PanelIdentity persisted;
  persisted.setup();
  assert(persisted.saved_name() == "Office");
  assert(persisted.target_hostname() == "office-a1b2c3");
  assert(!persisted.restart_required());

  // Migrate a previously saved shared-NVS name and keep it isolated afterward.
  identity_flash = dedicated_flash;
  dedicated_flash.clear();
  esphome::App = esphome::Application{};
  espcontrol::PanelIdentity migrated;
  migrated.setup();
  assert(migrated.ready() && migrated.saved_name() == "Office");
  assert(!dedicated_flash.empty());
  assert(migrated.save(""));
  esphome::App = esphome::Application{};
  espcontrol::PanelIdentity reset_name;
  reset_name.setup();
  assert(reset_name.saved_name().empty()); // Must not revive the legacy name.
  assert(!identity_flash.empty()); // Never erase shared settings.

  fail_init = 7;
  espcontrol::PanelIdentity failed_init;
  failed_init.setup();
  assert(!failed_init.ready());
  assert(!failed_init.save("Kitchen"));
  fail_init = 0;
  data_partition.size = 8192;
  espcontrol::PanelIdentity unsupported;
  unsupported.setup();
  assert(!unsupported.ready());
  assert(espcontrol::panel_config_partition_bytes(0x200000) == 0x1fc000);
  assert(espcontrol::panel_config_partition_bytes(8192) == 0);
}
