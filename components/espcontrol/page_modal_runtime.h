#pragma once

#include "control_modal_service.h"
#include "modal_callback_dispatch.h"
#include "page_renderer_runtime.h"

namespace espcontrol::pages {
struct PageModalToken {
  uint32_t configuration{0};
  uint64_t opening{0};
  CardId card{NO_CARD};
  ControlModalKind kind{ControlModalKind::NONE};
};
enum class PageModalClose : uint8_t { USER, SWITCH, DISCONNECTED, REPLACED, TAKEOVER, FAILED, SHUTDOWN };
struct PageModalRetirement {
  PageModalToken token;
  PageModalClose reason{PageModalClose::SHUTDOWN};
  ControlModalDismissPolicy policy{ControlModalDismissPolicy::DISMISS};
  uint64_t source_revision{0};
};
struct PageModalMemory {
  size_t fixed_bytes{0},active_contexts{0},retired_contexts{0},pending_requests{0};
  size_t openings{0},failures{0};
};
// Main-loop-only modal adapter. Contexts must own their configuration strings:
// retirement can outlive the old model until an in-flight callback returns.
class PageModalPort {
 public:
  virtual ~PageModalPort()=default;
  virtual bool valid_origin(PageBindingToken)const=0;
  virtual ControlModalKind kind(const PageStore &,ConfigurationHandle)const=0;
  // Explicit-purpose requests are admitted by the family, never by an arbitrary
  // caller-supplied modal kind. The default only accepts the normal card kind.
  virtual ControlModalKind kind(const PageStore& store, ConfigurationHandle card,
                                ControlModalKind requested) const {
    const auto normal = kind(store, card);
    return requested == ControlModalKind::NONE || requested == normal ? normal : ControlModalKind::NONE;
  }
  virtual bool ready()const=0; // Outside native/coordinator/LVGL callback dispatch.
  virtual bool connected()const=0;
  virtual uint32_t connection_generation()const=0;
  // Composite scenes route by the admitted family. Static local panels can use
  // a stable local epoch while HA-backed panels keep their captured HA epoch.
  virtual bool connected_for(ControlModalKind) const {
    return connected();
  }
  virtual uint32_t connection_generation_for(ControlModalKind) const {
    return connection_generation();
  }

  // create copies configuration only; initialize may subscribe/prime/open UI.
  // They run from loop, never from a configuration activation transaction.
  virtual void *create(const PageStore &,ConfigurationHandle,PageModalToken)=0;
  virtual bool initialize(void *,PageModalToken)=0;
  virtual bool visible(void *)const=0;
  virtual void update(void *,PageModalToken)=0;
  virtual ControlModalDismissPolicy dismiss_policy(void *)const=0;
  // Scalar family event marker captured before retirement; controllers use it
  // to order a close against fresh source reports without retaining contexts.
  virtual uint64_t source_revision(void*) const {
    return 0;
  }
  // Invalidate timers/callbacks and close UI immediately, but do not free ctx.
  // REPLACED/SHUTDOWN must cancel pending commands without allocating/sending.
  virtual void retire(void *,PageModalClose)=0;
  virtual void destroy(void *)=0;
};
class PageModalRuntime final:public PageRuntimeResources {
 public:
  explicit PageModalRuntime(PageModalPort &port,uint64_t first_opening=1):port_(port),next_opening_(first_opening){}
  ~PageModalRuntime()override;
  PageModalRuntime(const PageModalRuntime &)=delete;
  PageModalRuntime &operator=(const PageModalRuntime &)=delete;
  bool prepare(const PageStore &)override;
  void discard_prepared()override;
  void activate_prepared(const PageStore &)override;
  void retire_active()override;
  bool request(PageBindingToken);
  // Authenticated modal APIs can address a current navigable card directly,
  // including a hidden page. They must resolve against this generation.
  bool request_card(ConfigurationHandle);
  bool request_card(ConfigurationHandle, ControlModalKind);
  uint64_t pending_request() const {
    return pending_.pending ? pending_.opening : 0;
  }
  bool cancel_request(uint64_t);
  ControlModalDismissPolicy dismiss_policy(PageModalToken token) const {
    return valid(token) ? port_.dismiss_policy(context_) : ControlModalDismissPolicy::DISMISS;
  }
  void dismiss(PageModalClose=PageModalClose::USER);
  bool notify_closed(PageModalToken,PageModalClose=PageModalClose::USER);
  bool display_takeover(bool preserve_policy_active);
  void loop();
  bool valid(PageModalToken)const;
  bool interactive(PageModalToken token)const{return valid(token)&&port_.visible(context_);}
  PageModalToken token()const{return token_;}
  PageModalRetirement last_retirement() const {
    return last_retirement_;
  }
  PageModalMemory memory()const;
 private:
  struct Request {
    ConfigurationHandle card;
    PageBindingToken origin;
    uint64_t opening{0};
    uint32_t connection{0};
    ControlModalKind kind{ControlModalKind::NONE};
    bool pending{false}, from_binding{false};
  };
  bool valid_request(const Request&, bool verify_kind = true) const;
  bool valid_card(ConfigurationHandle, ControlModalKind, bool verify_kind) const;
  void retire_context(PageModalClose);
  void collect_retired();
  PageModalPort &port_;PageStore active_store_;
  Request pending_;
  PageModalToken token_;
  PageModalRetirement last_retirement_;
  void *context_{nullptr},*retired_{nullptr};
  uint64_t next_opening_{1};uint32_t connection_{0},prepared_generation_{0};
  bool changing_{false},prepared_{false};PageModalMemory memory_;
};
}  // namespace espcontrol::pages
