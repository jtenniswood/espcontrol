#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <new>
#include <string>

#include "page_modal_runtime.h"
#include "panel_config_document.h"

using namespace espcontrol;using namespace espcontrol::pages;using namespace espcontrol::configuration;
namespace {
struct Fixture {
  explicit Fixture(uint32_t generation=17) {
    const auto &limits=DEVICE_UI_PROFILES[8];PanelConfigWriter writer(document.data(),document.size());assert(writer.begin()==PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),std::strlen(limits.device_profile))==PanelConfigStatus::OK);
    for(uint8_t slot=1;slot<=3;++slot) {
      const std::string text=slot==3?";Room;Home;Auto;;;subpage;;":"climate."+std::string(96,'a'+slot)+";Room;Auto;Auto;;;climate;;";
      assert(writer.append_button(slot,reinterpret_cast<const uint8_t *>(text.data()),text.size())==PanelConfigStatus::OK);
    }
    const std::string children="~B|HC,climate.child,Room,Auto,Auto,,,,";
    for(uint8_t slot=2;slot<=3;++slot)assert(writer.append_subpage(slot,reinterpret_cast<const uint8_t *>(children.data()),children.size())==PanelConfigStatus::OK);
    assert(writer.finish(&size)==PanelConfigStatus::OK);assert(PageStore::stage_v1(document.data(),size,limits,generation,arena.data(),arena.size(),&store)==PageStoreStatus::OK);
  }
  std::array<uint8_t,8192>document{};size_t size{0};alignas(std::max_align_t)std::array<uint8_t,32768>arena{};PageStore store;
};
struct Port:PageModalPort {
  struct Context {std::string entity;PageModalToken token;bool callbacks{true},timer{true},pending{true},retired{false};};
  PageModalRuntime *runtime{nullptr};ControlModalStateService<int> service;int overlay{0},nested{0};
  alignas(Context)std::array<unsigned char,sizeof(Context)> storage{};
  Context *context{nullptr};uint32_t epoch{1},binding{99};bool connected_value{true},busy{false},fail_create{false},fail_initialize{false},close_initialize{false},close_update{false},preserve{false},reenter_retire{true};
  bool reconnect_create{false}, reconnect_initialize{false};
  size_t initializes{0}, creates{0}, destroys{0}, retires{0}, sends{0}, maximum_live{0};
  PageModalClose reason{PageModalClose::SHUTDOWN};
  mutable size_t kind_queries{0};
  bool valid_origin(PageBindingToken token)const override{return token.binding==binding;}
  ControlModalKind kind(const PageStore &store,ConfigurationHandle handle)const override {
    ++kind_queries;
    const auto *card=store.card(handle);if(!card)return ControlModalKind::NONE;
    const auto type=store.field(*card,CardField::TYPE);return type=="climate"||type=="climate_control"?ControlModalKind::CLIMATE:ControlModalKind::NONE;
  }
  bool ready()const override{return !busy;}
  bool connected()const override{return connected_value;}
  uint32_t connection_generation()const override{return epoch;}
  void *create(const PageStore &store,ConfigurationHandle handle,PageModalToken token)override {
    assert(!busy&&!context);if(fail_create)return nullptr;
    context =
        new (storage.data()) Context{std::string(store.field(*store.card(handle), CardField::ENTITY)), token};
    ++creates;
    maximum_live = 1;
    if (reconnect_create) ++epoch;
    return context;
  }
  bool initialize(void *pointer,PageModalToken token)override {
    auto* ctx = static_cast<Context*>(pointer);
    assert(ctx == context);
    ++initializes;
    if (reconnect_initialize) ++epoch;
    if(fail_initialize)return false;
    service.set_active(token.kind,&overlay,nullptr,dismiss_policy(ctx));
    if(close_initialize) {
      busy=true;assert(runtime->notify_closed(token));runtime->loop();
      assert(context==ctx&&ctx->retired&&!ctx->entity.empty());busy=false;
    }
    return true;
  }
  bool visible(void *pointer)const override{return pointer==context&&service.active().overlay==&overlay;}
  void update(void *pointer,PageModalToken token)override {
    auto *ctx=static_cast<Context *>(pointer);assert(ctx==context);
    if(close_update) {
      busy=true;assert(runtime->notify_closed(token));runtime->loop();
      assert(context==ctx&&ctx->retired&&!ctx->callbacks&&!ctx->timer&&!ctx->entity.empty());busy=false;
    }
  }
  ControlModalDismissPolicy dismiss_policy(void *)const override{return preserve?ControlModalDismissPolicy::PRESERVE_DURING_DISPLAY_TAKEOVER:ControlModalDismissPolicy::DISMISS;}
  void retire(void *pointer,PageModalClose why)override {
    auto *ctx=static_cast<Context *>(pointer);assert(ctx==context&&!ctx->retired);
    reason=why;++retires;
    if((why==PageModalClose::USER||why==PageModalClose::SWITCH)&&ctx->pending&&connected_value)++sends;
    ctx->callbacks=ctx->timer=ctx->pending=false;ctx->retired=true;
    service.reset_nested_menu();service.reset_active();
    assert(!runtime->notify_closed(ctx->token)); // Duplicate close during retirement.
    if(reenter_retire){runtime->loop();assert(context==ctx&&ctx->retired&&!ctx->entity.empty());}
  }
  void destroy(void *pointer)override {
    assert(!busy&&pointer==context&&context->retired&&!context->callbacks&&!context->timer);
    context->~Context();context=nullptr;++destroys;
  }
};
PageBindingToken origin(uint32_t generation=17,CardId card=0,uint32_t binding=99){return {generation,binding,0,card,0,false};}
void activate(PageModalRuntime &runtime,const PageStore &store){assert(runtime.prepare(store));runtime.activate_prepared(store);}
void queue_and_callback_lifetimes() {
  Fixture fixture;Port port;PageModalRuntime runtime(port);port.runtime=&runtime;activate(runtime,fixture.store);
  assert(!runtime.request_card({17, 0}, ControlModalKind::ALARM_CONTROL));
  const auto queries = port.kind_queries;
  assert(runtime.request(origin()));assert(port.creates==0&&runtime.memory().pending_requests==1);
  assert(port.kind_queries == queries + 1);
  ++port.binding;runtime.loop();assert(port.creates==0&&runtime.memory().pending_requests==0); // Rebound before loop.
  assert(!runtime.request(origin()));assert(runtime.request(origin(17,0,port.binding)));runtime.loop();
  const auto first=runtime.token();void *address=port.context;assert(runtime.valid(first)&&runtime.interactive(first));
  // Modal lifetime does not depend on the original grid binding after opening.
  ++port.binding;assert(runtime.valid(first));
  port.busy=true;assert(runtime.notify_closed(first));assert(!runtime.valid(first));
  assert(port.context==address&&port.context->retired&&!port.context->timer&&!port.context->callbacks);
  assert(runtime.memory().retired_contexts==1&&port.destroys==0);runtime.loop();assert(port.destroys==0);
  assert(runtime.request_card({17,1}));port.busy=false;runtime.loop();
  assert(port.destroys==1&&port.creates==2&&port.context==address&&runtime.valid(runtime.token()));
  assert(runtime.token().opening!=first.opening&&!runtime.notify_closed(first)); // Same address, new opening.
  port.close_update=true;runtime.loop();assert(!port.context&&port.creates==port.destroys);
  port.close_update=false;port.close_initialize=true;assert(runtime.request_card({17,0}));runtime.loop();
  assert(!port.context&&runtime.memory().active_contexts==0&&runtime.memory().retired_contexts==0);
}
void replacement_connection_and_takeover() {
  Fixture old,next(18);Port port;PageModalRuntime runtime(port);port.runtime=&runtime;activate(runtime,old.store);
  const auto inactive=old.store.page(1)->first_card,active=old.store.page(2)->first_card;
  assert(!old.store.page(1)->navigable&&old.store.page(2)->navigable);
  assert(!runtime.request_card({17,inactive}));assert(runtime.request_card({17,active}));runtime.loop();assert(port.context);
  auto token=runtime.token();port.busy=true;activate(runtime,next.store);
  assert(!runtime.valid(token)&&port.reason==PageModalClose::REPLACED&&port.sends==0&&port.context->retired);
  std::memset(old.arena.data(),0xA5,old.arena.size());assert(port.context->entity=="climate.child");
  runtime.loop();assert(port.context);port.busy=false;runtime.loop();assert(!port.context);
  assert(!runtime.request_card({17,0}));assert(runtime.request_card({18,0}));runtime.loop();token=runtime.token();
  port.connected_value=false;assert(!runtime.valid(token));runtime.loop();assert(!port.context&&port.reason==PageModalClose::DISCONNECTED&&port.sends==0);
  assert(!runtime.request_card({18,0}));port.connected_value=true;++port.epoch;
  assert(runtime.request_card({18,0}));runtime.loop();token=runtime.token();++port.epoch;assert(!runtime.valid(token));runtime.loop();assert(!port.context);
  port.preserve=true;assert(runtime.request_card({18,0}));runtime.loop();token=runtime.token();
  port.service.nested_active().overlay=&port.nested;assert(runtime.display_takeover(true));
  assert(runtime.valid(token)&&port.service.nested_active().overlay==&port.nested);
  assert(!runtime.display_takeover(false)&&!runtime.valid(token)&&port.reason==PageModalClose::TAKEOVER);
  runtime.loop();assert(!port.context&&port.sends==0);
}
void queued_requests_keep_their_connection() {
  Fixture fixture;
  Port port;
  PageModalRuntime runtime(port);
  port.runtime = &runtime;
  activate(runtime, fixture.store);
  for (bool binding : {false, true}) {
    port.busy = true;
    assert(binding ? runtime.request(origin()) : runtime.request_card({17, 0}));
    ++port.epoch;  // Includes disconnect/reconnect entirely between loop calls.
    runtime.loop();
    assert(runtime.memory().pending_requests == 1);
    port.busy = false;
    runtime.loop();
    assert(port.creates == 0 && port.initializes == 0 && runtime.memory().pending_requests == 0);
  }
  port.reconnect_create = true;
  assert(runtime.request_card({17, 0}));
  runtime.loop();
  assert(port.creates == 1 && port.initializes == 0 && port.destroys == 1 && !port.context &&
         port.sends == 0);
  port.reconnect_create = false;
  port.reconnect_initialize = true;
  assert(runtime.request_card({17, 0}));
  runtime.loop();
  assert(port.creates == 2 && port.initializes == 1 && port.destroys == 2 && !port.context &&
         port.sends == 0);
  port.reconnect_initialize = false;
  assert(runtime.request_card({17, 0}));
  runtime.loop();
  assert(port.context && runtime.valid(runtime.token()));
  runtime.dismiss(PageModalClose::SHUTDOWN);
  runtime.loop();
  assert(port.creates == port.destroys && port.sends == 0);
}
void local_settings_origin() {
  struct SettingsPort final : Port {
    uint64_t chrome{1};
    uint32_t configuration{17};
    PageId page{0};
    bool hold_retire{false}, cancel_create{false}, shutdown_create{false};
    PageLocalModalOrigin captured{};
    ControlModalKind local_kind(ControlModalKind requested) const override {
      return requested == ControlModalKind::NETWORK_STATUS ? requested : ControlModalKind::NONE;
    }
    bool valid_local_origin(PageLocalModalOrigin origin, ControlModalKind kind) const override {
      return kind == ControlModalKind::NETWORK_STATUS && origin.binding == chrome &&
             origin.configuration == configuration && origin.page == page;
    }
    bool connected_for(ControlModalKind kind) const override {
      return kind == ControlModalKind::NETWORK_STATUS || Port::connected();
    }
    uint32_t connection_generation_for(ControlModalKind kind) const override {
      return kind == ControlModalKind::NETWORK_STATUS ? 0 : Port::connection_generation();
    }
    void* create_local(const PageStore&, PageLocalModalOrigin origin, PageModalToken token) override {
      assert(!context && !busy && token.kind == ControlModalKind::NETWORK_STATUS && token.card == NO_CARD);
      captured = origin;
      context = new (storage.data()) Context{std::string(96, 'n'), token};
      ++creates;
      maximum_live = 1;
      if (cancel_create) runtime->display_takeover(false);
      if (shutdown_create) runtime->retire_active();
      return context;
    }
    void retire(void* pointer, PageModalClose why) override {
      auto* ctx = static_cast<Context*>(pointer);
      if (ctx->token.kind == ControlModalKind::NETWORK_STATUS) ctx->pending = false;
      Port::retire(pointer, why);
      if (hold_retire) {
        busy = true;
        ++chrome;
      }
    }
    PageLocalModalOrigin origin() const {
      return {configuration, chrome, page};
    }
  } port;
  Fixture fixture;
  PageModalRuntime runtime(port);
  port.runtime = &runtime;
  assert(!runtime.request_local(port.origin(), ControlModalKind::NETWORK_STATUS));
  activate(runtime, fixture.store);
  port.connected_value = false;
  assert(!runtime.request_local(port.origin(), ControlModalKind::WIFI_QR));
  auto bad = port.origin();
  bad.binding = 0;
  assert(!runtime.request_local(bad, ControlModalKind::NETWORK_STATUS));
  bad = port.origin();
  ++bad.configuration;
  assert(!runtime.request_local(bad, ControlModalKind::NETWORK_STATUS));
  bad = port.origin();
  bad.page = NO_PAGE;
  assert(!runtime.request_local(bad, ControlModalKind::NETWORK_STATUS));
  assert(!runtime.request_card({17, NO_CARD}, ControlModalKind::NETWORK_STATUS));
  assert(!runtime.request(origin(17, NO_CARD)));
  assert(runtime.request_local(port.origin(), ControlModalKind::NETWORK_STATUS));
  ++port.chrome;
  runtime.loop();
  assert(!port.context && runtime.memory().pending_requests == 0);
  const auto source = port.origin();
  assert(runtime.request_local(source, ControlModalKind::NETWORK_STATUS));
  assert(!port.creates);
  runtime.loop();
  const auto first = runtime.token();
  assert(first.card == NO_CARD && first.kind == ControlModalKind::NETWORK_STATUS &&
         runtime.interactive(first));
  assert(port.captured.binding == source.binding && port.captured.page == source.page);
  ++port.chrome;
  ++port.epoch;
  runtime.loop();
  assert(runtime.valid(first));
  assert(!runtime.request_card({17, 0}) && runtime.valid(first));
  runtime.dismiss(PageModalClose::SHUTDOWN);
  runtime.loop();
  assert(!port.context && port.sends == 0);
  // An accepted Settings open keeps its original source while its own previous
  // overlay retires; it never substitutes a freshly issued chrome identity.
  port.connected_value = true;
  assert(runtime.request_card({17, 0}));
  runtime.loop();
  port.context->pending = false;
  const auto accepted = port.origin();
  port.hold_retire = true;
  assert(runtime.request_local(accepted, ControlModalKind::NETWORK_STATUS));
  runtime.loop();
  assert(runtime.memory().retired_contexts == 1 && runtime.memory().pending_requests == 1);
  assert(port.chrome != accepted.binding);
  port.hold_retire = false;
  port.busy = false;
  runtime.loop();
  assert(runtime.valid(runtime.token()) && port.captured.binding == accepted.binding);
  runtime.dismiss(PageModalClose::SHUTDOWN);
  runtime.loop();
  port.cancel_create = true;
  const auto initialized = port.initializes;
  assert(runtime.request_local(port.origin(), ControlModalKind::NETWORK_STATUS));
  runtime.loop();
  assert(!port.context && port.initializes == initialized &&
         runtime.last_retirement().reason == PageModalClose::TAKEOVER);
  port.cancel_create = false;
  // Generation zero does not allow a reentrant shutdown to revive a new panel.
  Fixture zero(0);
  activate(runtime, zero.store);
  port.configuration = 0;
  port.shutdown_create = true;
  assert(runtime.request_local(port.origin(), ControlModalKind::NETWORK_STATUS));
  runtime.loop();
  assert(!port.context && port.initializes == initialized &&
         runtime.last_retirement().reason == PageModalClose::SHUTDOWN);
  port.shutdown_create = false;
  activate(runtime, zero.store);
  for (unsigned n = 0; n < 1000; ++n) {
    ++port.chrome;
    assert(runtime.request_local(port.origin(), ControlModalKind::NETWORK_STATUS));
    runtime.loop();
    assert(runtime.token().configuration == 0 && runtime.token().card == NO_CARD &&
           runtime.interactive(runtime.token()));
    ++port.epoch;
    port.connected_value = !port.connected_value;
    runtime.loop();
    assert(runtime.valid(runtime.token()));
    runtime.dismiss(PageModalClose::SHUTDOWN);
    runtime.loop();
    assert(port.creates == port.destroys);
  }
  assert(port.maximum_live == 1 && port.sends == 0);
  SettingsPort last;
  PageModalRuntime exhausted(last, UINT64_MAX);
  last.runtime = &exhausted;
  activate(exhausted, fixture.store);
  assert(exhausted.request_local(last.origin(), ControlModalKind::NETWORK_STATUS));
  exhausted.loop();
  assert(exhausted.token().opening == UINT64_MAX);
  exhausted.dismiss(PageModalClose::SHUTDOWN);
  exhausted.loop();
  assert(!exhausted.request_local(last.origin(), ControlModalKind::NETWORK_STATUS));
}

