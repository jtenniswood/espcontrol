#include "page_cover_art_policy.h"

#include <cassert>
#include <cstdlib>

#include "artwork_controller.h"

using namespace espcontrol::pages;
namespace {
size_t allocations = 0;
}
void* operator new(size_t size) {
  ++allocations;
  if (void* pointer = std::malloc(size ? size : 1)) return pointer;
  std::abort();
}
void* operator new[](size_t size) {
  return ::operator new(size);
}
void operator delete(void* pointer) noexcept {
  std::free(pointer);
}
void operator delete[](void* pointer) noexcept {
  std::free(pointer);
}
void operator delete(void* pointer, std::size_t) noexcept {
  std::free(pointer);
}
void operator delete[](void* pointer, std::size_t) noexcept {
  std::free(pointer);
}

int main() {
  for (const auto base : {"", "https://ha", "https://ha/", "https://ha/path"}) {
    for (const auto raw : {"", "unknown", "unavailable", "None", "relative", "/picture", "//picture",
                           "http://remote/picture", "https://remote/picture", "HTTPS://remote/picture"}) {
      const auto joined = PageArtworkUrl::join(base, raw);
      std::string expected;
      const std::string_view path(raw);
      if (!path.empty() && path != "unknown" && path != "unavailable" && path != "None") {
        if (path.substr(0, 7) == "http://" || path.substr(0, 8) == "https://")
          expected = path;
        else if (*base && path.front() == '/')
          expected = std::string(base) + raw;
      }
      assert(joined.copy() == expected && joined.equals(expected));
    }
  }
  const std::string long_url = "https://ha/" + std::string(4096, 'x');
  const auto first = PageArtworkUrl::join("https://ha", std::string_view(long_url).substr(10));
  const auto second = PageArtworkUrl::join({}, long_url);
  const auto before = allocations;
  for (int i = 0; i < 1000; ++i) {
    assert(first == second && first.equals(long_url));
    assert(PageArtworkUrl::join("ab", "/c") == (PageArtworkUrl{"a", "b/c"}));
    assert(PageArtworkUrl::join("ab", "/c") != PageArtworkUrl::join("ab", "/d"));
  }
  assert(allocations == before);
  using Signature = PageCoverArtSignature;
  assert(Signature::make("track", "title", "artist", true, "playing").view() == "content:track");
  assert(Signature::make("unknown", "A &amp; B", "Artist", true, "playing").view() ==
         "metadata:A & B\x1f"
         "Artist");
  assert(Signature::make("", "unknown", "unavailable", true, "paused").view() == "state:paused");
  assert(Signature::make("", "", "", false, "").view().empty());
  const std::string long_metadata(HA_STATE_TEXT_MAX_LEN + 10, 'x');
  const auto max_signature = Signature::make("", long_metadata, long_metadata, true, "playing");
  assert(max_signature.length == Signature::CAPACITY);
  assert(max_signature == Signature::make("", long_metadata + "ignored", long_metadata, true, "paused"));
  assert(Signature::make("track", "a", "b", true, "playing") ==
         Signature::make("track", "c", "d", true, "paused"));
  std::string encoded;
  for (size_t i = 0; i < 19; ++i) encoded += "&amp;";
  encoded += 'a';
  assert(encoded.size() == HA_STATE_TEXT_MAX_LEN);
  assert(Signature::make(encoded + "ignored", "", "", false, "").view() ==
         "content:" + std::string(19, '&') + "a");
  // View selection shares the legacy local-first policy and authoritative clear.
  using espcontrol::artwork::select_source_views;
  assert(select_source_views("", "local", false, "local", false).primary.empty());
  auto chosen = select_source_views("remote", "local", false, "local", true);
  assert(chosen.primary == "local" && chosen.fallback == "remote" && !chosen.preferred_refreshed_remote);
  chosen = select_source_views("remote", "local", true, "local", true);
  assert(chosen.primary == "remote" && chosen.fallback == "local" && chosen.preferred_refreshed_remote);
  chosen = select_source_views("remote", "local", true, "remote", true);
  assert(chosen.primary == "local" && chosen.fallback == "remote" && !chosen.preferred_refreshed_remote);
  assert(page_artwork_requires_token("https://ha/api/image_proxy/image.cover"));
  assert(!page_artwork_requires_token("https://ha/api/image_proxy/image.cover?token="));
  assert(!page_artwork_requires_token("https://ha/api/media_player_proxy/player"));
  assert(page_artwork_requires_token("https://ha/api/image_proxy/image.cover#fragment?token=fake"));
  assert(!page_artwork_query_has_param("url?another=token&token_extra=1#token=2", "token"));
  std::string protected_url = "https://ha/api/image_proxy/image.cover?width=200#picture";
  page_artwork_append_token(protected_url, "example");
  assert(protected_url == "https://ha/api/image_proxy/image.cover?width=200&token=example#picture");
  const auto hot_before = allocations;
  for (int i = 0; i < 1000; ++i) {
    assert(Signature::make("track", "a", "b", true, "playing").view() == "content:track");
    assert(select_source_views(long_url, "local", true, "local", true).primary == long_url);
  }
  assert(allocations == hot_before);
}
