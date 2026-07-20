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
  assert(media_group_discovery_entity("") == "sensor.speaker_group");
  assert(media_group_discovery_entity("media_player.compatible") ==
    "media_player.compatible");
  assert(std::string(media_group_discovery_attribute("sensor.speaker_group")) == "data");
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
    print("Firmware media-group checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
