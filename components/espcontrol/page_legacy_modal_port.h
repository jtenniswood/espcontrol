#pragma once

// Include after button_grid.h's legacy context, HA and UI helpers.
#include "page_entity_modal_replay.h"
#include "page_modal_gestures.h"
#include "page_lvgl_dispatch.h"

#include <cassert>
#include <new>
#include <type_traits>

namespace espcontrol::pages {
// Snapshot families keep their comparison state in the single demanded modal
// record, rather than adding fields to every legacy configured-card context.
template<typename Family,typename=void>struct PageModalPullState {struct Type {};};
template<typename Family>struct PageModalPullState<Family,std::void_t<typename Family::State>> {
  using Type=typename Family::State;
};
template<typename Family> struct PageLegacyModalStyle {
  typename Family::Environment environment{};
  size_t gesture_capacity{128};
  void *owner{nullptr};
  // Resolve by the opening's original configuration/card identity, including
  // hidden-page fonts; never infer the source from the current visible grid.
  bool (*resolve)(void*, PageModalToken, const ParsedCfg&, typename Family::Environment&){nullptr};
  // A scene retains the original full grid binding at request time and matches
  // this opening token before forwarding a numeric preview to the card bank.
  void (*numeric_action_sent)(void *,PageModalToken,float){nullptr};
  // The scene matches this opening to its original full grid binding.
  void (*state_confirmed)(void *,PageModalToken,bool){nullptr};
};

// A family supplies its existing UI/context operations; ownership, cache replay,
// gesture lifetime and retirement stay identical across the small control
// panels. All instances must share one PageModalRuntime through a composite
// port. The allocator outlives the port and every retired context.
template<typename Family> class PageLegacyModalPort final:public PageModalPort {
 public:
  using Context=typename Family::Context;
  using Style=PageLegacyModalStyle<Family>;
  using OriginValid=bool (*)(void *,PageBindingToken);
  PageLegacyModalPort(PageArenaAllocator &allocator,PageEntityStateRuntime &state,
                     Style style,void *owner,OriginValid valid)
      :allocator_(allocator),state_(state),style_(style),owner_(owner),valid_(valid){}
  ~PageLegacyModalPort()override {
    assert(record_==nullptr);
    if(Family::lifetime().owner==this)Family::lifetime()={};
  }
  bool install(PageModalRuntime &runtime,bool legacy_renderer_retired) {
    auto &observer=Family::lifetime();
    if(!legacy_renderer_retired||!lv_is_initialized()||record_||Family::active()||
        (observer.owner&&observer.owner!=this))return false;
    runtime_=&runtime;observer={this,managed,current,action,closed,widgets,numeric_action,identity,state_confirmed,refresh};return true;
  }
  bool valid_origin(PageBindingToken token)const override{return valid_&&valid_(owner_,token);}
  ControlModalKind kind(const PageStore &store,ConfigurationHandle handle)const override {
    if(!runtime_||!style_.resolve||Family::lifetime().owner!=this)return ControlModalKind::NONE;
    const auto *card=store.card(handle);
    if(!card||store.field(*card,CardField::ENTITY).empty())return ControlModalKind::NONE;
    return Family::matches(store,*card)?Family::kind:ControlModalKind::NONE;
  }
  bool ready()const override{return lv_is_initialized()&&!modal_callback_dispatching()&&!ha_read_coordinator().dispatching()&&!page_lvgl_dispatching();}
  bool connected()const override{return ha_api_state_connected();}
  uint32_t connection_generation()const override{return ha_read_coordinator().connection_generation();}
  void *create(const PageStore &store,ConfigurationHandle card,PageModalToken token)override {
    if(record_||kind(store,card)!=Family::kind||!ha_internal_heap_available(Family::tag))return nullptr;
    void *memory=allocator_.allocate(sizeof(Record));if(!memory)return nullptr;
    auto *record=new(memory)Record(*this,token,card);
    if(!record->gestures.available()){release_record(record);return nullptr;}
    const auto *definition=store.card(card);ParsedCfg config;
    config.entity=std::string(store.field(*definition,CardField::ENTITY));
    config.label=std::string(store.field(*definition,CardField::LABEL));
    config.icon=std::string(store.field(*definition,CardField::ICON));
    config.icon_on=std::string(store.field(*definition,CardField::ICON_ON));
    config.sensor=std::string(store.field(*definition,CardField::SENSOR));
    config.unit=std::string(store.field(*definition,CardField::UNIT));
    config.type=std::string(store.field(*definition,CardField::TYPE));
    config.precision=std::string(store.field(*definition,CardField::PRECISION));
    config.options=std::string(store.field(*definition,CardField::OPTIONS));
    auto environment=style_.environment;
    if (!style_.resolve(style_.owner, token, config, environment) ||
        !Family::initialize_context(record->context, config, environment) ||
        !initialize_state(*record, config, environment, style_.gesture_capacity, 0)) {
      release_record(record);return nullptr;
    }
    record->context.available=false;record_=record;return record;
  }
  bool initialize(void *pointer,PageModalToken)override {
    auto &record=*static_cast<Record *>(pointer);
    if(control_modal_active().kind!=ControlModalKind::NONE)return false;
    {
      HaCallbackOwnerScope owner_scope(&record.context);
      Family::subscribe(&record.context);
    }
    prime(record);
    if(record.retired||!record_can_open(record,0)||!connected()||record.connection!=connection_generation())return false;
    Family::open(&record.context);
    if(!visible(&record))return false;
    set_clock_bar_modal_label(Family::title(&record.context));return true;
  }
  bool visible(void *pointer)const override {
    const auto &record=*static_cast<Record *>(pointer);
    return !record.retired&&Family::active()==&record.context&&Family::overlay();
  }
  void update(void *pointer,PageModalToken)override {
    auto &record=*static_cast<Record *>(pointer);prime(record);
    if(!record.retired&&!record_can_open(record,0)&&Family::active()==&record.context)Family::hide();
  }
  ControlModalDismissPolicy dismiss_policy(void *)const override{return ControlModalDismissPolicy::DISMISS;}
  void retire(void *pointer,PageModalClose reason)override {
    auto &record=*static_cast<Record *>(pointer);record.retired=true;record.gestures.invalidate();
    ha_release_callbacks_for_owner(&record.context);
    if (page_lvgl_dispatching()) return;
    Family::cancel(&record.context);
    if(Family::active()!=&record.context)return;
    if(reason==PageModalClose::REPLACED||reason==PageModalClose::SHUTDOWN||!ready()) {
      // Widget/clock teardown is maintenance after dispatch, never part of the
      // durable activation. Context strings own their bytes until then.
      auto *overlay=Family::overlay();
      if(overlay)lv_obj_add_flag(overlay,LV_OBJ_FLAG_HIDDEN);
      if(control_modal_active().overlay==overlay)control_modal_service().reset_active();
    } else Family::hide();
  }
  void destroy(void *pointer)override {
    auto *record=static_cast<Record *>(pointer);assert(record==record_&&record->retired);
    Family::cancel(&record->context);
    if(Family::active()==&record->context)Family::hide();
    assert(record->gestures.live()==0);record_=nullptr;release_record(record);
  }
 private:
  struct Record {
    Record(PageLegacyModalPort &port,PageModalToken token,ConfigurationHandle card)
        :token(token),card(card),connection(port.connection_generation()),
         gestures(port.allocator_,port.style_.gesture_capacity,token,&port,gesture_valid){}
    Context context{};
    typename PageModalPullState<Family>::Type pull_state{};
    PageModalToken token;
    ConfigurationHandle card;
    uint32_t connection;
    bool retired{false};
    PageModalGestures gestures;
  };
  void release_record(Record *record){record->~Record();allocator_.release(record,sizeof(Record));}
  void prime(Record &record) {
    if(record.retired||!runtime_->valid(record.token))return;
    PageEntityModalReplay replay(state_,allocator_,record.card);
    ha_read_coordinator().replay_owner_missing(&record.context,replay);
    pull(record,state_,0);
  }
  template<typename F=Family>
  static auto initialize_state(Record& record,const ParsedCfg& config,
                               const typename Family::Environment& environment,size_t gesture_capacity,int)
      ->decltype(F::initialize_state(record.context,record.pull_state,config,environment,gesture_capacity),bool()) {
    return F::initialize_state(record.context,record.pull_state,config,environment,gesture_capacity);
  }
  static bool initialize_state(Record&,const ParsedCfg&,const typename Family::Environment&,size_t,long) {return true;}
  template<typename F=Family>
  static auto record_can_open(Record& record,int)
      ->decltype(F::can_open(&record.context,record.pull_state),bool()) {
    return F::can_open(&record.context,record.pull_state);
  }
  static bool record_can_open(Record& record,long) { return can_open(record.context,0); }
  template<typename F=Family>
  static auto can_open(Context& context,int)->decltype(F::can_open(&context),bool()) {
    return F::can_open(&context);
  }
  static bool can_open(Context& context,long) {return context.available;}
  static uint64_t identity(void* owner,const Context* context) {
    auto& port=*static_cast<PageLegacyModalPort*>(owner);
    return current(owner,context)?port.record_->token.opening:0;
  }
  template<typename F=Family>
  static auto pull(Record& record,PageEntityStateRuntime& state,int)
      ->decltype(F::pull(record.context,state,record.card,record.pull_state),void()) {
    F::pull(record.context,state,record.card,record.pull_state);
  }
  static void pull(Record&,PageEntityStateRuntime&,long) {}
  static void numeric_action(void *owner,const Context *context,float value) {
    auto& port=*static_cast<PageLegacyModalPort*>(owner);
    if(!action(owner,context)||!context->available||!port.style_.numeric_action_sent)return;
    const auto token=port.record_->token;
    port.style_.numeric_action_sent(port.style_.owner,token,value);
    // A notification may replace configuration/retire this exact opening.
  }
  static bool refresh(void* owner, const Context* context) {
    auto& port = *static_cast<PageLegacyModalPort*>(owner);
    if (!current(owner, context)) return false;
    pull(*port.record_, port.state_, 0);
    return current(owner, context);
  }
  static void state_confirmed(void* owner, const Context* context, bool value) {
    auto& port = *static_cast<PageLegacyModalPort*>(owner);
    if (!current(owner, context) || !context->available || !port.style_.state_confirmed) return;
    const auto token = port.record_->token;
    port.style_.state_confirmed(port.style_.owner, token, value);
  }
  static bool managed(void* owner, const Context* context) {
    const auto &port=*static_cast<PageLegacyModalPort *>(owner);
    return port.record_ && &port.record_->context==context;
  }
  static bool current(void *owner,const Context *context) {
    const auto &port=*static_cast<PageLegacyModalPort *>(owner);
    return managed(owner,context)&&!port.record_->retired&&port.runtime_->valid(port.record_->token)&&
      port.connected()&&port.record_->connection==port.connection_generation();
  }
  template<typename F=Family>
  static auto action_ready(Record& record,int)->decltype(F::action_ready(record.context,record.pull_state),bool()) {
    return F::action_ready(record.context,record.pull_state);
  }
  static bool action_ready(Record&,long) {return true;}
  static bool action(void *owner,const Context *context) {
    auto &port=*static_cast<PageLegacyModalPort *>(owner);
    if(!current(owner,context))return false;
    // Snapshot-backed families consume current cache state before an input
    // computes its command, just as the legacy live callback did.
    pull(*port.record_,port.state_,0);
    return current(owner,context)&&action_ready(*port.record_,0)&&record_can_open(*port.record_,0)&&port.runtime_->interactive(port.record_->token)&&port.connected()&&
      port.record_->connection==port.connection_generation();
  }
  static bool gesture_valid(void *owner,PageModalToken token) {
    const auto &port=*static_cast<PageLegacyModalPort *>(owner);
    return port.runtime_&&port.runtime_->interactive(token)&&port.connected()&&port.record_&&
      port.record_->connection==port.connection_generation();
  }
  static void closed(void *owner,Context *context) {
    auto &port=*static_cast<PageLegacyModalPort *>(owner);
    if(!managed(owner,context)||port.record_->retired)return;
    port.runtime_->notify_closed(port.record_->token,!port.connected()?PageModalClose::DISCONNECTED:
      context->available?PageModalClose::USER:PageModalClose::FAILED);
  }
  static bool widgets(void *owner,void *root) {
    auto &port=*static_cast<PageLegacyModalPort *>(owner);
    return port.record_&&!port.record_->retired&&port.record_->gestures.attach(static_cast<lv_obj_t *>(root));
  }
  PageArenaAllocator &allocator_;
  PageEntityStateRuntime &state_;
  Style style_;
  void *owner_{nullptr};
  OriginValid valid_{nullptr};
  PageModalRuntime *runtime_{nullptr};
  Record *record_{nullptr};
};
}  // namespace espcontrol::pages
