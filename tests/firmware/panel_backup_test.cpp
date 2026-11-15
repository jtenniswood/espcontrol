#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "../../components/espcontrol/panel_backup.h"

using namespace espcontrol::configuration;

std::vector<uint8_t> fixture() {
  std::vector<uint8_t> document(20000);
  PanelConfigWriter writer(document.data(), document.size());
  auto bytes = [](const std::string &s) { return reinterpret_cast<const uint8_t *>(s.data()); };
  const std::string profile = "guition-esp32-s3-4848s040";
  const std::string card = "~light.kitchen,Kitchen%3B%20%22table%22%0A%C2%B0,Lightbulb,Auto,,,,,card_on_color=FF8800";
  const std::string legacy = "sensor.room;Room;Thermometer;Auto;;°C;sensor;1;";
  const std::string subpage = "1,B=Return|scene.movie:Movie:Play:Auto:scene.turn_on::action::";
  const std::string order = "1,2,3,0,0,0,0,0,0", color = "AA4400";
  assert(writer.begin() == PanelConfigStatus::OK);
  assert(writer.append_device_profile(bytes(profile), profile.size()) == PanelConfigStatus::OK);
  assert(writer.append_button(1, bytes(card), card.size()) == PanelConfigStatus::OK);
  assert(writer.append_button(2, bytes(legacy), legacy.size()) == PanelConfigStatus::OK);
  assert(writer.append_subpage(1, bytes(subpage), subpage.size()) == PanelConfigStatus::OK);
  const std::string compact_subpage = "~1,B=Back%3B%20home|A,scene.movie,Movie%3A%20night,Play,Auto,scene.turn_on,,,";
  assert(writer.append_subpage(2, bytes(compact_subpage), compact_subpage.size()) == PanelConfigStatus::OK);
  // Include a large options field to exercise streaming beyond one chunk.
  const std::string large = "light.large;Large;Auto;Auto;;;;;" + std::string(3500, 'x');
  assert(writer.append_button(3, bytes(large), large.size()) == PanelConfigStatus::OK);
  assert(writer.append_setting(bytes("button_order"), 12, bytes(order), order.size()) == PanelConfigStatus::OK);
  assert(writer.append_setting(bytes("button_on_color"), 15, bytes(color), color.size()) == PanelConfigStatus::OK);
  size_t size = 0;
  assert(writer.finish(&size) == PanelConfigStatus::OK);
  document.resize(size);
  return document;
}

int main(int argc, char **) {
  const auto document = fixture();
  const std::string settings = R"({"language":"de","clock_bar_temperature_precision":2,"clock_bar":false,"ntp_server_1":"time.example.test","screensaver_timeout":90})";
  const std::string screen = R"({"brightness_day":42,"theme_mode":"Light","schedule_trigger":"sensor","schedule_sensor_entity":"binary_sensor.room"})";
  const std::string identity = R"({"version":1,"name":"Kitchen","hostname":"kitchen-ab1234","mac_suffix":"ab1234"})";
  std::string output;
  auto sink = [&](const std::string &part) { output += part; return true; };
  assert(espcontrol::backup::write_backup(document.data(), document.size(), 9, settings, screen,
                                         identity, "2026-11-02T12:00:00Z", sink));
  if (argc > 1) { std::cout << output; return 0; }
  assert(output.find("Kitchen; \\\"table\\\"\\u000a°") != std::string::npos);
  assert(output.find("\"native_config\":") != std::string::npos);
  // Invalid documents and slot mismatches must fail before writing any JSON.
  output.clear();
  assert(!espcontrol::backup::write_backup(document.data(), document.size() - 1, 9, settings, screen, "", "", sink));
  assert(output.empty());
  assert(!espcontrol::backup::write_backup(document.data(), document.size(), 2, settings, screen, "", "", sink));
  assert(output.empty());
  assert(!espcontrol::backup::write_backup(document.data(), document.size(), 33, settings, screen, "", "", sink));
  auto disconnected = [](const std::string &) { return false; };
  assert(!espcontrol::backup::write_backup(document.data(), document.size(), 9, settings, screen, "", "", disconnected));
  for (size_t slots : {9, 12, 20, 32}) {
    output.clear();
    assert(espcontrol::backup::write_backup(document.data(), document.size(), slots, settings, screen, "", "", sink));
    assert(output.find("exported_at") == std::string::npos);
  }
  assert(decode_compact_field("%C2%B0%3B") == "°;");
  assert(decode_compact_field("%FF%3B") == "%FF%3B");
  assert(espcontrol::backup::json_string(std::string("\0\t\\\"", 4)) == "\"\\u0000\\u0009\\\\\\\"\"");
}
