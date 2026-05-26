#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Home Assistant-driven page navigation ────────────────────────────

struct NavigationSubpageEntry {
  int slot = 0;
  int display_order = 0;
  std::string label;
  lv_obj_t *screen = nullptr;
};

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
  media_volume_hide_modal();
  climate_control_hide_modal();
  option_select_hide_modal();
  switch_confirmation_hide_modal();
  alarm_pin_hide_modal();
  alarm_control_hide_modal();
  network_status_hide_modal();
}

inline void navigation_clear_subpages() {
  navigation_subpages().clear();
}

inline void navigation_register_subpage(int slot, int display_order,
                                        const std::string &label,
                                        lv_obj_t *screen) {
  if (slot <= 0 || screen == nullptr) return;
  NavigationSubpageEntry entry;
  entry.slot = slot;
  entry.display_order = display_order;
  entry.label = navigation_trim(label);
  entry.screen = screen;
  navigation_subpages().push_back(entry);
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

inline NavigationSubpageEntry *navigation_find_label_target(
    const std::string &target, bool *duplicate_found = nullptr) {
  if (duplicate_found) *duplicate_found = false;
  std::string wanted = navigation_lower(navigation_trim(target));
  if (wanted.empty()) return nullptr;

  NavigationSubpageEntry *best = nullptr;
  for (auto &entry : navigation_subpages()) {
    if (entry.screen == nullptr || entry.label.empty()) continue;
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

inline NavigationSubpageEntry *navigation_find_slot_target(int slot) {
  if (slot <= 0) return nullptr;
  for (auto &entry : navigation_subpages()) {
    if (entry.slot == slot && entry.screen != nullptr) return &entry;
  }
  return nullptr;
}

inline bool espcontrol_navigate(const std::string &target,
                                lv_obj_t *main_page_obj) {
  std::string normalized = navigation_lower(navigation_trim(target));
  if (normalized.empty()) {
    ESP_LOGW("navigation", "Navigation target is empty");
    return false;
  }

  navigation_hide_modals();

  if (normalized == "home" || normalized == "main") {
    if (main_page_obj == nullptr) {
      ESP_LOGW("navigation", "Main page is not ready");
      return false;
    }
    lv_scr_load_anim(main_page_obj, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    return true;
  }

  bool duplicate_found = false;
  NavigationSubpageEntry *label_target =
    navigation_find_label_target(target, &duplicate_found);
  if (label_target != nullptr) {
    if (duplicate_found) {
      ESP_LOGW("navigation",
        "Multiple subpages are labelled '%s'; opening slot %d",
        navigation_trim(target).c_str(), label_target->slot);
    }
    lv_scr_load_anim(label_target->screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    return true;
  }

  int slot = navigation_slot_from_target(target);
  if (slot > 0) {
    NavigationSubpageEntry *slot_target = navigation_find_slot_target(slot);
    if (slot_target != nullptr) {
      lv_scr_load_anim(slot_target->screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
      return true;
    }
    ESP_LOGW("navigation", "Slot %d is not a configured subpage", slot);
    return false;
  }

  ESP_LOGW("navigation", "No subpage labelled '%s'", navigation_trim(target).c_str());
  return false;
}
