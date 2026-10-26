#pragma once

#include <string_view>

namespace espcontrol {
// One application-lifetime destination for the existing navigation/settings
// entry points. Queries borrow immutable model data only for the current call.
class ApplicationSceneNavigation {
public:
  virtual ~ApplicationSceneNavigation() = default;
  virtual bool navigation_ready() const = 0;
  virtual bool grid_configured() const = 0;
  virtual int subpage_slot() const = 0;
  virtual std::string_view subpage_label() const = 0;
  virtual bool critical_active() const = 0;
  virtual bool can_open_modal(std::string_view) const = 0;
  virtual bool open_modal(std::string_view) = 0;
  virtual bool can_open_subpage(std::string_view) const = 0;
  virtual bool open_subpage(std::string_view) = 0;
  virtual bool close_subpage() = 0;
  virtual bool navigate(std::string_view) = 0;
  virtual bool home() = 0;
  virtual void close_modal() = 0;
  virtual bool open_settings() = 0;
  virtual void close_settings() = 0;
  virtual void display_takeover() = 0;
  virtual void refresh_chrome() = 0;
};
inline ApplicationSceneNavigation *&application_scene_navigation() {
  static ApplicationSceneNavigation *navigation = nullptr;
  return navigation;
}
// The selected renderer is immutable for one boot, including startup failure.
// Device startup declares the selection before reserving any native resources.
inline bool &pooled_application_selection() {
  static bool selected = false;
  return selected;
}
inline bool pooled_application_selected() {
  return pooled_application_selection();
}
inline void select_pooled_application() {
  pooled_application_selection() = true;
}
inline bool application_grid_configured(bool legacy_configured) {
  if (!pooled_application_selected())
    return legacy_configured;
  auto *scene = application_scene_navigation();
  return scene && scene->grid_configured();
}
} // namespace espcontrol
