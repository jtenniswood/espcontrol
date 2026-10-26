#pragma once

#include <cassert>
#include <string>

#include "button_grid_subpage_parser.h"
#include "page_confirmation_policy.h"
#include "page_entity_state_runtime.h"
#include "page_fan_mode.h"
#include "page_modal_runtime.h"

namespace espcontrol::pages {
class PageConfirmationPort {
 public:
  virtual ~PageConfirmationPort() = default;
  // Read-only admission queries. Request/cancel/close only mutate scalar modal
  // state; they never pump callbacks. Send alone may reenter or replace config.
  virtual bool valid(PageBindingToken) const = 0;
  virtual bool valid(PageModalToken) const = 0;
  virtual bool connected() const = 0;
  virtual bool service_available() const = 0;
  virtual bool ready() const = 0;
  virtual uint64_t request(PageBindingToken) = 0;
  virtual bool pending_request(uint64_t) const = 0;
  virtual void cancel_request(uint64_t) = 0;
  virtual void close(PageModalToken) = 0;
  // Configuration is owned for this entire call, even if send retires the UI.
  virtual bool send(PageConfirmationAction, const ParsedCfg&) = 0;
};
struct PageConfirmationPlan {
  PageBindingToken source;
  PageModalToken modal;
  uint32_t connection{0};
  PageConfirmationAction action{PageConfirmationAction::NONE};
};
enum class PageConfirmationResult : uint8_t { NONE, CANCELLED, FAILED, SENT };
struct PageConfirmationMemory {
  size_t fixed_bytes{0}, pending_requests{0}, contexts{0}, queued{0}, sent{0}, cancelled{0}, failed{0};
};
// Exactly one pending opening and one demanded borrowed context. There are no
// per-configured-card records, timers, subscriptions, callbacks or widgets.
// Retire before the graph/store, and unbind before freeing the context's config.
class PageConfirmationRuntime final : public PageRuntimeResources {
 public:
  PageConfirmationRuntime(PageEntityStateRuntime& graph, PageConfirmationPort& port)
      : graph_(graph), port_(port) {}
  PageConfirmationRuntime(const PageConfirmationRuntime&) = delete;
  PageConfirmationRuntime& operator=(const PageConfirmationRuntime&) = delete;
  ~PageConfirmationRuntime() override {
    assert(!dispatching_);
    cancel();
  }
  bool prepare(const PageStore& store) override {
    discard_prepared();
    if (!store.source_document()) return false;
    staged_ = store;
    return true;
  }
  void discard_prepared() override {
    staged_ = {};
  }
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
  // The card owner supplies its coherent displayed state and the HA epoch
  // captured on PRESSED. Maintenance must never recompute/reverse this target.
  bool request(PageBindingToken source, uint32_t pressed_connection, bool currently_on) {
    const ConfigurationHandle card{source.configuration, source.card};
    const auto policy = page_confirmation_policy(active_, card);
    const auto action = policy.choose(currently_on);
    if (!source.binding || source.back || action == PageConfirmationAction::NONE || !port_.valid(source) ||
        !port_.connected() || !port_.service_available() ||
        pressed_connection != graph_.connection_generation() || !fresh(card, policy))
      return false;
    const auto opening = port_.request(source);
    if (!opening) return false;
    pending_ = {source,
                {source.configuration, opening, source.card, ControlModalKind::SWITCH_CONFIRMATION},
                pressed_connection,
                action};
    return true;
  }
  PageConfirmationPlan plan(PageModalToken token) const {
    return same(pending_.modal, token) ? pending_ : PageConfirmationPlan{};
  }
  bool bind(PageModalToken token, const ParsedCfg& owned) {
    if (!same(pending_.modal, token) || !port_.valid(token) || !port_.connected() ||
        pending_.connection != graph_.connection_generation())
      return false;
    const ConfigurationHandle handle{token.configuration, token.card};
    const auto* card = active_.card(handle);
    if (!card || owned.entity != active_.field(*card, CardField::ENTITY) ||
        owned.type != active_.field(*card, CardField::TYPE) ||
        owned.sensor != active_.field(*card, CardField::SENSOR) ||
        owned.options != active_.field(*card, CardField::OPTIONS) ||
        owned.unit != active_.field(*card, CardField::UNIT))
      return false;
    record_ = {pending_, &owned, false, false};
    pending_ = {};
    return true;
  }
  bool queue(PageModalToken token, uint32_t pressed_connection) {
    if (!same(record_.plan.modal, token) || record_.queued || record_.sent || !current(record_.plan) ||
        pressed_connection != record_.plan.connection)
      return false;
    record_.queued = true;
    return true;
  }
  bool pending(PageModalToken token) const {
    return same(record_.plan.modal, token) && record_.queued && current(record_.plan);
  }
  void cancel(PageModalToken token) {
    if (same(pending_.modal, token)) {
      const auto opening = pending_.modal.opening;
      pending_ = {};
      port_.cancel_request(opening);
    }
    if (same(record_.plan.modal, token)) {
      if (record_.queued) ++cancelled_;
      record_ = {};
    }
  }
  void cancel() {
    const auto opening = pending_.modal.opening;
    pending_ = {};
    if (record_.queued) ++cancelled_;
    record_ = {};
    if (opening) port_.cancel_request(opening);
  }
  // A temporary display/wake guard cancels unsent intent but keeps the demanded
  // panel bound. A later fresh press can confirm without reopening that panel.
  void cancel_queued() {
    if (record_.queued) { record_.queued = false; ++cancelled_; }
  }
  PageConfirmationResult loop() {
    if (dispatching_ || !port_.ready()) return PageConfirmationResult::NONE;
    if (pending_.modal.opening && !port_.pending_request(pending_.modal.opening) &&
        !port_.valid(pending_.modal))
      pending_ = {};
    if (!record_.plan.modal.opening) return PageConfirmationResult::NONE;
    const auto plan = record_.plan;
    if (!current(plan)) {
      const bool queued = record_.queued;
      record_ = {};
      if (queued) ++cancelled_;
      port_.close(plan.modal);
      return queued ? PageConfirmationResult::CANCELLED : PageConfirmationResult::NONE;
    }
    if (!record_.queued) return PageConfirmationResult::NONE;
    if (!port_.service_available()) {
      record_.queued = false;
      record_.sent = true;
      ++failed_;
      port_.close(plan.modal);
      return PageConfirmationResult::FAILED;
    }
    // Retiring/rebinding in send cannot invalidate the bytes being dispatched.
    const ParsedCfg config = *record_.config;
    record_.queued = false;
    record_.sent = true;
    dispatching_ = true;
    const bool sent = port_.send(plan.action, config);
    dispatching_ = false;
    if (sent)
      ++sent_;
    else
      ++failed_;
    if (same(record_.plan.modal, plan.modal)) port_.close(plan.modal);
    return sent ? PageConfirmationResult::SENT : PageConfirmationResult::FAILED;
  }
  PageConfirmationMemory memory() const {
    return {sizeof(*this),
            pending_.modal.opening ? 1u : 0u,
            record_.plan.modal.opening ? 1u : 0u,
            record_.queued ? 1u : 0u,
            sent_,
            cancelled_,
            failed_};
  }

