#pragma once

#include <utility>

// Include after page_native_scene.h and the existing display/profile helpers.
// Fonts describe the source page. They are captured before the shared bank is
// rebound, and never read back from a card that may now represent another page.
namespace espcontrol::pages {
struct PageNativeSourceFonts {
  const lv_font_t *label{nullptr}, *value{nullptr}, *unit{nullptr};
};
struct PageNativeDeviceStyleProfile {
  DisplayProfile display;
  PageNativeSourceFonts root, child;
  lv_obj_t *grid{nullptr};
  int slots{0}, columns{1}, radius{8}, card_pad{8};
  bool continuous_corners{false};
  std::string primary_color;
};

class PageNativeDeviceStyles final : public PageNativeSceneStyles {
public:
  explicit PageNativeDeviceStyles(PageNativeDeviceStyleProfile profile)
      : profile_(std::move(profile)) {}
  const char *icon(std::string_view name,
                   std::string_view domain) const override {
    if (name.empty() || name == "Auto")
      return domain_default_icon(std::string(domain));
    return find_icon(std::string(name).c_str());
  }
  // Called at a safe scene reconfiguration boundary, before preparing its view.
  void profile(PageNativeDeviceStyleProfile profile) {
    profile_ = std::move(profile);
  }
  const PageNativeDeviceStyleProfile &profile() const { return profile_; }

