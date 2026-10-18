#include "page_qr_matrix.h"

#include <cassert>
#include <string>

#include "wifi_qr_codec.h"
using namespace espcontrol::pages;
int main() {
  assert(PAGE_QR_BITMAP_BYTES == 668);
  for (const auto& payload : {std::string("WIFI:T:nopass;S:Guest;H:false;;"),
                              std::string("WIFI:T:nopass;S:Guest;H:true;;"), std::string("other")}) {
    const auto prefix = page_qr_compact_prefix(payload);
    assert(std::string(prefix) + (prefix.size() != payload.size() ? ";" : "") ==
           wifi_qr_compact_payload(payload));
  }
  PageQrMatrix matrix;
  matrix.clear();
  assert(!matrix.size());
  for (unsigned minimum = 1; minimum <= 12; ++minimum)
    for (int side = 1; side <= 1280; ++side)
      for (bool compact : {false, true}) {
        const auto layout = page_qr_layout(minimum, side, compact);
        if (!layout.valid()) continue;
        assert(layout.modules <= PAGE_QR_MODULES_MAX);
        assert(layout.scale * layout.modules + 2 * layout.margin <= side);
        assert(side - layout.scale * layout.modules - 2 * layout.margin < 2);
        assert(layout.margin >= 2 * layout.scale);
        if (compact)
          assert(layout.scale == wifi_qr_compact_scale(side, layout.modules) && layout.version == minimum);
        matrix.clear();
        matrix.layout = layout;
        assert(matrix.size() <= matrix.bytes.size());
        matrix.dark(0, 0, true);
        matrix.dark(layout.modules - 1, layout.modules - 1, true);
        assert(matrix.dark(0, 0) && matrix.dark(layout.modules - 1, layout.modules - 1));
        assert(!matrix.dark(layout.modules, layout.modules));
        matrix.dark(0, 0, false);
        assert(!matrix.dark(0, 0));
      }
  for (unsigned field = 0; field < 6; ++field) {
    auto layout = page_qr_layout(3, 132, true);
    assert(layout.valid());
    switch (field) {
      case 0:
        layout.version = 15;
        break;
      case 1:
        layout.modules = 74;
        break;
      case 2:
        layout.scale = 0;
        break;
      case 3:
        layout.scale = UINT16_MAX;
        break;
      case 4:
        layout.margin = UINT16_MAX;
        break;
      case 5:
        layout.side = 1;
        break;
    }
    assert(!layout.valid());
  }
  for (int side : {0, -1, 65536}) assert(!page_qr_layout(1, side, true).valid());
  assert(!page_qr_layout(0, 480, false).valid() && !page_qr_layout(15, 480, false).valid());
}
