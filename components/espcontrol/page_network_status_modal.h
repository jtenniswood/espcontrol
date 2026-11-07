#pragma once

// Include after the stateless grid-card, theme and text helpers. Settings owns
// only its demanded six cards; no legacy BtnSlot, timer or HA registration.
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <new>
#include <string>
#include <string_view>
#include <tuple>

#include "network_status_layout.h"
#include "network_status_text.h"
#include "page_lvgl_dispatch.h"
#include "page_modal_gestures.h"

namespace espcontrol::pages {
struct PageNetworkStatusSample {
  std::array<char, 64> ip{};
  float wifi_percent{NAN};
};
class PageNetworkStatusSource {
 public:
  virtual ~PageNetworkStatusSource() = default;
  // Identity is copied into the demanded context before constructing widgets.
  virtual std::string_view device_name() const = 0;
  virtual std::string_view firmware_version() const = 0;
  virtual bool wifi_available() const = 0;
  virtual bool has_pin() const = 0;
  virtual void sample(PageNetworkStatusSample&) = 0;
  virtual void activate_lock() = 0;
};
struct PageNetworkStatusEnvironment {
  const lv_font_t *label_font{nullptr}, *icon_font{nullptr};
  int width{0}, height{0}, top{0}, pad_bottom{0}, pad_left{0}, pad_right{0}, gap_row{0}, gap_column{0};
  int columns{1}, rows{1}, radius{8}, card_pad{8}, icon_compensation{100}, text_compensation{100};
  bool continuous_corners{false};
  bool operator==(const PageNetworkStatusEnvironment& b) const {
    return std::tie(label_font, icon_font, width, height, top, pad_bottom, pad_left, pad_right, gap_row,
                    gap_column, columns, rows, radius, card_pad, icon_compensation, text_compensation,
                    continuous_corners) ==
           std::tie(b.label_font, b.icon_font, b.width, b.height, b.top, b.pad_bottom, b.pad_left,
                    b.pad_right, b.gap_row, b.gap_column, b.columns, b.rows, b.radius, b.card_pad,
                    b.icon_compensation, b.text_compensation, b.continuous_corners);
  }
};
struct PageNetworkStatusStyle {
  void* owner{nullptr};
  bool (*resolve)(void*, PageLocalModalOrigin, PageModalToken, PageNetworkStatusEnvironment&){nullptr};
  // Scalar scene mode only: the scene recomputes its current underlying title.
  // Never save a borrowed old-page label or touch widgets from this callback.
  void (*settings)(void*, bool){nullptr};
};
class PageNetworkStatusModalPort final : public PageModalPort {
 public:
  using OriginValid = bool (*)(void*, PageLocalModalOrigin);
  PageNetworkStatusModalPort(PageArenaAllocator& allocator, PageNetworkStatusSource& source,
                             size_t profile_slots, PageNetworkStatusStyle style, void* owner,
                             OriginValid valid)
      : allocator_(allocator),
        source_(source),
        capacity_(std::max(profile_slots, size_t{NETWORK_STATUS_CARD_COUNT})),
        style_(style),
        owner_(owner),
        valid_(valid) {}
  ~PageNetworkStatusModalPort() override {
    assert(!record_);
    if (installed_ == this) installed_ = nullptr;
  }
  bool install(PageModalRuntime& runtime, bool legacy_renderer_retired) {
    if (!legacy_renderer_retired || !lv_is_initialized() || record_ || !style_.resolve || !valid_ ||
        capacity_ > 255 || (installed_ && installed_ != this))
      return false;
    installed_ = this;
    runtime_ = &runtime;
    return true;
  }
  bool valid_origin(PageBindingToken) const override {
    return false;
  }
  ControlModalKind kind(const PageStore&, ConfigurationHandle) const override {
    return ControlModalKind::NONE;
  }
  ControlModalKind local_kind(ControlModalKind requested) const override {
    return runtime_ && installed_ == this && requested == ControlModalKind::NETWORK_STATUS
               ? requested
               : ControlModalKind::NONE;
  }
  bool valid_local_origin(PageLocalModalOrigin origin, ControlModalKind requested) const override {
    return requested == ControlModalKind::NETWORK_STATUS && local_kind(requested) == requested &&
           origin.binding && valid_(owner_, origin);
  }
  bool ready() const override {
    return lv_is_initialized() && !modal_callback_dispatching() && !ha_read_coordinator().dispatching() &&
           !page_lvgl_dispatching();
  }
  bool connected() const override {
    return true;
  }
  uint32_t connection_generation() const override {
    return 0;
  }
  void* create(const PageStore&, ConfigurationHandle, PageModalToken) override {
    return nullptr;
  }
  void* create_local(const PageStore& store, PageLocalModalOrigin origin, PageModalToken token) override {
    const auto* page = store.page(origin.page);
    if (record_ || local_kind(token.kind) != ControlModalKind::NETWORK_STATUS || token.card != NO_CARD ||
        token.configuration != origin.configuration || store.generation() != origin.configuration || !page ||
        !page->navigable || !ha_internal_heap_available("network settings setup"))
      return nullptr;
    auto* memory = allocator_.allocate(sizeof(Record));
    if (!memory) return nullptr;
    auto* r = new (memory) Record(*this, origin, token);
    if (!r->gestures.available()) {
      release(r);
      return nullptr;
    }
    r->grid = static_cast<lv_coord_t*>(allocator_.allocate(grid_bytes()));
    if (!r->grid) {
      release(r);
      return nullptr;
    }
    r->name = source_.device_name();
    r->build = network_status_firmware_label(std::string(source_.firmware_version()));
    r->wifi = source_.wifi_available();
    if (!style_.resolve(style_.owner, origin, token, r->environment) || !valid_environment(r->environment)) {
      release(r);
      return nullptr;
    }
    record_ = r;
    return r;
  }
  bool initialize(void* pointer, PageModalToken token) override {
    auto& r = *static_cast<Record*>(pointer);
    if (control_modal_active().kind != ControlModalKind::NONE || !runtime_->valid(token)) return false;
    r.overlay = lv_obj_create(lv_layer_top());
    if (!r.overlay) return false;
    lv_obj_clear_flag(r.overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(r.overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(r.overlay, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(r.overlay, 0, LV_PART_MAIN);
    lv_obj_set_layout(r.overlay, LV_LAYOUT_GRID);
    const char* icons[] = {"\U000F0141", "\U000F0200", "\U000F05A9",
                           "\U000F035B", "\U000F0200", find_icon("Lock")};
    for (int i = 0; i < NETWORK_STATUS_CARD_COUNT; ++i) {
      if (i == NETWORK_STATUS_WIFI_CARD_INDEX && !r.wifi) continue;
      const auto& e = r.environment;
      auto* card = create_grid_card_button(r.overlay, e.radius, e.card_pad, e.label_font,
                                           lv_color_hex(current_theme().text_primary), e.continuous_corners);
      r.cards[i] = card;
      if (!card) return false;
      lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
      if (i != NETWORK_STATUS_BACK_CARD_INDEX && i != NETWORK_STATUS_LOCK_CARD_INDEX)
        lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
      r.icons[i] = lv_label_create(card);
      r.labels[i] = lv_label_create(card);
      if (!r.icons[i] || !r.labels[i]) return false;
      for (auto* label : {r.icons[i], r.labels[i]}) lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
      lv_label_set_display_text(r.icons[i], icons[i]);
      configure_button_label_wrap(r.labels[i]);
      lv_obj_align(r.icons[i], LV_ALIGN_TOP_LEFT, 0, 0);
      lv_obj_align(r.labels[i], LV_ALIGN_BOTTOM_LEFT, 0, 0);
    }
    // Attach the press guard before normal action callbacks.
    if (!r.gestures.attach(r.overlay) ||
        !lv_obj_add_event_cb(r.cards[NETWORK_STATUS_BACK_CARD_INDEX], back, LV_EVENT_CLICKED, &r) ||
        !lv_obj_add_event_cb(r.cards[NETWORK_STATUS_LOCK_CARD_INDEX], lock, LV_EVENT_CLICKED, &r))
      return false;
    control_modal_service().set_active(ControlModalKind::NETWORK_STATUS, r.overlay, close,
                                       ControlModalDismissPolicy::DISMISS);
    r.settings = true;
    if (style_.settings) style_.settings(style_.owner, true);
    if (!current(r)) return false;
    update(pointer, token);
    if (!current(r)) return false;
    lv_obj_move_foreground(r.overlay);
    return visible(pointer);
  }
  bool visible(void* pointer) const override {
    const auto& r = *static_cast<Record*>(pointer);
    return !r.retired && r.overlay && control_modal_active().overlay == r.overlay;
  }
  void update(void* pointer, PageModalToken) override {
    auto& r = *static_cast<Record*>(pointer);
    if (!current(r)) return;
    if (r.lock_pending) {
      r.lock_pending = false;
      const bool pin = source_.has_pin();
      if (!current(r)) return;
      if (pin) source_.activate_lock();
      if (!current(r)) return;
    }
    auto environment = r.environment;
    if (!style_.resolve(style_.owner, r.origin, r.token, environment) || !valid_environment(environment)) {
      runtime_->notify_closed(r.token, PageModalClose::FAILED);
      return;
    }
    if (!current(r)) return;
    const bool pin = source_.has_pin();
    if (!current(r)) return;
    const bool geometry = !r.seeded || !(environment == r.environment) || pin != r.pin;
    r.environment = environment;
    r.pin = pin;
    if (geometry) layout(r);
    const auto background = current_theme().background;
    const auto surface = theme_display_color(current_theme().surface_card);
    const auto text = current_theme().text_primary;
    if (geometry || background != r.background || surface != r.surface || text != r.text) {
      r.background = background;
      r.surface = surface;
      r.text = text;
      lv_obj_set_style_bg_color(r.overlay, lv_color_hex(background), LV_PART_MAIN);
      for (int i = 0; i < NETWORK_STATUS_CARD_COUNT; ++i)
        if (r.cards[i]) {
          lv_obj_set_style_bg_color(r.cards[i], lv_color_hex(surface), LV_PART_MAIN);
          lv_obj_set_style_bg_opa(r.cards[i], LV_OPA_COVER, LV_PART_MAIN);
          for (auto* label : {r.icons[i], r.labels[i]})
            lv_obj_set_style_text_color(label, lv_color_hex(text), LV_PART_MAIN);
        }
    }
    set_label(r, NETWORK_STATUS_BACK_CARD_INDEX, espcontrol_i18n("Back"));
    set_label(r, NETWORK_STATUS_BUILD_CARD_INDEX, r.build.c_str());
    set_label(r, NETWORK_STATUS_NAME_CARD_INDEX, r.name.c_str());
    set_label(r, NETWORK_STATUS_LOCK_CARD_INDEX, espcontrol_i18n("Screen Lock"));
    const auto now = lv_tick_get();
    if (!r.seeded || static_cast<uint32_t>(now - r.last_sample) >= 1000) {
      PageNetworkStatusSample sample;
      source_.sample(sample);
      if (!current(r)) return;
      sample.ip.back() = '\0';
      set_label(r, NETWORK_STATUS_IP_CARD_INDEX,
                sample.ip[0] ? sample.ip.data() : espcontrol_i18n("Not available"));
      if (r.wifi) {
        char quality[8]{};
        if (std::isfinite(sample.wifi_percent)) {
          std::snprintf(quality, sizeof(quality), "%d%%",
                        static_cast<int>(std::lround(std::max(0.0f, std::min(100.0f, sample.wifi_percent)))));
          set_label(r, NETWORK_STATUS_WIFI_CARD_INDEX, quality);
        } else
          set_label(r, NETWORK_STATUS_WIFI_CARD_INDEX, espcontrol_i18n_key("disconnected"));
      }
      r.last_sample = now;
    }
    r.seeded = true;
  }
  ControlModalDismissPolicy dismiss_policy(void*) const override {
    return ControlModalDismissPolicy::DISMISS;
  }
  void retire(void* pointer, PageModalClose) override {
    auto& r = *static_cast<Record*>(pointer);
    r.retired = true;
    r.lock_pending = false;
    r.gestures.invalidate();
    // Revoke in draw/event dispatch without invalidating the object being drawn.
    if (r.overlay && ready()) lv_obj_add_flag(r.overlay, LV_OBJ_FLAG_HIDDEN);
    if (r.overlay && control_modal_active().overlay == r.overlay) control_modal_service().reset_active();
    if (r.settings) {
      r.settings = false;
      if (style_.settings) style_.settings(style_.owner, false);
    }
  }
  void destroy(void* pointer) override {
    auto* r = static_cast<Record*>(pointer);
    assert(r == record_ && r->retired);
    if (r->overlay) lv_obj_delete(r->overlay);
    assert(!r->gestures.live());
    record_ = nullptr;
    release(r);
  }

 private:
  struct Record {
    Record(PageNetworkStatusModalPort& p, PageLocalModalOrigin o, PageModalToken t)
        : port(p), origin(o), token(t), gestures(p.allocator_, 8, t, &p, gesture_valid) {}
    PageNetworkStatusModalPort& port;
    PageLocalModalOrigin origin;
    PageModalToken token;
    PageModalGestures gestures;
    PageNetworkStatusEnvironment environment;
    std::string name, build;
    // Normalized display text need not compare equal to UTF-8 source bytes.
    std::array<std::string, NETWORK_STATUS_CARD_COUNT> painted;
    std::array<lv_obj_t*, NETWORK_STATUS_CARD_COUNT> cards{}, icons{}, labels{};
    lv_obj_t* overlay{nullptr};
    lv_coord_t* grid{nullptr};
    uint32_t last_sample{0}, background{0}, surface{0}, text{0};
    bool wifi{false}, pin{false}, seeded{false}, retired{false}, settings{false}, lock_pending{false};
  };
  bool current(const Record& r) const {
    return !r.retired && runtime_->valid(r.token);
  }
  bool valid_environment(const PageNetworkStatusEnvironment& e) const {
    return e.label_font && e.icon_font && e.width > 0 && e.height > e.top && e.top >= 0 && e.columns > 0 &&
           e.rows > 0 && static_cast<size_t>(e.columns) <= capacity_ &&
           static_cast<size_t>(e.rows) <= capacity_ &&
           static_cast<size_t>(network_status_grid_rows(e.columns, e.rows, NETWORK_STATUS_CARD_COUNT)) <=
               capacity_ &&
           e.card_pad >= 0 && e.radius >= 0 && e.pad_bottom >= 0 && e.pad_left >= 0 && e.pad_right >= 0 &&
           e.gap_row >= 0 && e.gap_column >= 0;
  }
  size_t grid_bytes() const {
    return 2 * (capacity_ + 1) * sizeof(lv_coord_t);
  }
  void layout(Record& r) {
    const auto& e = r.environment;
    lv_obj_set_pos(r.overlay, 0, e.top);
    lv_obj_set_size(r.overlay, e.width, e.height - e.top);
    lv_obj_set_style_pad_top(r.overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(r.overlay, e.pad_bottom, LV_PART_MAIN);
    lv_obj_set_style_pad_left(r.overlay, e.pad_left, LV_PART_MAIN);
    lv_obj_set_style_pad_right(r.overlay, e.pad_right, LV_PART_MAIN);
    lv_obj_set_style_pad_row(r.overlay, e.gap_row, LV_PART_MAIN);
    lv_obj_set_style_pad_column(r.overlay, e.gap_column, LV_PART_MAIN);
    auto* rows = r.grid + capacity_ + 1;
    for (int i = 0; i < e.columns; ++i) r.grid[i] = LV_GRID_FR(1);
    r.grid[e.columns] = LV_GRID_TEMPLATE_LAST;
    const int visible = NETWORK_STATUS_CARD_COUNT - (r.wifi ? 0 : 1) - (r.pin ? 0 : 1);
    const int count = network_status_grid_rows(e.columns, e.rows, visible);
    for (int i = 0; i < count; ++i) rows[i] = LV_GRID_FR(1);
    rows[count] = LV_GRID_TEMPLATE_LAST;
    lv_obj_set_grid_dsc_array(r.overlay, r.grid, rows);
    int index = 0;
    for (int i = 0; i < NETWORK_STATUS_CARD_COUNT; ++i)
      if (auto* card = r.cards[i]) {
        if (i == NETWORK_STATUS_LOCK_CARD_INDEX && !r.pin)
          lv_obj_add_flag(card, LV_OBJ_FLAG_HIDDEN);
        else {
          lv_obj_clear_flag(card, LV_OBJ_FLAG_HIDDEN);
          const auto cell = network_status_grid_cell(index++, e.columns);
          lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, cell.column, 1, LV_GRID_ALIGN_STRETCH, cell.row,
                               1);
        }
        apply_grid_card_corners(card, e.radius, e.continuous_corners);
        lv_obj_set_style_pad_all(card, e.card_pad, LV_PART_MAIN);
        lv_obj_set_style_text_font(r.icons[i], e.icon_font, LV_PART_MAIN);
        lv_obj_set_style_text_font(r.labels[i], e.label_font, LV_PART_MAIN);
        apply_width_compensation(r.icons[i], e.icon_compensation);
        apply_width_compensation(r.labels[i], e.text_compensation);
      }
  }
  void set_label(Record& r, int index, const char* text) {
    if (!r.labels[index]) return;
    if (!text) text = "";
    if (!r.seeded || r.painted[index] != text) {
      r.painted[index] = text;
      lv_label_set_display_text(r.labels[index], r.painted[index].c_str());
    }
  }
  static bool gesture_valid(void* owner, PageModalToken token) {
    auto& p = *static_cast<PageNetworkStatusModalPort*>(owner);
    return p.record_ && !p.record_->retired && p.runtime_->interactive(token);
  }
  static void close() {
    auto* p = installed_;
    if (p && p->record_ && p->runtime_->interactive(p->record_->token))
      p->runtime_->notify_closed(p->record_->token);
  }
  static void back(lv_event_t* event) {
    auto& r = *static_cast<Record*>(lv_event_get_user_data(event));
    if (r.port.current(r)) r.port.runtime_->notify_closed(r.token);
  }
  static void lock(lv_event_t* event) {
    auto& r = *static_cast<Record*>(lv_event_get_user_data(event));
    if (r.port.current(r) && r.pin) r.lock_pending = true;
  }
  void release(Record* r) {
    if (r->grid) allocator_.release(r->grid, grid_bytes());
    r->~Record();
    allocator_.release(r, sizeof(Record));
  }
  PageArenaAllocator& allocator_;
  PageNetworkStatusSource& source_;
  size_t capacity_;
  PageNetworkStatusStyle style_;
  void* owner_;
  OriginValid valid_;
  PageModalRuntime* runtime_{nullptr};
  Record* record_{nullptr};
  inline static PageNetworkStatusModalPort* installed_{nullptr};
};
}  // namespace espcontrol::pages
