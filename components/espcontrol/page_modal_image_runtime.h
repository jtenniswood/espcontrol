#pragma once

#include "page_image_runtime.h"
#include "page_modal_runtime.h"

namespace espcontrol::pages {
// A modal image belongs to an admitted opening, independently of the original
// grid binding. It has its own existing modal downloader, never a tile slot.
class PageModalImageOrigin {
 public:
  virtual ~PageModalImageOrigin() = default;
  // Delegate to PageModalRuntime.valid, usable during modal initialization.
  virtual bool valid(PageModalToken) const = 0;
  // Animation additionally requires the admitted overlay to be visible.
  virtual bool visible(PageModalToken token) const {
    return valid(token);
  }
  virtual bool screen_active(PageModalToken token) const {
    return valid(token);
  }
  virtual bool connected() const = 0;
  virtual uint32_t connection_generation() const = 0;
};
struct PageModalImageIdentity {
  static bool active(PageModalToken token) {
    return token.opening != 0;
  }
  static bool eligible(PageModalToken token) {
    return active(token) && token.card != NO_CARD && token.kind == ControlModalKind::IMAGE_CARD;
  }
  static bool same(PageModalToken a, PageModalToken b) {
    return active(a) && a.configuration == b.configuration && a.opening == b.opening && a.card == b.card &&
           a.kind == b.kind;
  }
};
using PageModalImageRuntime =
    PageImageRuntime<1, PageModalToken, PageModalImageOrigin, PageModalImageIdentity, true>;
}  // namespace espcontrol::pages
