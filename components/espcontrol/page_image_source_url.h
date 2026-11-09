#pragma once

#include <algorithm>
#include <cstdio>
#include <string>

#include "page_cover_art_policy.h"
#include "page_entity_state_runtime.h"
#include "page_image_tile_policy.h"

namespace espcontrol::pages {
struct PageImageTargetSize {
  uint16_t width{0}, height{0};
};
inline PageImageTargetSize page_image_limit_size(uint16_t width, uint16_t height, uint16_t maximum_side) {
  if (!width || !height) return {};
  const auto side = std::max<uint16_t>(1, maximum_side);
  const auto longest = std::max(width, height);
  if (longest <= side) return {width, height};
  return {static_cast<uint16_t>(std::max<uint32_t>(1, static_cast<uint32_t>(width) * side / longest)),
          static_cast<uint16_t>(std::max<uint32_t>(1, static_cast<uint32_t>(height) * side / longest))};
}
// Maintenance only: values remain graph-owned and URL bytes become local before
// transport/observer reentry. Never log the token or returned URL.
inline std::string page_image_select_source(PageEntityStateRuntime& state, ConfigurationHandle card,
                                            std::string_view entity, PageImageEntityKind kind,
                                            bool picture_event, std::string_view base) {
  const auto picture = state.read_attribute(card, entity, "entity_picture");
  const auto access = state.read_attribute(card, entity, "access_token");
  const auto raw =
      picture.status == PageEntityStatus::AVAILABLE ? picture.value.substr(0, 4096) : std::string_view{};
  const auto token =
      access.status == PageEntityStatus::AVAILABLE ? access.value.substr(0, 512) : std::string_view{};
  const bool usable_token = !token.empty() && token != "unknown" && token != "unavailable";
  std::string url;
  if (picture_event) url = PageArtworkUrl::join(base, raw).copy();
  if (url.empty() && !base.empty() && usable_token) {
    url = base;
    url += kind == PageImageEntityKind::CAMERA ? "/api/camera_proxy/" : "/api/image_proxy/";
    url.append(entity.data(), entity.size());
  } else if (url.empty())
    url = PageArtworkUrl::join(base, raw).copy();
  if (page_artwork_requires_token(url)) {
    if (!usable_token) return {};
    page_artwork_append_token(url, token);
  }
  return url;
}
inline void page_image_url_dimension(std::string& url, std::string_view name, uint16_t value) {
  if (page_artwork_query_has_param(url, name)) return;
  const auto fragment = url.find('#');
  const auto position = fragment == std::string::npos ? url.size() : fragment;
  char number[12];
  std::snprintf(number, sizeof(number), "%u", static_cast<unsigned>(value));
  std::string suffix = url.find('?') < position ? "&" : "?";
  suffix.append(name.data(), name.size());
  suffix += '=';
  suffix += number;
  url.insert(position, suffix);
}
inline uint16_t page_image_size_source(std::string& url, uint16_t width, uint16_t height,
                                       uint16_t maximum_side) {
  if (page_artwork_requires_token(url) || url.find("/api/camera_proxy/") != std::string::npos ||
      url.find("/api/image_proxy/") != std::string::npos) {
    const auto size = page_image_limit_size(width, height, maximum_side);
    page_image_url_dimension(url, "width", size.width);
    page_image_url_dimension(url, "height", size.height);
    return std::max(size.width, size.height);
  }
  return std::max(width, height);
}
}  // namespace espcontrol::pages
