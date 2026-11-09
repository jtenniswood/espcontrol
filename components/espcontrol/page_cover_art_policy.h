#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <string_view>

#include "button_grid_string.h"
#include "ha_state_limits.h"

namespace espcontrol::pages {
// A joined URL can be compared during a graph notification without allocating
// or keeping a second copy of its (up to 4 KiB) picture attribute.
struct PageArtworkUrl {
  std::string_view prefix, path;
  static PageArtworkUrl join(std::string_view base, std::string_view path) {
    if (path.empty() || path == "unknown" || path == "unavailable" || path == "None") return {};
    if (path.substr(0, 7) == "http://" || path.substr(0, 8) == "https://") return {{}, path};
    if (base.empty() || path.front() != '/') return {};
    return {base, path};
  }
  size_t size() const {
    return prefix.size() + path.size();
  }
  bool empty() const {
    return size() == 0;
  }
  char at(size_t i) const {
    return i < prefix.size() ? prefix[i] : path[i - prefix.size()];
  }
  bool operator==(PageArtworkUrl other) const {
    if (size() != other.size()) return false;
    for (size_t i = 0; i < size(); ++i)
      if (at(i) != other.at(i)) return false;
    return true;
  }
  bool operator!=(PageArtworkUrl other) const {
    return !(*this == other);
  }
  bool equals(std::string_view other) const {
    return *this == PageArtworkUrl{{}, other};
  }
  std::string copy() const {
    std::string result;
    result.reserve(size());
    if (!prefix.empty()) result.append(prefix.data(), prefix.size());
    if (!path.empty()) result.append(path.data(), path.size());
    return result;
  }
};

inline bool page_artwork_query_has_param(std::string_view url, std::string_view name) {
  const auto query = url.find('?');
  if (query == std::string_view::npos || url.find('#') < query) return false;
  const auto fragment = url.find('#', query + 1);
  const auto end = fragment == std::string_view::npos ? url.size() : fragment;
  size_t position = query + 1;
  while (position < end) {
    const auto amp = url.find('&', position);
    const auto next = amp == std::string_view::npos || amp > end ? end : amp;
    const auto equal = url.find('=', position);
    if (equal != std::string_view::npos && equal < next && url.substr(position, equal - position) == name)
      return true;
    position = next + 1;
  }
  return false;
}
inline bool page_artwork_requires_token(std::string_view url) {
  const bool protected_proxy = url.find("/api/camera_proxy/") != std::string_view::npos ||
                               url.find("/api/image_proxy/") != std::string_view::npos;
  return protected_proxy && !page_artwork_query_has_param(url, "token");
}
inline void page_artwork_append_token(std::string& url, std::string_view token) {
  // Query parameters precede a fragment; otherwise the token never reaches HA.
  const auto fragment = url.find('#');
  std::string suffix;
  if (fragment != std::string::npos) {
    suffix = url.substr(fragment);
    url.erase(fragment);
  }
  url += url.find('?') == std::string::npos ? "?token=" : "&token=";
  url.append(token.data(), token.size());
  url += suffix;
}

struct PageCoverArtSignature {
  // "metadata:" + two raw-limited, decoded metadata fields + separator.
  static constexpr size_t CAPACITY = 9 + 2 * HA_STATE_TEXT_MAX_LEN + 1;
  std::array<char, CAPACITY + 1> bytes{};
  uint16_t length{0};
  static PageCoverArtSignature make(std::string_view content, std::string_view title, std::string_view artist,
                                    bool state_known, std::string_view state) {
    PageCoverArtSignature result;
    auto append = [&](std::string_view text) {
      const auto count = std::min(text.size(), CAPACITY - result.length);
      if (count) std::memcpy(result.bytes.data() + result.length, text.data(), count);
      result.length += count;
    };
    char first[HA_STATE_TEXT_MAX_LEN + 1], second[HA_STATE_TEXT_MAX_LEN + 1];
    const auto metadata = [](char* output, std::string_view raw) {
      const auto decoded = decode_html_entities_bounded(output, HA_STATE_TEXT_MAX_LEN + 1,
                                                        raw.substr(0, HA_STATE_TEXT_MAX_LEN));
      return decoded == "unknown" || decoded == "unavailable" ? std::string_view{} : decoded;
    };
    const auto id = metadata(first, content);
    if (!id.empty()) {
      append("content:");
      append(id);
    } else {
      const auto t = metadata(first, title), a = metadata(second, artist);
      if (!t.empty() || !a.empty()) {
        append("metadata:");
        append(t);
        append("\x1f");
        append(a);
      } else if (state_known) {
        append("state:");
        append(state.substr(0, HA_SHORT_STATE_MAX_LEN));
      }
    }
    return result;
  }
  std::string_view view() const {
    return {bytes.data(), length};
  }
  bool operator==(const PageCoverArtSignature& other) const {
    return view() == other.view();
  }
  bool operator!=(const PageCoverArtSignature& other) const {
    return !(*this == other);
  }
};
}  // namespace espcontrol::pages
