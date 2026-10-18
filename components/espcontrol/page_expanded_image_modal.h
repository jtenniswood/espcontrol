#pragma once

// Include after the common config, HA and control-modal shell helpers. This
// adapter owns no legacy ImageCardCtx, callbacks, downloader arrays or timers.
#include <cassert>
#include <new>

#include "page_cover_art_frame.h"
#include "page_expanded_image_view.h"
#include "page_lvgl_dispatch.h"
#include "page_modal_gestures.h"

namespace espcontrol::pages {
struct PageExpandedImageModalEnvironment {
  const lv_font_t *label_font{nullptr}, *icon_font{nullptr};
  int width_compensation_percent{100};
  uint32_t background{0};
};
struct PageExpandedImageModalStyle {
  PageExpandedImageModalEnvironment environment;
  void* owner{nullptr};
  bool (*resolve)(void*, const ParsedCfg&, PageExpandedImageModalEnvironment&){nullptr};
  // Application scene keeps grid bindings but suppresses input, tile work and
  // animation under the overlay. It releases them for screensaver takeover.
  void (*covered)(void*, bool){nullptr};
};
class PageExpandedImageModalPort final : public PageModalPort {
 public:
  using OriginValid = bool (*)(void*, PageBindingToken);
  PageExpandedImageModalPort(PageArenaAllocator& allocator, PageEntityStateRuntime& graph,
                             PageExpandedImageView& view, PageExpandedImageModalStyle style, void* owner,
                             OriginValid valid)
      : allocator_(allocator), graph_(graph), view_(view), style_(style), owner_(owner), valid_(valid) {}
  ~PageExpandedImageModalPort() override {
    assert(!record_);
    if (installed_ == this) installed_ = nullptr;
  }
  PageExpandedImageModalPort(const PageExpandedImageModalPort&) = delete;
  PageExpandedImageModalPort& operator=(const PageExpandedImageModalPort&) = delete;
  bool install(PageModalRuntime& runtime, bool legacy_renderer_retired) {
    if (!legacy_renderer_retired || !lv_is_initialized() || !view_.ready() || record_ ||
        (installed_ && installed_ != this))
      return false;
    installed_ = this;
    runtime_ = &runtime;
    return true;
  }
  bool valid_origin(PageBindingToken token) const override {
    return valid_ && valid_(owner_, token);
  }
  ControlModalKind kind(const PageStore& store, ConfigurationHandle card) const override {
    const auto* row = store.card(card);
    return runtime_ && installed_ == this && style_.resolve && row &&
                   store.field(*row, CardField::TYPE) == "image" &&
                   page_image_entity_kind(store.field(*row, CardField::ENTITY)) !=
                       PageImageEntityKind::UNSUPPORTED
               ? ControlModalKind::IMAGE_CARD
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
    if (record_ || kind(store, card) != ControlModalKind::IMAGE_CARD ||
        !ha_internal_heap_available("image viewer setup"))
      return nullptr;
    auto* memory = allocator_.allocate(sizeof(Record));
    if (!memory) return nullptr;
    auto* r = new (memory) Record(*this, token, card);
    if (!r->gestures.available()) {
      release(r);
      return nullptr;
    }
    const auto* row = store.card(card);
    auto& p = r->config;
    p.entity = std::string(store.field(*row, CardField::ENTITY));
    p.label = std::string(store.field(*row, CardField::LABEL));
    p.icon = std::string(store.field(*row, CardField::ICON));
    p.icon_on = std::string(store.field(*row, CardField::ICON_ON));
    p.sensor = std::string(store.field(*row, CardField::SENSOR));
    p.unit = std::string(store.field(*row, CardField::UNIT));
    p.type = std::string(store.field(*row, CardField::TYPE));
    p.precision = std::string(store.field(*row, CardField::PRECISION));
    p.options = std::string(store.field(*row, CardField::OPTIONS));
    r->trigger = cfg_option_value(p.options, "image_modal_refresh_trigger");
    r->fit = image_card_modal_fit_enabled(p);
    r->environment = style_.environment;
    if (!style_.resolve(style_.owner, p, r->environment) || !r->environment.label_font ||
        !r->environment.icon_font) {
      release(r);
      return nullptr;
    }
    record_ = r;
    return r;
  }
  bool initialize(void* pointer, PageModalToken token) override {
    auto& r = *static_cast<Record*>(pointer);
    if (control_modal_active().kind != ControlModalKind::NONE) return false;
    auto shell =
        control_modal_open_shell(ControlModalKind::IMAGE_CARD, nullptr,
                                 r.environment.width_compensation_percent, r.environment.icon_font, close);
    r.overlay = shell.overlay;
    r.panel = shell.panel;
    r.back = shell.close_btn;
    if (!r.overlay || !r.panel || !r.back) return false;
    control_modal_block_close_for(350);
    control_modal_style_translucent_chrome_button(r.back);
    if (control_modal_uses_compact_portrait_tuning(shell.layout)) {
      const auto size = control_modal_scaled_px(58, shell.layout.short_side);
      if (size > shell.layout.back_size) {
        lv_obj_set_size(r.back, size, size);
        lv_obj_set_style_radius(r.back, size / 2, LV_PART_MAIN);
        lv_obj_align(r.back, LV_ALIGN_TOP_LEFT, shell.layout.back_inset_x, shell.layout.back_inset_y);
      }
    }
    r.surface = lv_obj_create(r.panel);
    r.loading = lv_obj_create(r.panel);
    if (!r.surface || !r.loading) return false;
    for (auto* object : {r.surface, r.loading}) {
      lv_obj_clear_flag(object, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_style_pad_all(object, 0, LV_PART_MAIN);
      lv_obj_set_style_shadow_width(object, 0, LV_PART_MAIN);
      lv_obj_set_style_clip_corner(object, true, LV_PART_MAIN);
    }
    lv_obj_set_style_border_width(r.surface, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(r.surface, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(r.loading, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_border_width(r.loading, 1, LV_PART_MAIN);
    r.icon = lv_label_create(r.loading);
    r.label = lv_label_create(r.loading);
    if (!r.icon || !r.label) return false;
    lv_label_set_display_text(r.icon, "\U000F02E9");
    apply_icon_width_compensation(r.icon);
    if (!lv_obj_add_event_cb(r.surface, draw, LV_EVENT_ALL, &r) || !r.gestures.attach(r.overlay))
      return false;
    if (style_.covered) {
      r.covered = true;
      style_.covered(style_.owner, true);
    }
    if (r.retired || !runtime_->valid(token)) return false;
    r.bound = true;
    if (!view_.bind(
            token,
            {r.config.entity, r.trigger,
             camera::refresh_mode(cfg_option_value(r.config.options, "image_modal_refresh_mode")),
             camera::refresh_interval_ms(cfg_option_value(r.config.options, "image_modal_refresh_interval")),
             r.fit}))
      return false;
    update(pointer, token);
    return visible(pointer);
  }
  bool visible(void* pointer) const override {
    const auto& r = *static_cast<Record*>(pointer);
    return !r.retired && r.overlay && r.panel && r.surface && control_modal_active().overlay == r.overlay;
  }
  void update(void* pointer, PageModalToken) override {
    auto& r = *static_cast<Record*>(pointer);
    if (r.retired || !runtime_->valid(r.token)) return;
    auto environment = style_.environment;
    if (!style_.resolve(style_.owner, r.config, environment) || !environment.label_font ||
        !environment.icon_font) {
      runtime_->notify_closed(r.token, PageModalClose::FAILED);
      return;
    }
    const bool style_changed =
        !r.paint.seeded || environment.label_font != r.environment.label_font ||
        environment.icon_font != r.environment.icon_font ||
        environment.background != r.environment.background ||
        environment.width_compensation_percent != r.environment.width_compensation_percent;
    r.environment = environment;
    lv_obj_update_layout(r.panel);
    const auto width = lv_obj_get_width(r.panel), height = lv_obj_get_height(r.panel);
    if (width <= 0 || height <= 0 || width > UINT16_MAX || height > UINT16_MAX) return;
    const auto radius = lv_obj_get_style_radius(r.panel, LV_PART_MAIN);
    const bool geometry_changed =
        !r.paint.seeded || width != r.paint.width || height != r.paint.height || radius != r.paint.radius;
    if (geometry_changed || style_changed) view_.size(r.token, width, height, r.environment.background);
    const auto frame = view_.surface(r.token);
    if (r.retired || !runtime_->valid(r.token)) return;
    const auto view_revision = view_.revision();
    const auto overlay = theme_display_color(current_theme().overlay);
    const auto surface = theme_display_color(current_theme().surface_primary);
    const auto border = theme_display_color(current_theme().border);
    const auto text = current_theme().text_primary;
    const bool has_frame = frame.policy.status == PageImageTileStatus::READY && frame.source;
    caption(r);
    if (!style_changed && !geometry_changed && r.paint.revision == view_revision &&
        r.paint.overlay == overlay && r.paint.surface == surface && r.paint.border == border &&
        r.paint.text == text && r.paint.status == frame.policy.status && r.paint.has_frame == has_frame)
      return;
    r.paint = {view_revision,       overlay,   surface, border, text, width, height, radius,
               frame.policy.status, has_frame, true};
    lv_obj_set_style_bg_color(r.panel, lv_color_hex(theme_display_color(current_theme().overlay)),
                              LV_PART_MAIN);
    lv_obj_set_style_bg_opa(r.panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(r.panel, true, LV_PART_MAIN);
    for (auto* object : {r.surface, r.loading}) {
      lv_obj_set_pos(object, 0, 0);
      lv_obj_set_size(object, width, height);
      lv_obj_set_style_radius(object, lv_obj_get_style_radius(r.panel, LV_PART_MAIN), LV_PART_MAIN);
    }
    lv_obj_set_style_bg_color(r.loading, lv_color_hex(theme_display_color(current_theme().surface_primary)),
                              LV_PART_MAIN);
    lv_obj_set_style_border_color(r.loading, lv_color_hex(theme_display_color(current_theme().border)),
                                  LV_PART_MAIN);
    lv_obj_set_style_text_font(r.icon, r.environment.icon_font, LV_PART_MAIN);
    apply_icon_width_compensation(r.icon);
    lv_obj_set_style_text_font(r.label, r.environment.label_font, LV_PART_MAIN);
    for (auto* label : {r.icon, r.label}) {
      lv_obj_set_style_text_color(label, lv_color_hex(current_theme().text_primary), LV_PART_MAIN);
      lv_obj_set_style_text_opa(label, LV_OPA_COVER, LV_PART_MAIN);
    }
    lv_obj_set_width(r.label, width);
    lv_label_set_long_mode(r.label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(r.label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(r.icon, LV_ALIGN_CENTER, 0, -18);
    lv_obj_align_to(r.label, r.icon, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
    if (has_frame)
      lv_obj_add_flag(r.loading, LV_OBJ_FLAG_HIDDEN);
    else {
      lv_label_set_display_text(
          r.label,
          espcontrol_i18n(frame.policy.status == PageImageTileStatus::LOADING ? "Loading" : "Unavailable"));
      lv_obj_clear_flag(r.loading, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_move_foreground(r.loading);
    lv_obj_move_foreground(r.back);
    lv_obj_invalidate(r.surface);
  }
  ControlModalDismissPolicy dismiss_policy(void*) const override {
    return ControlModalDismissPolicy::DISMISS;
  }
  void retire(void* pointer, PageModalClose reason) override {
    auto& r = *static_cast<Record*>(pointer);
    r.retired = true;
    r.gestures.invalidate();
    // Rendering forbids invalidating LVGL objects. Value/gesture ownership is
    // revoked now; maintenance destroys the overlay after dispatch completes.
    if (r.overlay && ready()) lv_obj_add_flag(r.overlay, LV_OBJ_FLAG_HIDDEN);
    if (control_modal_active().overlay == r.overlay) control_modal_service().reset_active();
    if (r.bound) {
      r.bound = false;
      view_.unbind(r.token);
    }
    if (r.covered) {
      r.covered = false;
      if (style_.covered) style_.covered(style_.owner, false);
    }
    (void)reason;
  }
  void destroy(void* pointer) override {
    auto* r = static_cast<Record*>(pointer);
    assert(r == record_ && r->retired);
    if (r->overlay) lv_obj_delete(r->overlay);
    assert(r->gestures.live() == 0);
    record_ = nullptr;
    set_clock_bar_modal_label("");
    release(r);
  }

 private:
  struct Paint {
    uint64_t revision{0};
    uint32_t overlay{0}, surface{0}, border{0}, text{0};
    int width{0}, height{0}, radius{0};
    PageImageTileStatus status{PageImageTileStatus::UNAVAILABLE};
    bool has_frame{false}, seeded{false};
  };
  struct Record {
    Record(PageExpandedImageModalPort& p, PageModalToken t, ConfigurationHandle c)
        : port(p), token(t), card(c), gestures(p.allocator_, 8, t, &p, gesture_valid) {}
    PageExpandedImageModalPort& port;
    PageModalToken token;
    ConfigurationHandle card;
    ParsedCfg config;
    std::string trigger, caption;
    PageExpandedImageModalEnvironment environment;
    Paint paint;
    PageModalGestures gestures;
    lv_obj_t *overlay{nullptr}, *panel{nullptr}, *back{nullptr}, *surface{nullptr}, *loading{nullptr},
        *icon{nullptr}, *label{nullptr};
    bool retired{false}, covered{false}, bound{false}, fit{false};
  };
  static bool gesture_valid(void* owner, PageModalToken token) {
    const auto& port = *static_cast<PageExpandedImageModalPort*>(owner);
    return port.record_ && !port.record_->retired && port.runtime_->interactive(token);
  }
  static void close() {
    auto* port = installed_;
    if (port && port->record_ && port->runtime_->interactive(port->record_->token))
      port->runtime_->notify_closed(port->record_->token);
  }
  static void draw(lv_event_t* event) {
    auto& r = *static_cast<Record*>(lv_event_get_user_data(event));
    if (r.retired || !r.bound || !r.port.runtime_->valid(r.token)) return;
    const auto code = lv_event_get_code(event);
    if (code != LV_EVENT_DRAW_MAIN && code != LV_EVENT_REFR_EXT_DRAW_SIZE) return;
    const auto surface = r.port.view_.surface(r.token);
    if (surface.policy.status != PageImageTileStatus::READY || !surface.source) return;
    PageCoverArtFrame frame;
    frame.source = surface.source;
    frame.fit = r.fit;
    const auto current = r.port.view_.surface(r.token);
    if (r.retired || !r.bound || !r.port.runtime_->valid(r.token) || !surface.same_frame(current)) return;
    if (code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
      auto* size = static_cast<int32_t*>(lv_event_get_param(event));
      if (size) frame.extend_draw_size(r.surface, *size);
      return;
    }
    // Descriptor is pulled anew for every draw; it is never attached to an
    // image widget. The coherent check above includes both role and opening;
    // retired pixel storage remains protected by the transport draw fence.
    frame.draw_image(r.surface, lv_event_get_layer(event));
  }
  void caption(Record& r) {
    auto text = std::string_view(r.config.label);
    if (text.empty()) {
      const auto friendly = graph_.read_attribute(r.card, r.config.entity, "friendly_name");
      if (friendly.status == PageEntityStatus::AVAILABLE) {
        text = friendly.value.substr(0, 64);
        const auto end = text.find('\0');
        if (end != std::string_view::npos) text = text.substr(0, end);
      }
      if (text.empty()) text = r.config.entity;
    }
    if (r.caption != text) {
      r.caption.assign(text.data(), text.size());
      set_clock_bar_modal_label(r.caption);
    }
  }
  void release(Record* record) {
    record->~Record();
    allocator_.release(record, sizeof(Record));
  }
  PageArenaAllocator& allocator_;
  PageEntityStateRuntime& graph_;
  PageExpandedImageView& view_;
  PageExpandedImageModalStyle style_;
  void* owner_;
  OriginValid valid_;
  PageModalRuntime* runtime_{nullptr};
  Record* record_{nullptr};
  inline static PageExpandedImageModalPort* installed_{nullptr};
};
}  // namespace espcontrol::pages
