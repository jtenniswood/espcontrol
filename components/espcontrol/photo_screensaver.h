#ifndef ESPCONTROL_PHOTO_SCREENSAVER_H
#define ESPCONTROL_PHOTO_SCREENSAVER_H

#pragma once

// Pure logic for the photo screensaver's Home Assistant media-source index.
//
// Everything here is host-testable and free of ESP-IDF, LVGL, and ArduinoJson.
// The websocket transport and JSON decoding live in the device-only layer and
// feed this index through add_photo(); the index owns ordering, rotation, and
// URL composition.
//
// Home Assistant facts this encodes (see dev-docs/photo-screensaver.md):
//   - A media-source child is an image when media_class == "image". The
//     media_content_type carries the guessed MIME type ("image/jpeg"), not the
//     literal "image", so both are accepted as evidence.
//   - can_play and can_expand are mutually exclusive for local media: a folder
//     expands, a file plays.
//   - resolve_media returns a *relative* signed path ("/media/local/a.jpg?authSig=..."),
//     which must be joined to the Home Assistant base URL before download.

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace espcontrol {

// Home Assistant serves every local media file through one browse response with
// no pagination, so a very large folder would otherwise grow this index without
// bound. Cap it and report the truncation rather than silently listing a subset.
inline constexpr std::size_t kPhotoIndexMaxEntries = 500;

enum class PhotoOrder : uint8_t {
  SEQUENTIAL,
  SHUFFLE,
};

// True when a browse child describes a still image.
inline bool media_child_is_image(const std::string &media_class,
                                 const std::string &media_content_type) {
  if (media_class == "image") return true;
  return media_content_type.rfind("image/", 0) == 0;
}

// True when a browse child describes a folder that can be expanded.
inline bool media_child_is_folder(const std::string &media_class, bool can_expand) {
  return can_expand || media_class == "directory";
}

// True when an index entry is already a downloadable path rather than a
// media_content_id that needs resolve_media. Browse children from proxying
// sources (Immich, for example) carry a thumbnail path like
// "/immich/<owner>/<asset>/thumbnail/image/jpeg"; media_content_ids always
// start with "media-source://".
inline bool media_item_is_direct_path(const std::string &item) {
  if (item.empty()) return false;
  if (item.front() == '/') return true;
  return item.rfind("http://", 0) == 0 || item.rfind("https://", 0) == 0;
}

// Immich's browse thumbnails are WebP regardless of their declared content
// type, which the artwork decoder cannot use — but the integration also serves
// a JPEG preview (~1440px) at the same path with the size segment swapped.
// Rewrite an Immich thumbnail path to its preview variant; anything else passes
// through untouched.
inline std::string immich_thumbnail_to_preview(const std::string &thumbnail) {
  if (thumbnail.rfind("/immich/", 0) != 0) return thumbnail;
  const std::string needle = "/thumbnail/";
  const size_t pos = thumbnail.find(needle);
  if (pos == std::string::npos) return thumbnail;
  std::string result = thumbnail;
  result.replace(pos, needle.size(), "/preview/");
  return result;
}

// Join the Home Assistant base URL to a resolve_media result. The signed path is
// relative; any other query parameter would invalidate the signature, so the
// path is used exactly as Home Assistant returned it.
inline std::string media_source_absolute_url(const std::string &base_url,
                                             const std::string &resolved_url) {
  if (resolved_url.empty()) return std::string();
  // Home Assistant may return an absolute URL for non-local sources.
  if (resolved_url.rfind("http://", 0) == 0 || resolved_url.rfind("https://", 0) == 0) {
    return resolved_url;
  }
  if (base_url.empty()) return std::string();

  std::string base = base_url;
  while (!base.empty() && base.back() == '/') base.pop_back();
  if (base.empty()) return std::string();

  if (resolved_url.front() != '/') return base + "/" + resolved_url;
  return base + resolved_url;
}

// Normalize a user-entered Home Assistant address into a websocket URL
// (ws(s)://host[:port]/api/websocket). Mirrors the browser-side normalizer in
// the web configurator so the device and the setup page agree on the endpoint.
//   - a bare host/IP over plain ws/http defaults to port 8123
//   - an explicit scheme is honoured; https/wss are left on their default port
//   - an explicit port (or IPv6 literal in brackets) is preserved
// Returns an empty string when the input has no host.
inline std::string normalize_ha_websocket_url(const std::string &raw) {
  std::string text;
  // Trim surrounding whitespace.
  size_t begin = raw.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) return std::string();
  size_t end = raw.find_last_not_of(" \t\r\n");
  text = raw.substr(begin, end - begin + 1);
  if (text.empty()) return std::string();

  std::string scheme;
  std::string rest;
  const size_t sep = text.find("://");
  if (sep == std::string::npos) {
    scheme = "http";
    rest = text;
  } else {
    scheme = text.substr(0, sep);
    for (char &c : scheme) c = static_cast<char>(tolower(c));
    rest = text.substr(sep + 3);
  }

  // Drop any trailing slashes and path; keep only the authority.
  while (!rest.empty() && rest.back() == '/') rest.pop_back();
  const size_t slash = rest.find('/');
  std::string host = slash == std::string::npos ? rest : rest.substr(0, slash);
  if (host.empty()) return std::string();

  const bool secure = scheme == "https" || scheme == "wss";
  // A colon means an explicit port (or an IPv6 literal, which is bracketed).
  if (!secure && host.find(':') == std::string::npos) {
    host += ":8123";
  }
  const std::string ws_scheme = secure ? "wss" : "ws";
  return ws_scheme + "://" + host + "/api/websocket";
}

