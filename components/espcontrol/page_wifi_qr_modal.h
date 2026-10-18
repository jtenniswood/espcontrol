#pragma once

// Include after common config, HA, control-modal shell and power-layout helpers.
// No legacy WifiQrModalUi, callbacks, QR canvas or guest timer is used.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <new>

#include "page_guest_wifi_runtime.h"
#include "page_lvgl_dispatch.h"
#include "page_lvgl_qr_frames.h"
#include "page_modal_gestures.h"
#include "page_modal_qr_runtime.h"
#include "wifi_qr_codec.h"

namespace espcontrol::pages {
struct PageWifiQrModalEnvironment {
  const lv_font_t *label_font{nullptr}, *heading_font{nullptr}, *icon_font{nullptr};
  int width_compensation_percent{100};
};
struct PageWifiQrModalStyle {
  PageWifiQrModalEnvironment environment;
  void* owner{nullptr};
  // Source identity is available during preflight, before the opening is interactive.
  bool (*resolve)(void*, PageModalToken, const ParsedCfg&, PageWifiQrModalEnvironment&){nullptr};
};
class PageWifiQrModalPort final : public PageModalPort {
 public:
  using OriginValid = bool (*)(void*, PageBindingToken);
  PageWifiQrModalPort(PageArenaAllocator& allocator, PageEntityStateRuntime& graph, PageQrEncoder& encoder,
                      PageGuestWifiRuntime& guest, PageWifiQrModalStyle style, void* owner, OriginValid valid)
      : allocator_(allocator),
        graph_(graph),
        encoder_(encoder),
        guest_(guest),
        style_(style),
        owner_(owner),
        valid_(valid) {}
  ~PageWifiQrModalPort() override {
    assert(!record_);
    if (installed_ == this) installed_ = nullptr;
  }
  PageWifiQrModalPort(const PageWifiQrModalPort&) = delete;
  PageWifiQrModalPort& operator=(const PageWifiQrModalPort&) = delete;
  bool install(PageModalRuntime& runtime, bool legacy_renderer_retired) {
    if (!legacy_renderer_retired || !lv_is_initialized() || record_ || (installed_ && installed_ != this))
      return false;
    runtime_ = &runtime;
    installed_ = this;
    return true;
  }
  bool valid_origin(PageBindingToken token) const override {
    return valid_ && valid_(owner_, token);
  }
  ControlModalKind kind(const PageStore& store, ConfigurationHandle card) const override {
    const auto* row = store.card(card);
    if (!runtime_ || installed_ != this || !style_.resolve || !row) return ControlModalKind::NONE;
    const auto type = store.field(*row, CardField::TYPE);
    return type == "wifi_qr" || type == "wifi_qr_card" ? ControlModalKind::WIFI_QR : ControlModalKind::NONE;
  }
  bool ready() const override {
    return lv_is_initialized() && !modal_callback_dispatching() && !ha_read_coordinator().dispatching() &&
           !page_lvgl_dispatching() && (!record_ || !record_->retired || record_->frames.idle());
  }
  bool connected() const override {
    return true;
  }
  uint32_t connection_generation() const override {
    return 0;
  }
  void* create(const PageStore& store, ConfigurationHandle card, PageModalToken token) override {
    if (record_ || kind(store, card) != ControlModalKind::WIFI_QR ||
        !ha_internal_heap_available("Wi-Fi sharing setup"))
      return nullptr;
    void* memory = allocator_.allocate(sizeof(Record));
    if (!memory) return nullptr;
    auto* r = new (memory) Record(*this, token);
    const auto fail = [&]() -> void* {
      release(r);
      return nullptr;
    };
    if (!r->gestures.available()) return fail();
    const auto* row = store.card(card);
    auto& c = r->config;
    c.entity = std::string(store.field(*row, CardField::ENTITY));
    c.label = std::string(store.field(*row, CardField::LABEL));
    c.icon = std::string(store.field(*row, CardField::ICON));
    c.icon_on = std::string(store.field(*row, CardField::ICON_ON));
    c.sensor = std::string(store.field(*row, CardField::SENSOR));
    c.unit = std::string(store.field(*row, CardField::UNIT));
    c.precision = std::string(store.field(*row, CardField::PRECISION));
    c.type = std::string(store.field(*row, CardField::TYPE));
    c.options = std::string(store.field(*row, CardField::OPTIONS));
    if (!wifi_qr_build_payload(cfg_option_value(c.options, "ssid64"), cfg_option_value(c.options, "security"),
                               cfg_option_value(c.options, "pass64"),
                               cfg_option_token_present(c.options, "hidden"), &r->payload, &r->ssid,
                               &r->password))
      return fail();
    for (const auto& tab : wifi_qr_tabs(cfg_option_value(c.options, "wifi_tabs"))) {
      r->tabs[r->count++] = tab == "guest" ? Tab::GUEST : tab == "credentials" ? Tab::DETAILS : Tab::QR;
    }
    r->selected = r->tabs[0];
    r->environment = style_.environment;
    if (!style_.resolve(style_.owner, token, c, r->environment) || !fonts(r->environment)) return fail();
    r->glyphs = {find_icon("Wifi QR Tab"), find_icon("Wifi Password Tab"), find_icon("Wifi"),
                 find_icon("Wifi Off")};
    for (const auto* glyph : r->glyphs)
      if (!glyph) return fail();
    if (r->has(Tab::QR) && !r->qr.initialize()) return fail();
    record_ = r;
    return r;
  }
  bool initialize(void* pointer, PageModalToken token) override {
    auto& r = *static_cast<Record*>(pointer);
    if (control_modal_active().kind != ControlModalKind::NONE) return false;
    auto shell =
        control_modal_open_shell(ControlModalKind::WIFI_QR, nullptr, 100, r.environment.icon_font, close);
    r.overlay = shell.overlay;
    r.panel = shell.panel;
    r.back = shell.close_btn;
    if (!r.overlay || !r.panel || !r.back) return false;
    control_modal_block_close_for(350);
    control_modal_style_translucent_chrome_button(r.back);
    r.tab_row = control_modal_create_tab_row(r.panel);
    if (!r.tab_row) return false;
    for (size_t i = 0; i < r.count; ++i) {
      const auto tab = r.tabs[i];
      r.tab_parameters[i] = {&r, tab};
      auto* button = control_modal_create_flat_icon_button(
          r.tab_row, r.glyphs[static_cast<size_t>(tab)], r.environment.icon_font,
          theme_display_color(current_theme().surface_primary), LV_OPA_TRANSP);
      if (!button || !lv_obj_add_event_cb(button, select_tab, LV_EVENT_CLICKED, &r.tab_parameters[i]))
        return false;
      r.buttons[static_cast<size_t>(tab)] = button;
      r.views[static_cast<size_t>(tab)] = view(r.panel);
      if (!r.views[static_cast<size_t>(tab)]) return false;
    }
    if (r.has(Tab::QR)) {
      r.surface = view(r.views[0]);
      if (!r.surface) return false;
      lv_obj_set_style_bg_color(r.surface, lv_color_white(), LV_PART_MAIN);
      lv_obj_set_style_bg_opa(r.surface, LV_OPA_COVER, LV_PART_MAIN);
      if (!lv_obj_add_event_cb(r.surface, draw, LV_EVENT_DRAW_MAIN, &r)) return false;
      r.error = detail(r.views[0], espcontrol_i18n_key("unable_to_create_qr_code"), r.environment.label_font);
      if (!r.error) return false;
      lv_obj_center(r.error);
      if (!r.qr.bind(0, token, r.payload)) return false;
    }
    if (r.has(Tab::DETAILS)) {
      auto* details = r.views[1];
      lv_obj_set_flex_flow(details, LV_FLEX_FLOW_COLUMN);
      lv_obj_set_style_flex_main_place(details, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
      lv_obj_set_style_flex_cross_place(details, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
      r.network_title = detail(details, espcontrol_i18n_key("network"), r.environment.heading_font);
      r.network = detail(details, r.ssid.c_str(), r.environment.heading_font);
      r.spacer = view(details);
      if (!r.spacer) return false;
      r.password_title = detail(details, espcontrol_i18n_key("password"), r.environment.heading_font);
      r.password_label =
          detail(details, r.password.empty() ? espcontrol_i18n_key("none") : r.password.c_str(),
                 r.environment.heading_font);
      if (!r.network_title || !r.network || !r.password_title || !r.password_label) return false;
    }
    if (r.has(Tab::GUEST)) {
      r.group = view(r.views[2]);
      if (!r.group) return false;
      lv_obj_set_style_bg_opa(r.group, LV_OPA_COVER, LV_PART_MAIN);
      r.on = control_modal_create_flat_icon_button(r.group, r.glyphs[2], r.environment.icon_font, 0,
                                                   LV_OPA_TRANSP);
      r.off = control_modal_create_flat_icon_button(r.group, r.glyphs[3], r.environment.icon_font, 0,
                                                    LV_OPA_TRANSP);
      if (!r.on || !r.off) return false;
      for (auto* button : {r.on, r.off}) lv_obj_clear_flag(button, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_flag(r.group, LV_OBJ_FLAG_CLICKABLE);
      control_modal_apply_pressed_fill(r.group);
      if (!lv_obj_add_event_cb(r.group, guest_event, LV_EVENT_ALL, &r)) return false;
      r.guest_bound = true;
      if (!guest_.bind(token, r.config.entity)) r.guest_bound = false;
    }
    if (!r.gestures.attach(r.overlay) || r.retired || !runtime_->valid(token)) return false;
    update(&r, token);
    return visible(&r);
  }
  bool visible(void* pointer) const override {
    const auto& r = *static_cast<Record*>(pointer);
    return !r.retired && r.overlay && r.panel && control_modal_active().overlay == r.overlay;
  }
  void update(void* pointer, PageModalToken) override;
  ControlModalDismissPolicy dismiss_policy(void*) const override {
    return ControlModalDismissPolicy::DISMISS;
  }
  void retire(void* pointer, PageModalClose) override {
    auto& r = *static_cast<Record*>(pointer);
    r.retired = true;
    r.gestures.invalidate();
    r.qr.retire();
    if (r.guest_bound) {
      r.guest_bound = false;
      guest_.unbind(r.token);
    }
    if (r.overlay && ready()) lv_obj_add_flag(r.overlay, LV_OBJ_FLAG_HIDDEN);
    if (control_modal_active().overlay == r.overlay) control_modal_service().reset_active();
  }
  void destroy(void* pointer) override {
    auto* r = static_cast<Record*>(pointer);
    assert(r == record_ && r->retired);
    const bool stopped = r->qr.shutdown();
    assert(stopped);
    (void)stopped;
    if (r->overlay) lv_obj_delete(r->overlay);
    assert(r->gestures.live() == 0);
    record_ = nullptr;
    set_clock_bar_modal_label("");
    release(r);
  }

 private:
  enum class Tab : uint8_t { QR, DETAILS, GUEST };
  struct Record;
  struct TabParameter {
    Record* record{nullptr};
    Tab tab{Tab::QR};
  };
  struct QrOrigin final : PageModalQrOrigin {
    PageWifiQrModalPort& port;
    explicit QrOrigin(PageWifiQrModalPort& p) : port(p) {}
    bool valid(PageModalToken token) const override {
      return port.admitted(token);
    }
  };
  struct Paint {
    std::array<int64_t, 25> geometry{};
    std::array<uint32_t, 5> palette{};
    PageWifiQrModalEnvironment environment;
    uint64_t qr_asset{0}, guest_revision{0};
    Tab tab{Tab::QR};
    bool qr_failed{false}, seeded{false};
  };
  struct Record {
    Record(PageWifiQrModalPort& p, PageModalToken t)
        : port(p),
          token(t),
          gestures(p.allocator_, 16, t, &p, gesture_valid),
          origin(p),
          frames(p.allocator_),
          qr(p.allocator_, p.encoder_, frames, origin, 1) {}
    bool has(Tab tab) const {
      for (size_t i = 0; i < count; ++i)
        if (tabs[i] == tab) return true;
      return false;
    }
    PageWifiQrModalPort& port;
    PageModalToken token;
    ParsedCfg config;
    std::string payload, ssid, password, caption;
    std::array<Tab, 3> tabs{};
    std::array<TabParameter, 3> tab_parameters{};
    std::array<lv_obj_t*, 3> buttons{}, views{};
    std::array<const char*, 4> glyphs{};
    std::array<const char*, 4> translated{};
    PageWifiQrModalEnvironment environment;
    Paint paint;
    PageModalGestures gestures;
    QrOrigin origin;
    PageLvglQrFrames frames;
    PageModalQrRuntime qr;
    lv_obj_t *overlay{nullptr}, *panel{nullptr}, *back{nullptr}, *tab_row{nullptr}, *surface{nullptr},
        *error{nullptr}, *network_title{nullptr}, *network{nullptr}, *spacer{nullptr},
        *password_title{nullptr}, *password_label{nullptr}, *group{nullptr}, *on{nullptr}, *off{nullptr};
    uint32_t pressed_connection{0};
    size_t count{0};
    Tab selected{Tab::QR};
    bool retired{false}, guest_bound{false};
  };
  static bool fonts(const PageWifiQrModalEnvironment& e) {
    return e.label_font && e.heading_font && e.icon_font;
  }
  bool admitted(PageModalToken token) const {
    return record_ && !record_->retired && runtime_ && runtime_->valid(token);
  }
  static bool gesture_valid(void* owner, PageModalToken token) {
    auto& port = *static_cast<PageWifiQrModalPort*>(owner);
    return port.admitted(token) && port.runtime_->interactive(token);
  }
  static void close() {
    auto* p = installed_;
    if (p && p->record_ && p->runtime_->interactive(p->record_->token))
      p->runtime_->notify_closed(p->record_->token);
  }
  static void select_tab(lv_event_t* event) {
    const auto& parameter = *static_cast<TabParameter*>(lv_event_get_user_data(event));
    auto& r = *parameter.record;
    if (!r.retired && r.port.runtime_->interactive(r.token)) r.selected = parameter.tab;
  }
  static void guest_event(lv_event_t* event) {
    auto& r = *static_cast<Record*>(lv_event_get_user_data(event));
    if (r.retired || !r.guest_bound || !r.port.runtime_->interactive(r.token)) return;
    const auto code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED)
      r.pressed_connection = r.port.graph_.connection_generation();
    else if (code == LV_EVENT_CLICKED)
      r.port.guest_.queue(r.token, r.pressed_connection);
  }
  static void draw(lv_event_t* event) {
    auto& r = *static_cast<Record*>(lv_event_get_user_data(event));
    if (r.retired || r.selected != Tab::QR || !r.port.runtime_->valid(r.token)) return;
    const auto state = r.qr.snapshot(0, r.token);
    if (state.ready && state.asset) r.frames.draw(0, r.surface, lv_event_get_layer(event));
  }
  static lv_obj_t* view(lv_obj_t* parent) {
    auto* object = lv_obj_create(parent);
    if (!object) return nullptr;
    lv_obj_set_style_bg_opa(object, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(object, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(object, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(object, 0, LV_PART_MAIN);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_CLICKABLE);
    return object;
  }
  static lv_obj_t* detail(lv_obj_t* parent, const char* text, const lv_font_t* font) {
    auto* label = lv_label_create(parent);
    if (!label) return nullptr;
    lv_label_set_display_text(label, text);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    return label;
  }
  static void show(lv_obj_t* object, bool visible) {
    if (!object) return;
    if (visible)
      lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
    else
      lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
  }
  static void caption(Record& r);
  static void tab_style(lv_obj_t* button, bool selected, const PageWifiQrModalEnvironment&);
  void release(Record* r) {
    r->~Record();
    allocator_.release(r, sizeof(Record));
  }
  PageArenaAllocator& allocator_;
  PageEntityStateRuntime& graph_;
  PageQrEncoder& encoder_;
  PageGuestWifiRuntime& guest_;
  PageWifiQrModalStyle style_;
  void* owner_;
  OriginValid valid_;
  PageModalRuntime* runtime_{nullptr};
  Record* record_{nullptr};
  inline static PageWifiQrModalPort* installed_{nullptr};
};
inline void PageWifiQrModalPort::caption(Record& r) {
  const char* title = r.config.label.empty() || r.config.label == "Connect"
                          ? espcontrol_i18n_key("label_connect")
                          : r.config.label.c_str();
  if (r.caption != title) {
    r.caption = title;
    set_clock_bar_modal_label(r.caption);
  }
  // Generated translations are immutable literals. Cache their identities,
  // since the display-text helper may normalize the rendered glyph bytes.
  const auto update = [&](size_t index, lv_obj_t* label, const char* text) {
    if (label && r.translated[index] != text) {
      r.translated[index] = text;
      lv_label_set_display_text(label, text);
    }
  };
  update(0, r.network_title, espcontrol_i18n_key("network"));
  update(1, r.password_title, espcontrol_i18n_key("password"));
  if (r.password.empty()) update(2, r.password_label, espcontrol_i18n_key("none"));
  update(3, r.error, espcontrol_i18n_key("unable_to_create_qr_code"));
}
inline void PageWifiQrModalPort::tab_style(lv_obj_t* button, bool selected,
                                           const PageWifiQrModalEnvironment& e) {
  if (!button) return;
  lv_obj_set_style_bg_color(button,
                            lv_color_hex(selected ? current_theme().text_primary
                                                  : theme_display_color(current_theme().surface_primary)),
                            LV_PART_MAIN);
  lv_obj_set_style_bg_opa(button, selected ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(button, 0, LV_PART_MAIN);
  auto* icon = control_modal_icon_label(button);
  if (icon) {
    lv_obj_set_style_text_color(icon,
                                lv_color_hex(selected ? theme_display_color(current_theme().surface_secondary)
                                                      : current_theme().text_primary),
                                LV_PART_MAIN);
    lv_obj_set_style_text_font(icon, e.icon_font, LV_PART_MAIN);
  }
}
inline void PageWifiQrModalPort::update(void* pointer, PageModalToken token) {
  auto& r = *static_cast<Record*>(pointer);
  if (r.retired || !runtime_->valid(token)) return;
  if (r.guest_bound) guest_.loop();
  if (r.retired || !runtime_->valid(token)) return;
  auto environment = style_.environment;
  if (!style_.resolve(style_.owner, token, r.config, environment) || !fonts(environment)) {
    runtime_->notify_closed(token, PageModalClose::FAILED);
    return;
  }
  if (r.retired || !runtime_->valid(token)) return;
  const auto layout = control_modal_calc_layout(environment.width_compensation_percent);
  const bool tab_bar = r.count > 1;
  const auto tabs = control_modal_calc_tab_layout(layout, r.count, tab_bar);
  const lv_coord_t safe_top = tab_bar ? 0 : layout.back_inset_y + layout.back_size + layout.inset;
  const auto content = control_modal_calc_content_layout(layout, tabs, tab_bar, 120, safe_top);
  const lv_coord_t qr_padding =
      std::max<lv_coord_t>(layout.inset, control_modal_scaled_px(24, layout.short_side));
  const lv_coord_t side =
      std::max<lv_coord_t>(120, std::min<lv_coord_t>(content.width, content.height) - qr_padding * 2);
  const lv_coord_t radius = control_modal_card_radius(nullptr);
  if (r.has(Tab::QR) && r.selected == Tab::QR) r.qr.size(0, token, side, false);
  r.qr.loop();
  if (r.retired || !runtime_->valid(token)) return;
  const auto qr = r.qr.snapshot(0, token);
  const auto guest = r.guest_bound ? guest_.snapshot(token) : PageGuestWifiSnapshot{};
  const std::array<int64_t, 25> geometry{
      layout.panel_x,   layout.panel_y,         layout.panel_w,      layout.panel_h,     layout.short_side,
      layout.inset,     layout.back_inset_x,    layout.back_inset_y, layout.back_size,   layout.title_gap,
      tabs.tab_size,    tabs.selected_tab_size, tabs.tab_frame_pad,  tabs.tab_gap,       tabs.tabs_total_w,
      tabs.tab_frame_w, tabs.tab_frame_h,       tabs.tab_safe_left,  tabs.centered_left, tabs.row_left,
      tabs.content_gap, content.width,          content.height,      content.top,        radius};
  const std::array<uint32_t, 5> palette{theme_display_color(current_theme().surface_primary),
                                        theme_display_color(current_theme().surface_secondary),
                                        theme_display_color(current_theme().overlay),
                                        current_theme().text_primary, current_theme().text_muted};
  caption(r);
  if (r.retired || !runtime_->valid(token)) return;
  const auto& old = r.paint;
  const auto guest_revision = r.guest_bound ? guest_.revision() : 0;
  const bool layout_changed =
      !old.seeded || old.geometry != geometry || old.palette != palette || old.tab != r.selected ||
      old.environment.label_font != environment.label_font ||
      old.environment.heading_font != environment.heading_font ||
      old.environment.icon_font != environment.icon_font ||
      old.environment.width_compensation_percent != environment.width_compensation_percent;
  const bool qr_changed = !old.seeded || old.qr_asset != qr.asset || old.qr_failed != qr.failed;
  const bool guest_changed = !old.seeded || old.guest_revision != guest_revision;
  if (!layout_changed && !qr_changed && !guest_changed) return;
  r.environment = environment;
  r.paint = {geometry, palette, environment, qr.asset, guest_revision, r.selected, qr.failed, true};
  if (layout_changed) {
    control_modal_apply_panel_layout(r.overlay, r.panel, layout, radius);
    control_modal_apply_back_button_layout(r.back, layout);
    if (auto* icon = control_modal_icon_label(r.back)) {
      lv_obj_set_style_text_font(icon, environment.icon_font, LV_PART_MAIN);
      lv_obj_set_style_text_color(icon, lv_color_hex(palette[3]), LV_PART_MAIN);
    }
    control_modal_apply_tab_row(r.tab_row, layout, tabs, environment.width_compensation_percent);
    lv_obj_set_style_bg_color(r.panel, lv_color_hex(palette[1]), LV_PART_MAIN);
    for (size_t i = 0; i < r.count; ++i) {
      const auto tab = r.tabs[i];
      auto* button = r.buttons[static_cast<size_t>(tab)];
      auto* object = r.views[static_cast<size_t>(tab)];
      tab_style(button, r.selected == tab, environment);
      control_modal_layout_tab_button(button, layout, tabs, i, r.selected == tab);
      lv_obj_set_size(object, content.width, content.height);
      lv_obj_align(object, LV_ALIGN_TOP_MID, 0, content.top);
      lv_obj_set_style_pad_left(object, layout.inset, LV_PART_MAIN);
      lv_obj_set_style_pad_right(object, layout.inset, LV_PART_MAIN);
      show(object, r.selected == tab);
    }
  }
  if (r.surface && (layout_changed || qr_changed)) {
    lv_obj_set_size(r.surface, side, side);
    lv_obj_center(r.surface);
    lv_obj_set_style_radius(r.surface, 0, LV_PART_MAIN);
    show(r.surface, !qr.failed);
    show(r.error, qr.failed);
    lv_obj_set_style_text_font(r.error, environment.label_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(r.error, lv_color_hex(palette[3]), LV_PART_MAIN);
    if (qr.ready) lv_obj_invalidate(r.surface);
  }
  if (layout_changed && r.has(Tab::DETAILS)) {
    lv_obj_set_style_pad_row(r.views[1], layout.title_gap, LV_PART_MAIN);
    lv_obj_set_size(r.spacer, 1, std::max<lv_coord_t>(16, layout.title_gap * 2));
    for (auto* label : {r.network_title, r.network, r.password_title, r.password_label}) {
      lv_obj_set_style_text_font(label, environment.heading_font, LV_PART_MAIN);
      lv_obj_set_style_text_color(
          label, lv_color_hex(label == r.network || label == r.password_label ? palette[3] : palette[4]),
          LV_PART_MAIN);
    }
  }
  if (r.group && (layout_changed || guest_changed)) {
    const lv_coord_t height = std::max<lv_coord_t>(
        136, std::min<lv_coord_t>(content.height, control_modal_scaled_px(300, layout.short_side)));
    const lv_coord_t width = std::min<lv_coord_t>(content.width - layout.inset * 2, height * 3 / 5);
    tab_style(r.on, guest.known && guest.on, environment);
    tab_style(r.off, guest.known && !guest.on, environment);
    if (layout_changed)
      light_control_layout_power(r.group, r.on, r.off, width, height, 0,
                                 environment.width_compensation_percent);
    lv_obj_set_style_bg_color(r.group, lv_color_hex(palette[0]), LV_PART_MAIN);
    lv_obj_set_style_opa(r.group, guest.known ? LV_OPA_COVER : LV_OPA_50, LV_PART_MAIN);
    if (guest.known && !guest.pending)
      lv_obj_clear_state(r.group, LV_STATE_DISABLED);
    else
      lv_obj_add_state(r.group, LV_STATE_DISABLED);

    auto* icon = control_modal_icon_label(r.buttons[2]);
    if (icon) {
      lv_label_set_display_text(icon, r.glyphs[guest.known && !guest.on ? 3 : 2]);
      control_modal_center_tab_icon(icon);
      lv_obj_set_style_opa(icon, guest.known ? LV_OPA_COVER : LV_OPA_50, LV_PART_MAIN);
    }
  }
  if (layout_changed) {
    if (r.back) lv_obj_move_foreground(r.back);
    if (r.tab_row) lv_obj_move_foreground(r.tab_row);
  }
}

}  // namespace espcontrol::pages
