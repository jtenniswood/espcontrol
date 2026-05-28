#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// Home Assistant todo card controls.

constexpr uint32_t TODO_CARD_CTX_MAGIC = 0x544F444F;  // TODO
constexpr int TODO_MAX_ITEMS = 8;
constexpr size_t TODO_RESPONSE_TEXT_MAX_LEN = 1536;
constexpr uint32_t TODO_ITEMS_REQUEST_TIMEOUT_MS = 10000;
constexpr uint32_t TODO_COMPLETED_TEXT_COLOR = 0x707070;
constexpr uint32_t TODO_COMPLETED_CHECK_COLOR = 0xC0C0C0;

struct TodoItem {
  std::string key;
  std::string summary;
  bool more = false;
};

struct TodoCardCtx {
  uint32_t magic = TODO_CARD_CTX_MAGIC;
  std::string entity_id;
  std::string configured_label;
  std::string friendly_name;
  std::string count_text = "--";
  std::string top_task_text = "--";
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *value_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  lv_obj_t *label_lbl = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *list_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t secondary_color = DEFAULT_OFF_COLOR;
  int width_compensation_percent = 100;
  bool available = false;
  bool show_count = true;
  bool show_top_task = false;
  bool top_task_limit_two_lines = false;
  bool label_shows_count = false;
  bool show_completed_items = true;
};

struct TodoItemClick {
  TodoCardCtx *ctx = nullptr;
  TodoItem item;
};

struct TodoModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *close_btn = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *list = nullptr;
  lv_obj_t *status_lbl = nullptr;
  lv_timer_t *loading_timer = nullptr;
  TodoCardCtx *active = nullptr;
  uint32_t loading_call_id = 0;
  TodoItemClick item_clicks[TODO_MAX_ITEMS];
  std::vector<TodoItem> visible_items;
  std::vector<TodoItem> completed_items;
};

inline TodoModalUi &todo_modal_ui() {
  static TodoModalUi ui;
  return ui;
}

inline bool todo_card_context_valid(TodoCardCtx *ctx) {
  return ctx != nullptr && ctx->magic == TODO_CARD_CTX_MAGIC;
}

inline bool todo_entity_id_safe(const std::string &entity_id) {
  if (entity_id.compare(0, 5, "todo.") != 0) return false;
  for (char ch : entity_id) {
    if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.')) return false;
  }
  return true;
}

inline std::string todo_card_label(TodoCardCtx *ctx) {
  if (!ctx) return "Todo";
  if (!ctx->configured_label.empty()) return ctx->configured_label;
  if (!ctx->friendly_name.empty()) return ctx->friendly_name;
  if (!ctx->entity_id.empty()) return ctx->entity_id;
  return "Todo";
}

inline std::string todo_card_count_label(TodoCardCtx *ctx) {
  if (!ctx || !ctx->available || ctx->count_text.empty() || ctx->count_text == "--") return "--";
  return ctx->count_text;
}

inline void todo_apply_card_text(TodoCardCtx *ctx) {
  if (!ctx) return;
  if (ctx->label_lbl) {
    std::string label = ctx->label_shows_count ? todo_card_count_label(ctx) : todo_card_label(ctx);
    lv_label_set_text(ctx->label_lbl, label.c_str());
  }
  if (!ctx->show_count) return;
  if (ctx->show_top_task) {
    if (ctx->value_lbl) lv_label_set_text(ctx->value_lbl, ctx->available ? ctx->top_task_text.c_str() : "--");
    if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
    return;
  }
  if (ctx->value_lbl) lv_label_set_text(ctx->value_lbl, ctx->available ? ctx->count_text.c_str() : "--");
  if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
}

