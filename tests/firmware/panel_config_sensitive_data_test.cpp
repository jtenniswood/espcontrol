#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>

#include "panel_config_sensitive_data.h"

using espcontrol::configuration::PanelConfigStatus;
using espcontrol::configuration::PanelConfigWriter;
using espcontrol::configuration::panel_config_contains_wifi_password;

template <size_t Size>
size_t write_document(std::array<uint8_t, Size> *document, const char *button,
                      const char *subpage = nullptr) {
  PanelConfigWriter writer(document->data(), document->size());
  assert(writer.begin() == PanelConfigStatus::OK);
  constexpr char profile[] = "test-panel";
  assert(writer.append_device_profile(
             reinterpret_cast<const uint8_t *>(profile), sizeof(profile) - 1) ==
         PanelConfigStatus::OK);
  assert(writer.append_button(1, reinterpret_cast<const uint8_t *>(button),
                              std::strlen(button)) == PanelConfigStatus::OK);
  if (subpage != nullptr) {
    assert(writer.append_subpage(1, reinterpret_cast<const uint8_t *>(subpage),
                                 std::strlen(subpage)) == PanelConfigStatus::OK);
  }
  size_t size = 0;
  assert(writer.finish(&size) == PanelConfigStatus::OK);
  return size;
}

int main() {
  std::array<uint8_t, 1024> document{};
  size_t size = write_document(
      &document, ";Connect;Wifi;Auto;;;wifi_qr;;ssid64=R3Vlc3Q,pass64=cGFzc3dvcmQ");
  assert(panel_config_contains_wifi_password(document.data(), size));

  size = write_document(
      &document, ";Connect;Wifi;Auto;;;wifi_qr;;ssid64=R3Vlc3Q,security=open");
  assert(!panel_config_contains_wifi_password(document.data(), size));

  size = write_document(
      &document, ";Label;Auto;Auto;;;;;pass64=not-a-wifi-card");
  assert(!panel_config_contains_wifi_password(document.data(), size));

  size = write_document(
      &document, ";Subpage;Auto;Auto;;;subpage",
      "~1|wifi_qr,,Connect,Wifi,Auto,,,,ssid64=R3Vlc3Q%2Cpass64=cGFzc3dvcmQ");
  assert(panel_config_contains_wifi_password(document.data(), size));

  document[0] = 'X';
  assert(!panel_config_contains_wifi_password(document.data(), size));
  return 0;
}
