#pragma once

#include "page_fan_mode.h"
#include "page_store.h"

namespace espcontrol::pages {
inline bool page_confirmation_known(std::string_view value) {
  return !fan_mode_trim(value).empty() && !fan_mode_equal(value, "unknown") &&
         !fan_mode_equal(value, "unavailable");
}
inline constexpr uint8_t PAGE_CONFIRM_ON = 1, PAGE_CONFIRM_OFF = 2;
enum class PageConfirmationAction : uint8_t { NONE, TURN_ON, TURN_OFF, OPEN, CLOSE, SCRIPT };
struct PageConfirmationPolicy {
  bool on{false}, off{false}, cover{false}, toggle{false};
  PageConfirmationAction explicit_action{PageConfirmationAction::NONE};
  uint8_t flags() const {
    return (on ? PAGE_CONFIRM_ON : 0) | (off ? PAGE_CONFIRM_OFF : 0);
  }
  bool enabled() const {
    return flags() != 0;
  }
  PageConfirmationAction choose(bool currently_on) const {
    if (explicit_action == PageConfirmationAction::SCRIPT)
      return on ? explicit_action : PageConfirmationAction::NONE;
    if (explicit_action != PageConfirmationAction::NONE)
      return (explicit_action == PageConfirmationAction::OPEN ? on : off) ? explicit_action
                                                                          : PageConfirmationAction::NONE;
    if (!toggle || !(currently_on ? off : on)) return PageConfirmationAction::NONE;
    return cover ? (currently_on ? PageConfirmationAction::CLOSE : PageConfirmationAction::OPEN)
                 : (currently_on ? PageConfirmationAction::TURN_OFF : PageConfirmationAction::TURN_ON);
  }
  static PageConfirmationPolicy fields(std::string_view type, std::string_view entity, std::string_view mode,
                                       std::string_view options) {
    PageConfirmationPolicy result;
    if (entity.empty()) return result;
    const auto has = [options](std::string_view name) {
      for (size_t start = 0; start <= options.size();) {
        const auto end = options.find(',', start);
        if (options.substr(start, end == std::string_view::npos ? end : end - start) == name) return true;
        if (end == std::string_view::npos) break;
        start = end + 1;
      }
      return false;
    };
    if (type == "action" && mode == "script.turn_on") {
      result.on = has("confirm_on");
      result.explicit_action = PageConfirmationAction::SCRIPT;
      return result;
    }
    if (type != "garage" && !type.empty()) return result;
    if (type.empty() && entity.size() > 7 && entity.substr(0, 7) == "button.") return result;
    result.cover = entity.size() > 6 && entity.substr(0, 6) == "cover.";
    result.on = has("confirm_on");
    result.off = has("confirm_off");
    if (type == "garage" && mode == "open")
      result.explicit_action = PageConfirmationAction::OPEN;
    else if (type == "garage" && mode == "close")
      result.explicit_action = PageConfirmationAction::CLOSE;
    else
      result.toggle = true;
    return result;
  }
};
inline PageConfirmationPolicy page_confirmation_policy(const PageStore& store, ConfigurationHandle handle) {
  const auto* card = store.runtime_card(handle);
  if (!card) return {};
  return PageConfirmationPolicy::fields(
      store.field(*card, CardField::TYPE), store.field(*card, CardField::ENTITY),
      store.field(*card, CardField::SENSOR), store.field(*card, CardField::OPTIONS));
}
}  // namespace espcontrol::pages
