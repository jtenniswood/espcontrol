#include "page_artwork_requests.h"

#include <cassert>
#include <string>

using namespace espcontrol::pages;
using esphome::artwork_image::ArtworkImage;
#ifdef USE_LVGL
size_t artwork_test_draw_fences = 0;
void lv_draw_wait_for_finish() { ++artwork_test_draw_fences; }
#endif
namespace {
struct Origin final : PageImageOrigin {
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
      a.reply(true, true);  // A 304 without a usable image cannot mark the slot ready.
      assert(runtime.snapshot(0, first).failed);
      assert(runtime.request(0, first, "https://ha/new", options));
      a.ready = true;
      a.reply();
      assert(runtime.snapshot(0, first).ready);
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
}
}  // namespace
int main() {
  pool_claims();
  integration();
}
