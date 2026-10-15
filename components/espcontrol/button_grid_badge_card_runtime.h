#pragma once
#include "badge_card_display_policy.h"

// Card-backed badges own a normal card runtime. The compact surface mirrors
// its state and uses its original slider or click handler. Modal controls
// remain full-size and need no matching grid card.
namespace espcontrol::cards {

struct BadgeCardRuntime {
  BadgeCtx *badge;
  BtnSlot slot{};
  ParsedCfg config;
  GridConfig grid;
  lv_obj_t *source_slider = nullptr;
  espcontrol::badge::CardContent content = espcontrol::badge::CardContent::AUTO;
  std::string caption;
  const ThemePalette *theme = &current_theme();

  void refresh() {
    auto *ctx = badge;
    if (!ctx->value_label) return;
    if (theme != &current_theme()) {
      const auto &targets = theme_grid_targets();
      ThemeTreeCorrection correction{targets.red_percent, targets.green_percent, targets.blue_percent};
      theme_restyle_tree(slot.btn, *theme, current_theme(), lv_obj_has_state(slot.btn, LV_STATE_CHECKED), correction);
      theme = &current_theme();
    }
    const bool numeric = slot.sensor_container && !lv_obj_has_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN) &&
      slot.sensor_lbl && !lv_obj_has_flag(slot.sensor_lbl, LV_OBJ_FLAG_HIDDEN);
    using Content = espcontrol::badge::CardContent;
    std::string text = lv_label_get_text(slot.text_lbl);
    if ((content == Content::VALUE || content == Content::AUTO || content == Content::DATE || content == Content::DATETIME) && numeric) {
      text = lv_label_get_text(slot.sensor_lbl);
      if (slot.unit_lbl && !lv_obj_has_flag(slot.unit_lbl, LV_OBJ_FLAG_HIDDEN)) text += lv_label_get_text(slot.unit_lbl);
      if (text != "--" && content == Content::DATE) text += std::string(" ") + lv_label_get_text(slot.text_lbl);
      if (text != "--" && content == Content::DATETIME) text = std::string(lv_label_get_text(slot.text_lbl)) + " · " + text;
    }
    if (content == Content::TRACK && slot.sensor_container &&
        (media_card_mode(config.sensor) == "now_playing" || media_card_mode(config.sensor) == "cover_art")) {
      auto *media = static_cast<MediaNowPlayingCtx *>(lv_obj_get_user_data(slot.sensor_container));
      if (media && media->title_lbl && !lv_obj_has_flag(media->title_lbl, LV_OBJ_FLAG_HIDDEN)) text = lv_label_get_text(media->title_lbl);
      else if (media && media->idle_lbl && !lv_obj_has_flag(media->idle_lbl, LV_OBJ_FLAG_HIDDEN)) text = lv_label_get_text(media->idle_lbl);
    }
    if (content == Content::SLIDER && source_slider) {
      const auto *control = static_cast<SliderCtx *>(lv_obj_get_user_data(source_slider));
      const int position = lv_slider_get_value(source_slider);
      if (!control || !control->available) text = "--";
      else if (control->numeric) {
        text = control->numeric_metadata_ready && control->numeric_state_known
          ? espcontrol::badge::slider_value_text(espcontrol::number_slider::value_for_position(slider_numeric_metadata(control), position)) + control->numeric_unit : "--";
      } else if (control->light_temp) {
        text = control->light_temp_has_kelvin
          ? std::to_string(control->kelvin_min + position * (control->kelvin_max - control->kelvin_min) / 100) + "K" : "--";
      } else if (media_now_playing_progress_enabled(config)) {
        // Keep the track title while its original seek slider fills the pill.
      } else if (control->media_position && control->media_value_lbl) text = lv_label_get_text(control->media_value_lbl);
      else text = std::to_string(position) + "%";
    }
    if (content == Content::LABEL) {
      text = caption;
    }
    if (text.empty()) text = config.label.empty() ? espcontrol_i18n(card_runtime_label(config.type)) : config.label;
    if (slot.icon_lbl && std::strlen(lv_label_get_text(slot.icon_lbl))) {
      const char *icon = lv_label_get_text(slot.icon_lbl);
      if (std::strcmp(icon, lv_label_get_text(ctx->icon_label))) lv_label_set_display_text(ctx->icon_label, icon);
    }
    lv_obj_t *state_button = slot.btn;
    const bool unavailable = lv_obj_has_state(state_button, LV_STATE_DISABLED);
    set_card_disabled_state(ctx->button, unavailable);
    set_card_checked_state(ctx->button, lv_obj_has_state(state_button, LV_STATE_CHECKED));
    // Carry the card's current status colour across the whole pill, including
    // checked and pressed selectors installed by the badge row's palette.
    const auto background = lv_obj_get_style_bg_color(state_button, LV_PART_MAIN);
    const auto foreground = lv_obj_get_style_text_color(state_button, LV_PART_MAIN);
    for (lv_state_t state : std::initializer_list<lv_state_t>{LV_STATE_DEFAULT, LV_STATE_CHECKED, LV_STATE_PRESSED,
                             static_cast<lv_state_t>(LV_STATE_CHECKED | LV_STATE_PRESSED)}) {
      const lv_style_selector_t selector = static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(state);
      lv_obj_set_style_bg_color(ctx->button, background, selector);
      lv_obj_set_style_text_color(ctx->button, foreground, selector);
    }
    sync_card_checked_text_color(ctx->button);
    badge_present_text(ctx, caption, text, content == Content::LABEL);
    if (ctx->slider && source_slider) {
      set_card_disabled_state(ctx->slider, unavailable);
      const auto *control = static_cast<SliderCtx *>(lv_obj_get_user_data(source_slider));
      badge_present_progress(ctx, control && control->fill
        ? lv_obj_get_style_bg_color(control->fill, LV_PART_MAIN) : lv_color_hex(current_button_primary_color()));
    }
  }
};

