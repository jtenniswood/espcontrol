#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

#include "grid_navigation_service.h"
#include "espcontrol_app_core.h"

// ── Home Assistant-driven home-screen navigation ─────────────────────

struct NavigationHomeTargetEntry {
  int slot = 0;
  int display_order = 0;
  std::string label;
  std::string config;
  lv_obj_t *button = nullptr;
};

struct NavigationSubpageEntry {
  int slot = 0;
  int display_order = 0;
  std::string kind;
  lv_obj_t *screen = nullptr;
  lv_obj_t *back_button = nullptr;
  BtnSlot back_slot{};
  struct Card {
    int index = 0;
    lv_obj_t *button = nullptr;
    BtnSlot slot{};
    SubpageBtn definition{};
  };
  std::vector<Card> cards;
};

inline void navigation_release_subpage_runtime(NavigationSubpageEntry &entry);

using ButtonGridNavigationService =
    GridNavigationService<NavigationHomeTargetEntry, NavigationSubpageEntry>;

inline ButtonGridNavigationService &grid_navigation_service() {
  if (espcontrol::EspControlAppCore *core =
          espcontrol::active_espcontrol_app_core()) {
    return core->grid_navigation_service<ButtonGridNavigationService>();
  }
  // ESPHome may dispatch navigation cleanup while the core is still being
  // registered during startup. Preserve that state until core ownership is
  // available instead of turning the callback into a reboot.
  static ButtonGridNavigationService service;
  return service;
}

// Compatibility accessors for existing grid code. New runtime ownership lives
// in GridNavigationService so it can be migrated independently of the UI.
inline std::vector<NavigationHomeTargetEntry> &navigation_home_targets() {
  return grid_navigation_service().home_targets();
}

inline std::vector<NavigationSubpageEntry> &navigation_subpages() {
  return grid_navigation_service().subpages();
}

inline std::string navigation_trim(const std::string &value) {
  size_t start = 0;
  while (start < value.size() &&
         std::isspace(static_cast<unsigned char>(value[start]))) {
    start++;
  }
  size_t end = value.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    end--;
  }
  return value.substr(start, end - start);
}

inline std::string navigation_lower(const std::string &value) {
  std::string out = value;
  for (char &ch : out) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return out;
}

inline void navigation_hide_modals() {
  control_modal_close_nested_menu();
  control_modal_force_close_active();
}

inline void navigation_close_modals_for_display_takeover() {
  control_modal_close_nested_menu();
  control_modal_close_for_display_takeover(alarm_display_takeover_active());
}

