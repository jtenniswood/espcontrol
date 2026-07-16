#include "p4_jpeg_backend.h"
#include "p4_scaling.h"

#if defined(USE_ESP_IDF) && defined(CONFIG_IDF_TARGET_ESP32P4)

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "driver/jpeg_decode.h"
#include "driver/ppa.h"
#include "esp_heap_caps.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace artwork_image {

static const char *const TAG = "artwork_image.p4_jpeg";

struct P4JpegWorkspace {
  uint8_t *input{nullptr};
  size_t input_capacity{0};
  uint8_t *output{nullptr};
  size_t output_capacity{0};
  uint8_t *scaled{nullptr};
  size_t scaled_capacity{0};
};

static P4JpegWorkspace &workspace() {
  static P4JpegWorkspace value;
  return value;
}

static jpeg_decoder_handle_t decoder() {
  static jpeg_decoder_handle_t handle = nullptr;
  static bool attempted = false;
  if (!attempted) {
    attempted = true;
    jpeg_decode_engine_cfg_t config{};
    config.intr_priority = 0;
    config.timeout_ms = 100;
    esp_err_t error = jpeg_new_decoder_engine(&config, &handle);
    if (error != ESP_OK) {
      ESP_LOGW(TAG, "JPEG hardware unavailable (%d); using software", error);
      handle = nullptr;
    }
  }
  return handle;
}

static ppa_client_handle_t scaler() {
  static ppa_client_handle_t handle = nullptr;
  static bool attempted = false;
  if (!attempted) {
    attempted = true;
    ppa_client_config_t config{};
    config.oper_type = PPA_OPERATION_SRM;
    config.max_pending_trans_num = 1;
    if (ppa_register_client(&config, &handle) != ESP_OK) handle = nullptr;
  }
  return handle;
}

static uint32_t align_up(uint32_t value, uint32_t alignment) {
  return (value + alignment - 1) / alignment * alignment;
}

static bool ensure_decoder_buffer(uint8_t *&buffer, size_t &capacity, size_t required,
                                  jpeg_dec_buffer_alloc_direction_t direction) {
  if (buffer != nullptr && capacity >= required) return true;
  free(buffer);
  buffer = nullptr;
  capacity = 0;
  jpeg_decode_memory_alloc_cfg_t config{};
  config.buffer_direction = direction;
  buffer = static_cast<uint8_t *>(jpeg_alloc_decoder_mem(required, &config, &capacity));
  return buffer != nullptr && capacity >= required;
}

