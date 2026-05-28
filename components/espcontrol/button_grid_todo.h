#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// Home Assistant todo card controls.

constexpr uint32_t TODO_CARD_CTX_MAGIC = 0x544F444F;  // TODO
constexpr int TODO_MAX_ITEMS = 8;
constexpr size_t TODO_RESPONSE_TEXT_MAX_LEN = 1536;
constexpr int TODO_RESPONSE_KEY_MAX_LEN = 96;
constexpr int TODO_RESPONSE_SUMMARY_MAX_LEN = 80;
constexpr uint32_t TODO_REQUEST_TIMEOUT_MS = 15000;

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
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *value_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  lv_obj_t *label_lbl = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *list_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  uint32_t secondary_color = DEFAULT_OFF_COLOR;
  int width_compensation_percent = 100;
  bool available = false;
};

struct TodoItemClick {
  TodoCardCtx *ctx = nullptr;
  std::string key;
  bool completed = false;
};

struct TodoModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *close_btn = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *list = nullptr;
  lv_obj_t *status_lbl = nullptr;
  TodoCardCtx *active = nullptr;
  TodoItemClick item_clicks[TODO_MAX_ITEMS];
  int visible_item_count = 0;
  bool more_items_visible = false;
  bool waiting_for_ha = false;
};

struct TodoRequestState {
  uint32_t call_id = 0;
  uint32_t started_ms = 0;
};

inline TodoModalUi &todo_modal_ui() {
  static TodoModalUi ui;
  return ui;
}

inline TodoRequestState &todo_request_state() {
  static TodoRequestState state;
  return state;
}

inline void todo_modal_set_status(const char *text);

inline void todo_clear_request_state(uint32_t call_id) {
  TodoRequestState &state = todo_request_state();
  if (state.call_id == call_id) state = TodoRequestState();
}

inline void todo_cancel_request(uint32_t call_id, const char *reason) {
  if (call_id == 0) return;
  ESP_LOGW("todo", "Cancelling todo request %u: %s",
    (unsigned) call_id, reason ? reason : "cancelled");
  todo_clear_request_state(call_id);
  ha_cancel_action_response_callback(call_id, reason);
}

inline void todo_cancel_pending_request(const char *reason) {
  uint32_t call_id = todo_request_state().call_id;
  if (call_id == 0) return;
  todo_cancel_request(call_id, reason);
  TodoModalUi &ui = todo_modal_ui();
  if (ui.active != nullptr) {
    ui.waiting_for_ha = true;
    todo_modal_set_status("Waiting for Home Assistant");
  }
}

