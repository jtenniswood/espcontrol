#pragma once

#include <array>
#include <cassert>

#include "button_grid_subpage_parser.h"
#include "number_slider_policy.h"
#include "page_confirmation_policy.h"
#include "page_entity_state_runtime.h"
#include "page_media_state_runtime.h"
#include "page_renderer_runtime.h"
#include "page_timer_runtime.h"
#include "page_toggle_state.h"

namespace espcontrol::pages {
enum class PageCardCommandKind : uint8_t {
  NONE,
  TOGGLE,
  BUTTON,
  TURN_ON,
  TURN_OFF,
  OPEN,
  CLOSE,
  ACTION,
  LOCAL,
  RELAY,
  PUSH,
  WEBHOOK,
  SCREENSAVER,
  SCREEN_LOCK,
  PERCENT,
  NUMBER,
  TEMPERATURE,
  SLIDER_TOGGLE,
  TIMER_START,
  TIMER_CANCEL,
  MEDIA_ACTION,
  MEDIA_SEEK,
  DEVICE_ACTION
};
struct PageCardCommandValue {
  double number{0};
  number_slider::Metadata metadata;
  int integer{0};
  uint32_t write_revision{0};
};
struct PageCardCommandSnapshot {
  PageBindingToken origin;
  PageCardCommandKind kind{PageCardCommandKind::NONE};
  ParsedCfg config;
  uint16_t slot{0};
  bool root{false};
  PageCardCommandValue value;
};
class PageCardControlPolicy {
 public:
  virtual ~PageCardControlPolicy() = default;
  virtual bool admit(const PageStore&, PageBindingToken, PageCardCommandKind, PageCardCommandValue&) = 0;
  virtual bool valid(const PageStore&, PageBindingToken, PageCardCommandKind,
                     const PageCardCommandValue&) = 0;
  virtual void discard(PageBindingToken, PageCardCommandKind, const PageCardCommandValue&) = 0;
  // Scalar history acknowledgement after a helper accepted a send. No callbacks.
  virtual void accepted(PageBindingToken, PageCardCommandKind, const PageCardCommandValue&) {}
};
class PageCardCommandPort {
 public:
  virtual ~PageCardCommandPort() = default;
  // Read-only queries: valid also rejects covered/locked/wake-guarded input.
  // No query may pump callbacks or replace configuration. Dispatch alone can
  // reenter; its snapshot owns every string until that call returns.
  virtual bool ready() const = 0;
  virtual bool valid(PageBindingToken) const = 0;
  virtual bool connected() const = 0;
  virtual bool service_available() const = 0;
  virtual void dispatch(const PageCardCommandSnapshot&) = 0;
  // Capture/validate scalar control data only. No UI, allocation, producer
  // callbacks or service dispatch. The state policy may poll reconnect flags.
  virtual bool admit_control(const PageStore&, PageBindingToken, PageCardCommandKind, PageCardCommandValue&) {
    return false;
  }
  virtual bool valid_control(const PageStore&, PageBindingToken, PageCardCommandKind,
                             const PageCardCommandValue&) {
    return false;
  }
  virtual void discard_control(PageBindingToken, PageCardCommandKind, const PageCardCommandValue&) {}
  virtual bool dispatch_control(const PageCardCommandSnapshot&) { return false; }
  // Safe dispatch notification. The scene can validate/apply a view preview;
  // this may reenter, and receives only the consumed intent's owned scalars.
  virtual void accepted_control(PageBindingToken, PageCardCommandKind, const PageCardCommandValue&) {}
};
enum class PageCardCommandResult : uint8_t { NONE, CANCELLED, DISPATCHED, FAILED };
struct PageCardCommandMemory {
  size_t fixed_bytes{0}, pending{0}, dispatched{0}, cancelled{0}, rejected{0}, failed{0};
};

// A fixed FIFO of basic-card intents, independent of configured card count. The
// native adapter consumes existing service/local/webhook policies in dispatch.
// Typed sliders, timers and media share this queue. Alarm routes remain separate.
class PageCardCommands final : public PageRuntimeResources {
  struct Pending;

