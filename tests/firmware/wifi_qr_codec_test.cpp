#include <cassert>
#include <string>

#include "wifi_qr_codec.h"

int main() {
  std::string payload, ssid, password;
  assert(wifi_qr_build_payload("R3Vlc3QgV2ktRmk", "wpa", "UGFzczt3b3JkOjEyMw", true, &payload, &ssid, &password));
  assert(ssid == "Guest Wi-Fi");
  assert(password == "Pass;word:123");
  assert(payload == "WIFI:T:WPA;S:Guest Wi-Fi;P:Pass\\;word\\:123;H:true;;");
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
