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

inline const char *media_content_id_external_source(std::string content_id) {
  content_id = normalize_media_kind_token(std::move(content_id));
  if (content_id.rfind("x_rincon_stream:", 0) == 0) return "Line-in";
  if (content_id.rfind("x_sonos_htastream:", 0) == 0 ||
      content_id.rfind("x_rincon_htastream:", 0) == 0 ||
      content_id.find(":spdif") != std::string::npos) {
    return "TV";
  }
  return "";
}

inline bool media_content_id_external_input(const std::string &content_id) {
  return media_content_id_external_source(content_id)[0] != '\0';
}

inline const char *media_source_external_label(std::string source) {
  source = normalize_media_kind_token(std::move(source));
  if (source == "line_in") return "Line-in";
  if (source == "tv" || source.rfind("hdmi", 0) == 0) return "TV";
  return "";
}

inline bool media_content_id_should_replace_external_source(
    bool source_observed_for_state, bool retained_source_external,
    bool retained_source_present) {
  return !source_observed_for_state || !retained_source_external ||
         !retained_source_present;
}

inline bool media_content_id_should_override_source_update(
    bool content_id_observed_for_state, const std::string &content_id,
    const std::string &source_update) {
  if (!content_id_observed_for_state) return false;
  const char *content_label = media_content_id_external_source(content_id);
  return content_label[0] != '\0' &&
         std::string(content_label) != media_source_external_label(source_update);
}

inline bool media_content_id_should_clear_external_source(
    const std::string &content_id, bool current_source_external) {
  return current_source_external && !content_id.empty() &&
         !media_content_id_external_input(content_id);
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
  const size_t scheme_end = content_id.find(':');
  if (scheme_end == std::string::npos) return {};
  const std::string scheme = normalize_media_kind_token(
    content_id.substr(0, scheme_end));
  if (scheme == "http" || scheme == "https") return {};
  size_t token_start = scheme_end + 1;
  if (content_id.compare(token_start, 2, "//") == 0) token_start += 2;
  const size_t token_end = content_id.find_first_of(":/?#", token_start);
  if (token_start >= content_id.size() || token_end == token_start) return {};
  return content_id.substr(token_start, token_end - token_start);
}

inline uint64_t media_content_identity_fingerprint(const char *data,
                                                   size_t length) {
  if (data == nullptr || length == 0) return 0;
  uint64_t hash = UINT64_C(14695981039346656037);
  for (size_t index = 0; index < length; index++) {
    hash ^= static_cast<uint8_t>(data[index]);
    hash *= UINT64_C(1099511628211);
  }
  // Zero is reserved for a missing identity.
  return hash == 0 ? 1 : hash;
}

inline uint64_t media_content_identity_fingerprint(
    const std::string &content_id) {
  return media_content_identity_fingerprint(
    content_id.data(), content_id.size());
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
    uint64_t previous_fingerprint, MediaItemKind previous_kind,
    uint64_t next_fingerprint, MediaItemKind next_kind) {
  const bool item_changed =
    previous_fingerprint != 0 && next_fingerprint != 0 &&
    previous_fingerprint != next_fingerprint;
  return {
    item_changed,
    item_changed,
    item_changed && previous_kind != MediaItemKind::UNKNOWN &&
      next_kind != MediaItemKind::UNKNOWN && previous_kind != next_kind,
  };
}

inline bool should_replace_media_metadata_identity(
    const std::string &next_content_id) {
  // Home Assistant may briefly clear media_content_id between queue items.
  // Retain the last established identity through that gap so the next item can
  // still be compared with the item whose metadata is currently displayed.
  return !next_content_id.empty();
}

inline bool media_modal_artist_visible(bool cover_art_mode,
                                       const std::string &state) {
  return !cover_art_mode || normalize_media_kind_token(state) != "idle";
}

}  // namespace espcontrol::media