inline void todo_cancel_stale_request() {
  TodoRequestState &state = todo_request_state();
  if (state.call_id == 0) return;
  if (esphome::millis() - state.started_ms >= TODO_REQUEST_TIMEOUT_MS) {
    uint32_t call_id = state.call_id;
    todo_cancel_request(call_id, "timeout");
  }
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

inline void todo_apply_card_text(TodoCardCtx *ctx) {
  if (!ctx) return;
  if (ctx->label_lbl) lv_label_set_text(ctx->label_lbl, todo_card_label(ctx).c_str());
  if (ctx->value_lbl) lv_label_set_text(ctx->value_lbl, ctx->available ? ctx->count_text.c_str() : "--");
  if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
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
  lv_label_set_text(s.icon_lbl,
    (!p.icon.empty() && p.icon != "Auto") ? find_icon(p.icon.c_str()) : find_icon("Check"));
  lv_label_set_text(s.sensor_lbl, "--");
  lv_label_set_text(s.unit_lbl, "");
  lv_label_set_text(s.text_lbl, p.label.empty() ? "Todo" : p.label.c_str());
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

inline bool parse_todo_response_json(JsonObjectConst json,
                                     const std::string &entity_id,
                                     std::vector<TodoItem> &items) {
  JsonVariantConst response = json["response"];
  const char *payload = response.as<const char *>();
  if (payload != nullptr) {
    items = parse_todo_response_payload(std::string(payload).substr(0, TODO_RESPONSE_TEXT_MAX_LEN));
    return true;
  }

  JsonObjectConst response_obj = response.as<JsonObjectConst>();
  if (response_obj.isNull()) response_obj = json;
  JsonArrayConst item_array = response_obj[entity_id]["items"].as<JsonArrayConst>();
  if (item_array.isNull()) return false;

  int incomplete_count = 0;
  for (JsonVariantConst value : item_array) {
    const char *status = value["status"] | "";
    if (status[0] != '\0' && strcmp(status, "needs_action") != 0) continue;
    if (incomplete_count < TODO_MAX_ITEMS) {
      const char *summary = value["summary"] | "";
      const char *uid = value["uid"] | "";
      TodoItem item;
      item.key = uid[0] != '\0' ? uid : summary;
      item.summary = summary;
      items.push_back(item);
    }
    incomplete_count++;
  }

  if (incomplete_count > TODO_MAX_ITEMS) {
    TodoItem more;
    more.key = "__MORE__";
    more.summary = std::to_string(incomplete_count - TODO_MAX_ITEMS);
    more.more = true;
    items.push_back(more);
  }
  return true;
}

inline std::string todo_item_action_key(const TodoItem &item) {
  return item.key.empty() ? item.summary : item.key;
}

inline void todo_modal_hide() {
  TodoModalUi &ui = todo_modal_ui();
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

inline void todo_modal_clear_items() {
  TodoModalUi &ui = todo_modal_ui();
  if (ui.list) lv_obj_clean(ui.list);
  ui.status_lbl = nullptr;
  ui.visible_item_count = 0;
  ui.more_items_visible = false;
  for (int i = 0; i < TODO_MAX_ITEMS; i++) ui.item_clicks[i] = TodoItemClick();
}

inline void request_todo_items(TodoCardCtx *ctx);
inline void todo_modal_render_items(TodoCardCtx *ctx, const std::vector<TodoItem> &items);

inline void send_todo_complete_action(TodoCardCtx *ctx, const std::string &key) {
  if (!todo_card_context_valid(ctx) || key.empty()) return;
  if (!ha_api_state_connected()) {
    todo_modal_set_status("Could not complete");
    return;
  }

  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, "todo.update_item", false, 3)) {
    todo_modal_set_status("Could not complete");
    return;
  }
  ha_action_add_entity(req, ctx->entity_id);
  ha_action_add_data(req, "item", key.c_str());
  ha_action_add_data(req, "status", "completed");

  if (!ha_action_send(req)) {
    todo_modal_set_status("Could not complete");
    return;
  }
}

inline lv_obj_t *todo_modal_create_list_item_row(
    lv_obj_t *parent,
    const std::string &label,
    bool clickable,
    bool show_checkbox,
    bool checked,
    lv_coord_t height,
    lv_coord_t content_width,
    lv_coord_t checkbox_size,
    lv_coord_t gap,
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
    uint32_t checkbox_color = checked ? DARK_TEXT_SOFT : DARK_TEXT_MUTED;
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
      lv_obj_set_style_text_color(check, lv_color_hex(checkbox_color), LV_PART_MAIN);
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
    lv_color_hex(show_checkbox ? DARK_TEXT_SOFT : DARK_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(value, font, LV_PART_MAIN);
  apply_width_compensation(value, width_compensation_percent);
  lv_obj_align(value, LV_ALIGN_LEFT_MID, label_x, 0);

  return row;
}

inline void todo_modal_render_items(TodoCardCtx *ctx, const std::vector<TodoItem> &items) {
  TodoModalUi &ui = todo_modal_ui();
  if (!todo_card_context_valid(ctx) || ui.active != ctx || !ui.list) return;
  todo_modal_clear_items();
  todo_modal_set_status("");

  if (items.empty()) {
    todo_modal_set_status("All done");
    return;
  }

  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);
  lv_coord_t checkbox_size = ctx->label_font && ctx->label_font->line_height > 0
    ? ctx->label_font->line_height
    : control_modal_scaled_px(15, layout.short_side);
  bool compact_portrait = control_modal_uses_compact_portrait_tuning(layout);
  if (compact_portrait) checkbox_size = checkbox_size * 3 / 4;
  lv_coord_t min_checkbox_size = compact_portrait ? 12 : 14;
  if (checkbox_size < min_checkbox_size) checkbox_size = min_checkbox_size;
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
    if (item.more) {
      ui.more_items_visible = true;
      std::string label = item.summary.empty() ? "More items" : item.summary + " more";
      todo_modal_create_list_item_row(
        ui.list, label, false, false, false, row_h, content_w,
        checkbox_size, item_gap, ctx->label_font,
        ctx->icon_font, ctx->width_compensation_percent);
      continue;
    }
    if (click_index >= TODO_MAX_ITEMS) break;
    lv_obj_t *row = todo_modal_create_list_item_row(
      ui.list, item.summary.empty() ? "(untitled)" : item.summary, true, true, false,
      row_h, content_w, checkbox_size, item_gap,
      ctx->label_font, ctx->icon_font, ctx->width_compensation_percent);
    ui.item_clicks[click_index].ctx = ctx;
    ui.item_clicks[click_index].key = todo_item_action_key(item);
    ui.visible_item_count++;
    lv_obj_add_event_cb(row, [](lv_event_t *e) {
      TodoItemClick *click = (TodoItemClick *)lv_event_get_user_data(e);
      if (!click || !click->ctx || click->completed) return;
      click->completed = true;
      lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
      if (target) {
        lv_obj_add_state(target, LV_STATE_DISABLED);
        lv_obj_add_flag(target, LV_OBJ_FLAG_HIDDEN);
      }
      TodoModalUi &ui = todo_modal_ui();
      if (ui.visible_item_count > 0) ui.visible_item_count--;
      if (ui.visible_item_count == 0 && !ui.more_items_visible) todo_modal_set_status("All done");
      send_todo_complete_action(click->ctx, click->key);
    }, LV_EVENT_CLICKED, &ui.item_clicks[click_index]);
    click_index++;
  }
}