void family_connection_policy() {
  struct LocalPort final : Port {
    uint32_t local_epoch{0};
    bool local_card{true};
    ControlModalKind kind(const PageStore& store, ConfigurationHandle card) const override {
      const auto normal = Port::kind(store, card);
      return local_card && card.index == 0 && normal == ControlModalKind::CLIMATE ? ControlModalKind::WIFI_QR
                                                                                  : normal;
    }
    bool connected_for(ControlModalKind kind) const override {
      return kind == ControlModalKind::WIFI_QR || Port::connected();
    }
    uint32_t connection_generation_for(ControlModalKind kind) const override {
      return kind == ControlModalKind::WIFI_QR ? local_epoch : Port::connection_generation();
    }
    void retire(void* pointer, PageModalClose reason) override {
      if (static_cast<Context*>(pointer)->token.kind == ControlModalKind::WIFI_QR)
        static_cast<Context*>(pointer)->pending = false;
      Port::retire(pointer, reason);
    }
  } port;
  Fixture fixture;
  PageModalRuntime runtime(port);
  port.runtime = &runtime;
  activate(runtime, fixture.store);
  port.connected_value = false;
  for (bool binding : {false, true}) {
    port.busy = true;
    assert(binding ? runtime.request(origin()) : runtime.request_card({17, 0}));
    ++port.epoch;
    port.busy = false;
    runtime.loop();
    const auto qr = runtime.token();
    assert(qr.kind == ControlModalKind::WIFI_QR && runtime.valid(qr) && runtime.interactive(qr));
    ++port.epoch;
    runtime.loop();
    assert(runtime.valid(qr));
    assert(!runtime.request_card({17, 1}) && runtime.valid(qr));
    runtime.dismiss(PageModalClose::SHUTDOWN);
    runtime.loop();
  }
  // A source-local invalidation still cancels a queued local opening.
  assert(runtime.request_card({17, 0}));
  ++port.local_epoch;
  runtime.loop();
  assert(!port.context);
  // A changed family cannot adopt a queued request under its different epoch.
  assert(runtime.request_card({17, 0}));
  port.local_card = false;
  runtime.loop();
  assert(!port.context);
  port.local_card = true;
  port.reconnect_create = true;
  assert(runtime.request_card({17, 0}));
  runtime.loop();
  assert(runtime.valid(runtime.token()));
  port.reconnect_create = false;
  runtime.dismiss(PageModalClose::SHUTDOWN);
  runtime.loop();
  port.reconnect_initialize = true;
  assert(runtime.request_card({17, 0}));
  runtime.loop();
  assert(runtime.valid(runtime.token()));
  port.reconnect_initialize = false;
  for (unsigned n = 0; n < 1000; ++n) {
    port.connected_value = true;
    ++port.epoch;
    assert(runtime.request_card({17, 1}));
    runtime.loop();
    const auto ha = runtime.token();
    assert(ha.kind == ControlModalKind::CLIMATE && runtime.valid(ha));
    port.connected_value = false;
    ++port.epoch;
    assert(!runtime.valid(ha));
    assert(runtime.request_card({17, 0}));
    runtime.loop();
    const auto qr = runtime.token();
    assert(qr.kind == ControlModalKind::WIFI_QR && runtime.valid(qr));
    assert(runtime.last_retirement().reason == PageModalClose::DISCONNECTED);
    // A reconnect invalidates an old pending HA request without closing QR.
    port.connected_value = true;
    assert(runtime.request_card({17, 1}));
    ++port.epoch;
    runtime.loop();
    assert(runtime.valid(qr) && runtime.token().opening == qr.opening);
    port.connected_value = false;
    ++port.epoch;
    runtime.loop();
    assert(runtime.valid(qr));
    assert(!runtime.notify_closed(ha));
  }
  runtime.dismiss(PageModalClose::SHUTDOWN);
  runtime.loop();
  assert(port.creates == port.destroys && port.maximum_live == 1 && port.sends == 0);
}

