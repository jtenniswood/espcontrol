#pragma once

// Include after the common shell/text-button helpers. No legacy confirmation
// UI, button pointer, global ParsedCfg or configured-card context is borrowed.
#include <array>
#include <cassert>
#include <new>

#include "page_confirmation_runtime.h"
#include "page_lvgl_dispatch.h"
#include "page_modal_gestures.h"

namespace espcontrol::pages {
struct PageConfirmationModalEnvironment {
  const lv_font_t *button_font{nullptr}, *message_font{nullptr}, *icon_font{nullptr};
  uint32_t accent{0};
  // View into immutable config or graph state, copied before returning to LVGL.
  std::string_view caption;
  bool same_style(const PageConfirmationModalEnvironment& b) const {
    return button_font == b.button_font && message_font == b.message_font && icon_font == b.icon_font &&
           accent == b.accent;
  }
};
struct PageConfirmationModalStyle {
  void* owner{nullptr};
  bool (*resolve)(void*, PageModalToken, const ParsedCfg&, PageConfirmationModalEnvironment&){nullptr};
};
class PageConfirmationModalPort final : public PageModalPort {
 public:
  using OriginValid = bool (*)(void*, PageBindingToken);
  PageConfirmationModalPort(PageArenaAllocator& allocator, PageEntityStateRuntime& graph,
                            PageConfirmationModalStyle style, void* owner, OriginValid valid)
      : allocator_(allocator), graph_(graph), style_(style), owner_(owner), valid_(valid) {}
  ~PageConfirmationModalPort() override {
    assert(!record_);
    if (installed_ == this) installed_ = nullptr;
  }
  PageConfirmationModalPort(const PageConfirmationModalPort&) = delete;
  PageConfirmationModalPort& operator=(const PageConfirmationModalPort&) = delete;
  bool install(PageModalRuntime& runtime, PageConfirmationRuntime& commands, bool legacy_renderer_retired) {
    if (!legacy_renderer_retired || !lv_is_initialized() || record_ || !style_.resolve || !valid_ ||
        (installed_ && installed_ != this))
      return false;
    installed_ = this;
    runtime_ = &runtime;
    commands_ = &commands;
    return true;
  }
  bool valid_origin(PageBindingToken token) const override {
    return valid_ && valid_(owner_, token);
  }
  ControlModalKind kind(const PageStore& store, ConfigurationHandle card) const override {
    return runtime_ && installed_ == this && page_confirmation_policy(store, card).enabled()
               ? ControlModalKind::SWITCH_CONFIRMATION
               : ControlModalKind::NONE;
  }
  bool ready() const override {
    return lv_is_initialized() && !modal_callback_dispatching() && !ha_read_coordinator().dispatching() &&
           !page_lvgl_dispatching();
  }
  bool connected() const override {
    return ha_api_state_connected();
  }
  uint32_t connection_generation() const override {
    return graph_.connection_generation();
  }
  void* create(const PageStore& store, ConfigurationHandle card, PageModalToken token) override {
    if (record_ || kind(store, card) != ControlModalKind::SWITCH_CONFIRMATION ||
        commands_->plan(token).action == PageConfirmationAction::NONE ||
        !ha_internal_heap_available("confirmation setup")) {
      if (commands_) commands_->cancel(token);
      return nullptr;
    }
    auto* memory = allocator_.allocate(sizeof(Record));
    if (!memory) {
      commands_->cancel(token);
      return nullptr;
    }
    auto* r = new (memory) Record(*this, token);
    if (!r->gestures.available()) {
      release(r);
      return nullptr;
    }
    const auto* row = store.card(card);
    auto& p = r->config;
    p.entity = store.field(*row, CardField::ENTITY);
    p.label = store.field(*row, CardField::LABEL);
    p.icon = store.field(*row, CardField::ICON);
    p.icon_on = store.field(*row, CardField::ICON_ON);
    p.sensor = store.field(*row, CardField::SENSOR);
    p.unit = store.field(*row, CardField::UNIT);
    p.type = store.field(*row, CardField::TYPE);
    p.precision = store.field(*row, CardField::PRECISION);
    p.options = store.field(*row, CardField::OPTIONS);
    if (!style_.resolve(style_.owner, token, p, r->environment) || !fonts(r->environment)) {
      release(r);
      return nullptr;
    }
    r->caption = r->environment.caption;
    r->environment.caption = {};
    r->message = switch_confirmation_message(p);
    r->yes_text = switch_confirmation_yes_text(p);
    r->no_text = switch_confirmation_no_text(p);
    record_ = r;
    return r;
  }
  bool initialize(void* pointer, PageModalToken token) override {
    auto& r = *static_cast<Record*>(pointer);
    if (control_modal_active().kind != ControlModalKind::NONE || !commands_->bind(token, r.config))
      return false;
    auto shell = control_modal_open_shell(ControlModalKind::SWITCH_CONFIRMATION, nullptr, 100,
                                          r.environment.icon_font, close);
    r.overlay = shell.overlay;
    r.panel = shell.panel;
    r.back = shell.close_btn;
    if (!r.overlay || !r.panel || !r.back) return false;
    r.message_label = lv_label_create(r.panel);
    if (!r.message_label) return false;
    lv_obj_clear_flag(r.message_label, LV_OBJ_FLAG_CLICKABLE);
    lv_label_set_display_text(r.message_label, r.message.c_str());
    lv_label_set_long_mode(r.message_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(r.message_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    const auto size = button_size(shell.layout, shell.content_w);
    r.no = control_modal_create_text_button(r.panel, r.no_text, size[0], size[1], size[2], size[2] / 2,
                                            theme_display_color(current_theme().border),
                                            r.environment.button_font);
    r.yes = control_modal_create_text_button(r.panel, r.yes_text, size[0], size[1], size[2], size[2] / 2,
                                             r.environment.accent, r.environment.button_font);
    if (!r.no || !r.yes || !r.gestures.attach(r.overlay) ||
        !lv_obj_add_event_cb(r.no, no, LV_EVENT_CLICKED, &r) ||
        !lv_obj_add_event_cb(r.yes, yes, LV_EVENT_ALL, &r))
      return false;
    if (!current(r)) return false;
    update(pointer, token);
    if (!current(r)) return false;
    lv_obj_move_foreground(r.overlay);
    return visible(pointer);
  }
  bool visible(void* pointer) const override {
    const auto& r = *static_cast<Record*>(pointer);
    return !r.retired && r.overlay && r.panel && control_modal_active().overlay == r.overlay;
  }
  void update(void* pointer, PageModalToken) override {
    auto& r = *static_cast<Record*>(pointer);
    if (!current(r)) return;
    commands_->loop();
    if (!current(r)) return;
    PageConfirmationModalEnvironment e;
    if (!style_.resolve(style_.owner, r.token, r.config, e) || !fonts(e)) {
      runtime_->notify_closed(r.token, PageModalClose::FAILED);
      return;
    }
    if (!current(r)) return;
    if (!r.seeded || r.caption != e.caption) {
      r.caption = e.caption;
      set_clock_bar_modal_label(r.caption);
    }
    e.caption = {};
    const auto layout = control_modal_calc_layout(100);
    int content = layout.panel_w - 2 * layout.inset;
    if (content < 120) content = layout.panel_w;
    const auto radius = lv_obj_get_style_radius(r.panel, LV_PART_MAIN);
    const std::array<int64_t, 13> geometry = {layout.sw,
                                              layout.sh,
                                              layout.panel_x,
                                              layout.panel_y,
                                              layout.panel_w,
                                              layout.panel_h,
                                              layout.inset,
                                              layout.back_inset_x,
                                              layout.back_inset_y,
                                              layout.back_size,
                                              layout.short_side,
                                              content,
                                              radius};
    const std::array<uint32_t, 4> palette = {theme_display_color(current_theme().surface_secondary),
                                             theme_display_color(current_theme().border),
                                             current_theme().text_primary, e.accent};
    if (r.seeded && geometry == r.geometry && palette == r.palette && e.same_style(r.environment)) return;
    r.geometry = geometry;
    r.palette = palette;
    r.environment = e;
    r.seeded = true;
    control_modal_apply_panel_layout(r.overlay, r.panel, layout, radius);
    control_modal_apply_back_button_layout(r.back, layout);
    if (auto* icon = control_modal_icon_label(r.back)) {
      lv_obj_set_style_text_font(icon, e.icon_font, LV_PART_MAIN);
      lv_obj_set_style_text_color(icon, lv_color_hex(palette[2]), LV_PART_MAIN);
    }
    lv_obj_set_style_bg_color(r.panel, lv_color_hex(palette[0]), LV_PART_MAIN);
    lv_obj_set_style_text_font(r.message_label, e.message_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(r.message_label, lv_color_hex(palette[2]), LV_PART_MAIN);
    lv_obj_set_width(r.message_label, content);
    const auto size = button_size(layout, content);
    for (auto* button : {r.no, r.yes}) {
      auto* label = lv_obj_get_child(button, 0);
      lv_obj_set_style_text_font(label, e.button_font, LV_PART_MAIN);
      lv_obj_set_style_radius(button, size[2] / 2, LV_PART_MAIN);
      control_modal_layout_text_button(button, label, size[0], size[1], size[2]);
    }
    lv_obj_set_style_bg_color(r.no, lv_color_hex(palette[1]), LV_PART_MAIN);
    lv_obj_set_style_bg_color(r.yes, lv_color_hex(e.accent), LV_PART_MAIN);
    lv_obj_set_style_text_color(lv_obj_get_child(r.no, 0), lv_color_hex(palette[2]), LV_PART_MAIN);
    lv_obj_set_style_text_color(lv_obj_get_child(r.yes, 0),
                                lv_color_hex(readable_text_color_for_bg(e.accent)), LV_PART_MAIN);
    arrange(r, layout, size[3], size[4]);
  }
  ControlModalDismissPolicy dismiss_policy(void*) const override {
    return ControlModalDismissPolicy::DISMISS;
  }
  void retire(void* pointer, PageModalClose) override {
    auto& r = *static_cast<Record*>(pointer);
    r.retired = true;
    r.gestures.invalidate();
    commands_->cancel(r.token);
    if (r.overlay && ready()) lv_obj_add_flag(r.overlay, LV_OBJ_FLAG_HIDDEN);
    if (r.overlay && control_modal_active().overlay == r.overlay) control_modal_service().reset_active();
  }
  void destroy(void* pointer) override {
    auto* r = static_cast<Record*>(pointer);
    assert(r == record_ && r->retired);
    if (r->overlay) lv_obj_delete(r->overlay);
    assert(!r->gestures.live());
    record_ = nullptr;
    set_clock_bar_modal_label("");
    release(r);
  }

 private:
  struct Record {
    Record(PageConfirmationModalPort& p, PageModalToken t)
        : port(p), token(t), gestures(p.allocator_, 8, t, &p, gesture_valid) {}
    PageConfirmationModalPort& port;
    PageModalToken token;
    PageModalGestures gestures;
    ParsedCfg config;
    std::string caption, message, yes_text, no_text;
    PageConfirmationModalEnvironment environment;
    std::array<int64_t, 13> geometry{};
    std::array<uint32_t, 4> palette{};
    lv_obj_t *overlay{nullptr}, *panel{nullptr}, *back{nullptr}, *message_label{nullptr}, *no{nullptr},
        *yes{nullptr};
    uint32_t pressed_connection{0};
    bool seeded{false}, retired{false};
  };
  static bool fonts(const PageConfirmationModalEnvironment& e) {
    return e.button_font && e.message_font && e.icon_font;
  }
  bool current(const Record& r) const {
    return !r.retired && runtime_->valid(r.token);
  }
  static bool gesture_valid(void* owner, PageModalToken token) {
    auto& p = *static_cast<PageConfirmationModalPort*>(owner);
    return p.record_ && !p.record_->retired && p.runtime_->interactive(token);
  }
  static void close() {
    auto* p = installed_;
    if (p && p->record_ && p->runtime_->interactive(p->record_->token))
      p->runtime_->notify_closed(p->record_->token);
  }
  static void no(lv_event_t* event) {
    auto& r = *static_cast<Record*>(lv_event_get_user_data(event));
    if (r.port.current(r) && r.port.runtime_->interactive(r.token)) r.port.runtime_->notify_closed(r.token);
  }
  static void yes(lv_event_t* event) {
    auto& r = *static_cast<Record*>(lv_event_get_user_data(event));
    if (!r.port.current(r) || !r.port.runtime_->interactive(r.token)) return;
    if (lv_event_get_code(event) == LV_EVENT_PRESSED)
      r.pressed_connection = r.port.graph_.connection_generation();
    else if (lv_event_get_code(event) == LV_EVENT_CLICKED)
      r.port.commands_->queue(r.token, r.pressed_connection);
  }
  static std::array<int, 5> button_size(const ControlModalLayout& l, int content) {
    const int gap = std::max(8, static_cast<int>(control_modal_scaled_px(12, l.short_side)));
    const int message_gap = std::max(18, static_cast<int>(control_modal_scaled_px(28, l.short_side)));
    const int height = std::max(36, static_cast<int>(control_modal_scaled_px(52, l.short_side)));
    int maximum = (content - gap) / 2;
    if (maximum < 48) maximum = content;
    const int minimum =
        std::min(maximum, std::max(56, height + static_cast<int>(control_modal_scaled_px(16, l.short_side))));
    return {maximum, minimum, height, gap, message_gap};
  }
  static void arrange(Record& r, const ControlModalLayout& l, int gap, int message_gap) {
    lv_obj_update_layout(r.message_label);
    lv_obj_update_layout(r.no);
    lv_obj_update_layout(r.yes);
    const int message_h = lv_obj_get_height(r.message_label), no_w = lv_obj_get_width(r.no),
              yes_w = lv_obj_get_width(r.yes);
    const int action_h = std::max(lv_obj_get_height(r.no), lv_obj_get_height(r.yes));
    const int group_h = message_h + message_gap + action_h;
    int top = (l.panel_h - group_h) / 2;
    const int top_limit = l.inset + l.back_size + message_gap;
    if (top < top_limit) top = top_limit;
    if (top + group_h > l.panel_h - l.inset) top = l.panel_h - l.inset - group_h;
    if (top < l.inset) top = l.inset;
    const int action_y = top + message_h + message_gap + action_h / 2 - l.panel_h / 2;
    const int width = no_w + gap + yes_w;
    lv_obj_align(r.message_label, LV_ALIGN_CENTER, 0, top + message_h / 2 - l.panel_h / 2);
    lv_obj_align(r.no, LV_ALIGN_CENTER, -width / 2 + no_w / 2, action_y);
    lv_obj_align(r.yes, LV_ALIGN_CENTER, width / 2 - yes_w / 2, action_y);
  }
  void release(Record* r) {
    commands_->cancel(r->token);
    r->~Record();
    allocator_.release(r, sizeof(Record));
  }
  PageArenaAllocator& allocator_;
  PageEntityStateRuntime& graph_;
  PageConfirmationModalStyle style_;
  void* owner_;
  OriginValid valid_;
  PageModalRuntime* runtime_{nullptr};
  PageConfirmationRuntime* commands_{nullptr};
  Record* record_{nullptr};
  inline static PageConfirmationModalPort* installed_{nullptr};
};
}  // namespace espcontrol::pages
