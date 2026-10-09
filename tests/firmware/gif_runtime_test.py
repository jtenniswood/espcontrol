#!/usr/bin/env python3
"""Compile the production GIF decoder and playback loop with host display doubles."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
COMPONENT = ROOT / "components/artwork_image"
artwork = (COMPONENT / "artwork_image.cpp").read_text()
methods = []
for name in ("retain_animation_", "pause_animation_for_refresh_", "replace_animation_",
             "end_connection_", "stop_animation_", "loop_animation_"):
    match = re.search(rf"^void ArtworkImage::{name}\(\) \{{\n.*?^\}}", artwork, re.M | re.S)
    assert match, name
    methods.append(match[0])

stub = r'''
#pragma once
#include <cassert>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>
#include "gif_image.h"
#include "esphome/core/log.h"
#define ESP_LOGD(tag, ...) ((void)(tag))
#define ESP_LOGW(tag, ...) ((void)(tag))
#define ESP_LOGI(tag, ...) ((void)(tag))
namespace esphome::artwork_image {
enum P4PipelinePriority { P4_PIPELINE_TILE, P4_PIPELINE_MODAL };
struct Downloader { void end() {} };
struct ArtworkImage {
  bool service_active_ = false, p4_pipeline_pending_ = false, s3_transfer_pending_ = false;
  std::shared_ptr<Downloader> downloader_;
  std::unique_ptr<ImageDecoder> decoder_;
  DownloadBuffer download_buffer_{0};
  esphome::RAMAllocator<uint8_t> allocator_;
  bool gif_decoding_ = true;
  P4PipelinePriority p4_pipeline_priority_ = P4_PIPELINE_TILE;
  std::unique_ptr<GifDecoder> animation_;
  bool animation_frame_pending_ = false, animation_frame_ready_ = false, fail_resize = false;
  uint32_t animation_frame_started_ms_ = 0, animation_frame_delay_ms_ = 0;
  std::function<bool()> animation_visible_;
  std::function<bool()> animation_screen_active_;
  std::function<void()> animation_redraw_;
  std::vector<uint8_t> active, staging;
  uint8_t *buffer_ = nullptr, *decode_buffer_ = nullptr;
  int cache_invalidations = 0, width = 0, height = 0, decode_buffer_width_ = 0;
  int get_bpp() const { return 16; }
  bool has_transparency() const { return false; }
  bool is_big_endian() const { return false; }
  size_t get_buffer_size_() const { return active.size(); }
  size_t get_decode_buffer_size_() const { return staging.size(); }
  void invalidate_lvgl_cache_() { ++cache_invalidations; }
  void draw_pixel_(int x, int y, Color color) {
    assert(x >= 0 && x < width && y >= 0 && y < height);
    const auto pixel = ((color.r & 248) << 8) | ((color.g & 252) << 3) | (color.b >> 3);
    const auto pos = (y * width + x) * 2;
    staging.at(pos) = pixel; staging.at(pos + 1) = pixel >> 8;
  }
  void discard_decode_buffer_() { staging.clear(); decode_buffer_ = nullptr; }
  void retain_animation_();
  void pause_animation_for_refresh_();
  void replace_animation_();
  void end_connection_();
  void cancel_s3_transfer_() { s3_transfer_pending_ = false; }
  void cancel_p4_pipeline_() { p4_pipeline_pending_ = false; }
  void stop_animation_();
  void loop_animation_();
  void publish_first() { active = std::move(staging); buffer_ = active.data(); decode_buffer_ = nullptr; }
};
}
'''

source = r'''
#include <cassert>
#include <fstream>
#include <iterator>
#include "artwork_image.h"
uint32_t now_ms = 0;
namespace esphome {
uint32_t millis() { return now_ms; }
uint32_t micros() { return now_ms * 1000; }
namespace artwork_image {
static ArtworkImage *gif_slots[2] = {nullptr, nullptr};
static ArtworkImage *gif_playing = nullptr;
static const char *TAG = "test";
ImageDecoder::~ImageDecoder() { release_filtered_resize(); }
void ImageDecoder::release_filtered_resize() {
  if (resample_workspace_) resample_allocator_.deallocate(resample_workspace_, resample_workspace_size_);
  resample_workspace_ = nullptr; resample_workspace_size_ = 0;
}
bool ImageDecoder::set_size(int width, int height) {
  if (image_->fail_resize) return false;
  image_->width = image_->decode_buffer_width_ = width; image_->height = height;
  image_->staging.assign(static_cast<size_t>(width) * height * 2, 0);
  image_->decode_buffer_ = image_->staging.data(); return true;
}
bool ImageDecoder::prepare_filtered_resize(int width, int height) {
  release_filtered_resize();
  resample_workspace_size_ = ScanlineResampler::workspace_size(width);
  resample_workspace_ = resample_allocator_.allocate(resample_workspace_size_);
  return resampler_.configure(width, height, width, height, width, height, 0, 0,
                             resample_workspace_, resample_workspace_size_);
}
void ImageDecoder::draw_filtered_rgb888_row(int y, const uint8_t *data) {
  assert(resampler_.push_row(y, [data](int x) {
    return ResampleColor{data[x * 3], data[x * 3 + 1], data[x * 3 + 2]};
  }, [this](int x, int y, ResampleColor color) {
    const auto pixel = ((color.r & 248) << 8) | ((color.g & 252) << 3) | (color.b >> 3);
    const auto pos = (y * image_->width + x) * 2;
    image_->staging.at(pos) = pixel; image_->staging.at(pos + 1) = pixel >> 8;
  }));
}
void DownloadBuffer::shrink_to(size_t size) {
  assert(size == 0); allocator_.deallocate(buffer_, size_);
  buffer_ = nullptr; size_ = unread_ = 0;
}
DownloadBuffer::DownloadBuffer(size_t size) : buffer_(nullptr), size_(size), unread_(0) { assert(!size); }
bool DownloadBuffer::adopt(uint8_t *buffer, size_t size) {
  assert(!buffer_); if (!buffer || !size) return false;
  buffer_ = buffer; size_ = unread_ = size; return true;
}
}
}
'''

source += '\nnamespace esphome::artwork_image {\n' + 'void ImageDecoder::draw_fast_filtered_rgb565_row(int y, const uint16_t *data) {\n  if (!this->resampler_.push_row_fast(y, [data](int x) {\n        const auto pixel = data[x];\n        const uint8_t r = (pixel >> 11) & 31, g = (pixel >> 5) & 63, b = pixel & 31;\n        return ResampleColor{static_cast<uint8_t>((r << 3) | (r >> 2)),\n                             static_cast<uint8_t>((g << 2) | (g >> 4)),\n                             static_cast<uint8_t>((b << 3) | (b >> 2))};\n      }, [this](int x, int y, ResampleColor color) {\n        this->image_->draw_pixel_(x, y, Color(color.r, color.g, color.b, 0xFF));\n      })) this->failed_ = true;\n}' + '\n}\n'

main = r'''
using namespace esphome::artwork_image;
static std::vector<uint8_t> read(const std::string &path) {
  std::ifstream f(path, std::ios::binary); assert(f.good());
  return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
static void load(ArtworkImage &image, const std::vector<uint8_t> &file, bool unknown_length, bool animated = true) {
  image.service_active_ = true;
  image.pause_animation_for_refresh_();
  image.gif_decoding_ = true;
  if (!image.animation_visible_) image.animation_visible_ = []() { return true; };
  esphome::RAMAllocator<uint8_t> allocator;
  auto *data = allocator.allocate(file.size()); std::memcpy(data, file.data(), file.size());
  auto decoder = std::make_unique<GifDecoder>(&image);
  assert(decoder->prepare(unknown_length ? 0 : file.size()) == 0);
  // Partial and unknown-length transfers must wait without allocating a canvas.
  assert(decoder->decode(data, file.size() - 1) == 0 && !decoder->is_finished());
  if (unknown_length) {
    assert(decoder->decode(data, file.size()) == 0 && !decoder->is_finished());
    decoder->set_download_size(file.size());
  }
  for (int i = 0; !decoder->is_finished() && i < 1000; ++i) assert(decoder->decode(data, file.size()) == 0);
  assert(decoder->is_finished());
  assert(!esphome::HighFrequencyLoopRequester::is_high_frequency());
  assert(!std::memcmp(data, file.data(), file.size()));
  assert(image.download_buffer_.adopt(data, file.size()));
  image.publish_first(); image.decoder_ = std::move(decoder);
  image.replace_animation_();
  assert(bool(image.animation_) == animated);
  image.end_connection_(); image.service_active_ = false;
}
int main() {
  auto file = read(std::string(GIF_FIXTURE_DIR) + "/disposal.gif");
  auto expected = read(std::string(GIF_FIXTURE_DIR) + "/disposal.rgb565");
  for (bool unknown : {false, true}) {
    ArtworkImage image;
    bool visible = true; int redraws = 0;
    image.animation_visible_ = [&]() { return visible; };
    image.animation_redraw_ = [&]() { ++redraws; };
    load(image, file, unknown);
    assert(std::equal(image.active.begin(), image.active.end(), expected.begin() + 6));
    const auto first = image.active;
    now_ms += 99; image.loop_animation_(); assert(redraws == 0);
    visible = false; now_ms += 1000;
    for (int i = 0; i < 100; ++i) image.loop_animation_();
    assert(redraws == 0 && image.active == first && !gif_playing);
    assert(!esphome::HighFrequencyLoopRequester::is_high_frequency());
    visible = true; now_ms += 100;
    for (int i = 0; i < 100 && !redraws; ++i) image.loop_animation_();
    assert(redraws == 1 && image.cache_invalidations == 1);
    assert(std::equal(image.active.begin(), image.active.end(), expected.begin() + 6 + 128));
    // A second visible decoder yields while this image owns playback.
    ArtworkImage other; other.p4_pipeline_priority_ = P4_PIPELINE_MODAL; load(other, file, false);
    other.animation_visible_ = []() { return true; };
    now_ms += 1000; other.loop_animation_(); assert(!other.cache_invalidations);
    visible = false; image.loop_animation_();
    for (int i = 0; i < 100 && !other.cache_invalidations; ++i) other.loop_animation_();
    assert(other.cache_invalidations == 1);
    other.stop_animation_();
    visible = true;
    // Complete the finite animation, comparing every composited frame.
    for (int n = 2; n < 8; ++n) {
      const int before = redraws;
      now_ms += image.animation_->delay_ms();
      for (int i = 0; i < 100 && redraws == before; ++i) image.loop_animation_();
      assert(redraws == before + 1);
      assert(std::equal(image.active.begin(), image.active.end(), expected.begin() + 6 + (n % 4) * 128));
    }
    now_ms += image.animation_->delay_ms();
    for (int i = 0; i < 100 && image.animation_; ++i) image.loop_animation_();
    assert(!image.animation_ && !gif_playing);
    const auto last = image.active;
    image.stop_animation_(); image.stop_animation_();
    assert(!image.animation_ && !gif_slots[0] && !gif_playing && image.active == last);
    assert(fake_esphome_allocator::external_pointers.empty());
  }
  // Multiple GIF cards share one animation slot. The expanded view has its
  // own slot, leaves the chosen card resident, and releases it on close.
  ArtworkImage card, second, expanded;
  card.animation_screen_active_ = []() { return true; };
  second.animation_screen_active_ = []() { return true; };
  load(card, file, false);
  const auto *card_decoder = card.animation_.get();
  const auto retained_allocations = fake_esphome_allocator::external_pointers.size();
  // A covered widget is still on this page and must keep its reservation.
  card.animation_visible_ = []() { return false; };
  load(second, file, false, false);
  assert(!second.animation_ && second.active == card.active);
  assert(fake_esphome_allocator::external_pointers.size() == retained_allocations);
  expanded.p4_pipeline_priority_ = P4_PIPELINE_MODAL;
  load(expanded, file, false);
  assert(gif_slots[0] == &card && gif_slots[1] == &expanded);
  assert(card.animation_.get() == card_decoder);
  expanded.stop_animation_();
  assert(gif_slots[0] == &card && !gif_slots[1]);
  // Refreshing the chosen card preserves its eligibility. Releasing the page
  // allows a GIF on the next page to take the card slot.
  load(card, file, false);
  assert(gif_slots[0] == &card);
  card.animation_screen_active_ = []() { return false; };
  second.animation_screen_active_ = []() { return true; };
  load(second, file, false);
  assert(!card.animation_ && gif_slots[0] == &second);
  second.stop_animation_();
  assert(fake_esphome_allocator::external_pointers.empty());

  // A valid GIF with a large comment exercises complete-file ownership and
  // first-frame decoding above the previous 2 MiB limit without a huge fixture.
  auto large_file = file;
  std::vector<uint8_t> comment{0x21, 0xfe};
  while (comment.size() < 3 * 1024 * 1024) {
    comment.push_back(255); comment.insert(comment.end(), 255, 'a');
  }
  comment.push_back(0);
  const size_t palette_end = 13 + ((file[10] & 0x80) ? 3u * (1u << ((file[10] & 7) + 1)) : 0);
  large_file.insert(large_file.begin() + palette_end, comment.begin(), comment.end());
  ArtworkImage large_image; load(large_image, large_file, false);
  assert(std::equal(large_image.active.begin(), large_image.active.end(), expected.begin() + 6));
  large_image.stop_animation_();
  assert(fake_esphome_allocator::external_pointers.empty());

  // Prefetching overlaps decode with the displayed frame's delay. Preparing
  // a different-delay next frame must not publish early or use its delay for
  // the current frame. A refresh must replay a prepared frame, not skip it.
  for (bool refresh : {false, true}) {
    ArtworkImage timed;
    load(timed, file, false);
    const auto first = timed.active;
    const auto start = now_ms;
    now_ms += 1; timed.loop_animation_();
    assert(timed.animation_frame_ready_ && timed.active == first && !timed.cache_invalidations);
    assert(!esphome::HighFrequencyLoopRequester::is_high_frequency());
    assert(timed.animation_frame_delay_ms_ == 100 && timed.animation_->delay_ms() == 200);
    if (refresh) {
      timed.service_active_ = true; timed.pause_animation_for_refresh_();
      timed.end_connection_(); timed.service_active_ = false;
      assert(!timed.animation_frame_ready_ && timed.active == first);
    }
    now_ms = start + 99;
    timed.loop_animation_(); assert(!timed.cache_invalidations);
    now_ms = start + 100;
    timed.loop_animation_(); assert(timed.cache_invalidations == 1);
    assert(std::equal(timed.active.begin(), timed.active.end(), expected.begin() + 6 + 128));
    const auto second = timed.active;
    now_ms += 1; timed.loop_animation_();
    assert(timed.animation_frame_ready_ && timed.animation_frame_delay_ms_ == 200);
    now_ms = start + 299;
    timed.loop_animation_(); assert(timed.active == second && timed.cache_invalidations == 1);
    now_ms = start + 300;
    timed.loop_animation_(); assert(timed.cache_invalidations == 2);
    assert(std::equal(timed.active.begin(), timed.active.end(), expected.begin() + 6 + 256));
    now_ms = start + 600;
    timed.loop_animation_(); assert(timed.cache_invalidations == 3);
    const auto fourth = timed.active;
    now_ms += 1; timed.loop_animation_();
    assert(timed.animation_frame_ready_ && timed.animation_->delay_ms() == 100);
    assert(timed.animation_frame_delay_ms_ == 400);
    now_ms = start + 999;
    timed.loop_animation_(); assert(timed.active == fourth && timed.cache_invalidations == 3);
    now_ms = start + 1000;
    timed.loop_animation_(); assert(timed.cache_invalidations == 4);
    assert(std::equal(timed.active.begin(), timed.active.end(), expected.begin() + 6));
    timed.stop_animation_();
    assert(fake_esphome_allocator::external_pointers.empty());
  }

  // Refreshes keep the existing source/player until replacement promotion.
  // Exercise both an unchanged HTTP response and a failed replacement while
  // an old frame is halfway through resizing the shared staging surface.
  auto refresh_file = file;
  refresh_file[8] = 640 & 255; refresh_file[9] = 640 >> 8;
  for (int outcome = 0; outcome < 4; ++outcome) {
    ArtworkImage refreshed;
    load(refreshed, refresh_file, false);
    const auto *old_decoder = refreshed.animation_.get();
    const auto first = refreshed.active;
    now_ms += 100;
    refreshed.loop_animation_();
    assert(refreshed.animation_frame_pending_ && refreshed.decode_buffer_);
    assert(esphome::HighFrequencyLoopRequester::is_high_frequency());
    refreshed.service_active_ = true;
    refreshed.pause_animation_for_refresh_();
    assert(refreshed.animation_.get() == old_decoder && gif_slots[0] == &refreshed);
    assert(!refreshed.decode_buffer_ && !esphome::HighFrequencyLoopRequester::is_high_frequency());
    // Playback must not touch a replacement's partially decoded pixels.
    refreshed.staging.assign(32, 0xa5); refreshed.decode_buffer_ = refreshed.staging.data();
    refreshed.loop_animation_();
    assert(refreshed.staging == std::vector<uint8_t>(32, 0xa5) && refreshed.active == first);
    if (outcome == 1) {
      refreshed.decoder_ = std::make_unique<GifDecoder>(&refreshed);
      refreshed.decoder_->prepare(file.size());
      refreshed.fail_resize = true;
      assert(refreshed.decoder_->decode(file.data(), file.size()) < 0);
      refreshed.fail_resize = false;
    } else if (outcome == 2) {
      refreshed.decoder_ = std::make_unique<GifDecoder>(&refreshed);
      refreshed.decoder_->prepare(file.size());
      fake_esphome_allocator::external_available = false;
      assert(refreshed.decoder_->decode(file.data(), file.size()) == DECODE_ERROR_OUT_OF_MEMORY);
      fake_esphome_allocator::external_available = true;
    }
    // HTTP 304, decode/allocation failure and cancellation all clean up only
    // request resources. Native P4/S3 pending work must be cancelled too.
    refreshed.p4_pipeline_pending_ = refreshed.s3_transfer_pending_ = true;
    refreshed.end_connection_(); refreshed.service_active_ = false;
    assert(refreshed.animation_.get() == old_decoder && refreshed.active == first);
    for (int i = 0; i < 100 && !refreshed.cache_invalidations; ++i) refreshed.loop_animation_();
    assert(refreshed.cache_invalidations == 1 && refreshed.animation_.get() == old_decoder);
    assert(std::equal(refreshed.active.begin(), refreshed.active.begin() + 128,
                      expected.begin() + 6 + 128));
    assert(std::equal(refreshed.active.begin() + 128, refreshed.active.end(), first.begin() + 128));
    // A successfully decoded GIF claims the same slot only after promotion.
    load(refreshed, file, false);
    assert(refreshed.animation_ && gif_slots[0] == &refreshed);
    assert(std::equal(refreshed.active.begin(), refreshed.active.end(), expected.begin() + 6));
    // A successful static replacement must release the old animation too.
    refreshed.service_active_ = true; refreshed.pause_animation_for_refresh_();
    refreshed.staging.assign(128, 0x5a); refreshed.decode_buffer_ = refreshed.staging.data();
    refreshed.publish_first(); refreshed.gif_decoding_ = false;
    refreshed.replace_animation_(); refreshed.end_connection_(); refreshed.service_active_ = false;
    assert(!refreshed.animation_ && !gif_slots[0] && !gif_playing);
    assert(refreshed.active == std::vector<uint8_t>(128, 0x5a));
    assert(fake_esphome_allocator::external_pointers.empty());
  }

  // A frame spanning multiple slices requests prompt component loops. Hidden
  // or superseded playback and decoder destruction must release that request.
  auto tall = file;
  tall[8] = 640 & 255; tall[9] = 640 >> 8;
  ArtworkImage sliced;
  load(sliced, tall, false);
  sliced.animation_visible_ = []() { return true; };
  now_ms += 100;
  sliced.loop_animation_();
  assert(esphome::HighFrequencyLoopRequester::is_high_frequency());
  sliced.animation_visible_ = []() { return false; };
  sliced.loop_animation_();
  assert(!esphome::HighFrequencyLoopRequester::is_high_frequency());
  sliced.animation_visible_ = []() { return true; };
  sliced.loop_animation_();
  assert(esphome::HighFrequencyLoopRequester::is_high_frequency());
  ArtworkImage owner;
  owner.animation_visible_ = []() { return true; };
  gif_playing = &owner;
  sliced.loop_animation_();
  assert(!esphome::HighFrequencyLoopRequester::is_high_frequency());
  gif_playing = nullptr;
  sliced.loop_animation_();
  assert(esphome::HighFrequencyLoopRequester::is_high_frequency());
  sliced.stop_animation_();
  assert(!esphome::HighFrequencyLoopRequester::is_high_frequency());
  assert(fake_esphome_allocator::external_pointers.empty());

  // Exhausted PSRAM fails cleanly; it must not fall back to internal RAM.
  ArtworkImage image;
  GifDecoder decoder(&image); decoder.prepare(file.size());
  fake_esphome_allocator::external_available = false;
  assert(decoder.decode(file.data(), file.size()) == DECODE_ERROR_OUT_OF_MEMORY);
  fake_esphome_allocator::external_available = true;
  // A resize failure returns while the decoder is still alive and frees all
  // its PSRAM when the caller retires it.
  image.fail_resize = true;
  assert(decoder.decode(file.data(), file.size()) < 0);
}
'''

with tempfile.TemporaryDirectory(prefix="gif-runtime-") as directory:
    temp = Path(directory)
    (temp / "esphome/core").mkdir(parents=True)
    (temp / "esphome/core/defines.h").write_text("#pragma once\n#define USE_ARTWORK_IMAGE_GIF_SUPPORT 1\n")
    (temp / "esphome/core/hal.h").write_text(
        "#pragma once\n#include <cstdint>\nnamespace esphome { uint32_t millis(); uint32_t micros(); }\n")
    (temp / "esphome/core/color.h").write_text(
        '#pragma once\n#include "esphome/core/helpers.h"\nnamespace esphome { struct Color { uint8_t r,g,b,w; Color(uint8_t r,uint8_t g,uint8_t b,uint8_t w=0) : r(r),g(g),b(b),w(w) {} }; }\n')
    (temp / "artwork_image.h").write_text(stub)
    (temp / "gif_image.cpp").write_text((COMPONENT / "gif_image.cpp").read_text())
    (temp / "test.cpp").write_text(source + "\nnamespace esphome::artwork_image {\n" +
                                  "\n".join(methods) + "\n}\n" + main)
    executable = temp / "test"
    subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + [
        "-std=c++17", "-Wall", "-Wextra", "-Werror", "-DUSE_ARTWORK_IMAGE_GIF_SUPPORT",
        "-I", str(temp), "-I", str(COMPONENT), "-I", str(ROOT / "tests/firmware/stubs"),
        '-DGIF_FIXTURE_DIR="' + str(ROOT / "tests/firmware/fixtures/gif") + '"',
        str(temp / "gif_image.cpp"), str(temp / "test.cpp"), "-o", str(executable),
    ], check=True)
    subprocess.run([str(executable)], check=True)
print("GIF runtime: transfers, frame publication, pause/resume, arbitration, one card per screen, refresh recovery, frame timing, loop scheduling, cleanup and PSRAM failure passed")
