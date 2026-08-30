#include <cassert>

#include "panel_config_legacy_entity_guard.h"

int main() {
  using espcontrol::configuration::PanelConfigLegacyEntityTarget;
  using espcontrol::configuration::panel_config_legacy_entity_target;
  using espcontrol::configuration::panel_config_legacy_entity_path;

  assert(panel_config_legacy_entity_path("/text/Button 1 Config"));
  assert(panel_config_legacy_entity_path("/text/Button 40 Config"));
  assert(panel_config_legacy_entity_path("/text/Subpage 3 Config"));
  assert(panel_config_legacy_entity_path("/text/Subpage 3 Config Ext"));
  assert(panel_config_legacy_entity_path("/text/Subpage 3 Config Ext 7"));
  assert(panel_config_legacy_entity_path("/text/button_1_config"));
  assert(panel_config_legacy_entity_path("/text/subpage_3_config_ext_2"));

  PanelConfigLegacyEntityTarget target;
  assert(panel_config_legacy_entity_target("/text/Button 40 Config", &target));
  assert(target.slot == 40 && target.subpage_chunk == -1);
  assert(panel_config_legacy_entity_target(
      "/text/Subpage 3 Config Ext 7", &target));
  assert(target.slot == 3 && target.subpage_chunk == 7);
  assert(panel_config_legacy_entity_target(
      "/text/subpage_3_config_ext", &target));
  assert(target.slot == 3 && target.subpage_chunk == 1);

  assert(!panel_config_legacy_entity_path("/text/Button Order"));
  assert(!panel_config_legacy_entity_path("/text/Button On Color"));
  assert(!panel_config_legacy_entity_path("/text/Button Config"));
  assert(!panel_config_legacy_entity_path("/text/Button 1 Configuration"));
  assert(!panel_config_legacy_entity_path("/text/Subpage 3 Config Ext 8"));
  assert(!panel_config_legacy_entity_path("/sensor/Button 1 Config"));
  assert(!panel_config_legacy_entity_path("/text/Button 0 Config"));
}
