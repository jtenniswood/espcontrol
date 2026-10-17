#pragma once

// Include after button_grid.h's legacy context, HA and UI helpers.
#include "page_entity_modal_replay.h"
#include "page_modal_gestures.h"
#include "page_lvgl_dispatch.h"

#include <cassert>
#include <new>

namespace espcontrol::pages {
template<typename Family> struct PageLegacyModalStyle {
  typename Family::Environment environment{};
  size_t gesture_capacity{128};
  void *owner{nullptr};
  bool (*resolve)(void *,const ParsedCfg &,typename Family::Environment &){nullptr};
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
    runtime_=&runtime;observer={this,managed,current,action,closed,widgets};return true;
  }
  bool valid_origin(PageBindingToken token)const override{return valid_&&valid_(owner_,token);}
  ControlModalKind kind(const PageStore &store,ConfigurationHandle handle)const override {
    if(!runtime_||!style_.resolve||Family::lifetime().owner!=this)return ControlModalKind::NONE;
    const auto *card=store.card(handle);
    if(!card||store.field(*card,CardField::ENTITY).empty())return ControlModalKind::NONE;
    return Family::matches(store,*card)?Family::kind:ControlModalKind::NONE;
  }
  bool ready()const override{return lv_is_initialized()&&!ha_read_coordinator().dispatching()&&!page_lvgl_dispatching();}
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
    if(!style_.resolve(style_.owner,config,environment)||
        !Family::initialize_context(record->context,config,environment)){
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
    if(record.retired||!record.context.available||!connected()||record.connection!=connection_generation())return false;
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
    if(!record.retired&&!record.context.available&&Family::active()==&record.context)Family::hide();
  }
  ControlModalDismissPolicy dismiss_policy(void *)const override{return ControlModalDismissPolicy::DISMISS;}
  void retire(void *pointer,PageModalClose reason)override {
    auto &record=*static_cast<Record *>(pointer);record.retired=true;record.gestures.invalidate();
    ha_release_callbacks_for_owner(&record.context);Family::cancel(&record.context);
    if(Family::active()!=&record.context)return;
    if(reason==PageModalClose::REPLACED||reason==PageModalClose::SHUTDOWN) {
      // Widget/clock teardown is maintenance after dispatch, never part of the
      // durable activation. Context strings own their bytes until then.
      auto *overlay=Family::overlay();
      if(overlay)lv_obj_add_flag(overlay,LV_OBJ_FLAG_HIDDEN);
      if(control_modal_active().overlay==overlay)control_modal_service().reset_active();
    } else Family::hide();
  }
  void destroy(void *pointer)override {
    auto *record=static_cast<Record *>(pointer);assert(record==record_&&record->retired);
    if(Family::active()==&record->context)Family::hide();
    assert(record->gestures.live()==0);record_=nullptr;release_record(record);
  }
 private:
  struct Record {
    Record(PageLegacyModalPort &port,PageModalToken token,ConfigurationHandle card)
        :token(token),card(card),connection(port.connection_generation()),
         gestures(port.allocator_,port.style_.gesture_capacity,token,&port,gesture_valid){}
    Context context{};
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
  }
  static bool managed(void *owner,const Context *context) {
    const auto &port=*static_cast<PageLegacyModalPort *>(owner);
    return port.record_ && &port.record_->context==context;
  }
  static bool current(void *owner,const Context *context) {
    const auto &port=*static_cast<PageLegacyModalPort *>(owner);
    return managed(owner,context)&&!port.record_->retired&&port.runtime_->valid(port.record_->token)&&
      port.connected()&&port.record_->connection==port.connection_generation();
  }
  static bool action(void *owner,const Context *context) {
    const auto &port=*static_cast<PageLegacyModalPort *>(owner);
    return current(owner,context)&&port.runtime_->interactive(port.record_->token)&&port.connected()&&
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
