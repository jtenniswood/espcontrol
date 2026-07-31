#pragma once

// Shared lifecycle driver for Image cards. The downloader, cache, geometry,
// and modal implementation remain in button_grid_image.h; this driver owns
// the grid and subpage lifecycle boundary.
// Contract coverage marker: "image".

namespace espcontrol::cards {

inline bool image_driver_matches(const Context &context) {
  return !context.legacy_dispatch &&
         context.runtime.driver == card_runtime::CardDriverId::IMAGE;
}

inline bool image_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!image_driver_matches(context)) return false;
  setup_image_card(slot);
  return true;
}

inline bool image_driver_attach_interaction(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return image_driver_matches(context);
}

inline bool image_driver_refresh_layout(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!image_driver_matches(context)) return false;
  ImageCardCtx *image_context = slot.btn
    ? static_cast<ImageCardCtx *>(lv_obj_get_user_data(slot.btn))
    : nullptr;
  if (image_context && image_context->active) {
    image_card_refresh_tile_geometry(image_context);
  } else {
    lv_obj_t *widget = slot.sensor_container
      ? static_cast<lv_obj_t *>(lv_obj_get_user_data(slot.sensor_container))
      : nullptr;
    if (widget) {
      image_card_position_widget(slot.btn, widget);
      lv_obj_t *loading = image_card_loading_widget(widget);
      image_card_position_widget(slot.btn, loading);
      image_card_refresh_loading_layout(loading);
    }
  }
  if (slot.text_lbl && !lv_obj_has_flag(slot.text_lbl, LV_OBJ_FLAG_HIDDEN)) {
    image_card_align_label_stack(slot.text_lbl, slot.btn);
  }
  if (slot.icon_lbl && !lv_obj_has_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN)) {
    image_card_align_icon(slot.icon_lbl, slot.btn);
  }
  return true;
}

inline bool image_driver_cleanup(
    BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!image_driver_matches(context)) return false;
  if (slot.btn) lv_obj_set_user_data(slot.btn, nullptr);
  return true;
}

inline void image_driver_reset_pool(const GridConfig &grid) {
  reset_image_card_pool(grid);
}

inline bool image_driver_bind_main(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const GridConfig &grid) {
  if (!image_driver_matches(context) ||
      context.surface != Surface::MAIN_GRID) return false;
  return image_card_bind_runtime(slot, config, grid, false);
}

inline bool image_driver_bind_subpage(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const GridConfig &grid) {
  if (!image_driver_matches(context) ||
      context.surface != Surface::SUBPAGE) return false;
  return image_card_bind_runtime(slot, config, grid, true);
}

inline bool image_driver_handle_main_click(
    const Context &context, const ParsedCfg &, lv_obj_t *button) {
  if (!image_driver_matches(context)) return false;
  ImageCardCtx *image_context = button
    ? static_cast<ImageCardCtx *>(lv_obj_get_user_data(button)) : nullptr;
  if (image_context) image_card_open_modal(image_context);
  return true;
}

}  // namespace espcontrol::cards
