#include <cstdlib>
#include <string>

#include "photo_screensaver.h"

using namespace espcontrol;

#define CHECK(condition) do { if (!(condition)) return EXIT_FAILURE; } while (false)

int main() {
  // Home Assistant reports still images with media_class "image" and a guessed
  // MIME media_content_type, so either is decisive.
  CHECK(media_child_is_image("image", "image/jpeg"));
  CHECK(media_child_is_image("image", ""));
  CHECK(media_child_is_image("", "image/png"));
  CHECK(!media_child_is_image("directory", ""));
  CHECK(!media_child_is_image("music", "audio/mpeg"));
  CHECK(!media_child_is_image("video", "video/mp4"));
  // "image/..." must be a prefix, not a substring.
  CHECK(!media_child_is_image("", "x-image/jpeg"));

  CHECK(media_child_is_folder("directory", true));
  CHECK(media_child_is_folder("directory", false));
  CHECK(media_child_is_folder("", true));
  CHECK(!media_child_is_folder("image", false));

  // resolve_media returns a relative signed path that must be joined to the base
  // URL untouched, because any extra query parameter breaks the signature.
  const std::string base = "http://10.0.0.5:8123";
  CHECK(media_source_absolute_url(base, "/media/local/a.jpg?authSig=abc") ==
        "http://10.0.0.5:8123/media/local/a.jpg?authSig=abc");
  // A trailing slash on the base must not produce a doubled separator.
  CHECK(media_source_absolute_url(base + "/", "/media/local/a.jpg") ==
        "http://10.0.0.5:8123/media/local/a.jpg");
  CHECK(media_source_absolute_url(base + "///", "/media/local/a.jpg") ==
        "http://10.0.0.5:8123/media/local/a.jpg");
  // A relative path without a leading slash still joins cleanly.
  CHECK(media_source_absolute_url(base, "media/local/a.jpg") ==
        "http://10.0.0.5:8123/media/local/a.jpg");
  // Absolute URLs from non-local sources pass through untouched.
  CHECK(media_source_absolute_url(base, "https://cdn.example/a.jpg") ==
        "https://cdn.example/a.jpg");
  CHECK(media_source_absolute_url(base, "http://cdn.example/a.jpg") ==
        "http://cdn.example/a.jpg");
  // Missing inputs yield nothing rather than a malformed URL.
  CHECK(media_source_absolute_url(base, "").empty());
  CHECK(media_source_absolute_url("", "/media/local/a.jpg").empty());
  CHECK(media_source_absolute_url("/", "/media/local/a.jpg").empty());

  // Immich thumbnail paths rewrite to the JPEG preview variant; everything else
  // passes through untouched.
  CHECK(immich_thumbnail_to_preview("/immich/owner/asset/thumbnail/image/jpeg") ==
        "/immich/owner/asset/preview/image/jpeg");
  CHECK(immich_thumbnail_to_preview("/immich/owner/asset/fullsize/image/jpeg") ==
        "/immich/owner/asset/fullsize/image/jpeg");
  // Only Immich paths are rewritten; other sources may serve real files at a
  // path that happens to contain the word.
  CHECK(immich_thumbnail_to_preview("/other/thumbnail/a.jpg") ==
        "/other/thumbnail/a.jpg");
  CHECK(immich_thumbnail_to_preview("").empty());

  // A thumbnail path from a proxying source is downloadable as-is; a
  // media_content_id needs resolve_media first.
  CHECK(media_item_is_direct_path("/immich/owner/asset/thumbnail/image/jpeg"));
  CHECK(media_item_is_direct_path("http://cdn.example/a.jpg"));
  CHECK(media_item_is_direct_path("https://cdn.example/a.jpg"));
  CHECK(!media_item_is_direct_path("media-source://media_source/local/a.jpg"));
  CHECK(!media_item_is_direct_path("media-source://immich/a|albums|b"));
  CHECK(!media_item_is_direct_path(""));

  // A bare host over plain http gets Home Assistant's default port and path.
  CHECK(normalize_ha_websocket_url("10.0.0.5") ==
        "ws://10.0.0.5:8123/api/websocket");
  CHECK(normalize_ha_websocket_url("  homeassistant.local  ") ==
        "ws://homeassistant.local:8123/api/websocket");
  CHECK(normalize_ha_websocket_url("http://10.0.0.5") ==
        "ws://10.0.0.5:8123/api/websocket");
  // An explicit port is preserved rather than replaced.
  CHECK(normalize_ha_websocket_url("10.0.0.5:9000") ==
        "ws://10.0.0.5:9000/api/websocket");
  CHECK(normalize_ha_websocket_url("http://10.0.0.5:8123/") ==
        "ws://10.0.0.5:8123/api/websocket");
  // https/wss keep their default port and become wss.
  CHECK(normalize_ha_websocket_url("https://ha.example.com") ==
        "wss://ha.example.com/api/websocket");
  CHECK(normalize_ha_websocket_url("wss://ha.example.com/api/websocket") ==
        "wss://ha.example.com/api/websocket");
  // A trailing path is discarded; only the authority is kept.
  CHECK(normalize_ha_websocket_url("http://10.0.0.5:8123/lovelace/0") ==
        "ws://10.0.0.5:8123/api/websocket");
  // Blank input yields nothing rather than a malformed URL.
  CHECK(normalize_ha_websocket_url("").empty());
  CHECK(normalize_ha_websocket_url("   ").empty());
  CHECK(normalize_ha_websocket_url("http://").empty());

  // An empty index yields nothing and never advances.
  PhotoIndex index;
  CHECK(index.empty());
  CHECK(index.size() == 0);
  CHECK(!index.truncated());
  CHECK(index.current().empty());
  CHECK(index.advance(PhotoOrder::SEQUENTIAL, 0).empty());

  CHECK(index.add_photo("media-source://media_source/local/photos/a.jpg"));
  CHECK(index.add_photo("media-source://media_source/local/photos/b.jpg"));
  CHECK(index.add_photo("media-source://media_source/local/photos/c.jpg"));
  CHECK(index.size() == 3);
  CHECK(!index.empty());

  // Empty and duplicate entries are rejected without disturbing the index.
  CHECK(!index.add_photo(""));
  CHECK(!index.add_photo("media-source://media_source/local/photos/a.jpg"));
  CHECK(index.size() == 3);

  // Rotation has not started, so there is nothing on screen yet.
  CHECK(index.current().empty());

  // The first advance shows the first photo rather than skipping it.
  CHECK(index.advance(PhotoOrder::SEQUENTIAL, 0) ==
        "media-source://media_source/local/photos/a.jpg");
  CHECK(index.current() == "media-source://media_source/local/photos/a.jpg");
  CHECK(index.advance(PhotoOrder::SEQUENTIAL, 0) ==
        "media-source://media_source/local/photos/b.jpg");
  CHECK(index.advance(PhotoOrder::SEQUENTIAL, 0) ==
        "media-source://media_source/local/photos/c.jpg");
  // Sequential rotation wraps back to the start.
  CHECK(index.advance(PhotoOrder::SEQUENTIAL, 0) ==
        "media-source://media_source/local/photos/a.jpg");

  // A single photo is a fixed point: it neither wraps away nor repeats wrongly.
  PhotoIndex single;
  CHECK(single.add_photo("only.jpg"));
  CHECK(single.advance(PhotoOrder::SEQUENTIAL, 0) == "only.jpg");
  CHECK(single.advance(PhotoOrder::SEQUENTIAL, 0) == "only.jpg");
  CHECK(single.advance(PhotoOrder::SHUFFLE, 7) == "only.jpg");

  // Shuffle must never show the same photo twice in a row, for any random value.
  PhotoIndex shuffle;
  CHECK(shuffle.add_photo("a.jpg"));
  CHECK(shuffle.add_photo("b.jpg"));
  CHECK(shuffle.add_photo("c.jpg"));
  shuffle.advance(PhotoOrder::SHUFFLE, 0);
  for (uint32_t random_value = 0; random_value < 64; ++random_value) {
    const std::string previous = shuffle.current();
    const std::string next = shuffle.advance(PhotoOrder::SHUFFLE, random_value);
    CHECK(next != previous);
    CHECK(!next.empty());
  }

  // Shuffle reaches every photo rather than oscillating between a subset. The
  // device seeds this from esp_random(); a counter is deliberately not used here
  // because a low-entropy sequence makes any modulo-based pick degenerate.
  bool seen_a = false, seen_b = false, seen_c = false;
  uint32_t lcg = 12345;
  for (int i = 0; i < 64; ++i) {
    lcg = lcg * 1103515245u + 12345u;
    const std::string next = shuffle.advance(PhotoOrder::SHUFFLE, lcg >> 16);
    if (next == "a.jpg") seen_a = true;
    if (next == "b.jpg") seen_b = true;
    if (next == "c.jpg") seen_c = true;
  }
  CHECK(seen_a && seen_b && seen_c);

  // The cap bounds memory on very large folders and is reported, not silent.
  PhotoIndex capped;
  for (std::size_t i = 0; i < kPhotoIndexMaxEntries; ++i) {
    CHECK(capped.add_photo("photo_" + std::to_string(i) + ".jpg"));
  }
  CHECK(capped.size() == kPhotoIndexMaxEntries);
  CHECK(!capped.truncated());
  CHECK(!capped.add_photo("one_too_many.jpg"));
  CHECK(capped.truncated());
  CHECK(capped.size() == kPhotoIndexMaxEntries);

  // clear() returns the index to its initial state, including truncation.
  capped.clear();
  CHECK(capped.empty());
  CHECK(!capped.truncated());
  CHECK(capped.current().empty());

  // A re-index (folder changed) restarts rotation from the first photo.
  index.clear();
  CHECK(index.add_photo("x.jpg"));
  CHECK(index.current().empty());
  CHECK(index.advance(PhotoOrder::SEQUENTIAL, 0) == "x.jpg");

  return EXIT_SUCCESS;
}
