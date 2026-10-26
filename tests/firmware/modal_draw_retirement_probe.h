#pragma once

#include <array>
#include <cassert>
#include <cstdint>

#include "lvgl.h"
#include "page_lvgl_dispatch.h"
#include "page_modal_runtime.h"

// Run after retained-resource baselines: the first real refresh can warm
// LVGL's global font/draw caches. Use an actual layer and refresh cycle.
inline void assert_panel_draw_retirement(espcontrol::pages::PageModalRuntime& runtime, lv_obj_t* overlay) {
  using namespace espcontrol::pages;
  assert(overlay && !lv_obj_has_flag(overlay, LV_OBJ_FLAG_HIDDEN));
  auto* display = lv_obj_get_display(overlay);
  alignas(64) static std::array<uint8_t, 1280 * 32 * 4> buffer{};
  lv_display_set_buffers(display, buffer.data(), nullptr, buffer.size(), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display,
                          [](lv_display_t* d, const lv_area_t*, uint8_t*) { lv_display_flush_ready(d); });
  struct Probe {
    PageModalRuntime* runtime;
    lv_obj_t* overlay;
    PageModalToken token;
    bool ran{false};
  } probe{&runtime, overlay, runtime.token()};
  assert(runtime.valid(probe.token));
  lv_obj_add_event_cb(
      overlay,
      [](lv_event_t* event) {
        auto& p = *static_cast<Probe*>(lv_event_get_user_data(event));
        if (p.ran) return;
        p.ran = true;
        assert(page_lvgl_dispatching());
        const auto children = lv_obj_get_child_count(p.overlay);
        p.runtime->dismiss(PageModalClose::REPLACED);
        p.runtime->loop();
        assert(!p.runtime->valid(p.token) && p.runtime->memory().retired_contexts == 1);
        assert(lv_obj_is_valid(p.overlay) && !lv_obj_has_flag(p.overlay, LV_OBJ_FLAG_HIDDEN));
        assert(lv_obj_get_child_count(p.overlay) == children);
      },
      LV_EVENT_DRAW_MAIN, &probe);
  lv_obj_invalidate(overlay);
  lv_refr_now(display);
  assert(probe.ran && !runtime.valid(probe.token));
  runtime.loop();
  assert(runtime.memory().retired_contexts == 0 && !lv_obj_is_valid(overlay));
}
