#include <cstdlib>
#include <cstring>
#include <array>

#include "panel_config_capabilities.h"
#include "panel_config_http_context.h"

int main() {
  using namespace espcontrol::configuration;
  std::array<char, PANEL_CONFIG_CAPABILITIES_MAX_JSON_BYTES> capabilities{};
  size_t capabilities_size = 0;
  const bool passed = PANEL_CONFIG_API_VERSION == 1 &&
                      PANEL_CONFIG_WEB_ASSET_VERSION == 1 &&
                      std::strcmp(PANEL_CONFIG_WEB_ASSET_DELIVERY, "manifest") == 0 &&
                      write_panel_config_capabilities_json(
                          capabilities.data(), capabilities.size(),
                          &capabilities_size) &&
                      capabilities_size > 0 &&
                      std::strstr(capabilities.data(), "\"document_versions\":[1]") !=
                          nullptr &&
                      std::strstr(capabilities.data(), "\"read\":false") != nullptr &&
                      std::strstr(capabilities.data(), "\"write\":false") != nullptr &&
                      std::strstr(capabilities.data(), "\"web_assets\"") != nullptr &&
                      std::strstr(capabilities.data(), "\"delivery\":\"manifest\"") !=
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
  set_panel_config_http_context_initialization_complete(false);
  const bool startup_capability_is_retryable =
      !panel_config_http_context_initialization_complete();
  set_panel_config_http_context_initialization_complete(true);
  const bool completed_initialization_is_observable =
      panel_config_http_context_initialization_complete();
  return passed && read_capability_is_advertised &&
                 startup_capability_is_retryable &&
                 completed_initialization_is_observable
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
