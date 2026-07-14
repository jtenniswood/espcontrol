#!/usr/bin/env node
"use strict";

const assert = require("assert");
const childProcess = require("child_process");
const fs = require("fs");
const os = require("os");
const path = require("path");
const { loadTypeScriptModule } = require("./load_typescript_module");

const ROOT = path.resolve(__dirname, "..");

function compiler() {
  for (const candidate of [process.env.CXX, "c++", "g++", "clang++"].filter(Boolean)) {
    if (childProcess.spawnSync(candidate, ["--version"], { stdio: "ignore" }).status === 0) return candidate;
  }
  throw new Error("No C++ compiler found for saved-config production check");
}

function checkCompiledHelper() {
  const temporary = fs.mkdtempSync(path.join(os.tmpdir(), "espcontrol-saved-config-production-"));
  try {
    const source = path.join(temporary, "saved_config_vacuum.cpp");
    const binary = path.join(temporary, "saved_config_vacuum");
    fs.writeFileSync(source, `
#include <cassert>
#include <string>
#include "button_grid_saved_config_action_generated.h"
#include "button_grid_saved_config_access_generated.h"
#include "button_grid_saved_config_security_generated.h"
#include "button_grid_saved_config_weather_generated.h"
#include "button_grid_saved_config_image_generated.h"
#include "button_grid_saved_config_climate_generated.h"
#include "button_grid_saved_config_light_control_generated.h"
#include "button_grid_saved_config_webhook_generated.h"
#include "button_grid_saved_config_subpage_generated.h"
#include "button_grid_saved_config_switch_generated.h"
#include "button_grid_saved_config_date_time_generated.h"
#include "button_grid_saved_config_fan_generated.h"
#include "button_grid_saved_config_media_generated.h"
#include "button_grid_saved_config_mower_generated.h"
#include "button_grid_saved_config_occupancy_generated.h"
#include "button_grid_saved_config_sensor_generated.h"
#include "button_grid_saved_config_static_generated.h"
#include "button_grid_saved_config_vacuum_generated.h"
struct Config {
  std::string type;
  std::string sensor;
  std::string unit;
  std::string precision;
  std::string options;
  std::string icon_on;
  std::string entity;
  std::string label;
  std::string icon;
};
int main() {
  Config local_action{"local", "stale", "unit", "2", "unknown=1", "Custom", "", "", ""};
  assert(migrate_saved_config_action_legacy(local_action));
  assert(local_action.type == "action" && local_action.sensor == "local");
  assert(local_action.unit.empty() && local_action.precision.empty() && local_action.options.empty());
  assert(local_action.icon_on == "Auto");
  Config option_select{"option_select", "stale", "unit", "2", "unknown=1", "Custom", "", "", ""};
  assert(migrate_saved_config_action_legacy(option_select));
  assert(option_select.type == "action" && option_select.sensor == "input_select.select_option");
  assert(option_select.unit.empty() && option_select.precision.empty() && option_select.options.empty());
  assert(option_select.icon_on == "Auto");
  Config regular_action{"action", "scene.turn_on", "", "", "", "Auto", "", "", ""};
  assert(!migrate_saved_config_action_legacy(regular_action));
  bool action_fields_called = false;
  bool action_options_called = false;
  regular_action.precision = "2";
  regular_action.options = "unknown=1";
  assert(normalize_saved_config_action(
    regular_action,
    [&](Config &config) {
      action_fields_called = true;
      config.precision.clear();
    },
    [&](const std::string &options, const std::string &action) {
      action_options_called = action == "scene.turn_on";
      return options + "option-hook";
    }
  ));
  assert(action_fields_called && action_options_called);
  assert(regular_action.precision.empty() && regular_action.options == "unknown=1option-hook");
  Config media{"media", "controls", "", "state", "unknown=1", "Auto", "media_player.living_room", "Media", ""};
  bool media_fields_called = false;
  bool media_options_called = false;
  assert(normalize_saved_config_media(
    media,
    [&](Config &config) {
      media_fields_called = true;
      config.sensor = "play_pause";
      config.label = "Play/Pause";
    },
    [&](const std::string &options, const std::string &mode) {
      media_options_called = mode == "play_pause";
      return options + "option-hook";
    }
  ));
  assert(media_fields_called && media_options_called);
  assert(media.sensor == "play_pause" && media.label == "Play/Pause");
  assert(media.options == "unknown=1option-hook");
  Config start{"action", "vacuum.start", "area", "2", "unknown=1", "Custom", "", "", ""};
  assert(migrate_saved_config_vacuum_legacy(start));
  assert(start.type == "vacuum" && start.sensor == "start_stop");
  assert(start.unit.empty() && start.precision.empty() && start.options.empty());
  assert(start.icon_on == "Auto");
  Config dock{"action", "vacuum.return_to_base", "area", "2", "unknown=1", "Custom", "", "", ""};
  assert(migrate_saved_config_vacuum_legacy(dock));
  assert(dock.type == "vacuum" && dock.sensor == "dock");
  Config unrelated{"action", "light.turn_on", "", "", "", "Auto", "", "", ""};
  assert(!migrate_saved_config_vacuum_legacy(unrelated));
  assert(normalize_saved_config_vacuum_sensor("dock") == "dock");
  assert(normalize_saved_config_vacuum_sensor("vacuum.start") == "start_stop");
  assert(normalize_saved_config_vacuum_sensor("vacuum.return_to_base") == "dock");
  assert(normalize_saved_config_vacuum_sensor("unknown") == "start_stop");
  assert(normalize_saved_config_vacuum_icon_on("Custom") == "Auto");
  assert(normalize_saved_config_vacuum_precision("2").empty());
  assert(normalize_saved_config_vacuum_options("").empty());
  assert(normalize_saved_config_vacuum_options("unknown=1").empty());
  Config local_sensor{"local_sensor", "stale", "unit", "7", "unknown=1", "Custom", "", "", ""};
  assert(migrate_saved_config_sensor_legacy(local_sensor));
  assert(local_sensor.type == "sensor" && local_sensor.sensor == "local");
  assert(local_sensor.icon_on == "Auto" && local_sensor.options.empty());
  Config regular_sensor{"sensor", "", "", "", "", "Auto", "", "", ""};
  assert(!migrate_saved_config_sensor_legacy(regular_sensor));
  Config text_sensor{"text_sensor", "stale", "unit", "7", "unknown=1", "Custom", "sensor.old", "Old label", ""};
  assert(migrate_saved_config_sensor_legacy(text_sensor));
  assert(text_sensor.type == "sensor" && text_sensor.precision == "text");
  assert(text_sensor.entity.empty() && text_sensor.label.empty() && text_sensor.unit.empty());
  assert(text_sensor.icon_on == "Auto");
  bool fields_called = false;
  bool options_called = false;
  assert(normalize_saved_config_sensor(
    text_sensor, true,
    [&](Config &config, bool was_legacy_text_sensor) {
      fields_called = was_legacy_text_sensor;
      if (was_legacy_text_sensor) config.sensor = "field-hook";
    },
    [&](const std::string &options, const std::string &precision) {
      options_called = precision == "text";
      return options + "option-hook";
    }
  ));
  assert(fields_called && options_called);
  assert(text_sensor.sensor == "field-hook" && text_sensor.options == "unknown=1option-hook");
  assert(!normalize_saved_config_sensor(
    unrelated, false,
    [](Config &, bool) {},
    [](const std::string &options, const std::string &) { return options; }
  ));
  Config screen_lock{"screen_lock", "stale", "unit", "2", "unknown=1", "Auto", "switch.stale", "Stale", "Security"};
  assert(normalize_saved_config_static(screen_lock));
  // Screen-lock modes reuse the sensor (lock display), unit (unlock method)
  // and entity (PIN) fields, so those are kept; label, precision and options
  // are stripped and the lock icons are forced.
  assert(screen_lock.sensor == "stale" && screen_lock.unit == "unit" && screen_lock.entity == "switch.stale");
  assert(screen_lock.label.empty() && screen_lock.precision.empty() && screen_lock.options.empty());
  assert(screen_lock.icon == "Lock" && screen_lock.icon_on == "Lock Open");
  Config light_switch{"light_switch", "stale", "unit", "2", "unknown=1", "Custom", "light.kitchen", "Kitchen", "Custom"};
  assert(normalize_saved_config_static(light_switch));
  assert(light_switch.sensor.empty() && light_switch.unit.empty());
  assert(light_switch.precision.empty() && light_switch.options.empty());
  assert(light_switch.entity == "light.kitchen" && light_switch.icon == "Custom");
  Config slider{"slider", "stale", "%", "2", "unknown=1", "Auto", "number.level", "Level", "Tune"};
  assert(normalize_saved_config_static(slider));
  assert(slider.sensor.empty() && slider.options.empty());
  assert(slider.unit == "%" && slider.precision == "2");
  Config light_temperature{"light_temperature", "sensor.temp", "K", "2", "unknown=1", "Auto", "light.kitchen", "Kitchen", "Thermometer"};
  assert(normalize_saved_config_static(light_temperature));
  assert(light_temperature.sensor == "sensor.temp" && light_temperature.unit == "K");
  assert(light_temperature.precision == "2" && light_temperature.options.empty());
  assert(!normalize_saved_config_static(unrelated));
  Config fan_control{"fan_control", "stale", "unit", "2", "fan_tabs=speed%7Cpower", "Custom", "fan.office", "Office", "Auto"};
  bool fan_fields_called = false;
  bool fan_options_called = false;
  assert(normalize_saved_config_fan(
    fan_control,
    [&](Config &config) {
      fan_fields_called = true;
      config.icon = "Fan";
      config.icon_on = "Auto";
    },
    [&](const std::string &options) {
      fan_options_called = true;
      return options + ",normalized";
    }
  ));
  assert(fan_fields_called && fan_options_called);
  assert(fan_control.sensor.empty() && fan_control.unit.empty() && fan_control.precision.empty());
  assert(fan_control.icon == "Fan" && fan_control.icon_on == "Auto");
  assert(fan_control.options == "fan_tabs=speed%7Cpower,normalized");
  Config fan_switch{"fan_switch", "stale", "unit", "2", "unknown=1", "Auto", "fan.office", "Office", "Auto"};
  assert(normalize_saved_config_fan(
    fan_switch,
    [](Config &config) { config.icon = "Fan Off"; config.icon_on = "Fan"; },
    [](const std::string &) { return std::string("unexpected"); }
  ));
  assert(fan_switch.sensor.empty() && fan_switch.unit.empty() && fan_switch.precision.empty());
  assert(fan_switch.options.empty() && fan_switch.icon == "Fan Off" && fan_switch.icon_on == "Fan");
  assert(!normalize_saved_config_fan(
    unrelated, [](Config &) {}, [](const std::string &options) { return options; }
  ));
  Config calendar{"calendar", "stale", "unit", "datetime", "large_numbers,unknown=1", "Custom", "", "Old", "Custom"};
  bool date_time_fields_called = false;
  bool date_time_options_called = false;
  assert(normalize_saved_config_date_time(
    calendar,
    [&](Config &config) {
      date_time_fields_called = true;
      if (config.entity.empty()) config.entity = "sensor.date";
    },
    [&](const std::string &options, const Config &config) {
      date_time_options_called = config.type == "calendar" && config.precision == "datetime";
      return options + ",normalized";
    }
  ));
  assert(date_time_fields_called && date_time_options_called);
  assert(calendar.entity == "sensor.date" && calendar.label.empty());
  assert(calendar.icon == "Auto" && calendar.icon_on == "Auto");
  assert(calendar.sensor.empty() && calendar.unit.empty() && calendar.precision == "datetime");
  assert(calendar.options == "large_numbers,unknown=1,normalized");
  Config clock{"clock", "stale", "unit", "2", "large_numbers", "Custom", "sensor.old", "Old", "Custom"};
  date_time_fields_called = false;
  assert(normalize_saved_config_date_time(
    clock,
    [&](Config &) { date_time_fields_called = true; },
    [](const std::string &options, const Config &) { return options; }
  ));
  assert(!date_time_fields_called);
  assert(clock.entity.empty() && clock.label.empty() && clock.sensor.empty());
  assert(clock.unit.empty() && clock.precision.empty() && clock.options == "large_numbers");
  assert(!normalize_saved_config_date_time(
    unrelated, [](Config &) {}, [](const std::string &options, const Config &) { return options; }
  ));
  Config mower{"lawn_mower", "bad_mode", "unit", "2", "unknown=1", "Custom", "lawn_mower.backyard", "Backyard", "Auto"};
  bool mower_fields_called = false;
  assert(normalize_saved_config_mower(mower, [&](Config &config) {
    mower_fields_called = true;
    config.sensor = "start_mowing";
    config.icon = "Robot Mower";
  }));
  assert(mower_fields_called);
  assert(mower.entity == "lawn_mower.backyard" && mower.label == "Backyard");
  assert(mower.sensor == "start_mowing" && mower.icon == "Robot Mower");
  assert(mower.icon_on == "Auto" && mower.unit.empty());
  assert(mower.precision.empty() && mower.options.empty());
  assert(!normalize_saved_config_mower(unrelated, [](Config &) {}));
  Config door_window{"door_window", "binary_sensor.patio", "unit", "bad", "large_numbers,active_color", "Auto", "sensor.old", "Patio", "Auto"};
  bool occupancy_fields_called = false;
  bool occupancy_options_called = false;
  assert(normalize_saved_config_occupancy(
    door_window,
    [&](Config &config) {
      occupancy_fields_called = true;
      config.precision = "door";
      config.icon = "Door";
      config.icon_on = "Door Open";
    },
    [&](const std::string &, const Config &config) {
      occupancy_options_called = config.type == "door_window";
      return std::string("active_color");
    }
  ));
  assert(occupancy_fields_called && occupancy_options_called);
  assert(door_window.entity.empty() && door_window.label == "Patio");
  assert(door_window.sensor == "binary_sensor.patio" && door_window.unit.empty());
  assert(door_window.precision == "door" && door_window.icon == "Door");
  assert(door_window.icon_on == "Door Open" && door_window.options == "active_color");
  assert(!normalize_saved_config_occupancy(
    unrelated, [](Config &) {},
    [](const std::string &options, const Config &) { return options; }
  ));
  Config cover{"cover", "bad_mode", "55", "2", "cover_tabs=position%7Ccontrols", "Custom", "cover.office", "Office", "Blinds"};
  bool access_fields_called = false;
  bool access_options_called = false;
  assert(normalize_saved_config_access(
    cover,
    [&](Config &config) {
      access_fields_called = true;
      config.sensor.clear();
      config.unit.clear();
    },
    [&](const std::string &, const Config &config) {
      access_options_called = config.type == "cover" && config.sensor.empty();
      return std::string();
    }
  ));
  assert(access_fields_called && access_options_called);
  assert(cover.entity == "cover.office" && cover.label == "Office");
  assert(cover.sensor.empty() && cover.unit.empty() && cover.precision.empty());
  assert(cover.icon == "Blinds" && cover.icon_on == "Custom" && cover.options.empty());
  Config lock{"lock", "unlock", "unit", "2", "unknown=1", "Lock Open", "lock.front", "Front", "Lock"};
  assert(normalize_saved_config_access(
    lock,
    [](Config &config) { config.icon_on = config.sensor.empty() ? "Lock Open" : "Auto"; },
    [](const std::string &, const Config &) { return std::string("unexpected"); }
  ));
  assert(lock.sensor == "unlock" && lock.icon_on == "Auto");
  assert(lock.unit.empty() && lock.precision.empty() && lock.options.empty());
  assert(!normalize_saved_config_access(
    unrelated, [](Config &) {},
    [](const std::string &options, const Config &) { return options; }
  ));
  Config alarm{"alarm", "stale", "unit", "2", "pin_arm=0,unknown=1", "Custom", "alarm_control_panel.home", "Home", "Auto"};
  bool security_fields_called = false;
  bool security_options_called = false;
  assert(normalize_saved_config_security(
    alarm,
    [&](Config &config) {
      security_fields_called = true;
      config.icon = "Security";
    },
    [&](const std::string &, const Config &config) {
      security_options_called = config.type == "alarm" && config.sensor.empty();
      return std::string("pin_arm=0");
    }
  ));
  assert(security_fields_called && security_options_called);
  assert(alarm.entity == "alarm_control_panel.home" && alarm.label == "Home");
  assert(alarm.icon == "Security" && alarm.icon_on == "Auto");
  assert(alarm.sensor.empty() && alarm.unit.empty() && alarm.precision.empty());
  assert(alarm.options == "pin_arm=0");
  Config alarm_action{"alarm_action", "bad", "unit", "2", "pin_disarm=0", "Custom", "alarm_control_panel.home", "", "Auto"};
  assert(normalize_saved_config_security(
    alarm_action,
    [](Config &config) {
      config.sensor = "away";
      config.label = "Arm Away";
      config.icon = "Shield Lock";
    },
    [](const std::string &, const Config &) { return std::string("pin_disarm=0"); }
  ));
  assert(alarm_action.sensor == "away" && alarm_action.label == "Arm Away");
  assert(alarm_action.icon == "Shield Lock" && alarm_action.icon_on == "Auto");
  assert(alarm_action.unit.empty() && alarm_action.precision.empty());
  assert(alarm_action.options == "pin_disarm=0");
  assert(!normalize_saved_config_security(
    unrelated, [](Config &) {},
    [](const std::string &options, const Config &) { return options; }
  ));
  Config legacy_weather{"weather_forecast", "stale", "unit", "2", "large_numbers,unknown=1", "Auto", "weather.home", "Weather", "Auto"};
  assert(migrate_saved_config_weather_legacy(legacy_weather));
  assert(legacy_weather.type == "weather" && legacy_weather.precision == "tomorrow");
  bool weather_fields_called = false;
  bool weather_options_called = false;
  assert(normalize_saved_config_weather(
    legacy_weather, true,
    [&](Config &config, bool was_legacy) {
      weather_fields_called = was_legacy;
      config.label.clear();
    },
    [&](const std::string &, const Config &config) {
      weather_options_called = config.sensor.empty() && config.precision == "tomorrow";
      return std::string("large_numbers");
    }
  ));
  assert(weather_fields_called && weather_options_called);
  assert(legacy_weather.entity == "weather.home" && legacy_weather.label.empty());
  assert(legacy_weather.sensor.empty() && legacy_weather.unit == "unit");
  assert(legacy_weather.precision == "tomorrow" && legacy_weather.options == "large_numbers");
  assert(!migrate_saved_config_weather_legacy(unrelated));
  assert(!normalize_saved_config_weather(
    unrelated, false, [](Config &, bool) {},
    [](const std::string &options, const Config &) { return options; }
  ));
  Config image{"image", "stale", "unit", "2", "image_label,image_icon,image_modal_mode=fit,unknown=1", "Custom", "camera.front", "Front", "Auto"};
  bool image_fields_called = false;
  bool image_options_called = false;
  assert(normalize_saved_config_image(
    image,
    [&](Config &config) {
      image_fields_called = true;
      config.icon = "Camera";
    },
    [&](const std::string &, const Config &config) {
      image_options_called = config.type == "image" && config.label == "Front";
      return std::string("image_label,image_icon,image_modal_mode=fit");
    }
  ));
  assert(image_fields_called && image_options_called);
  assert(image.entity == "camera.front" && image.label == "Front" && image.icon == "Camera");
  assert(image.icon_on == "Auto" && image.sensor.empty() && image.unit.empty());
  assert(image.precision.empty() && image.options == "image_label,image_icon,image_modal_mode=fit");
  assert(!normalize_saved_config_image(
    unrelated, [](Config &) {},
    [](const std::string &options, const Config &) { return options; }
  ));
  Config climate{"climate", "stale", "unit", "bad", "label_display=status,number_display=actual,unknown=1", "Radiator", "climate.hall", "Hall", ""};
  bool climate_fields_called = false;
  bool climate_options_called = false;
  assert(normalize_saved_config_climate(
    climate,
    [&](Config &config) {
      climate_fields_called = true;
      config.icon = "Thermostat";
      config.precision.clear();
    },
    [&](const std::string &, const Config &config) {
      climate_options_called = config.type == "climate_control" && config.sensor.empty();
      return std::string("label_display=status,number_display=actual");
    }
  ));
  assert(climate_fields_called && climate_options_called);
  assert(climate.type == "climate_control" && climate.entity == "climate.hall");
  assert(climate.label == "Hall" && climate.icon == "Thermostat" && climate.icon_on == "Radiator");
  assert(climate.sensor.empty() && climate.unit.empty() && climate.precision.empty());
  assert(climate.options == "label_display=status,number_display=actual");
  assert(!normalize_saved_config_climate(
    unrelated, [](Config &) {},
    [](const std::string &options, const Config &) { return options; }
  ));
  Config light_control{"light_control", "stale", "unit", "2", "light_tabs=brightness%7Cpower,unknown=1", "Lightbulb", "light.kitchen", "Kitchen", "Lightbulb Outline"};
  bool light_control_options_called = false;
  assert(normalize_saved_config_light_control(
    light_control,
    [&](const std::string &, const Config &config) {
      light_control_options_called = config.type == "light_control" && config.sensor.empty();
      return std::string("light_tabs=brightness%7Cpower");
    }
  ));
  assert(light_control_options_called);
  assert(light_control.entity == "light.kitchen" && light_control.label == "Kitchen");
  assert(light_control.icon == "Lightbulb Outline" && light_control.icon_on == "Lightbulb");
  assert(light_control.sensor.empty() && light_control.unit.empty() && light_control.precision.empty());
  assert(light_control.options == "light_tabs=brightness%7Cpower");
  assert(!normalize_saved_config_light_control(
    unrelated, [](const std::string &options, const Config &) { return options; }
  ));
  Config webhook{"webhook", " post ", "payload", "stale", "webhook_headers=Content-Type%3A%20application%2Fjson,unknown=1", "Custom", "http://example.local/hook", "Gate", ""};
  bool webhook_fields_called = false;
  bool webhook_options_called = false;
  assert(normalize_saved_config_webhook(
    webhook,
    [&](Config &config) {
      webhook_fields_called = true;
      config.sensor = "POST";
      config.icon = "Auto";
    },
    [&](const std::string &, const Config &config) {
      webhook_options_called = config.sensor == "POST" && config.precision.empty();
      return std::string("webhook_headers=Content-Type%3A%20application%2Fjson");
    }
  ));
  assert(webhook_fields_called && webhook_options_called);
  assert(webhook.entity == "http://example.local/hook" && webhook.label == "Gate");
  assert(webhook.icon == "Auto" && webhook.icon_on == "Auto");
  assert(webhook.sensor == "POST" && webhook.unit == "payload" && webhook.precision.empty());
  assert(webhook.options == "webhook_headers=Content-Type%3A%20application%2Fjson");
  assert(!normalize_saved_config_webhook(
    unrelated, [](Config &) {},
    [](const std::string &options, const Config &) { return options; }
  ));
  Config subpage{"subpage", "sensor.temperature", "C", "2", "subpage_kind=climate,large_numbers,unknown=1", "Custom", "", "", "Auto"};
  bool subpage_fields_called = false;
  bool subpage_options_called = false;
  assert(normalize_saved_config_subpage(
    subpage,
    [&](Config &config) {
      subpage_fields_called = true;
      config.label = "Climate";
      config.icon = "Thermostat";
      config.icon_on = "Auto";
      config.sensor = "indicator";
      config.unit.clear();
      config.precision.clear();
    },
    [&](const std::string &, const Config &config) {
      subpage_options_called = config.sensor == "indicator" && config.precision.empty();
      return std::string("subpage_kind=climate");
    }
  ));
  assert(subpage_fields_called && subpage_options_called);
  assert(subpage.label == "Climate" && subpage.icon == "Thermostat");
  assert(subpage.icon_on == "Auto" && subpage.sensor == "indicator");
  assert(subpage.unit.empty() && subpage.precision.empty());
  assert(subpage.options == "subpage_kind=climate");
  assert(!normalize_saved_config_subpage(
    unrelated, [](Config &) {},
    [](const std::string &options, const Config &) { return options; }
  ));
  Config switch_card{"", "sensor.power", "W", "1", "large_numbers,confirm_off,ignored=1", "Power", "switch.printer", "Printer", "Printer 3D"};
  bool switch_options_called = false;
  assert(normalize_saved_config_switch(
    switch_card,
    [&](const std::string &) {
      switch_options_called = true;
      return std::string("large_numbers,confirm_off");
    }
  ));
  assert(switch_options_called);
  assert(switch_card.entity == "switch.printer" && switch_card.label == "Printer");
  assert(switch_card.icon == "Printer 3D" && switch_card.icon_on == "Power");
  assert(switch_card.sensor == "sensor.power" && switch_card.unit == "W" && switch_card.precision == "1");
  assert(switch_card.options == "large_numbers,confirm_off");
  Config empty_switch{"", "", "", "", "", "", "", "", ""};
  assert(normalize_saved_config_switch(
    empty_switch, [](const std::string &options) { return options; }
  ));
  assert(empty_switch.icon == "Auto" && empty_switch.icon_on == "Auto");
  assert(!normalize_saved_config_switch(
    unrelated, [](const std::string &options) { return options; }
  ));
}
`);
    childProcess.execFileSync(compiler(), [
      "-std=c++17", "-Wall", "-Wextra", "-Werror",
      `-I${path.join(ROOT, "components/espcontrol")}`, source, "-o", binary,
    ]);
    childProcess.execFileSync(binary);
  } finally {
    fs.rmSync(temporary, { recursive: true, force: true });
  }
}