struct BadgePendingCardRuntime {
  BadgeCtx *badge;
  ParsedCfg config;
  GridConfig grid;
  CardPalette palette;
  BtnSlot reference;
};
inline std::vector<BadgePendingCardRuntime *> &badge_pending_card_runtimes() {
  static std::vector<BadgePendingCardRuntime *> runtimes;
  return runtimes;
}

inline void badge_card_runtime_queue(BadgeCtx *ctx, const ParsedCfg &config,
                                    const GridConfig &grid, const CardPalette &palette,
                                    const BtnSlot &reference) {
  auto *pending = new BadgePendingCardRuntime{ctx, config, grid, palette, reference};
  badge_pending_card_runtimes().push_back(pending);
  ctx->card_cleanup = [pending] {
    auto &queue = badge_pending_card_runtimes();
    queue.erase(std::remove(queue.begin(), queue.end(), pending), queue.end());
    delete pending;
  };
}

inline void badge_card_runtime_bind_pending() {
  // Bind grid cards first so badges cannot take their image downloaders.
  auto &queue = badge_pending_card_runtimes();
  while (!queue.empty()) {
    auto *pending = queue.back();
    queue.pop_back();
    pending->badge->card_cleanup = {};
    badge_card_runtime_setup(pending->badge, pending->config, pending->grid, pending->palette, pending->reference);
    delete pending;
  }
}

inline std::vector<BadgeCardRuntime *> &badge_card_runtimes() {
  static std::vector<BadgeCardRuntime *> runtimes;
  return runtimes;
}
inline lv_timer_t *&badge_card_refresh_timer() { static lv_timer_t *timer = nullptr; return timer; }

inline lv_obj_t *badge_card_find_slider(lv_obj_t *parent) {
  if (lv_obj_check_type(parent, &lv_slider_class)) return parent;
  for (uint32_t index = 0; index < lv_obj_get_child_cnt(parent); ++index) {
    if (auto *slider = badge_card_find_slider(lv_obj_get_child(parent, index))) return slider;
  }
  return nullptr;
}