 private:
  struct Record {
    PageConfirmationPlan plan;
    const ParsedCfg* config{nullptr};
    bool queued{false}, sent{false};
  };
  static bool same(PageModalToken a, PageModalToken b) {
    return a.opening && a.opening == b.opening && a.configuration == b.configuration && a.card == b.card &&
           a.kind == ControlModalKind::SWITCH_CONFIRMATION && b.kind == a.kind;
  }
  bool fresh(ConfigurationHandle handle, PageConfirmationPolicy policy) const {
    if (!policy.toggle) return true;  // Explicit garage direction/script requires no state subscription.
    const auto* card = active_.card(handle);
    if (!card) return false;
    const auto entity = active_.field(*card, CardField::ENTITY);
    const auto value = graph_.read_attribute(handle, entity, {});
    return value.status == PageEntityStatus::AVAILABLE && page_confirmation_known(value.value);
  }
  bool current(const PageConfirmationPlan& plan) const {
    return plan.modal.opening && active_.source_document() &&
           active_.generation() == plan.modal.configuration && port_.valid(plan.modal) && port_.connected() &&
           plan.connection == graph_.connection_generation() &&
           fresh({plan.modal.configuration, plan.modal.card},
                 page_confirmation_policy(active_, {plan.modal.configuration, plan.modal.card}));
  }
  PageEntityStateRuntime& graph_;
  PageConfirmationPort& port_;
  PageStore active_, staged_;
  PageConfirmationPlan pending_;
  Record record_;
  bool dispatching_{false};
  size_t sent_{0}, cancelled_{0}, failed_{0};
};
}  // namespace espcontrol::pages
