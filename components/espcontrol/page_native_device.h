#pragma once

// The device startup owner is deliberately outside the ESPHome component's
// constructor: PSRAM, restored settings and LVGL must be available first.
#include "button_grid.h"
#include "page_local_network_status_source.h"
#include "page_native_device_platform.h"
#include "page_native_device_styles.h"
#include "page_scene_selection.h"

#ifdef USE_WIFI
#include "esphome/components/wifi/wifi_component.h"
#endif

namespace espcontrol::pages {
template <size_t Slots> class PageNativeWidgetBank {
public:
  explicit PageNativeWidgetBank(const std::array<BtnSlot, Slots> &source) {
    for (size_t i = 0; i < Slots; ++i) {
      const auto &s = source[i];
      widgets[i] = {s.btn,
                    s.icon_lbl,
                    s.text_lbl,
                    s.sensor_container,
                    s.sensor_lbl,
                    s.unit_lbl,
                    lv_obj_create(s.btn),
                    lv_obj_create(s.btn),
                    lv_slider_create(s.btn)};
    }
  }
  ~PageNativeWidgetBank() {
    // Declared before the scene so its callbacks/leases are gone by this point.
    for (const auto &w : widgets) {
      lv_obj_delete(w.slider);
      lv_obj_delete(w.forecast);
      lv_obj_delete(w.fill);
    }
  }
  PageNativeWidgetBank(const PageNativeWidgetBank &) = delete;
  std::array<PageBasicWidgets, Slots> widgets{};
};

inline PageNativeDeviceStyleProfile page_device_style_profile(
    const GridConfig &config, lv_obj_t *grid, PageNativeSourceFonts root,
    PageNativeSourceFonts child, int card_pad, std::string primary) {
  PageNativeDeviceStyleProfile p;
  p.display = display_profile_from_grid_config(config);
  p.root = root;
  p.child = child;
  p.grid = grid;
  p.slots = config.num_slots;
  p.columns = config.cols;
  p.radius = config.card_corner_radius;
  p.card_pad = card_pad;
  p.continuous_corners = config.continuous_card_corners;
  p.primary_color = std::move(primary);
  return p;
}

inline PageBasicTheme
page_device_theme(const PageNativeDeviceStyleProfile &profile,
                  uint64_t revision) {
  const auto &d = profile.display;
  const auto &palette = current_theme();
  const auto correct = [&d](uint32_t rgb) {
    return display_correct_color(rgb, d);
  };
  PageBasicTheme t;
  t.revision = revision;
  t.surface = correct(palette.surface_card);
  t.sensor_surface = correct(palette.surface_sensor);
  t.media_surface = correct(palette.surface_secondary);
  t.alarm_surface = correct(palette.surface_card);
  t.image_loading_surface = correct(palette.surface_secondary);
  t.text = correct(palette.text_primary);
  t.disabled_text = correct(palette.text_disabled);
  bool valid = false;
  const auto primary = parse_hex_color(profile.primary_color, valid);
  t.active = valid ? correct(primary) : DEFAULT_ACCENT_COLOR;
  t.red_percent = d.color.red_percent;
  t.green_percent = d.color.green_percent;
  t.blue_percent = d.color.blue_percent;
  t.sensor_font = display_sensor_font(d);
  t.large_sensor_font = display_large_sensor_font(d);
  t.large_sensor_unit_offset_percent =
      display_large_sensor_unit_offset_percent(d);
  t.back_label = espcontrol_i18n("Back");
  t.empty_label = espcontrol_i18n("Configure");
  t.folder_label = espcontrol_i18n("Subpage");
  t.temperature_unit = display_temperature_unit_symbol();
  t.translate = [](const char *key) { return espcontrol_i18n(key); };
  t.forecast_small_font = display_small_text_font(d, profile.root.label);
  t.forecast_icon_font = display_icon_font(d);
  t.forecast_temperature = [](float value, const char *unit) {
    return convert_temperature_value_for_display_float(value, unit ? unit : "");
  };
  t.forecast_scale_x =
      d.width.vertical_axis ? 256 : 256 * display_main_width_percent(d) / 100;
  t.forecast_scale_y =
      d.width.vertical_axis ? 256 * display_main_width_percent(d) / 100 : 256;
  t.media_title_font = display_media_title_font(d);
  t.icon_font = display_icon_font(d);
  t.label_font = profile.root.label;
  t.media_text_scale_x =
      d.width.vertical_axis ? 256 : 256 * display_text_width_percent(d) / 100;
  t.media_text_scale_y =
      d.width.vertical_axis ? 256 * display_text_width_percent(d) / 100 : 256;
  t.device_width_profile = true;
  t.icon_scale_x = t.forecast_scale_x;
  t.icon_scale_y = t.forecast_scale_y;
  t.label_scale_x = t.media_text_scale_x;
  t.label_scale_y = t.media_text_scale_y;
  t.child_label_font = profile.child.label;
  t.child_unit_font = profile.child.unit;
  t.cover_title_font = display_media_cover_art_title_font(d);
  t.cover_artist_font =
      display_media_cover_art_artist_font(d, profile.root.label);
  t.media_control_title_font = display_media_control_title_font(d);
  t.media_control_artist_font =
      display_media_control_artist_font(d, profile.root.label);
  t.option_select_font =
      display_option_select_value_font_or(d, profile.root.label);
  t.compact_portrait = display_modal_uses_family(
      d.modal, DisplayModalLayoutFamily::COMPACT_PORTRAIT);
  return t;
}

template <size_t Slots, size_t ImageSlots> class PageNativeDevice {
public:
  using Scene = PageNativeScene<Slots, ImageSlots>;
  // The application keeps the returned owner for its whole lifetime. Failed
  // startup unwinds native listeners/feeds before deleting reserved children.
  static PageNativeDevice *
  create(EspControlApp &app, const DeviceUiLimits &limits,
         const std::array<BtnSlot, Slots> &source, const GridConfig &config,
         lv_obj_t *grid, std::string primary, PageNativeSceneBudget budget,
         PageDeviceDisplayHooks hooks, std::string name, std::string version,
         void (*lock)(), float (*wifi)() = nullptr,
         bool legacy_renderer_retired = false) {
    if (!legacy_renderer_retired || !app.require_application_scene())
      return nullptr;
    if (!page_native_command_ready() || !grid || !lock ||
        limits.slot_count != Slots || limits.image_slot_count != ImageSlots ||
        !valid_source(source))
      return nullptr;
    PageDeviceArena allocator;
    void *memory = allocator.allocate(sizeof(PageNativeDevice));
    if (!memory)
      return nullptr;
    const auto display = display_profile_from_grid_config(config);
    display_activate_profile(display);
    set_display_temperature_unit(config.temperature_unit, config.timezone);
    configure_grid_layout(grid, config.num_slots, config.cols);
    for (const auto &slot : source) {
      apply_grid_card_corners(slot.btn, config.card_corner_radius,
                              config.continuous_card_corners);
      display_apply_main_width(slot.icon_lbl, display);
      display_apply_slot_text_width(slot, display);
    }
    auto *result = new (memory) PageNativeDevice(
        app, limits, source, config, grid, std::move(primary), budget,
        std::move(hooks), std::move(name), std::move(version), lock, wifi);
    if (!result->platform_.ready() ||
        !result->scene_.initialize(
            config.image_card_images, config.image_card_image_count,
            config.image_card_modal_image, legacy_renderer_retired)) {
      destroy(result);
      return nullptr;
    }
    return result;
  }
  static void destroy(PageNativeDevice *device) {
    if (!device)
      return;
    assert(page_native_command_ready());
    device->~PageNativeDevice();
    PageDeviceArena allocator;
    allocator.release(device, sizeof(PageNativeDevice));
  }
  PageNativeDevice(const PageNativeDevice &) = delete;
  Scene &scene() { return scene_; }
  bool install(EspControlApp &app) {
    auto *&navigation = application_scene_navigation();
    if (navigation && navigation != &scene_)
      return false;
    if (!app.install_application_scene(scene_))
      return false;
    navigation = &scene_;
    set_backlight_display_takeover_callback(
        navigation_close_modals_for_display_takeover);
    const auto &profile = styles_.profile();
    set_control_modal_grid_metrics(profile.grid, profile.columns,
                                   (profile.slots + profile.columns - 1) /
                                       profile.columns,
                                   bank_.widgets[0].button);
    return true;
  }
  bool refresh(const GridConfig &config, std::string primary) {
    if (!scene_.maintenance_ready() || theme_revision_ == UINT64_MAX)
      return false;
    const auto previous = styles_.profile();
    auto next = page_device_style_profile(config, previous.grid, previous.root,
                                          previous.child, previous.card_pad,
                                          std::move(primary));
    auto theme = initial_theme(next, config);
    theme.revision = theme_revision_ + 1;
    const auto unit =
        normalize_temperature_unit_option(config.temperature_unit);
    const bool fahrenheit =
        unit == "°F" ||
        (unit == "Auto" && timezone_prefers_fahrenheit(
                               effective_timezone_option(config.timezone)));
    theme.temperature_unit = fahrenheit ? "°F" : "°C";
    styles_.profile(next);
    if (!scene_.reconfigure({static_cast<uint16_t>(config.num_slots),
                             static_cast<uint16_t>(config.cols)},
                            theme)) {
      styles_.profile(previous);
      return false;
    }
    ++theme_revision_;
    platform_.profile(config.modal_profile);
    display_activate_profile(next.display);
    set_display_temperature_unit(config.temperature_unit, config.timezone);
    configure_grid_layout(previous.grid, config.num_slots, config.cols);
    set_control_modal_grid_metrics(previous.grid, config.cols,
                                   (config.num_slots + config.cols - 1) /
                                       config.cols,
                                   bank_.widgets[0].button);
    for (const auto &w : bank_.widgets)
      apply_grid_card_corners(w.button, config.card_corner_radius,
                              config.continuous_card_corners);
    return true;
  }

private:
  static bool valid_source(const std::array<BtnSlot, Slots> &source) {
    for (const auto &s : source)
      if (!s.btn || !s.icon_lbl || !s.text_lbl || !s.sensor_container ||
          !s.sensor_lbl || !s.unit_lbl)
        return false;
    return true;
  }
  static PageNativeSourceFonts root_fonts(const BtnSlot &source) {
    return {lv_obj_get_style_text_font(source.text_lbl, LV_PART_MAIN),
            lv_obj_get_style_text_font(source.sensor_lbl, LV_PART_MAIN),
            lv_obj_get_style_text_font(source.unit_lbl, LV_PART_MAIN)};
  }
  static PageNativeSourceFonts child_fonts(const BtnSlot &source,
                                           const GridConfig &config) {
    const auto *label = lv_obj_get_style_text_font(source.btn, LV_PART_MAIN);
    return {label, config.sp_sensor_font, label};
  }
  PageNativeDevice(EspControlApp &app, const DeviceUiLimits &limits,
                   const std::array<BtnSlot, Slots> &source,
                   const GridConfig &config, lv_obj_t *grid,
                   std::string primary, PageNativeSceneBudget budget,
                   PageDeviceDisplayHooks hooks, std::string name,
                   std::string version, void (*lock)(), float (*wifi)())
      : bank_(source),
        styles_(page_device_style_profile(
            config, grid, root_fonts(source[0]), child_fonts(source[0], config),
            lv_obj_get_style_pad_top(source[0].btn, LV_PART_MAIN),
            std::move(primary))),
        platform_(app, grid, config.modal_profile, std::move(hooks)),
        network_(std::move(name), std::move(version), *lock, wifi),
        scene_(
            limits, allocator_, budget, bank_.widgets,
            {static_cast<uint16_t>(config.num_slots),
             static_cast<uint16_t>(config.cols)},
            initial_theme(styles_.profile(), config), page_timezone_resolver(),
            {+[](const char *text) { return espcontrol_i18n(text); },
             +[](const char *key) { return espcontrol_i18n_key(key); }},
            +[](const char *text) { return espcontrol_i18n(text); }, styles_,
            platform_, network_) {}
  static PageBasicTheme
  initial_theme(const PageNativeDeviceStyleProfile &profile,
                const GridConfig &config) {
    auto theme = page_device_theme(profile, 1);
    theme.media_artwork_width_percent =
        config.media_artwork_width_compensation_percent;
    theme.label_lines = config.label_lines;
    theme.label_lines_tall = config.label_lines_tall;
    return theme;
  }

  PageDeviceArena allocator_;
  PageNativeWidgetBank<Slots> bank_;
  PageNativeDeviceStyles styles_;
  PageNativeDevicePlatform platform_;
  PageLocalNetworkStatusSource network_;
  Scene scene_;
  uint64_t theme_revision_{1};
};

#ifdef ESPCONTROL_DEVICE_UI_SLOT_COUNT
using PageSelectedNativeDevice =
    PageNativeDevice<ESPCONTROL_DEVICE_UI_SLOT_COUNT,
                     DEVICE_UI_LIMITS.image_slot_count>;
inline PageSelectedNativeDevice *&page_native_device() {
  static PageSelectedNativeDevice *device = nullptr;
  return device;
}
inline bool page_native_device_start(
    EspControlApp &app,
    const BtnSlot (&source)[ESPCONTROL_DEVICE_UI_SLOT_COUNT],
    const GridConfig &config, lv_obj_t *grid, std::string primary,
    PageDeviceDisplayHooks hooks, std::string name, std::string version) {
  auto *&device = page_native_device();
  if (device)
    return device->scene().ready();
  if (!app.require_application_scene())
    return false;
  select_pooled_application();
  std::array<BtnSlot, ESPCONTROL_DEVICE_UI_SLOT_COUNT> slots;
  std::copy_n(source, slots.size(), slots.begin());
  const auto cards = DEVICE_UI_LIMITS.max_cards();
  const bool constrained = display_modal_is_constrained(config.modal_profile);
  const PageNativeSceneBudget budget{
      {cards,
       cards * 512,
       {cards * 40, cards * 40, constrained ? 128u * 1024 : 512u * 1024,
        32u * 1024},
       {cards, cards, 16u * 1024, HA_TEXT_SENSOR_STATE_MAX_LEN}},
      constrained ? 128u * 1024 : 256u * 1024};
  float (*wifi)() = nullptr;
#ifdef USE_WIFI
  wifi = +[]() -> float {
    auto *connection = esphome::wifi::global_wifi_component;
    if (!connection || !connection->is_connected())
      return NAN;
    return std::min(100.0f,
                    std::max(0.0f, 2.0f * (connection->wifi_rssi() + 100.0f)));
  };
#endif
  device = PageSelectedNativeDevice::create(
      app, DEVICE_UI_LIMITS, slots, config, grid, std::move(primary), budget,
      std::move(hooks), std::move(name), std::move(version),
      +[]() { screen_lock_activate(true); }, wifi, true);
  if (!device)
    return false;
  if (device->install(app))
    return true;
  PageSelectedNativeDevice::destroy(device);
  device = nullptr;
  return false;
}
#endif
} // namespace espcontrol::pages
