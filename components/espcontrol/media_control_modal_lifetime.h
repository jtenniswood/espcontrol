#pragma once

#include "modal_callback_dispatch.h"
#include "modal_context_lifetime.h"

struct MediaControlCtx;
using MediaControlModalLifetime = espcontrol::ModalContextLifetime<MediaControlCtx>;
inline MediaControlModalLifetime& media_control_modal_lifetime() {
  static MediaControlModalLifetime observer;
  return observer;
}
inline bool media_control_modal_managed(const MediaControlCtx* context) {
  return media_control_modal_lifetime().managed_context(context);
}
inline bool media_control_context_current(const MediaControlCtx* context) {
  return media_control_modal_lifetime().current_context(context);
}
inline bool media_control_action_allowed(const MediaControlCtx* context) {
  return media_control_modal_lifetime().action_allowed(context);
}
inline void media_control_notify_closed(MediaControlCtx* context) {
  media_control_modal_lifetime().notify_closed(context);
}
inline bool media_control_widgets_ready(void* root) {
  return media_control_modal_lifetime().widgets_ready(root);
}
inline void media_control_notify_value_sent(const MediaControlCtx* context, float percent) {
  media_control_modal_lifetime().notify_numeric_action(context, percent);
}
inline uint64_t media_control_context_identity(const MediaControlCtx* context) {
  return media_control_modal_lifetime().context_identity(context);
}
inline bool media_control_identity_current(const MediaControlCtx* context, uint64_t opening) {
  return media_control_context_current(context) &&
         (opening == 0 ? !media_control_modal_lifetime().owner
                       : media_control_context_identity(context) == opening);
}
