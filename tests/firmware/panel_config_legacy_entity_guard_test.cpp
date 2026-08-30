#include <cassert>

#include "panel_config_legacy_entity_guard.h"

int main() {
  using espcontrol::configuration::panel_config_legacy_entity_path;

  assert(panel_config_legacy_entity_path("/text/Button 1 Config"));
  assert(panel_config_legacy_entity_path("/text/Button 40 Config"));
  assert(panel_config_legacy_entity_path("/text/Subpage 3 Config"));
  assert(panel_config_legacy_entity_path("/text/Subpage 3 Config Ext"));
  assert(panel_config_legacy_entity_path("/text/Subpage 3 Config Ext 7"));
  assert(panel_config_legacy_entity_path("/text/button_1_config"));
  assert(panel_config_legacy_entity_path("/text/subpage_3_config_ext_2"));

  assert(!panel_config_legacy_entity_path("/text/Button Order"));
  assert(!panel_config_legacy_entity_path("/text/Button On Color"));
  assert(!panel_config_legacy_entity_path("/text/Button Config"));
  assert(!panel_config_legacy_entity_path("/text/Button 1 Configuration"));
  assert(!panel_config_legacy_entity_path("/text/Subpage 3 Config Ext 8"));
  assert(!panel_config_legacy_entity_path("/sensor/Button 1 Config"));
}
