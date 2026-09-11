#pragma once

// Shared main-grid/subpage lifecycle. The grid releases tracked allocations
// before rebinding; TimerCardCtx releases its subscriptions and LVGL timers.
template<typename T>
inline T *grid_track_runtime_allocation(lv_obj_t *owner, T *ptr);

namespace espcontrol::cards {
inline bool timer_driver_matches(const Context &context) {
  return context.runtime.driver == card_runtime::CardDriverId::TIMER;
}

inline bool timer_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!timer_driver_matches(context)) return false;
  lv_obj_set_user_data(slot.btn, nullptr);
  setup_timer_card(slot, config, nullptr);
  return true;
}

inline bool timer_driver_handle_main_click(
    const Context &context, const ParsedCfg &, lv_obj_t *button) {
  if (!timer_driver_matches(context)) return false;
  if (button) handle_timer_card_click(
    static_cast<TimerCardCtx *>(lv_obj_get_user_data(button)));
  return true;
}

inline bool timer_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    std::function<void(const std::string &)> add_parent_indicator = {}) {
  if (!timer_driver_matches(context)) return false;
  lv_obj_set_user_data(slot.btn, nullptr);
  if (config.entity.compare(0, 6, "timer.") != 0) return true;
  auto *timer = grid_track_runtime_allocation(slot.btn, new TimerCardCtx());
  timer->entity_id = config.entity;
  timer->btn = slot.btn;
  timer->value_lbl = slot.sensor_lbl;
  timer->text_lbl = slot.text_lbl;
  timer->confirm_enabled = config.sensor == "confirm";
  timer->confirm_timeout_secs = timer_parse_confirm_timeout(config.unit);
  lv_obj_set_user_data(slot.btn, timer);
  subscribe_timer_card(timer);
  if (config.label.empty()) {
    HaCallbackOwnerScope callback_owner(timer);
    ha_subscribe_attribute(config.entity, "friendly_name", [timer](esphome::StringRef name) {
      if (name.empty()) return;
      const std::string label(name.c_str(), name.size());
      if (timer->confirm.armed) timer->confirm.original_text = label;
      else lv_label_set_display_text(timer->text_lbl, label.c_str());
    });
  }
  timer->tick_timer = lv_timer_create(timer_card_tick_cb, 250, timer);
  if (context.surface == Surface::SUBPAGE) {
    if (add_parent_indicator) add_parent_indicator(config.entity);
    lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
      handle_timer_card_click(static_cast<TimerCardCtx *>(lv_event_get_user_data(event)));
    }, LV_EVENT_CLICKED, timer);
  }
  return true;
}
}  // namespace espcontrol::cards
