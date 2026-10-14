#pragma once

#include <cstring>

namespace esphome::web_server_idf {

inline bool request_uri_path_equals(const char *uri, const char *path) {
  if (uri == nullptr || path == nullptr || path[0] == '\0') return false;
  const size_t path_length = std::strlen(path);
  return std::strncmp(uri, path, path_length) == 0 &&
         (uri[path_length] == '\0' || uri[path_length] == '?');
}

}  // namespace esphome::web_server_idf