inline std::string todo_items_response_template(const std::string &entity_id) {
  return std::string("{% set entity = '") + entity_id + "' %}"
    "{% set items = response.get(entity, {}).get('items', []) %}"
    "{% set ns = namespace(count=0, shown=0, out='') %}"
    "{% macro esc(v) -%}{{ (v|string)|replace('%','%25')|replace('|','%7C')|replace('\\n','%0A')|replace('\\r','%0D') }}{%- endmacro %}"
    "{% for item in items %}"
    "{% if item.status is not defined or item.status == 'needs_action' %}"
    "{% if ns.shown < " + std::to_string(TODO_MAX_ITEMS) + " %}"
    "{% set summary = item.summary if item.summary is defined else '' %}"
    "{% set key = item.uid if item.uid is defined and item.uid else summary %}"
    "{% set key = (key|string)[:" + std::to_string(TODO_RESPONSE_KEY_MAX_LEN) + "] %}"
    "{% set summary = (summary|string)[:" + std::to_string(TODO_RESPONSE_SUMMARY_MAX_LEN) + "] %}"
    "{% set line = esc(key) ~ '|' ~ esc(summary) %}"
    "{% set next_len = (ns.out|length) + (1 if ns.out else 0) + (line|length) %}"
    "{% if next_len <= " + std::to_string(TODO_RESPONSE_TEXT_MAX_LEN) + " %}"
    "{% set ns.out = ns.out ~ ('\\n' if ns.out else '') ~ line %}"
    "{% set ns.shown = ns.shown + 1 %}"
    "{% endif %}"
    "{% endif %}"
    "{% set ns.count = ns.count + 1 %}"
    "{% endif %}"
    "{% endfor %}"
    "{{ ns.out }}{% if ns.count > ns.shown %}"
    "{% set more = '__MORE__|' ~ (ns.count - ns.shown) %}"
    "{% set next_len = (ns.out|length) + (1 if ns.out else 0) + (more|length) %}"
    "{% if next_len <= " + std::to_string(TODO_RESPONSE_TEXT_MAX_LEN) + " %}"
    "{{ '\\n' if ns.out else '' }}{{ more }}"
    "{% endif %}{% endif %}";
}

inline uint32_t next_todo_items_call_id() {
  static uint32_t call_id = 300000;
  return call_id++;
}

inline bool todo_begin_get_items_request(esphome::api::HomeassistantActionRequest &req,
                                         TodoCardCtx *ctx,
                                         uint32_t call_id,
                                         const std::string &response_template) {
  if (!todo_card_context_valid(ctx) || !todo_entity_id_safe(ctx->entity_id)) return false;
  if (!ha_action_begin(req, "todo.get_items", false, 1, call_id)) return false;
  req.wants_response = true;
  req.response_template = decltype(req.response_template)(response_template);
  ha_action_add_entity(req, ctx->entity_id);
  return true;
}

