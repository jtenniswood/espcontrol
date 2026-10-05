#!/usr/bin/env python3
"""Track allocations through the production ArtworkImage release methods."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
implementation = (root / "components/artwork_image/artwork_image.cpp").read_text()
methods = []
for name in ("release", "end_connection_", "discard_decode_buffer_", "retire_active_buffer_",
             "cleanup_retired_buffers_", "retired_buffer_bytes_", "limit_retired_buffers_"):
    match = re.search(rf"^(?:void|size_t) ArtworkImage::{name}\([^\n]*\).*?^\}}", implementation,
                      re.M | re.S)
    assert match, name
    methods.append(match[0])

source = r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#define ESP_LOGI(...) do {} while(false)
#define ESP_LOGW(...) do {} while(false)
constexpr uint32_t RETIRED_BUFFER_GRACE_MS = 300;
constexpr size_t MAX_RETIRED_IMAGE_BUFFERS = 1;
uint32_t millis() { return 0; }
std::unordered_map<void*, size_t> live;
uint8_t *allocate(size_t size) {
  auto *p = static_cast<uint8_t*>(malloc(size));
  assert(p && live.emplace(p, size).second);
  return p;
}
void deallocate(uint8_t *p, size_t size) {
  assert(live.at(p) == size);
  assert(live.erase(p) == 1);
  free(p);
}
struct Allocator { void deallocate(uint8_t *p, size_t size) { ::deallocate(p, size); } };
struct Decoder {
  uint8_t *scratch = allocate(48);
  ~Decoder() { deallocate(scratch, 48); }
};
struct DownloadBuffer {
  uint8_t *data = nullptr;
  size_t capacity = 0;
  void reset() {}
  void shrink_to(size_t size) {
    assert(size == 0);
    if (data) deallocate(data, capacity);
    data = nullptr;
    capacity = 0;
  }
};
struct Downloader { bool ended = false; void end() { ended = true; } };
struct ArtworkImage {
  struct RetiredBuffer { uint8_t *data; size_t size; uint32_t retired_at; };
  Allocator allocator_;
  uint8_t *buffer_ = nullptr, *decode_buffer_ = nullptr;
  const uint8_t *data_start_ = nullptr;
  int buffer_width_ = 0, buffer_height_ = 0, width_ = 0, height_ = 0;
  int buffer_content_width_ = 0, buffer_content_height_ = 0;
  int buffer_offset_x_ = 0, buffer_offset_y_ = 0;
  int decode_buffer_width_ = 0, decode_buffer_height_ = 0;
  int decode_content_width_ = 0, decode_content_height_ = 0;
  int decode_offset_x_ = 0, decode_offset_y_ = 0;
  bool update_pending_ = false, service_pending_ = false, cache_invalidated = false;
  std::string pending_url_;
  std::vector<RetiredBuffer> retired_buffers_;
  std::unique_ptr<Decoder> decoder_;
  DownloadBuffer download_buffer_;
  Downloader *downloader_ = nullptr;
  void cancel_s3_transfer_() {}
  void cancel_p4_pipeline_() {}
  void cancel_service_request_() { service_pending_ = false; }
  void invalidate_lvgl_cache_() { cache_invalidated = true; }
  void enable_loop() {}
  size_t get_buffer_size_() const { return buffer_width_ * buffer_height_ * 2; }
  size_t get_decode_buffer_size_() const { return decode_buffer_width_ * decode_buffer_height_ * 2; }
  void release();
  void end_connection_();
  void discard_decode_buffer_();
  void retire_active_buffer_();
  void cleanup_retired_buffers_(bool);
  size_t retired_buffer_bytes_() const;
  void limit_retired_buffers_();
};
''' + "\n".join(methods) + r'''
int main() {
  for (int cycle = 0; cycle < 300; ++cycle) {
    ArtworkImage image;
    Downloader downloader;
    image.downloader_ = &downloader;
    image.update_pending_ = image.service_pending_ = true;
    image.pending_url_ = "http://pending/image.jpg";
    image.buffer_width_ = 20;
    image.buffer_height_ = 30;
    image.buffer_ = allocate(1200);
    image.data_start_ = image.buffer_;
    image.decode_buffer_width_ = image.decode_buffer_height_ = 10;
    image.decode_buffer_ = allocate(200);
    image.retired_buffers_.push_back({allocate(400), 400, 0});
    image.decoder_ = std::make_unique<Decoder>();
    image.download_buffer_.data = allocate(128);
    image.download_buffer_.capacity = 128;
    image.release();
    assert(live.empty());
    assert(downloader.ended && image.downloader_ == nullptr);
    assert(image.cache_invalidated && image.data_start_ == nullptr);
    assert(image.buffer_ == nullptr && image.decode_buffer_ == nullptr);
    assert(image.retired_buffers_.empty() && image.decoder_ == nullptr);
    assert(!image.update_pending_ && !image.service_pending_ && image.pending_url_.empty());
    assert(image.download_buffer_.capacity == 0);
    image.release();  // Repeated stop/disable must not double-free anything.
    assert(live.empty());
  }
}
'''
with tempfile.TemporaryDirectory(prefix="artwork-release-") as temp:
    cpp = Path(temp) / "release.cpp"
    cpp.write_text(source)
    binary = Path(temp) / "release"
    subprocess.run([sys.argv[1], "-std=c++17", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                    str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    print("Artwork release: active, retired, decoder and transfer allocations freed across 300 cycles")
