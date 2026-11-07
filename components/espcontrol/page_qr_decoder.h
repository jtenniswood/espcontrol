#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <climits>
#include <cstring>
#include <new>

#include "lvgl.h"
#include "page_qr_matrix.h"
#include "src/core/lv_global.h"
#include "src/draw/lv_image_decoder_private.h"

#if LVGL_VERSION_MAJOR != 9 || LVGL_VERSION_MINOR != 5 || LVGL_VERSION_PATCH != 0
#error "Audit QR row decoder ownership for the selected LVGL version"
#endif

namespace espcontrol::pages {
class PageQrDecoder;
struct PageQrImageData {
  static constexpr uint64_t SIGNATURE = 0x45535051524D4154ULL;
  uint64_t signature{SIGNATURE};
  PageQrDecoder* owner{nullptr};
  const PageQrMatrix* matrix{nullptr};
};
// Expand the small packed matrix to one temporary, unscaled A8 row. This
// preserves exact integer module edges and quiet zones without a full canvas
// or image-transform rounding. Repeated module rows reuse the same mask bytes.
// No decoded pixels are retained in image cache.
// Install once during scene preflight, outside dispatch. Every persistent frame
// must reset after a draw fence before shutdown; active decode sessions also
// prevent removal. Unrelated image sources never enter this decoder.
class PageQrDecoder {
 public:
  PageQrDecoder() = default;
  ~PageQrDecoder() {
    const bool stopped = shutdown();
    assert(stopped);
    (void)stopped;
  }
  PageQrDecoder(const PageQrDecoder&) = delete;
  PageQrDecoder& operator=(const PageQrDecoder&) = delete;
  bool initialize() {
    if (decoder_) return true;
    if (!lv_is_initialized()) return false;
    // The public 9.5.0 constructor asserts on allocation failure. Scene
    // preflight must fail normally; use its checked list insertion instead.
    // Keep this paired with the pinned-version guard and public delete helper.
    decoder_ = static_cast<lv_image_decoder_t*>(lv_ll_ins_head(&LV_GLOBAL_DEFAULT()->img_decoder_ll));
    if (!decoder_) return false;
    std::memset(decoder_, 0, sizeof(*decoder_));
    decoder_->user_data = this;
    decoder_->name = "EspControl QR";
    lv_image_decoder_set_info_cb(decoder_, info);
    lv_image_decoder_set_open_cb(decoder_, open);
    lv_image_decoder_set_close_cb(decoder_, close);
    lv_image_decoder_set_get_area_cb(decoder_, area);
    return true;
  }
  bool ready() const {
    return decoder_ != nullptr;
  }
  bool shutdown() {
    if (frames_ || sessions_.load()) return false;
    if (decoder_) lv_image_decoder_delete(decoder_);
    decoder_ = nullptr;
    return true;
  }
  size_t sessions() const {
    return sessions_.load();
  }
  void retain_frame() {
    assert(ready());
    ++frames_;
  }
  void release_frame() {
    assert(frames_);
    --frames_;
  }