 public:
  PageCardCommands(PageEntityStateRuntime& graph, PageCardCommandPort& port) : graph_(graph), port_(port) {}
  PageCardCommands(const PageCardCommands&) = delete;
  PageCardCommands& operator=(const PageCardCommands&) = delete;
  ~PageCardCommands() override { assert(!dispatching_); }
  bool prepare(const PageStore& store) override {
    discard_prepared();
    if (!store.source_document() || !store.page_count() || activation_ == UINT64_MAX) return false;
    staged_ = store;
    return true;
  }
  void discard_prepared() override { staged_ = {}; }
  void activate_prepared(const PageStore& store) override {
    if (!staged_.source_document() ||
        !page_prepared_generation_matches(staged_.generation(), store.generation()))
      return;
    cancel();
    ++activation_;
    active_ = store;
    discard_prepared();
  }
  void retire_active() override {
    cancel();
    active_ = {};
  }
  bool request(PageBindingToken origin, uint32_t pressed_connection) {
    const ConfigurationHandle handle{origin.configuration, origin.card};
    const auto* card = active_.card(handle);
    auto kind = card ? classify(active_, *card) : PageCardCommandKind::NONE;
    if (kind == PageCardCommandKind::TOGGLE) {
      const auto policy = page_confirmation_policy(active_, handle);
      if (policy.enabled()) {
        const auto value = graph_.read_attribute(handle, active_.field(*card, CardField::ENTITY), {});
        if (value.status != PageEntityStatus::AVAILABLE || !page_confirmation_known(value.value))
          kind = PageCardCommandKind::NONE;
        else {
          const bool on = page_toggle_on(value.value);
          // Never bypass a required confirmation or reverse an unconfirmed
          // direction after the HA state changes while this intent is queued.
          kind = policy.choose(on) != PageConfirmationAction::NONE ? PageCardCommandKind::NONE
                 : policy.cover ? (on ? PageCardCommandKind::CLOSE : PageCardCommandKind::OPEN)
                 : on           ? PageCardCommandKind::TURN_OFF
                                : PageCardCommandKind::TURN_ON;
        }
      }
    }
    Pending next{origin, kind, local(kind) ? 0 : pressed_connection, true, {}, activation_};
    return enqueue(next);
  }
  bool request_percent(PageBindingToken origin, uint32_t connection, int percent) {
    PageCardCommandValue value;
    value.integer = percent;
    return request_control(origin, connection, PageCardCommandKind::PERCENT, value);
  }
  bool request_number(PageBindingToken origin, uint32_t connection, double number,
                      number_slider::Metadata metadata) {
    PageCardCommandValue value;
    value.number = number;
    value.metadata = metadata;
    return request_control(origin, connection, PageCardCommandKind::NUMBER, value);
  }
  bool request_temperature(PageBindingToken origin, uint32_t connection, int kelvin) {
    PageCardCommandValue value;
    value.integer = kelvin;
    return request_control(origin, connection, PageCardCommandKind::TEMPERATURE, value);
  }
  bool request_slider_toggle(PageBindingToken origin, uint32_t connection) {
    return request_control(origin, connection, PageCardCommandKind::SLIDER_TOGGLE, {});
  }
  bool request_timer(PageBindingToken origin, PageTimerAction action, PageTimerCommandProof proof) {
    if (action != PageTimerAction::START && action != PageTimerAction::CANCEL) {
      ++rejected_;
      return false;
    }
    PageCardCommandValue value;
    value.integer = proof.confirmed ? 1 : 0;
    value.write_revision = proof.state_revision;
    return request_control(origin, proof.connection,
                           action == PageTimerAction::START ? PageCardCommandKind::TIMER_START
                                                            : PageCardCommandKind::TIMER_CANCEL,
                           value);
  }
  bool request_media(PageBindingToken origin, uint32_t connection) {
    return request_control(origin, connection, PageCardCommandKind::MEDIA_ACTION, {});
  }
  bool request_seek(PageBindingToken origin, PageMediaSeekProof proof, uint8_t percent, float duration) {
    PageCardCommandValue value;
    value.integer = percent;
    value.number = duration;
    value.write_revision = proof.revision;
    return request_control(origin, proof.connection, PageCardCommandKind::MEDIA_SEEK, value);
  }
  bool request_device(PageBindingToken origin, uint32_t connection) {
    return request_control(origin, connection, PageCardCommandKind::DEVICE_ACTION, {});
  }
  static bool control(PageCardCommandKind kind) {
    return kind == PageCardCommandKind::PERCENT || kind == PageCardCommandKind::NUMBER ||
           kind == PageCardCommandKind::TEMPERATURE || kind == PageCardCommandKind::SLIDER_TOGGLE ||
           kind == PageCardCommandKind::TIMER_START || kind == PageCardCommandKind::TIMER_CANCEL ||
           kind == PageCardCommandKind::MEDIA_ACTION || kind == PageCardCommandKind::MEDIA_SEEK ||
           kind == PageCardCommandKind::DEVICE_ACTION;
  }

