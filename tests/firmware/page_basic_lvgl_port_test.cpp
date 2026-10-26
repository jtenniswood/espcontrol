#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "page_basic_lvgl_port.h"
#include "page_label_refresh.h"
#include "page_modal_gestures.h"
#include "page_card_dependencies.h"
#include "page_runtime_resources_group.h"
#include "panel_config_document.h"

using namespace espcontrol;using namespace espcontrol::pages;using namespace espcontrol::configuration;
static size_t lv_allocations=0,lv_live_bytes=0,cpp_allocations=0;
void *operator new(size_t size){++cpp_allocations;if(void *p=std::malloc(size?size:1))return p;std::abort();}
void *operator new[](size_t size){return ::operator new(size);}
void operator delete(void *p)noexcept{std::free(p);}
void operator delete[](void *p)noexcept{std::free(p);}
void operator delete(void *p, size_t) noexcept { std::free(p); }
void operator delete[](void *p, size_t) noexcept { std::free(p); }
struct alignas(std::max_align_t) Allocation {size_t bytes;};
extern "C" {
void lv_mem_init(){}
void lv_mem_deinit(){}
lv_mem_pool_t lv_mem_add_pool(void *,size_t){return nullptr;}
void lv_mem_remove_pool(lv_mem_pool_t){}
void *lv_malloc_core(size_t size){++lv_allocations;auto *p=static_cast<Allocation *>(std::malloc(size+sizeof(Allocation)));assert(p);p->bytes=size;lv_live_bytes+=size;return p+1;}
void lv_free_core(void *ptr){if(!ptr)return;auto *p=static_cast<Allocation *>(ptr)-1;lv_live_bytes-=p->bytes;std::free(p);}
void *lv_realloc_core(void *ptr,size_t size){void *p=lv_malloc_core(size);if(ptr){std::memcpy(p,ptr,std::min(size,(static_cast<Allocation *>(ptr)-1)->bytes));lv_free_core(ptr);}return p;}
void lv_mem_monitor_core(lv_mem_monitor_t *out){*out={};}
lv_result_t lv_mem_test_core(){return LV_RESULT_OK;}
}
namespace {
struct Arena:PageArenaAllocator {
  void *allocate(size_t bytes)override{++allocations;if(fail)return nullptr;live+=bytes;return std::malloc(bytes);}
  void release(void *ptr,size_t bytes)override{live-=bytes;std::free(ptr);}
  size_t live{0},allocations{0};bool fail{false};
};
struct Subscriptions:PageEntitySubscriptions {
  struct Binding {const char *entity,*attribute;void *owner;uint32_t revision,channel;Deliver deliver;};
  std::vector<Binding>bindings;uint32_t generation{1};
  bool subscribe(const char *entity,const char *attribute,void *owner,uint32_t revision,uint32_t channel,Deliver deliver)override{bindings.push_back({entity,attribute,owner,revision,channel,deliver});return true;}
  void release(void *owner)override{bindings.erase(std::remove_if(bindings.begin(),bindings.end(),[owner](const auto &b){return b.owner==owner;}),bindings.end());}
  uint32_t connection_generation()const override{return generation;}
  void send(const char *entity,const char *attribute,std::string_view value){for(auto &b:bindings)if(std::strcmp(b.entity,entity)==0&&std::strcmp(b.attribute,attribute)==0)b.deliver(b.owner,b.revision,b.channel,value);}
};
struct Fixture {
  explicit Fixture(const DeviceUiLimits &limits,bool unsupported=false,bool portrait=false,bool stateless=false) {
    PanelConfigWriter writer(document.data(),document.size());assert(writer.begin()==PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),std::strlen(limits.device_profile))==PanelConfigStatus::OK);
    auto button=[&](uint8_t slot,std::string text){assert(writer.append_button(slot,reinterpret_cast<const uint8_t *>(text.data()),text.size())==PanelConfigStatus::OK);};
    if(stateless) {
      button(1,";Actions;Home;Lightbulb;;;subpage;;");
      button(2,"script.root;Run;Flash;Lightbulb;script.turn_on;;action;;state_entity=binary_sensor.action,state_precision=icon");
      button(3,";;Auto;Auto;binary_sensor.presence;;presence;;active_color");
      button(4,";;Auto;Auto;binary_sensor.window;;door_window;window;active_color");
      button(5,"test_local;;;;local;;action;;");button(6,";Ignored;Auto;Auto;;;screensaver;;");
      const std::string child="~B,1,2,3,4|A,script.child,Power,Flash,Lightbulb,script.turn_on,,0,state_entity=sensor.power%2Cstate_precision=2%2Cstate_unit="+std::string(600,'W')+
        "|A,script.text,Message,Flash,Lightbulb,script.turn_on,,0,state_entity=sensor.message%2Cstate_precision=text|P,,Push,Gesture Tap,Auto,,,,|WH,https://example.test/hook,Send,Auto,Auto,,,,";
      assert(writer.append_subpage(1,reinterpret_cast<const uint8_t *>(child.data()),child.size())==PanelConfigStatus::OK);
      const std::string key="button_order",order="1,2,3,4,5,6";
      assert(writer.append_setting(reinterpret_cast<const uint8_t *>(key.data()),key.size(),reinterpret_cast<const uint8_t *>(order.data()),order.size())==PanelConfigStatus::OK);
      assert(writer.finish(&size)==PanelConfigStatus::OK);
      assert(PageStore::stage_v1(document.data(),size,limits,11,arena.data(),arena.size(),&store)==PageStoreStatus::OK);return;
    }
    button(1,";Rooms;Home;Lightbulb;indicator;;subpage;;");
    button(2,"light.main;;Lightbulb;Lightbulb;sensor.temp; C ;;1;card_off_color=336699");
    button(3,unsupported?"media_player.test;;;;control;;media;;":";;Gauge;Gauge;sensor.temp;°C;sensor;1;active_color");
    button(4,";;;;sensor.message;;sensor;text;state_labels,state_input=active,state_output="+std::string(600,'x'));
    button(5,";Timer;;;sensor.timer;;sensor;time;");
    button(6,"switch.cycle;Standby;Auto;Auto;sensor.message;;;text;");
    std::string child="~B=Go%20Home:FFFFFF,1,2,3,4|S,,Child,Auto,Auto,sensor.temp,°C,1,|,light.child,Child switch,Auto,Auto,,,,|S,,Message,Auto,Auto,sensor.message,,text,|HC,climate.room,Climate,Auto,Lightbulb,,,0,label_display=status%2Cnumber_display=target%2Ctemperature_step=0.5|,light.omitted,Omitted,Auto,Auto,,,,";
    if(portrait) {
      const auto separator=child.find('|');std::string order="~B";
      for(size_t i=1;i<limits.slot_count;++i)order+=","+std::to_string(i);
      child=order+child.substr(separator);
      for(size_t i=6;i<limits.slot_count;++i)child+="|,light.extra"+std::to_string(i)+",Extra,Auto,Auto,,,,";
    }
    assert(writer.append_subpage(1,reinterpret_cast<const uint8_t *>(child.data()),child.size())==PanelConfigStatus::OK);
    const std::string key="button_order",order="1,2,3,4,5,6";
    assert(writer.append_setting(reinterpret_cast<const uint8_t *>(key.data()),key.size(),reinterpret_cast<const uint8_t *>(order.data()),order.size())==PanelConfigStatus::OK);
    assert(writer.finish(&size)==PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(),size,limits,11,arena.data(),arena.size(),&store)==PageStoreStatus::OK);
  }
  std::array<uint8_t,8192>document{};size_t size{0};alignas(std::max_align_t)std::array<uint8_t,65536>arena{};PageStore store;
};
template<size_t Slots>struct Actions {
  PageRendererRuntime<Slots> *renderer{nullptr};size_t toggles{0},climates{0},commands{0};PageBindingToken last;char entity[64]{};
  static const char *icon(void *,std::string_view name,std::string_view){return name=="Lightbulb"?"ON":"I";}
  static bool valid(void *owner,PageBindingToken t){return static_cast<Actions *>(owner)->renderer->valid(t);}
  static void toggle(void *owner,PageBindingToken t,std::string_view entity){auto &a=*static_cast<Actions *>(owner);++a.toggles;a.last=t;assert(entity.size()<sizeof(a.entity));std::memcpy(a.entity,entity.data(),entity.size());a.entity[entity.size()]='\0';}
  static void open(void *owner,PageBindingToken,PageId page){assert(static_cast<Actions *>(owner)->renderer->open_page(page));}
  static void back(void *owner,PageBindingToken){assert(static_cast<Actions *>(owner)->renderer->back());}
  static void climate(void *owner,PageBindingToken token){auto &a=*static_cast<Actions *>(owner);++a.climates;a.last=token;}
  static void action(void *owner,PageBindingToken token){auto &a=*static_cast<Actions *>(owner);++a.commands;a.last=token;}
  PageBasicActions callbacks(){return {this,icon,valid,toggle,open,back,climate,action};}
};
void click(lv_obj_t *object){lv_obj_send_event(object,LV_EVENT_PRESSED,nullptr);lv_obj_send_event(object,LV_EVENT_RELEASED,nullptr);lv_obj_send_event(object,LV_EVENT_CLICKED,nullptr);}
template<size_t Slots>void profile(const DeviceUiLimits &limits) {
  Fixture fixture(limits);Arena arena;Subscriptions subscriptions;PageCardDependencies dependencies;
  PageEntityStateRuntime state(dependencies,arena,subscriptions,{2048,2048,8192});
  auto *page=lv_obj_create(lv_screen_active());
  std::array<lv_coord_t,Slots+1>columns{},rows{};
  const auto row_count=(Slots+limits.grid_columns-1)/limits.grid_columns;
  for(size_t i=0;i<limits.grid_columns;++i)columns[i]=LV_GRID_FR(1);columns[limits.grid_columns]=LV_GRID_TEMPLATE_LAST;
  for(size_t i=0;i<row_count;++i)rows[i]=LV_GRID_FR(1);rows[row_count]=LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page,480,480);lv_obj_set_grid_dsc_array(page,columns.data(),rows.data());
  std::array<PageBasicWidgets,Slots>widgets{};
  for(auto &w:widgets) {
    w.button=lv_button_create(page);w.icon=lv_label_create(w.button);w.label=lv_label_create(w.button);
    w.sensor_container=lv_obj_create(w.button);w.value=lv_label_create(w.sensor_container);w.unit=lv_label_create(w.sensor_container);
  }
  const auto objects=lv_obj_get_child_count(page);assert(objects==Slots);
  PageBasicTheme theme;theme.sensor_font=&lv_font_montserrat_14;Actions<Slots> actions;
  {
    auto invalid=widgets;invalid[1]=invalid[0];
    PageBasicLvglPort<Slots>bad(arena,state,invalid,{limits.slot_count,limits.grid_columns},theme,actions.callbacks());
    assert(!bad.initialize());
  }
  {
    auto callbacks=actions.callbacks();callbacks.climate=nullptr;
    PageBasicLvglPort<Slots>missing(arena,state,widgets,{limits.slot_count,limits.grid_columns},theme,callbacks);
    assert(missing.initialize());assert(!missing.prepare(fixture.store,Slots));
  }
  {
    PageLabelRefresh refresh(arena);
    PageBasicLvglPort<Slots>port(arena,state,widgets,{limits.slot_count,limits.grid_columns},theme,actions.callbacks());
    assert(port.initialize());assert(port.initialize());
    std::array<lv_obj_t *,Slots*4> labels{};size_t label_index=0;
    for(const auto &w:widgets)for(auto *label:{w.icon,w.label,w.value,w.unit})labels[label_index++]=label;
    assert(refresh.initialize(labels.data(),labels.size()));
    assert(port.memory().fixed_bytes==sizeof(port)&&port.memory().borrowed_widgets==6*Slots);
    PageRendererRuntime<Slots>renderer(limits,arena,port);actions.renderer=&renderer;
    assert(renderer.prepare(fixture.store));assert(state.prepare(fixture.store));
    renderer.activate_prepared(fixture.store);state.activate_prepared(fixture.store);
    subscriptions.send("sensor.temp","","21.4");subscriptions.send("light.main","","on");subscriptions.send("light.main","friendly_name",u8"Ṣade lamp");
    subscriptions.send("sensor.message","","pre_wash");subscriptions.send("sensor.timer","","90");subscriptions.send("sensor.timer","unit_of_measurement","seconds");subscriptions.send("switch.cycle","","on");
    renderer.update_visible();
    subscriptions.send("light.omitted","","on");renderer.update_visible();
    assert(!lv_obj_has_state(widgets[0].button,LV_STATE_CHECKED));
    subscriptions.send("light.child","","on");renderer.update_visible();
    assert(lv_obj_has_state(widgets[0].button,LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[0].icon),"ON")==0);
    assert(std::strcmp(lv_label_get_text(widgets[3].label),"Pre Wash")==0);
    assert(std::strcmp(lv_label_get_text(widgets[4].value),"1m")==0);
    assert(std::strcmp(lv_label_get_text(widgets[5].label),"Pre Wash")==0);
    assert(lv_obj_has_flag(widgets[5].sensor_container,LV_OBJ_FLAG_HIDDEN));
    subscriptions.send("sensor.message","","active");renderer.update_visible();
    assert(std::strlen(lv_label_get_text(widgets[3].label))==600);
    subscriptions.send("sensor.message","","pre_wash");renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label),"Sade lamp")==0);
    assert(std::strcmp(lv_label_get_text(widgets[1].value),"21.4")==0);
    assert(std::strcmp(lv_label_get_text(widgets[1].unit),"C")==0);
    assert(lv_obj_has_state(widgets[1].button,LV_STATE_CHECKED));
    click(widgets[1].button);assert(actions.toggles==1&&std::strcmp(actions.entity,"light.main")==0);
    const auto child=fixture.store.card(0)->target;
    // A press begun on a root control must not trigger a child control when
    // CLICKED is delivered after the slot has been rebound.
    lv_obj_send_event(widgets[2].button,LV_EVENT_PRESSED,nullptr);assert(renderer.open_page(child));renderer.update_visible();
    lv_obj_send_event(widgets[2].button,LV_EVENT_CLICKED,nullptr);assert(actions.toggles==1);
    click(widgets[2].button);assert(actions.toggles==2&&std::strcmp(actions.entity,"light.child")==0);
    subscriptions.send("climate.room","","cool");subscriptions.send("climate.room","hvac_action","cooling");
    subscriptions.send("climate.room","supported_features","426");subscriptions.send("climate.room","target_temp_low","65");subscriptions.send("climate.room","target_temp_high","75");renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[4].value),"65.0-75.0")==0);
    assert(std::strcmp(lv_label_get_text(widgets[4].label),"Cooling")==0&&lv_obj_has_state(widgets[4].button,LV_STATE_CHECKED));
    click(widgets[4].button);assert(actions.climates==1&&actions.renderer->valid(actions.last));
    subscriptions.send("climate.room","hvac_action","idle");renderer.update_visible();
    assert(!lv_obj_has_state(widgets[4].button,LV_STATE_CHECKED)&&std::strcmp(lv_label_get_text(widgets[4].label),"Idle")==0);
    lv_obj_send_event(widgets[4].button,LV_EVENT_PRESSED,nullptr);
    assert(renderer.back());renderer.update_visible();lv_obj_send_event(widgets[4].button,LV_EVENT_CLICKED,nullptr);assert(actions.climates==1);
    assert(renderer.open_page(child));renderer.update_visible();lv_obj_send_event(widgets[4].button,LV_EVENT_CLICKED,nullptr);assert(actions.climates==1);
    assert(std::strcmp(lv_label_get_text(widgets[0].label),"Go Home")==0);
    click(widgets[0].button);assert(renderer.active_page()==0);renderer.update_visible();
    // Warm every configured type/state before checking LVGL's own allocations.
    assert(renderer.open_page(child));renderer.update_visible();assert(renderer.back());renderer.update_visible();
    lv_obj_update_layout(page);
    const auto lv_before=lv_allocations,cpp_before=cpp_allocations,arena_before=arena.allocations,live_before=lv_live_bytes;
    const auto channels=subscriptions.bindings.size();
    size_t layout_allocations=0,library_label_allocations=0;
    for(size_t cycle=0;cycle<1000;++cycle) {
      const auto before=lv_allocations;
      assert(renderer.open_page(child));renderer.update_visible();assert(renderer.back());renderer.update_visible();
      library_label_allocations+=lv_allocations-before;assert(cpp_allocations==cpp_before);assert(arena.allocations==arena_before);
      const auto before_layout=lv_allocations;
      lv_obj_update_layout(page);layout_allocations+=lv_allocations-before_layout;
      assert(lv_live_bytes==live_before);assert(subscriptions.bindings.size()==channels);assert(lv_obj_get_child_count(page)==objects);
    }
    assert(library_label_allocations==0);
    assert(lv_allocations==lv_before+layout_allocations);
    std::printf("%s: 1000 cycles, 0 C++/arena allocations, %zu LVGL label-event and %zu grid scratch allocations, stable %zu LVGL bytes\n",limits.device_profile,library_label_allocations,layout_allocations,lv_live_bytes);
    assert(port.memory().text_truncations==0&&port.memory().invalid_text==0);
    ++subscriptions.generation;renderer.update_visible();assert(std::strcmp(lv_label_get_text(widgets[1].value),"")==0);
    assert(!lv_obj_has_state(widgets[1].button,LV_STATE_CHECKED));
    assert(!lv_obj_has_state(widgets[0].button,LV_STATE_CHECKED));
    subscriptions.send("sensor.temp","","19.5");renderer.update_visible();assert(std::strcmp(lv_label_get_text(widgets[2].value),"19.5")==0);
    Fixture unsupported(limits,true);const auto token=renderer.binding(1);assert(!renderer.prepare(unsupported.store));assert(renderer.valid(token));
    arena.fail=true;assert(!renderer.prepare(fixture.store));arena.fail=false;assert(renderer.valid(token));
    theme.text=0x123456;++theme.revision;renderer.update_visible();
    // Repeated successful replacement must detach static label pointers before
    // releasing the retired text arena. Alternate model arenas, as production
    // preparation must never overwrite the currently published model.
    Fixture replacement_a(limits),replacement_b(limits);
    const auto active_bytes=port.memory().active_bytes;
    for(uint32_t generation=20;generation<120;++generation) {
      auto &next=generation%2?replacement_a:replacement_b;
      next.store={}; // This alternate arena is no longer published.
      assert(PageStore::stage_v1(next.document.data(),next.size,limits,generation,next.arena.data(),next.arena.size(),&next.store)==PageStoreStatus::OK);
      const auto old=renderer.binding(1);
      assert(renderer.prepare(next.store));assert(state.prepare(next.store));
      renderer.activate_prepared(next.store);state.activate_prepared(next.store);renderer.update_visible();lv_obj_update_layout(page);
      assert(!renderer.valid(old));assert(port.memory().active_bytes==active_bytes);
      assert(port.memory().staged_bytes==0&&port.memory().peak_bytes==2*active_bytes);
      assert(lv_obj_get_child_count(page)==objects&&subscriptions.bindings.size()==channels);
    }
    const auto unchanged_allocations=lv_allocations;
    for(size_t i=0;i<100;++i){renderer.refresh();renderer.update_visible();}
    assert(lv_allocations==unchanged_allocations);
    const auto retired=renderer.binding(1);assert(renderer.suspend());lv_obj_send_event(widgets[1].button,LV_EVENT_PRESSED,nullptr);lv_obj_send_event(widgets[1].button,LV_EVENT_CLICKED,nullptr);assert(actions.toggles==2);
    assert(renderer.resume());assert(!renderer.valid(retired));renderer.update_visible();
    renderer.retire_active();assert(std::strcmp(lv_label_get_text(widgets[1].label),"")==0);state.retire_active();
  }
  assert(arena.live==0);
  if(std::strcmp(limits.device_profile,"guition-esp32-p4-jc8012p4a1-v3")==0) {
    // Retain all twenty configuration slots while only eighteen can be bound.
    for(size_t i=0;i<3;++i)columns[i]=LV_GRID_FR(1);columns[3]=LV_GRID_TEMPLATE_LAST;
    for(size_t i=0;i<6;++i)rows[i]=LV_GRID_FR(1);rows[6]=LV_GRID_TEMPLATE_LAST;
    lv_obj_set_grid_dsc_array(page,columns.data(),rows.data());
    Fixture portrait(limits,false,true);
    PageBasicLvglPort<Slots>port(arena,state,widgets,{18,3},theme,actions.callbacks());assert(port.initialize());
    PageRendererRuntime<Slots>renderer(limits,arena,port);actions.renderer=&renderer;
    assert(renderer.prepare(portrait.store));assert(state.prepare(portrait.store));renderer.activate_prepared(portrait.store);state.activate_prepared(portrait.store);
    subscriptions.send("light.extra19","","on");renderer.update_visible();assert(!lv_obj_has_state(widgets[0].button,LV_STATE_CHECKED));
    subscriptions.send("light.extra17","","on");renderer.update_visible();assert(lv_obj_has_state(widgets[0].button,LV_STATE_CHECKED));
    assert(portrait.store.page(portrait.store.card(0)->target)->card_count==19);
    assert(renderer.open_page(portrait.store.card(0)->target));renderer.update_visible();assert(renderer.back());renderer.update_visible();lv_obj_update_layout(page);
    const auto before_live=lv_live_bytes,before_cpp=cpp_allocations,before_arena=arena.allocations,before_channels=subscriptions.bindings.size();
    for(size_t cycle=0;cycle<1000;++cycle) {
      assert(renderer.open_page(portrait.store.card(0)->target));renderer.update_visible();assert(renderer.binding(18).binding==0&&renderer.binding(19).binding==0);
      assert(renderer.back());renderer.update_visible();lv_obj_update_layout(page);assert(lv_live_bytes==before_live);
    }
    assert(cpp_allocations==before_cpp&&arena.allocations==before_arena&&subscriptions.bindings.size()==before_channels);
    renderer.retire_active();state.retire_active();
  }
  assert(arena.live==0);lv_obj_delete(page);
}
template<size_t Slots>void status_action_profile(const DeviceUiLimits &limits) {
  Fixture fixture(limits,false,false,true);Arena arena;Subscriptions subscriptions;PageCardDependencies dependencies;
  PageEntityStateRuntime state(dependencies,arena,subscriptions,{2048,2048,8192});
  auto *page=lv_obj_create(lv_screen_active());
  std::array<lv_coord_t,Slots+1>columns{},rows{};
  const auto row_count=(Slots+limits.grid_columns-1)/limits.grid_columns;
  for(size_t i=0;i<limits.grid_columns;++i)columns[i]=LV_GRID_FR(1);columns[limits.grid_columns]=LV_GRID_TEMPLATE_LAST;
  for(size_t i=0;i<row_count;++i)rows[i]=LV_GRID_FR(1);rows[row_count]=LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page,480,480);lv_obj_set_grid_dsc_array(page,columns.data(),rows.data());
  std::array<PageBasicWidgets,Slots>widgets{};
  for(auto &w:widgets) {
    w.button=lv_button_create(page);w.icon=lv_label_create(w.button);w.label=lv_label_create(w.button);
    w.sensor_container=lv_obj_create(w.button);w.value=lv_label_create(w.sensor_container);w.unit=lv_label_create(w.sensor_container);
    lv_obj_set_style_bg_opa(w.button,LV_OPA_COVER,LV_PART_MAIN);
    lv_obj_set_style_bg_color(w.button,lv_color_hex(0xFF0000),LV_PART_MAIN|LV_STATE_PRESSED);
  }
  PageBasicTheme theme;theme.sensor_font=&lv_font_montserrat_14;Actions<Slots> actions;
  {
    auto callbacks=actions.callbacks();callbacks.action=nullptr;
    PageBasicLvglPort<Slots>missing(arena,state,widgets,{limits.slot_count,limits.grid_columns},theme,callbacks);
    assert(missing.initialize());assert(!missing.prepare(fixture.store,Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots>port(arena,state,widgets,{limits.slot_count,limits.grid_columns},theme,actions.callbacks());
    assert(port.initialize());
    std::array<lv_obj_t *,Slots*4> labels{};size_t index=0;
    for(const auto &w:widgets)for(auto *label:{w.icon,w.label,w.value,w.unit})labels[index++]=label;
    assert(cache.initialize(labels.data(),labels.size()));
    PageRendererRuntime<Slots>renderer(limits,arena,port);actions.renderer=&renderer;
    assert(renderer.prepare(fixture.store));assert(state.prepare(fixture.store));
    renderer.activate_prepared(fixture.store);state.activate_prepared(fixture.store);
    subscriptions.send("binary_sensor.action","","on");subscriptions.send("binary_sensor.presence","","detected");
    subscriptions.send("binary_sensor.window","","open");subscriptions.send("sensor.power","","20.25");subscriptions.send("sensor.message","","pre_wash");
    renderer.update_visible();lv_obj_update_layout(page);
    assert(!lv_obj_has_flag(widgets[2].button,LV_OBJ_FLAG_CLICKABLE));
    assert(!lv_obj_has_flag(widgets[3].button,LV_OBJ_FLAG_CLICKABLE));
    assert(std::strcmp(lv_label_get_text(widgets[2].label),"Presence")==0&&lv_obj_has_state(widgets[2].button,LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[3].label),"Window")==0&&lv_obj_has_state(widgets[3].button,LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[4].label),"Test Local")==0);
    assert(std::strcmp(lv_label_get_text(widgets[5].label),"Screensaver")==0);
    click(widgets[1].button);assert(actions.commands==1&&renderer.valid(actions.last));
    size_t draws=0;lv_obj_add_event_cb(widgets[1].button,[](lv_event_t *e){++*static_cast<size_t *>(lv_event_get_user_data(e));},LV_EVENT_DRAW_MAIN,&draws);
    lv_refr_now(lv_display_get_default());assert(draws>0);
    const auto animations=lv_anim_count_running();lv_obj_add_state(widgets[1].button,LV_STATE_PRESSED);
    assert(lv_anim_count_running()>animations);
    const auto child=fixture.store.card(0)->target;
    assert(renderer.open_page(child));renderer.update_visible();
    assert(lv_anim_count_running()==animations);
    lv_tick_inc(500);lv_timer_handler();
    assert(lv_color_eq(lv_obj_get_style_bg_color(widgets[1].button,LV_PART_MAIN),lv_color_hex(theme.active)));
    assert(std::strcmp(lv_label_get_text(widgets[1].value),"20.25")==0);
    assert(std::strlen(lv_label_get_text(widgets[1].unit))==600);
    assert(std::strcmp(lv_label_get_text(widgets[2].label),"Pre Wash")==0);
    lv_obj_send_event(widgets[1].button,LV_EVENT_CLICKED,nullptr);assert(actions.commands==1);
    click(widgets[1].button);assert(actions.commands==2);
    lv_obj_send_event(widgets[1].button,LV_EVENT_PRESSED,nullptr);assert(renderer.back());renderer.update_visible();
    lv_obj_send_event(widgets[1].button,LV_EVENT_CLICKED,nullptr);assert(actions.commands==2);
    assert(renderer.open_page(child));renderer.update_visible();assert(renderer.back());renderer.update_visible();lv_obj_update_layout(page);
    const auto before=lv_allocations,cpp_before=cpp_allocations,reserved=arena.allocations,retained=lv_live_bytes;
    size_t scratch=0,style_scratch=0;const auto descriptors=cache.memory().descriptor_adds;
    for(size_t n=0;n<1000;++n) {
      const auto update_allocations=lv_allocations;
      assert(renderer.open_page(child));renderer.update_visible();assert(renderer.back());renderer.update_visible();
      assert(lv_allocations-update_allocations<=2*Slots);
      style_scratch+=lv_allocations-update_allocations;
      assert(cpp_allocations==cpp_before&&arena.allocations==reserved);
      assert(cache.memory().descriptor_adds==descriptors&&cache.memory().retained_callbacks==labels.size());
      const auto layout=lv_allocations;lv_obj_update_layout(page);scratch+=lv_allocations-layout;
      assert(lv_live_bytes==retained);
    }
    assert(lv_allocations==before+scratch+style_scratch);assert(port.memory().text_truncations==0);
    std::printf("%s: status/actions, 1000 cycles, 0 C++/arena/label-descriptor allocations, %zu style-state and %zu grid scratch allocations\n",limits.device_profile,style_scratch,scratch);
  }
  state.retire_active();assert(arena.live==0);lv_obj_delete(page);
}

