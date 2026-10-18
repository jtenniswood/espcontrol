#pragma once

#include <string_view>

#include "modal_context_lifetime.h"

struct AlarmCardCtx;
struct AlarmModalLifetime : espcontrol::ModalContextLifetime<AlarmCardCtx> {
  bool (*defer)(void*, const AlarmCardCtx*, std::string_view, std::string_view, bool){nullptr};
  int (*remaining)(void*, const AlarmCardCtx*){nullptr};
  int (*progress)(void*, const AlarmCardCtx*){nullptr};
};
inline AlarmModalLifetime& alarm_modal_lifetime() {
  static AlarmModalLifetime observer;
  return observer;
}
inline bool alarm_modal_managed(const AlarmCardCtx* ctx) {
  return alarm_modal_lifetime().managed_context(ctx);
}
inline bool alarm_modal_context_current(const AlarmCardCtx* ctx) {
  return alarm_modal_lifetime().current_context(ctx);
}
inline void alarm_modal_notify_closed(AlarmCardCtx* ctx) {
  alarm_modal_lifetime().notify_closed(ctx);
}
inline bool alarm_modal_defer(const AlarmCardCtx* ctx, std::string_view mode, std::string_view code,
                              bool submit_pin) {
  const auto observer = alarm_modal_lifetime();
  return observer.owner && observer.defer && observer.action_allowed(ctx) &&
         observer.defer(observer.owner, ctx, mode, code, submit_pin);
}
inline int alarm_modal_remaining(const AlarmCardCtx* ctx) {
  const auto observer = alarm_modal_lifetime();
  return observer.owner && observer.remaining ? observer.remaining(observer.owner, ctx) : -1;
}
inline int alarm_modal_progress(const AlarmCardCtx* ctx) {
  const auto observer = alarm_modal_lifetime();
  return observer.owner && observer.progress ? observer.progress(observer.owner, ctx) : -1;
}