 private:
  static const PageQrImageData* source(lv_image_decoder_t* decoder, lv_image_decoder_dsc_t* dsc) {
    if (dsc->src_type != LV_IMAGE_SRC_VARIABLE || !dsc->src) return nullptr;
    const auto* image = static_cast<const lv_image_dsc_t*>(dsc->src);
    if (image->header.cf != LV_COLOR_FORMAT_RAW_ALPHA || image->header.flags != LV_IMAGE_FLAGS_USER8 ||
        image->data_size != sizeof(PageQrImageData) || !image->data)
      return nullptr;
    const auto* data = reinterpret_cast<const PageQrImageData*>(image->data);
    if (data->signature != PageQrImageData::SIGNATURE || data->owner != decoder->user_data || !data->matrix ||
        !data->matrix->layout.valid() || data->matrix->layout.modules > PAGE_QR_MODULES_MAX)
      return nullptr;
    const auto stride = lv_draw_buf_width_to_stride(data->matrix->layout.side, LV_COLOR_FORMAT_A8);
    if (!stride || stride > UINT16_MAX) return nullptr;
    return data;
  }
  static lv_result_t info(lv_image_decoder_t* decoder, lv_image_decoder_dsc_t* dsc,
                          lv_image_header_t* header) {
    const auto* data = source(decoder, dsc);
    if (!data) return LV_RESULT_INVALID;
    *header = {};
    header->magic = LV_IMAGE_HEADER_MAGIC;
    header->cf = LV_COLOR_FORMAT_A8;
    header->w = header->h = data->matrix->layout.side;
    return LV_RESULT_OK;
  }
  struct Session {
    lv_draw_buf_t row{};
    void* pixels{nullptr};
    const PageQrMatrix* matrix{nullptr};
    int cached_y{INT_MIN}, cached_x{INT_MIN}, cached_width{0};
  };
  static lv_result_t open(lv_image_decoder_t* decoder, lv_image_decoder_dsc_t* dsc) {
    const auto* data = source(decoder, dsc);
    if (!data) return LV_RESULT_INVALID;
    void* memory = lv_malloc(sizeof(Session));
    if (!memory) return LV_RESULT_INVALID;
    auto* session = new (memory) Session{};
    session->matrix = data->matrix;
    const auto stride = lv_draw_buf_width_to_stride(data->matrix->layout.side, LV_COLOR_FORMAT_A8);
    session->pixels = lv_malloc(stride + LV_DRAW_BUF_ALIGN);
    if (!session->pixels) {
      session->~Session();
      lv_free(memory);
      return LV_RESULT_INVALID;
    }
    void* aligned = lv_draw_buf_align(session->pixels, LV_COLOR_FORMAT_A8);
    if (lv_draw_buf_init(&session->row, data->matrix->layout.side, 1, LV_COLOR_FORMAT_A8, stride, aligned,
                         stride) != LV_RESULT_OK) {
      lv_free(session->pixels);
      session->~Session();
      lv_free(memory);
      return LV_RESULT_INVALID;
    }
    auto& self = *static_cast<PageQrDecoder*>(decoder->user_data);
    self.sessions_.fetch_add(1);
    dsc->user_data = session;
    dsc->decoded = nullptr;
    dsc->args.no_cache = true;
    return LV_RESULT_OK;
  }
  static lv_result_t area(lv_image_decoder_t*, lv_image_decoder_dsc_t* dsc, const lv_area_t* full,
                          lv_area_t* decoded) {
    auto* session = static_cast<Session*>(dsc->user_data);
    if (!session) return LV_RESULT_INVALID;
    const auto& matrix = *session->matrix;
    const auto& g = matrix.layout;
    if (full->x1 < 0 || full->x2 < full->x1 || full->x2 >= g.side || full->y1 < 0 || full->y2 < full->y1 ||
        full->y2 >= g.side)
      return LV_RESULT_INVALID;
    if (decoded->y1 != LV_COORD_MIN && (decoded->y2 < full->y1 || decoded->y2 >= full->y2))
      return LV_RESULT_INVALID;
    const int y =
        decoded->y1 == LV_COORD_MIN ? static_cast<int>(full->y1) : static_cast<int>(decoded->y2) + 1;
    const int x = static_cast<int>(full->x1);
    const int width = static_cast<int>(full->x2) - x + 1;
    const int my = y >= g.margin && y < g.margin + g.modules * g.scale ? (y - g.margin) / g.scale : -1;
    if (session->cached_y != my || session->cached_x != full->x1 || session->cached_width != width) {
      std::memset(session->row.data, 0, session->row.header.stride);
      if (my >= 0) {
        for (unsigned mx = 0; mx < g.modules;) {
          if (!matrix.dark(mx, my)) {
            ++mx;
            continue;
          }
          const int first = mx;
          while (mx < g.modules && matrix.dark(mx, my)) ++mx;
          const int left = std::max(0, g.margin + first * g.scale - x);
          const int right = std::min(width, g.margin + static_cast<int>(mx) * g.scale - x);
          if (right > left) std::memset(session->row.data + left, 0xFF, right - left);
        }
      }
      session->cached_y = my;
      session->cached_x = full->x1;
      session->cached_width = width;
      if (dsc->args.flush_cache) lv_draw_buf_flush_cache(&session->row, nullptr);
    }
    *decoded = {full->x1, y, full->x2, y};
    dsc->decoded = &session->row;
    return LV_RESULT_OK;
  }
  static void close(lv_image_decoder_t* decoder, lv_image_decoder_dsc_t* dsc) {
    auto* session = static_cast<Session*>(dsc->user_data);
    if (!session) return;
    lv_free(session->pixels);
    session->~Session();
    lv_free(session);
    dsc->user_data = nullptr;
    dsc->decoded = nullptr;
    auto& self = *static_cast<PageQrDecoder*>(decoder->user_data);
    const auto previous = self.sessions_.fetch_sub(1);
    assert(previous);
    (void)previous;
  }
  lv_image_decoder_t* decoder_{nullptr};
  size_t frames_{0};
  std::atomic<size_t> sessions_{0};
};
}  // namespace espcontrol::pages
