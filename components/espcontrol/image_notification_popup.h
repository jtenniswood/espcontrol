// =============================================================================
// IMAGE NOTIFICATION POPUP - state + event helper for screen_image_notification
// =============================================================================
// Parallel to notification_popup.h. Holds the current image notification's
// message_id and forwards ack/expire events through the shared text-popup
// helper so HA sees an identical event payload for text and image popups.
// Auto-included into the build by ESPHome's component resource system.
// =============================================================================
#pragma once
#include <string>
#include "notification_popup.h"  // reuse notification_fire_event
// Full grid-dashboard facade, not just button_grid_modal.h: button_grid_modal.h
// is not self-contained (it depends on types/constants from button_grid_display.h,
// button_grid_style.h, etc. that it doesn't include itself — see ADR
// dev-docs/adr/0006-hybrid-compiled-firmware-modules.md, "button_grid.h remains
// the supported include"). Reused here for the "modal" notification style's
// control_modal_* panel/chrome helpers.
#include "button_grid.h"

// Message id of the image notification currently on screen (one at a time).
static std::string s_image_notif_msg_id;

// Record the message_id for the active image notification.
inline void image_notification_set_msg_id(const std::string &message_id) {
  s_image_notif_msg_id = message_id;
}

// Fire a HA event (acknowledged/expired) with device_name + the stored message_id.
inline void image_notification_fire(const char *event_name) {
  notification_fire_event(event_name, s_image_notif_msg_id);
}

// Resolve a caller-supplied image_url against the cover-art HA base URL. Full
// http(s) URLs pass through; relative paths are appended to the base. Pure
// string work — no artwork_image dependency, so it lives in this header.
inline std::string image_notification_resolve_url(const std::string &image_url,
                                                  const std::string &ha_base_url) {
  if (image_url.rfind("http://", 0) == 0 || image_url.rfind("https://", 0) == 0)
    return image_url;
  std::string base = ha_base_url;
  while (!base.empty() && base.back() == '/') base.pop_back();
  if (base.empty()) return image_url;
  if (image_url.empty()) return base;
  return base + (image_url.front() == '/' ? image_url : ("/" + image_url));
}

// Lazily-created, cached close button for the "modal" notification style.
// Same pattern as control_modal_active() / image_card_modal_ui() in
// button_grid_modal.h / button_grid_image.h: a function-local static, not an
// ESPHome `globals:` entry — there's no precedent anywhere in this codebase
// for storing a raw lv_obj_t* in `globals:`, and both of those existing
// helpers already establish this exact idiom for the same kind of value.
inline lv_obj_t *&image_notif_modal_close_btn() {
  static lv_obj_t *btn = nullptr;
  return btn;
}