inline bool badge_card_bind_runtime(BadgeCardRuntime &runtime, const CardPalette &base_palette) {
  auto &slot = runtime.slot;
  const auto &config = runtime.config;
  const auto &grid = runtime.grid;
  const auto context = card_runtime_context(config, Surface::SUBPAGE);
  const auto display = display_profile_from_grid_config(grid);
  const auto palette = card_palette_for_config(base_palette, config, display);
  HaCallbackOwnerScope owner(slot.btn);
  if (image_driver_bind_subpage(slot, config, context, grid, palette)) return true;
  if (wifi_qr_driver_bind_subpage(slot, config, context)) return true;
  if (light_control_driver_bind_subpage(slot, config, context, light_control_driver_environment(palette, display, slot))) return true;
  if (fan_control_driver_bind_subpage(slot, config, context, fan_control_driver_environment(palette, display, slot))) return true;
  if (climate_control_driver_bind_subpage(slot, config, context, climate_control_driver_environment(palette, display, slot))) return true;
  if (alarm_driver_bind_subpage(slot, config, context, alarm_driver_environment(palette, display, slot, grid, lv_obj_get_screen(runtime.badge->button), grid.num_slots, grid.cols))) return true;
  if (cover_modal_driver_bind_subpage(slot, config, context, cover_modal_driver_environment(palette, display, slot))) return true;
  if (media_driver_bind_subpage(slot, config, context, media_driver_environment(palette, display, slot, grid))) return true;
  if (timer_driver_bind_data(slot, config, context)) return true;
  if (bind_basic_sensor_card(slot, config, context, palette, 1)) return true;
  BasicActionSubpageEnvironment action;
  action.grid_config = &grid; action.palette = palette; action.display = display;
  action.grid_page = lv_obj_get_screen(runtime.badge->button); action.grid_cols = grid.cols;
  if (basic_action_driver_bind_subpage(slot, config, context, action)) return true;
  NumericSelectableSubpageEnvironment numeric;
  numeric.palette = palette; numeric.display = display;
  if (numeric_selectable_driver_bind_subpage(slot, config, context, numeric)) return true;
  if (cleaning_driver_bind_subpage(slot, config, context, {})) return true;
  AccessCoverDriverSubpageEnvironment access;
  access.add_toggle_click = [](lv_obj_t *button, const std::string &entity, bool) {
    auto *stored = grid_delete_with_owner(button, new std::string(entity));
    lv_obj_add_event_cb(button, [](lv_event_t *event) {
      auto *entity = static_cast<std::string *>(lv_event_get_user_data(event));
      if (entity) send_toggle_action(*entity);
    }, LV_EVENT_CLICKED, stored);
  };
  return access_cover_driver_bind_subpage(slot, config, context, access);
}

