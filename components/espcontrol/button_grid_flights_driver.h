#pragma once

#include "card_modal_target.h"

// Shared main-grid/subpage lifecycle. The grid releases tracked allocations
// before rebinding; FlightsCardCtx releases its subscriptions and open modal.
template<typename T>
inline T *grid_track_runtime_allocation(lv_obj_t *owner, T *ptr);

namespace espcontrol::cards {
inline bool flights_driver_matches(const Context &context) {
  return context.runtime.driver == card_runtime::CardDriverId::FLIGHTS;
}

inline bool flights_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!flights_driver_matches(context)) return false;
  lv_obj_set_user_data(slot.btn, nullptr);
  setup_flights_card(slot, config);
  return true;
}

inline bool flights_driver_handle_main_click(
    const Context &context, const ParsedCfg &, lv_obj_t *button) {
  if (!flights_driver_matches(context)) return false;
  if (button) handle_flights_card_click(
    static_cast<FlightsCardCtx *>(lv_obj_get_user_data(button)));
  return true;
}

inline bool flights_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!flights_driver_matches(context)) return false;
  lv_obj_set_user_data(slot.btn, nullptr);
  if (config.entity.compare(0, 7, "sensor.") != 0) return true;
  auto *flights = grid_track_runtime_allocation(slot.btn, new FlightsCardCtx());
  flights->entity_id = config.entity;
  flights->configured_label = config.label;
  flights->btn = slot.btn;
  flights->value_lbl = slot.sensor_lbl;
  flights->text_lbl = slot.text_lbl;
  flights->label_font = slot.text_lbl
    ? lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN) : nullptr;
  flights->icon_font = slot.icon_lbl
    ? lv_obj_get_style_text_font(slot.icon_lbl, LV_PART_MAIN) : nullptr;
  flights->imperial = cfg_option_value(config.options, "units") == "imperial";
  lv_obj_set_user_data(slot.btn, flights);
  subscribe_flights_card(flights);
  if (context.surface == Surface::SUBPAGE) {
    lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
      handle_flights_card_click(
        static_cast<FlightsCardCtx *>(lv_event_get_user_data(event)));
    }, LV_EVENT_CLICKED, flights);
  }
  return true;
}

inline ModalTarget flights_driver_modal_target(
    const Context &context, const ParsedCfg &config, lv_obj_t *button) {
  if (!flights_driver_matches(context) || !button) return {};
  auto *flights = static_cast<FlightsCardCtx *>(lv_obj_get_user_data(button));
  if (!flights) return {};
  ModalTarget target;
  target.entity = config.entity;
  target.kind = ControlModalKind::FLIGHTS;
  target.available = flights->available;
  target.open = [button]() {
    handle_flights_card_click(static_cast<FlightsCardCtx *>(lv_obj_get_user_data(button)));
  };
  return target;
}
}  // namespace espcontrol::cards
