#include <cstdint>
#include <cstdlib>

#include "panel_config_http_etag.h"

int main() {
  using espcontrol::configuration::parse_panel_config_etag;
  uint32_t generation = 0;
  const bool passed =
      parse_panel_config_etag("\"0\"", &generation) && generation == 0 &&
      parse_panel_config_etag("\"4294967295\"", &generation) &&
      generation == UINT32_MAX &&
      parse_panel_config_etag("\"v2-4294967295\"", &generation, 2) && generation == UINT32_MAX &&
      !parse_panel_config_etag("\"v2-4294967296\"", &generation, 2) &&
      !parse_panel_config_etag("\"v2-\"", &generation, 2) &&
      !parse_panel_config_etag("\"v2-1\"", &generation, 1) &&
      !parse_panel_config_etag("\"1\"", &generation, 2) &&
      !parse_panel_config_etag("\"v2-1\"", &generation, 3) &&
      !parse_panel_config_etag("7", &generation) &&
      !parse_panel_config_etag("\"7", &generation) &&
      !parse_panel_config_etag("\"7\"suffix", &generation) &&
      !parse_panel_config_etag("\"4294967296\"", &generation) &&
      !parse_panel_config_etag("\"x\"", &generation) &&
      !parse_panel_config_etag(nullptr, &generation) &&
      !parse_panel_config_etag("\"1\"", nullptr);
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