function main() {
  const contract = JSON.parse(fs.readFileSync(path.join(ROOT, "common/config/card_contract.json"), "utf8"));
  assert.deepStrictEqual(contract.cards.action.normalization.migrationActions.slice(0, 2), ["legacy_local_action", "legacy_option_select"]);
  const generatedAction = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_action.ts"));
  const localAction = { type: "local", sensor: "stale", unit: "unit", precision: "2", options: "unknown=1", icon_on: "Custom" };
  assert.strictEqual(generatedAction.migrateSavedConfigActionLegacy(localAction), true);
  assert.deepStrictEqual(localAction, { type: "action", sensor: "local", unit: "", precision: "", options: "", icon_on: "Auto" });
  const optionSelect = { type: "option_select", sensor: "stale", unit: "unit", precision: "2", options: "unknown=1", icon_on: "Custom" };
  assert.strictEqual(generatedAction.migrateSavedConfigActionLegacy(optionSelect), true);
  assert.deepStrictEqual(optionSelect, { type: "action", sensor: "input_select.select_option", unit: "", precision: "", options: "", icon_on: "Auto" });
  assert.strictEqual(generatedAction.migrateSavedConfigActionLegacy({ type: "action", sensor: "scene.turn_on" }), false);
  const normalizedAction = { type: "action", sensor: "scene.turn_on", precision: "2", options: "unknown=1" };
  let actionFieldsCalled = false;
  let actionOptionsCalled = false;
  assert.strictEqual(generatedAction.normalizeSavedConfigAction(
    normalizedAction,
    (config) => {
      actionFieldsCalled = true;
      config.precision = "";
    },
    (options, action) => {
      actionOptionsCalled = action === "scene.turn_on";
      return options + "option-hook";
    },
  ), true);
  assert(actionFieldsCalled && actionOptionsCalled);
  assert.deepStrictEqual(normalizedAction, { type: "action", sensor: "scene.turn_on", precision: "", options: "unknown=1option-hook" });
  const generatedMedia = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_media.ts"));
  const normalizedMedia = { type: "media", sensor: "controls", label: "Media", options: "unknown=1" };
  let mediaFieldsCalled = false;
  let mediaOptionsCalled = false;
  assert.strictEqual(generatedMedia.normalizeSavedConfigMedia(
    normalizedMedia,
    (config) => {
      mediaFieldsCalled = true;
      config.sensor = "play_pause";
      config.label = "Play/Pause";
    },
    (options, mode) => {
      mediaOptionsCalled = mode === "play_pause";
      return options + "option-hook";
    },
  ), true);
  assert(mediaFieldsCalled && mediaOptionsCalled);
  assert.deepStrictEqual(normalizedMedia, { type: "media", sensor: "play_pause", label: "Play/Pause", options: "unknown=1option-hook" });
  assert.strictEqual(generatedMedia.normalizeSavedConfigMedia(
    { type: "sensor", options: "keep", sensor: "" }, () => {}, (options) => options,
  ), false);
  const fields = contract.cards.vacuum.normalization.fields;
  assert.strictEqual(fields.sensor.policy, "allowed");
  assert.strictEqual(fields.sensor.fallback, "start_stop");
  assert.deepStrictEqual(fields.sensor.aliases, { "vacuum.start": "start_stop", "vacuum.return_to_base": "dock" });
  assert.strictEqual(fields.icon_on.policy, "default");
  assert.strictEqual(fields.icon_on.value, "Auto");
  assert.strictEqual(fields.precision.policy, "clear");
  assert.strictEqual(fields.options.policy, "clear");
  assert.deepStrictEqual(contract.cards.vacuum.normalization.migrationActions, ["legacy_vacuum_start", "legacy_vacuum_dock"]);

  const generated = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_vacuum.ts"));
  const start = { type: "action", sensor: "vacuum.start", unit: "area", precision: "2", options: "unknown=1", icon_on: "Custom" };
  assert.strictEqual(generated.migrateSavedConfigVacuumLegacy(start), true);
  assert.deepStrictEqual(start, { type: "vacuum", sensor: "start_stop", unit: "", precision: "", options: "", icon_on: "Auto" });
  const dock = { type: "action", sensor: "vacuum.return_to_base", unit: "area", precision: "2", options: "unknown=1", icon_on: "Custom" };
  assert.strictEqual(generated.migrateSavedConfigVacuumLegacy(dock), true);
  assert.strictEqual(dock.type, "vacuum");
  assert.strictEqual(dock.sensor, "dock");
  assert.strictEqual(generated.migrateSavedConfigVacuumLegacy({ type: "action", sensor: "light.turn_on" }), false);
  assert.strictEqual(generated.normalizeSavedConfigVacuumSensor("dock"), "dock");
  assert.strictEqual(generated.normalizeSavedConfigVacuumSensor("vacuum.start"), "start_stop");
  assert.strictEqual(generated.normalizeSavedConfigVacuumSensor("vacuum.return_to_base"), "dock");
  assert.strictEqual(generated.normalizeSavedConfigVacuumSensor("unknown"), "start_stop");
  assert.strictEqual(generated.normalizeSavedConfigVacuumIconOn("Custom"), "Auto");
  assert.strictEqual(generated.normalizeSavedConfigVacuumPrecision("2"), "");
  assert.strictEqual(generated.normalizeSavedConfigVacuumOptions(""), "");
  assert.strictEqual(generated.normalizeSavedConfigVacuumOptions("unknown=1"), "");

  assert.deepStrictEqual(contract.cards.sensor.normalization.migrationActions.slice(0, 2), ["legacy_local_sensor", "legacy_text_sensor"]);
  const generatedSensor = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_sensor.ts"));
  const localSensor = { type: "local_sensor", sensor: "stale", precision: "7", options: "unknown=1", icon_on: "Custom" };
  assert.strictEqual(generatedSensor.migrateSavedConfigSensorLegacy(localSensor), true);
  assert.deepStrictEqual(localSensor, { type: "sensor", sensor: "local", precision: "7", options: "", icon_on: "Auto" });
  assert.strictEqual(generatedSensor.migrateSavedConfigSensorLegacy({ type: "sensor", sensor: "local" }), false);
  const textSensor = { type: "text_sensor", entity: "sensor.old", label: "Old label", unit: "°C", precision: "7", icon_on: "Custom" };
  assert.strictEqual(generatedSensor.migrateSavedConfigSensorLegacy(textSensor), true);
  assert.deepStrictEqual(textSensor, { type: "sensor", entity: "", label: "", unit: "", precision: "text", icon_on: "Auto" });
  let sensorFieldsCalled = false;
  let sensorOptionsCalled = false;
  assert.strictEqual(generatedSensor.normalizeSavedConfigSensor(
    textSensor,
    true,
    (config, wasLegacyTextSensor) => {
      sensorFieldsCalled = wasLegacyTextSensor;
      config.sensor = "field-hook";
    },
    (options, precision) => {
      sensorOptionsCalled = precision === "text";
      return options + "option-hook";
    },
  ), true);
  assert(sensorFieldsCalled && sensorOptionsCalled);
  assert.strictEqual(textSensor.sensor, "field-hook");
  assert.strictEqual(textSensor.options, "option-hook");
  assert.strictEqual(generatedSensor.normalizeSavedConfigSensor(
    { type: "action", options: "keep", precision: "" }, false, () => {}, (options) => options,
  ), false);

  const generatedStatic = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_static.ts"));
  const screenLock = {
    type: "screen_lock", entity: "switch.stale", label: "Stale", icon: "Security", icon_on: "Auto",
    sensor: "stale", unit: "unit", precision: "2", options: "unknown=1",
  };
  assert.strictEqual(generatedStatic.normalizeSavedConfigStatic(screenLock), true);
  assert.deepStrictEqual(screenLock, {
    // Modes reuse entity (PIN), sensor (lock display) and unit (unlock method).
    type: "screen_lock", entity: "switch.stale", label: "", icon: "Lock", icon_on: "Lock Open",
    sensor: "stale", unit: "unit", precision: "", options: "",
  });
  const internal = { type: "internal", entity: "relay_1", sensor: "push", options: "unknown=1" };
  assert.strictEqual(generatedStatic.normalizeSavedConfigStatic(internal), true);
  assert.deepStrictEqual(internal, { type: "internal", entity: "relay_1", sensor: "push", options: "" });
  const slider = { type: "slider", sensor: "stale", unit: "%", precision: "2", options: "unknown=1" };
  assert.strictEqual(generatedStatic.normalizeSavedConfigStatic(slider), true);
  assert.deepStrictEqual(slider, { type: "slider", sensor: "", unit: "%", precision: "2", options: "" });
  const lightTemperature = { type: "light_temperature", sensor: "sensor.temp", unit: "K", precision: "2", options: "unknown=1" };
  assert.strictEqual(generatedStatic.normalizeSavedConfigStatic(lightTemperature), true);
  assert.deepStrictEqual(lightTemperature, { type: "light_temperature", sensor: "sensor.temp", unit: "K", precision: "2", options: "" });
  assert.strictEqual(generatedStatic.normalizeSavedConfigStatic({ type: "sensor", options: "keep" }), false);

  const generatedFan = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_fan.ts"));
  const fanControl = { type: "fan_control", sensor: "stale", unit: "unit", precision: "2", icon: "Auto", icon_on: "Custom", options: "fan_tabs=speed%7Cpower" };
  let fanFieldsCalled = false;
  let fanOptionsCalled = false;
  assert.strictEqual(generatedFan.normalizeSavedConfigFan(
    fanControl,
    (config) => { fanFieldsCalled = true; config.icon = "Fan"; config.icon_on = "Auto"; },
    (options) => { fanOptionsCalled = true; return options + ",normalized"; },
  ), true);
  assert(fanFieldsCalled && fanOptionsCalled);
  assert.deepStrictEqual(fanControl, { type: "fan_control", sensor: "", unit: "", precision: "", icon: "Fan", icon_on: "Auto", options: "fan_tabs=speed%7Cpower,normalized" });
  const fanSwitch = { type: "fan_switch", sensor: "stale", unit: "unit", precision: "2", icon: "Auto", icon_on: "Auto", options: "unknown=1" };
  assert.strictEqual(generatedFan.normalizeSavedConfigFan(
    fanSwitch,
    (config) => { config.icon = "Fan Off"; config.icon_on = "Fan"; },
    () => "unexpected",
  ), true);
  assert.deepStrictEqual(fanSwitch, { type: "fan_switch", sensor: "", unit: "", precision: "", icon: "Fan Off", icon_on: "Fan", options: "" });
  assert.strictEqual(generatedFan.normalizeSavedConfigFan({ type: "sensor", options: "keep" }, () => {}, (options) => options), false);

  const generatedDateTime = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_date_time.ts"));
  const calendar = { type: "calendar", entity: "", label: "Old", icon: "Custom", icon_on: "Custom", sensor: "stale", unit: "unit", precision: "datetime", options: "large_numbers,unknown=1" };
  let dateTimeFieldsCalled = false;
  let dateTimeOptionsCalled = false;
  assert.strictEqual(generatedDateTime.normalizeSavedConfigDateTime(
    calendar,
    (config) => { dateTimeFieldsCalled = true; if (!config.entity) config.entity = "sensor.date"; },
    (options, config) => { dateTimeOptionsCalled = config.type === "calendar" && config.precision === "datetime"; return options + ",normalized"; },
  ), true);
  assert(dateTimeFieldsCalled && dateTimeOptionsCalled);
  assert.deepStrictEqual(calendar, { type: "calendar", entity: "sensor.date", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", precision: "datetime", options: "large_numbers,unknown=1,normalized" });
  const clock = { type: "clock", entity: "sensor.old", label: "Old", icon: "Custom", icon_on: "Custom", sensor: "stale", unit: "unit", precision: "2", options: "large_numbers" };
  dateTimeFieldsCalled = false;
  assert.strictEqual(generatedDateTime.normalizeSavedConfigDateTime(
    clock, () => { dateTimeFieldsCalled = true; }, (options) => options,
  ), true);
  assert.strictEqual(dateTimeFieldsCalled, false);
  assert.deepStrictEqual(clock, { type: "clock", entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", precision: "", options: "large_numbers" });
  assert.strictEqual(generatedDateTime.normalizeSavedConfigDateTime({ type: "sensor", options: "keep" }, () => {}, (options) => options), false);

  const generatedMower = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_mower.ts"));
  const mower = { type: "lawn_mower", entity: "lawn_mower.backyard", label: "Backyard", icon: "Auto", icon_on: "Custom", sensor: "bad_mode", unit: "unit", precision: "2", options: "unknown=1" };
  let mowerFieldsCalled = false;
  assert.strictEqual(generatedMower.normalizeSavedConfigMower(mower, (config) => {
    mowerFieldsCalled = true;
    config.sensor = "start_mowing";
    config.icon = "Robot Mower";
  }), true);
  assert.strictEqual(mowerFieldsCalled, true);
  assert.deepStrictEqual(mower, { type: "lawn_mower", entity: "lawn_mower.backyard", label: "Backyard", icon: "Robot Mower", icon_on: "Auto", sensor: "start_mowing", unit: "", precision: "", options: "" });
  assert.strictEqual(generatedMower.normalizeSavedConfigMower({ type: "sensor", options: "keep" }, () => {}), false);

  const generatedOccupancy = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_occupancy.ts"));
  const presence = { type: "presence", entity: "sensor.old", label: "Living Room", icon: "Auto", icon_on: "Auto", sensor: "binary_sensor.presence", unit: "unit", precision: "2", options: "large_numbers,active_color" };
  let occupancyFieldsCalled = false;
  let occupancyOptionsCalled = false;
  assert.strictEqual(generatedOccupancy.normalizeSavedConfigOccupancy(
    presence,
    (config) => { occupancyFieldsCalled = true; config.icon = "Motion Sensor Off"; config.icon_on = "Motion Sensor"; },
    (_options, config) => { occupancyOptionsCalled = config.type === "presence"; return "active_color"; },
  ), true);
  assert(occupancyFieldsCalled && occupancyOptionsCalled);
  assert.deepStrictEqual(presence, { type: "presence", entity: "", label: "Living Room", icon: "Motion Sensor Off", icon_on: "Motion Sensor", sensor: "binary_sensor.presence", unit: "", precision: "", options: "active_color" });
  assert.strictEqual(generatedOccupancy.normalizeSavedConfigOccupancy({ type: "sensor", options: "keep" }, () => {}, (options) => options), false);

  const generatedAccess = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_access.ts"));
  const gate = { type: "gate", entity: "cover.gate", label: "Driveway", icon: "Gate", icon_on: "Gate Open", sensor: "bad_mode", unit: "unit", precision: "2", options: "label_display=status,unknown=1" };
  let accessFieldsCalled = false;
  let accessOptionsCalled = false;
  assert.strictEqual(generatedAccess.normalizeSavedConfigAccess(
    gate,
    (config) => { accessFieldsCalled = true; config.sensor = ""; },
    (_options, config) => { accessOptionsCalled = config.type === "gate" && config.sensor === ""; return "label_display=status"; },
  ), true);
  assert(accessFieldsCalled && accessOptionsCalled);
  assert.deepStrictEqual(gate, { type: "gate", entity: "cover.gate", label: "Driveway", icon: "Gate", icon_on: "Gate Open", sensor: "", unit: "", precision: "", options: "label_display=status" });
  assert.strictEqual(generatedAccess.normalizeSavedConfigAccess({ type: "sensor", options: "keep" }, () => {}, (options) => options), false);

  const generatedSecurity = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_security.ts"));
  const alarmAction = { type: "alarm_action", entity: "alarm_control_panel.home", label: "", icon: "Security", icon_on: "Custom", sensor: "bad", unit: "unit", precision: "2", options: "pin_arm=0,unknown=1" };
  let securityFieldsCalled = false;
  let securityOptionsCalled = false;
  assert.strictEqual(generatedSecurity.normalizeSavedConfigSecurity(
    alarmAction,
    (config) => { securityFieldsCalled = true; config.sensor = "away"; config.label = "Arm Away"; config.icon = "Shield Lock"; },
    (_options, config) => { securityOptionsCalled = config.type === "alarm_action" && config.sensor === "away"; return "pin_arm=0"; },
  ), true);
  assert(securityFieldsCalled && securityOptionsCalled);
  assert.deepStrictEqual(alarmAction, { type: "alarm_action", entity: "alarm_control_panel.home", label: "Arm Away", icon: "Shield Lock", icon_on: "Auto", sensor: "away", unit: "", precision: "", options: "pin_arm=0" });
  assert.strictEqual(generatedSecurity.normalizeSavedConfigSecurity({ type: "sensor", options: "keep" }, () => {}, (options) => options), false);

  const generatedWeather = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_weather.ts"));
  const legacyWeather = { type: "weather_forecast", entity: "weather.home", label: "Weather", icon: "Auto", icon_on: "Auto", sensor: "stale", unit: "unit", precision: "bad", options: "large_numbers,unknown=1" };
  assert.strictEqual(generatedWeather.migrateSavedConfigWeatherLegacy(legacyWeather), true);
  let weatherFieldsCalled = false;
  let weatherOptionsCalled = false;
  assert.strictEqual(generatedWeather.normalizeSavedConfigWeather(
    legacyWeather, true,
    (config, wasLegacy) => { weatherFieldsCalled = wasLegacy; config.label = ""; },
    (_options, config) => { weatherOptionsCalled = config.sensor === "" && config.precision === "tomorrow"; return "large_numbers"; },
  ), true);
  assert(weatherFieldsCalled && weatherOptionsCalled);
  assert.deepStrictEqual(legacyWeather, { type: "weather", entity: "weather.home", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "unit", precision: "tomorrow", options: "large_numbers" });
  assert.strictEqual(generatedWeather.migrateSavedConfigWeatherLegacy({ type: "sensor" }), false);
  assert.strictEqual(generatedWeather.normalizeSavedConfigWeather({ type: "sensor" }, false, () => {}, (options) => options), false);

  const generatedImage = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_image.ts"));
  const imageCard = { type: "image", entity: "camera.front", label: "Front", icon: "Auto", icon_on: "Custom", sensor: "stale", unit: "unit", precision: "2", options: "image_label,image_icon,image_modal_mode=fit,unknown=1" };
  let imageFieldsCalled = false;
  let imageOptionsCalled = false;
  assert.strictEqual(generatedImage.normalizeSavedConfigImage(
    imageCard,
    (config) => { imageFieldsCalled = true; config.icon = "Camera"; },
    (_options, config) => { imageOptionsCalled = config.type === "image" && config.label === "Front"; return "image_label,image_icon,image_modal_mode=fit"; },
  ), true);
  assert(imageFieldsCalled && imageOptionsCalled);
  assert.deepStrictEqual(imageCard, { type: "image", entity: "camera.front", label: "Front", icon: "Camera", icon_on: "Auto", sensor: "", unit: "", precision: "", options: "image_label,image_icon,image_modal_mode=fit" });
  assert.strictEqual(generatedImage.normalizeSavedConfigImage({ type: "sensor" }, () => {}, (options) => options), false);

  const generatedClimate = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_climate.ts"));
  const climateCard = { type: "climate", entity: "climate.hall", label: "Hall", icon: "", icon_on: "Radiator", sensor: "stale", unit: "unit", precision: "bad", options: "label_display=status,number_display=actual,unknown=1" };
  let climateFieldsCalled = false;
  let climateOptionsCalled = false;
  assert.strictEqual(generatedClimate.normalizeSavedConfigClimate(
    climateCard,
    (config) => { climateFieldsCalled = true; config.icon = "Thermostat"; config.precision = ""; },
    (_options, config) => { climateOptionsCalled = config.type === "climate_control" && config.sensor === ""; return "label_display=status,number_display=actual"; },
  ), true);
  assert(climateFieldsCalled && climateOptionsCalled);
  assert.deepStrictEqual(climateCard, { type: "climate_control", entity: "climate.hall", label: "Hall", icon: "Thermostat", icon_on: "Radiator", sensor: "", unit: "", precision: "", options: "label_display=status,number_display=actual" });
  assert.strictEqual(generatedClimate.normalizeSavedConfigClimate({ type: "sensor" }, () => {}, (options) => options), false);

  const generatedLightControl = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_light_control.ts"));
  const lightControlCard = { type: "light_control", entity: "light.kitchen", label: "Kitchen", icon: "Lightbulb Outline", icon_on: "Lightbulb", sensor: "stale", unit: "unit", precision: "2", options: "light_tabs=brightness%7Cpower,unknown=1" };
  let lightControlOptionsCalled = false;
  assert.strictEqual(generatedLightControl.normalizeSavedConfigLightControl(
    lightControlCard,
    (_options, config) => { lightControlOptionsCalled = config.type === "light_control" && config.sensor === ""; return "light_tabs=brightness%7Cpower"; },
  ), true);
  assert(lightControlOptionsCalled);
  assert.deepStrictEqual(lightControlCard, { type: "light_control", entity: "light.kitchen", label: "Kitchen", icon: "Lightbulb Outline", icon_on: "Lightbulb", sensor: "", unit: "", precision: "", options: "light_tabs=brightness%7Cpower" });
  assert.strictEqual(generatedLightControl.normalizeSavedConfigLightControl({ type: "sensor" }, (options) => options), false);

  const generatedWebhook = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_webhook.ts"));
  const webhookCard = { type: "webhook", entity: "http://example.local/hook", label: "Gate", icon: "", icon_on: "Custom", sensor: " post ", unit: "payload", precision: "stale", options: "webhook_headers=Content-Type%3A%20application%2Fjson,unknown=1" };
  let webhookFieldsCalled = false;
  let webhookOptionsCalled = false;
  assert.strictEqual(generatedWebhook.normalizeSavedConfigWebhook(
    webhookCard,
    (config) => { webhookFieldsCalled = true; config.sensor = "POST"; config.icon = "Auto"; },
    (_options, config) => { webhookOptionsCalled = config.sensor === "POST" && config.precision === ""; return "webhook_headers=Content-Type%3A%20application%2Fjson"; },
  ), true);
  assert(webhookFieldsCalled && webhookOptionsCalled);
  assert.deepStrictEqual(webhookCard, { type: "webhook", entity: "http://example.local/hook", label: "Gate", icon: "Auto", icon_on: "Auto", sensor: "POST", unit: "payload", precision: "", options: "webhook_headers=Content-Type%3A%20application%2Fjson" });
  assert.strictEqual(generatedWebhook.normalizeSavedConfigWebhook({ type: "sensor" }, () => {}, (options) => options), false);

  const generatedSubpage = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_subpage.ts"));
  const subpageCard = { type: "subpage", entity: "", label: "", icon: "Auto", icon_on: "Custom", sensor: "sensor.temperature", unit: "C", precision: "2", options: "subpage_kind=climate,large_numbers,unknown=1" };
  let subpageFieldsCalled = false;
  let subpageOptionsCalled = false;
  assert.strictEqual(generatedSubpage.normalizeSavedConfigSubpage(
    subpageCard,
    (config) => { subpageFieldsCalled = true; config.label = "Climate"; config.icon = "Thermostat"; config.icon_on = "Auto"; config.sensor = "indicator"; config.unit = ""; config.precision = ""; },
    (_options, config) => { subpageOptionsCalled = config.sensor === "indicator" && config.precision === ""; return "subpage_kind=climate"; },
  ), true);
  assert(subpageFieldsCalled && subpageOptionsCalled);
  assert.deepStrictEqual(subpageCard, { type: "subpage", entity: "", label: "Climate", icon: "Thermostat", icon_on: "Auto", sensor: "indicator", unit: "", precision: "", options: "subpage_kind=climate" });
  assert.strictEqual(generatedSubpage.normalizeSavedConfigSubpage({ type: "sensor" }, () => {}, (options) => options), false);

  const generatedSwitch = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_switch.ts"));
  const switchCard = { type: "", entity: "switch.printer", label: "Printer", icon: "Printer 3D", icon_on: "Power", sensor: "sensor.power", unit: "W", precision: "1", options: "large_numbers,confirm_off,ignored=1" };
  let switchOptionsCalled = false;
  assert.strictEqual(generatedSwitch.normalizeSavedConfigSwitch(
    switchCard,
    () => { switchOptionsCalled = true; return "large_numbers,confirm_off"; },
  ), true);
  assert(switchOptionsCalled);
  assert.deepStrictEqual(switchCard, { type: "", entity: "switch.printer", label: "Printer", icon: "Printer 3D", icon_on: "Power", sensor: "sensor.power", unit: "W", precision: "1", options: "large_numbers,confirm_off" });
  const emptySwitch = { type: "", icon: "", icon_on: "", options: "" };
  assert.strictEqual(generatedSwitch.normalizeSavedConfigSwitch(emptySwitch, (options) => options), true);
  assert.deepStrictEqual(emptySwitch, { type: "", icon: "Auto", icon_on: "Auto", options: "" });
  assert.strictEqual(generatedSwitch.normalizeSavedConfigSwitch({ type: "sensor" }, (options) => options), false);

  const browser = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_codec.ts"), "utf8");
  assert.match(browser, /from "\.\.\/generated\/saved_config_vacuum";/);
  assert.match(browser, /migrateSavedConfigVacuumLegacy\(b\)/);
  assert.doesNotMatch(browser, /b\.type === "action" && b\.sensor === "vacuum\.(?:start|return_to_base)"/);
  assert.match(browser, /sensor = normalizeSavedConfigVacuumSensor\(sensor\);/);
  assert.match(browser, /precision = normalizeSavedConfigVacuumPrecision\(precision\);/);
  assert.match(browser, /iconOn = normalizeSavedConfigVacuumIconOn\(iconOn\);/);
  assert.match(browser, /type === "vacuum"[\s\S]*?normalizeSavedConfigVacuumOptions\(options\)/);
  assert.doesNotMatch(browser, /type === "vacuum" \|\| type === "lawn_mower"/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_sensor";/);
  assert.match(browser, /migrateSavedConfigSensorLegacy\(b\)/);
  assert.match(browser, /normalizeSavedConfigSensor\(b, wasLegacyTextSensor, normalizeSavedConfigSensorFields, normalizeSensorOptions\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "local_sensor"\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "text_sensor"\)/);
  assert.doesNotMatch(browser, /else if \(b && b\.type === "sensor"\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_action";/);
  assert.match(browser, /migrateSavedConfigActionLegacy\(b\)/);
  assert.match(browser, /normalizeSavedConfigAction\(b, normalizeSavedConfigActionFields, normalizeActionOptions\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "local"\) \{\s*b\.type = "action"/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "option_select"\) \{\s*b\.type = "action"/);
  assert.doesNotMatch(browser, /else if \(b && b\.type === "action"\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_media";/);
  assert.match(browser, /normalizeSavedConfigMedia\(b, normalizeSavedConfigMediaFields, normalizeMediaOptions\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "media"\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_static";/);
  assert.match(browser, /normalizeSavedConfigStatic\(b\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "screen_lock"\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "light_switch"\)/);
  assert.doesNotMatch(browser, /if \(b && isBrightnessSliderType\(b\.type\) && b\.sensor\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_fan";/);
  assert.match(browser, /normalizeSavedConfigFan\(b, normalizeSavedConfigFanFields, normalizeFanControlOptions\)/);
  assert.doesNotMatch(browser, /if \(b && isFanCardType\(b\.type\)\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "fan_control"\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_date_time";/);
  assert.match(browser, /normalizeSavedConfigDateTime\(b, normalizeSavedConfigDateTimeFields, normalizeSavedConfigDateTimeOptions\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "calendar"\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "clock"\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "timezone"\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_mower";/);
  assert.match(browser, /normalizeSavedConfigMower\(b, normalizeSavedConfigMowerFields\)/);
  assert.match(browser, /b\.type === "action" \|\| b\.type === "lawn_mower"/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_occupancy";/);
  assert.match(browser, /normalizeSavedConfigOccupancy\(b, normalizeSavedConfigOccupancyFields, normalizeSavedConfigOccupancyOptions\)/);
  assert.doesNotMatch(browser, /else if \(b && b\.type === "door_window"\)/);
  assert.doesNotMatch(browser, /else if \(b && b\.type === "presence"\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_access";/);
  assert.match(browser, /normalizeSavedConfigAccess\(b, normalizeSavedConfigAccessFields, normalizeSavedConfigAccessOptions\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "garage"\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "gate"\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "cover"\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "lock"\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_security";/);
  assert.match(browser, /normalizeSavedConfigSecurity\(b, normalizeSavedConfigSecurityFields, normalizeSavedConfigSecurityOptions\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "alarm"\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "alarm_action"\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_weather";/);
  assert.match(browser, /migrateSavedConfigWeatherLegacy\(b\)/);
  assert.match(browser, /normalizeSavedConfigWeather\(b, wasLegacyWeatherForecast,/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "weather_forecast"\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "weather"\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_image";/);
  assert.match(browser, /normalizeSavedConfigImage\(b, normalizeSavedConfigImageFields, normalizeSavedConfigImageOptions\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "image"\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_climate";/);
  assert.match(browser, /normalizeSavedConfigClimate\(b, normalizeSavedConfigClimateFields, normalizeSavedConfigClimateOptions\)/);
  assert.doesNotMatch(browser, /if \(b && isClimateCardType\(b\.type\)\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_light_control";/);
  assert.match(browser, /normalizeSavedConfigLightControl\(b, normalizeSavedConfigLightControlOptions\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "light_control"\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_webhook";/);
  assert.match(browser, /normalizeSavedConfigWebhook\(b, normalizeSavedConfigWebhookFields, normalizeSavedConfigWebhookOptions\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "webhook"\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_subpage";/);
  assert.match(browser, /normalizeSavedConfigSubpage\(b, normalizeSavedConfigSubpageFields, normalizeSavedConfigSubpageOptions\)/);
  assert.doesNotMatch(browser, /if \(b && b\.type === "subpage"\)/);
  assert.match(browser, /from "\.\.\/generated\/saved_config_switch";/);
  assert.match(browser, /normalizeSavedConfigSwitch\(b, normalizeSwitchConfirmationOptions\)/);
  assert.doesNotMatch(browser, /if \(b && !normalizedSavedSensor && !b\.type\)/);

  const vacuumCard = fs.readFileSync(path.join(ROOT, "src/webserver/cards/vacuum.ts"), "utf8");
  assert.match(vacuumCard, /normalizeSavedConfigVacuumSensor\(String\(b\.sensor \|\| ""\)\)/);
  assert.match(vacuumCard, /normalizeSavedConfigVacuumPrecision\(String\(b\.precision \|\| ""\)\)/);
  assert.match(vacuumCard, /normalizeSavedConfigVacuumOptions\(String\(b\.options \|\| ""\)\)/);
  assert.match(vacuumCard, /normalizeSavedConfigVacuumIconOn\(String\(b\.icon_on \|\| ""\)\)/);
  assert.doesNotMatch(vacuumCard, /normalizeEntityModeCardConfig\(b,/);

  const firmware = fs.readFileSync(path.join(ROOT, "components/espcontrol/button_grid_config_parser.h"), "utf8");
  assert.match(firmware, /#include "button_grid_saved_config_vacuum_generated\.h"/);
  const vacuumStart = firmware.indexOf('if (p.type == "vacuum")');
  const vacuumEnd = firmware.indexOf('const bool normalized_saved_mower', vacuumStart);
  assert(vacuumStart >= 0 && vacuumEnd > vacuumStart, "Vacuum production normalization block not found");
  const vacuumBlock = firmware.slice(vacuumStart, vacuumEnd);
  assert.match(firmware, /migrate_saved_config_vacuum_legacy\(p\)/);
  assert.doesNotMatch(firmware, /p\.type == "action" && p\.sensor == "vacuum\.(?:start|return_to_base)"/);
  assert.match(vacuumBlock, /p\.sensor = normalize_saved_config_vacuum_sensor\(p\.sensor\);/);
  assert.match(vacuumBlock, /p\.precision = normalize_saved_config_vacuum_precision\(p\.precision\);/);
  assert.match(vacuumBlock, /p\.icon_on = normalize_saved_config_vacuum_icon_on\(p\.icon_on\);/);
  assert.match(vacuumBlock, /p\.options = normalize_saved_config_vacuum_options\(p\.options\);/);
  assert.doesNotMatch(vacuumBlock, /p\.options\.clear\(\);/);
  assert.match(firmware, /#include "button_grid_saved_config_sensor_generated\.h"/);
  assert.match(firmware, /migrate_saved_config_sensor_legacy\(p\)/);
  assert.match(firmware, /normalize_saved_config_sensor\(p, was_legacy_text_sensor,/);
  assert.doesNotMatch(firmware, /p\.type == "local_sensor"/);
  assert.doesNotMatch(firmware, /if \(p\.type == "text_sensor"\)/);
  assert.match(firmware, /#include "button_grid_saved_config_action_generated\.h"/);
  assert.match(firmware, /migrate_saved_config_action_legacy\(p\)/);
  assert.match(firmware, /normalize_saved_config_action\(p, normalize_saved_config_action_fields,/);
  assert.doesNotMatch(firmware, /if \(p\.type == "local"\)/);
  assert.doesNotMatch(firmware, /if \(p\.type == "option_select"\)/);
  assert.doesNotMatch(firmware, /if \(p\.type == "action"\) \{\s*p\.precision\.clear\(\);/);
  assert.match(firmware, /#include "button_grid_saved_config_media_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_media\(p, normalize_saved_config_media_fields,/);
  assert.doesNotMatch(firmware, /if \(p\.type == "media"\) \{\s*if \(p\.sensor == "controls"\)/);
  assert.match(firmware, /#include "button_grid_saved_config_static_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_static\(p\)/);
  assert.doesNotMatch(firmware, /if \(p\.type == "screen_lock"\)/);
  assert.doesNotMatch(firmware, /if \(p\.type == "light_switch"\)/);
  assert.doesNotMatch(firmware, /brightness_slider_type\(p\.type\) && !p\.sensor\.empty\(\)/);
  assert.match(firmware, /#include "button_grid_saved_config_fan_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_fan\(\s*p, normalize_saved_config_fan_fields, fan_control_card_options_normalized\)/);
  assert.doesNotMatch(firmware, /if \(fan_card_type\(p\.type\)\)/);
  assert.match(firmware, /#include "button_grid_saved_config_date_time_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_date_time\(\s*p, normalize_saved_config_date_time_fields, date_time_card_options_normalized\)/);
  assert.doesNotMatch(firmware, /if \(p\.type == "calendar"\) \{/);
  assert.doesNotMatch(firmware, /if \(p\.type == "clock"\) \{/);
  assert.doesNotMatch(firmware, /if \(p\.type == "timezone"\) \{/);
  assert.match(firmware, /#include "button_grid_saved_config_mower_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_mower\(p, normalize_saved_config_mower_fields\)/);
  assert.doesNotMatch(firmware, /if \(p\.type == "lawn_mower"\) \{/);
  assert.match(firmware, /#include "button_grid_saved_config_occupancy_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_occupancy\(\s*p, normalize_saved_config_occupancy_fields,/);
  assert.doesNotMatch(firmware, /if \(p\.type == "door_window"\) \{\s*p\.entity\.clear\(\);/);
  assert.doesNotMatch(firmware, /if \(p\.type == "presence"\) \{\s*p\.entity\.clear\(\);/);
  assert.match(firmware, /#include "button_grid_saved_config_access_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_access\(\s*p, normalize_saved_config_access_fields,/);
  const normalizeParser = firmware.slice(firmware.indexOf("inline ParsedCfg normalize_parsed_cfg"));
  assert.doesNotMatch(normalizeParser, /if \(p\.type == "garage"\) \{\s*if \(!card_runtime_garage_mode_valid/);
  assert.doesNotMatch(normalizeParser, /if \(p\.type == "gate"\) \{\s*if \(!card_runtime_gate_mode_valid/);
  assert.doesNotMatch(normalizeParser, /if \(p\.type == "cover"\) \{\s*if \(!card_runtime_cover_mode_valid/);
  assert.doesNotMatch(normalizeParser, /if \(p\.type == "lock"\) \{\s*if \(!card_runtime_lock_mode_valid/);
  assert.match(firmware, /#include "button_grid_saved_config_security_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_security\(\s*p, normalize_saved_config_security_fields,/);
  assert.doesNotMatch(normalizeParser, /if \(p\.type == "alarm"\) \{\s*p\.sensor\.clear\(\);/);
  assert.doesNotMatch(normalizeParser, /if \(p\.type == "alarm_action"\) \{\s*if \(!alarm_action_mode_valid/);
  assert.match(firmware, /#include "button_grid_saved_config_weather_generated\.h"/);
  assert.match(firmware, /migrate_saved_config_weather_legacy\(p\)/);
  assert.match(firmware, /normalize_saved_config_weather\(\s*p, was_legacy_weather_forecast,/);
  assert.doesNotMatch(normalizeParser, /if \(p\.type == "weather_forecast"\)/);
  assert.doesNotMatch(normalizeParser, /if \(p\.type == "weather"\)/);
  assert.match(firmware, /#include "button_grid_saved_config_image_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_image\(\s*p, normalize_saved_config_image_fields,/);
  assert.doesNotMatch(normalizeParser, /if \(p\.type == "image"\)/);
  assert.match(firmware, /#include "button_grid_saved_config_climate_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_climate\(\s*p, normalize_saved_config_climate_fields,/);
  assert.doesNotMatch(normalizeParser, /if \(climate_card_type\(p\.type\)\)/);
  assert.match(firmware, /#include "button_grid_saved_config_light_control_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_light_control\(p, normalize_saved_config_light_control_options\)/);
  assert.doesNotMatch(normalizeParser, /if \(p\.type == "light_control"\)/);
  assert.match(firmware, /#include "button_grid_saved_config_webhook_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_webhook\(\s*p, normalize_saved_config_webhook_fields, normalize_saved_config_webhook_options\)/);
  assert.doesNotMatch(normalizeParser, /if \(p\.type == "webhook"\)/);
  assert.match(firmware, /#include "button_grid_saved_config_subpage_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_subpage\(\s*p, normalize_saved_config_subpage_fields, normalize_saved_config_subpage_options\)/);
  assert.doesNotMatch(normalizeParser, /if \(p\.type == "subpage"\)/);
  assert.match(firmware, /#include "button_grid_saved_config_switch_generated\.h"/);
  assert.match(firmware, /normalize_saved_config_switch\(p, switch_card_options_normalized\)/);
  assert.doesNotMatch(normalizeParser, /if \(p\.type\.empty\(\)\) \{\s*p\.options = switch_card_options_normalized/);

  checkCompiledHelper();
  console.log("Saved-config production check passed: Access, Action, Climate, Date/Time, Fan, Image, Lawn Mower, Light Control, Media, Occupancy, Security, Sensor, Subpage, Switch, Vacuum, Weather, Webhook, and static card normalization use generated browser and compiled firmware helpers.");
}

main();