// Ordered set of media_content_ids to rotate through.
//
// A large Home Assistant album holds hundreds of long media_content_ids, which
// is real memory on a panel whose internal heap has only tens of kilobytes
// free. The container types are template parameters so the firmware can keep
// the index in PSRAM-backed strings while host tests use plain std::string via
// the PhotoIndex alias below.
template<class String = std::string, class VecAlloc = std::allocator<String>>
class BasicPhotoIndex {
 public:
  // Returns false when the entry was rejected: either empty, a duplicate, or
  // beyond the cap. truncated() reports whether the cap was reached.
  bool add_photo(const char *media_content_id) {
    if (media_content_id == nullptr || media_content_id[0] == '\0') return false;
    if (photos_.size() >= kPhotoIndexMaxEntries) {
      truncated_ = true;
      return false;
    }
    for (const String &existing : photos_) {
      if (existing == media_content_id) return false;
    }
    photos_.emplace_back(media_content_id);
    return true;
  }

  bool add_photo(const String &media_content_id) {
    return this->add_photo(media_content_id.c_str());
  }

  void clear() {
    photos_.clear();
    cursor_ = 0;
    truncated_ = false;
    started_ = false;
  }

  bool empty() const { return photos_.empty(); }
  std::size_t size() const { return photos_.size(); }
  bool truncated() const { return truncated_; }
  const std::vector<String, VecAlloc> &photos() const { return photos_; }

  // The photo the screensaver should currently be showing. Empty when the index
  // holds nothing or rotation has not started.
  const String &current() const {
    static const String kEmpty;
    if (photos_.empty() || !started_) return kEmpty;
    return photos_[cursor_];
  }

  // Advance to the next photo and return it. The first call after clear()/add()
  // yields the first photo rather than skipping it. Wraps at the end.
  const String &advance(PhotoOrder order, uint32_t random_value) {
    static const String kEmpty;
    if (photos_.empty()) return kEmpty;

    if (!started_) {
      started_ = true;
      cursor_ = order == PhotoOrder::SHUFFLE ? random_value % photos_.size() : 0;
      return photos_[cursor_];
    }

    if (photos_.size() == 1) return photos_[cursor_];

    if (order == PhotoOrder::SHUFFLE) {
      // Pick from the other entries so the same photo never repeats twice in a
      // row, which reads as a stall rather than a shuffle.
      const std::size_t offset = random_value % (photos_.size() - 1);
      cursor_ = (cursor_ + 1 + offset) % photos_.size();
    } else {
      cursor_ = (cursor_ + 1) % photos_.size();
    }
    return photos_[cursor_];
  }

 private:
  std::vector<String, VecAlloc> photos_;
  std::size_t cursor_{0};
  bool truncated_{false};
  bool started_{false};
};

using PhotoIndex = BasicPhotoIndex<>;

}  // namespace espcontrol

#endif  // ESPCONTROL_PHOTO_SCREENSAVER_H
