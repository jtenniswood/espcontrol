#!/usr/bin/env python3
"""Compile and run host-side checks for media speaker-group helpers."""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parent.parent

CPP_SOURCE = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace esphome::api {
struct ActionResponse {};
struct KeyValue { std::string key; std::string value; };
struct RepeatedKeyValue {
  std::vector<KeyValue> values;
  void init(size_t count) { values.reserve(count); }
  KeyValue &emplace_back() { values.emplace_back(); return values.back(); }
};
struct HomeassistantActionRequest {
  std::string service;
  bool is_event = false;
  uint32_t call_id = 0;
  RepeatedKeyValue data;
  RepeatedKeyValue data_template;
  RepeatedKeyValue variables;
};
}

using HomeAssistantActionResponseCallback =
  std::function<void(const esphome::api::ActionResponse &)>;

static esphome::api::HomeassistantActionRequest last_request;

inline bool ha_action_begin(esphome::api::HomeassistantActionRequest &req,
                            const char *service, bool is_event,
                            size_t data_count, uint32_t call_id = 0) {
  req.service = service ? service : "";
  req.is_event = is_event;
  req.call_id = call_id;
  req.data.init(data_count);
  return !req.service.empty();
}
inline void ha_action_add_data(esphome::api::HomeassistantActionRequest &req,
                               const char *key, const char *value) {
  auto &kv = req.data.emplace_back(); kv.key = key; kv.value = value;
}
inline void ha_action_add_data_template(esphome::api::HomeassistantActionRequest &req,
                                        const char *key, const char *value) {
  auto &kv = req.data_template.emplace_back(); kv.key = key; kv.value = value;
}
inline void ha_action_add_variable(esphome::api::HomeassistantActionRequest &req,
                                   const char *key, const char *value) {
  auto &kv = req.variables.emplace_back(); kv.key = key; kv.value = value;
}
inline void ha_action_add_entity(esphome::api::HomeassistantActionRequest &req,
                                 const std::string &entity_id) {
  ha_action_add_data(req, "entity_id", entity_id.c_str());
}
inline bool ha_register_action_response_callback(uint32_t,
                                                 HomeAssistantActionResponseCallback) {
  return true;
}
inline bool ha_action_send(esphome::api::HomeassistantActionRequest &req) {
  last_request = req;
  return true;
}
inline bool ha_cancel_action_response_callback(uint32_t, const char * = "cancelled") {
  return true;
}

#include "components/espcontrol/button_grid_media_group.h"

static std::string value_for(const esphome::api::RepeatedKeyValue &values,
                             const std::string &key) {
  for (const auto &value : values.values) if (value.key == key) return value.value;
  return "";
}

