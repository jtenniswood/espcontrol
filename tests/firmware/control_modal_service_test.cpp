#include <cassert>

#include "control_modal_service.h"
#include "espcontrol_app_core.h"

namespace {

struct Overlay {};

int close_calls = 0;

void close_modal() { ++close_calls; }

}  // namespace

int main() {
  ControlModalStateService<Overlay> modal;
  Overlay overlay;
  Overlay nested_overlay;

  modal.set_active(ControlModalKind::IMAGE_CARD, &overlay, close_modal,
                   ControlModalDismissPolicy::DISMISS);
  assert(modal.active().overlay == &overlay);
  assert(modal.active().kind == ControlModalKind::IMAGE_CARD);

  modal.block_close_for(100, 50);
  assert(modal.close_guard_active(149));
  assert(!modal.close_guard_active(150));

  ControlModalKind closing_kind = ControlModalKind::NONE;
  ControlModalCloseCallback close_callback = nullptr;
  assert(!modal.begin_active_close(149, true, &closing_kind, &close_callback));
  assert(modal.begin_active_close(149, false, &closing_kind, &close_callback));
  assert(closing_kind == ControlModalKind::IMAGE_CARD);
  assert(close_callback == close_modal);
  close_callback();
  assert(close_calls == 1);
  assert(!modal.begin_active_close(150, false, nullptr, nullptr));

  modal.reset_active();
  modal.nested_active().overlay = &nested_overlay;
  modal.nested_active().close_callback = close_modal;
  Overlay *closing_overlay = nullptr;
  close_callback = nullptr;
  assert(modal.begin_nested_close(&closing_overlay, &close_callback));
  assert(closing_overlay == &nested_overlay);
  assert(close_callback == close_modal);
  modal.clear_nested_menu(&nested_overlay);
  assert(modal.nested_active().overlay == nullptr);

  espcontrol::EspControlAppCore app;
  assert(app.start());
  auto &core_modal = app.modal_state_service<ControlModalStateService<Overlay>>();
  core_modal.set_active(ControlModalKind::IMAGE_CARD, &overlay, nullptr,
                        ControlModalDismissPolicy::DISMISS);
  assert(core_modal.active().kind == ControlModalKind::IMAGE_CARD);
  assert(app.stop());

  return 0;
}
