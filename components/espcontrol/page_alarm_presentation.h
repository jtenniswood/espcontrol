#pragma once

#include "page_alarm_ack_runtime.h"
#include "page_modal_runtime.h"

namespace espcontrol::pages {
class PageAlarmPresentationPort {
 public:
  virtual ~PageAlarmPresentationPort() = default;
  virtual bool ready() const = 0;
  virtual bool connected() const = 0;
  virtual uint32_t connection_generation() const = 0;
  // One scene-owned critical display lease, independent of the grid binding.
  // These callbacks run only from safe main-loop maintenance, never activation.
  virtual void critical(bool) = 0;
  virtual void failure(PageAlarmFailure) = 0;
};
struct PageAlarmPresentationMemory {
  size_t fixed_bytes{0}, feedback{0}, modal_requests{0};
  bool critical{false}, suppressed{false};
};
// Pump before the shared modal owner, then present the frame. It does not
// suspend the renderer: the application scene reconciles actual modal visibility.
class PageAlarmPresentation final : public PageRuntimeResources {
 public:
  PageAlarmPresentation(PageAlarmStateRuntime&, PageAlarmAckRuntime&, PageModalRuntime&,
                        PageAlarmPresentationPort&);
  ~PageAlarmPresentation() override;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void loop();
  PageAlarmPresentationMemory memory() const;

 private:
  void reset_tracking(bool close_auto);
  void set_critical(bool);
  bool same_origin(ConfigurationHandle) const;
  PageAlarmStateRuntime& state_;
  PageAlarmAckRuntime& acknowledgements_;
  PageModalRuntime& modal_;
  PageAlarmPresentationPort& port_;
  ConfigurationHandle origin_;
  PageModalToken watched_;
  uint64_t pending_{0}, events_{0}, publication_{0};
  uint32_t generation_{0}, staged_generation_{0}, connection_{0};
  bool active_{false}, staged_{false}, choice_{false}, critical_{false}, suppressed_{false},
      attempted_{false}, auto_opened_{false}, running_{false};
  size_t feedback_{0}, requests_{0};
};
}  // namespace espcontrol::pages