inline void badge_card_runtime_setup(BadgeCtx *ctx, const ParsedCfg &config,
                                    const GridConfig &grid, const CardPalette &palette,
                                    const BtnSlot &reference) {
  auto *runtime = new BadgeCardRuntime();
  runtime->badge = ctx; runtime->grid = grid; runtime->config = config;
  runtime->config.type = cfg_option_value(config.options, "badge_card_type");
  if (runtime->config.type == "switch") runtime->config.type.clear();
  if (runtime->config.type == "badge" || runtime->config.type == "subpage" || !card_runtime_context(runtime->config, Surface::SUBPAGE).known) {
    set_card_disabled_state(ctx->button, true);
    delete runtime;
    return;
  }
  runtime->config.options = espcontrol::badge::card_options(config.options);
  runtime->content = espcontrol::badge::card_content(runtime->config.type, runtime->config.sensor,
    runtime->config.precision, cfg_option_value(config.options, "badge_display"));
  runtime->caption = runtime->config.label.empty()
    ? espcontrol_i18n(card_runtime_label(runtime->config.type)) : runtime->config.label;
  const auto display = display_profile_from_grid_config(grid);
  const auto *font = lv_obj_get_style_text_font(reference.text_lbl, LV_PART_MAIN);
  const auto color = lv_obj_get_style_text_color(reference.btn, LV_PART_MAIN);
  auto *proxy = create_grid_card_button(lv_obj_get_screen(ctx->button), 0, 0, font, color);
  lv_obj_add_flag(proxy, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_clear_flag(proxy, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_set_size(proxy, std::max(160, static_cast<int>(lv_obj_get_width(reference.btn))),
                         std::max(120, static_cast<int>(lv_obj_get_height(reference.btn))));
  runtime->slot = create_dynamic_card_slot(proxy, display_icon_font(display), display_sensor_font(display), font, color, grid.subpage_chevron_font);
  setup_card_visual(runtime->slot, runtime->config, card_runtime_context(runtime->config, Surface::SUBPAGE), runtime->grid, palette, 1, 1);
  badge_card_bind_runtime(*runtime, palette);
  if (runtime->config.label.empty()) {
    const auto &sensor = runtime->config.sensor;
    const auto &entity = runtime->config.entity;
    const auto target = sensor.find('.') != std::string::npos ? sensor : entity;
    if (target.find('.') != std::string::npos) {
      HaCallbackOwnerScope owner(proxy);
      ha_subscribe_attribute(target, "friendly_name", [runtime](esphome::StringRef name) {
        if (!ha_state_unavailable_ref(name) && !name.empty()) runtime->caption = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
      });
    }
  }
  runtime->source_slider = badge_card_find_slider(proxy);
  lv_obj_add_flag(proxy, LV_OBJ_FLAG_HIDDEN);
  ctx->card_refresh = [runtime] { runtime->refresh(); };
  ctx->card_click = [runtime] {
    if (runtime->source_slider || lv_obj_has_state(runtime->slot.btn, LV_STATE_DISABLED) ||
        (screen_lock_enabled() && runtime->config.type != "screen_lock") || alarm_display_takeover_active()) return;
    const auto event = runtime->config.type == "media" && media_fast_press_mode(media_card_mode(runtime->config.sensor))
      ? LV_EVENT_PRESSED : LV_EVENT_CLICKED;
    lv_obj_send_event(runtime->slot.btn, event, nullptr);
    runtime->refresh();
  };
  if (runtime->source_slider) {
    auto *slider = runtime->source_slider;
    auto *control = static_cast<SliderCtx *>(lv_obj_get_user_data(slider));
    if (control) {
      if (control->geometry_timer) { lv_timer_del(control->geometry_timer); control->geometry_timer = nullptr; }
      if (control->geometry_parent) lv_obj_remove_event_cb_with_user_data(control->geometry_parent, slider_geometry_refresh_event_cb, slider);
      control->horizontal = true;
      control->geometry_parent = ctx->button;
    }
    // Keep the original widget and callbacks: numeric steps, capability checks
    // and release commands remain the card driver's responsibility.
    ctx->slider_action = true;
    ctx->slider = slider;
    lv_obj_set_parent(slider, ctx->button);
    lv_obj_remove_style_all(slider);
    lv_obj_clear_flag(slider, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(slider, LV_PCT(100), LV_PCT(100));
    lv_obj_center(slider);
    lv_obj_set_style_radius(slider, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(current_button_primary_color()), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_width(slider, 0, LV_PART_KNOB);
    lv_obj_set_style_height(slider, 0, LV_PART_KNOB);
    lv_obj_move_to_index(slider, 0);
    screen_lock_register_controlled_button(slider);
    lv_obj_add_event_cb(slider, [](lv_event_t *event) {
      auto *ctx = static_cast<BadgeCtx *>(lv_event_get_user_data(event));
      if (!ctx) return;
      const auto code = lv_event_get_code(event);
      if (code == LV_EVENT_DELETE) { ctx->slider = nullptr; return; }
      if (code == LV_EVENT_PRESSED) ctx->dragging = true;
      if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) ctx->dragging = false;
      if (ctx->card_refresh && (code == LV_EVENT_PRESSED || code == LV_EVENT_VALUE_CHANGED || code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)) ctx->card_refresh();
    }, LV_EVENT_ALL, ctx);
  }
  badge_card_runtimes().push_back(runtime);
  if (!badge_card_refresh_timer()) badge_card_refresh_timer() = lv_timer_create([](lv_timer_t *) {
    for (auto *runtime : badge_card_runtimes()) runtime->refresh();
  }, 200, nullptr);
  ctx->card_cleanup = [runtime] {
    auto &runtimes = badge_card_runtimes();
    runtimes.erase(std::remove(runtimes.begin(), runtimes.end(), runtime), runtimes.end());
    if (runtimes.empty() && badge_card_refresh_timer()) { lv_timer_del(badge_card_refresh_timer()); badge_card_refresh_timer() = nullptr; }
    ha_release_callbacks_for_owner(runtime->slot.btn);
    screen_lock_unregister_tree(runtime->slot.btn);
    if (runtime->badge->slider) {
      screen_lock_unregister_tree(runtime->badge->slider);
      lv_obj_del(runtime->badge->slider);
    }
    grid_release_runtime_allocations(runtime->slot.btn);
    lv_obj_del(runtime->slot.btn);
    compact_calendar_card_refs();
    compact_timezone_card_refs();
    delete runtime;
  };
  runtime->refresh();
}

}  // namespace espcontrol::cards
