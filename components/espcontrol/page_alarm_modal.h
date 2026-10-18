#pragma once

// Include after button_grid.h's existing alarm UI helpers.
#include "page_alarm_commands.h"
#include "page_lvgl_dispatch.h"
#include "page_modal_gestures.h"

namespace espcontrol::pages {
struct PageAlarmModalEnvironment {
  uint32_t on_color{DEFAULT_ACCENT_COLOR}, off_color{0x212121}, tertiary_color{0x424242};
  int red_percent{100}, green_percent{100}, blue_percent{100}, width_compensation_percent{100};
  int grid_columns{3};
  lv_obj_t* grid_page{nullptr};
  const lv_font_t *label_font{nullptr}, *pin_font{nullptr}, *key_font{nullptr}, *icon_font{nullptr},
      *title_font{nullptr};
};
struct PageAlarmModalStyle {
  PageAlarmModalEnvironment environment;
  size_t gesture_capacity{128};
  void* owner{nullptr};
  bool (*resolve)(void*, const ParsedCfg&, PageAlarmModalEnvironment&){nullptr};
};
// One demanded record supplies control and PIN UI for one modal opening.
// Switching to PIN keeps that opening; retiring it invalidates both interfaces.
class PageAlarmModalPort final : public PageModalPort {
 public:
  using OriginValid = bool (*)(void*, PageBindingToken);
  PageAlarmModalPort(PageArenaAllocator& allocator, PageEntityStateRuntime& graph,
                     PageAlarmStateRuntime& state, PageAlarmCommands& commands, PageAlarmModalStyle style,
                     void* owner, OriginValid valid)
      : allocator_(allocator),
        graph_(graph),
        state_(state),
        commands_(commands),
        style_(style),
        owner_(owner),
        valid_(valid) {}
  ~PageAlarmModalPort() override {
    assert(!record_);
    if (alarm_modal_lifetime().owner == this) alarm_modal_lifetime() = {};
  }
  bool install(PageModalRuntime& runtime, bool legacy_renderer_retired) {
    auto& observer = alarm_modal_lifetime();
    if (!legacy_renderer_retired || !lv_is_initialized() || record_ || alarm_control_modal_ui().active ||
        alarm_pin_modal_ui().active || alarm_arming_takeover_ctx() || alarm_deferred_action().timer ||
        (observer.owner && observer.owner != this))
      return false;
    runtime_ = &runtime;
    observer = {};
    observer.owner = this;
    observer.managed = managed;
    observer.current = current;
    observer.action = action;
    observer.closed = closed;
    observer.defer = defer;
    observer.remaining = remaining;
    observer.progress = progress;
    return true;
  }
  bool selected_pin(PageModalToken token, alarm::Action mode) const {
    return record_ && !record_->retired && record_->pin_phase && record_->pin_mode == mode &&
           record_->token.opening == token.opening && record_->token.configuration == token.configuration &&
           record_->token.card == token.card && record_->token.kind == token.kind && pin_overlay(*record_) &&
           runtime_->interactive(token);
  }
  bool valid_origin(PageBindingToken token) const override {
    return valid_ && valid_(owner_, token);
  }
  ControlModalKind kind(const PageStore& store, ConfigurationHandle handle) const override {
    if (!runtime_ || !style_.resolve || alarm_modal_lifetime().owner != this || !store.card(handle))
      return ControlModalKind::NONE;
    const auto type = PageAlarmStateRuntime::classify(store, handle.index);
    if (type == PageAlarmSourceKind::CONTROL) return ControlModalKind::ALARM_CONTROL;
    if (type != PageAlarmSourceKind::ACTION) return ControlModalKind::NONE;
    const auto mode = store.field(*store.card(handle), CardField::SENSOR);
    const std::string options(PageAlarmStateRuntime::effective_options(store, handle.index));
    return cfg_option_value(options, mode == "disarm" ? "pin_disarm" : "pin_arm") == "0"
               ? ControlModalKind::NONE
               : ControlModalKind::ALARM_PIN;
  }
  ControlModalKind kind(const PageStore& store, ConfigurationHandle handle,
                        ControlModalKind requested) const override {
    if (requested != ControlModalKind::ALARM_CONTROL) return PageModalPort::kind(store, handle, requested);
    state_.loop();
    const auto takeover = state_.takeover();
    if (takeover.valid && takeover.origin.generation == handle.generation &&
        takeover.origin.index == handle.index && runtime_ && style_.resolve &&
        alarm_modal_lifetime().owner == this && store.card(handle))
      return ControlModalKind::ALARM_CONTROL;
    return kind(store, handle) == requested ? requested : ControlModalKind::NONE;
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
  void* create(const PageStore& store, ConfigurationHandle handle, PageModalToken token) override {
    if (record_ || kind(store, handle, token.kind) != token.kind ||
        !ha_internal_heap_available("alarm panel setup"))
      return nullptr;
    auto* memory = allocator_.allocate(sizeof(Record));
    if (!memory) return nullptr;
    auto* record = new (memory) Record(*this, token, handle);
    if (!record->gestures.available()) {
      release(record);
      return nullptr;
    }
    const auto* card = store.card(handle);
    ParsedCfg config;
    config.entity = std::string(PageAlarmStateRuntime::effective_entity(store, handle.index));
    config.label = std::string(store.field(*card, CardField::LABEL));
    config.icon = std::string(store.field(*card, CardField::ICON));
    config.icon_on = std::string(store.field(*card, CardField::ICON_ON));
    config.sensor = std::string(store.field(*card, CardField::SENSOR));
    config.unit = std::string(store.field(*card, CardField::UNIT));
    config.type = std::string(store.field(*card, CardField::TYPE));
    config.precision = std::string(store.field(*card, CardField::PRECISION));
    config.options = std::string(store.field(*card, CardField::OPTIONS));
    auto environment = style_.environment;
    if (!style_.resolve(style_.owner, config, environment)) {
      release(record);
      return nullptr;
    }
    auto& ctx = record->context;
    ctx.entity_id = config.entity;
    ctx.label = config.label.empty() ? espcontrol_i18n(std::string("Alarm")) : config.label;
    ctx.options = std::string(PageAlarmStateRuntime::effective_options(store, handle.index));
    ctx.state.reserve(32);
    ctx.arm_mode.reserve(32);
    ctx.label_font = environment.label_font;
    ctx.pin_label_font = environment.pin_font;
    ctx.key_label_font = environment.key_font;
    ctx.icon_font = environment.icon_font;
    ctx.arming_title_font = environment.title_font;
    ctx.on_color = environment.on_color;
    ctx.off_color = environment.off_color;
    ctx.tertiary_color = environment.tertiary_color;
    ctx.theme_red_percent = environment.red_percent;
    ctx.theme_green_percent = environment.green_percent;
    ctx.theme_blue_percent = environment.blue_percent;
    ctx.width_compensation_percent = environment.width_compensation_percent;
    ctx.grid_cols = environment.grid_columns;
    ctx.grid_page = environment.grid_page;
    ctx.off_theme_owned =
        PageAlarmStateRuntime::classify(store, handle.index) == PageAlarmSourceKind::CONTROL &&
        cfg_option_value(ctx.options, "card_off_color").empty();
    record->action_mode = PageAlarmStateRuntime::classify(store, handle.index) == PageAlarmSourceKind::ACTION
                              ? alarm::action(config.sensor)
                              : alarm::Action::NONE;
    if (token.kind == ControlModalKind::ALARM_PIN) record->pin_mode = record->action_mode;
    record_ = record;
    return record;
  }
  bool initialize(void* pointer, PageModalToken) override {
    auto& record = *static_cast<Record*>(pointer);
    if (control_modal_active().kind != ControlModalKind::NONE) return false;
    pull(record);
    // The legacy panel does not open for an explicitly unavailable alarm.
    if (!current(this, &record.context) || !record.context.available) return false;
    if (record.token.kind == ControlModalKind::ALARM_PIN) {
      if (!record.command_available || record.pin_mode == alarm::Action::NONE) return false;
      open_pin(record);
    } else
      alarm_control_open_modal(&record.context);
    if (!visible(&record) || !attach(record)) return false;
    set_clock_bar_modal_label(record.context.label);
    return true;
  }
  bool visible(void* pointer) const override {
    const auto& record = *static_cast<Record*>(pointer);
    if (record.retired) return false;
    const auto& control = alarm_control_modal_ui();
    const auto& pin = alarm_pin_modal_ui();
    return (control.active == &record.context && control.overlay) ||
           (pin.active && pin.active->card == &record.context && pin.overlay);
  }
  void update(void* pointer, PageModalToken) override {
    auto& record = *static_cast<Record*>(pointer);
    pull(record);
    if (record.retired) return;
    if (record.open_pin) {
      record.open_pin = false;
      if (!record.command_available) return;
      record.transitioning = true;
      open_pin(record);
      record.transitioning = false;
      if (!visible(&record) || !attach(record))
        runtime_->notify_closed(record.token, PageModalClose::FAILED);
      else
        set_clock_bar_modal_label(record.context.label);
    }
    if (record.awaiting_submission && commands_.memory().pending == 0 && current(this, &record.context))
      alarm_pin_hide_modal();
  }
  ControlModalDismissPolicy dismiss_policy(void* pointer) const override {
    const auto& record = *static_cast<Record*>(pointer);
    const auto snapshot = state_.read(record.card);
    const bool pending_pin =
        record.open_pin && snapshot.command_available &&
        graph_.read_attribute(record.card, snapshot.entity, {}).status != PageEntityStatus::EXHAUSTED;
    return (record.pin_phase || pending_pin) ? ControlModalDismissPolicy::DISMISS
                                             : ControlModalDismissPolicy::PRESERVE_DURING_DISPLAY_TAKEOVER;
  }
  uint64_t source_revision(void*) const override {
    return state_.takeover_events();
  }
  void retire(void* pointer, PageModalClose reason) override {
    auto& record = *static_cast<Record*>(pointer);
    record.retired = true;
    record.gestures.invalidate();
    commands_.cancel(record.token);
    record.open_pin = record.awaiting_submission = false;
    auto& pin = alarm_pin_modal_ui();
    if (pin.active && pin.active->card == &record.context) alarm_clear_pin(pin.pin);
    if (reason == PageModalClose::REPLACED || reason == PageModalClose::SHUTDOWN || !ready()) {
      for (auto* overlay : {control_overlay(record), pin_overlay(record)})
        if (overlay) {
          lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
          if (control_modal_active().overlay == overlay) control_modal_service().reset_active();
        }
    } else
      hide(record);
  }
  void destroy(void* pointer) override {
    auto* record = static_cast<Record*>(pointer);
    assert(record == record_ && record->retired);
    hide(*record);
    assert(record->gestures.live() == 0 && !record->context.arm_delay_timer &&
           !record->context.pending_action_timer);
    record_ = nullptr;
    release(record);
  }

 private:
  struct Record {
    Record(PageAlarmModalPort& port, PageModalToken opening, ConfigurationHandle handle)
        : token(opening),
          card(handle),
          connection(port.connection_generation()),
          gestures(port.allocator_, port.style_.gesture_capacity, opening, &port, gesture_valid) {}
    AlarmCardCtx context;
    PageModalToken token;
    ConfigurationHandle card;
    uint32_t connection;
    PageModalGestures gestures;
    alarm::Action pin_mode{alarm::Action::NONE}, action_mode{alarm::Action::NONE};
    bool retired{false}, transitioning{false}, open_pin{false}, awaiting_submission{false}, pin_phase{false},
        command_available{false};
    uint64_t seen{UINT64_MAX};
  };
  void release(Record* record) {
    record->~Record();
    allocator_.release(record, sizeof(Record));
  }
  static lv_obj_t* control_overlay(Record& record) {
    const auto& ui = alarm_control_modal_ui();
    return ui.active == &record.context ? ui.overlay : nullptr;
  }
  static lv_obj_t* pin_overlay(Record& record) {
    const auto& ui = alarm_pin_modal_ui();
    return ui.active && ui.active->card == &record.context ? ui.overlay : nullptr;
  }
  static void hide(Record& record) {
    if (pin_overlay(record)) alarm_pin_hide_modal();
    if (control_overlay(record)) alarm_control_hide_modal();
  }
  bool attach(Record& record) {
    for (auto* overlay : {control_overlay(record), pin_overlay(record)})
      if (overlay && !record.gestures.attach(overlay)) return false;
    return true;
  }
  void open_pin(Record& record) {
    AlarmActionCtx action;
    action.card = &record.context;
    action.mode = mode_name(record.pin_mode);
    action.requires_pin = true;
    alarm_pin_open_modal(&action);
    if (pin_overlay(record)) record.pin_phase = true;
  }
  static const char* mode_name(alarm::Action action) {
    switch (action) {
      case alarm::Action::AWAY:
        return "away";
      case alarm::Action::HOME:
        return "home";
      case alarm::Action::NIGHT:
        return "night";
      case alarm::Action::VACATION:
        return "vacation";
      case alarm::Action::DISARM:
        return "disarm";
      default:
        return "";
    }
  }
  void pull(Record& record) {
    if (record.retired || !current(this, &record.context)) return;
    state_.loop();
    const auto snapshot = state_.read(record.card);
    record.command_available =
        snapshot.valid && snapshot.command_available &&
        graph_.read_attribute(record.card, snapshot.entity, {}).status != PageEntityStatus::EXHAUSTED;
    const auto revision = state_.revision();
    if (record.seen == revision) return;
    record.seen = revision;
    record.context.available = snapshot.valid && snapshot.available;
    if (snapshot.state.empty())
      record.context.state.clear();
    else
      record.context.state.assign(snapshot.state.data(), snapshot.state.size());
    if (snapshot.arm_mode.empty())
      record.context.arm_mode.clear();
    else
      record.context.arm_mode.assign(snapshot.arm_mode.data(), snapshot.arm_mode.size());
    if (control_overlay(record)) alarm_control_update_modal(&record.context);
  }
  static bool managed(void* owner, const AlarmCardCtx* ctx) {
    const auto& port = *static_cast<PageAlarmModalPort*>(owner);
    return port.record_ && &port.record_->context == ctx;
  }
  static bool current(void* owner, const AlarmCardCtx* ctx) {
    const auto& port = *static_cast<PageAlarmModalPort*>(owner);
    return managed(owner, ctx) && !port.record_->retired && port.runtime_->valid(port.record_->token) &&
           port.record_->connection == port.connection_generation();
  }
  static bool action(void* owner, const AlarmCardCtx* ctx) {
    auto& port = *static_cast<PageAlarmModalPort*>(owner);
    if (!current(owner, ctx)) return false;
    port.pull(*port.record_);
    return port.record_->command_available && port.runtime_->interactive(port.record_->token);
  }
  static bool gesture_valid(void* owner, PageModalToken token) {
    const auto& port = *static_cast<PageAlarmModalPort*>(owner);
    return port.record_ && port.runtime_->interactive(token) && current(owner, &port.record_->context);
  }
  static void closed(void* owner, AlarmCardCtx* ctx) {
    auto& port = *static_cast<PageAlarmModalPort*>(owner);
    if (!managed(owner, ctx) || port.record_->retired || port.record_->transitioning) return;
    port.runtime_->notify_closed(port.record_->token,
                                 port.connected() && port.record_->connection == port.connection_generation()
                                     ? PageModalClose::USER
                                     : PageModalClose::DISCONNECTED);
  }
  static bool defer(void* owner, const AlarmCardCtx* ctx, std::string_view mode, std::string_view code,
                    bool submit_pin) {
    auto& port = *static_cast<PageAlarmModalPort*>(owner);
    if (!action(owner, ctx)) return false;
    auto& record = *port.record_;
    const auto selected = alarm::action(mode);
    if (selected == alarm::Action::NONE) return false;
    if (submit_pin) {
      if (record.pin_mode != selected || !pin_overlay(record) || code.empty()) return false;
      const bool queued = port.commands_.queue({{}, record.token, record.connection}, selected, code);
      if (queued) record.awaiting_submission = true;
      return queued;
    }
    if (record.action_mode != alarm::Action::NONE && record.token.kind == ControlModalKind::ALARM_CONTROL &&
        selected != record.action_mode) {
      const auto takeover = port.state_.takeover();
      if (!takeover.valid || takeover.origin.generation != record.card.generation ||
          takeover.origin.index != record.card.index)
        return false;
    }
    if (alarm_action_requires_pin(record.context.options, std::string(mode))) {
      record.pin_mode = selected;
      record.open_pin = true;
      return true;
    }
    return port.commands_.queue({{}, record.token, record.connection}, selected);
  }
  static int remaining(void* owner, const AlarmCardCtx* ctx) {
    auto& port = *static_cast<PageAlarmModalPort*>(owner);
    return current(owner, ctx) ? port.state_.read(port.record_->card).remaining : -1;
  }
  static int progress(void* owner, const AlarmCardCtx* ctx) {
    auto& port = *static_cast<PageAlarmModalPort*>(owner);
    return current(owner, ctx) ? port.state_.read(port.record_->card).progress : -1;
  }
  PageArenaAllocator& allocator_;
  PageEntityStateRuntime& graph_;
  PageAlarmStateRuntime& state_;
  PageAlarmCommands& commands_;
  PageAlarmModalStyle style_;
  void* owner_;
  OriginValid valid_;
  PageModalRuntime* runtime_{nullptr};
  Record* record_{nullptr};
};
}  // namespace espcontrol::pages
