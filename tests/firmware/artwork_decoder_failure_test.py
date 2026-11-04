"""Exercise production decoder error classification with controlled backends."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
component = root / "components/artwork_image"
implementation = (component / "artwork_image.cpp").read_text()
methods = []
for name in ("create_decoder_", "decode_buffered_data_"):
    match = re.search(rf"^bool ArtworkImage::{name}\(.*?^\}}", implementation, re.M | re.S)
    assert match, name
    methods.append(match[0])
errors = re.search(r"enum DecodeError : int \{.*?\};",
                   (component / "image_decoder.h").read_text(), re.S)
assert errors

source = r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include "transfer_observer.h"
using namespace esphome::artwork_image;
using std::make_unique;
#define USE_ARTWORK_IMAGE_GIF_SUPPORT
#define USE_ARTWORK_IMAGE_JPEG_SUPPORT
#define USE_ARTWORK_IMAGE_PNG_SUPPORT
#define USE_ARTWORK_IMAGE_BMP_SUPPORT
#define ESP_LOGD(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
uint32_t millis() { return 50; }
''' + errors[0] + r'''
enum class ImageFormat { AUTO, GIF, JPEG, PNG, BMP, HEIC };
struct ArtworkImage;
int prepare_result = 0, decode_result = 0, decoders_alive = 0;
struct Decoder {
  explicit Decoder(ArtworkImage*) { ++decoders_alive; }
  ~Decoder() { --decoders_alive; }
  int prepare(size_t) { return prepare_result; }
  int decode(uint8_t*, size_t) { return decode_result; }
};
using GifDecoder = Decoder;
using JpegDecoder = Decoder;
using PngDecoder = Decoder;
using BmpDecoder = Decoder;
namespace esphome { using std::make_unique; }
struct DownloadBuffer {
  std::array<uint8_t, 8> bytes{};
  size_t offset = 0;
  size_t unread() const { return bytes.size() - offset; }
  uint8_t* data() { return bytes.data() + offset; }
  void read(size_t count) { assert(count <= unread()); offset += count; }
};
struct ArtworkImage {
  bool gif_decoding_ = false;
  uint32_t decode_started_ms_ = 0;
  std::unique_ptr<Decoder> decoder_;
  DownloadBuffer download_buffer_;
  TransferFailure transfer_failure_ = TransferFailure::CONTENT;
  void suspend_hidden_animations_() {}
  bool create_decoder_(ImageFormat, size_t);
  bool decode_buffered_data_();
};
''' + "\n".join(methods) + r'''
int main() {
  for (auto format : {ImageFormat::GIF, ImageFormat::JPEG, ImageFormat::PNG, ImageFormat::BMP}) {
    for (int error : {DECODE_ERROR_INVALID_TYPE, DECODE_ERROR_UNSUPPORTED_FORMAT,
                      DECODE_ERROR_OUT_OF_MEMORY}) {
      ArtworkImage image;
      prepare_result = error;
      assert(!image.create_decoder_(format, 8));
      assert(!image.decoder_ && decoders_alive == 0);
      assert(image.download_buffer_.unread() == 8);
      assert(image.transfer_failure_ == (error == DECODE_ERROR_OUT_OF_MEMORY
                                              ? TransferFailure::RESOURCE : TransferFailure::CONTENT));
    }
    for (int error : {DECODE_ERROR_INVALID_TYPE, DECODE_ERROR_UNSUPPORTED_FORMAT,
                      DECODE_ERROR_OUT_OF_MEMORY}) {
      ArtworkImage image;
      prepare_result = 0;
      assert(image.create_decoder_(format, 8));
      decode_result = error;
      assert(!image.decode_buffered_data_());
      assert(image.download_buffer_.unread() == 8 && image.decode_started_ms_ == 50);
      assert(image.transfer_failure_ == (error == DECODE_ERROR_OUT_OF_MEMORY
                                              ? TransferFailure::RESOURCE : TransferFailure::CONTENT));
    }
    for (int consumed : {0, 3, 8, 9}) {
      ArtworkImage image;
      prepare_result = 0;
      assert(image.create_decoder_(format, 8));
      decode_result = consumed;
      assert(image.decode_buffered_data_() == (consumed <= 8));
      assert(image.transfer_failure_ == TransferFailure::CONTENT);
      assert(image.download_buffer_.unread() == static_cast<size_t>(consumed <= 8 ? 8 - consumed : 8));
    }
  }
  assert(decoders_alive == 0);
  ArtworkImage empty;
  assert(empty.decode_buffered_data_());
  assert(!empty.create_decoder_(ImageFormat::HEIC, 8));
  assert(!empty.create_decoder_(ImageFormat::AUTO, 8));
  assert(empty.transfer_failure_ == TransferFailure::CONTENT);
}
'''
with tempfile.TemporaryDirectory(prefix="artwork-decoder-failure-") as temp:
    cpp = Path(temp) / "decoder.cpp"
    cpp.write_text(source)
    binary = Path(temp) / "decoder"
    subprocess.run([sys.argv[1], "-std=c++17", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                    "-I" + str(component), str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    print("Decoder preparation/decode OOM, content failures and successful consumption passed")
