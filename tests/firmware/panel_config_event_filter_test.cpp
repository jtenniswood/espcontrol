#include <cassert>
#include <cstring>

#include "panel_config_event_filter.h"

int main() {
  using esphome::web_server_idf::event_payload_is_legacy_panel_config;
  const char button[] = R"({"id":"text-button_1_config","value":"pass64=secret"})";
  const char subpage[] = R"({"id":"text-subpage_3_config_ext","value":"secret"})";
  const char order[] = R"({"id":"text-button_order","value":"1,2,3"})";
  const char normal[] = R"({"id":"sensor-wifi_strength","value":-58})";
  assert(event_payload_is_legacy_panel_config(button, std::strlen(button)));
  assert(event_payload_is_legacy_panel_config(subpage, std::strlen(subpage)));
  assert(!event_payload_is_legacy_panel_config(order, std::strlen(order)));
  assert(!event_payload_is_legacy_panel_config(normal, std::strlen(normal)));
}
