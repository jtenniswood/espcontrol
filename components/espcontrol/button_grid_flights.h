#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

struct FlightsCardCtx {
  std::string entity_id;
  std::string configured_label;
  std::string friendly_name;
  lv_obj_t *count_lbl = nullptr;
  lv_obj_t *name_lbl = nullptr;
  bool available = false;
  std::string count;
  void detach();
  ~FlightsCardCtx() { detach(); }
};

inline void FlightsCardCtx::detach() {
  ha_release_callbacks_for_owner(this);
  count_lbl = nullptr;
  name_lbl = nullptr;
}

inline std::string flights_card_title(const FlightsCardCtx *ctx) {
  if (ctx && !ctx->configured_label.empty()) return ctx->configured_label;
  if (ctx && !ctx->friendly_name.empty()) return ctx->friendly_name;
  return espcontrol_i18n(std::string("Flights"));
}

inline void flights_apply_card_text(FlightsCardCtx *ctx) {
  if (!ctx) return;
  if (ctx->count_lbl) {
    const auto &count = ctx->count;
    lv_label_set_display_text(
      ctx->count_lbl, ctx->available && !count.empty() ? count.c_str() : "--");
  }
  if (ctx->name_lbl) {
    lv_label_set_display_text(ctx->name_lbl, flights_card_title(ctx).c_str());
  }
}

inline void subscribe_flights_card(FlightsCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  HaCallbackOwnerScope callback_owner(ctx);
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      ctx->available = !ha_entity_state_unavailable_ref(ctx->entity_id, state);
      ctx->count = ctx->available ? string_ref_limited(state, 8) : "";
      ESP_LOGD("flights", "%s count: %s", ctx->entity_id.c_str(),
               ctx->available ? ctx->count.c_str() : "unavailable");
      flights_apply_card_text(ctx);
    }), HA_SUBSCRIPTION_SCOPE_DEFAULT, true
  );
  if (ctx->configured_label.empty()) {
    ha_subscribe_attribute(
      ctx->entity_id, "friendly_name",
      std::function<void(esphome::StringRef)>([ctx](esphome::StringRef name) {
        ctx->friendly_name = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
        ESP_LOGD("flights", "%s sensor name received", ctx->entity_id.c_str());
        flights_apply_card_text(ctx);
      }), HA_SUBSCRIPTION_SCOPE_DEFAULT, true
    );
  }
}
