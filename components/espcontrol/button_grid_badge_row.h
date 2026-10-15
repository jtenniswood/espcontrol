#pragma once
#include "badge_row_config.h"

inline void badge_row_remove(lv_obj_t *page) {
  for (const auto &entry : page_badge_rows()) {
    if (entry.page != page) continue;
    const lv_coord_t top = lv_obj_get_style_pad_top(page, LV_PART_MAIN) - entry.inset;
    lv_obj_t *row = entry.row;
    lv_obj_set_style_pad_top(page, top, LV_PART_MAIN);
    lv_obj_del(row);
    return;
  }
}

inline void badge_row_build(lv_obj_t *page, int page_index,
                            const GridConfig &cfg, const CardPalette &palette,
                            const BtnSlot &reference,
                            const std::function<void(const std::string &)> &add_parent_indicator = {}) {
  badge_row_remove(page);
  if (cfg.info_only) return;
  auto badges = parse_subpage_config(espcontrol::badge::configuration().page(page_index));
  badges.erase(std::remove_if(badges.begin(), badges.end(), [](const SubpageBtn &b) {
    return b.type != "badge";
  }), badges.end());
  if (badges.empty()) return;
  if (badges.size() > espcontrol::badge::MAX_ROW_BADGES) badges.resize(espcontrol::badge::MAX_ROW_BADGES);
  const DisplayProfile display = display_profile_from_grid_config(cfg);
  const lv_font_t *font = lv_obj_get_style_text_font(reference.text_lbl, LV_PART_MAIN);
  const lv_coord_t line = font ? font->line_height : 24;
  const lv_coord_t height = espcontrol::badge::pill_height(line);
  const lv_coord_t top_gap = line / 2;
  const lv_coord_t bottom_gap = line - top_gap;
  const lv_coord_t inset = top_gap + height + bottom_gap;
  const lv_coord_t top = lv_obj_get_style_pad_top(page, LV_PART_MAIN);
  lv_obj_t *row = lv_obj_create(page);
  lv_obj_remove_style_all(row);
  lv_obj_add_flag(row, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_set_scroll_dir(row, LV_DIR_HOR);
  lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_size(row, LV_PCT(100), height);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  // Centre a content-sized group when it fits. Let it grow beyond the viewport
  // for scrolling, rather than centring overflowing badges off the left edge.
  lv_obj_t *group = lv_obj_create(row);
  lv_obj_remove_style_all(group);
  lv_obj_clear_flag(group, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(group, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(group, LV_SIZE_CONTENT, height);
  lv_obj_set_style_min_width(group, LV_PCT(100), LV_PART_MAIN);
  lv_obj_set_flex_flow(group, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  const lv_coord_t margin = line / 2;
  lv_obj_set_style_pad_left(group, margin, LV_PART_MAIN);
  lv_obj_set_style_pad_right(group, margin, LV_PART_MAIN);
  lv_obj_set_style_pad_column(group, margin, LV_PART_MAIN);
  page_badge_rows().push_back({page, row, inset, {}});
  lv_obj_add_event_cb(row, [](lv_event_t *event) {
    auto &rows = page_badge_rows();
    const auto target = static_cast<lv_obj_t *>(lv_event_get_target(event));
    screen_lock_unregister_tree(target);
    rows.erase(std::remove_if(rows.begin(), rows.end(), [target](const PageBadgeRow &item) {
      return item.row == target;
    }), rows.end());
  }, LV_EVENT_DELETE, nullptr);
  lv_obj_set_style_pad_top(page, top + inset, LV_PART_MAIN);
  // Position is relative to the grid origin. Share the existing spacing
  // above and below the row without moving or shrinking the cards.
  lv_obj_set_pos(row, 0, top_gap - inset);
  lv_obj_update_layout(page);
  for (const auto &badge : badges) {
    ParsedCfg config = parsed_cfg_from_subpage_btn(badge);
    const auto context = card_runtime_context(config, espcontrol::cards::Surface::SUBPAGE);
    lv_obj_t *button = create_grid_card_button(group, cfg.card_corner_radius >= 0 ? cfg.card_corner_radius
      : lv_obj_get_style_radius(reference.btn, LV_PART_MAIN), 0, font,
      lv_obj_get_style_text_color(reference.btn, LV_PART_MAIN));
    lv_point_t text_size{};
    lv_text_get_size(&text_size, config.label.c_str(), font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    const bool slider = cfg_option_value(config.options, "badge_action") == "slider";
    const lv_coord_t width = espcontrol::badge::width_for_text(line, text_size.x, slider);
    lv_obj_set_size(button, width, height);
    apply_grid_card_corners(button, cfg.card_corner_radius, cfg.continuous_card_corners);
    const auto badge_palette = card_palette_for_config(palette, config, display);
    theme_set_content_background(button, badge_palette.custom_background);
    apply_button_colors(button, badge_palette.has_on, badge_palette.on_val, badge_palette.has_off, badge_palette.off_val);
    BtnSlot slot = create_dynamic_card_slot(button, display_icon_font(display), display_sensor_font(display), font,
      lv_obj_get_style_text_color(reference.btn, LV_PART_MAIN), cfg.subpage_chevron_font);
    espcontrol::cards::badge_driver_setup_visual(slot, config, context, display, badge_palette);
    auto *badge_context = espcontrol::cards::badge_context(button);
    if (badge_context && badge_context->card_mode) {
      espcontrol::cards::badge_card_runtime_queue(badge_context, config, cfg, palette, reference);
    }
    espcontrol::cards::badge_driver_fit_width(slot.btn);
    espcontrol::cards::badge_driver_bind_subpage(slot, config, context, add_parent_indicator);
    page_badge_rows().back().buttons.push_back(button);
  }
  lv_obj_update_layout(page);
  clock_bar_refresh_responsive_grid_cards(page);
}
