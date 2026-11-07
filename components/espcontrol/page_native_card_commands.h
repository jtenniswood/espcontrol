#pragma once

// Include after button_grid.h: consume the established service/local/webhook
// helpers without registering any of the legacy per-card interaction handlers.
#include "modal_callback_dispatch.h"
#include "page_card_commands.h"
#include "page_lvgl_dispatch.h"
#include "page_media_command_policy.h"
#include "page_modal_admission.h"
#include "page_native_device_commands.h"

namespace espcontrol::pages {
class PageNativeCardCommandPort final : public PageCardCommandPort {
 public:
  using OriginValid = bool (*)(void*, PageBindingToken);
  using Accepted = void (*)(void*, PageBindingToken, PageCardCommandKind, const PageCardCommandValue&);
  PageNativeCardCommandPort(void* owner, OriginValid valid, PageCardControlPolicy* controls = nullptr,
                            Accepted accepted = nullptr)
      : owner_(owner), valid_(valid), controls_(controls), accepted_(accepted) {}
  bool admit_control(const PageStore& store, PageBindingToken origin, PageCardCommandKind kind,
                     PageCardCommandValue& value) override {
    return controls_ && controls_->admit(store, origin, kind, value);
  }
  bool valid_control(const PageStore& store, PageBindingToken origin, PageCardCommandKind kind,
                     const PageCardCommandValue& value) override {
    return controls_ && controls_->valid(store, origin, kind, value);
  }
  void discard_control(PageBindingToken origin, PageCardCommandKind kind,
                       const PageCardCommandValue& value) override {
    if (controls_) controls_->discard(origin, kind, value);
  }
  void accepted_control(PageBindingToken origin, PageCardCommandKind kind,
                        const PageCardCommandValue& value) override {
    assert(ready());
    if (controls_) controls_->accepted(origin, kind, value);
    if (accepted_) accepted_(owner_, origin, kind, value);
  }
  bool dispatch_control(const PageCardCommandSnapshot& snapshot) override {
    assert(ready());
    const auto& config = snapshot.config;
    if (snapshot.kind == PageCardCommandKind::DEVICE_ACTION)
      return send_page_device_action(config, static_cast<PageDeviceAction>(snapshot.value.integer));
    if (snapshot.kind == PageCardCommandKind::MEDIA_ACTION) {
      const auto mode = page_media_command_mode(config.sensor);
      return mode == "playlist"
                 ? send_media_playlist_action(config)
                 : send_media_playback_action(config.entity,
                                              mode == "now_playing" ? "play_pause" : std::string(mode));
    }
    if (snapshot.kind == PageCardCommandKind::MEDIA_SEEK)
      return send_media_seek_action(config.entity, snapshot.value.integer,
                                    static_cast<float>(snapshot.value.number));
    if (snapshot.kind == PageCardCommandKind::PERCENT)
      return send_slider_action(config.entity, snapshot.value.integer,
                                config.type == "cover" && config.sensor == "tilt");
    if (snapshot.kind == PageCardCommandKind::NUMBER)
      return send_numeric_slider_action(config.entity, snapshot.value.number, snapshot.value.metadata);
    if (snapshot.kind == PageCardCommandKind::TIMER_START)
      return ha_send_entity_action(config.entity, "timer.start");
    if (snapshot.kind == PageCardCommandKind::TIMER_CANCEL)
      return ha_send_entity_action(config.entity, "timer.cancel");
    if (snapshot.kind == PageCardCommandKind::SLIDER_TOGGLE)
      return ha_send_entity_action(config.entity, "homeassistant.toggle");
    if (snapshot.kind == PageCardCommandKind::TEMPERATURE) {
      char buffer[16];
      snprintf(buffer, sizeof(buffer), "%d", snapshot.value.integer);
      return ha_send_entity_action(config.entity, "light.turn_on", "color_temp_kelvin", buffer);
    }
    return false;
  }
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
      case PageCardCommandKind::PERCENT:
      case PageCardCommandKind::NUMBER:
      case PageCardCommandKind::TEMPERATURE:
      case PageCardCommandKind::SLIDER_TOGGLE:
      case PageCardCommandKind::TIMER_START:
      case PageCardCommandKind::TIMER_CANCEL:
      case PageCardCommandKind::MEDIA_ACTION:
      case PageCardCommandKind::MEDIA_SEEK:
      case PageCardCommandKind::DEVICE_ACTION:
        dispatch_control(snapshot);
        break;
    }
  }

 private:
  void* owner_;
  OriginValid valid_;
  PageCardControlPolicy* controls_;
  Accepted accepted_;
};
}  // namespace espcontrol::pages
