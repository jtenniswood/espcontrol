#pragma once

#include "modal_context_lifetime.h"

struct MediaVolumeCtx;
using MediaVolumeModalLifetime = espcontrol::ModalContextLifetime<MediaVolumeCtx>;
inline MediaVolumeModalLifetime& media_volume_modal_lifetime() {
  static MediaVolumeModalLifetime observer;
  return observer;
}
inline bool media_volume_modal_managed(const MediaVolumeCtx* context) {
  return media_volume_modal_lifetime().managed_context(context);
}
inline bool media_volume_context_current(const MediaVolumeCtx* context) {
  return media_volume_modal_lifetime().current_context(context);
}
inline bool media_volume_action_allowed(const MediaVolumeCtx* context) {
  return media_volume_modal_lifetime().action_allowed(context);
}
inline void media_volume_notify_closed(MediaVolumeCtx* context) {
  media_volume_modal_lifetime().notify_closed(context);
}
inline bool media_volume_widgets_ready(void* root) {
  return media_volume_modal_lifetime().widgets_ready(root);
}
inline void media_volume_notify_value_sent(const MediaVolumeCtx* context, float percent) {
  media_volume_modal_lifetime().notify_numeric_action(context, percent);
}