inline void todo_apply_top_task_text_layout(TodoCardCtx *ctx) {
  if (!todo_card_context_valid(ctx) || !ctx->show_top_task || !ctx->value_lbl) return;
  if (ctx->list_font) lv_obj_set_style_text_font(ctx->value_lbl, ctx->list_font, LV_PART_MAIN);
  lv_label_set_long_mode(ctx->value_lbl, LV_LABEL_LONG_DOT);
  lv_obj_set_width(ctx->value_lbl, lv_pct(100));
  if (ctx->top_task_limit_two_lines && ctx->list_font && ctx->list_font->line_height > 0) {
    constexpr lv_coord_t TODO_TOP_TASK_LINE_SPACE = -1;
    lv_obj_set_style_text_line_space(ctx->value_lbl, TODO_TOP_TASK_LINE_SPACE, LV_PART_MAIN);
    lv_obj_set_height(ctx->value_lbl, ctx->list_font->line_height * 2 + TODO_TOP_TASK_LINE_SPACE);
  }
}

inline void setup_todo_card(BtnSlot &s, const ParsedCfg &p, uint32_t secondary_color) {
  lv_obj_set_style_bg_color(s.btn, lv_color_hex(secondary_color),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  bool show_count = todo_card_show_count(p);
  if (show_count) {
    lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  }
  if (todo_card_shows_top_task(p)) {
    lv_obj_set_width(s.sensor_container, lv_pct(100));
    lv_obj_set_width(s.sensor_lbl, lv_pct(100));
    lv_label_set_long_mode(s.sensor_lbl, LV_LABEL_LONG_DOT);
  }
  lv_label_set_text(s.icon_lbl,
    (!p.icon.empty() && p.icon != "Auto") ? find_icon(p.icon.c_str()) : find_icon("Check"));
  lv_label_set_text(s.sensor_lbl, "--");
  lv_label_set_text(s.unit_lbl, "");
  lv_obj_add_flag(s.unit_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.text_lbl, todo_card_label_shows_count(p) ? "--" : (p.label.empty() ? "Todo" : p.label.c_str()));
  apply_push_button_transition(s.btn);
}

inline std::string todo_percent_decode(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  for (size_t i = 0; i < value.size(); i++) {
    if (value[i] == '%' && i + 2 < value.size() &&
        std::isxdigit(static_cast<unsigned char>(value[i + 1])) &&
        std::isxdigit(static_cast<unsigned char>(value[i + 2]))) {
      char hex[3] = {value[i + 1], value[i + 2], '\0'};
      out.push_back(static_cast<char>(std::strtol(hex, nullptr, 16)));
      i += 2;
    } else {
      out.push_back(value[i]);
    }
  }
  return out;
}

inline std::vector<TodoItem> parse_todo_response_payload(const std::string &payload) {
  std::vector<TodoItem> items;
  size_t start = 0;
  while (start <= payload.size() && items.size() < TODO_MAX_ITEMS + 1) {
    size_t end = payload.find('\n', start);
    if (end == std::string::npos) end = payload.size();
    std::string line = payload.substr(start, end - start);
    if (!line.empty()) {
      size_t sep = line.find('|');
      std::string key = sep == std::string::npos ? line : line.substr(0, sep);
      std::string summary = sep == std::string::npos ? line : line.substr(sep + 1);
      TodoItem item;
      item.key = todo_percent_decode(key);
      item.summary = todo_percent_decode(summary);
      item.more = item.key == "__MORE__";
      items.push_back(item);
    }
    if (end == payload.size()) break;
    start = end + 1;
  }
  return items;
}

inline void todo_set_top_task_from_items(TodoCardCtx *ctx, const std::vector<TodoItem> &items) {
  if (!todo_card_context_valid(ctx) || !ctx->show_top_task) return;
  ctx->top_task_text = "All done";
  for (const auto &item : items) {
    if (!item.more && !item.summary.empty()) {
      ctx->top_task_text = item.summary;
      break;
    }
  }
  todo_apply_card_text(ctx);
}

inline std::string todo_item_action_key(const TodoItem &item) {
  return item.key.empty() ? item.summary : item.key;
}

inline bool todo_items_match(const TodoItem &a, const TodoItem &b) {
  std::string a_key = todo_item_action_key(a);
  std::string b_key = todo_item_action_key(b);
  return !a_key.empty() && a_key == b_key;
}

inline bool todo_completed_contains(const TodoModalUi &ui, const TodoItem &item) {
  for (const auto &completed : ui.completed_items) {
    if (todo_items_match(completed, item)) return true;
  }
  return false;
}

inline void todo_completed_remove(const TodoItem &item) {
  TodoModalUi &ui = todo_modal_ui();
  std::vector<TodoItem> kept;
  kept.reserve(ui.completed_items.size());
  for (const auto &completed : ui.completed_items) {
    if (!todo_items_match(completed, item)) kept.push_back(completed);
  }
  ui.completed_items = kept;
}

inline void todo_completed_add(const TodoItem &item) {
  if (item.more || todo_item_action_key(item).empty()) return;
  TodoModalUi &ui = todo_modal_ui();
  if (todo_completed_contains(ui, item)) return;
  ui.completed_items.push_back(item);
  while (ui.completed_items.size() > TODO_MAX_ITEMS) ui.completed_items.erase(ui.completed_items.begin());
}

inline void todo_modal_cancel_loading_timer() {
  TodoModalUi &ui = todo_modal_ui();
  ui.loading_call_id = 0;
  if (ui.loading_timer) {
    lv_timer_del(ui.loading_timer);
    ui.loading_timer = nullptr;
  }
}

inline void todo_modal_hide() {
  TodoModalUi &ui = todo_modal_ui();
  todo_modal_cancel_loading_timer();
  control_modal_delete_overlay(ControlModalKind::TODO_LIST, ui.overlay);
  ui = TodoModalUi();
}

inline void todo_modal_set_status(const char *text) {
  TodoModalUi &ui = todo_modal_ui();
  bool wants_visible = text && text[0];
  if (!wants_visible && !ui.status_lbl) return;
  if (!ui.status_lbl && ui.list) {
    ui.status_lbl = lv_label_create(ui.list);
    lv_label_set_long_mode(ui.status_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ui.status_lbl, lv_pct(100));
    lv_obj_set_style_text_color(ui.status_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui.status_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (ui.active && ui.active->label_font)
      lv_obj_set_style_text_font(ui.status_lbl, ui.active->label_font, LV_PART_MAIN);
  }
  if (!ui.status_lbl) return;
  lv_label_set_text(ui.status_lbl, text ? text : "");
  if (wants_visible) lv_obj_clear_flag(ui.status_lbl, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(ui.status_lbl, LV_OBJ_FLAG_HIDDEN);
}

inline void todo_modal_loading_timeout_cb(lv_timer_t *timer) {
  TodoModalUi &ui = todo_modal_ui();
  if (ui.loading_timer != timer) {
    lv_timer_del(timer);
    return;
  }
  TodoCardCtx *ctx = ui.active;
  ui.loading_timer = nullptr;
  ui.loading_call_id = 0;
  if (todo_card_context_valid(ctx)) {
    ESP_LOGW("todo", "Todo request timed out for %s",
      ctx->entity_id.empty() ? "todo" : ctx->entity_id.c_str());
    todo_modal_set_status("Could not load");
  }
  lv_timer_del(timer);
}

inline void todo_modal_start_loading_timer(uint32_t call_id) {
  todo_modal_cancel_loading_timer();
  TodoModalUi &ui = todo_modal_ui();
  ui.loading_call_id = call_id;
  ui.loading_timer = lv_timer_create(todo_modal_loading_timeout_cb, TODO_ITEMS_REQUEST_TIMEOUT_MS, nullptr);
}

inline void todo_modal_clear_items() {
  TodoModalUi &ui = todo_modal_ui();
  if (ui.list) lv_obj_clean(ui.list);
  ui.status_lbl = nullptr;
  for (int i = 0; i < TODO_MAX_ITEMS; i++) ui.item_clicks[i] = TodoItemClick();
}

inline void request_todo_items(TodoCardCtx *ctx);
inline void todo_modal_render_items(TodoCardCtx *ctx, const std::vector<TodoItem> &items);

inline void send_todo_status_action(TodoCardCtx *ctx, const TodoItem &item, const char *status) {
  std::string key = todo_item_action_key(item);
  if (!todo_card_context_valid(ctx) || key.empty() || !status || !status[0]) return;

  esphome::api::HomeassistantActionRequest req;
  static uint32_t call_id = 350000;
  uint32_t next_id = call_id++;
  if (!ha_action_begin(req, "todo.update_item", false, 3, next_id)) return;
  ha_action_add_entity(req, ctx->entity_id);
  ha_action_add_data(req, "item", key.c_str());
  ha_action_add_data(req, "status", status);

  bool completing = std::string(status) == "completed";
  ha_register_action_response_callback(
    req.call_id,
    [ctx, item, completing](const esphome::api::ActionResponse &response) {
      if (!response.is_success()) {
        ESP_LOGW("todo", "Updating todo item failed for %s: %s",
          ctx && !ctx->entity_id.empty() ? ctx->entity_id.c_str() : "todo",
          response.get_error_message().c_str());
        if (completing) todo_completed_remove(item);
        else todo_completed_add(item);
        if (todo_modal_ui().active == ctx) todo_modal_render_items(ctx, todo_modal_ui().visible_items);
        todo_modal_set_status(completing ? "Could not complete" : "Could not restore");
        return;
      }
    });
  ha_action_send(req);
}

inline void send_todo_complete_action(TodoCardCtx *ctx, const TodoItem &item) {
  send_todo_status_action(ctx, item, "completed");
}

inline void send_todo_restore_action(TodoCardCtx *ctx, const TodoItem &item) {
  send_todo_status_action(ctx, item, "needs_action");
}

inline lv_obj_t *todo_modal_create_list_item_row(
    lv_obj_t *parent,
    const std::string &label,
    bool clickable,
    bool show_checkbox,
    bool checked,
    bool completed,
    lv_coord_t height,
    lv_coord_t content_width,
    lv_coord_t checkbox_size,
    lv_coord_t gap,
    uint32_t accent_color,
    const lv_font_t *font,
    const lv_font_t *icon_font,
    int width_compensation_percent) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_set_width(row, lv_pct(100));
  lv_obj_set_height(row, height);
  lv_obj_set_style_radius(row, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  if (clickable) {
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    control_modal_apply_pressed_fill(row);
  } else {
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
  }

  lv_coord_t label_x = 0;
  lv_coord_t label_w = content_width;
  if (show_checkbox) {
    uint32_t checkbox_color = completed ? TODO_COMPLETED_TEXT_COLOR : (checked ? accent_color : DARK_TEXT_MUTED);
    uint32_t check_color = completed ? TODO_COMPLETED_CHECK_COLOR : checkbox_color;
    lv_obj_t *box = lv_obj_create(row);
    lv_obj_set_size(box, checkbox_size, checkbox_size);
    lv_obj_set_style_radius(box, checkbox_size / 4, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(box, lv_color_hex(checkbox_color), LV_PART_MAIN);
    lv_obj_set_style_border_width(box, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(box, 0, LV_PART_MAIN);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(box, LV_ALIGN_LEFT_MID, 0, 0);
    if (checked) {
      lv_obj_t *check = lv_label_create(box);
      lv_label_set_text(check, find_icon("Check"));
      lv_obj_set_style_text_color(check, lv_color_hex(check_color), LV_PART_MAIN);
      lv_obj_set_style_text_align(check, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
      if (icon_font) lv_obj_set_style_text_font(check, icon_font, LV_PART_MAIN);
      uint16_t check_zoom = 96;
      if (icon_font && icon_font->line_height > 0) {
        int fit_zoom = static_cast<int>(checkbox_size) * 300 / icon_font->line_height;
        if (fit_zoom < 72) fit_zoom = 72;
        if (fit_zoom > 110) fit_zoom = 110;
        check_zoom = static_cast<uint16_t>(fit_zoom);
      }
      lv_obj_update_layout(check);
      lv_coord_t offset_x = lv_obj_get_width(check) * (256 - check_zoom) / 512;
      lv_coord_t offset_y = lv_obj_get_height(check) * (256 - check_zoom) / 512;
      lv_obj_set_style_transform_zoom(check, check_zoom, LV_PART_MAIN);
      lv_obj_align(check, LV_ALIGN_CENTER, offset_x, offset_y);
    }
    label_x = checkbox_size + gap;
    label_w = content_width > label_x ? content_width - label_x : lv_pct(100);
  }

  lv_obj_t *value = lv_label_create(row);
  lv_label_set_text(value, label.c_str());
  lv_label_set_long_mode(value, LV_LABEL_LONG_DOT);
  lv_obj_set_width(value, label_w);
  lv_obj_set_style_text_color(value,
    lv_color_hex(completed ? TODO_COMPLETED_TEXT_COLOR : (show_checkbox ? DARK_TEXT_SOFT : DARK_TEXT_MUTED)), LV_PART_MAIN);
  lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(value, font, LV_PART_MAIN);
  apply_width_compensation(value, width_compensation_percent);
  lv_obj_align(value, LV_ALIGN_LEFT_MID, label_x, 0);

  return row;
}

inline void todo_modal_render_items(TodoCardCtx *ctx, const std::vector<TodoItem> &items) {
  TodoModalUi &ui = todo_modal_ui();
  if (!todo_card_context_valid(ctx) || ui.active != ctx || !ui.list) return;
  ui.visible_items = items;
  todo_modal_clear_items();
  todo_modal_set_status("");
  if (ctx->show_top_task) {
    std::vector<TodoItem> top_items;
    for (const auto &item : items) {
      if (item.more || todo_completed_contains(ui, item)) continue;
      top_items.push_back(item);
      break;
    }
    todo_set_top_task_from_items(ctx, top_items);
  }

  bool has_visible_item = false;
  for (const auto &item : items) {
    if (item.more || !todo_completed_contains(ui, item)) has_visible_item = true;
  }

  if (!has_visible_item &&
      (ui.completed_items.empty() || !ctx->show_completed_items)) {
    todo_modal_set_status("All done");
    return;
  }

  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);
  lv_coord_t checkbox_size = ctx->label_font && ctx->label_font->line_height > 0
    ? ctx->label_font->line_height
    : control_modal_scaled_px(15, layout.short_side);
  if (checkbox_size < 14) checkbox_size = 14;
  lv_coord_t row_pad = control_modal_scaled_px(6, layout.short_side);
  if (row_pad < 4) row_pad = 4;
  lv_coord_t row_h = checkbox_size + row_pad;
  if (ctx->label_font && ctx->label_font->line_height + row_pad > row_h) {
    row_h = ctx->label_font->line_height + row_pad;
  }
  lv_coord_t item_gap = control_modal_scaled_px(18, layout.short_side);
  if (item_gap < 12) item_gap = 12;
  lv_coord_t content_w = ui.list ? lv_obj_get_width(ui.list) : layout.panel_w;
  if (content_w <= 0) content_w = layout.panel_w - layout.inset * 2;
  int click_index = 0;
  for (const auto &item : items) {
    if (!item.more && todo_completed_contains(ui, item)) continue;
    if (item.more) {
      std::string label = item.summary.empty() ? "More items" : item.summary + " more";
      todo_modal_create_list_item_row(
        ui.list, label, false, false, false, false, row_h, content_w,
        checkbox_size, item_gap, ctx->accent_color, ctx->label_font,
        ctx->icon_font, ctx->width_compensation_percent);
      continue;
    }
    if (click_index >= TODO_MAX_ITEMS) break;
    lv_obj_t *row = todo_modal_create_list_item_row(
      ui.list, item.summary.empty() ? "(untitled)" : item.summary, true, true, false, false,
      row_h, content_w, checkbox_size, item_gap, ctx->accent_color,
      ctx->label_font, ctx->icon_font, ctx->width_compensation_percent);
    ui.item_clicks[click_index].ctx = ctx;
    ui.item_clicks[click_index].item = item;
    lv_obj_add_event_cb(row, [](lv_event_t *e) {
      TodoItemClick *click = (TodoItemClick *)lv_event_get_user_data(e);
      if (!click || !click->ctx) return;
      TodoCardCtx *click_ctx = click->ctx;
      TodoItem item = click->item;
      todo_completed_add(item);
      todo_modal_render_items(click_ctx, todo_modal_ui().visible_items);
      send_todo_complete_action(click_ctx, item);
    }, LV_EVENT_CLICKED, &ui.item_clicks[click_index]);
    click_index++;
  }

  if (ctx->show_completed_items && !ui.completed_items.empty()) {
    for (const auto &completed : ui.completed_items) {
      if (click_index >= TODO_MAX_ITEMS) break;
      lv_obj_t *row = todo_modal_create_list_item_row(
        ui.list, completed.summary.empty() ? "(untitled)" : completed.summary,
        true, true, true, true, row_h, content_w, checkbox_size, item_gap,
        ctx->accent_color, ctx->label_font, ctx->icon_font,
        ctx->width_compensation_percent);
      ui.item_clicks[click_index].ctx = ctx;
      ui.item_clicks[click_index].item = completed;
      lv_obj_add_event_cb(row, [](lv_event_t *e) {
        TodoItemClick *click = (TodoItemClick *)lv_event_get_user_data(e);
        if (!click || !click->ctx) return;
        TodoCardCtx *click_ctx = click->ctx;
        TodoItem item = click->item;
        todo_completed_remove(item);
        todo_modal_render_items(click_ctx, todo_modal_ui().visible_items);
        send_todo_restore_action(click_ctx, item);
      }, LV_EVENT_CLICKED, &ui.item_clicks[click_index]);
      click_index++;
    }
  }
}

inline std::string todo_items_response_template(const std::string &entity_id) {
  return std::string("{% set entity = '") + entity_id + "' %}"
    "{% set items = response.get(entity, {}).get('items', []) %}"
    "{% set ns = namespace(count=0, out='') %}"
    "{% macro esc(v) -%}{{ (v|string)|replace('%','%25')|replace('|','%7C')|replace('\\n','%0A')|replace('\\r','%0D') }}{%- endmacro %}"
    "{% for item in items %}"
    "{% if item.status is not defined or item.status == 'needs_action' %}"
    "{% if ns.count < " + std::to_string(TODO_MAX_ITEMS) + " %}"
    "{% set summary = item.summary if item.summary is defined else '' %}"
    "{% set key = item.uid if item.uid is defined and item.uid else summary %}"
    "{% set ns.out = ns.out ~ ('\\n' if ns.out else '') ~ esc(key) ~ '|' ~ esc(summary) %}"
    "{% endif %}"
    "{% set ns.count = ns.count + 1 %}"
    "{% endif %}"
    "{% endfor %}"
    "{{ ns.out }}{% if ns.count > " + std::to_string(TODO_MAX_ITEMS) +
    " %}\\n__MORE__|{{ ns.count - " + std::to_string(TODO_MAX_ITEMS) + " }}{% endif %}";
}

inline uint32_t next_todo_items_call_id() {
  static uint32_t call_id = 300000;
  return call_id++;
}

inline bool todo_begin_get_items_request(esphome::api::HomeassistantActionRequest &req,
                                         TodoCardCtx *ctx,
                                         uint32_t call_id) {
  if (!todo_card_context_valid(ctx) || !todo_entity_id_safe(ctx->entity_id)) return false;
  if (!ha_action_begin(req, "todo.get_items", false, 1, call_id)) return false;
  req.wants_response = true;
  std::string response_template = todo_items_response_template(ctx->entity_id);
  req.response_template = decltype(req.response_template)(response_template);
  ha_action_add_entity(req, ctx->entity_id);
  return true;
}

inline void request_todo_items(TodoCardCtx *ctx) {
  if (!todo_card_context_valid(ctx) || !todo_entity_id_safe(ctx->entity_id)) return;
  todo_modal_clear_items();
  todo_modal_set_status("Loading");

  esphome::api::HomeassistantActionRequest req;
  uint32_t call_id = next_todo_items_call_id();
  if (!todo_begin_get_items_request(req, ctx, call_id)) {
    todo_modal_set_status("Could not load");
    return;
  }
  todo_modal_start_loading_timer(call_id);

  bool registered = ha_register_action_response_callback(
    req.call_id,
    [ctx, call_id](const esphome::api::ActionResponse &response) {
      TodoModalUi &ui = todo_modal_ui();
      if (ui.active != ctx || ui.loading_call_id != call_id) return;
      todo_modal_cancel_loading_timer();
      if (!response.is_success()) {
        ESP_LOGW("todo", "Todo request failed for %s: %s",
          ctx && !ctx->entity_id.empty() ? ctx->entity_id.c_str() : "todo",
          response.get_error_message().c_str());
        todo_modal_set_status("Could not load");
        return;
      }
      auto json = response.get_json();
      const char *payload = json["response"].as<const char *>();
      if (payload == nullptr) {
        todo_modal_set_status("Could not load");
        return;
      }
      std::vector<TodoItem> items =
        parse_todo_response_payload(std::string(payload).substr(0, TODO_RESPONSE_TEXT_MAX_LEN));
      todo_modal_render_items(ctx, items);
    });
  if (!registered || !ha_action_send(req)) {
    todo_modal_cancel_loading_timer();
    todo_modal_set_status("Could not load");
  }
}

inline void request_todo_top_task(TodoCardCtx *ctx) {
  if (!todo_card_context_valid(ctx) || !ctx->show_top_task ||
      !ctx->available || !todo_entity_id_safe(ctx->entity_id)) return;

  esphome::api::HomeassistantActionRequest req;
  uint32_t call_id = next_todo_items_call_id();
  if (!todo_begin_get_items_request(req, ctx, call_id)) return;

  ha_register_action_response_callback(
    req.call_id,
    [ctx](const esphome::api::ActionResponse &response) {
      if (!response.is_success()) {
        ESP_LOGW("todo", "Todo top task request failed for %s: %s",
          ctx && !ctx->entity_id.empty() ? ctx->entity_id.c_str() : "todo",
          response.get_error_message().c_str());
        return;
      }
      auto json = response.get_json();
      const char *payload = json["response"].as<const char *>();
      if (payload == nullptr) return;
      std::vector<TodoItem> items =
        parse_todo_response_payload(std::string(payload).substr(0, TODO_RESPONSE_TEXT_MAX_LEN));
      todo_set_top_task_from_items(ctx, items);
    });
  ha_action_send(req);
}

inline void todo_card_open_modal(TodoCardCtx *ctx) {
  if (!todo_card_context_valid(ctx) || ctx->entity_id.empty() || !ctx->available) return;
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::TODO_LIST, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, "\U000F0141", false, todo_modal_hide);
  TodoModalUi &ui = todo_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.close_btn = shell.close_btn;

  ControlModalLayout &layout = shell.layout;
  lv_coord_t content_w = shell.content_w;
  lv_coord_t gap = control_modal_scaled_px(12, layout.short_side);
  if (gap < 8) gap = 8;
  lv_coord_t title_y = layout.inset + layout.back_size / 2;
  lv_coord_t list_y = layout.inset + layout.back_size + gap;
  lv_coord_t list_h = layout.panel_h - list_y - layout.inset;
  if (list_h < 60) list_h = 60;
  lv_coord_t list_pad = control_modal_scaled_px(14, layout.short_side);
  if (list_pad < 6) list_pad = 6;
  lv_coord_t list_w = content_w - list_pad * 2;
  if (list_w < 80) {
    list_w = content_w;
    list_pad = 0;
  }

  ui.title_lbl = control_modal_create_title(
    ui.panel, todo_card_label(ctx), content_w - layout.back_size - gap,
    ctx->list_font, ctx->width_compensation_percent);
  lv_obj_set_style_text_color(ui.title_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_align(ui.title_lbl, LV_ALIGN_TOP_MID, 0, title_y - layout.back_size / 2);

  lv_coord_t row_gap = control_modal_scaled_px(6, layout.short_side);
  if (row_gap < 4) row_gap = 4;
  ui.list = control_modal_create_scroll_list(ui.panel, list_w, list_h, row_gap);
  lv_obj_align(ui.list, LV_ALIGN_TOP_LEFT, layout.inset + list_pad, list_y);

  lv_obj_move_foreground(ui.overlay);
  request_todo_items(ctx);
}

inline TodoCardCtx *create_todo_card_context(
    BtnSlot &s, const ParsedCfg &p,
    uint32_t accent_color, uint32_t secondary_color,
    const lv_font_t *label_font,
    const lv_font_t *list_font,
    const lv_font_t *icon_font,
    int width_compensation_percent,
    bool top_task_limit_two_lines = false) {
  TodoCardCtx *ctx = new TodoCardCtx();
  ctx->entity_id = p.entity;
  ctx->configured_label = p.label;
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->value_lbl = s.sensor_lbl;
  ctx->unit_lbl = s.unit_lbl;
  ctx->label_lbl = s.text_lbl;
  ctx->accent_color = accent_color;
  ctx->secondary_color = secondary_color;
  ctx->label_font = label_font;
  ctx->list_font = list_font ? list_font : label_font;
  ctx->icon_font = icon_font;
  ctx->width_compensation_percent = width_compensation_percent;
  ctx->show_count = todo_card_show_count(p);
  ctx->show_top_task = todo_card_shows_top_task(p);
  ctx->top_task_limit_two_lines = top_task_limit_two_lines;
  todo_apply_top_task_text_layout(ctx);
  ctx->label_shows_count = todo_card_label_shows_count(p);
  ctx->show_completed_items = todo_card_shows_completed_items(p);
  lv_obj_set_user_data(s.btn, ctx);
  todo_apply_card_text(ctx);
  return ctx;
}

inline void subscribe_todo_state(TodoCardCtx *ctx) {
  if (!todo_card_context_valid(ctx) || ctx->entity_id.empty()) return;
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      bool unavailable = ha_state_unavailable_ref(state);
      ctx->available = !unavailable;
      ctx->count_text = unavailable ? "--" : string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
      apply_control_availability(ctx->btn, ctx->btn, ctx->available);
      todo_apply_card_text(ctx);
      if (todo_modal_ui().active == ctx && !ctx->available) todo_modal_set_status("Could not load");
      else if (todo_modal_ui().active == ctx) request_todo_items(ctx);
      else request_todo_top_task(ctx);
    })
  );
}

inline void subscribe_todo_friendly_name(TodoCardCtx *ctx) {
  if (!todo_card_context_valid(ctx) || ctx->entity_id.empty() || !ctx->configured_label.empty()) return;
  ha_subscribe_attribute(
    ctx->entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef name) {
      ctx->friendly_name = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
      todo_apply_card_text(ctx);
      TodoModalUi &ui = todo_modal_ui();
      if (ui.active == ctx && ui.title_lbl) lv_label_set_text(ui.title_lbl, todo_card_label(ctx).c_str());
    })
  );
}
