#pragma once

// Internal climate subscription maintenance. Included after the context and
// rendering helpers by button_grid_climate.h; host tests include this directly.

inline lv_timer_t *&climate_optional_subscription_timer() {
  static lv_timer_t *timer = nullptr;
  return timer;
}
inline espcontrol::climate::SubscriptionCapabilities
climate_subscription_capabilities(ClimateControlCtx *ctx) {
  espcontrol::climate::SubscriptionCapabilities capabilities;
  if (!ctx) return capabilities;
  capabilities.temperature = climate_temperature_target_available(ctx);
  capabilities.hvac = !ctx->hvac_modes.empty();
  capabilities.preset = !ctx->preset_modes.empty();
  capabilities.fan = !ctx->fan_modes.empty();
  capabilities.swing = !ctx->swing_modes.empty();
  return capabilities;
}

inline bool climate_subscribe_optional_field(ClimateControlCtx *ctx,
                                             uint8_t field) {
  if (!ctx || ctx->entity_id.empty() ||
      (ctx->optional_subscriptions.subscribed & field) != 0) return false;

  const char *attribute = nullptr;
  std::string ClimateControlCtx::*target = nullptr;
  if (field == espcontrol::climate::OPTIONAL_SUBSCRIPTION_PRESET) {
    attribute = "preset_mode";
    target = &ClimateControlCtx::preset_mode;
  } else if (field == espcontrol::climate::OPTIONAL_SUBSCRIPTION_FAN) {
    attribute = "fan_mode";
    target = &ClimateControlCtx::fan_mode;
  } else if (field == espcontrol::climate::OPTIONAL_SUBSCRIPTION_SWING) {
    attribute = "swing_mode";
    target = &ClimateControlCtx::swing_mode;
  } else {
    return false;
  }

  const uint32_t generation = ctx->subscription_generation;
  HaCallbackOwnerScope owner_scope(ctx);
  bool subscribed = ha_subscribe_attribute(
    ctx->entity_id, std::string(attribute),
    std::function<void(esphome::StringRef)>(
      [ctx, generation, target](esphome::StringRef value) {
        if (generation != ha_subscription_generation()) return;
        ctx->*target = climate_lower(climate_trim(
          string_ref_limited(value, HA_SHORT_STATE_MAX_LEN)));
        climate_update_card(ctx);
        climate_control_set_modal_value(ctx);
      })
  );
  if (subscribed) {
    ctx->optional_subscriptions.mark_subscribed(field);
  }
  return subscribed;
}

inline bool climate_subscribe_optional_fields(ClimateControlCtx *ctx,
                                              uint8_t fields) {
  bool added = false;
  const uint8_t optional_fields[] = {
    espcontrol::climate::OPTIONAL_SUBSCRIPTION_PRESET,
    espcontrol::climate::OPTIONAL_SUBSCRIPTION_FAN,
    espcontrol::climate::OPTIONAL_SUBSCRIPTION_SWING,
  };
  for (uint8_t field : optional_fields) {
    if ((fields & field) != 0) {
      added = climate_subscribe_optional_field(ctx, field) || added;
    }
  }
  return added;
}

inline void climate_process_pending_optional_subscriptions(lv_timer_t *timer);

inline void climate_schedule_optional_subscription_maintenance() {
  lv_timer_t *&timer = climate_optional_subscription_timer();
  if (timer != nullptr) return;
  timer = lv_timer_create(
    climate_process_pending_optional_subscriptions, 250, nullptr);
}

inline void climate_mark_optional_subscription_needs(ClimateControlCtx *ctx) {
  if (!ctx) return;
  uint8_t required = espcontrol::climate::required_optional_subscription_mask(
    ctx->configured_tab_mask, climate_subscription_capabilities(ctx));
  if (!ctx->optional_subscriptions.mark_required(required)) return;
  climate_schedule_optional_subscription_maintenance();
}

inline void climate_process_pending_optional_subscriptions(lv_timer_t *timer) {
  bool added = false;
  ClimateControlCtx **refs = climate_control_refs();
  const int count = climate_control_ref_count();
  for (int index = 0; index < count; index++) {
    ClimateControlCtx *ctx = refs[index];
    if (!ctx || ctx->optional_subscriptions.pending == 0) continue;
    if (ctx->subscription_generation != ha_subscription_generation()) {
      ctx->optional_subscriptions.clear_pending();
      continue;
    }
    uint8_t required = espcontrol::climate::required_optional_subscription_mask(
      ctx->configured_tab_mask, climate_subscription_capabilities(ctx));
    uint8_t missing = ctx->optional_subscriptions.update_required(required);
    added = climate_subscribe_optional_fields(ctx, missing) || added;
  }
  // A callback can attach to an append-only channel that Home Assistant
  // already knows. Reannounce that batch too so the new listener receives a
  // fresh current value instead of waiting for the attribute to change.
  if (added && ha_api_state_connected()) {
    ha_reannounce_state_subscriptions();
  }
  // Failed registrations stay pending on the same guarded timer. Recompute
  // capabilities next time so obsolete fallback requests are discarded.
  bool retry = false;
  for (int index = 0; index < climate_control_ref_count(); index++) {
    ClimateControlCtx *ctx = climate_control_refs()[index];
    retry = (ctx && ctx->optional_subscriptions.pending != 0) || retry;
  }
  if (!retry) {
    climate_optional_subscription_timer() = nullptr;
    lv_timer_del(timer);
  }
}