static bool scale_cover(const uint8_t *source, uint32_t stride_pixels, uint32_t source_width,
                        uint32_t source_height, uint32_t target_width, uint32_t target_height,
                        P4JpegWorkspace &memory) {
  ppa_client_handle_t handle = scaler();
  if (handle == nullptr || target_width == 0 || target_height == 0) return false;

  uint32_t crop_width = source_width;
  uint32_t crop_height = source_height;
  if (static_cast<uint64_t>(source_width) * target_height >
      static_cast<uint64_t>(source_height) * target_width) {
    crop_width = std::max<uint32_t>(1, static_cast<uint64_t>(source_height) * target_width / target_height);
  } else {
    crop_height = std::max<uint32_t>(1, static_cast<uint64_t>(source_width) * target_height / target_width);
  }

  uint32_t scale_x_steps = p4::exact_scale_steps(crop_width, target_width);
  uint32_t scale_y_steps = p4::exact_scale_steps(crop_height, target_height);
  if (scale_x_steps == 0 || scale_y_steps == 0) {
    ESP_LOGD(TAG, "PPA cannot exactly scale %ux%u to %ux%u; using CPU scaling",
             crop_width, crop_height, target_width, target_height);
    return false;
  }

  static constexpr size_t ALIGNMENT = 64;
  size_t target_size = static_cast<size_t>(target_width) * target_height * 2;
  size_t required = (target_size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
  if (memory.scaled == nullptr || memory.scaled_capacity < required) {
    heap_caps_free(memory.scaled);
    memory.scaled = static_cast<uint8_t *>(
        heap_caps_aligned_alloc(ALIGNMENT, required, MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM));
    memory.scaled_capacity = memory.scaled == nullptr ? 0 : required;
  }
  if (memory.scaled == nullptr) return false;

  ppa_srm_oper_config_t config{};
  config.in.buffer = source;
  config.in.pic_w = stride_pixels;
  config.in.pic_h = source_height;
  config.in.block_w = crop_width;
  config.in.block_h = crop_height;
  config.in.block_offset_x = (source_width - crop_width) / 2;
  config.in.block_offset_y = (source_height - crop_height) / 2;
  config.in.srm_cm = PPA_SRM_COLOR_MODE_RGB565;
  config.out.buffer = memory.scaled;
  config.out.buffer_size = memory.scaled_capacity;
  config.out.pic_w = target_width;
  config.out.pic_h = target_height;
  config.out.srm_cm = PPA_SRM_COLOR_MODE_RGB565;
  config.rotation_angle = PPA_SRM_ROTATION_ANGLE_0;
  config.scale_x = static_cast<float>(scale_x_steps) / p4::SCALE_FRACTION_STEPS;
  config.scale_y = static_cast<float>(scale_y_steps) / p4::SCALE_FRACTION_STEPS;
  config.mode = PPA_TRANS_MODE_BLOCKING;
  esp_err_t error = ppa_do_scale_rotate_mirror(handle, &config);
  if (error != ESP_OK) {
    ESP_LOGW(TAG, "PPA image scaling failed (%d); using CPU scaling", error);
    return false;
  }
  return true;
}

bool try_decode_p4_jpeg(const uint8_t *data, size_t size, int target_width, int target_height,
                       bool cover_mode, bool big_endian, HardwareJpegFrame &frame) {
  jpeg_decoder_handle_t handle = decoder();
  if (handle == nullptr || data == nullptr || size == 0) return false;

  jpeg_decode_picture_info_t info{};
  if (jpeg_decoder_get_info(data, size, &info) != ESP_OK || info.width == 0 || info.height == 0 ||
      info.sample_method == JPEG_DOWN_SAMPLING_GRAY) {
    return false;
  }

  uint32_t padded_width = align_up(info.width, 16);
  uint32_t padded_height = align_up(info.height, 16);
  size_t output_bytes = static_cast<size_t>(padded_width) * padded_height * 2;
  P4JpegWorkspace &memory = workspace();
  if (!ensure_decoder_buffer(memory.input, memory.input_capacity, size, JPEG_DEC_ALLOC_INPUT_BUFFER) ||
      !ensure_decoder_buffer(memory.output, memory.output_capacity, output_bytes, JPEG_DEC_ALLOC_OUTPUT_BUFFER)) {
    ESP_LOGW(TAG, "JPEG hardware workspace allocation failed; using software");
    release_p4_jpeg_workspace();
    return false;
  }
  memcpy(memory.input, data, size);

  jpeg_decode_cfg_t config{};
  config.output_format = JPEG_DECODE_OUT_FORMAT_RGB565;
  config.rgb_order = big_endian ? JPEG_DEC_RGB_ELEMENT_ORDER_RGB : JPEG_DEC_RGB_ELEMENT_ORDER_BGR;
  config.conv_std = JPEG_YUV_RGB_CONV_STD_BT601;

  uint32_t actual_output_bytes = 0;
  uint32_t started_at = millis();
  esp_err_t error = jpeg_decoder_process(handle, &config, memory.input, size, memory.output,
                                         memory.output_capacity, &actual_output_bytes);
  if (error != ESP_OK || actual_output_bytes < output_bytes) {
    ESP_LOGD(TAG, "JPEG hardware rejected input (%d); using software", error);
    release_p4_jpeg_workspace();
    return false;
  }

  bool scaled = cover_mode && target_width > 0 && target_height > 0 &&
                (target_width != static_cast<int>(info.width) || target_height != static_cast<int>(info.height)) &&
                scale_cover(memory.output, padded_width, info.width, info.height, target_width, target_height, memory);
  frame.data = scaled ? memory.scaled : memory.output;
  frame.width = scaled ? target_width : info.width;
  frame.height = scaled ? target_height : info.height;
  frame.stride_bytes = static_cast<size_t>(scaled ? target_width : padded_width) * 2;
  frame.ppa_scaled = scaled;
  ESP_LOGI(TAG, "Hardware JPEG decoded %ux%u in %lu ms (PPA scale: %s)", info.width, info.height,
           static_cast<unsigned long>(millis() - started_at), scaled ? "yes" : "no");
  return true;
}

void release_p4_jpeg_workspace() {
  P4JpegWorkspace &memory = workspace();
  free(memory.input);
  free(memory.output);
  heap_caps_free(memory.scaled);
  memory = {};
}

}  // namespace artwork_image
}  // namespace esphome

#else

namespace esphome {
namespace artwork_image {

bool try_decode_p4_jpeg(const uint8_t *, size_t, int, int, bool, bool, HardwareJpegFrame &) { return false; }
void release_p4_jpeg_workspace() {}

}  // namespace artwork_image
}  // namespace esphome

#endif
