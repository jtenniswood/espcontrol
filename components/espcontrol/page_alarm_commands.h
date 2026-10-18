#pragma once

#include "page_alarm_ack_runtime.h"
#include "page_modal_runtime.h"

namespace espcontrol::pages {
// Exactly one opening or grid binding. Connection is captured with the input,
// never sampled later to adopt an old intent into a new connection.
struct PageAlarmCommandOrigin {
  PageBindingToken binding;
  PageModalToken modal;
  uint32_t connection{0};
  ConfigurationHandle card() const {
    return modal.opening ? ConfigurationHandle{modal.configuration, modal.card}
                         : ConfigurationHandle{binding.configuration, binding.card};
  }
};
class PageAlarmCommandPort {
 public:
  virtual ~PageAlarmCommandPort() = default;
  // Validation/readiness methods are read-only. Only send may reenter producers
  // or replace configuration; borrowed entity/code views last through that call.
  virtual bool ready() const = 0;
  virtual bool connected() const = 0;
  virtual bool service_available() const = 0;
  virtual bool valid(PageBindingToken) const = 0;
  virtual bool valid(PageModalToken) const = 0;
  virtual bool send(std::string_view entity, const char* service, std::string_view code) = 0;
};
enum class PageAlarmCommandResult : uint8_t { NONE, CANCELLED, FAILED, SENT };
struct PageAlarmCommandMemory {
  size_t fixed_bytes{0}, pending{0}, pending_code_bytes{0};
  size_t sent{0}, cancelled{0}, rejected{0};
};
// One fixed deferred command, no timers, subscriptions or per-card records.
// Put before acknowledgements/state/graph in the application resource group.
// Call loop on the main loop outside native/coordinator/LVGL dispatch.
class PageAlarmCommands final : public PageRuntimeResources {
 public:
  PageAlarmCommands(PageEntityStateRuntime&, PageAlarmStateRuntime&, PageAlarmAckRuntime&,
                    PageAlarmCommandPort&);
  ~PageAlarmCommands() override;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  // PIN entry belongs to the demanded UI. Queue only a completed code. A valid
  // later command replaces the previous one; invalid input leaves it intact.
  bool queue(PageAlarmCommandOrigin, alarm::Action, std::string_view code = {});
  void cancel();
  PageAlarmCommandResult loop();
  PageAlarmCommandMemory memory() const;

 private:
  struct Pending {
    PageAlarmCommandOrigin origin;
    alarm::Action action{alarm::Action::NONE};
    char code[17]{};
    uint8_t code_size{0};
    bool present{false};
  };
  bool current(const Pending&);
  static void wipe(Pending&);
  PageEntityStateRuntime& graph_;
  PageAlarmStateRuntime& state_;
  PageAlarmAckRuntime& acknowledgements_;
  PageAlarmCommandPort& port_;
  PageStore active_, staged_;
  Pending pending_;
  bool dispatching_{false};
  size_t sent_{0}, cancelled_{0}, rejected_{0};
};
}  // namespace espcontrol::pages
