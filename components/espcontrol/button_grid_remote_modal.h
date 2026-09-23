#pragma once

// Included after the card drivers. Resolve afresh before and after waking;
// this file owns no card runtime, subscriptions, or persistent target cache.

namespace espcontrol::cards {

inline ModalTarget card_modal_target(const Context &context, const ParsedCfg &config,
                                      lv_obj_t *button) {
  using Probe = ModalTarget (*)(const Context &, const ParsedCfg &, lv_obj_t *);
  static const Probe probes[] = {
    light_control_driver_modal_target, cover_modal_driver_modal_target,
    climate_control_driver_modal_target, fan_control_driver_modal_target,
    numeric_selectable_driver_modal_target, media_driver_modal_target,
    image_driver_modal_target, alarm_driver_modal_target, wifi_qr_driver_modal_target,
  };
  for (auto probe : probes) {
    ModalTarget target = probe(context, config, button);
    if (target.supported()) return target;
  }
  return {};
}

}  // namespace espcontrol::cards

struct RemoteModalSelection {
  espcontrol::cards::ModalTarget target;
  int slot = 0;
  int subcard = 0;
  int display_order = 0;
  int child_order = 0;
  int matches = 0;
  bool entity_configured = false;

  void consider(espcontrol::cards::ModalTarget candidate, const std::string &wanted,
                const std::string &configured_entity, int candidate_slot,
                int candidate_subcard, int order, int child) {
    if (configured_entity == wanted || candidate.entity == wanted) entity_configured = true;
    if (!candidate.supported() || candidate.entity != wanted) return;
    ++matches;
    const bool home = candidate_subcard == 0;
    const bool best_home = subcard == 0;
    if (slot != 0 && !(home && !best_home) &&
        !(home == best_home && (order < display_order ||
          (order == display_order && child < child_order)))) return;
    target = std::move(candidate);
    slot = candidate_slot;
    subcard = candidate_subcard;
    display_order = order;
    child_order = child;
  }
};

inline RemoteModalSelection remote_modal_find(const std::string &entity) {
  RemoteModalSelection selection;
  for (const auto &entry : navigation_home_targets()) {
    if (!entry.button || lv_obj_has_flag(entry.button, LV_OBJ_FLAG_HIDDEN)) continue;
    const ParsedCfg config = parse_cfg(entry.config);
    selection.consider(espcontrol::cards::card_modal_target(
      card_runtime_context(config), config, entry.button), entity, config.entity,
      entry.slot, 0, entry.display_order, 0);
  }
  for (const auto &page : navigation_subpages()) {
    const auto *parent = navigation_find_slot_target(page.slot);
    if (!page.screen || !parent || !parent->button ||
        lv_obj_has_flag(parent->button, LV_OBJ_FLAG_HIDDEN)) continue;
    for (const auto &card : page.cards) {
      if (!card.button || lv_obj_has_flag(card.button, LV_OBJ_FLAG_HIDDEN)) continue;
      const ParsedCfg config = parsed_cfg_from_subpage_btn(card.definition);
      selection.consider(espcontrol::cards::card_modal_target(
        card_runtime_context(config, espcontrol::cards::Surface::SUBPAGE),
        config, card.button), entity, config.entity, page.slot, card.index,
        parent->display_order, card.display_order);
    }
  }
  return selection;
}

inline bool remote_modal_resolve(const std::string &entity_id, bool ui_ready,
                                  RemoteModalSelection &selection) {
  const std::string entity = navigation_trim(entity_id);
  const char *reason = nullptr;
  if (entity.empty()) reason = "empty entity ID";
  else if (!ui_ready) reason = "display is not ready";
  else if (screen_lock_enabled()) reason = "screen is locked";
  else if (alarm_display_takeover_active()) reason = "alarm display is active";
  if (reason) {
    ESP_LOGW("open_modal", "Rejected request: %s", reason);
    return false;
  }
  selection = remote_modal_find(entity);
  if (selection.slot == 0) {
    ESP_LOGW("open_modal", "%s: %s", entity.c_str(), selection.entity_configured
      ? "configured card has no control modal" : "no configured card");
    return false;
  }
  if (selection.matches > 1) {
    ESP_LOGI("open_modal", "%s: %d matching cards; using slot %d, subcard %d",
             entity.c_str(), selection.matches, selection.slot, selection.subcard);
  }
  if (!selection.target.available || !selection.target.open) {
    ESP_LOGW("open_modal", "%s: selected control is unavailable", entity.c_str());
    return false;
  }
  return true;
}

