#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

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
};

inline std::vector<NavigationHomeTargetEntry> &navigation_home_targets() {
  static std::vector<NavigationHomeTargetEntry> entries;
  return entries;
}

inline std::vector<NavigationSubpageEntry> &navigation_subpages() {
  static std::vector<NavigationSubpageEntry> entries;
  return entries;
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
    button_grid_load_screen(main_page_obj);
  }
  return true;
}

inline void navigation_clear_home_targets() {
  navigation_home_targets().clear();
}

inline void navigation_clear_subpages() {
  lv_obj_t *active = lv_scr_act();
  for (auto &entry : navigation_subpages()) {
    if (entry.screen != nullptr && entry.screen != active) {
      lv_obj_del(entry.screen);
    }
  }
  navigation_subpages().clear();
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

inline bool navigation_open_first_kind(const std::string &kind,
                                       lv_obj_t *main_page_obj) {
  navigation_hide_modals();
  NavigationSubpageEntry *target = navigation_find_first_kind(kind);
  if (target == nullptr) {
    ESP_LOGW("navigation", "No subpage of kind '%s'", navigation_trim(kind).c_str());
    return false;
  }
  button_grid_load_screen(target->screen);
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
