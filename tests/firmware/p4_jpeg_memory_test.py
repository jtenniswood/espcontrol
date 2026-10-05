"""Run the production P4 JPEG decode path with tracked hardware allocations."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
production = (root / "components/artwork_image/jpeg_image.cpp").read_text()
workspace = production.split("struct P4JpegWorkspace {", 1)[1].split(
    "static ppa_client_handle_t p4_ppa_scaler()", 1
)[0]
decode = production.split("int JpegDecoder::decode_hardware_", 1)[1].split("\n#endif", 1)[0]

source = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
static std::unordered_map<void *, size_t> live;
static int allocation_count = 0, fail_allocation = 0;
static void *tracked_alloc(size_t size) {
  if (++allocation_count == fail_allocation) return nullptr;
  void *ptr = std::malloc(size);
  assert(ptr);
  live[ptr] = size;
  return ptr;
}
static void tracked_free(void *ptr) {
  if (!ptr) return;
  assert(live.erase(ptr) == 1);
  std::free(ptr);
}
#define free tracked_free
#define heap_caps_free tracked_free
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGD(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
using esp_err_t = int;
using jpeg_decoder_handle_t = void *;
using jpeg_dec_buffer_alloc_direction_t = int;
constexpr int ESP_OK = 0, ESP_ERR_NOT_SUPPORTED = 1;
constexpr int JPEG_DEC_ALLOC_INPUT_BUFFER = 0, JPEG_DEC_ALLOC_OUTPUT_BUFFER = 1;
constexpr int JPEG_DOWN_SAMPLING_GRAY = 1, JPEG_DECODE_OUT_FORMAT_RGB565 = 0;
constexpr int JPEG_DEC_RGB_ELEMENT_ORDER_RGB = 0, JPEG_DEC_RGB_ELEMENT_ORDER_BGR = 1;
constexpr int JPEG_YUV_RGB_CONV_STD_BT601 = 0, DECODE_ERROR_OUT_OF_MEMORY = -3;
struct jpeg_decode_memory_alloc_cfg_t { int buffer_direction; };
struct jpeg_decode_picture_info_t { uint32_t width, height; int sample_method; };
struct jpeg_decode_cfg_t { int output_format, rgb_order, conv_std; };
static uint32_t source_size = 64;
static bool driver_available = true, valid_header = true, scaling_succeeds = true;
static int driver_error = ESP_OK;
static jpeg_decoder_handle_t p4_jpeg_decoder() {
  return driver_available ? &driver_available : nullptr;
}
static bool p4_jpeg_hardware_target_supported(bool rgb565) { return rgb565; }
static uint32_t align_up(uint32_t n, uint32_t alignment) {
  return (n + alignment - 1) / alignment * alignment;
}
static uint32_t millis() { return 1; }
static void *jpeg_alloc_decoder_mem(size_t size, jpeg_decode_memory_alloc_cfg_t *, size_t *capacity) {
  auto *p = tracked_alloc(size);
  *capacity = p ? size : 0;
  return p;
}
static int jpeg_decoder_get_info(uint8_t *, size_t, jpeg_decode_picture_info_t *info) {
  *info = {source_size, source_size, 0};
  return valid_header ? ESP_OK : ESP_ERR_NOT_SUPPORTED;
}
static int jpeg_decoder_process(jpeg_decoder_handle_t, jpeg_decode_cfg_t *, uint8_t *,
                                size_t, uint8_t *, size_t capacity, uint32_t *written) {
  *written = capacity;
  return driver_error;
}
namespace image { enum class ImageType { IMAGE_TYPE_RGB565, OTHER }; }
enum class ImageResizeMode { FIT, COVER };
struct Image {
  bool rgb565 = true;
  ImageResizeMode mode = ImageResizeMode::FIT;
  image::ImageType image_type() const {
    return rgb565 ? image::ImageType::IMAGE_TYPE_RGB565 : image::ImageType::OTHER;
  }
  bool is_big_endian() const { return false; }
  int get_fixed_width() const { return 32; }
  int get_fixed_height() const { return 32; }
  ImageResizeMode get_resize_mode() const { return mode; }
};
struct JpegDecoder {
  Image *image_;
  size_t decoded_bytes_ = 0;
  bool size_succeeds = true, draw_fails = false, drew_frame = false;
  bool set_size(int, int) { return size_succeeds; }
  bool has_failed() const { return draw_fails; }
  void draw_rgb565_frame(int, int, size_t, const uint8_t *data) {
    assert(live.count(const_cast<uint8_t *>(data)) == 1);
    drew_frame = true;  // The scratch must still be alive during the copy.
  }
  int decode_hardware_(uint8_t *, size_t);
};
static bool p4_scale_rgb565(const uint8_t *, uint32_t, uint32_t, uint32_t,
                           uint32_t w, uint32_t h, uint8_t *&scaled, size_t &capacity) {
  capacity = w * h * 2;
  scaled = static_cast<uint8_t *>(tracked_alloc(capacity));
  return scaled && scaling_succeeds;
}
'''
source += "struct P4JpegWorkspace {" + workspace
source += "int JpegDecoder::decode_hardware_" + decode
source += r'''
int main() {
  uint8_t compressed[128]{};
  Image image;
  JpegDecoder decoder{&image};
  auto run = [&](int expected) {
    assert(decoder.decode_hardware_(compressed, sizeof(compressed)) == expected);
    assert(live.empty());  // Success, fallback and error must all return scratch.
  };
  for (int i = 0; i < 300; ++i) {
    source_size = 64 + (i % 8) * 16;
    image.mode = i % 2 ? ImageResizeMode::COVER : ImageResizeMode::FIT;
    decoder.drew_frame = false;
    run(sizeof(compressed));
    assert(decoder.drew_frame && decoder.decoded_bytes_ == sizeof(compressed));
  }
  // Fail input, output and scaling allocation. Scaling can fall back to CPU.
  image.mode = ImageResizeMode::COVER;
  for (int nth = 1; nth <= 3; ++nth) {
    fail_allocation = allocation_count + nth;
    run(nth == 3 ? sizeof(compressed) : 0);
  }
  fail_allocation = 0;
  driver_error = ESP_ERR_NOT_SUPPORTED;
  run(0);
  driver_error = ESP_OK;
  for (auto mode : {ImageResizeMode::FIT, ImageResizeMode::COVER}) {
    image.mode = mode;
    decoder.size_succeeds = false;
    run(DECODE_ERROR_OUT_OF_MEMORY);
    decoder.size_succeeds = true;
    decoder.draw_fails = true;
    run(DECODE_ERROR_OUT_OF_MEMORY);
    decoder.draw_fails = false;
  }
  scaling_succeeds = false;
  run(sizeof(compressed));
  valid_header = false;
  run(0);
  valid_header = true;
  driver_available = false;
  run(0);
  driver_available = true;
  image.rgb565 = false;
  run(0);
}
'''

with tempfile.TemporaryDirectory() as tmp:
    cpp, binary = Path(tmp) / "test.cpp", Path(tmp) / "test"
    cpp.write_text(source)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "c++", "-std=c++17",
                    "-Wall", "-Wextra", "-Werror", "-Wno-unused-variable",
                    str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