inline bool espcontrol_can_open_modal(const std::string &entity_id, bool ui_ready) {
  RemoteModalSelection selection;
  return remote_modal_resolve(entity_id, ui_ready, selection);
}

inline bool espcontrol_open_modal(const std::string &entity_id, bool ui_ready) {
  RemoteModalSelection selection;
  if (!remote_modal_resolve(entity_id, ui_ready, selection)) return false;
  // Keep the underlying screen. Closing a remote subpage modal must not navigate.
  navigation_hide_modals();
  selection.target.open();
  const auto &active = control_modal_active();
  if (!active.overlay || active.kind != selection.target.kind) {
    ESP_LOGW("open_modal", "%s: modal creation failed", selection.target.entity.c_str());
    return false;
  }
  ESP_LOGI("open_modal", "Opened %s (slot %d, subcard %d)",
           selection.target.entity.c_str(), selection.slot, selection.subcard);
  return true;
}

inline void espcontrol_close_modal() {
  control_modal_close_nested_menu();
  control_modal_force_close_active();
}

inline bool remote_subpage_resolve(const std::string &label, bool ui_ready,
                                   NavigationHomeTargetEntry *&target) {
  const std::string wanted = navigation_trim(label);
  const char *reason = nullptr;
  if (wanted.empty()) reason = "empty subpage label";
  else if (!ui_ready) reason = "display is not ready";
  else if (screen_lock_enabled()) reason = "screen is locked";
  else if (alarm_display_takeover_active()) reason = "alarm display is active";
  if (reason) {
    ESP_LOGW("open_subpage", "Rejected request: %s", reason);
    return false;
  }

  const std::string normalized = navigation_lower(wanted);
  int matches = 0;
  for (auto &entry : navigation_home_targets()) {
    NavigationSubpageEntry *page = navigation_find_slot(entry.slot);
    if (!entry.button || entry.label.empty() ||
        navigation_lower(entry.label) != normalized ||
        parse_cfg(entry.config).type != "subpage" ||
        page == nullptr || page->screen == nullptr) {
      continue;
    }
    ++matches;
    if (!target || entry.display_order < target->display_order) target = &entry;
  }
  if (!target) {
    ESP_LOGW("open_subpage", "No subpage labelled '%s'", wanted.c_str());
    return false;
  }
  if (matches > 1) {
    ESP_LOGI("open_subpage", "Multiple subpages are labelled '%s'; using slot %d",
             wanted.c_str(), target->slot);
  }
  return true;
}

inline bool espcontrol_can_open_subpage(const std::string &label, bool ui_ready) {
  NavigationHomeTargetEntry *target = nullptr;
  return remote_subpage_resolve(label, ui_ready, target);
}

inline bool espcontrol_open_subpage(const std::string &label, bool ui_ready,
                                    lv_obj_t *main_page_obj) {
  NavigationHomeTargetEntry *target = nullptr;
  if (!remote_subpage_resolve(label, ui_ready && main_page_obj != nullptr, target)) return false;
  return navigation_activate_home_target(target, main_page_obj);
}

inline bool espcontrol_close_subpage(lv_obj_t *main_page_obj) {
  if (alarm_display_takeover_active()) {
    ESP_LOGW("close_subpage", "Rejected request: alarm display is active");
    return false;
  }
  if (navigation_active_subpage_slot() == 0) {
    ESP_LOGI("close_subpage", "No subpage is open");
    return false;
  }
  if (main_page_obj == nullptr) {
    ESP_LOGW("close_subpage", "Main page is not ready");
    return false;
  }
  return navigation_return_home(main_page_obj);
}
