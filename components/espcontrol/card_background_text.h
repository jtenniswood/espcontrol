#pragma once

// Called from LVGL's draw-task event, before dispatch. Render the shadow and
// foreground from the same live label descriptor, so text, fonts, wrapping,
// scrolling, visibility and geometry cannot drift between two label objects.
using CardBackgroundTextQuery = bool (*)(lv_obj_t *);
inline CardBackgroundTextQuery &card_background_text_query() {
  static CardBackgroundTextQuery query = nullptr;
  return query;
}

inline lv_draw_task_t *card_background_prepare_text_task(lv_draw_task_t *task) {
  if (!task || lv_draw_task_get_type(task) != LV_DRAW_TASK_TYPE_LABEL) return task;
  auto *descriptor = static_cast<lv_draw_label_dsc_t *>(lv_draw_task_get_draw_dsc(task));
  if (!card_background_text_query() || !card_background_text_query()(descriptor->base.obj)) return task;

  auto foreground = *descriptor;
  foreground.color = lv_color_hex(0xFFFFFF);
  foreground.opa = LV_OPA_COVER;
  foreground.text_local = 1;
  auto *layer = descriptor->base.layer;
  const auto clip = layer->_clip_area;
  const auto area = task->area;
  auto *tail = task;
  while (tail->next) tail = tail->next;
  layer->_clip_area = task->clip_area;
  // LVGL suppresses recursive draw-task events for tasks added in this event.
  // The existing task draws the shadow first; the new task draws the foreground.
  lv_draw_label(layer, &foreground, &area);
  layer->_clip_area = clip;
  if (!tail->next) return task;
  descriptor->color = lv_color_hex(0x000000);
  descriptor->opa = LV_OPA_80;
  ++task->area.x1; ++task->area.x2;
  ++task->area.y1; ++task->area.y2;
  task->_real_area = task->area;
  return tail->next;
}

inline void card_background_text_event_cb(lv_event_t *event) {
  card_background_prepare_text_task(static_cast<lv_draw_task_t *>(lv_event_get_param(event)));
}

inline void card_background_bind_text_label(lv_obj_t *object, bool slider = false) {
  lv_obj_remove_event_cb(object, card_background_text_event_cb);
  if (!slider && lv_obj_check_type(object, &lv_label_class)) {
    lv_obj_add_flag(object, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(object, card_background_text_event_cb, LV_EVENT_DRAW_TASK_ADDED, nullptr);
  }
}
