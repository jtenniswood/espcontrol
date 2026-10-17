#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <unordered_map>

#include "page_renderer_runtime.h"
#include "page_runtime_resources_group.h"
#include "panel_config_document.h"
#include "ha_read_coordinator.h"
#include "page_card_dependencies.h"
#include "page_entity_coordinator_adapter.h"

using namespace espcontrol;using namespace espcontrol::pages;using namespace espcontrol::configuration;
static size_t allocations=0;
void *operator new(size_t size){++allocations;if(void *p=std::malloc(size?size:1))return p;std::abort();}
void *operator new[](size_t size){return ::operator new(size);}
void operator delete(void *p)noexcept{std::free(p);}
void operator delete[](void *p)noexcept{std::free(p);}
void operator delete(void *p,size_t)noexcept{std::free(p);}
void operator delete[](void *p,size_t)noexcept{std::free(p);}
namespace {
struct Arena final:PageArenaAllocator {
  void *allocate(size_t bytes)override{if(fail)return nullptr;auto *p=std::malloc(bytes);assert(p);live[p]=bytes;return p;}
  void release(void *p,size_t bytes)override{assert(live.at(p)==bytes);live.erase(p);std::free(p);}
  bool fail{false};std::unordered_map<void *,size_t>live;
};
struct Fixture {
  Fixture(const DeviceUiLimits &limits,bool first=true,bool second=true,std::string order={}) {
    PanelConfigWriter w(document.data(),document.size());assert(w.begin()==PanelConfigStatus::OK);
    assert(w.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),std::strlen(limits.device_profile))==PanelConfigStatus::OK);
    auto button=[&](uint8_t slot,const std::string &text){assert(w.append_button(slot,reinterpret_cast<const uint8_t *>(text.data()),text.size())==PanelConfigStatus::OK);};
    auto child=[&](uint8_t slot,const std::string &text){assert(w.append_subpage(slot,reinterpret_cast<const uint8_t *>(text.data()),text.size())==PanelConfigStatus::OK);};
    if(first){button(1,";Rooms;Home;Auto;indicator;;subpage;;");child(1,"~B=Return%3AHome:FF8C00,1,2|S,,Temp,Auto,Auto,sensor.room,C,1,|,light.child,Child,Auto,Auto,,,,");}
    else button(1,"light.first;First;Auto;Auto;;;;;");
    button(2,"light.second;Second;Auto;Auto;;;;;");
    if(second){button(3,";Devices;Home;Auto;indicator;;subpage;;");child(3,"~B,1|S,,Value,Auto,Auto,sensor.device,,1,");}
    if(order.empty())for(size_t i=1;i<=limits.slot_count;++i){if(!order.empty())order+=",";order+=std::to_string(i);}
    const std::string key="button_order";
    assert(w.append_setting(reinterpret_cast<const uint8_t *>(key.data()),key.size(),reinterpret_cast<const uint8_t *>(order.data()),order.size())==PanelConfigStatus::OK);
    assert(w.finish(&size)==PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(),size,limits,17,storage.data(),storage.size(),&store)==PageStoreStatus::OK);
  }
  std::array<uint8_t,4096>document{};size_t size{0};
  alignas(std::max_align_t)std::array<uint8_t,32768>storage{};PageStore store;
};
template<size_t Slots>
struct Port final:PageRendererPort {
  explicit Port(const DeviceUiLimits &limits):shape{limits.slot_count,limits.grid_columns}{}
  PageViewGeometry geometry(const PageStore &)const override{return shape;}
  bool prepare(const PageStore &,size_t count)override{assert(count==Slots);++preparations;prepared=!fail;return !fail;}
  void discard_prepared()override{prepared=false;}
  void activate_prepared(const PageStore &active)override{assert(prepared);prepared=false;generation=active.generation();if(shells==0)shells=Slots;}
  void retire_active()override{++retirements;}
  void unbind(size_t shell,PageBindingToken token)override{
    assert(bound[shell].binding==token.binding);bound[shell]={};cells[shell]={};styles[shell]={};labels[shell][0]='\0';++unbinds;
    if(on_unbind)on_unbind();
  }
  void bind(size_t shell,const PageStore &store,const PageCellLayout &cell,PageBackStyleView back,PageBindingToken token)override{
    assert(shell<Slots);assert(store.generation()==generation);assert(token.configuration==generation);
    bound[shell]=token;cells[shell]=cell;styles[shell]=back;++binds;
  }
  void update(size_t shell,PageBindingToken token)override{
    ++updates;
    if(state!=nullptr) {
      const auto current=state->dependency({token.configuration,token.card},0);
      const size_t count=std::min(current.value.size(),labels[shell].size()-1);
      if(count!=0)std::memcpy(labels[shell].data(),current.value.data(),count);
      labels[shell][count]='\0';
      seen_revision=state->revision();
    }
    if(on_update)on_update();
  }
  void suspend()override{++suspends;}
  bool state_changed()const override{return state==nullptr?changed:state->revision()!=seen_revision;}
  PageViewGeometry shape;bool prepared{false},fail{false},changed{true};uint32_t generation{0};
  size_t shells{0},preparations{0},binds{0},unbinds{0},updates{0},suspends{0},retirements{0};
  std::array<PageBindingToken,Slots>bound{};std::array<PageCellLayout,Slots>cells{};std::array<PageBackStyleView,Slots>styles{};
  std::array<std::array<char,64>,Slots>labels{};PageEntityStateRuntime *state{nullptr};
  uint64_t seen_revision{0};
  std::function<void()>on_unbind,on_update;
};
template<size_t Slots>
void profile_navigation(const DeviceUiLimits &limits) {
  Fixture fixture(limits);Arena arena;Port<Slots>port(limits);
  {
    PageRendererRuntime<Slots>renderer(limits,arena,port);
    assert(renderer.prepare(fixture.store));assert(port.binds==0);assert(arena.live.size()==1);
    const auto before=allocations;renderer.activate_prepared(fixture.store);assert(allocations==before);
    assert(renderer.shell_count()==Slots);assert(port.shells==Slots);assert(renderer.active_page()==0);
    const auto root_token=renderer.binding(0);assert(renderer.valid(root_token));
    const auto child_page=fixture.store.card(0)->target;
    const auto pools=port.preparations;const auto arena_count=arena.live.size();
    const auto navigation_allocations=allocations;
    for(size_t cycle=0;cycle<1000;++cycle) {
      assert(renderer.open_page(child_page));assert(!renderer.valid(root_token));
      auto back=renderer.binding(0);assert(back.back&&renderer.valid(back));
      assert(port.styles[0].custom_label=="Return:Home");assert(port.styles[0].custom_color);
      assert(port.styles[0].background_color==0xFF8C00);
      auto sensor=renderer.binding(1);assert(renderer.valid(sensor));renderer.update_visible();
      assert(renderer.back());assert(!renderer.valid(sensor));assert(!renderer.valid(back));
      renderer.update_visible();
    }
    assert(allocations==navigation_allocations);assert(port.preparations==pools);assert(arena.live.size()==arena_count);
    assert(port.shells==Slots);assert(!renderer.back());
    auto token=renderer.binding(1);assert(renderer.suspend());assert(!renderer.valid(token));
    const auto updates=port.updates;renderer.update_visible();assert(port.updates==updates);
    assert(!renderer.open_page(child_page));assert(renderer.resume());assert(!renderer.valid(token));
    assert(renderer.valid(renderer.binding(1)));
    token=renderer.binding(1);port.on_update=[&](){assert(!renderer.open_page(child_page));assert(!renderer.valid(token));};
    renderer.refresh();renderer.update_visible();port.on_update=nullptr;assert(renderer.valid(token));
    port.on_unbind=[&](){assert(!renderer.valid(token));};
    renderer.retire_active();port.on_unbind=nullptr;assert(!renderer.valid(token));assert(arena.live.empty());
  }
  assert(arena.live.empty());
}
void replacements_and_failures() {
  const auto &limits=DEVICE_UI_PROFILES[8];Fixture original(limits),replacement(limits,false,true),without(limits,false,false);
  Arena arena;Port<9>port(limits);PageRendererRuntime<9>renderer(limits,arena,port);
  assert(renderer.prepare(original.store));renderer.activate_prepared(original.store);
  assert(renderer.open_page(original.store.card(2)->target));const auto old=renderer.binding(1);
  assert(renderer.prepare(replacement.store));assert(renderer.valid(old));assert(arena.live.size()==2);
  const auto allocated=allocations;renderer.activate_prepared(replacement.store);assert(allocations==allocated);
  assert(renderer.active_page()==replacement.store.card(2)->target);assert(renderer.active_page()==1);
  assert(!renderer.valid(old));assert(arena.live.size()==1);
  auto active=renderer.binding(1);port.fail=true;assert(!renderer.prepare(without.store));port.fail=false;
  assert(renderer.valid(active));assert(renderer.memory().staged_layout_bytes==0);assert(arena.live.size()==1);
  arena.fail=true;assert(!renderer.prepare(without.store));arena.fail=false;assert(renderer.valid(active));
  assert(renderer.prepare(original.store));assert(renderer.return_home());
  renderer.activate_prepared(original.store);assert(renderer.active_page()==0);
  assert(renderer.open_page(original.store.card(2)->target));
  assert(renderer.prepare(without.store));renderer.activate_prepared(without.store);assert(renderer.active_page()==0);
  assert(renderer.memory().preparation_failures==2);
  port.shape={18,3};assert(!renderer.prepare(original.store));assert(renderer.memory().staged_layout_bytes==0);
}
void layouts_and_rotation() {
  const auto &limits=DEVICE_UI_PROFILES[8];Fixture spanned(limits,true,true,"1b,2,3,4,5,6,7,8,9");
  std::array<PageCellLayout,20>cells{};PageBackAppearance back;
  assert(compile_page_layout(spanned.store,0,{9,3},cells.data(),cells.size(),&back));
  assert(cells[0].visible&&cells[0].row_span==2&&cells[0].column_span==2);
  assert(!cells[1].visible&&!cells[3].visible&&!cells[4].visible);
  Fixture overflow(limits,true,true,"999999999999999999999999999999,2,3");
  assert(compile_page_layout(overflow.store,0,{9,3},cells.data(),cells.size(),&back));assert(!cells[0].visible);assert(cells[1].visible);
  const auto &wide=DEVICE_UI_PROFILES[6];Fixture twenty(wide);assert(wide.slot_count==20);
  assert(compile_page_layout(twenty.store,0,{18,3},cells.data(),cells.size(),&back));
  assert(cells[17].visible&&!cells[18].visible&&!cells[19].visible);
  assert(twenty.store.card_count()>=20);assert(twenty.store.card(19)!=nullptr);
}
struct StateResource final:PageRuntimeResources {
  bool prepare(const PageStore &)override{prepared=true;return !fail;}
  void discard_prepared()override{prepared=false;}
  void activate_prepared(const PageStore &store)override{assert(prepared);prepared=false;generation=store.generation();}
  void retire_active()override{generation=0;}
  bool prepared{false},fail{false};uint32_t generation{0};
};
void composed_atomic_activation() {
  const auto &limits=DEVICE_UI_PROFILES[8];Fixture fixture(limits);Arena arena;Port<9>port(limits);
  PageRendererRuntime<9>renderer(limits,arena,port);StateResource state;
  PageRuntimeResourcesGroup<2>group({&renderer,&state});
  {
    PageStoreRuntime runtime(limits,arena,32768,&group);
    assert(runtime.prepare(1,fixture.document.data(),fixture.size));const auto before=allocations;
    assert(runtime.activate_prepared(8,1,fixture.document.data(),fixture.size));assert(allocations==before);
    assert(state.generation==8);assert(renderer.binding(0).configuration==8);assert(renderer.valid(renderer.binding(0)));
    const auto token=renderer.binding(0);state.fail=true;
    assert(!runtime.prepare(1,fixture.document.data(),fixture.size));assert(renderer.valid(token));
    assert(runtime.active().generation()==8);assert(state.generation==8);assert(arena.live.size()==2);
    assert(!state.prepared);assert(renderer.memory().staged_layout_bytes==0);
  }
  assert(arena.live.empty());assert(state.generation==0);
}
void binding_exhaustion_preserves_current_view() {
  const auto &limits=DEVICE_UI_PROFILES[8];Fixture fixture(limits);Arena arena;Port<9>port(limits);
  PageRendererRuntime<9>renderer(limits,arena,port,UINT32_MAX);
  assert(renderer.prepare(fixture.store));renderer.activate_prepared(fixture.store);
  const auto token=renderer.binding(0);assert(renderer.valid(token));
  assert(!renderer.open_page(fixture.store.card(0)->target));assert(renderer.valid(token));
  assert(!renderer.prepare(fixture.store));assert(renderer.valid(token));
}
struct Transport {
  using State=std::string;using Callback=std::function<void(State)>;
  static constexpr bool RECLAIMABLE_SUBSCRIPTIONS=true;
  struct Entry{std::string entity,attribute;Callback callback;uint32_t token;};
  bool available()const{return true;}bool state_connected()const{return true;}
  uint32_t subscribe(const std::string &entity,const std::string &attribute,Callback callback){
    const auto token=++next;entries.push_back({entity,attribute,std::move(callback),token});return token;
  }
  void unsubscribe(uint32_t token){entries.erase(std::remove_if(entries.begin(),entries.end(),[token](const Entry&e){return e.token==token;}),entries.end());}
  bool request(const std::string &,const std::string &){return true;}
  void publish(const std::string &entity,const std::string &value){
    for(auto &entry:entries)if(entry.entity==entity&&entry.attribute.empty()){auto callback=entry.callback;callback(value);return;}
    assert(false);
  }
  uint32_t next{0};std::vector<Entry>entries;
};
struct Heap{bool available(const char *,size_t,size_t){return true;}};
void renderer_and_real_state_owner_share_one_transaction() {
  const auto &limits=DEVICE_UI_PROFILES[8];Fixture original(limits),replacement(limits,false,true);
  Arena arena;Port<9>port(limits);PageRendererRuntime<9>renderer(limits,arena,port);
  HaReadCoordinator<Transport,Heap>coordinator;
  CoordinatorPageEntitySubscriptions adapter(coordinator,1u<<4);
  PageCardDependencies source;PageEntityStateRuntime state(source,arena,adapter,{64,64,1024});port.state=&state;
  PageRuntimeResourcesGroup<2>group({&renderer,&state});
  {
    PageStoreRuntime runtime(limits,arena,32768,&group);
    assert(runtime.prepare(1,original.document.data(),original.size));
    assert(runtime.activate_prepared(8,1,original.document.data(),original.size));
    assert(coordinator.subscription_channel_count()==4);
    coordinator.transport().publish("sensor.room","21.4");
    const auto page=runtime.active().card(0)->target;
    const auto count=coordinator.transport().entries.size();
    const auto before=allocations;
    for(size_t cycle=0;cycle<1000;++cycle){
      assert(renderer.open_page(page));renderer.update_visible();assert(std::string_view(port.labels[1].data())=="21.4");
      assert(renderer.return_home());renderer.update_visible();
    }
    assert(allocations==before);assert(coordinator.transport().entries.size()==count);
    assert(renderer.open_page(page));renderer.update_visible();const auto old_revision=state.revision();
    coordinator.invalidate_retained_state();renderer.update_visible();assert(port.labels[1][0]=='\0');
    assert(state.revision()!=old_revision);
    coordinator.transport().publish("sensor.room","22.1");renderer.update_visible();assert(std::string_view(port.labels[1].data())=="22.1");
    const auto token=renderer.binding(1);
    assert(runtime.prepare(1,replacement.document.data(),replacement.size));
    port.on_unbind=[&](){assert(state.dependency({8,token.card},0).value=="22.1");};
    const auto activation_allocations=allocations;
    assert(runtime.activate_prepared(9,1,replacement.document.data(),replacement.size));
    assert(allocations==activation_allocations);port.on_unbind=nullptr;
    assert(!renderer.valid(token));assert(renderer.active_page()==0);
    coordinator.reclaim_unused_channels();assert(coordinator.transport().entries.size()==3);
    assert(state.memory().active_channels==3);assert(arena.live.size()==3);
  }
  coordinator.reclaim_unused_channels();assert(coordinator.transport().entries.empty());assert(arena.live.empty());
}
}  // namespace
int main() {
  for(const auto &limits:DEVICE_UI_PROFILES) {
    switch(limits.slot_count) {
      case 6:profile_navigation<6>(limits);break;
      case 9:profile_navigation<9>(limits);break;
      case 15:profile_navigation<15>(limits);break;
      case 20:profile_navigation<20>(limits);break;
      default:assert(false);
    }
  }
  replacements_and_failures();layouts_and_rotation();composed_atomic_activation();binding_exhaustion_preserves_current_view();
  renderer_and_real_state_owner_share_one_transaction();
}
