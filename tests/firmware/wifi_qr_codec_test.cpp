#include <cassert>
#include <string>

#include "wifi_qr_codec.h"
#include "wifi_qr_layout.h"

int main() {
  assert(wifi_qr_tile_vertical_inset(1, 1) == 6);
  assert(wifi_qr_tile_vertical_inset(2, 2) == 3);
  assert(wifi_qr_tile_vertical_inset(3, 3) == 3);

  assert((wifi_qr_tabs("") == std::vector<std::string>{"qr", "credentials"}));
  assert((wifi_qr_tabs("credentials|qr") == std::vector<std::string>{"credentials", "qr"}));
  assert((wifi_qr_tabs("credentials") == std::vector<std::string>{"credentials"}));
  assert((wifi_qr_tabs("invalid") == std::vector<std::string>{"qr"}));
  assert((wifi_qr_tabs("credentials|credentials|qr") == std::vector<std::string>{"credentials", "qr"}));
  std::string payload, ssid, password;
  assert(wifi_qr_build_payload("R3Vlc3QgV2lmaQ", "wpa", "UGFzczt3b3JkOjEyMw", true, &payload, &ssid, &password));
  assert(ssid == "Guest Wifi");
  assert(password == "Pass;word:123");
  assert(payload == "WIFI:T:WPA;S:Guest Wifi;P:Pass\\;word\\:123;H:true;;");
  assert(wifi_qr_build_payload("Q2Fmw6k", "open", "", false, &payload, &ssid));
  assert(ssid == "Café");
  assert(wifi_qr_build_payload("R3Vlc3Q", "open", "", false, &payload, &ssid));
  assert(payload == "WIFI:T:nopass;S:Guest;H:false;;");
  assert(wifi_qr_build_payload("R3Vlc3Q", "open", "", false, &payload, &ssid, &password));
  assert(password.empty());
  assert(!wifi_qr_build_payload("R3Vlc3Q", "wpa", "c2hvcnQ", false, &payload, &ssid));
  assert(!wifi_qr_build_payload("!", "open", "", false, &payload, &ssid));
  return 0;
}