  bool resolve(
      const PageStore &store, PageId page, PageModalToken token,
      const ParsedCfg &config,
      espcontrol::cards::ClimateControlDriverEnvironment &e) const override {
    if (!source(store, page, token))
      return false;
    const auto p = palette(config);
    const auto &d = profile_.display;
    const auto f = fonts(page);
    e.accent_color = p.on_val;
    e.secondary_color = p.off_val;
    e.tertiary_color = p.sensor_val;
    e.number_font = display_volume_number_font(d);
    e.range_number_font = display_media_control_title_font(d);
    e.unit_font = display_volume_label_font(d, f.unit);
    e.label_font = display_volume_label_font(d, f.label);
    e.option_title_font = display_climate_option_title_font(d, f.label);
    e.option_value_font = display_climate_option_value_font(d, f.label);
    e.option_menu_font = e.option_title_font;
    e.card_icon_font = display_climate_card_icon_font(d);
    e.icon_font = display_icon_font(d);
    e.width_compensation_percent = display_volume_width_percent(d);
    return true;
  }
  bool resolve(const PageStore &store, PageId page, PageModalToken token,
               const ParsedCfg &config,
               PageControlModalEnvironment &e) const override {
    if (!source(store, page, token))
      return false;
    const auto p = palette(config);
    const auto &d = profile_.display;
    const auto f = fonts(page);
    e.accent_color = p.on_val;
    e.secondary_color = p.off_val;
    e.number_font = display_volume_number_font(d);
    e.label_font = display_volume_label_font(d, f.label);
    e.option_title_font = display_climate_option_title_font(d, f.label);
    e.option_value_font = display_climate_option_value_font(d, f.label);
    e.option_menu_font = e.option_title_font;
    e.card_icon_font = display_climate_card_icon_font(d);
    e.icon_font = display_icon_font(d);
    e.width_compensation_percent = display_volume_width_percent(d);
    return true;
  }
  bool resolve(const PageStore &store, PageId page, PageModalToken token,
               const ParsedCfg &config,
               PageFanControlModalEnvironment &e) const override {
    if (!source(store, page, token))
      return false;
    colors(config, e);
    e.label_font =
        display_volume_label_font(profile_.display, fonts(page).label);
    e.icon_font = display_icon_font(profile_.display);
    e.width_compensation_percent =
        display_volume_width_percent(profile_.display);
    return true;
  }
  bool resolve(const PageStore &store, PageId page, PageModalToken token,
               const ParsedCfg &config,
               PageOptionSelectModalEnvironment &e) const override {
    if (!source(store, page, token))
      return false;
    colors(config, e);
    const auto &d = profile_.display;
    const auto f = fonts(page);
    e.label_font = f.label;
    e.value_font = display_option_select_value_font_or(d, f.label);
    e.icon_font = display_icon_font(d);
    e.width_compensation_percent = display_main_width_percent(d);
    return true;
  }
  bool resolve(const PageStore &store, PageId page, PageModalToken token,
               const ParsedCfg &config,
               PageMediaVolumeModalEnvironment &e) const override {
    if (!source(store, page, token))
      return false;
    colors(config, e);
    const auto &d = profile_.display;
    const auto f = fonts(page);
    e.value_font = f.value;
    e.number_font = display_volume_number_font(d);
    e.unit_font = display_volume_label_font(d, f.unit);
    e.label_font = display_volume_label_font(d, f.label);
    e.icon_font = display_icon_font(d);
    e.width_compensation_percent = display_volume_width_percent(d);
    return true;
  }
  bool resolve(const PageStore &store, PageId page, PageModalToken token,
               const ParsedCfg &config,
               PageMediaControlModalEnvironment &e) const override {
    if (!source(store, page, token))
      return false;
    colors(config, e);
    const auto &d = profile_.display;
    e.title_font = display_media_control_title_font(d);
    e.label_font = display_media_control_artist_font(d, fonts(page).label);
    e.number_font = display_volume_number_font(d);
    e.icon_font = display_icon_font(d);
    e.width_compensation_percent = display_volume_width_percent(d);
    // The scene supplies its shared media model; style updates preserve it.
    return true;
  }
  bool resolve(const PageStore &store, PageId page, PageModalToken token,
               const ParsedCfg &, PageWeatherModalStyle &e) const override {
    if (!source(store, page, token))
      return false;
    e.font = fonts(page).label;
    e.icon_font = display_icon_font(profile_.display);
    e.temperature_font = display_small_text_font(profile_.display, e.font);
    e.radius = profile_.radius;
    return true;
  }
  bool resolve(const PageStore &store, PageId page, PageModalToken token,
               const ParsedCfg &config,
               PageConfirmationModalEnvironment &e) const override {
    if (!source(store, page, token))
      return false;
    e.button_font = fonts(page).label;
    e.message_font = display_switch_confirmation_message_font(profile_.display);
    if (!e.message_font)
      e.message_font = e.button_font;
    e.icon_font = display_icon_font(profile_.display);
    e.accent = palette(config).on_val;
    // Borrow immutable active configuration, never the temporary ParsedCfg.
    const auto *card = store.card({token.configuration, token.card});
    e.caption = store.field(*card, CardField::LABEL);
    if (e.caption.empty())
      e.caption = store.field(*card, CardField::ENTITY);
    return true;
  }
  bool resolve(const PageStore &store, PageId page, PageModalToken token,
               const ParsedCfg &config,
               PageAlarmModalEnvironment &e) const override {
    if (!source(store, page, token))
      return false;
    const auto p = palette(config);
    const auto &d = profile_.display;
    e.on_color = p.on_val;
    e.off_color = p.off_val;
    e.tertiary_color = p.sensor_val;
    e.red_percent = d.color.red_percent;
    e.green_percent = d.color.green_percent;
    e.blue_percent = d.color.blue_percent;
    e.width_compensation_percent = display_main_width_percent(d);
    e.grid_columns = profile_.columns;
    e.grid_page = profile_.grid;
    e.label_font = fonts(page).label;
    e.pin_font = display_sensor_font(d);
    e.key_font = display_optional_media_title_font(d);
    e.icon_font = display_icon_font(d);
    e.title_font = display_media_title_font_or(d, e.label_font);
    return true;
  }
  bool resolve(const PageStore &store, PageId page, PageModalToken token,
               const ParsedCfg &,
               PageExpandedImageModalEnvironment &e) const override {
    if (!source(store, page, token))
      return false;
    e.label_font = fonts(page).label;
    e.icon_font = display_icon_font(profile_.display);
    e.width_compensation_percent = display_main_width_percent(profile_.display);
    e.background =
        display_correct_color(current_theme().background, profile_.display);
    return true;
  }
  bool resolve(const PageStore &store, PageId page, PageModalToken token,
               const ParsedCfg &,
               PageWifiQrModalEnvironment &e) const override {
    if (!source(store, page, token))
      return false;
    e.label_font = fonts(page).label;
    e.heading_font =
        display_media_title_font_or(profile_.display, e.label_font);
    e.icon_font = display_icon_font(profile_.display);
    e.width_compensation_percent = display_main_width_percent(profile_.display);
    return true;
  }
  bool resolve(const PageStore &store, PageLocalModalOrigin origin,
               PageModalToken token,
               PageNetworkStatusEnvironment &e) const override {
    if (origin.configuration != store.generation() ||
        token.configuration != store.generation() || !store.page(origin.page) ||
        !profile_.grid || profile_.slots < 1 || profile_.columns < 1)
      return false;
    auto *page = profile_.grid;
    lv_obj_update_layout(page);
    e.label_font = fonts(origin.page).label;
    e.icon_font = display_icon_font(profile_.display);
    e.width = lv_obj_get_width(page);
    e.height = lv_obj_get_height(page);
    e.top = lv_obj_get_style_pad_top(page, LV_PART_MAIN);
    e.pad_bottom = lv_obj_get_style_pad_bottom(page, LV_PART_MAIN);
    e.pad_left = lv_obj_get_style_pad_left(page, LV_PART_MAIN);
    e.pad_right = lv_obj_get_style_pad_right(page, LV_PART_MAIN);
    e.gap_row = lv_obj_get_style_pad_row(page, LV_PART_MAIN);
    e.gap_column = lv_obj_get_style_pad_column(page, LV_PART_MAIN);
    e.columns = profile_.columns;
    e.rows = (profile_.slots + profile_.columns - 1) / profile_.columns;
    e.radius = profile_.radius;
    e.card_pad = profile_.card_pad;
    e.continuous_corners = profile_.continuous_corners;
    e.icon_compensation = display_main_width_percent(profile_.display);
    e.text_compensation = display_text_width_percent(profile_.display);
    return true;
  }

private:
  bool source(const PageStore &store, PageId page, PageModalToken token) const {
    const auto *definition = store.page(page);
    return definition && definition->navigable &&
           token.configuration == store.generation() &&
           token.card >= definition->first_card &&
           token.card - definition->first_card < definition->card_count &&
           store.card({token.configuration, token.card});
  }
  PageNativeSourceFonts fonts(PageId page) const {
    return page == 0 ? profile_.root : profile_.child;
  }
  CardPalette palette(const ParsedCfg &config) const {
    const auto &d = profile_.display;
    CardPalette p;
    p.has_off = p.has_sensor_color = true;
    bool valid = false;
    const auto primary = parse_hex_color(profile_.primary_color, valid);
    p.has_on = valid;
    p.on_val = valid ? display_correct_color(primary, d) : DEFAULT_ACCENT_COLOR;
    p.off_val = display_correct_color(current_theme().surface_card, d);
    p.sensor_val = display_correct_color(current_theme().surface_secondary, d);
    p.surface_sensor_val =
        display_correct_color(current_theme().surface_sensor, d);
    return card_palette_for_config(p, config, d);
  }
  template <typename Environment>
  void colors(const ParsedCfg &config, Environment &e) const {
    const auto p = palette(config);
    e.accent_color = p.on_val;
    e.secondary_color = p.off_val;
    e.tertiary_color = p.sensor_val;
  }
  PageNativeDeviceStyleProfile profile_;
};
} // namespace espcontrol::pages