inline void request_todo_items(TodoCardCtx *ctx) {
  if (!todo_card_context_valid(ctx) || !todo_entity_id_safe(ctx->entity_id)) return;
  TodoModalUi &ui = todo_modal_ui();
  todo_modal_clear_items();
  todo_modal_set_status("Loading");
  todo_cancel_stale_request();
  if (todo_request_state().call_id != 0) {
    ESP_LOGI("todo", "Todo request already pending for %s",
      ctx->entity_id.c_str());
    return;
  }
  if (!ha_api_state_connected()) {
    ESP_LOGI("todo", "Waiting for Home Assistant before loading %s",
      ctx->entity_id.c_str());
    ui.waiting_for_ha = true;
    todo_modal_set_status("Waiting for Home Assistant");
    return;
  }
  ui.waiting_for_ha = false;

  esphome::api::HomeassistantActionRequest req;
  uint32_t call_id = next_todo_items_call_id();
  std::string response_template = todo_items_response_template(ctx->entity_id);
  if (!todo_begin_get_items_request(req, ctx, call_id, response_template)) {
    ESP_LOGW("todo", "Could not build todo request for %s", ctx->entity_id.c_str());
    todo_modal_set_status("Could not load");
    return;
  }

  if (!ha_register_action_response_callback(
    req.call_id,
    [ctx, call_id](const esphome::api::ActionResponse &response) {
      todo_clear_request_state(call_id);
      if (todo_modal_ui().active != ctx) return;
      if (!response.is_success()) {
        todo_modal_ui().waiting_for_ha = false;
        ESP_LOGW("todo", "Todo request failed for %s: %s",
          ctx && !ctx->entity_id.empty() ? ctx->entity_id.c_str() : "todo",
          response.get_error_message().c_str());
        todo_modal_set_status("Could not load");
        return;
      }
      auto json = response.get_json();
      std::vector<TodoItem> items;
      if (!parse_todo_response_json(json, ctx ? ctx->entity_id : "", items)) {
        todo_modal_ui().waiting_for_ha = false;
        ESP_LOGW("todo", "Could not parse todo response %u for %s",
          (unsigned) call_id,
          ctx && !ctx->entity_id.empty() ? ctx->entity_id.c_str() : "todo");
        todo_modal_set_status("Could not load");
        return;
      }
      ESP_LOGI("todo", "Todo request %u loaded %u rows for %s",
        (unsigned) call_id, (unsigned) items.size(),
        ctx && !ctx->entity_id.empty() ? ctx->entity_id.c_str() : "todo");
      todo_modal_ui().waiting_for_ha = false;
      todo_modal_render_items(ctx, items);
    })) {
    ESP_LOGW("todo", "Could not register todo response callback %u for %s",
      (unsigned) req.call_id, ctx->entity_id.c_str());
    todo_modal_set_status("Could not load");
    return;
  }

  TodoRequestState &state = todo_request_state();
  state.call_id = req.call_id;
  state.started_ms = esphome::millis();
  ESP_LOGI("todo", "Sending todo request %u for %s",
    (unsigned) req.call_id, ctx->entity_id.c_str());
  if (!ha_action_send(req)) {
    todo_cancel_request(req.call_id, "send failed");
    ui.waiting_for_ha = false;
    todo_modal_set_status("Could not load");
  }
}

inline void todo_reload_active_modal() {
  TodoModalUi &ui = todo_modal_ui();
  if (!todo_card_context_valid(ui.active) || ui.list == nullptr) return;
  if (todo_request_state().call_id != 0 || !ha_api_state_connected()) return;
  ESP_LOGI("todo", "Reloading open todo modal after Home Assistant reconnect");
  request_todo_items(ui.active);
}

inline void todo_retry_waiting_modal() {
  TodoModalUi &ui = todo_modal_ui();
  if (!ui.waiting_for_ha) return;
  todo_reload_active_modal();
}

inline void todo_card_open_modal(TodoCardCtx *ctx) {
  if (!todo_card_context_valid(ctx) || ctx->entity_id.empty()) return;
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
  lv_coord_t gap = control_modal_scaled_px(
    control_modal_uses_compact_portrait_tuning(layout) ? 28 : 12,
    layout.short_side);
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

  lv_coord_t row_gap = control_modal_scaled_px(
    control_modal_uses_compact_portrait_tuning(layout) ? 18 : 10,
    layout.short_side);
  if (row_gap < 8) row_gap = 8;
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
  (void) accent_color;
  (void) top_task_limit_two_lines;
  TodoCardCtx *ctx = new TodoCardCtx();
  ctx->entity_id = p.entity;
  ctx->configured_label = p.label;
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->value_lbl = s.sensor_lbl;
  ctx->unit_lbl = s.unit_lbl;
  ctx->label_lbl = s.text_lbl;
  ctx->secondary_color = secondary_color;
  ctx->label_font = label_font;
  ctx->list_font = list_font ? list_font : label_font;
  ctx->icon_font = icon_font;
  ctx->width_compensation_percent = width_compensation_percent;
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
      apply_control_availability(ctx->btn, ctx->btn, ctx->available, false);
      todo_apply_card_text(ctx);
      if (todo_modal_ui().active == ctx && !ctx->available) {
        todo_modal_ui().waiting_for_ha = true;
        todo_modal_set_status("Waiting for Home Assistant");
      }
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
