// Grouped speakers report their own volume and availability. These readings
// must work without a discovery sensor and override stale discovery values.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// Minimal stand-ins for the Home Assistant action senders that share this
// header. These tests only exercise its pure helpers.
namespace esphome::api {
struct Field { void init(int) {} };
struct HomeassistantActionRequest { Field data_template; Field variables; };
}
using HomeAssistantActionResponseCallback = int;
inline bool ha_action_begin(esphome::api::HomeassistantActionRequest &, const char *, bool,
                            int, uint32_t) { return false; }
inline void ha_action_add_entity(esphome::api::HomeassistantActionRequest &,
                                 const std::string &) {}
inline void ha_action_add_variable(esphome::api::HomeassistantActionRequest &, const char *,
                                   const char *) {}
inline void ha_action_add_data_template(esphome::api::HomeassistantActionRequest &,
                                        const char *, const char *) {}
inline bool ha_register_action_response_callback(uint32_t, int) { return false; }
inline bool ha_action_send(esphome::api::HomeassistantActionRequest &) { return false; }
inline bool ha_cancel_action_response_callback(uint32_t, const char *) { return false; }

#include "button_grid_media_group.h"

namespace {

const MediaGroupDiscoveryItem *find_item(const std::vector<MediaGroupDiscoveryItem> &items,
                                         const std::string &entity_id) {
  for (const MediaGroupDiscoveryItem &item : items) {
    if (item.entity_id == entity_id) return &item;
  }
  return nullptr;
}

MediaGroupMemberReading reading(const std::string &entity_id) {
  MediaGroupMemberReading out;
  out.entity_id = entity_id;
  return out;
}

void test_parse_volume_level() {
  int pct = -1;
  assert(media_group_parse_volume_level("0.5025000000000001", &pct) && pct == 50);
  assert(media_group_parse_volume_level(" 0.14 ", &pct) && pct == 14);
  assert(media_group_parse_volume_level("1.7", &pct) && pct == 100);
  assert(!media_group_parse_volume_level("None", &pct));
  assert(!media_group_parse_volume_level("", &pct));
  assert(!media_group_parse_volume_level("nan", &pct));
}

void test_state_available() {
  assert(media_group_state_available("playing"));
  assert(media_group_state_available(""));
  assert(media_group_state_available("unknown"));
  assert(!media_group_state_available("unavailable"));
}

void test_readings_without_discovery_sensor() {
  MediaGroupMemberReading ensuite = reading("media_player.amplipi_master_ensuite");
  ensuite.volume_reported = true;
  ensuite.volume_known = true;
  ensuite.volume_pct = 50;
  ensuite.friendly_name = "AmpliPi Master Ensuite";
  const auto items = media_group_merge_member_readings({}, {ensuite});
  const MediaGroupDiscoveryItem *item = find_item(items, ensuite.entity_id);
  assert(item);
  assert(item->volume_known && item->volume_pct == 50);
  assert(item->available);
  assert(item->friendly_name == "AmpliPi Master Ensuite");
}

void test_readings_override_stale_discovery() {
  MediaGroupDiscoveryItem stale;
  stale.entity_id = "media_player.kitchen";
  stale.friendly_name = "Kitchen";
  stale.volume_pct = 20;
  stale.volume_known = true;
  stale.available = true;
  MediaGroupMemberReading live = reading("media_player.kitchen");
  live.volume_reported = true;
  live.volume_known = true;
  live.volume_pct = 35;
  const auto items = media_group_merge_member_readings({stale}, {live});
  assert(items.size() == 1);
  assert(items[0].volume_pct == 35);
  // An empty live name keeps the discovery name.
  assert(items[0].friendly_name == "Kitchen");
}

void test_unavailable_member_and_unknown_volume() {
  MediaGroupDiscoveryItem stale;
  stale.entity_id = "media_player.patio";
  stale.volume_pct = 40;
  stale.volume_known = true;
  stale.available = true;
  MediaGroupMemberReading live = reading("media_player.patio");
  live.volume_reported = true;
  live.volume_known = false;
  live.state_known = true;
  live.available = false;
  const auto items = media_group_merge_member_readings({stale}, {live});
  assert(!items[0].volume_known);
  assert(!items[0].available);
}

void test_readings_without_data_are_ignored() {
  const auto items = media_group_merge_member_readings({}, {reading("media_player.office")});
  assert(items.empty());
}

void test_group_mean_uses_member_readings() {
  // Office is the card's own player; Master Ensuite is known only through its
  // own subscription. The group level must be available.
  std::vector<MediaGroupVolumeState> members = {
    {"media_player.amplipi_office", 14, true, true},
    {"media_player.amplipi_master_ensuite", 50, true, true},
  };
  int mean = 0;
  assert(media_group_mean_volume(members, &mean) && mean == 32);
}

}  // namespace

int main() {
  test_parse_volume_level();
  test_state_available();
  test_readings_without_discovery_sensor();
  test_readings_override_stale_discovery();
  test_unavailable_member_and_unknown_volume();
  test_readings_without_data_are_ignored();
  test_group_mean_uses_member_readings();
  return 0;
}
