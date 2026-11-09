"""Exercise production first-frame publication with controlled allocation pressure."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
component = root / "components/artwork_image"
implementation = (component / "artwork_image.cpp").read_text()
method = re.search(r"^void ArtworkImage::finish_download_\(\) \{.*?^\}", implementation, re.M | re.S)
assert method
source = r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include "image_pipeline_policy.h"
#include "transfer_observer.h"
using namespace esphome::artwork_image;
#define USE_ARTWORK_IMAGE_GIF_SUPPORT
#ifdef TEST_S3
#define USE_ESP_IDF
#define CONFIG_IDF_TARGET_ESP32S3
#endif
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGD(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
uint32_t millis() { return 100; }
[[maybe_unused]] constexpr unsigned MALLOC_CAP_SPIRAM = 1, MALLOC_CAP_8BIT = 2;
size_t psram_free = 1024 * 1024;
size_t heap_caps_get_free_size(unsigned) { return psram_free; }
struct Application { void feed_wdt() {} } App;
std::unordered_map<uint8_t*, size_t> live;
uint8_t* allocate(size_t size) {
  auto* p = static_cast<uint8_t*>(malloc(size));
  assert(p && live.emplace(p, size).second); return p;
}
void deallocate(uint8_t* p, size_t size) {
  if (!p) return;
  assert(live.at(p) == size && live.erase(p) == 1); free(p);
}
struct Allocator {
  bool fail = false;
  unsigned attempts = 0;
  uint8_t* allocate(size_t size) { ++attempts; return fail ? nullptr : ::allocate(size); }
  void deallocate(uint8_t* p, size_t size) { ::deallocate(p, size); }
};
struct Decoder { virtual ~Decoder() = default; };
struct GifDecoder : Decoder {
  bool animated = true;
  bool is_animated() const { return animated; }
};
struct Downloader { size_t get_bytes_read() { return 128; } };
struct ArtworkImage {
  Allocator allocator_;
  std::unique_ptr<Decoder> decoder_ = std::make_unique<GifDecoder>();
  std::function<bool()> animation_visible_ = [] { return true; };
  bool gif_decoding_ = true, animation_ = false, claimable = true;
  bool reclaim_hidden_owner = false;
  bool newer = false, reject_promotion = false, rebind_during_cleanup = false;
  bool notified = false, succeeded = false;
  unsigned promotions = 0, completes = 0;
  uint32_t service_generation_ = 1;
  uint32_t transfer_complete_ms_ = 0;
  size_t completed_transfer_bytes_ = 128;
  Downloader* downloader_ = nullptr;
  TransferFailure transfer_failure_ = TransferFailure::CONTENT;
  uint8_t* buffer_ = ::allocate(64);
  uint8_t* decode_buffer_ = ::allocate(64);
  int buffer_width_ = 8, buffer_height_ = 4, buffer_content_width_ = 8, buffer_content_height_ = 4;
  int buffer_offset_x_ = 0, buffer_offset_y_ = 0;
  int decode_buffer_width_ = 8, decode_buffer_height_ = 4;
  int decode_content_width_ = 8, decode_content_height_ = 4;
  int decode_offset_x_ = 0, decode_offset_y_ = 0;
  std::vector<uint8_t*> retired;
  ~ArtworkImage() {
    ::deallocate(buffer_, 64); ::deallocate(decode_buffer_, 64);
    for (auto* p : retired) ::deallocate(p, 64);
  }
  bool has_newer_pending_update_() const { return newer; }
  int prepare_animation_slot_(bool) {
    if (reclaim_hidden_owner) allocator_.fail = false;
    return claimable ? 0 : -1;
  }
  size_t get_decode_buffer_size_() const { return size_t(decode_buffer_width_) * decode_buffer_height_ * 2; }
  void discard() {
    ::deallocate(decode_buffer_, get_decode_buffer_size_()); decode_buffer_ = nullptr;
    decode_buffer_width_ = decode_buffer_height_ = 0;
  }
  bool promote_decode_buffer_() {
    if (reject_promotion) return false;
    ++promotions; retired.push_back(buffer_); buffer_ = decode_buffer_; decode_buffer_ = nullptr;
    decode_buffer_width_ = decode_buffer_height_ = 0; return true;
  }
  void fail_download_() { discard(); decoder_.reset(); notified = true; succeeded = false; }
  void complete_service_request_() { ++completes; }
  void replace_animation_() {
    discard();
    animation_ = gif_decoding_ && animation_visible_ && claimable &&
                 static_cast<GifDecoder*>(decoder_.get())->is_animated();
  }
  void end_connection_() {
    discard(); decoder_.reset(); gif_decoding_ = false;
    if (rebind_during_cleanup) ++service_generation_;
  }
  void log_timing_(const char*, size_t) {}
  void log_state_(const char*) {}
  void notify_result_(bool success, bool) { notified = true; succeeded = success; }
  void finish_download_();
};
''' + method[0] + r'''
int main() {
#ifdef TEST_S3
  for (bool headroom_failure : {false, true}) {
    ArtworkImage image;
    const auto* previous = image.buffer_;
    image.allocator_.fail = !headroom_failure;
    psram_free = headroom_failure ? IMAGE_PIPELINE_S3_PSRAM_HEADROOM_BYTES - 1 : 1024 * 1024;
    image.finish_download_();
    assert(image.notified && !image.succeeded && image.promotions == 0);
    assert(image.buffer_ == previous && image.transfer_failure_ == TransferFailure::RESOURCE);
    assert(live.size() == 1);  // Neither failed first frame nor reservation leaks.
  }
  assert(live.empty());
  {
    psram_free = 1024 * 1024;
    ArtworkImage image;
    image.allocator_.fail = true;
    image.reclaim_hidden_owner = true;
    image.finish_download_();
    assert(image.notified && image.succeeded && image.decode_buffer_);
  }
  assert(live.empty());
#endif
  psram_free = 1024 * 1024;
  {
    ArtworkImage image;
    image.finish_download_();
    assert(image.notified && image.succeeded && image.promotions == 1 && image.animation_);
#ifdef TEST_S3
    assert(image.decode_buffer_ && image.get_decode_buffer_size_() == 64);
    assert(image.decode_content_width_ == image.buffer_content_width_);
    assert(image.decode_content_height_ == image.buffer_content_height_);
    assert(image.allocator_.attempts == 1);
#else
    assert(!image.decode_buffer_ && image.allocator_.attempts == 0);
#endif
  }
  assert(live.empty());
  for (int reason = 0; reason < 3; ++reason) {
    ArtworkImage image;
    if (reason == 0) image.claimable = false;
    if (reason == 1) static_cast<GifDecoder*>(image.decoder_.get())->animated = false;
    if (reason == 2) image.animation_visible_ = {};
    image.allocator_.fail = true;
    image.finish_download_();
    assert(image.notified && image.succeeded && !image.animation_);
    assert(image.allocator_.attempts == 0 && !image.decode_buffer_);
  }
  assert(live.empty());
  {
    ArtworkImage image;
    const auto* previous = image.buffer_;
    image.reject_promotion = true; image.finish_download_();
    assert(image.notified && !image.succeeded && image.buffer_ == previous && live.size() == 1);
  }
  assert(live.empty());
  {
    ArtworkImage image;
    image.newer = true; image.finish_download_();
    assert(!image.notified && image.completes == 1 && image.promotions == 0);
    assert(image.allocator_.attempts == 0);
  }
  assert(live.empty());
#ifdef TEST_S3
  {
    ArtworkImage image;
    image.rebind_during_cleanup = true; image.finish_download_();
    assert(!image.notified && !image.decode_buffer_);
  }
  assert(live.empty());
#endif
}
'''
with tempfile.TemporaryDirectory(prefix="artwork-gif-admission-") as temp:
    cpp = Path(temp) / "admission.cpp"
    cpp.write_text(source)
    for mode in ("normal", "s3"):
        binary = Path(temp) / mode
        command = [sys.argv[1], "-std=c++17", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                   "-I" + str(component), str(cpp), "-o", str(binary)]
        if mode == "s3":
            command.append("-DTEST_S3")
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)
    print("GIF first-frame admission: allocation/headroom refusal, cleanup, eligibility and publication passed")
