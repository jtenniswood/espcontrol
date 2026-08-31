#include <iostream>

#include "panel_config_endpoint_policy.h"

namespace {

bool expect(bool condition, const char *message) {
  if (condition) return true;
  std::cerr << message << '\n';
  return false;
}

}  // namespace

int main() {
  using espcontrol::configuration::ServiceStatus;
  using espcontrol::configuration::panel_config_load_allows_native_endpoints;

  if (!expect(!panel_config_load_allows_native_endpoints(
                  ServiceStatus::BUFFER_TOO_SMALL),
              "Oversized legacy documents must keep native endpoints disabled"))
    return 1;
  if (!expect(panel_config_load_allows_native_endpoints(ServiceStatus::OK),
              "A loaded native document should enable native endpoints"))
    return 1;
  if (!expect(panel_config_load_allows_native_endpoints(
                  ServiceStatus::IMPORTED_LEGACY),
              "An imported legacy document should enable native endpoints"))
    return 1;
  if (!expect(panel_config_load_allows_native_endpoints(ServiceStatus::EMPTY),
              "An empty store should allow a new native document"))
    return 1;
  if (!expect(!panel_config_load_allows_native_endpoints(
                  ServiceStatus::STORE_FAILED),
              "An unreadable pre-upgrade NVS blob must disable native endpoints"))
    return 1;
  if (!expect(!panel_config_load_allows_native_endpoints(
                  ServiceStatus::INVALID_DOCUMENT),
              "A damaged native document must disable native endpoints"))
    return 1;
  if (!expect(!panel_config_load_allows_native_endpoints(
                  ServiceStatus::UNSUPPORTED_VERSION),
              "An unsupported native document must disable native endpoints"))
    return 1;
  return 0;
}
