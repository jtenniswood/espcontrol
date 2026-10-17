#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "esphome/core/string_ref.h"
#include "ha_read_coordinator.h"
#include "page_entity_coordinator_adapter.h"
#include "page_entity_modal_replay.h"
#include "panel_config_document.h"

using namespace espcontrol;using namespace espcontrol::pages;using namespace espcontrol::configuration;
namespace {
struct Arena:PageArenaAllocator {
  void *allocate(size_t size)override{if(fail)return nullptr;auto *p=std::malloc(size);assert(p);live[p]=size;if(on_allocate){auto call=std::move(on_allocate);on_allocate={};call();}return p;}
  void release(void *p,size_t size)override{assert(live.at(p)==size);live.erase(p);std::free(p);}
  bool fail{false};std::function<void()>on_allocate;std::unordered_map<void *,size_t>live;
};
struct Transport {
  using State=esphome::StringRef;using Callback=std::function<void(State)>;
  static constexpr bool RECLAIMABLE_SUBSCRIPTIONS=true;
  struct Binding{uint32_t token;std::string entity,attribute;Callback callback;};
  bool available()const{return api;}bool state_connected()const{return connected;}
  uint32_t subscribe(std::string entity,std::string attribute,Callback callback){bindings.push_back({next++,std::move(entity),std::move(attribute),std::move(callback)});return next-1;}
  void unsubscribe(uint32_t token){bindings.erase(std::remove_if(bindings.begin(),bindings.end(),[token](const auto &b){return b.token==token;}),bindings.end());}
  bool request(const std::string &,const std::string &){++requests;return true;}
  void send(std::string entity,std::string attribute,std::string_view value){for(auto &b:bindings)if(b.entity==entity&&b.attribute==attribute){const auto callback=b.callback;callback({value.data(),value.size()});return;}}
  bool api{true},connected{true};uint32_t next{1};size_t requests{0};std::vector<Binding>bindings;
};
struct Heap{bool available(const char *,size_t,size_t)const{return true;}};
using Coordinator=HaReadCoordinator<Transport,Heap>;
struct Source:PageDependencySource {
  bool enumerate(const PageStore &,PageDependencySink &sink)const override {
    return sink.add(0,"climate.room","")&&sink.add(0,"climate.room","temperature")&&sink.add(0,"climate.room","hvac_modes")&&sink.add(1,"sensor.other","");
  }
};
struct Fixture {
  Fixture(){const auto &limits=DEVICE_UI_PROFILES[8];PanelConfigWriter writer(document.data(),document.size());assert(writer.begin()==PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),std::strlen(limits.device_profile))==PanelConfigStatus::OK);
    const std::string config="climate.room;Room;;;;;climate_control;1;";
    assert(writer.append_button(1,reinterpret_cast<const uint8_t *>(config.data()),config.size())==PanelConfigStatus::OK);assert(writer.finish(&size)==PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(),size,limits,17,arena.data(),arena.size(),&store)==PageStoreStatus::OK);}
  std::array<uint8_t,2048>document{};size_t size{0};alignas(std::max_align_t)std::array<uint8_t,16384>arena{};PageStore store;
};
void real_cache_and_modal_lifetime() {
  Fixture fixture;Arena arena;Source source;Coordinator coordinator;CoordinatorPageEntitySubscriptions subscriptions(coordinator,16);
  PageEntityStateRuntime state(source,arena,subscriptions,{8,8,2048});assert(state.prepare(fixture.store));state.activate_prepared(fixture.store);
  coordinator.transport().send("climate.room","","heat");coordinator.transport().send("climate.room","temperature","21.5");
  coordinator.transport().send("climate.room","hvac_modes","['off','heat','cool']");coordinator.transport().send("sensor.other","","FOREIGN");
  const auto channels=coordinator.subscription_channel_count(),native=coordinator.transport().bindings.size();assert(channels==4);
  PageEntityModalReplay replay(state,arena,{17,0});int owner=0,foreign_owner=0;size_t values=0,foreign_values=0;std::string temperature;
  auto receive=[&](Transport::State value){++values;assert(value.c_str()[value.size()]=='\0');temperature.assign(value.c_str(),value.size());};
  assert(coordinator.subscribe("climate.room","temperature",receive,32,&owner,false,false));assert(values==0);
  assert(coordinator.subscribe("climate.room","temperature",[&](Transport::State){++foreign_values;},32,&foreign_owner,false,false));
  assert(coordinator.replay_owner_missing(&owner,replay)==1&&values==1&&temperature=="21.5"&&foreign_values==0);
  assert(coordinator.replay_owner_missing(&owner,replay)==0&&values==1);
  // Lazily registered controls get their own first value, without resending the
  // original temperature and overwriting a user's pending local edit.
  std::string modes;assert(coordinator.subscribe("climate.room","hvac_modes",[&](Transport::State value){modes.assign(value.c_str(),value.size());},32,&owner,false,false));
  temperature="pending 23";assert(coordinator.replay_owner_missing(&owner,replay)==1&&modes=="['off','heat','cool']"&&temperature=="pending 23");
  coordinator.transport().send("climate.room","temperature","22.5");assert(values==2&&temperature=="22.5"&&foreign_values==1);
  assert(coordinator.replay_owner_missing(&owner,replay)==0);
  assert(coordinator.subscription_channel_count()==channels&&coordinator.transport().bindings.size()==native&&coordinator.transport().requests==0);
  coordinator.release_owner(&owner);coordinator.release_owner(&foreign_owner);coordinator.reclaim_unused_channels();assert(coordinator.subscription_count()==4);
  for(size_t cycle=0;cycle<1000;++cycle){assert(coordinator.subscribe("climate.room","temperature",receive,32,&owner,false,false));assert(coordinator.replay_owner_missing(&owner,replay)==1);coordinator.release_owner(&owner);coordinator.reclaim_unused_channels();assert(coordinator.subscription_count()==4&&coordinator.subscription_channel_count()==channels&&coordinator.transport().bindings.size()==native&&arena.live.size()==1);}
  bool empty=false;coordinator.transport().send("climate.room","hvac_modes","");
  assert(coordinator.subscribe("climate.room","hvac_modes",[&](Transport::State value){empty=value.size()==0&&value.c_str()[0]=='\0';},32,&owner,false,false));
  assert(coordinator.replay_owner_missing(&owner,replay)==1&&empty);coordinator.release_owner(&owner);
  // Cache retirement during the callback must not invalidate its borrowed State.
  const auto original=state.dependency({17,0},1).value.data();bool retired=false;
  assert(coordinator.subscribe("climate.room","temperature",[&](Transport::State value){assert(value.c_str()!=original);state.retire_active();coordinator.release_owner(&owner);assert(std::string_view(value.c_str(),value.size())=="22.5");retired=true;},32,&owner,false,false));
  assert(coordinator.replay_owner_missing(&owner,replay)==1&&retired&&arena.live.empty());coordinator.reclaim_unused_channels();assert(coordinator.transport().bindings.empty());
}
void failures_and_generation_guards() {
  Fixture fixture;Arena arena;Source source;Coordinator coordinator;CoordinatorPageEntitySubscriptions subscriptions(coordinator,16);PageEntityStateRuntime state(source,arena,subscriptions,{8,8,128});
  assert(state.prepare(fixture.store));state.activate_prepared(fixture.store);coordinator.transport().send("climate.room","temperature","20");
  int owner=0;size_t calls=0;assert(coordinator.subscribe("climate.room","temperature",[&](Transport::State){++calls;},32,&owner,false,false));
  PageEntityModalReplay replay(state,arena,{17,0}),wrong(state,arena,{16,0});assert(coordinator.replay_owner_missing(&owner,wrong)==0);
  arena.fail=true;assert(coordinator.replay_owner_missing(&owner,replay)==0&&calls==0);
  auto failed=[&](std::string_view,std::string_view){return std::optional<PageEntityReplaySnapshot>(std::in_place,arena,state,state.revision(),4);};
  assert(coordinator.replay_owner_missing(&owner,failed)==0&&calls==0);arena.fail=false;
  coordinator.transport().connected=false;assert(coordinator.replay_owner_missing(&owner,replay)==0);coordinator.transport().connected=true;
  coordinator.transport().api=false;assert(coordinator.replay_owner_missing(&owner,replay)==0);coordinator.transport().api=true;
  assert(coordinator.replay_owner_missing(nullptr,replay)==0);
  arena.on_allocate=[&](){coordinator.invalidate_retained_state();};assert(coordinator.replay_owner_missing(&owner,replay)==0&&calls==0&&arena.live.size()==1);
  assert(coordinator.replay_owner_missing(&owner,replay)==0);coordinator.transport().send("climate.room","temperature","fresh");assert(calls==1);assert(coordinator.replay_owner_missing(&owner,replay)==0);
  coordinator.release_owner(&owner);
  // A live value delivered while Source was obtaining a snapshot wins.
  std::string seen;assert(coordinator.subscribe("climate.room","temperature",[&](Transport::State value){seen.assign(value.c_str(),value.size());},32,&owner,false,false));
  auto concurrent=[&](std::string_view entity,std::string_view attr){auto snapshot=replay(entity,attr);coordinator.transport().send("climate.room","temperature","NEWEST");return snapshot;};
  assert(coordinator.replay_owner_missing(&owner,concurrent)==0&&seen=="NEWEST");coordinator.release_owner(&owner);
  // An address-reused owner created from the old callback is seeded only by a
  // subsequent setup step. It cannot be included in the in-flight old batch.
  size_t replacement=0;assert(coordinator.subscribe("climate.room","temperature",[&](Transport::State){coordinator.release_owner(&owner);assert(coordinator.subscribe("climate.room","temperature",[&](Transport::State){++replacement;},32,&owner,false,false));assert(coordinator.replay_owner_missing(&owner,replay)==0);},32,&owner,false,false));
  assert(coordinator.replay_owner_missing(&owner,replay)==1&&replacement==0);assert(coordinator.replay_owner_missing(&owner,replay)==1&&replacement==1);coordinator.release_owner(&owner);
  // Values not declared for this card are never replayed into its modal.
  assert(coordinator.subscribe("sensor.other","",[&](Transport::State){++calls;},32,&owner,false,false));coordinator.transport().send("sensor.other","","foreign");
  coordinator.release_owner(&owner);assert(coordinator.subscribe("sensor.other","",[&](Transport::State){++calls;},32,&owner,false,false));assert(coordinator.replay_owner_missing(&owner,replay)==0);
  coordinator.release_owner(&owner);
  assert(coordinator.subscribe("climate.room","temperature",[&](Transport::State){++calls;},32,&owner,false,false));const auto before=calls;
  auto replaced=[&](std::string_view entity,std::string_view attr){auto snapshot=replay(entity,attr);state.retire_active();return snapshot;};
  assert(coordinator.replay_owner_missing(&owner,replaced)==0&&calls==before);
  coordinator.release_owner(&owner);coordinator.reclaim_unused_channels();assert(arena.live.empty());
}
void cover_position_history() {
  struct Positions:PageDependencySource {bool enumerate(const PageStore &,PageDependencySink &sink)const override{return sink.add(0,"cover.room","current_position")&&sink.add(0,"cover.room","current_tilt_position");}} source;
  Fixture fixture;Arena arena;Coordinator coordinator;CoordinatorPageEntitySubscriptions subscriptions(coordinator,4);PageEntityStateRuntime state(source,arena,subscriptions,{4,4,64});assert(state.prepare(fixture.store));state.activate_prepared(fixture.store);PageEntityModalReplay replay(state,arena,{17,0});
  coordinator.transport().send("cover.room","current_position","42.5");coordinator.transport().send("cover.room","current_position","bad");coordinator.transport().send("cover.room","current_tilt_position","20");coordinator.transport().send("cover.room","current_tilt_position","nan");
  {auto position=replay("cover.room","current_position"),tilt=replay("cover.room","current_tilt_position");assert(position&&position->current()&&std::string_view(position->data(),position->size())=="43");assert(tilt&&std::string_view(tilt->data(),tilt->size())=="20");}
  coordinator.transport().send("cover.room","current_position",std::string(200,'x'));{auto position=replay("cover.room","current_position");assert(position&&std::string_view(position->data(),position->size())=="43");}
  coordinator.transport().connected=false;coordinator.invalidate_retained_state();assert(!replay("cover.room","current_position"));coordinator.transport().connected=true;assert(!replay("cover.room","current_tilt_position"));state.retire_active();assert(arena.live.empty());
}

}
int main(){cover_position_history();real_cache_and_modal_lifetime();failures_and_generation_guards();}
