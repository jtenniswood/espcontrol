#pragma once

// Include after button_grid.h: consume the established service/local/webhook
// helpers without registering any of the legacy per-card interaction handlers.
#include "modal_callback_dispatch.h"
#include "page_card_commands.h"
#include "page_lvgl_dispatch.h"
#include "page_modal_admission.h"

namespace espcontrol::pages {
class PageNativeCardCommandPort final : public PageCardCommandPort {
 public:
  using OriginValid = bool (*)(void*, PageBindingToken);
  PageNativeCardCommandPort(void* owner, OriginValid valid) : owner_(owner), valid_(valid) {}
  bool ready() const override {
    return lv_is_initialized() && !page_lvgl_dispatching() && !modal_callback_dispatching() &&
           !ha_read_coordinator().dispatching();
  }
  bool valid(PageBindingToken token) const override {
    // The scene additionally checks active display/wake guard and modal cover.
    return valid_ && !espcontrol::screensaver_pin().locked() && valid_(owner_, token);
  }
  bool connected() const override { return ha_api_state_connected(); }
  bool service_available() const override { return ha_api_available(); }
  void dispatch(const PageCardCommandSnapshot& snapshot) override {
    assert(ready());
    const auto& config = snapshot.config;
    switch (snapshot.kind) {
      case PageCardCommandKind::TOGGLE:
        send_toggle_action(config.entity);
        break;
      case PageCardCommandKind::BUTTON:
        ha_send_entity_action(config.entity, "button.press");
        break;
      case PageCardCommandKind::TURN_ON:
        send_turn_on_action(config.entity);
        break;
      case PageCardCommandKind::TURN_OFF:
        send_turn_off_action(config.entity);
        break;
      case PageCardCommandKind::OPEN:
        ha_send_entity_action(config.entity, "cover.open_cover");
        break;
      case PageCardCommandKind::CLOSE:
        ha_send_entity_action(config.entity, "cover.close_cover");
        break;
      case PageCardCommandKind::ACTION:
        send_action_card_action(config);
        break;
      case PageCardCommandKind::LOCAL:
        send_local_action(config.entity);
        break;
      case PageCardCommandKind::RELAY:
        send_internal_relay_action(config);
        break;
      case PageCardCommandKind::WEBHOOK:
        send_webhook_action(config);
        break;
      case PageCardCommandKind::SCREENSAVER:
        if (force_screensaver_action()) force_screensaver_action()();
        break;
      case PageCardCommandKind::SCREEN_LOCK:
        screen_lock_activate(admission::option_token(config.options, "screensaver_on_lock"));
        break;
      case PageCardCommandKind::PUSH: {
        std::string label = config.label;
        if (label.empty()) {
          if (snapshot.root) {
            char buffer[24];
            snprintf(buffer, sizeof(buffer), "Push %u", static_cast<unsigned>(snapshot.slot));
            label = buffer;
          } else
            label = espcontrol_i18n(std::string("Push"));
        }
        esphome::api::HomeassistantActionRequest request;
        if (!ha_action_begin(request, "esphome.push_button_pressed", true, snapshot.root ? 2 : 1)) break;
        ha_action_add_data(request, "label", label.c_str());
        if (snapshot.root) {
          char buffer[8];
          snprintf(buffer, sizeof(buffer), "%u", static_cast<unsigned>(snapshot.slot));
          ha_action_add_data(request, "slot", buffer);
        }
        ha_action_send(request);
        break;
      }
      case PageCardCommandKind::NONE:
        break;
    }
  }

 private:
  void* owner_;
  OriginValid valid_;
};
}  // namespace espcontrol::pages
