#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <utility>

namespace espcontrol::media {

enum class MediaItemKind : uint8_t {
  UNKNOWN = 0,
  TRACK,
  AUDIOBOOK,
  PODCAST,
  RADIO,
  VIDEO,
  COLLECTION,
};

struct MediaMetadataClearDecision {
  bool item_changed = false;
  bool clear_title = false;
  bool clear_grouping = false;
};

inline std::string normalize_media_kind_token(std::string value) {
  std::transform(
    value.begin(), value.end(), value.begin(),
    [](unsigned char ch) {
      if (ch == '-' || ch == ' ') return '_';
      return static_cast<char>(std::tolower(ch));
    });
  return value;
}

inline MediaItemKind media_item_kind_from_token(std::string token) {
  token = normalize_media_kind_token(std::move(token));
  if (token == "track") return MediaItemKind::TRACK;
  if (token == "audiobook" || token == "audiobooks") {
    return MediaItemKind::AUDIOBOOK;
  }
  if (token == "podcast" || token == "podcasts" ||
      token == "podcast_episode" || token == "podcastepisode" ||
      token == "episode") {
    return MediaItemKind::PODCAST;
  }
  if (token == "radio") return MediaItemKind::RADIO;
  if (token == "video" || token == "movie" || token == "tv" ||
      token == "tv_show" || token == "tvshow" || token == "channel") {
    return MediaItemKind::VIDEO;
  }
  if (token == "playlist" || token == "album" || token == "artist" ||
      token == "collection") {
    return MediaItemKind::COLLECTION;
  }
  return MediaItemKind::UNKNOWN;
}

inline std::string media_item_kind_token_from_uri(const std::string &content_id) {
  const size_t scheme_end = content_id.find("://");
  if (scheme_end == std::string::npos) return {};
  const std::string scheme = normalize_media_kind_token(
    content_id.substr(0, scheme_end));
  if (scheme == "http" || scheme == "https") return {};
  const size_t token_start = scheme_end + 3;
  const size_t token_end = content_id.find_first_of("/?#", token_start);
  if (token_start >= content_id.size() || token_end == token_start) return {};
  return content_id.substr(token_start, token_end - token_start);
}

inline MediaItemKind media_item_kind(const std::string &content_id,
                                     const std::string &content_type = {}) {
  const MediaItemKind uri_kind = media_item_kind_from_token(
    media_item_kind_token_from_uri(content_id));
  if (uri_kind != MediaItemKind::UNKNOWN) return uri_kind;
  // Music Assistant exposes every queue item as generic "music", including
  // audiobooks, so only specific content types are useful as a fallback.
  if (normalize_media_kind_token(content_type) == "music") {
    return MediaItemKind::UNKNOWN;
  }
  return media_item_kind_from_token(content_type);
}

inline MediaMetadataClearDecision media_metadata_clear_decision(
    const std::string &previous_content_id, MediaItemKind previous_kind,
    const std::string &next_content_id, MediaItemKind next_kind) {
  const bool item_changed =
    !previous_content_id.empty() && !next_content_id.empty() &&
    previous_content_id != next_content_id;
  return {
    item_changed,
    item_changed,
    item_changed && previous_kind != MediaItemKind::UNKNOWN &&
      next_kind != MediaItemKind::UNKNOWN && previous_kind != next_kind,
  };
}

}  // namespace espcontrol::media
