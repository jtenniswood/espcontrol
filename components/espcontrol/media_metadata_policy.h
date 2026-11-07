#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
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

constexpr uint32_t MEDIA_TITLE_REFRESH_TIMEOUT_MS = 2000;

inline bool media_title_refresh_pending(bool awaiting_refresh, uint32_t refresh_started_ms, uint32_t now_ms) {
  return awaiting_refresh &&
         static_cast<uint32_t>(now_ms - refresh_started_ms) < MEDIA_TITLE_REFRESH_TIMEOUT_MS;
}

inline std::string normalize_media_kind_token(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    if (ch == '-' || ch == ' ') return '_';
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

inline const char* media_content_id_external_source(std::string content_id) {
  content_id = normalize_media_kind_token(std::move(content_id));
  if (content_id.rfind("x_rincon_stream:", 0) == 0) return "Line-in";
  if (content_id.rfind("x_sonos_htastream:", 0) == 0 || content_id.rfind("x_rincon_htastream:", 0) == 0 ||
      content_id.find(":spdif") != std::string::npos) {
    return "TV";
  }
  return "";
}

inline bool media_content_id_external_input(const std::string& content_id) {
  return media_content_id_external_source(content_id)[0] != '\0';
}

inline const char* media_source_external_label(std::string source) {
  source = normalize_media_kind_token(std::move(source));
  if (source == "line_in") return "Line-in";
  if (source == "tv" || source.rfind("hdmi", 0) == 0) return "TV";
  return "";
}

inline bool media_content_id_should_replace_external_source(bool source_observed_for_state,
                                                            bool retained_source_external,
                                                            bool retained_source_present) {
  return !source_observed_for_state || !retained_source_external || !retained_source_present;
}

inline bool media_content_id_should_override_source_update(bool content_id_observed_for_state,
                                                           const std::string& content_id,
                                                           const std::string& source_update) {
  if (!content_id_observed_for_state) return false;
  const char* content_label = media_content_id_external_source(content_id);
  return content_label[0] != '\0' && std::string(content_label) != media_source_external_label(source_update);
}

inline bool media_content_id_should_clear_external_source(const std::string& content_id,
                                                          bool current_source_external) {
  return current_source_external && !content_id.empty() && !media_content_id_external_input(content_id);
}

inline bool media_kind_token_is(std::string_view value, std::string_view normalized) {
  if (value.size() != normalized.size()) return false;
  for (size_t i = 0; i < value.size(); ++i) {
    const auto ch = static_cast<unsigned char>(value[i]);
    const char folded = ch == '-' || ch == ' ' ? '_' : static_cast<char>(std::tolower(ch));
    if (folded != normalized[i]) return false;
  }
  return true;
}
inline MediaItemKind media_item_kind_from_token_view(std::string_view token) {
  const auto is = [token](std::string_view expected) { return media_kind_token_is(token, expected); };
  if (is("track")) return MediaItemKind::TRACK;
  if (is("audiobook") || is("audiobooks")) return MediaItemKind::AUDIOBOOK;
  if (is("podcast") || is("podcasts") || is("podcast_episode") || is("podcastepisode") || is("episode"))
    return MediaItemKind::PODCAST;
  if (is("radio")) return MediaItemKind::RADIO;
  if (is("video") || is("movie") || is("tv") || is("tv_show") || is("tvshow") || is("channel"))
    return MediaItemKind::VIDEO;
  if (is("playlist") || is("album") || is("artist") || is("collection")) return MediaItemKind::COLLECTION;
  return MediaItemKind::UNKNOWN;
}
inline MediaItemKind media_item_kind_from_token(std::string token) {
  return media_item_kind_from_token_view(token);
}
inline std::string_view media_item_kind_token_from_uri_view(std::string_view content_id) {
  const size_t scheme_end = content_id.find(':');
  if (scheme_end == std::string_view::npos) return {};
  const auto scheme = content_id.substr(0, scheme_end);
  if (media_kind_token_is(scheme, "http") || media_kind_token_is(scheme, "https")) return {};
  size_t token_start = scheme_end + 1;
  if (content_id.substr(token_start, 2) == "//") token_start += 2;
  const size_t token_end = content_id.find_first_of(":/?#", token_start);
  if (token_start >= content_id.size() || token_end == token_start) return {};
  return content_id.substr(token_start,
                           token_end == std::string_view::npos ? token_end : token_end - token_start);
}
inline std::string media_item_kind_token_from_uri(const std::string& content_id) {
  return std::string(media_item_kind_token_from_uri_view(content_id));
}
inline MediaItemKind media_item_kind_view(std::string_view content_id, std::string_view content_type = {}) {
  const auto uri_kind = media_item_kind_from_token_view(media_item_kind_token_from_uri_view(content_id));
  if (uri_kind != MediaItemKind::UNKNOWN) return uri_kind;
  // Generic music remains unknown so it cannot hide a provider's specific kind.
  return media_item_kind_from_token_view(content_type);
}

inline uint64_t media_content_identity_fingerprint(const char* data, size_t length) {
  if (data == nullptr || length == 0) return 0;
  uint64_t hash = UINT64_C(14695981039346656037);
  for (size_t index = 0; index < length; index++) {
    hash ^= static_cast<uint8_t>(data[index]);
    hash *= UINT64_C(1099511628211);
  }
  // Zero is reserved for a missing identity.
  return hash == 0 ? 1 : hash;
}

inline uint64_t media_content_identity_fingerprint(const std::string& content_id) {
  return media_content_identity_fingerprint(content_id.data(), content_id.size());
}

inline MediaItemKind media_item_kind(const std::string& content_id, const std::string& content_type = {}) {
  return media_item_kind_view(content_id, content_type);
}

inline MediaMetadataClearDecision media_metadata_clear_decision(uint64_t previous_fingerprint,
                                                                MediaItemKind previous_kind,
                                                                uint64_t next_fingerprint,
                                                                MediaItemKind next_kind) {
  const bool item_changed =
      previous_fingerprint != 0 && next_fingerprint != 0 && previous_fingerprint != next_fingerprint;
  return {
      item_changed,
      item_changed,
      item_changed && previous_kind != MediaItemKind::UNKNOWN && next_kind != MediaItemKind::UNKNOWN &&
          previous_kind != next_kind,
  };
}

inline bool should_replace_media_metadata_identity(const std::string& next_content_id) {
  // Home Assistant may briefly clear media_content_id between queue items.
  // Retain the last established identity through that gap so the next item can
  // still be compared with the item whose metadata is currently displayed.
  return !next_content_id.empty();
}

inline bool media_modal_artist_visible(bool cover_art_mode, const std::string& state, bool artist_present) {
  return artist_present || !cover_art_mode || normalize_media_kind_token(state) != "idle";
}

inline bool media_control_updates_parent_label(bool cover_art_mode) {
  return !cover_art_mode;
}

}  // namespace espcontrol::media
