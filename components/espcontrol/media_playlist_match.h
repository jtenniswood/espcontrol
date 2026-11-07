#pragma once

#include <cctype>
#include <string_view>

namespace espcontrol::media {
// Match the existing playlist highlighting rules on borrowed cache/configuration
// text. These helpers do not allocate or retain either view.
inline std::string_view playlist_trim(std::string_view value) {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.remove_prefix(1);
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.remove_suffix(1);
  return value;
}
inline std::string_view playlist_without_suffix(std::string_view value) {
  value = value.substr(0, value.find_first_of("#?"));
  while (!value.empty() && value.back() == '/') value.remove_suffix(1);
  return value;
}
inline bool playlist_equal(std::string_view left, std::string_view right) {
  if (left.size() != right.size()) return false;
  for (size_t i = 0; i < left.size(); ++i)
    if (std::tolower(static_cast<unsigned char>(left[i])) !=
        std::tolower(static_cast<unsigned char>(right[i])))
      return false;
  return true;
}
inline bool playlist_contains(std::string_view value, std::string_view needle) {
  if (needle.size() > value.size()) return false;
  for (size_t i = 0; i <= value.size() - needle.size(); ++i)
    if (playlist_equal(value.substr(i, needle.size()), needle)) return true;
  return false;
}
inline std::string_view playlist_leaf(std::string_view value) {
  value = playlist_without_suffix(playlist_trim(value));
  const auto separator = value.find_last_of(":/");
  return playlist_trim(separator == std::string_view::npos || separator + 1 >= value.size()
                           ? value
                           : value.substr(separator + 1));
}
inline bool playlist_content_id_matches(std::string_view configured, std::string_view current) {
  const auto wanted = playlist_trim(playlist_without_suffix(playlist_trim(configured)));
  const auto actual = playlist_trim(playlist_without_suffix(playlist_trim(current)));
  if (wanted.empty() || actual.empty()) return false;
  if (playlist_equal(wanted, actual)) return true;
  constexpr std::string_view prefix = "media-source://";
  if (wanted.size() >= prefix.size() && playlist_equal(wanted.substr(0, prefix.size()), prefix)) {
    const auto path = wanted.substr(prefix.size());
    if (!path.empty() && playlist_contains(actual, path)) return true;
  }
  const auto wanted_leaf = playlist_leaf(wanted), actual_leaf = playlist_leaf(actual);
  return (wanted_leaf.size() >= 4 && playlist_equal(wanted_leaf, actual_leaf)) ||
         (wanted_leaf.size() >= 8 && playlist_contains(actual, wanted_leaf));
}
inline bool playlist_content_type_matches(std::string_view configured, std::string_view current) {
  const auto wanted = playlist_trim(configured.empty() ? std::string_view("playlist") : configured);
  const auto actual = playlist_trim(current);
  return actual.empty() || playlist_equal(wanted, actual) ||
         (playlist_equal(wanted, "playlist") &&
          (playlist_equal(actual, "music") || playlist_equal(actual, "audio"))) ||
         (playlist_equal(wanted, "track") && playlist_equal(actual, "music"));
}
}  // namespace espcontrol::media
