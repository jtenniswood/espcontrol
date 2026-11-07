#pragma once

#include "page_basic_card.h"
#include "page_card_commands.h"

namespace espcontrol::pages {
// Only scalar selections enter the shared FIFO. Config strings are copied by
// PageCardCommands immediately before dispatch, outside the LVGL callback.
enum class PageDeviceAction : uint8_t {
  NONE,
  TOGGLE,
  BUTTON,
  TURN_ON,
  TURN_OFF,
  FAN_ON,
  FAN_OFF,
  OSCILLATE_ON,
  OSCILLATE_OFF,
  FORWARD,
  REVERSE,
  COVER,
  COVER_TILT,
  OPEN,
  CLOSE,
  LOCK,
  UNLOCK,
  VACUUM_START,
  VACUUM_STOP,
  VACUUM_DOCK,
  VACUUM_PAUSE,
  VACUUM_START_PAUSE,
  VACUUM_SPOT,
  VACUUM_LOCATE,
  VACUUM_AREA,
  MOWER_START,
  MOWER_PAUSE,
  MOWER_DOCK
};
inline const char* page_device_service(PageDeviceAction action) {
  switch (action) {
    case PageDeviceAction::TOGGLE:
      return "homeassistant.toggle";
    case PageDeviceAction::BUTTON:
      return "button.press";
    case PageDeviceAction::TURN_ON:
      return "homeassistant.turn_on";
    case PageDeviceAction::TURN_OFF:
      return "homeassistant.turn_off";
    case PageDeviceAction::FAN_ON:
      return "fan.turn_on";
    case PageDeviceAction::FAN_OFF:
      return "fan.turn_off";
    case PageDeviceAction::OSCILLATE_ON:
    case PageDeviceAction::OSCILLATE_OFF:
      return "fan.oscillate";
    case PageDeviceAction::FORWARD:
    case PageDeviceAction::REVERSE:
      return "fan.set_direction";
    case PageDeviceAction::OPEN:
      return "cover.open_cover";
    case PageDeviceAction::CLOSE:
      return "cover.close_cover";
    case PageDeviceAction::LOCK:
      return "lock.lock";
    case PageDeviceAction::UNLOCK:
      return "lock.unlock";
    case PageDeviceAction::VACUUM_START:
      return "vacuum.start";
    case PageDeviceAction::VACUUM_STOP:
      return "vacuum.stop";
    case PageDeviceAction::VACUUM_DOCK:
      return "vacuum.return_to_base";
    case PageDeviceAction::VACUUM_PAUSE:
      return "vacuum.pause";
    case PageDeviceAction::VACUUM_START_PAUSE:
      return "vacuum.start_pause";
    case PageDeviceAction::VACUUM_SPOT:
      return "vacuum.clean_spot";
    case PageDeviceAction::VACUUM_LOCATE:
      return "vacuum.locate";
    case PageDeviceAction::VACUUM_AREA:
      return "vacuum.clean_area";
    case PageDeviceAction::MOWER_START:
      return "lawn_mower.start_mowing";
    case PageDeviceAction::MOWER_PAUSE:
      return "lawn_mower.pause";
    case PageDeviceAction::MOWER_DOCK:
      return "lawn_mower.dock";
    default:
      return nullptr;
  }
}
class PageDeviceCommandPolicy final : public PageCardControlPolicy {
 public:
  PageDeviceCommandPolicy(PageEntityStateRuntime& graph, PageFanStatusRuntime& fan,
                          PageAccessStateRuntime& access)
      : graph_(graph), fan_(fan), access_(access) {}
  bool admit(const PageStore& store, PageBindingToken origin, PageCardCommandKind kind,
             PageCardCommandValue& value) override {
    if (kind != PageCardCommandKind::DEVICE_ACTION) return false;
    const auto action = select(store, origin);
    value.integer = static_cast<int>(action);
    if (const auto* card = store.runtime_card({origin.configuration, origin.card}))
      value.write_revision =
          graph_
              .read_attribute({origin.configuration, origin.card}, store.field(*card, CardField::ENTITY), {})
              .revision;
    return action != PageDeviceAction::NONE;
  }
  bool valid(const PageStore& store, PageBindingToken origin, PageCardCommandKind kind,
             const PageCardCommandValue& value) override {
    // A state/capability change may cancel a queued selection; it must never
    // reinterpret the same press as its opposite or bypass confirmation.
    return kind == PageCardCommandKind::DEVICE_ACTION && value.integer != 0 &&
           value.integer == static_cast<int>(select(store, origin));
  }
  void discard(PageBindingToken, PageCardCommandKind, const PageCardCommandValue&) override {}