void pending_request_identity() {
  Fixture fixture;
  Port port;
  PageModalRuntime runtime(port);
  port.runtime = &runtime;
  activate(runtime, fixture.store);
  assert(runtime.request_card({17, 0}));
  const auto first = runtime.pending_request();
  assert(first);
  assert(runtime.request_card({17, 1}));
  const auto second = runtime.pending_request();
  assert(second && second != first);
  assert(!runtime.cancel_request(first) && runtime.pending_request() == second);
  assert(runtime.cancel_request(second) && runtime.pending_request() == 0);
  runtime.loop();
  assert(!port.context);
  assert(runtime.request_card({17, 0}));
  const auto opening = runtime.pending_request();
  runtime.loop();
  assert(runtime.token().opening == opening && !runtime.cancel_request(opening));
  assert(runtime.dismiss_policy(runtime.token()) == ControlModalDismissPolicy::DISMISS);
  port.preserve = true;
  assert(runtime.dismiss_policy(runtime.token()) ==
         ControlModalDismissPolicy::PRESERVE_DURING_DISPLAY_TAKEOVER);
}
void action_response_dispatch_lifetime() {
  Fixture fixture;Port port;PageModalRuntime runtime(port);port.runtime=&runtime;activate(runtime,fixture.store);
  assert(runtime.request_card({17,0}));runtime.loop();auto *context=port.context;
  {
    espcontrol::ModalCallbackDispatchScope dispatch;
    assert(espcontrol::modal_callback_dispatching());
    runtime.dismiss(PageModalClose::REPLACED);runtime.loop();
    assert(port.context==context&&context->retired&&runtime.memory().retired_contexts==1);
    {espcontrol::ModalCallbackDispatchScope nested;runtime.loop();assert(port.context==context);}
    assert(espcontrol::modal_callback_dispatching());
  }
  assert(!espcontrol::modal_callback_dispatching());runtime.loop();assert(!port.context);
}
void bounded_cycles_failures_and_exhaustion() {
  Fixture fixture;Port port;PageModalRuntime runtime(port);port.runtime=&runtime;activate(runtime,fixture.store);
  port.fail_create=true;assert(runtime.request_card({17,0}));runtime.loop();assert(!port.context&&runtime.memory().failures==1);
  port.fail_create=false;port.fail_initialize=true;assert(runtime.request_card({17,0}));runtime.loop();assert(!port.context&&runtime.memory().failures==2);
  port.fail_initialize=false;
  for(size_t cycle=0;cycle<1000;++cycle) {
    assert(runtime.request_card({17,static_cast<CardId>(cycle%2)}));runtime.loop();const auto token=runtime.token();assert(runtime.valid(token));
    port.service.block_close_for(100,50);assert(!port.service.begin_active_close(120,true,nullptr,nullptr));runtime.loop();assert(runtime.valid(token));
    assert(port.service.begin_active_close(150,true,nullptr,nullptr));port.service.reset_active();
    assert(runtime.notify_closed(token));runtime.loop();assert(!port.context&&port.creates==port.destroys);
    assert(runtime.memory().active_contexts==0&&runtime.memory().retired_contexts==0&&runtime.memory().pending_requests==0);
  }
  assert(port.maximum_live==1&&runtime.memory().fixed_bytes==sizeof(runtime));
  runtime.retire_active();assert(!runtime.request_card({17,0}));
  Port last;PageModalRuntime exhausted(last,UINT64_MAX);last.runtime=&exhausted;activate(exhausted,fixture.store);
  assert(exhausted.request_card({17,0}));exhausted.loop();assert(exhausted.token().opening==UINT64_MAX);
  exhausted.dismiss();exhausted.loop();assert(!exhausted.request_card({17,0}));
}
}
int main() {
  local_settings_origin();
  family_connection_policy();
  pending_request_identity();
  queued_requests_keep_their_connection();
  action_response_dispatch_lifetime();
  queue_and_callback_lifetimes();
  replacement_connection_and_takeover();
  bounded_cycles_failures_and_exhaustion();
}
