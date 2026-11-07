#pragma once

#include "page_modal_runtime.h"
#include "page_qr_runtime.h"

namespace espcontrol::pages {
class PageModalQrOrigin {
 public:
  virtual ~PageModalQrOrigin() = default;
  virtual bool valid(PageModalToken) const = 0;
};
struct PageModalQrIdentity {
  static bool active(PageModalToken token) {
    return token.opening != 0;
  }
  static bool eligible(PageModalToken token) {
    return active(token) && token.kind == ControlModalKind::WIFI_QR && token.card != NO_CARD;
  }
  static bool same(PageModalToken a, PageModalToken b) {
    return active(a) && a.configuration == b.configuration && a.opening == b.opening && a.card == b.card &&
           a.kind == b.kind;
  }
};
using PageModalQrRuntime = PageQrRuntime<PageModalToken, PageModalQrOrigin, PageModalQrIdentity>;
}  // namespace espcontrol::pages