 private:
  template <size_t N>
  static bool command(std::string_view mode, const char* const (&modes)[N]) {
    if (mode.empty()) return false;
    for (const auto* candidate : modes)
      if (mode == candidate) return true;
    return false;
  }
  PageDeviceAction select(const PageStore& store, PageBindingToken origin) {
    using A = PageDeviceAction;
    const ConfigurationHandle handle{origin.configuration, origin.card};
    const auto* card = store.runtime_card(handle);
    if (!card) return A::NONE;
    const auto type = store.field(*card, CardField::TYPE);
    const auto mode = store.field(*card, CardField::SENSOR);
    BasicCardDefinition definition;
    definition.entity = store.field(*card, CardField::ENTITY);
    definition.sensor = mode;
    if (definition.entity.empty()) return A::NONE;
    const auto toggle =
        definition.entity.size() > 7 && definition.entity.substr(0, 7) == "button." ? A::BUTTON : A::TOGGLE;
    if (type == "fan_switch" || type == "fan_oscillate" || type == "fan_direction") {
      const auto primary = graph_.read_attribute(handle, definition.entity, {});
      if (primary.status != PageEntityStatus::AVAILABLE || !page_confirmation_known(primary.value))
        return A::NONE;
      if (type == "fan_direction") {
        const auto direction = graph_.read_attribute(handle, definition.entity, "direction");
        if (direction.status != PageEntityStatus::AVAILABLE) return A::NONE;
        return fan_mode_equal(direction.value, "reverse")   ? A::FORWARD
               : fan_mode_equal(direction.value, "forward") ? A::REVERSE
                                                            : A::NONE;
      }
      const auto history = fan_.read(handle);
      if (history.status == PageFanStatus::UNAVAILABLE || history.status == PageFanStatus::UNSUPPORTED)
        return A::NONE;
      if (type == "fan_switch") return history.on ? A::FAN_OFF : A::FAN_ON;
      if (graph_.read_attribute(handle, definition.entity, "oscillating").status !=
          PageEntityStatus::AVAILABLE)
        return A::NONE;
      return history.on ? A::OSCILLATE_OFF : A::OSCILLATE_ON;
    }
    if (type == "cover") {
      if (mode == "toggle")
        return graph_.read_attribute(handle, definition.entity, {}).status == PageEntityStatus::EXHAUSTED
                   ? A::NONE
                   : toggle;
      if (mode != "open" && mode != "close" && mode != "stop" && mode != "set_position") return A::NONE;
      definition.kind = BasicCardKind::COVER_COMMAND;
      const auto snapshot = read_basic_cover_command(definition, handle, graph_);
      return !snapshot.supported ? A::NONE : snapshot.tilt ? A::COVER_TILT : A::COVER;
    }
    if (type == "garage" || type == "gate" || type == "lock") {
      const bool lock = type == "lock", gate = type == "gate";
      definition.kind = BasicCardKind::ACCESS;
      const bool explicit_command = lock   ? command(mode, CARD_CONTRACT_LOCK_MODES)
                                    : gate ? command(mode, CARD_CONTRACT_GATE_MODES)
                                           : command(mode, CARD_CONTRACT_GARAGE_MODES);
      definition.access = {lock   ? BasicAccessKind::LOCK
                           : gate ? BasicAccessKind::GATE
                                  : BasicAccessKind::GARAGE,
                           explicit_command, false};
      const auto snapshot = read_basic_access(definition, handle, graph_, &access_);
      if (!snapshot.enabled) return A::NONE;
      const bool currently_on = explicit_command ? mode != "open" : snapshot.history.on;
      const auto confirmation = page_confirmation_policy(store, handle);
      if (confirmation.choose(currently_on) != PageConfirmationAction::NONE) return A::NONE;
      if (lock) return (explicit_command ? mode == "lock" : !snapshot.history.locked) ? A::LOCK : A::UNLOCK;
      if (explicit_command) return A::COVER;  // Access commands use normal cover services.
      if (confirmation.enabled()) {
        const auto primary = graph_.read_attribute(handle, definition.entity, {});
        if (primary.status != PageEntityStatus::AVAILABLE || !page_confirmation_known(primary.value))
          return A::NONE;
        if (confirmation.cover) return currently_on ? A::CLOSE : A::OPEN;
        return currently_on ? A::TURN_OFF : A::TURN_ON;
      }
      return toggle;
    }
    if (type == "vacuum" || type == "lawn_mower") {
      definition.kind = BasicCardKind::CLEANING;
      definition.cleaning = {cleaning::mode(mode, type == "lawn_mower"), type == "lawn_mower", false};
      definition.sensor = store.field(*card, CardField::UNIT);
      const auto* service = read_basic_cleaning(definition, handle, graph_).service;
      if (!service) return A::NONE;
      for (int selected = static_cast<int>(A::VACUUM_START); selected <= static_cast<int>(A::MOWER_DOCK);
           ++selected)
        if (std::string_view(service) == page_device_service(static_cast<A>(selected)))
          return static_cast<A>(selected);
    }
    return A::NONE;
  }
  PageEntityStateRuntime& graph_;
  PageFanStatusRuntime& fan_;
  PageAccessStateRuntime& access_;
};
}  // namespace espcontrol::pages
