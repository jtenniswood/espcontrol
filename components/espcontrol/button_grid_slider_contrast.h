#pragma once

// LVGL's draw-task clip is needed to split a glyph at the fill boundary.
// Reuse the native label descriptor so wrapping, scrolling and fonts stay intact.
#include "src/draw/lv_draw_private.h"
#include "src/misc/lv_area_private.h"

inline void slider_text_contrast_event_cb(lv_event_t *event) {
  auto *fill = static_cast<lv_obj_t *>(lv_event_get_user_data(event));
  auto *task = static_cast<lv_draw_task_t *>(lv_event_get_param(event));
  if (!fill || !task || lv_draw_task_get_type(task) != LV_DRAW_TASK_TYPE_LABEL ||
      lv_obj_get_width(fill) <= 0 || lv_obj_get_height(fill) <= 0) return;
  auto *descriptor = static_cast<lv_draw_label_dsc_t *>(lv_draw_task_get_draw_dsc(task));
  lv_obj_t *button = lv_obj_get_parent(fill);
  const lv_color_t base_color = lv_obj_get_style_text_color(button, LV_PART_MAIN);
  const lv_color_t fill_color = lv_color_hex(display_text_color_for_bg(
    lv_color_to_int(lv_obj_get_style_bg_color(fill, LV_PART_MAIN))));
  descriptor->color = base_color;
  if (lv_color_eq(base_color, fill_color)) return;
  lv_area_t fill_area, covered;
  lv_obj_get_coords(fill, &fill_area);
  if (!lv_area_intersect(&covered, &task->clip_area, &fill_area)) return;

  const lv_area_t clip = task->clip_area;
  const lv_area_t outside[] = {
    {clip.x1, clip.y1, clip.x2, covered.y1 - 1},
    {clip.x1, covered.y2 + 1, clip.x2, clip.y2},
    {clip.x1, covered.y1, covered.x1 - 1, covered.y2},
    {covered.x2 + 1, covered.y1, clip.x2, covered.y2},
  };
  lv_layer_t *layer = descriptor->base.layer;
  const lv_area_t saved_clip = layer->_clip_area;
  lv_draw_label_dsc_t uncovered_descriptor = *descriptor;
  uncovered_descriptor.text_local = 1;
  for (const auto &area : outside) {
    if (area.x1 > area.x2 || area.y1 > area.y2) continue;
    layer->_clip_area = area;
    lv_draw_label(layer, &uncovered_descriptor, &task->area);
  }
  layer->_clip_area = saved_clip;
  task->clip_area = covered;
  descriptor->color = fill_color;
}

inline void slider_bind_text_contrast(lv_obj_t *object, lv_obj_t *fill) {
  if (!object) return;
  lv_obj_remove_event_cb(object, slider_text_contrast_event_cb);
  if (fill) {
    lv_obj_add_flag(object, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(object, slider_text_contrast_event_cb, LV_EVENT_DRAW_TASK_ADDED, fill);
  }
  for (uint32_t i = 0; i < lv_obj_get_child_cnt(object); ++i) {
    slider_bind_text_contrast(lv_obj_get_child(object, i), fill);
  }
}