 private:
  bool request_control(PageBindingToken origin, uint32_t connection, PageCardCommandKind kind,
                       PageCardCommandValue value) {
    Pending next{origin, kind, connection, true, value, activation_};
    if (!binding_current(next)) {
      ++rejected_;
      return false;
    }
    if (!port_.admit_control(active_, origin, kind, next.value)) {
      ++rejected_;
      discard(next, false);
      return false;
    }
    return enqueue(next);
  }
  bool enqueue(const Pending& next) {
    if (!current(next) || count_ == pending_.size()) {
      ++rejected_;
      discard(next, false);
      return false;
    }
    pending_[(head_ + count_) % pending_.size()] = next;
    ++count_;
    return true;
  }

 public:
  void cancel() {
    while (count_) {
      const auto next = pop();
      discard(next);
      ++cancelled_;
    }
  }
  PageCardCommandResult loop() {
    if (dispatching_ || !port_.ready() || !count_) return PageCardCommandResult::NONE;
    const auto pending = pop();
    if (!current(pending)) {
      discard(pending);
      ++cancelled_;
      return PageCardCommandResult::CANCELLED;
    }
    const auto* card = active_.card({pending.origin.configuration, pending.origin.card});
    PageCardCommandSnapshot snapshot;
    snapshot.origin = pending.origin;
    snapshot.kind = pending.kind;
    snapshot.value = pending.value;
    snapshot.slot = card->slot;
    snapshot.root = pending.origin.page == 0;
    auto& config = snapshot.config;
    config.entity = active_.field(*card, CardField::ENTITY);
    config.label = active_.field(*card, CardField::LABEL);
    config.icon = active_.field(*card, CardField::ICON);
    config.icon_on = active_.field(*card, CardField::ICON_ON);
    config.sensor = active_.field(*card, CardField::SENSOR);
    config.unit = active_.field(*card, CardField::UNIT);
    config.type = active_.field(*card, CardField::TYPE);
    config.precision = active_.field(*card, CardField::PRECISION);
    config.options = active_.field(*card, CardField::OPTIONS);
    // Reentrant input can queue a later intent, never this one twice.
    dispatching_ = true;
    const bool typed = control(pending.kind);
    bool sent = true;
    if (typed)
      sent = port_.dispatch_control(snapshot);
    else
      port_.dispatch(snapshot);
    if (typed && sent && active_.source_document() && pending.activation == activation_ &&
        pending.connection == graph_.connection_generation())
      port_.accepted_control(pending.origin, pending.kind, pending.value);
    dispatching_ = false;
    if (!sent) {
      discard(pending);
      ++failed_;
      return PageCardCommandResult::FAILED;
    }
    ++dispatched_;
    return PageCardCommandResult::DISPATCHED;
  }
  PageCardCommandMemory memory() const {
    return {sizeof(*this), count_, dispatched_, cancelled_, rejected_, failed_};
  }
  static bool local(PageCardCommandKind kind) {
    return kind == PageCardCommandKind::LOCAL || kind == PageCardCommandKind::RELAY ||
           kind == PageCardCommandKind::WEBHOOK || kind == PageCardCommandKind::SCREENSAVER ||
           kind == PageCardCommandKind::SCREEN_LOCK;
  }
  static PageCardCommandKind classify(const PageStore& store, const CardDefinition& card) {
    const auto type = store.field(card, CardField::TYPE), entity = store.field(card, CardField::ENTITY),
               mode = store.field(card, CardField::SENSOR);
    if (type == "screensaver") return PageCardCommandKind::SCREENSAVER;
    if (type == "screen_lock") return PageCardCommandKind::SCREEN_LOCK;
    if (type == "push") return PageCardCommandKind::PUSH;
    if (entity.empty()) return PageCardCommandKind::NONE;
    if (type == "webhook") return PageCardCommandKind::WEBHOOK;
    if (type == "internal") return PageCardCommandKind::RELAY;
    if (type == "local" || (type == "action" && mode == "local")) return PageCardCommandKind::LOCAL;
    if (type.empty() || type == "light_switch")
      return entity.size() > 7 && entity.substr(0, 7) == "button." ? PageCardCommandKind::BUTTON
                                                                   : PageCardCommandKind::TOGGLE;
    if (type != "action" ||
        PageConfirmationPolicy::fields(type, entity, mode, store.field(card, CardField::OPTIONS)).enabled())
      return PageCardCommandKind::NONE;
    // Selectors open a demanded panel, never the simple action route.
    if (mode == "scene.turn_on" || mode == "script.turn_on" || mode == "automation.trigger" ||
        mode == "button.press" || mode == "vacuum.start" || mode == "vacuum.return_to_base" ||
        mode == "input_button.press" || mode == "input_boolean.toggle" || mode == "input_boolean.turn_on" ||
        mode == "input_boolean.turn_off" ||
        ((mode == "number.set_value" || mode == "input_number.set_value") &&
         !store.field(card, CardField::UNIT).empty()))
      return PageCardCommandKind::ACTION;
    return PageCardCommandKind::NONE;
  }

