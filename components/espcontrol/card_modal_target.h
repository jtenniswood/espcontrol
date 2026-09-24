#pragma once

#include <functional>
#include <string>
#include <utility>

#include "control_modal_service.h"

namespace espcontrol::cards {

// A short-lived view of an existing card. Never retain it across a wake delay
// or a grid rebuild: the owner and its runtime may have been deleted.
struct ModalTarget {
  std::string entity;
  ControlModalKind kind = ControlModalKind::NONE;
  bool available = false;
  std::function<void()> open;

  bool supported() const { return kind != ControlModalKind::NONE; }
};

template<typename Runtime>
inline ModalTarget modal_target(Runtime *runtime, const std::string &configured_entity,
                                ControlModalKind kind, bool available,
                                void (*open)(Runtime *)) {
  ModalTarget target;
  // Keep a configured media target stable when its live playback route switches.
  // Empty subpage entities (notably inherited alarms) use the bound runtime.
  target.entity = !configured_entity.empty() ? configured_entity
    : runtime ? runtime->entity_id : "";
  target.kind = kind;
  target.available = runtime && available;
  if (runtime) target.open = [runtime, open]() { open(runtime); };
  return target;
}

}  // namespace espcontrol::cards
