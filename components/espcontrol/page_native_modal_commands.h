#pragma once

// Include after button_grid.h. These adapters borrow the scene's single modal
// owner; they register no legacy handlers, subscriptions or configured contexts.
#include "page_alarm_commands.h"
#include "page_confirmation_runtime.h"
#include "page_guest_wifi_runtime.h"
#include "page_lvgl_dispatch.h"

namespace espcontrol::pages {
inline bool page_native_command_ready() {
  return lv_is_initialized() && !page_lvgl_dispatching() && !modal_callback_dispatching() &&
         !ha_read_coordinator().dispatching();
}

class PageNativeConfirmationPort final : public PageConfirmationPort {
 public:
  using OriginValid = bool (*)(void*, PageBindingToken);
  PageNativeConfirmationPort(PageModalRuntime& modal, void* owner, OriginValid valid)
      : modal_(modal), owner_(owner), valid_(valid) {}
  bool valid(PageBindingToken token) const override {
    return valid_ && !espcontrol::screensaver_pin().locked() && valid_(owner_, token);
  }
  bool valid(PageModalToken token) const override {
    return token.kind == ControlModalKind::SWITCH_CONFIRMATION && !espcontrol::screensaver_pin().locked() &&
           modal_.valid(token);
  }
  bool connected() const override { return ha_api_state_connected(); }
  bool service_available() const override { return ha_api_available(); }
  bool ready() const override { return page_native_command_ready(); }
  uint64_t request(PageBindingToken source) override {
    // Called by input dispatch: admission is scalar-only and must not require
    // maintenance readiness or create a panel from an LVGL callback.
    return valid(source) && modal_.request(source, ControlModalKind::SWITCH_CONFIRMATION)
               ? modal_.pending_request()
               : 0;
  }
  bool pending_request(uint64_t opening) const override {
    return opening && modal_.pending_request() == opening;
  }
  void cancel_request(uint64_t opening) override { modal_.cancel_request(opening); }
  void close(PageModalToken token) override {
    if (token.kind == ControlModalKind::SWITCH_CONFIRMATION) modal_.notify_closed(token);
  }
  bool send(PageConfirmationAction action, const ParsedCfg& config) override {
    if (!ready() || !connected() || !service_available() || espcontrol::screensaver_pin().locked())
      return false;
    switch (action) {
      case PageConfirmationAction::TURN_ON:
        return ha_send_entity_action(config.entity, "homeassistant.turn_on");
      case PageConfirmationAction::TURN_OFF:
        return ha_send_entity_action(config.entity, "homeassistant.turn_off");
      case PageConfirmationAction::OPEN:
        return ha_send_entity_action(config.entity, "cover.open_cover");
      case PageConfirmationAction::CLOSE:
        return ha_send_entity_action(config.entity, "cover.close_cover");
      case PageConfirmationAction::SCRIPT:
        return config.type == "action" && config.sensor == "script.turn_on" &&
               send_action_card_action(config);
      case PageConfirmationAction::NONE:
        return false;
    }
    return false;
  }

 private:
  PageModalRuntime& modal_;
  void* owner_;
  OriginValid valid_;
};

class PageNativeAlarmCommandPort final : public PageAlarmCommandPort {
 public:
  using OriginValid = bool (*)(void*, PageBindingToken);
  using SelectedPin = bool (*)(void*, PageModalToken, alarm::Action);
  PageNativeAlarmCommandPort(PageModalRuntime& modal, void* owner, OriginValid valid, SelectedPin selected)
      : modal_(modal), owner_(owner), valid_(valid), selected_(selected) {}
  bool ready() const override { return page_native_command_ready(); }
  bool connected() const override { return ha_api_state_connected(); }
  bool service_available() const override { return ha_api_available(); }
  bool valid(PageBindingToken token) const override {
    return valid_ && !espcontrol::screensaver_pin().locked() && valid_(owner_, token);
  }
  bool valid(PageModalToken token) const override {
    return (token.kind == ControlModalKind::ALARM_CONTROL || token.kind == ControlModalKind::ALARM_PIN) &&
           !espcontrol::screensaver_pin().locked() && modal_.valid(token);
  }
  bool selected_pin(PageModalToken token, alarm::Action action) const override {
    return valid(token) && selected_ && selected_(owner_, token, action);
  }
  bool send(std::string_view entity, const char* service, std::string_view code) override {
    if (!ready() || !connected() || !service_available() || espcontrol::screensaver_pin().locked() ||
        entity.empty() || !service || code.size() > 16 || code.find('\0') != std::string_view::npos)
      return false;
    // Request fields own their bytes before the transport can reenter. Views
    // need not be null terminated; do not retain a second PIN copy in this port.
    esphome::api::HomeassistantActionRequest request;
    if (!ha_action_begin(request, service, false, code.empty() ? 1 : 2)) return false;
    ha_action_add_entity(request, std::string(entity));
    if (!code.empty()) ha_action_add_data(request, "code", std::string(code).c_str());
    return ha_action_send(request);
  }

 private:
  PageModalRuntime& modal_;
  void* owner_;
  OriginValid valid_;
  SelectedPin selected_;
};

class PageNativeGuestWifiPort final : public PageGuestWifiPort {
 public:
  explicit PageNativeGuestWifiPort(PageModalRuntime& modal) : modal_(modal) {}
  bool valid(PageModalToken token) const override {
    return token.kind == ControlModalKind::WIFI_QR && !espcontrol::screensaver_pin().locked() &&
           modal_.valid(token);
  }
  bool connected() const override { return ha_api_state_connected(); }
  bool service_available() const override { return ha_api_available(); }
  bool ready() const override { return page_native_command_ready(); }
  bool send(std::string_view entity, bool turn_on) override {
    return ready() && connected() && service_available() && !espcontrol::screensaver_pin().locked() &&
           guest_wifi_valid_entity(entity) &&
           ha_send_entity_action(std::string(entity),
                                 turn_on ? "homeassistant.turn_on" : "homeassistant.turn_off");
  }

 private:
  PageModalRuntime& modal_;
};
}  // namespace espcontrol::pages