 private:
  struct Pending {
    PageBindingToken origin;
    PageCardCommandKind kind{PageCardCommandKind::NONE};
    uint32_t connection{0};
    bool present{false};
    PageCardCommandValue value;
    uint64_t activation{0};
  };
  Pending pop() {
    const auto next = pending_[head_];
    pending_[head_] = {};
    head_ = (head_ + 1) % pending_.size();
    --count_;
    return next;
  }
  void discard(const Pending& next, bool honor_pending = true) {
    if (!control(next.kind) || !active_.source_document() || next.activation != activation_) return;
    for (size_t i = 0; honor_pending && i < count_; ++i) {
      const auto& newer = pending_[(head_ + i) % pending_.size()];
      if (newer.kind == next.kind && newer.activation == next.activation &&
          newer.origin.configuration == next.origin.configuration && newer.origin.card == next.origin.card)
        return;
    }
    port_.discard_control(next.origin, next.kind, next.value);
  }
  bool binding_current(const Pending& pending) const {
    const auto& origin = pending.origin;
    if (!pending.present || pending.kind == PageCardCommandKind::NONE || !origin.binding || origin.back ||
        pending.activation != activation_ || !active_.source_document() || !port_.valid(origin))
      return false;
    const auto* card = active_.card({origin.configuration, origin.card});
    const auto* page = active_.page(origin.page);
    if (!card || !page || !page->navigable || origin.card < page->first_card ||
        origin.card - page->first_card >= page->card_count)
      return false;
    return true;
  }
  bool common_current(const Pending& pending) const {
    if (!binding_current(pending)) return false;
    return local(pending.kind) || (port_.connected() && port_.service_available() &&
                                   pending.connection == graph_.connection_generation());
  }
  bool current(const Pending& pending) const {
    if (!common_current(pending)) return false;
    const auto& origin = pending.origin;
    if (control(pending.kind)) return port_.valid_control(active_, origin, pending.kind, pending.value);
    const auto* card = active_.card({origin.configuration, origin.card});
    auto kind = classify(active_, *card);
    if (pending.kind == PageCardCommandKind::TURN_ON || pending.kind == PageCardCommandKind::TURN_OFF ||
        pending.kind == PageCardCommandKind::OPEN || pending.kind == PageCardCommandKind::CLOSE) {
      kind = kind == PageCardCommandKind::TOGGLE ? pending.kind : PageCardCommandKind::NONE;
      const auto value = graph_.read_attribute({origin.configuration, origin.card},
                                               active_.field(*card, CardField::ENTITY), {});
      if (value.status != PageEntityStatus::AVAILABLE || !page_confirmation_known(value.value)) return false;
    }
    return kind == pending.kind;
  }
  PageEntityStateRuntime& graph_;
  PageCardCommandPort& port_;
  PageStore active_, staged_;
  std::array<Pending, 8> pending_{};
  size_t head_{0}, count_{0};
  bool dispatching_{false};
  uint64_t activation_{0};
  size_t dispatched_{0}, cancelled_{0}, rejected_{0}, failed_{0};
};
}  // namespace espcontrol::pages
