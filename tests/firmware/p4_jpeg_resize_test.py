"""Exercise the production JPEG hardware branch with simulated ESP-IDF I/O."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
component = root / "components/artwork_image"
production = (component / "jpeg_image.cpp").read_text()
start = production.index("int JpegDecoder::decode_hardware_(")
end = production.index("\n#endif", start)

source = r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "image_pipeline_policy.h"
template<typename... Args> void log_unused(Args...) {}
#define ESP_LOGI(...) log_unused(__VA_ARGS__)
#define ESP_LOGD(...) log_unused(__VA_ARGS__)
#define ESP_LOGW(...) log_unused(__VA_ARGS__)
static const char *const TAG="test";
#define ESP_OK 0
#define ESP_ERR_NOT_SUPPORTED 1
#define JPEG_DOWN_SAMPLING_GRAY 1
#define JPEG_DEC_ALLOC_INPUT_BUFFER 0
#define JPEG_DEC_ALLOC_OUTPUT_BUFFER 1
#define JPEG_DECODE_OUT_FORMAT_RGB565 0
#define JPEG_DEC_RGB_ELEMENT_ORDER_RGB 0
#define JPEG_DEC_RGB_ELEMENT_ORDER_BGR 1
#define JPEG_YUV_RGB_CONV_STD_BT601 0
using jpeg_decoder_handle_t = void *;
struct jpeg_decode_picture_info_t { uint32_t width=640, height=640; int sample_method=0; } info;
struct jpeg_decode_cfg_t { int output_format, rgb_order, conv_std; };
using esp_err_t = int;
int ppa_calls = 0;
bool scaler_available = true;
int last_rgb_order = -1;
uint32_t millis() { return 0; }
jpeg_decoder_handle_t p4_jpeg_decoder() { return &info; }
uint32_t align_up(uint32_t value, uint32_t alignment) { return (value+alignment-1)/alignment*alignment; }
int jpeg_decoder_get_info(uint8_t *, size_t, jpeg_decode_picture_info_t *out) { *out=info; return ESP_OK; }
int jpeg_decoder_process(jpeg_decoder_handle_t, jpeg_decode_cfg_t *config, uint8_t *, size_t, uint8_t *, size_t capacity, uint32_t *out) {
  last_rgb_order=config->rgb_order; *out=capacity; return ESP_OK;
}
namespace esphome {
namespace image { enum ImageType { IMAGE_TYPE_RGB565, IMAGE_TYPE_RGB }; }
namespace artwork_image {
enum ImageResizeMode { FIT, COVER };
constexpr int DECODE_ERROR_OUT_OF_MEMORY=-3;
struct P4JpegWorkspace {
 uint8_t *input=nullptr, *output=nullptr, *scaled=nullptr;
 size_t input_capacity=0, output_capacity=0, scaled_capacity=0;
} memory;
P4JpegWorkspace &p4_jpeg_workspace() { return memory; }
void p4_release_jpeg_workspace() {
 free(memory.input); free(memory.output); free(memory.scaled); memory={};
}
bool p4_ensure_jpeg_buffer(uint8_t *&data, size_t &capacity, size_t required, int) {
 if (capacity < required) { free(data); data=static_cast<uint8_t *>(malloc(required)); capacity=required; }
 return data != nullptr;
}
bool p4_scale_rgb565(const uint8_t *, uint32_t, uint32_t width, uint32_t height,
                    uint32_t target_width, uint32_t target_height, bool cover,
                    uint8_t *&data, size_t &capacity) {
 ++ppa_calls;
 const auto plan=p4_image_scale_plan(width,height,target_width,target_height,16,4095,cover);
 return scaler_available && plan.valid && p4_ensure_jpeg_buffer(data,capacity,target_width*target_height*2,0);
}
struct ArtworkImage {
 int width=800, height=800;
 bool big_endian=false;
 ImageResizeMode mode=FIT;
 image::ImageType type=image::IMAGE_TYPE_RGB565;
 int get_fixed_width() { return width; }
 int get_fixed_height() { return height; }
 ImageResizeMode get_resize_mode() { return mode; }
 image::ImageType image_type() { return type; }
 bool is_big_endian() { return big_endian; }
};
struct JpegDecoder {
 ArtworkImage *image_;
 size_t decoded_bytes_=0;
 int frame_width=0, frame_height=0;
 bool set_size(int, int) { return true; }
 void draw_rgb565_frame(int width,int height,size_t,const uint8_t *) { frame_width=width; frame_height=height; }
 bool has_failed() { return false; }
 int decode_hardware_(uint8_t *,size_t);
};
'''
source += production[start:end]
source += r'''
}}
int main() {
 using namespace esphome::artwork_image;
 uint8_t input[12]{};
 for (bool big_endian : {false,true}) {
   ArtworkImage image; image.big_endian=big_endian;
   JpegDecoder decoder{&image};
   ppa_calls=0;
   assert(decoder.decode_hardware_(input,sizeof(input)) == sizeof(input));
   assert(ppa_calls == 1 && decoder.frame_width == 800 && decoder.frame_height == 800);
   assert(last_rgb_order == (big_endian ? JPEG_DEC_RGB_ELEMENT_ORDER_RGB : JPEG_DEC_RGB_ELEMENT_ORDER_BGR));
   assert(decoder.decoded_bytes_ == sizeof(input));
 }
 // Letterboxed FIT keeps the source geometry for the filtered resizer.
 info.height=480;
 ArtworkImage image;
 JpegDecoder fit{&image};
 assert(fit.decode_hardware_(input,sizeof(input)) == sizeof(input));
 assert(fit.frame_width == 640 && fit.frame_height == 480);
 // COVER continues to return a completely filled 800x800 surface.
 image.mode=COVER;
 JpegDecoder cover{&image};
 assert(cover.decode_hardware_(input,sizeof(input)) == sizeof(input));
 assert(cover.frame_width == 800 && cover.frame_height == 800);
 // A failed PPA operation still falls back to the source frame.
 scaler_available=false;
 JpegDecoder fallback{&image};
 assert(fallback.decode_hardware_(input,sizeof(input)) == sizeof(input));
 assert(fallback.frame_width == 640 && fallback.frame_height == 480);
 p4_release_jpeg_workspace();
}
'''

with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / "test.cpp"
    binary = Path(tmp) / "test"
    cpp.write_text(source)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "c++", "-std=c++17",
                    "-Wall", "-Wextra", "-Werror", "-I", str(component),
                    str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