inline bool navigation_return_home(lv_obj_t *main_page_obj) {
  navigation_hide_modals();
  if (main_page_obj == nullptr) {
    ESP_LOGW("navigation", "Main page is not ready");
    return false;
  }
  if (lv_scr_act() != main_page_obj) {
    lv_scr_load_anim(main_page_obj, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
  }
  return true;
}

inline void navigation_clear_home_targets() {
  grid_navigation_service().clear_home_targets();
}

inline void navigation_clear_subpages() {
  lv_obj_t *active = lv_scr_act();
  for (auto &entry : navigation_subpages()) {
    if (entry.screen != nullptr && entry.screen != active) {
      lv_obj_del(entry.screen);
    }
  }
  grid_navigation_service().clear_subpages();
  clock_bar_clear_button_grid_pages();
}

inline void navigation_register_home_target(int slot, int display_order,
                                            const std::string &label,
                                            const std::string &config,
                                            lv_obj_t *button) {
  if (slot <= 0 || button == nullptr) return;
  NavigationHomeTargetEntry entry;
  entry.slot = slot;
  entry.display_order = display_order;
  entry.label = navigation_trim(label);
  entry.config = config;
  entry.button = button;
  navigation_home_targets().push_back(entry);
}

inline void navigation_register_subpage(int slot, int display_order,
                                        const std::string &kind,
                                        lv_obj_t *screen) {
  if (slot <= 0 || screen == nullptr) return;
  NavigationSubpageEntry entry;
  entry.slot = slot;
  entry.display_order = display_order;
  entry.kind = navigation_lower(navigation_trim(kind));
  entry.screen = screen;
  navigation_subpages().push_back(entry);
  clock_bar_register_button_grid_page(screen);
}

inline int navigation_slot_from_target(const std::string &target) {
  std::string value = navigation_lower(navigation_trim(target));
  const std::string prefix = "slot:";
  if (value.rfind(prefix, 0) != 0) return -1;
  value = navigation_trim(value.substr(prefix.size()));
  if (value.empty()) return -1;
  int slot = 0;
  for (char ch : value) {
    if (ch < '0' || ch > '9') return -1;
    slot = slot * 10 + (ch - '0');
    if (slot > MAX_GRID_SLOTS) return -1;
  }
  return slot;
}

inline bool navigation_is_voice_target(const std::string &target) {
  std::string normalized = navigation_lower(navigation_trim(target));
  return normalized == "voice" || normalized == "mic" ||
         normalized == "microphone" || normalized == "speaker" ||
         normalized == "volume" || normalized == "device_volume";
}

inline bool navigation_has_home_label_target(const std::string &target) {
  std::string wanted = navigation_lower(navigation_trim(target));
  if (wanted.empty()) return false;

  for (auto &entry : navigation_home_targets()) {
    if (entry.button == nullptr || entry.label.empty()) continue;
    if (navigation_lower(entry.label) == wanted) return true;
  }
  return false;
}

inline NavigationHomeTargetEntry *navigation_find_label_target(
    const std::string &target, bool *duplicate_found = nullptr) {
  if (duplicate_found) *duplicate_found = false;
  std::string wanted = navigation_lower(navigation_trim(target));
  if (wanted.empty()) return nullptr;

  NavigationHomeTargetEntry *best = nullptr;
  for (auto &entry : navigation_home_targets()) {
    if (entry.button == nullptr || entry.label.empty()) continue;
    if (navigation_lower(entry.label) != wanted) continue;
    if (best == nullptr || entry.display_order < best->display_order) {
      if (best != nullptr && duplicate_found) *duplicate_found = true;
      best = &entry;
    } else if (duplicate_found) {
      *duplicate_found = true;
    }
  }
  return best;
}

inline NavigationHomeTargetEntry *navigation_find_slot_target(int slot) {
  if (slot <= 0) return nullptr;
  for (auto &entry : navigation_home_targets()) {
    if (entry.slot == slot && entry.button != nullptr) return &entry;
  }
  return nullptr;
}

inline NavigationSubpageEntry *navigation_find_first_kind(const std::string &kind) {
  std::string wanted = navigation_lower(navigation_trim(kind));
  if (wanted.empty()) return nullptr;
  NavigationSubpageEntry *best = nullptr;
  for (auto &entry : navigation_subpages()) {
    if (entry.screen == nullptr || entry.kind != wanted) continue;
    if (best == nullptr || entry.display_order < best->display_order) {
      best = &entry;
    }
  }
  return best;
}

inline NavigationSubpageEntry *navigation_find_slot(int slot) {
  if (slot <= 0) return nullptr;
  for (auto &entry : navigation_subpages()) {
    if (entry.screen != nullptr && entry.slot == slot) return &entry;
  }
  return nullptr;
}

inline int navigation_active_subpage_slot() {
  lv_obj_t *active = lv_scr_act();
  for (const auto &entry : navigation_subpages()) {
    if (entry.screen == active) return entry.slot;
  }
  return 0;
}

inline std::string navigation_active_companion_subpage_label() {
  const int slot = navigation_active_subpage_slot();
  NavigationSubpageEntry *entry = navigation_find_slot(slot);
  NavigationHomeTargetEntry *parent = navigation_find_slot_target(slot);
  if (entry == nullptr || entry->kind != "app_shortcuts" || parent == nullptr) return "";
  const ParsedCfg parent_config = parse_cfg(parent->config);
  if (!companion_app_shortcuts_enabled(parent_config)) return "";
  if (!navigation_trim(parent->label).empty()) return navigation_trim(parent->label);
  if (parent_config.entity == "com.apple.Safari") return "Safari";
  if (parent_config.entity == "com.openai.codex") return "Codex";
  if (parent_config.entity == "com.tinyspeck.slackmacgap") return "Slack";
  return "";
}

inline void navigation_refresh_companion_subpage_label() {
  set_clock_bar_companion_subpage_label(navigation_active_companion_subpage_label());
}

inline bool navigation_return_from_companion_shortcuts_if_needed(
    lv_obj_t *main_page_obj) {
  // Automatic Companion navigation must not dismiss active alarm controls.
  if (alarm_display_takeover_active()) return false;
  if (!companion_subpage_return_requested().load()) return false;
  const int slot = navigation_active_subpage_slot();
  NavigationSubpageEntry *entry = navigation_find_slot(slot);
  NavigationHomeTargetEntry *parent = navigation_find_slot_target(slot);
  if (entry == nullptr || entry->kind != "app_shortcuts" || parent == nullptr) {
    companion_consume_subpage_return_request();
    return false;
  }
  const ParsedCfg parent_config = parse_cfg(parent->config);
  if (!companion_app_shortcuts_enabled(parent_config)) {
    companion_consume_subpage_return_request();
    return false;
  }
  companion_consume_subpage_return_request();
  return navigation_return_home(main_page_obj);
}

inline bool navigation_open_companion_subpage_if_requested(
    lv_obj_t *main_page_obj) {
  // Automatic app navigation must never dismiss alarm disarm/countdown UI.
  if (alarm_display_takeover_active()) return false;
  const std::string requested = companion_pending_auto_subpage_action();
  if (requested.empty()) return false;
  for (const auto &parent : navigation_home_targets()) {
    const ParsedCfg parent_config = parse_cfg(parent.config);
    if (!companion_app_subpage_auto_switch_enabled(parent_config) ||
        parent_config.entity != requested) continue;
    NavigationSubpageEntry *entry = navigation_find_slot(parent.slot);
    if (entry == nullptr || entry->screen == nullptr) return false;
    if (!companion_consume_auto_subpage_action(requested)) return false;
    navigation_hide_modals();
    if (lv_scr_act() != entry->screen) {
      lv_scr_load_anim(entry->screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }
    return true;
  }
  return false;
}

inline bool navigation_restore_subpage_slot(int slot) {
  NavigationSubpageEntry *entry = navigation_find_slot(slot);
  if (entry == nullptr || entry->screen == nullptr) return false;
  lv_scr_load_anim(entry->screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
  return true;
}

inline void navigation_register_subpage_back_button(int slot,
                                                    const BtnSlot &back_slot) {
  NavigationSubpageEntry *entry = navigation_find_slot(slot);
  if (entry != nullptr) {
    entry->back_button = back_slot.btn;
    entry->back_slot = back_slot;
  }
}

inline void navigation_register_subpage_card(int slot, int index,
                                             const BtnSlot &card_slot,
                                             const SubpageBtn &definition) {
  if (index <= 0 || card_slot.btn == nullptr) return;
  NavigationSubpageEntry *entry = navigation_find_slot(slot);
  if (entry == nullptr) return;
  entry->cards.push_back({index, card_slot.btn, card_slot, definition});
}

inline void navigation_retire_subpage(int slot, lv_obj_t *main_page_obj) {
  std::vector<NavigationSubpageEntry> &entries = navigation_subpages();
  for (auto it = entries.begin(); it != entries.end(); ++it) {
    if (it->slot != slot) continue;
    if (it->screen != nullptr && it->screen == lv_scr_act()) {
      navigation_return_home(main_page_obj);
    }
    navigation_release_subpage_runtime(*it);
    clock_bar_unregister_button_grid_page(it->screen);
    if (it->screen != nullptr) lv_obj_del(it->screen);
    entries.erase(it);
    return;
  }
}

inline NavigationSubpageEntry::Card *navigation_subpage_card(
    NavigationSubpageEntry &entry, int index) {
  for (auto &card : entry.cards) {
    if (card.index == index) return &card;
  }
  return nullptr;
}

inline lv_obj_t *navigation_subpage_card_button(
    const NavigationSubpageEntry &entry, int index) {
  for (const auto &card : entry.cards) {
    if (card.index == index) return card.button;
  }
  return nullptr;
}

inline bool navigation_open_first_kind(const std::string &kind,
                                       lv_obj_t *main_page_obj) {
  navigation_hide_modals();
  NavigationSubpageEntry *target = navigation_find_first_kind(kind);
  if (target == nullptr) {
    ESP_LOGW("navigation", "No subpage of kind '%s'", navigation_trim(kind).c_str());
    return false;
  }
  lv_scr_load_anim(target->screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
  return true;
}

inline bool navigation_activate_home_target(NavigationHomeTargetEntry *target,
                                            lv_obj_t *main_page_obj) {
  if (target == nullptr || target->button == nullptr) return false;
  if (!navigation_return_home(main_page_obj)) return false;
  handle_button_click(target->config, target->slot, target->button);
  return true;
}

inline bool espcontrol_navigate(const std::string &target,
                                lv_obj_t *main_page_obj) {
  std::string normalized = navigation_lower(navigation_trim(target));
  if (normalized.empty()) {
    ESP_LOGW("navigation", "Navigation target is empty");
    return false;
  }

  if (normalized == "home" || normalized == "main") {
    return navigation_return_home(main_page_obj);
  }

  navigation_hide_modals();

  bool duplicate_found = false;
  NavigationHomeTargetEntry *label_target =
    navigation_find_label_target(target, &duplicate_found);
  if (label_target != nullptr) {
    if (duplicate_found) {
      ESP_LOGW("navigation",
        "Multiple home-screen cards are labelled '%s'; activating slot %d",
        navigation_trim(target).c_str(), label_target->slot);
    }
    return navigation_activate_home_target(label_target, main_page_obj);
  }

  int slot = navigation_slot_from_target(target);
  if (slot > 0) {
    NavigationHomeTargetEntry *slot_target = navigation_find_slot_target(slot);
    if (slot_target != nullptr) {
      return navigation_activate_home_target(slot_target, main_page_obj);
    }
    ESP_LOGW("navigation", "Slot %d is not a configured home-screen card", slot);
    return false;
  }

  ESP_LOGW("navigation", "No home-screen card labelled '%s'", navigation_trim(target).c_str());
  return false;
}
