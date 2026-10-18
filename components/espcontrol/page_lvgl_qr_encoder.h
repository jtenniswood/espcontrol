#pragma once

#include <cstring>

#include "page_qr_matrix.h"
#include "src/libs/qrcode/qrcodegen.h"

#if LVGL_VERSION_MAJOR != 9 || LVGL_VERSION_MINOR != 5 || LVGL_VERSION_PATCH != 0
#error "Audit QR version/quiet-zone parity for the selected LVGL version"
#endif

#if !LV_USE_QRCODE
#error "The pooled QR encoder requires the firmware's QR library"
#endif

namespace espcontrol::pages {
// One scene-owned scratch buffer, reused sequentially. Place with the other
// preflighted scene resources, never on the small UI task stack per encode.
class PageLvglQrEncoder final : public PageQrEncoder {
 public:
  PageLvglQrEncoder() = default;
  PageLvglQrEncoder(const PageLvglQrEncoder&) = delete;
  PageLvglQrEncoder& operator=(const PageLvglQrEncoder&) = delete;
  bool encode(std::string_view payload, int side, bool compact, PageQrMatrix& output) override {
    output.clear();
    if (payload.empty() || payload.size() > PAGE_QR_PAYLOAD_MAX) return false;
    const auto prefix = compact ? page_qr_compact_prefix(payload) : payload;
    const bool short_suffix = prefix.size() != payload.size();
    const size_t length = prefix.size() + (short_suffix ? 1 : 0);
    std::memcpy(data_.data(), prefix.data(), prefix.size());
    if (short_suffix) data_[prefix.size()] = ';';
    const auto correction = compact ? qrcodegen_Ecc_LOW : qrcodegen_Ecc_MEDIUM;
    const auto layout = page_qr_layout(qrcodegen_getMinFitVersion(correction, length), side, compact);
    if (!layout.valid()) return false;
    if (!qrcodegen_encodeBinary(data_.data(), length, output.bytes.data(), correction, layout.version,
                                layout.version, qrcodegen_Mask_AUTO, true))
      return false;
    output.layout = layout;
    return true;
  }

 private:
  static constexpr size_t BUFFER = qrcodegen_BUFFER_LEN_FOR_VERSION(PAGE_QR_VERSION_MAX);
  static_assert(BUFFER == PAGE_QR_BITMAP_BYTES);
  std::array<uint8_t, BUFFER> data_{};
};
}  // namespace espcontrol::pages
