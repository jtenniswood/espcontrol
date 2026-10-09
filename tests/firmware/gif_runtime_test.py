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
for name in ("stop_animation_", "loop_animation_"):
    match = re.search(rf"^void ArtworkImage::{name}\(\) \{{\n.*?^\}}", artwork, re.M | re.S)
    assert match, name
    methods.append(match[0])

stub = r'''
#pragma once
#include <cstring>
#include <functional>
#include <memory>
#include <vector>
#include "gif_image.h"
#include "esphome/core/log.h"
#define ESP_LOGW(tag, ...) ((void)(tag))
namespace esphome::artwork_image {
struct ArtworkImage {
  std::unique_ptr<GifDecoder> animation_;
  bool animation_frame_pending_ = false, fail_resize = false;
  uint32_t animation_frame_started_ms_ = 0;
  std::function<bool()> animation_visible_;
  std::function<void()> animation_redraw_;
  std::vector<uint8_t> active, staging;
  uint8_t *buffer_ = nullptr, *decode_buffer_ = nullptr;
  int cache_invalidations = 0, width = 0, height = 0;
  size_t get_buffer_size_() const { return active.size(); }
  size_t get_decode_buffer_size_() const { return staging.size(); }
  void invalidate_lvgl_cache_() { ++cache_invalidations; }
  void discard_decode_buffer_() { staging.clear(); decode_buffer_ = nullptr; }
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
  image_->width = width; image_->height = height;
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
DownloadBuffer::DownloadBuffer(size_t size) : buffer_(nullptr), size_(size), unread_(0) { assert(!size); }
bool DownloadBuffer::adopt(uint8_t *buffer, size_t size) {
  assert(!buffer_); if (!buffer || !size) return false;
  buffer_ = buffer; size_ = unread_ = size; return true;
}
}
}
'''

main = r'''
using namespace esphome::artwork_image;
static std::vector<uint8_t> read(const std::string &path) {
  std::ifstream f(path, std::ios::binary); assert(f.good());
  return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
static void load(ArtworkImage &image, const std::vector<uint8_t> &file, bool unknown_length) {
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
  assert(!std::memcmp(data, file.data(), file.size()));
  assert(decoder->retain_source(data, file.size()));
  image.publish_first(); image.animation_ = std::move(decoder);
  image.animation_frame_started_ms_ = now_ms;
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
    gif_slots[0] = &image;
    const auto first = image.active;
    now_ms += 99; image.loop_animation_(); assert(redraws == 0);
    visible = false; now_ms += 1000;
    for (int i = 0; i < 100; ++i) image.loop_animation_();
    assert(redraws == 0 && image.active == first && !gif_playing);
    visible = true; now_ms += 100;
    for (int i = 0; i < 100 && !redraws; ++i) image.loop_animation_();
    assert(redraws == 1 && image.cache_invalidations == 1);
    assert(std::equal(image.active.begin(), image.active.end(), expected.begin() + 6 + 128));
    // A second visible decoder yields while this image owns playback.
    ArtworkImage other; load(other, file, false);
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
        "#pragma once\n#include <cstdint>\nnamespace esphome { uint32_t millis(); }\n")
    (temp / "esphome/core/color.h").write_text(
        '#pragma once\n#include "esphome/core/helpers.h"\nnamespace esphome { struct Color {}; }\n')
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
print("GIF runtime: complete transfers, frame publication, pause/resume, arbitration, cleanup and PSRAM failure passed")
