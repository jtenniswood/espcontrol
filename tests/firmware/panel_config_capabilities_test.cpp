#include <cstdlib>
#include <cstring>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "panel_config_capabilities.h"
#include "panel_config_http_context.h"

int main() {
  using namespace espcontrol::configuration;
  std::array<char, PANEL_CONFIG_CAPABILITIES_BASE_JSON_BYTES> capabilities{};
  size_t capabilities_size = 0;
  const bool passed = PANEL_CONFIG_API_VERSION == 1 &&
                      PANEL_CONFIG_WEB_ASSET_VERSION == 2 &&
                      std::strcmp(PANEL_CONFIG_WEB_ASSET_DELIVERY, "manifest") == 0 &&
                      write_panel_config_capabilities_json(
                          capabilities.data(), capabilities.size(),
                          &capabilities_size) &&
                      capabilities_size > 0 &&
                      std::strstr(capabilities.data(), "\"max_record_body_bytes\":4096") != nullptr &&
                      std::strstr(capabilities.data(), "\"screensaver_pin\":{\"version\":1}") != nullptr &&
                      std::strstr(capabilities.data(), "\"identity\":{\"version\":1}") != nullptr &&
                      std::strstr(capabilities.data(), "\"reset\":{\"modes\":[\"customization\",\"factory\"]") != nullptr &&
                      std::strstr(capabilities.data(), "\"document_versions\":[1]") !=
                          nullptr &&
                      std::strstr(capabilities.data(), "\"read\":false") != nullptr &&
                      std::strstr(capabilities.data(), "\"write\":false") != nullptr &&
                      std::strstr(capabilities.data(), "\"web_assets\"") != nullptr &&
                      std::strstr(capabilities.data(), "\"delivery\":\"manifest\"") !=
                          nullptr &&
                      std::strstr(capabilities.data(), "\"icons\":{\"custom\":[]}") !=
                          nullptr &&
                      !write_panel_config_capabilities_json(nullptr,
                                                            capabilities.size(),
                                                            &capabilities_size);
  set_panel_config_read_supported(true);
  set_panel_config_write_supported(true);
  const bool read_capability_is_advertised =
      write_panel_config_capabilities_json(capabilities.data(),
                                           capabilities.size(),
                                           &capabilities_size) &&
      std::strstr(capabilities.data(), "\"read\":true") != nullptr &&
      std::strstr(capabilities.data(), "\"write\":true") != nullptr;
  // Firmware compiled with extra icons lists each one for the web picker,
  // dropping anything outside the custom icon range.
  set_panel_config_custom_icons({0xF00E6, 0x41, 0xF194B, 0x10FFFD, 0x110000});
  std::vector<char> listed(panel_config_capabilities_json_capacity());
  size_t listed_size = 0;
  const bool listed_ok = write_panel_config_capabilities_json(
      listed.data(), listed.size(), &listed_size);
  const std::string listed_json(listed.data(), listed_size);
  const bool custom_icons_are_advertised =
      listed_ok &&
      listed_json.find("\"icons\":{\"custom\":[\"F00E6\",\"F194B\",\"10FFFD\"]}") !=
          std::string::npos &&
      listed_json.back() == '}';
  // The response grows with the list instead of being cut off.
  std::vector<uint32_t> many;
  std::string many_json;
  {
    for (uint32_t codepoint = 0x100000; codepoint < 0x100000 + 200; codepoint++)
      many.push_back(codepoint);
    auto &icons = panel_config_custom_icons();
    icons = many;
    std::vector<char> large(panel_config_capabilities_json_capacity());
    size_t large_size = 0;
    if (write_panel_config_capabilities_json(large.data(), large.size(), &large_size))
      many_json.assign(large.data(), large_size);
  }
  const bool long_lists_fit = !many_json.empty() &&
      many_json.find("\"1000C7\"]}}") != std::string::npos;
  // A buffer sized without the list is rejected rather than truncated.
  const bool small_buffer_is_rejected = !write_panel_config_capabilities_json(
      capabilities.data(), capabilities.size(), &capabilities_size);
  set_panel_config_custom_icons({});
  set_panel_config_http_context_initialization_complete(false);
  const bool startup_capability_is_retryable =
      !panel_config_http_context_initialization_complete();
  set_panel_config_http_context_initialization_complete(true);
  const bool completed_initialization_is_observable =
      panel_config_http_context_initialization_complete();
  return passed && read_capability_is_advertised && custom_icons_are_advertised &&
                 long_lists_fit && small_buffer_is_rejected &&
                 startup_capability_is_retryable &&
                 completed_initialization_is_observable
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
