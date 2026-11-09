#include "page_alarm_presentation.h"

#include <cassert>

namespace espcontrol::pages {
PageAlarmPresentation::PageAlarmPresentation(PageAlarmStateRuntime& state, PageAlarmAckRuntime& ack,
                                             PageModalRuntime& modal, PageAlarmPresentationPort& port)
    : state_(state), acknowledgements_(ack), modal_(modal), port_(port) {}
PageAlarmPresentation::~PageAlarmPresentation() {
  assert(!running_);
  retire_active();
  loop();
  assert(!critical_);
}
bool PageAlarmPresentation::prepare(const PageStore& store) {
  discard_prepared();
  if (!store.source_document() || !store.page_count()) return false;
  staged_generation_ = store.generation();
  staged_ = true;
  return true;
}
void PageAlarmPresentation::discard_prepared() {
  staged_ = false;
  staged_generation_ = 0;
}
void PageAlarmPresentation::activate_prepared(const PageStore& store) {
  if (!staged_ || !page_prepared_generation_matches(staged_generation_, store.generation())) return;
  generation_ = store.generation();
  active_ = true;
  ++publication_;
  discard_prepared();
  // The modal owner cancels the old document as part of this same transaction.
  // Display effects and new UI requests wait for safe maintenance.
  choice_ = false;
}
void PageAlarmPresentation::retire_active() {
  active_ = false;
  choice_ = false;
  ++publication_;
  discard_prepared();
}
bool PageAlarmPresentation::same_origin(ConfigurationHandle origin) const {
  return choice_ && origin_.generation == origin.generation && origin_.index == origin.index;
}
void PageAlarmPresentation::set_critical(bool value) {
  if (critical_ == value) return;
  critical_ = value;
  port_.critical(value);
}
void PageAlarmPresentation::reset_tracking(bool close_auto) {
  const auto token = modal_.token();
  if (pending_ && token.opening == pending_ && modal_.valid(token)) {
    watched_ = token;
    auto_opened_ = true;
  }
  modal_.cancel_request(pending_);
  pending_ = 0;
  if (close_auto && auto_opened_ && modal_.valid(watched_) &&
      modal_.dismiss_policy(watched_) == ControlModalDismissPolicy::PRESERVE_DURING_DISPLAY_TAKEOVER)
    modal_.notify_closed(watched_, PageModalClose::TAKEOVER);
  watched_ = {};
  auto_opened_ = false;
  attempted_ = false;
  suppressed_ = false;
}
void PageAlarmPresentation::loop() {
  if (running_) return;
  state_.loop();
  acknowledgements_.loop();
  if (!port_.ready()) return;
  struct Guard {
    bool& flag;
    explicit Guard(bool& f) : flag(f) {
      flag = true;
    }
    ~Guard() {
      flag = false;
    }
  } guard(running_);
  const auto publication = publication_;
  auto selected = active_ ? state_.takeover() : PageAlarmTakeover{};
  if (selected.valid && selected.origin.generation != generation_) selected = {};
  if (!selected.valid) {
    reset_tracking(true);
    choice_ = false;
    set_critical(false);
  } else {
    const auto connection = port_.connection_generation();
    const auto events = state_.takeover_events();
    const bool changed = !same_origin(selected.origin), reconnected = connection_ != connection;
    const bool refreshed = changed || reconnected || events_ != events;
    if (changed) {
      reset_tracking(true);
      origin_ = selected.origin;
      choice_ = true;
    }
    if (reconnected) {
      modal_.cancel_request(pending_);
      pending_ = 0;
      watched_ = {};
      auto_opened_ = false;
      attempted_ = false;
    }
    if (refreshed) {
      suppressed_ = false;
      attempted_ = false;
    }
    connection_ = connection;
    events_ = events;
    const auto token = modal_.token();
    const bool current = token.configuration == origin_.generation && token.card == origin_.index &&
                         token.kind == ControlModalKind::ALARM_CONTROL && modal_.valid(token);
    if (current) {
      if (pending_ && token.opening == pending_) {
        auto_opened_ = true;
        pending_ = 0;
      } else if (watched_.opening != token.opening) {
        auto_opened_ = false;
        if (modal_.pending_request() != pending_) pending_ = 0;
      }
      watched_ = token;
      attempted_ = true;
    } else {
      const auto retired = modal_.last_retirement();
      const auto closed_after_report = [&](uint64_t opening) {
        return opening && retired.token.opening == opening && retired.source_revision == events &&
               (retired.reason == PageModalClose::USER || retired.reason == PageModalClose::SWITCH ||
                retired.reason == PageModalClose::TAKEOVER);
      };
      if (watched_.opening) {
        if (closed_after_report(watched_.opening)) {
          suppressed_ = retired.policy == ControlModalDismissPolicy::PRESERVE_DURING_DISPLAY_TAKEOVER;
          attempted_ = true;
        } else if (retired.token.opening == watched_.opening &&
                   retired.reason == PageModalClose::DISCONNECTED)
          attempted_ = false;
        else if (!refreshed && retired.token.opening == watched_.opening &&
                 (retired.reason == PageModalClose::USER || retired.reason == PageModalClose::SWITCH ||
                  retired.reason == PageModalClose::TAKEOVER)) {
          suppressed_ = retired.policy == ControlModalDismissPolicy::PRESERVE_DURING_DISPLAY_TAKEOVER;
          attempted_ = true;
        }
        watched_ = {};
        auto_opened_ = false;
      }
      if (pending_ && modal_.pending_request() != pending_) {
        if (closed_after_report(pending_))
          suppressed_ = retired.policy == ControlModalDismissPolicy::PRESERVE_DURING_DISPLAY_TAKEOVER;
        else if (modal_.pending_request() || (token.opening && modal_.valid(token)))
          suppressed_ = true;
        pending_ = 0;
        // A close after the last report consumes that event. A newer report
        // after the close may retry immediately; failed setup waits for one.
        if (retired.source_revision == events || !refreshed) attempted_ = true;
      }
      if (!suppressed_ && port_.connected() && !attempted_) {
        attempted_ = true;
        if (modal_.request_card(origin_, ControlModalKind::ALARM_CONTROL)) {
          pending_ = modal_.pending_request();
          ++requests_;
        }
      }
    }
    set_critical(!suppressed_);
  }
  if (publication_ != publication || !active_ || !port_.connected()) return;
  const auto failure = acknowledgements_.take_failure();
  if (failure.valid && failure.origin.generation == generation_ &&
      failure.connection == port_.connection_generation()) {
    ++feedback_;
    port_.failure(failure);
  }
}
PageAlarmPresentationMemory PageAlarmPresentation::memory() const {
  return {sizeof(*this), feedback_, requests_, critical_, suppressed_};
}
}  // namespace espcontrol::pages
