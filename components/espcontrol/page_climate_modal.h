#pragma once

// Include after the legacy climate UI and driver definitions (button_grid.h).
#include "page_entity_modal_replay.h"
#include "page_modal_gestures.h"
#include "page_lvgl_dispatch.h"

namespace espcontrol::pages {
struct PageClimateModalStyle {
  espcontrol::cards::ClimateControlDriverEnvironment environment;
  size_t gesture_capacity{128}; // Setup budget; qualify the real panel's peak.
  // Resolve current profile fonts and per-card palette without retaining a
  // grid widget in the modal context. Required before this adapter can open.
  void *owner{nullptr};
  // Resolve by the opening's original configuration/card identity, including
  // hidden-page fonts; never infer the source from the current visible grid.
  bool (*resolve)(void*, PageModalToken, const ParsedCfg&,
                  espcontrol::cards::ClimateControlDriverEnvironment&){nullptr};
};
// Demand-created adapter for the existing climate UI. No instance is selected
// by production until scene/configuration ownership is migrated. A composite
// PageModalPort can delegate climate methods here while sharing one runtime
// with other modal families. install() requires the legacy eager owner retired.
class PageClimateModalPort final:public PageModalPort {
 public:
  using OriginValid=bool (*)(void *,PageBindingToken);
  PageClimateModalPort(PageArenaAllocator &allocator,PageEntityStateRuntime &state,
                       PageClimateModalStyle style,void *owner,OriginValid valid)
      :allocator_(allocator),state_(state),style_(style),owner_(owner),valid_(valid){}
  ~PageClimateModalPort()override {
    assert(record_==nullptr);
    if(climate_modal_lifetime().owner==this)climate_modal_lifetime()={};
  }
  bool install(PageModalRuntime &runtime) {
    auto &observer=climate_modal_lifetime();
    if(!lv_is_initialized()||record_||(observer.owner&&observer.owner!=this)||climate_control_modal_ui().active||climate_control_ref_count()!=0)return false;
    runtime_=&runtime;observer={this,managed,current,action,closed,widgets};return true;
  }
  bool valid_origin(PageBindingToken token)const override{return valid_&&valid_(owner_,token);}
  ControlModalKind kind(const PageStore &store,ConfigurationHandle handle)const override {
    if(!runtime_||!style_.resolve||climate_modal_lifetime().owner!=this)return ControlModalKind::NONE;
    const auto *card=store.card(handle);if(!card||store.field(*card,CardField::ENTITY).empty())return ControlModalKind::NONE;
    const auto type=store.field(*card,CardField::TYPE);
    return type=="climate"||type=="climate_control"?ControlModalKind::CLIMATE:ControlModalKind::NONE;
  }
  bool ready()const override{return lv_is_initialized()&&!ha_read_coordinator().dispatching()&&!page_lvgl_dispatching();}
  bool connected()const override{return ha_api_state_connected();}
  uint32_t connection_generation()const override{return ha_read_coordinator().connection_generation();}
  void *create(const PageStore &store,ConfigurationHandle card,PageModalToken token)override {
    if(record_||kind(store,card)!=ControlModalKind::CLIMATE||!ha_internal_heap_available("climate panel setup"))return nullptr;
    void *memory=allocator_.allocate(sizeof(Record));if(!memory)return nullptr;
    auto *record=new(memory)Record(*this,token,card);
    if(!record->gestures.available()){record->~Record();allocator_.release(memory,sizeof(Record));return nullptr;}
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
    auto env=style_.environment;
    if (!style_.resolve(style_.owner, token, config, env)) {
      record->~Record();
      allocator_.release(memory, sizeof(Record));
      return nullptr;
    }
    record->context=create_climate_control_context(nullptr,nullptr,nullptr,config,
        env.accent_color,env.secondary_color,env.tertiary_color,env.number_font,env.range_number_font,
        env.unit_font,env.label_font,env.option_title_font,env.option_value_font,env.option_menu_font,
        env.card_icon_font,env.icon_font,env.width_compensation_percent,nullptr,nullptr,nullptr);
    record->context->available=false;record->context->hvac_mode="off";
    record_=record;return record;
  }
  bool initialize(void *pointer,PageModalToken)override {
    auto &record=*static_cast<Record *>(pointer);
    // The central owner must finish closing the previous family first. Never
    // stack a new overlay when the shared service's old close was guarded.
    if(control_modal_active().kind!=ControlModalKind::NONE)return false;
    subscribe_climate_control_state(record.context);prime(record);
    if(record.retired||!record.context->available)return false;
    climate_control_open_modal(record.context);
    if(climate_control_modal_ui().active!=record.context)return false;
    set_clock_bar_modal_label(climate_card_label(record.context));
    return true;
  }
  bool visible(void *pointer)const override {
    const auto &record=*static_cast<Record *>(pointer);
    return !record.retired&&climate_control_modal_ui().active==record.context&&climate_control_modal_ui().overlay;
  }
  void update(void *pointer,PageModalToken)override {prime(*static_cast<Record *>(pointer));}
  ControlModalDismissPolicy dismiss_policy(void *)const override{return ControlModalDismissPolicy::DISMISS;}
  void retire(void *pointer,PageModalClose reason)override {
    auto &record=*static_cast<Record *>(pointer);record.retired=true;record.gestures.invalidate();
    auto *context=record.context;context->optional_subscriptions.clear_pending();ha_release_callbacks_for_owner(context);
    unregister_climate_control_context(context);
    if(context->debounce_timer)lv_timer_pause(context->debounce_timer);
    const bool finish=(reason==PageModalClose::USER||reason==PageModalClose::SWITCH)&&connected()&&
        record.connection==connection_generation()&&context->available&&context->pending_temp_send;
    if(finish){final_send_=context;climate_send_temperature(context);final_send_=nullptr;}
    climate_cancel_temperature_send(context);
    if(climate_control_modal_ui().active==context) {
      if(reason==PageModalClose::REPLACED||reason==PageModalClose::SHUTDOWN) {
        // Keep label/widget teardown out of durable activation. All UI text is
        // owned by this context/LVGL, so it can outlive the retired PageStore.
        auto &ui=climate_control_modal_ui();
        if(ui.overlay)lv_obj_add_flag(ui.overlay,LV_OBJ_FLAG_HIDDEN);
        if(ui.menu_overlay)lv_obj_add_flag(ui.menu_overlay,LV_OBJ_FLAG_HIDDEN);
        if(control_modal_active().overlay==ui.overlay)control_modal_service().reset_active();
        if(control_modal_nested_active().overlay==ui.menu_overlay)control_modal_service().reset_nested_menu();
      } else climate_control_hide_modal();
    }
  }
  void destroy(void *pointer)override {
    auto *record=static_cast<Record *>(pointer);assert(record==record_&&record->retired);
    delete_climate_control_context(record->context);assert(record->gestures.live()==0);record_=nullptr;
    record->~Record();allocator_.release(record,sizeof(Record));
  }
 private:
  struct Record {
    Record(PageClimateModalPort &port,PageModalToken token,ConfigurationHandle card)
        :token(token),card(card),connection(port.connection_generation()),gestures(port.allocator_,port.style_.gesture_capacity,token,&port,gesture_valid){}
    ClimateControlCtx *context{nullptr};PageModalToken token;ConfigurationHandle card;uint32_t connection;
    bool retired{false};PageModalGestures gestures;
  };
  void prime(Record &record) {
    if(record.retired||!runtime_->valid(record.token))return;
    PageEntityModalReplay replay(state_,allocator_,record.card);
    ha_read_coordinator().replay_owner_missing(record.context,replay);
    // Capability/list callbacks can demand additional optional fields. They
    // attach to existing graph channels and are seeded without reannouncement.
    for(size_t pass=0;pass<2&&!record.retired;++pass) {
      const uint8_t pending=record.context->optional_subscriptions.pending;
      if(!pending)break;
      climate_subscribe_optional_fields(record.context,pending);
      ha_read_coordinator().replay_owner_missing(record.context,replay);
    }
  }
  static bool managed(void *owner,const ClimateControlCtx *context) {
    const auto &port=*static_cast<PageClimateModalPort *>(owner);return port.record_&&port.record_->context==context;
  }
  static bool current(void *owner,const ClimateControlCtx *context) {
    const auto &port=*static_cast<PageClimateModalPort *>(owner);
    return managed(owner,context)&&!port.record_->retired&&port.runtime_->valid(port.record_->token);
  }
  static bool action(void *owner,const ClimateControlCtx *context) {
    const auto &port=*static_cast<PageClimateModalPort *>(owner);
    return context&&(context==port.final_send_||(current(owner,context)&&port.runtime_->interactive(port.record_->token)));
  }
  static bool gesture_valid(void *owner,PageModalToken token) {
    const auto &port=*static_cast<PageClimateModalPort *>(owner);return port.runtime_&&port.runtime_->interactive(token);
  }
  static void closed(void *owner,ClimateControlCtx *context) {
    auto &port=*static_cast<PageClimateModalPort *>(owner);
    if(!managed(owner,context)||port.record_->retired)return;
    port.runtime_->notify_closed(port.record_->token,!port.connected()?PageModalClose::DISCONNECTED:
        context->available?PageModalClose::USER:PageModalClose::FAILED);
  }
  static bool widgets(void *owner,void *root) {
    auto &port=*static_cast<PageClimateModalPort *>(owner);
    return port.record_&&!port.record_->retired&&port.record_->gestures.attach(static_cast<lv_obj_t *>(root));
  }
  PageArenaAllocator &allocator_;PageEntityStateRuntime &state_;PageClimateModalStyle style_;
  void *owner_{nullptr};OriginValid valid_{nullptr};PageModalRuntime *runtime_{nullptr};Record *record_{nullptr};
  ClimateControlCtx *final_send_{nullptr};
};
}  // namespace espcontrol::pages
