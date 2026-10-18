#pragma once

#include <string>

#include "guest_wifi_state.h"
#include "page_entity_state_runtime.h"
#include "page_modal_runtime.h"

namespace espcontrol::pages {
class PageGuestWifiPort {
 public:
  virtual ~PageGuestWifiPort() = default;
  // Validation is read-only. Only send may deliver/reenter or replace config;
  // the runtime owns its entity bytes through that call.
  virtual bool valid(PageModalToken) const = 0;
  virtual bool connected() const = 0;
  virtual bool service_available() const = 0;
  virtual bool ready() const = 0;
  virtual bool send(std::string_view entity, bool turn_on) = 0;
};
struct PageGuestWifiSnapshot {
  bool bound{false}, known{false}, on{false}, pending{false}, failed{false}, target_on{false};
  const char* status{"unavailable"};
};
enum class PageGuestWifiDispatch : uint8_t { NONE, CANCELLED, FAILED, SENT };
// One demanded guest control, never per-card state. The graph already registers
// the guest switch even while closed. Poll its bounded value; no observer,
// subscription or timer is added. Entity belongs to the modal's owned context:
// unbind before that context is destroyed, even after admission is revoked.
class PageGuestWifiRuntime {
 public:
  using Now = uint32_t (*)();
  PageGuestWifiRuntime(PageEntityStateRuntime& graph, PageGuestWifiPort& port, Now now,
                       uint64_t first_command = 1)
      : graph_(graph), port_(port), now_(now), next_command_(first_command) {}
  PageGuestWifiRuntime(const PageGuestWifiRuntime&) = delete;
  PageGuestWifiRuntime& operator=(const PageGuestWifiRuntime&) = delete;
  bool bind(PageModalToken token, std::string_view entity) {
    if (!now_ || !token.opening || token.card == NO_CARD || token.kind != ControlModalKind::WIFI_QR ||
        !port_.valid(token) || !guest_wifi_valid_entity(entity))
      return false;
    if (graph_.read_attribute({token.configuration, token.card}, entity, {}).entity != entity) return false;
    if (same(record_.token, token)) return record_.entity == entity;
    record_ = {};
    record_.token = token;
    record_.entity = entity;
    record_.connection = graph_.connection_generation();
    record_.serial = ++serial_;
    ++revision_;
    poll();
    return true;
  }
  void unbind(PageModalToken token) {
    if (same(record_.token, token)) retire();
  }
  void retire() {
    record_ = {};
    ++serial_;
    ++revision_;
  }
  // Connection comes from the original press, not a later click/dispatch.
  bool queue(PageModalToken token, uint32_t pressed_connection) {
    if (!next_command_ || !same(record_.token, token) || !port_.valid(token) || !port_.connected() ||
        !port_.service_available() || pressed_connection != graph_.connection_generation())
      return false;
    poll();
    if (!same(record_.token, token) || record_.connection != pressed_connection ||
        !record_.state.begin(now_()))
      return false;
    record_.command = next_command_++;
    record_.queued = true;
    ++revision_;
    return true;
  }
  PageGuestWifiDispatch loop() {
    if (dispatching_ || !record_.token.opening) return PageGuestWifiDispatch::NONE;
    const bool queued = record_.queued;
    if (!port_.valid(record_.token)) {
      retire();
      return queued ? PageGuestWifiDispatch::CANCELLED : PageGuestWifiDispatch::NONE;
    }
    poll();
    auto& r = record_;
    if (!r.token.opening) return queued ? PageGuestWifiDispatch::CANCELLED : PageGuestWifiDispatch::NONE;
    const auto before = r.state.pending;
    r.state.tick(now_());
    if (before != r.state.pending) ++revision_;
    if (!r.queued) return queued ? PageGuestWifiDispatch::CANCELLED : PageGuestWifiDispatch::NONE;
    if (!r.state.pending || !r.state.known) {
      r.queued = false;
      ++revision_;
      return PageGuestWifiDispatch::CANCELLED;
    }
    if (!port_.connected() || r.connection != graph_.connection_generation()) {
      r.queued = false;
      r.state.disconnect();
      ++revision_;
      return PageGuestWifiDispatch::CANCELLED;
    }
    if (!port_.service_available()) {
      r.queued = false;
      r.state.send_failed();
      ++revision_;
      return PageGuestWifiDispatch::FAILED;
    }
    if (!port_.ready()) return PageGuestWifiDispatch::NONE;
    const auto token = r.token;
    const auto serial = r.serial;
    const auto command = r.command;
    const bool target = r.state.target_on;
    const std::string entity(r.entity);  // Maintenance only; survives reentrant retirement.
    r.queued = false;
    dispatching_ = true;
    const bool sent = port_.send(entity, target);
    dispatching_ = false;
    if (!sent && record_.serial == serial && record_.command == command && same(record_.token, token)) {
      record_.state.send_failed();
      ++revision_;
    }
    return sent ? PageGuestWifiDispatch::SENT : PageGuestWifiDispatch::FAILED;
  }
  PageGuestWifiSnapshot snapshot(PageModalToken token) const {
    if (!same(record_.token, token) || !port_.valid(token)) return {};
    if (!port_.connected() || record_.connection != graph_.connection_generation()) return {true};
    const auto& s = record_.state;
    return {true, s.known, s.on, s.pending, s.failed, s.target_on, s.status_key()};
  }
  uint64_t revision() const {
    return revision_;
  }

 private:
  struct Record {
    PageModalToken token;
    std::string_view entity;
    GuestWifiState state;
    uint32_t connection{0}, seen{0};
    uint64_t serial{0}, command{0};
    PageEntityStatus status{PageEntityStatus::STALE};
    bool seeded{false}, queued{false}, online{false};
  };
  static bool same(PageModalToken a, PageModalToken b) {
    return a.opening && a.opening == b.opening && a.configuration == b.configuration && a.card == b.card &&
           a.kind == b.kind;
  }
  void poll() {
    auto& r = record_;
    if (!r.token.opening || !port_.valid(r.token)) return;
    const auto epoch = graph_.connection_generation();
    const bool online = port_.connected();
    if (r.connection != epoch || r.online != online) {
      r.connection = epoch;
      r.online = online;
      r.state.disconnect();
      r.queued = false;
      r.seeded = false;
      ++revision_;
    }
    if (!online) return;
    const auto value = graph_.read_attribute({r.token.configuration, r.token.card}, r.entity, {});
    if (r.seeded && value.revision == r.seen && value.status == r.status) return;
    r.seeded = true;
    r.seen = value.revision;
    r.status = value.status;
    if (value.status == PageEntityStatus::AVAILABLE)
      r.state.receive(value.value);
    else
      r.state.disconnect();
    ++revision_;
  }
  PageEntityStateRuntime& graph_;
  PageGuestWifiPort& port_;
  Now now_;
  Record record_;
  uint64_t serial_{0}, revision_{0}, next_command_{1};
  bool dispatching_{false};
};
}  // namespace espcontrol::pages
