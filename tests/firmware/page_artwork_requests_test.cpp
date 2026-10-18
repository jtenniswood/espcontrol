#include "page_artwork_requests.h"

#include <cassert>
#include <string>

#include "page_modal_image_runtime.h"

using namespace espcontrol::pages;
using esphome::artwork_image::ArtworkImage;
#ifdef USE_LVGL
size_t artwork_test_draw_fences = 0;
void lv_draw_wait_for_finish() { ++artwork_test_draw_fences; }
#endif
namespace {
struct Origin final : PageImageOrigin {
  bool shown{true};
  bool visible(PageBindingToken) const override {
    return shown;
  }
  bool valid(PageBindingToken token) const override {
    return token.binding != 0;
  }
  bool connected() const override {
    return true;
  }
  uint32_t connection_generation() const override {
    return 1;
  }
};
void pool_claims() {
  ArtworkImage a, b;
  ArtworkImage* images[]{&a, &b};
  PageArtworkRequests<2> pool;
  assert(!pool.initialize(nullptr, 2));
  assert(!pool.initialize(images, 1));
  b.legacy_owned = true;
  assert(!pool.initialize(images, 2));
  assert(!a.channel.claimed() && !b.channel.claimed());
  b.legacy_owned = false;
  images[1] = &a;
  assert(!pool.initialize(images, 2));  // Duplicate component, independent observers.
  assert(!a.channel.claimed());
  images[1] = nullptr;
  assert(!pool.initialize(images, 2));
  assert(!a.channel.claimed());
  images[1] = &b;
  assert(pool.initialize(images, 2));
  assert(!pool.initialize(images, 2));
  assert(pool.image(0) == &a && pool.image(2) == nullptr);
  PageArtworkRequests<2> second;
  assert(!second.initialize(images, 2));
  assert(a.channel.claimed() && b.channel.claimed());
}
void request_priority_and_cap() {
  ArtworkImage image;
  ArtworkImage* images[]{&image};
  PageArtworkRequests<1> requests;
  assert(requests.initialize(images, 1));
  Origin origin;
  PageImageRuntime<1> runtime(requests, origin);
  const PageBindingToken token{1, 1, 0, 0, 0, false};
  assert(runtime.bind(0, token));
  PageImageOptions options{123, 234};
  options.max_source_dimension = 200;
  for (const auto priority : {PageImagePriority::BACKGROUND, PageImagePriority::TILE,
                              PageImagePriority::COVER_ART, PageImagePriority::MODAL}) {
    options.priority = priority;
    assert(runtime.request(0, token, "https://ha/picture", options));
    assert(
        image.priority ==
        (priority == PageImagePriority::BACKGROUND ? esphome::artwork_image::ImageRequestPriority::BACKGROUND
         : priority == PageImagePriority::TILE     ? esphome::artwork_image::ImageRequestPriority::TILE
         : priority == PageImagePriority::MODAL ? esphome::artwork_image::ImageRequestPriority::MODAL
                                                : esphome::artwork_image::ImageRequestPriority::COVER_ART));
    assert(image.max_source_dimension == 200);
  }
}
void modal_and_tile_components() {
  ArtworkImage tile, modal_image;
  ArtworkImage* tile_components[]{&tile};
  ArtworkImage* modal_components[]{&modal_image};
  PageArtworkRequests<1> tile_requests, modal_requests;
  assert(tile_requests.initialize(tile_components, 1) && modal_requests.initialize(modal_components, 1));
  PageArtworkRequests<1> overlapping;
  assert(!overlapping.initialize(tile_components, 1));
  Origin tile_origin;
  PageImageRuntime<1> tiles(tile_requests, tile_origin);
  const PageBindingToken tile_token{0, 1, 0, 0, 0, false};
  assert(tiles.acquire(tile_token) == 0);
  assert(tiles.request(0, tile_token, "https://test/tile", {200, 100}));
  tile.ready = true;
  tile.reply();
  assert(tile.visible());
  struct ModalOrigin final : PageModalImageOrigin {
    PageModalToken token{0, 1, 0, ControlModalKind::IMAGE_CARD};
    bool shown{true}, alive{true};
    bool valid(PageModalToken value) const override {
      return alive && PageModalImageIdentity::same(value, token);
    }
    bool visible(PageModalToken value) const override {
      return shown && valid(value);
    }
    bool connected() const override {
      return true;
    }
    uint32_t connection_generation() const override {
      return 1;
    }
  } origin;
  {
    PageModalImageRuntime modal(modal_requests, origin);
    assert(modal.acquire(origin.token) == 0);
    PageImageOptions options{800, 600, 0x123456, false, true, PageImagePriority::MODAL, 800};
    assert(modal.request(0, origin.token, "https://test/modal", options));
    assert(modal_image.priority == esphome::artwork_image::ImageRequestPriority::MODAL &&
           modal_image.max_source_dimension == 800);
    modal_image.ready = true;
    modal_image.reply();
    assert(modal_image.visible() && tile.visible());
    const auto old = origin.token;
    const auto generation = modal_image.generation;
    const auto tile_asset = tiles.snapshot(0, tile_token).asset;
    origin.shown = false;
    assert(!modal_image.visible() && modal_image.screen_active());
    origin.shown = true;
    const auto releases = modal_image.releases;
    const auto fences = artwork_test_draw_fences;
    origin.alive = false;
    modal.unbind(0);
    assert(!modal_image.visible() && modal_image.releases == releases);
    modal.loop();
    assert(modal_image.releases == releases + 1 && artwork_test_draw_fences == fences + 1);
    ++origin.token.opening;
    origin.alive = true;
    assert(modal.acquire(origin.token) == 0);
    assert(modal.request(0, origin.token, "https://test/new-modal", options));
    modal_image.channel.deliver(generation, true, false);
    assert(modal.snapshot(0, origin.token).pending && !modal.snapshot(0, old).ready);
    modal_image.ready = true;
    modal_image.reply();
    assert(modal.snapshot(0, origin.token).ready && tiles.snapshot(0, tile_token).asset == tile_asset);
    const auto cached_asset = modal.snapshot(0, origin.token).asset;
    const auto cached_generation = modal_image.generation;
    uint64_t parked = 0;
    origin.alive = false;
    assert(modal.park(0, origin.token, parked));
    const auto parked_fences = artwork_test_draw_fences;
    const auto parked_releases = modal_image.releases;
    modal.loop();
    assert(modal_image.ready && !modal_image.visible() && !modal_image.screen_active() &&
           artwork_test_draw_fences == parked_fences && modal_image.releases == parked_releases);
    ++origin.token.opening;
    origin.alive = true;
    assert(modal.acquire(origin.token) == 0 && !modal.snapshot(0, origin.token).ready);
    assert(modal.adopt(0, origin.token, parked));
    modal_image.channel.deliver(cached_generation, true, true);
    assert(modal.snapshot(0, origin.token).asset == cached_asset && modal_image.visible());
    assert(modal.park(0, origin.token, parked));
    assert(modal.discard(0, parked));
    assert(modal_image.releases == parked_releases);
    modal.loop();
    assert(modal_image.releases == parked_releases + 1 && artwork_test_draw_fences == parked_fences + 1);
    assert(modal.acquire(origin.token) == 0);
    assert(modal.request(0, origin.token, "https://test/no-pixels", options));
    modal_image.ready = false;
    modal_image.reply(true, true);
    assert(modal.snapshot(0, origin.token).failed && !modal.snapshot(0, origin.token).ready);
  }
  assert(!modal_image.ready && tile.visible());
}
void integration() {
  ArtworkImage a, b;
  ArtworkImage* images[]{&a, &b};
  Origin origin;
  {
    PageArtworkRequests<2> requests;
    assert(requests.initialize(images, 2));
    {
      PageImageRuntime<2> runtime(requests, origin);
      const PageBindingToken first{1, 1, 0, 0, 0, false}, second{2, 2, 0, 1, 0, false};
      const PageImageOptions options{123, 234, 0x123456, false};
      assert(runtime.bind(0, first));
      {
        const std::string source = "https://ha/artwork/very-long-picture-path?token=test";
        assert(runtime.request(0, first, source, options));
      }
      assert(a.url == "https://ha/artwork/very-long-picture-path?token=test");
      assert(a.width == 123 && a.height == 234 && a.background == 0x123456);
      assert(a.mode == esphome::artwork_image::ImageResizeMode::FIT);
      assert(a.priority == esphome::artwork_image::ImageRequestPriority::COVER_ART &&
             a.max_source_dimension == 0);
      assert(a.visible && a.redraw && a.screen_active);
      assert(!a.visible() && a.screen_active());
      {
        PageImageRuntime<2> second_runtime(requests, origin);
        assert(!second_runtime.bind(1, second));
      }
      assert(a.screen_active());  // Failed competing owner cannot revoke the first.

      a.reply(true, true);  // A 304 without a usable image cannot mark the slot ready.
      assert(runtime.snapshot(0, first).failed);
      assert(runtime.request(0, first, "https://ha/new", options));
      a.ready = true;
      a.reply();
      assert(runtime.snapshot(0, first).ready);
      assert(a.visible());
      const auto asset = runtime.snapshot(0, first).asset;
      const auto revision = runtime.revision();
      a.redraw();
      assert(runtime.revision() == revision + 1 && runtime.snapshot(0, first).asset == asset);
      origin.shown = false;
      assert(!a.visible() && a.screen_active());  // A modal pauses animation, retaining its screen.
      origin.shown = true;

      const auto generation = a.generation;
      const auto releases = a.releases;
      runtime.unbind(0);  // The view already detached its LVGL descriptor.
      assert(a.releases == releases && a.ready);
      assert(runtime.bind(0, second));
      assert(runtime.request(0, second, "https://ha/other", options));
      assert(a.releases == releases + 1 && !a.ready);
      a.channel.deliver(generation, true, false);
      assert(runtime.snapshot(0, second).pending);
      a.ready = true;
      a.reply();
      assert(runtime.snapshot(0, second).ready);
      assert(runtime.bind(1, first));
      assert(runtime.request(1, first, "https://ha/independent", options));
      b.ready = true;
      b.reply(false);
      assert(runtime.snapshot(1, first).failed && runtime.snapshot(0, second).ready);
      b.accept = false;
      assert(!runtime.request(1, first, "https://ha/rejected", options));
      assert(runtime.snapshot(1, first).failed);
    }
    assert(!a.ready && !b.ready);  // Runtime drains retired surfaces before adapter destruction.
  }
  assert(!a.channel.claimed() && !b.channel.claimed());
  assert(!a.visible && !a.redraw && !a.screen_active);
}
}  // namespace
int main() {
  pool_claims();
  integration();
  request_priority_and_cap();
  modal_and_tile_components();
}
