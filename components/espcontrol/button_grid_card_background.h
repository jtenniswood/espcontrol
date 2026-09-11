#pragma once
#include "esphome/components/lvgl/lvgl_esphome.h"

struct BtnSlot;
struct ParsedCfg;
struct GridConfig;

// Stable grid/YAML adapters; private rendering state and LVGL behavior live in
// card_background_rendering.cpp. Shared limits still apply to every unit.
namespace espcontrol::card_background {
void apply_image(BtnSlot &slot, const ParsedCfg &config, const GridConfig &grid);
void sync_image(BtnSlot &slot, const ParsedCfg &config, const GridConfig &grid);
void clear_image(BtnSlot &slot);
void activate_page(const GridConfig &grid, lv_obj_t *page);
bool configured_for_card(const ParsedCfg &config);
void move_content_foreground(const BtnSlot &slot);
void refresh_due();
void unregister_page(lv_obj_t *page);
void reset_image_pool(const GridConfig &grid);
}

inline void apply_card_background_image(BtnSlot &slot, const ParsedCfg &config, const GridConfig &grid) {
  return espcontrol::card_background::apply_image(slot, config, grid);
}
inline void sync_card_background_image(BtnSlot &slot, const ParsedCfg &config, const GridConfig &grid) {
  return espcontrol::card_background::sync_image(slot, config, grid);
}
inline void clear_card_background_image(BtnSlot &slot) {
  return espcontrol::card_background::clear_image(slot);
}
inline void card_background_activate_page(const GridConfig &grid, lv_obj_t *page) {
  return espcontrol::card_background::activate_page(grid, page);
}
inline bool card_background_configured_for_card(const ParsedCfg &config) {
  return espcontrol::card_background::configured_for_card(config);
}
inline void card_background_move_content_foreground(const BtnSlot &slot) {
  return espcontrol::card_background::move_content_foreground(slot);
}
inline void card_background_refresh_due() {
  return espcontrol::card_background::refresh_due();
}
inline void card_background_unregister_page(lv_obj_t *page) {
  return espcontrol::card_background::unregister_page(page);
}
inline void reset_card_background_image_pool(const GridConfig &grid) {
  return espcontrol::card_background::reset_image_pool(grid);
}