int main() {
  assert((media_group_parse_entity_list(
    "['media_player.office', 'media_player.kitchen']") ==
    std::vector<std::string>{"media_player.office", "media_player.kitchen"}));
  assert((media_group_parse_entity_list(
    "[\"media_player.office\", bad, sensor.temperature, media_player.office]") ==
    std::vector<std::string>{"media_player.office"}));
  assert(media_group_parse_entity_list("not a list").empty());
  assert((media_group_parse_discovery_data(
    "office,kitchen|Office,Kitchen|0.14,0.25") ==
    std::vector<std::string>{"media_player.office", "media_player.kitchen"}));
  assert((media_group_parse_discovery_data(
    "media_player.office, patio |Office,Patio|0.14,0.25") ==
    std::vector<std::string>{"media_player.office", "media_player.patio"}));
  assert(media_group_parse_discovery_data("unknown").empty());
  assert(media_group_parse_discovery_data("unavailable").empty());
  assert(media_group_parse_discovery_data("null").empty());
  auto discovered = media_group_parse_discovery_items(
    "office,kitchen|Office,Kitchen|0.14,0.25|Sonos,Sonos");
  assert(discovered.size() == 2);
  assert(discovered[0].entity_id == "media_player.office");
  assert(discovered[0].friendly_name == "Office");
  assert(discovered[0].volume_known && discovered[0].volume_pct == 14);
  assert(discovered[1].entity_id == "media_player.kitchen");
  assert(discovered[1].friendly_name == "Kitchen");
  assert(discovered[1].volume_known && discovered[1].volume_pct == 25);
  auto discovered_v2 = media_group_parse_discovery_items(
    "v2|[[\"media_player.office\",\"Office, Upstairs\",0.14],"
    "[\"media_player.kitchen\",\"K\\u00fcche\",null]]");
  assert(discovered_v2.size() == 2);
  assert(discovered_v2[0].entity_id == "media_player.office");
  assert(discovered_v2[0].friendly_name == "Office, Upstairs");
  assert(discovered_v2[0].volume_known && discovered_v2[0].volume_pct == 14);
  assert(discovered_v2[1].friendly_name == u8"Küche");
  assert(!discovered_v2[1].volume_known);
  assert(media_group_parse_discovery_items("v2|[[\"media_player.office\"]]").empty());
  assert(media_group_parse_discovery_items("v2|not-json").empty());
  assert(media_group_parse_discovery_items("unknown").empty());
  assert(media_group_parse_discovery_items("unavailable").empty());
  assert(media_group_parse_discovery_items("null").empty());
  assert(media_group_discovery_entity("") == "sensor.speaker_group");
  assert(media_group_discovery_entity("media_player.compatible") ==
    "media_player.compatible");
  assert(std::string(media_group_discovery_attribute("sensor.speaker_group")) == "data");
  assert(std::string(media_group_discovery_attribute("sensor.sonos_speakers")) == "data");
  assert(std::string(media_group_discovery_attribute("media_player.compatible")) == "entity_id");
  std::string long_list = "[";
  for (int i = 0; i < 12; i++) {
    if (i != 0) long_list += ",";
    long_list += "media_player.speaker_" + std::to_string(i);
  }
  long_list += "]";
  assert(long_list.size() > 96);
  assert(media_group_parse_entity_list(long_list.data(), long_list.size()).size() == 12);
  assert(!media_group_valid_entity_id("light.office"));
  assert(!media_group_valid_entity_id("media_player.bad-name"));

  assert(media_grouping_supported(524288));
  assert(media_grouping_supported(524288 | 1));
  assert(!media_grouping_supported(65536));
  assert(!media_group_discovery_available({}));
  assert(media_group_discovery_available({"media_player.office"}));
  assert(!media_group_speaker_tab_available(false, false));
  assert(!media_group_speaker_tab_available(true, false));
  assert(!media_group_speaker_tab_available(false, true));
  assert(media_group_speaker_tab_available(true, true));
  assert(!media_group_defer_volume_actions(1));
  assert(media_group_defer_volume_actions(2));
  assert(media_group_step_volume(80, false, 50) == 79);
  assert(media_group_step_volume(80, true, 50) == 80);
  assert(media_group_step_volume(49, true, 50) == 50);
  assert(media_group_step_volume(0, false, 50) == 0);
  assert((media_group_merge_candidates(
    "media_player.office",
    {"media_player.kitchen", "media_player.office"},
    {"media_player.patio", "media_player.kitchen"}) ==
    std::vector<std::string>{"media_player.office", "media_player.kitchen", "media_player.patio"}));

  std::vector<MediaGroupVolumeState> volumes = {
    {"media_player.one", 10, true, true},
    {"media_player.two", 25, true, true},
    {"media_player.three", 40, true, true},
  };
  int mean = 0;
  assert(media_group_mean_volume(volumes, &mean) && mean == 25);
  assert((media_group_delta_volumes(volumes, 35, 100) == std::vector<int>{20, 35, 50}));
  assert((media_group_delta_volumes(volumes, 100, 45) == std::vector<int>{30, 45, 45}));
  std::vector<MediaGroupVolumeState> above_max = {
    {"media_player.one", 80, true, true},
    {"media_player.two", 20, true, true},
  };
  assert(media_group_mean_volume(above_max, &mean) && mean == 50);
  assert((media_group_delta_volumes(above_max, 40, 50) == std::vector<int>{50, 10}));
  volumes[1].volume_known = false;
  assert(!media_group_mean_volume(volumes, &mean));
  assert(media_group_delta_volumes(volumes, 35, 100).empty());
  volumes[1].available = false;
  assert(media_group_mean_volume(volumes, &mean) && mean == 25);

  uint32_t call_id = 0;
  assert(send_media_group_join_action(
    "media_player.office",
    {"media_player.office", "media_player.kitchen", "media_player.patio"},
    [](const esphome::api::ActionResponse &) {}, &call_id));
  assert(call_id != 0);
  assert(last_request.service == "media_player.join");
  assert(value_for(last_request.data, "entity_id") == "media_player.office");
  assert(value_for(last_request.variables, "group_members_json") ==
    "[\"media_player.kitchen\",\"media_player.patio\"]");
  assert(value_for(last_request.data_template, "group_members") ==
    "{{ group_members_json | from_json }}");

  assert(send_media_group_unjoin_action(
    "media_player.kitchen", [](const esphome::api::ActionResponse &) {}, &call_id));
  assert(last_request.service == "media_player.unjoin");
  assert(value_for(last_request.data, "entity_id") == "media_player.kitchen");
  return 0;
}
'''


def main() -> int:
    cxx = shutil.which("c++") or shutil.which("g++") or shutil.which("clang++")
    if not cxx:
        print("No C++ compiler found; cannot run media-group firmware checks.")
        return 1
    with TemporaryDirectory() as tmp:
        source = Path(tmp) / "check_firmware_media_group.cpp"
        binary = Path(tmp) / "check_firmware_media_group"
        source.write_text(CPP_SOURCE, encoding="utf-8")
        subprocess.run(
            [cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT),
             str(source), "-o", str(binary)], check=True
        )
        subprocess.run([str(binary)], check=True)
    media_header = (ROOT / "components/espcontrol/button_grid_media.h").read_text(
        encoding="utf-8"
    )
    if "ha_subscribe_state_reusable" in media_header or "ha_subscribe_attribute_reusable" in media_header:
        raise SystemExit("Speaker modal must not install lifetime reusable subscriptions")
    if "media_group_defer_volume_actions" not in media_header:
        raise SystemExit("Grouped volume arc must defer actions until release")
    if "speaker_discovery_available" not in media_header:
        raise SystemExit("Speaker tab must track discovery availability")
    if "lv_event_get_target(event) != lv_event_get_current_target(event)" not in media_header:
        raise SystemExit("Speaker card must ignore clicks bubbled from its volume controls")
    print("Firmware media-group checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
