#pragma once

#include <array>
#include <cassert>

#include "button_grid_subpage_parser.h"
#include "page_confirmation_policy.h"
#include "page_entity_state_runtime.h"
#include "page_renderer_runtime.h"
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
  SCREEN_LOCK
};
struct PageCardCommandSnapshot {
  PageBindingToken origin;
  PageCardCommandKind kind{PageCardCommandKind::NONE};
  ParsedCfg config;
  uint16_t slot{0};
  bool root{false};
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
};
enum class PageCardCommandResult : uint8_t { NONE, CANCELLED, DISPATCHED };
struct PageCardCommandMemory {
  size_t fixed_bytes{0}, pending{0}, dispatched{0}, cancelled{0}, rejected{0};
};

// A fixed FIFO of basic-card intents, independent of configured card count. The
// native adapter consumes existing service/local/webhook policies in dispatch.
// Control/slider/media/alarm intents are outside this basic command route.
class PageCardCommands final : public PageRuntimeResources {
 public:
  PageCardCommands(PageEntityStateRuntime& graph, PageCardCommandPort& port) : graph_(graph), port_(port) {}
  PageCardCommands(const PageCardCommands&) = delete;
  PageCardCommands& operator=(const PageCardCommands&) = delete;
  ~PageCardCommands() override { assert(!dispatching_); }
  bool prepare(const PageStore& store) override {
    discard_prepared();
    if (!store.source_document() || !store.page_count()) return false;
    staged_ = store;
    return true;
  }
  void discard_prepared() override { staged_ = {}; }
  void activate_prepared(const PageStore& store) override {
    if (!staged_.source_document() ||
        !page_prepared_generation_matches(staged_.generation(), store.generation()))
      return;
    cancel();
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
    Pending next{origin, kind, local(kind) ? 0 : pressed_connection, true};
    if (!current(next) || count_ == pending_.size()) {
      ++rejected_;
      return false;
    }
    pending_[(head_ + count_) % pending_.size()] = next;
    ++count_;
    return true;
  }
  void cancel() {
    cancelled_ += count_;
    pending_ = {};
    head_ = count_ = 0;
  }
  PageCardCommandResult loop() {
    if (dispatching_ || !port_.ready() || !count_) return PageCardCommandResult::NONE;
    const auto pending = pending_[head_];
    pending_[head_] = {};
    head_ = (head_ + 1) % pending_.size();
    --count_;
    if (!current(pending)) {
      ++cancelled_;
      return PageCardCommandResult::CANCELLED;
    }
    const auto* card = active_.card({pending.origin.configuration, pending.origin.card});
    PageCardCommandSnapshot snapshot;
    snapshot.origin = pending.origin;
    snapshot.kind = pending.kind;
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
    port_.dispatch(snapshot);
    dispatching_ = false;
    ++dispatched_;
    return PageCardCommandResult::DISPATCHED;
  }
  PageCardCommandMemory memory() const { return {sizeof(*this), count_, dispatched_, cancelled_, rejected_}; }
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
  };
  bool current(const Pending& pending) const {
    const auto& origin = pending.origin;
    if (!pending.present || pending.kind == PageCardCommandKind::NONE || !origin.binding || origin.back ||
        !port_.valid(origin))
      return false;
    const auto* card = active_.card({origin.configuration, origin.card});
    const auto* page = active_.page(origin.page);
    if (!card || !page || !page->navigable || origin.card < page->first_card ||
        origin.card - page->first_card >= page->card_count)
      return false;
    auto kind = classify(active_, *card);
    if (pending.kind == PageCardCommandKind::TURN_ON || pending.kind == PageCardCommandKind::TURN_OFF ||
        pending.kind == PageCardCommandKind::OPEN || pending.kind == PageCardCommandKind::CLOSE) {
      kind = kind == PageCardCommandKind::TOGGLE ? pending.kind : PageCardCommandKind::NONE;
      const auto value = graph_.read_attribute({origin.configuration, origin.card},
                                               active_.field(*card, CardField::ENTITY), {});
      if (value.status != PageEntityStatus::AVAILABLE || !page_confirmation_known(value.value)) return false;
    }
    return kind == pending.kind && (local(kind) || (port_.connected() && port_.service_available() &&
                                                    pending.connection == graph_.connection_generation()));
  }
  PageEntityStateRuntime& graph_;
  PageCardCommandPort& port_;
  PageStore active_, staged_;
  std::array<Pending, 8> pending_{};
  size_t head_{0}, count_{0};
  bool dispatching_{false};
  size_t dispatched_{0}, cancelled_{0}, rejected_{0};
};
}  // namespace espcontrol::pages