void modal_gesture_cycles() {
  Arena arena;PageModalToken current{17,1,0,ControlModalKind::CLIMATE};size_t clicks=0,changes=0;
  const auto valid=[](void *owner,PageModalToken token){const auto &now=*static_cast<PageModalToken *>(owner);return token.configuration==now.configuration&&token.opening==now.opening&&token.card==now.card&&token.kind==now.kind;};
  auto *root=lv_obj_create(lv_screen_active());
  {
    PageModalGestures guards(arena,3,current,&current,valid);assert(guards.available());const auto allocations=arena.allocations;
    for(size_t cycle=0;cycle<1000;++cycle) {
      auto *button=lv_button_create(root);
      // Install the real action first; the preprocessing guard must still win.
      lv_obj_add_event_cb(button,[](lv_event_t *e){++*static_cast<size_t *>(lv_event_get_user_data(e));},LV_EVENT_CLICKED,&clicks);
      lv_obj_add_event_cb(button,[](lv_event_t *e){++*static_cast<size_t *>(lv_event_get_user_data(e));},LV_EVENT_VALUE_CHANGED,&changes);
      assert(guards.attach(root));assert(guards.attach(root));const auto before=clicks;
      lv_obj_send_event(button,LV_EVENT_CLICKED,nullptr);assert(clicks==before);
      click(button);assert(clicks==before+1);
      lv_obj_send_event(button,LV_EVENT_PRESSED,nullptr);lv_obj_send_event(button,LV_EVENT_PRESS_LOST,nullptr);lv_obj_send_event(button,LV_EVENT_CLICKED,nullptr);assert(clicks==before+1);
      lv_obj_send_event(button,LV_EVENT_PRESSED,nullptr);lv_obj_send_event(button,LV_EVENT_VALUE_CHANGED,nullptr);assert(changes==cycle+1);
      ++current.opening;lv_obj_send_event(button,LV_EVENT_CLICKED,nullptr);assert(clicks==before+1);--current.opening;
      lv_obj_delete(button);assert(guards.live()<=1&&arena.allocations==allocations);
    }
    guards.invalidate();auto *button=lv_button_create(root);assert(!guards.attach(root));lv_obj_delete(button);
    lv_obj_delete(root);assert(guards.live()==0);
  }
  assert(arena.live==0);
  root=lv_obj_create(lv_screen_active());lv_button_create(root);
  {
    PageModalGestures small(arena,1,current,&current,valid);assert(!small.attach(root));
    lv_obj_delete(root);assert(small.live()==0);
  }
  assert(arena.live==0);
  arena.fail=true;PageModalGestures failed(arena,10,current,&current,valid);assert(!failed.available());arena.fail=false;
}
}
int main(){lv_init();auto *display=lv_display_create(480,480);assert(display);static std::array<uint16_t,480*32>draw_buffer{};lv_display_set_buffers(display,draw_buffer.data(),nullptr,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,[](lv_display_t *d,const lv_area_t *,uint8_t *){lv_display_flush_ready(d);});for(const auto &limits:DEVICE_UI_PROFILES){switch(limits.slot_count){case 6:profile<6>(limits);status_action_profile<6>(limits);break;case 9:profile<9>(limits);status_action_profile<9>(limits);break;case 15:profile<15>(limits);status_action_profile<15>(limits);break;case 20:profile<20>(limits);status_action_profile<20>(limits);break;default:std::abort();}}modal_gesture_cycles();lv_display_delete(display);lv_deinit();assert(lv_live_bytes==0);}
