#include <cstdlib>

#include "request_uri.h"

using esphome::web_server_idf::request_uri_path_equals;

int main() {
  constexpr const char *path = "/api/card-images";
  if (!request_uri_path_equals("/api/card-images", path)) return EXIT_FAILURE;
  if (!request_uri_path_equals("/api/card-images?restore=0123456789abcdef", path)) return EXIT_FAILURE;
  if (request_uri_path_equals("/api/card-images/restore/begin", path)) return EXIT_FAILURE;
  if (request_uri_path_equals("/api/card-images-extra", path)) return EXIT_FAILURE;
  if (request_uri_path_equals(nullptr, path)) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
