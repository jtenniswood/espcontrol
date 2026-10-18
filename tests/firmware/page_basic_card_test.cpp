#include "page_basic_card.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "bounded_display_text.h"
#include "button_grid_subpage_parser.h"
#include "light_temperature_color.h"
#include "page_card_dependencies.h"
#include "panel_config_document.h"
#include "weather_condition.h"

using namespace espcontrol;using namespace espcontrol::pages;using namespace espcontrol::configuration;
static size_t allocations=0;
void *operator new(size_t bytes){++allocations;if(void *p=std::malloc(bytes?bytes:1))return p;std::abort();}
void *operator new[](size_t bytes){return ::operator new(bytes);}
void operator delete(void *p)noexcept{std::free(p);}
void operator delete[](void *p)noexcept{std::free(p);}
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
namespace {
struct Arena:PageArenaAllocator {
  void *allocate(size_t bytes)override{return std::malloc(bytes);}
  void release(void *p,size_t)override{std::free(p);}
};
struct Subscriptions:PageEntitySubscriptions {
  struct Binding{const char *entity,*attribute;void *owner;uint32_t revision,channel;Deliver deliver;};std::vector<Binding>bindings;
  bool subscribe(const char *entity,const char *attribute,void *owner,uint32_t revision,uint32_t channel,Deliver deliver)override{bindings.push_back({entity,attribute,owner,revision,channel,deliver});return true;}
  void release(void *owner)override{bindings.erase(std::remove_if(bindings.begin(),bindings.end(),[owner](const auto &b){return b.owner==owner;}),bindings.end());}
  uint32_t connection_generation()const override{return generation;}
  void send(const char *entity,const char *attribute,std::string_view value){for(auto &b:bindings)if(std::strcmp(b.entity,entity)==0&&std::strcmp(b.attribute,attribute)==0)b.deliver(b.owner,b.revision,b.channel,value);}
  uint32_t generation{1};
};
struct TextPool:BasicCardTextSink {
  bool copy(std::string_view value,std::string_view &stored)override{if(value.size()>text.size()-used)return false;std::memcpy(text.data()+used,value.data(),value.size());stored={text.data()+used,value.size()};used+=value.size();return true;}
  std::array<char,8192>text{};size_t used{0};
};
struct Fixture {
  Fixture(const DeviceUiLimits &limits,const std::string &text) {
    PanelConfigWriter writer(document.data(),document.size());assert(writer.begin()==PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),std::strlen(limits.device_profile))==PanelConfigStatus::OK);
    assert(writer.append_button(1,reinterpret_cast<const uint8_t *>(text.data()),text.size())==PanelConfigStatus::OK);
    assert(writer.finish(&size)==PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(),size,limits,17,arena.data(),arena.size(),&store)==PageStoreStatus::OK);
  }
  std::array<uint8_t,8192>document{};size_t size{0};alignas(std::max_align_t)std::array<uint8_t,32768>arena{};PageStore store;
};
void display_text() {
  char buffer[32];const auto before=allocations;
  assert(copy_display_text(buffer,sizeof(buffer),u8"Ṣade ṣ").bytes==6);assert(std::strcmp(buffer,"Sade s")==0);
  assert(copy_display_text(buffer,4,u8"ABé").truncated);assert(std::strcmp(buffer,"AB")==0);
  assert(!copy_display_text(buffer,5,u8"ABé").truncated);assert(std::strcmp(buffer,u8"ABé")==0);
  const char bounded[]={'A',char(0xC3),char(0xA9),'B','X'};
  assert(!copy_display_text(buffer,sizeof(buffer),{bounded,4}).invalid);assert(std::strcmp(buffer,u8"AéB")==0);
  assert(copy_display_text(buffer,sizeof(buffer),{bounded,4},2).truncated);assert(std::strcmp(buffer,"A")==0);
  assert(copy_display_text(buffer,sizeof(buffer),"\xED\xA0\x80\xF4\x90\x80\x80").invalid);assert(buffer[0]=='\0');
  assert(copy_display_text(buffer,sizeof(buffer),std::string_view("A\0B",3)).invalid);assert(std::strcmp(buffer,"AB")==0);
  assert(copy_display_text(buffer,1,"abc").truncated&&buffer[0]=='\0');
  assert(copy_display_text(nullptr,0,"a").truncated);assert(!copy_display_text(nullptr,0,{}).truncated);
  assert(allocations==before);
}
void profile(const DeviceUiLimits &limits) {
  Fixture sensor(limits,";;Gauge;Gauge;sensor.temp; °C ;sensor;1;active_color,card_off_color=336699,card_active_color=custom,card_on_color=FF8C00");
  BasicCardDefinition definition;assert(prepare_basic_card(sensor.store,0,definition));assert(definition.kind==BasicCardKind::NUMBER);
  assert(definition.unit==u8"°C"&&definition.precision==1&&definition.active_color);assert(definition.custom_off&&definition.custom_on);
  PageCardDependencies dependencies;Arena arena;Subscriptions subs;PageEntityStateRuntime state(dependencies,arena,subs,{32,32,2048});
  assert(state.prepare(sensor.store));state.activate_prepared(sensor.store);
  // Exact-length cache input: the byte following 21.4 is a digit and must not
  // accidentally be read by strtof from a non-NUL-terminated view.
  const char number[]={'2','1','.','4','9'};subs.send("sensor.temp","",{number,4});subs.send("sensor.temp","friendly_name","Room temp");
  const auto before=allocations;for(size_t i=0;i<1000;++i){const auto value=read_basic_card(definition,{17,0},state);assert(value.numeric&&value.on&&value.label=="Room temp");assert(std::strcmp(value.number,"21.4")==0);assert(value.unit==u8"°C");}assert(allocations==before);
  subs.send("sensor.temp","","unavailable");auto value=read_basic_card(definition,{17,0},state);assert(!value.numeric&&!value.on&&value.unit.empty());
  for(const auto number_text:{"nan","inf","-inf","garbage",""}){subs.send("sensor.temp","",number_text);assert(!read_basic_card(definition,{17,0},state).numeric);}
  subs.send("sensor.temp","","3.4028235e38");value=read_basic_card(definition,{17,0},state);assert(value.numeric&&std::strlen(value.number)<sizeof(value.number));
  subs.send("sensor.temp","",std::string(257,'1'));assert(!read_basic_card(definition,{17,0},state).numeric);
  for(const auto text:{"-21.45","0.05","1.25","2.75"}){subs.send("sensor.temp","",text);value=read_basic_card(definition,{17,0},state);assert(value.numeric);}
  auto invalid_precision=definition;invalid_precision.precision=UINT8_MAX;
  subs.send("sensor.temp","","1.2345");
  assert(std::strcmp(read_basic_card(invalid_precision,{17,0},state).number,"1.235")==0);
  ++subs.generation;assert(!read_basic_card(definition,{17,0},state).numeric);
  for (const auto& text :
       {std::string("light.test;;;;;;;;confirm_off"), std::string("alarm.test;;;;;;alarm_control;;")}) {
    Fixture unsupported(limits,text);assert(!prepare_basic_card(unsupported.store,0,definition));
  }
  Fixture toggle(limits,"light.test;;Lightbulb;Lightbulb;sensor.temp;%;;2;");assert(prepare_basic_card(toggle.store,0,definition));assert(definition.kind==BasicCardKind::TOGGLE);
  assert(state.prepare(toggle.store));state.activate_prepared(toggle.store);subs.send("light.test",""," ON ");subs.send("sensor.temp","","45.25");
  value=read_basic_card(definition,{17,0},state);assert(value.on&&value.numeric&&std::strcmp(value.number,"45.25")==0);
  assert(!read_basic_card(definition,{18,0},state).numeric);
  Fixture text(limits,";;;;sensor.temp;;sensor;text;state_labels,state_input=high,state_output=Already%20High,state_input_2=low,state_output_2=all%20LOW");
  assert(!prepare_basic_card(text.store,0,definition)); // Mapping storage must be reserved.
  TextPool strings;assert(prepare_basic_card(text.store,0,definition,&strings));assert(definition.kind==BasicCardKind::TEXT&&definition.text_input=="high");
  assert(state.prepare(text.store));state.activate_prepared(text.store);
  subs.send("sensor.temp",""," HIGH ");value=read_basic_card(definition,{17,0},state);assert(value.label=="Already High"&&!value.formatted_label);
  subs.send("sensor.temp","","low");assert(read_basic_card(definition,{17,0},state).label=="all LOW");
  subs.send("sensor.temp","",u8"ÜBERMORGEN");value=read_basic_card(definition,{17,0},state);assert(value.formatted_label&&std::strcmp(value.text,u8"Übermorgen")==0);
  subs.send("sensor.temp","","unavailable");value=read_basic_card(definition,{17,0},state);assert(!value.on&&std::strcmp(value.text,"Unavailable")==0);
  const auto text_allocations=allocations;for(size_t i=0;i<1000;++i)assert(read_basic_card(definition,{17,0},state).formatted_label);assert(allocations==text_allocations);
  ++subs.generation;value=read_basic_card(definition,{17,0},state);assert(!value.formatted_label&&value.label=="--");
  Fixture overlay(limits,"light.test;Standby;Auto;Auto;sensor.temp;;;text;");assert(prepare_basic_card(overlay.store,0,definition));assert(definition.text_overlay);
  assert(state.prepare(overlay.store));state.activate_prepared(overlay.store);subs.send("light.test","","on");subs.send("sensor.temp","","pre_wash");
  value=read_basic_card(definition,{17,0},state);assert(value.formatted_label&&std::strcmp(value.text,"Pre Wash")==0);
  subs.send("light.test","","off");value=read_basic_card(definition,{17,0},state);assert(!value.formatted_label&&value.label=="Standby");
  Fixture duration(limits,";;;;sensor.temp;;sensor;time;");assert(prepare_basic_card(duration.store,0,definition));assert(definition.kind==BasicCardKind::DURATION);
  assert(state.prepare(duration.store));state.activate_prepared(duration.store);subs.send("sensor.temp","","90");assert(!read_basic_card(definition,{17,0},state).numeric);
  subs.send("sensor.temp","unit_of_measurement","seconds");value=read_basic_card(definition,{17,0},state,2);assert(value.numeric&&std::strcmp(value.number,"1m 30s")==0&&value.unit.empty());
  assert(std::strcmp(read_basic_card(definition,{17,0},state,1).number,"1m")==0);
  subs.send("sensor.temp","unit_of_measurement","hours");subs.send("sensor.temp","","28");assert(std::strcmp(read_basic_card(definition,{17,0},state,2).number,"1d 4h")==0);
  for(const auto bad:{"nan","inf","-1","1x","","   "}){subs.send("sensor.temp","",bad);assert(!read_basic_card(definition,{17,0},state).numeric);}
  const char embedded[]={'9','0','\0','x'};subs.send("sensor.temp","",{embedded,4});assert(!read_basic_card(definition,{17,0},state).numeric);
  Fixture manual(limits,";;;;sensor.temp;;sensor;time;time_unit=hours");TextPool units;assert(prepare_basic_card(manual.store,0,definition,&units));assert(definition.time_unit=="hours");
  assert(state.prepare(manual.store));state.activate_prepared(manual.store);assert(state.memory().active_channels==2);
  subs.send("sensor.temp","","28");value=read_basic_card(definition,{17,0},state,2);assert(value.numeric&&std::strcmp(value.number,"1d 4h")==0);
  const auto duration_allocations=allocations;for(size_t i=0;i<1000;++i)assert(read_basic_card(definition,{17,0},state).numeric);assert(allocations==duration_allocations);

  Fixture folder_number(limits,";Room;;;sensor.temp;°C;subpage;1;");assert(prepare_basic_card(folder_number.store,0,definition));assert(definition.folder_mode==BasicFolderMode::NUMBER);
  assert(state.prepare(folder_number.store));state.activate_prepared(folder_number.store);subs.send("sensor.temp","","21.4");value=read_basic_card(definition,{17,0},state);assert(value.numeric&&!value.on&&std::strcmp(value.number,"21.4")==0);
  Fixture folder_text(limits,";Room;;;sensor.temp;;subpage;text;");assert(prepare_basic_card(folder_text.store,0,definition));assert(definition.folder_mode==BasicFolderMode::TEXT);
  assert(state.prepare(folder_text.store));state.activate_prepared(folder_text.store);subs.send("sensor.temp","","pre_wash");value=read_basic_card(definition,{17,0},state);assert(value.formatted_label&&!value.on&&std::strcmp(value.text,"Pre Wash")==0);
  Fixture folder_entity(limits,"light.room;Room;;;indicator;;subpage;;");assert(prepare_basic_card(folder_entity.store,0,definition));assert(definition.folder_mode==BasicFolderMode::ENTITY);
  assert(state.prepare(folder_entity.store));state.activate_prepared(folder_entity.store);subs.send("light.room","","on");assert(read_basic_card(definition,{17,0},state).on);
  subs.send("light.room","","unavailable");assert(!read_basic_card(definition,{17,0},state).on);
  Fixture folder_climate(limits,"climate.room;Room;;;indicator;;subpage;;subpage_kind=climate");assert(prepare_basic_card(folder_climate.store,0,definition));assert(definition.folder_mode==BasicFolderMode::CLIMATE);
  assert(state.prepare(folder_climate.store));state.activate_prepared(folder_climate.store);subs.send("climate.room","","heat");assert(!read_basic_card(definition,{17,0},state).on);
  for(const auto working:{"heating","cooling","drying","fan"}){subs.send("climate.room","hvac_action",working);assert(read_basic_card(definition,{17,0},state).on);}
  for(const auto inactive:{"idle","off","unknown","unavailable",""}){subs.send("climate.room","hvac_action",inactive);assert(!read_basic_card(definition,{17,0},state).on);}
  subs.send("climate.room","hvac_action","heating");subs.send("climate.room","","off");assert(!read_basic_card(definition,{17,0},state).on);
  Fixture folder_mower(limits,"lawn_mower.garden;Garden;;;indicator;;subpage;;subpage_kind=lawn_mower");assert(prepare_basic_card(folder_mower.store,0,definition));assert(definition.folder_mode==BasicFolderMode::MOWER);
  assert(state.prepare(folder_mower.store));state.activate_prepared(folder_mower.store);subs.send("lawn_mower.garden","","returning");assert(read_basic_card(definition,{17,0},state).on);
  subs.send("lawn_mower.garden","","docked");assert(!read_basic_card(definition,{17,0},state).on);
  Fixture children(limits,";Room;;;indicator;;subpage;;");assert(prepare_basic_card(children.store,0,definition));assert(definition.folder_mode==BasicFolderMode::CHILDREN);
  assert(basic_folder_child_indicator(toggle.store,0)=="light.test");assert(basic_folder_child_indicator(sensor.store,0).empty());

}
void climate_profile(const DeviceUiLimits &limits) {
  Arena arena;Subscriptions subs;PageCardDependencies dependencies;
  PageEntityStateRuntime state(dependencies,arena,subs,{64,64,8192});
  const auto translated=[](const char *key)->const char * { return std::strcmp(key,"Cooling")==0?u8"Kühlen":key; };
  Fixture fixture(limits,"climate.room;Den;Thermostat;Thermostat;;;climate;0;label_display=status,number_display=target,temperature_step=0.5");
  BasicCardDefinition definition;assert(prepare_basic_card(fixture.store,0,definition));assert(definition.kind==BasicCardKind::CLIMATE);
  assert(state.prepare(fixture.store));state.activate_prepared(fixture.store);
  auto value=read_basic_card(definition,{17,0},state,2,u8"°F",translated);
  assert(!value.on&&!value.climate_icon_on&&value.numeric&&std::strcmp(value.number,"--")==0&&std::strcmp(value.text,"Unavailable")==0);
  subs.send("climate.room","","<HVACMode.COOL: 'cool'>");subs.send("climate.room","hvac_action"," COOLING ");
  subs.send("climate.room","supported_features","426");subs.send("climate.room","target_temp_low","65");subs.send("climate.room","target_temp_high","75");subs.send("climate.room","current_temperature","70.2");
  value=read_basic_card(definition,{17,0},state,2,u8"°F",translated);
  assert(value.on&&value.climate_icon_on&&value.numeric&&std::strcmp(value.number,"65.0-75.0")==0&&value.unit==u8"°F"&&std::strcmp(value.text,u8"Kühlen")==0);
  subs.send("climate.room","hvac_action","idle");value=read_basic_card(definition,{17,0},state);assert(!value.on&&value.climate_icon_on&&std::strcmp(value.text,"Idle")==0);
  subs.send("climate.room","hvac_action","unknown");value=read_basic_card(definition,{17,0},state);assert(value.on&&value.climate_icon_on&&std::strcmp(value.text,"Cool")==0);
  subs.send("climate.room","","off");value=read_basic_card(definition,{17,0},state);assert(!value.on&&!value.climate_icon_on&&std::strcmp(value.text,"Off")==0);
  subs.send("climate.room","","heat/cool");subs.send("climate.room","supported_features","0");value=read_basic_card(definition,{17,0},state);assert(std::strcmp(value.number,"--")==0&&value.unit.empty()&&std::strcmp(value.text,"Heat/Cool")==0);
  subs.send("climate.room","supported_features","427");subs.send("climate.room","temperature","68.5");value=read_basic_card(definition,{17,0},state);assert(std::strcmp(value.number,"68.5")==0);
  subs.send("climate.room","temperature","nan");value=read_basic_card(definition,{17,0},state);assert(std::strcmp(value.number,"65.0-75.0")==0);
  subs.send("climate.room","target_temp_high","unavailable");value=read_basic_card(definition,{17,0},state);assert(std::strcmp(value.number,"--")==0);
  subs.send("climate.room","target_temp_high","75");
  const auto allocations_before=allocations;
  for(size_t i=0;i<10000;++i) {
    subs.send("climate.room","current_temperature",i%2?"70.2":"71.3");
    value=read_basic_card(definition,{17,0},state);assert(std::strcmp(value.number,"65.0-75.0")==0);
  }
  assert(allocations==allocations_before);
  for(const auto bad:{"inf","-inf","1e30","-1e30","garbage",""}) {
    subs.send("climate.room","target_temp_low",bad);value=read_basic_card(definition,{17,0},state);assert(std::strcmp(value.number,"--")==0);
  }
  ++subs.generation;value=read_basic_card(definition,{17,0},state);assert(std::strcmp(value.text,"Unavailable")==0&&!value.on);
  for(const auto type:{"climate","climate_control"})for(const auto label:{"label","actual","target","status"})for(const auto number:{"actual","target","icon"}) {
    Fixture next(limits,std::string("climate.room;")+std::string(600,'L')+";Auto;Auto;;;"+type+";2;label_display="+label+",number_display="+number);
    assert(prepare_basic_card(next.store,0,definition));assert(state.prepare(next.store));state.activate_prepared(next.store);
    subs.send("climate.room","","heat");subs.send("climate.room","hvac_action","heating");subs.send("climate.room","current_temperature","-2.35");subs.send("climate.room","temperature","20.15");
    value=read_basic_card(definition,{17,0},state,2,u8"°C");
    assert(value.on&&value.climate_icon_on&&value.numeric==(std::strcmp(number,"icon")!=0));
    if(std::strcmp(number,"actual")==0)assert(std::strcmp(value.number,"-2.40")==0);
    if(std::strcmp(number,"target")==0)assert(std::strcmp(value.number,"20.20")==0);
    if(std::strcmp(label,"label")==0)assert(value.label.size()==600&&!value.formatted_label);
    if(std::strcmp(label,"actual")==0)assert(std::strcmp(value.text,u8"-2.40°C")==0);
    if(std::strcmp(label,"target")==0)assert(std::strcmp(value.text,u8"20.20°C")==0);
    if(std::strcmp(label,"status")==0)assert(std::strcmp(value.text,"Heating")==0);
  }
}

void status_actions_profile(const DeviceUiLimits &limits) {
  Arena arena;Subscriptions subs;PageCardDependencies dependencies;
  PageEntityStateRuntime state(dependencies,arena,subs,{32,32,4096});
  BasicCardDefinition definition;
  for(const auto &kind:{std::string("presence"),std::string("door_window")}) {
    Fixture status(limits,";;Auto;Auto;binary_sensor.room;;"+kind+";window;active_color");
    assert(prepare_basic_card(status.store,0,definition));assert(definition.kind==BasicCardKind::STATUS);
    assert(state.prepare(status.store));state.activate_prepared(status.store);
    subs.send("binary_sensor.room","friendly_name","");
    auto value=read_basic_card(definition,{17,0},state);
    assert(value.label==(kind=="presence"?"Presence":"Window"));
    for(const auto text:{" ON ","off","detected","Detected ","home","OPEN","opening","unlocked","jammed","true","1","0","unknown","unavailable","", "2"}) {
      const auto ref=esphome::StringRef(text);
      const bool expected=!ha_state_unavailable_ref(ref)&&(kind=="presence"?presence_detected_ref(ref):is_entity_on_ref(ref));
      subs.send("binary_sensor.room","",text);
      const auto before=allocations;
      for(size_t i=0;i<100;++i){value=read_basic_card(definition,{17,0},state);assert(value.on==expected&&!value.numeric);}
      assert(allocations==before);
    }
    subs.send("binary_sensor.room","friendly_name","Front window");
    assert(read_basic_card(definition,{17,0},state).label=="Front window");
  }
  for(const auto mode:{"icon","text","0","1","2",""}) {
    const std::string settings=std::string("state_entity=sensor.power,state_precision=")+mode;
    Fixture action(limits,"script.scene;;Auto;Lightbulb;script.turn_on;;action;;"+settings);
    TextPool strings;assert(prepare_basic_card(action.store,0,definition,&strings));
    assert(definition.kind==BasicCardKind::ACTION&&definition.active_color);
    assert(state.prepare(action.store));state.activate_prepared(action.store);
    for(const auto text:{" ON ","off","playing","true","1","0","-3","21.25","unknown","unavailable","nan","inf",""}) {
      subs.send("sensor.power","",text);
      const auto ref=esphome::StringRef(text);
      const bool numeric=mode[0]>='0'&&mode[0]<='2';
      const bool expected=!ha_entity_state_unavailable_ref("sensor.power",ref)&&
        (numeric?numeric_state_positive_ref(ref):is_entity_on_ref(ref));
      const auto expected_text=text_sensor_display_text(ref,HA_STATE_TEXT_MAX_LEN);
      const auto before=allocations;
      for(size_t i=0;i<100;++i) {
        const auto value=read_basic_card(definition,{17,0},state);
        assert(value.on==expected);
        if(std::strcmp(mode,"text")==0)assert(value.formatted_label&&value.text==expected_text);
        else assert(value.label=="script.scene");
      }
      assert(allocations==before);
    }
  }
  const std::string long_unit(600,'W');
  Fixture unit(limits,"script.scene;;Auto;Auto;script.turn_on;;action;;state_entity=sensor.power,state_precision=2,state_unit="+long_unit);
  TextPool strings;assert(prepare_basic_card(unit.store,0,definition,&strings));
  assert(state.prepare(unit.store));state.activate_prepared(unit.store);subs.send("sensor.power","","12.345");
  auto value=read_basic_card(definition,{17,0},state);assert(value.numeric&&value.on&&value.unit==long_unit&&std::strcmp(value.number,"12.35")==0);
  for(const auto &entry:std::array<std::pair<std::string,std::string>,4>{{
    {"ble_keyboard_action;;;;local;;action;;","Ble Keyboard Action"},
    {";;;;;;push;;","Push"},
    {";Ignored;Auto;Auto;;;screensaver;;","Screensaver"},
    {"https://example.test/hook;Send;Auto;Auto;;;webhook;;","Send"}}}) {
    Fixture fixture(limits,entry.first);TextPool text;
    assert(prepare_basic_card(fixture.store,0,definition,&text));
    assert(state.prepare(fixture.store));state.activate_prepared(fixture.store);
    value=read_basic_card(definition,{17,0},state);assert(value.label==entry.second&&!value.on&&!value.numeric);
    assert(state.memory().active_channels==0);
  }
  Fixture confirmation(limits,"script.scene;;;;script.turn_on;;action;;confirm_on");
  assert(!prepare_basic_card(confirmation.store,0,definition,&strings));
}

void control_profile(const DeviceUiLimits &limits) {
  PageCardDependencies source;Arena arena;Subscriptions transport;PageEntityStateRuntime state(source,arena,transport,{32,32,2048});BasicCardDefinition definition;
  Fixture light(limits,"light.room;;Auto;Auto;;;light_control;0;");assert(prepare_basic_card(light.store,0,definition));assert(definition.kind==BasicCardKind::LIGHT&&definition.icon=="Lightbulb Outline"&&definition.icon_on=="Lightbulb");assert(state.prepare(light.store));state.activate_prepared(light.store);transport.send("light.room","","on");transport.send("light.room","friendly_name","Room lamp");auto value=read_basic_card(definition,{17,0},state);assert(value.on&&value.label=="Room lamp");transport.send("light.room","","unavailable");assert(!read_basic_card(definition,{17,0},state).on);
  Fixture cover(limits,"cover.room;;Auto;Auto;modal;;cover;0;");assert(prepare_basic_card(cover.store,0,definition));assert(definition.kind==BasicCardKind::COVER&&!definition.active_color&&definition.icon=="Blinds"&&definition.icon_on=="Blinds Open");assert(state.prepare(cover.store));state.activate_prepared(cover.store);
  for(const auto &pair:{std::pair<const char *,int>{"closed",100},{"closing",100},{"open",0},{"opening",0},{"unknown",0}}){transport.send("cover.room","",pair.first);value=read_basic_card(definition,{17,0},state);assert(value.fill_percent==pair.second);assert(value.on==(std::strcmp(pair.first,"open")==0||std::strcmp(pair.first,"opening")==0));}
  const char packed[]={'4','2','.','5','9'};transport.send("cover.room","current_position",{packed,4});value=read_basic_card(definition,{17,0},state);assert(value.on&&value.fill_percent==57);
  for(const auto invalid:{"unavailable","unknown","nan","inf","bad",""}){transport.send("cover.room","current_position",invalid);value=read_basic_card(definition,{17,0},state);assert(value.fill_percent==57&&value.on);}
  // Valid history is kept even while the page is hidden and no presenter runs.
  transport.send("cover.room","current_position","71");transport.send("cover.room","current_position","nan");value=read_basic_card(definition,{17,0},state);assert(value.fill_percent==29);
  for(const auto &pair:{std::pair<const char *,int>{"0",100},{"100",0},{"-1e30",100},{"1e30",0},{"25prefix",75}}){transport.send("cover.room","current_position",pair.first);value=read_basic_card(definition,{17,0},state);assert(value.fill_percent==pair.second);}
  const auto before=allocations;for(int i=0;i<1000;++i){value=read_basic_card(definition,{17,0},state);assert(value.fill_percent==75);}assert(allocations==before);
  ++transport.generation;value=read_basic_card(definition,{17,0},state);assert(value.fill_percent==0&&!value.on);transport.send("cover.room","","closed");value=read_basic_card(definition,{17,0},state);assert(value.fill_percent==100);
}

void local_relay_profile(const DeviceUiLimits& limits) {
  struct Provider : PageLocalRelayProvider {
    PageLocalRelaySnapshot read(std::string_view key) const override {
      return {on, key == "relay_one" ? std::string_view("Hall Relay") : std::string_view{}};
    }
    bool on{false};
  } provider;
  Arena arena;
  PageLocalRelayRuntime relays(arena, provider);
  assert(!relays.install(false) && relays.install(true));
  PageCardDependencies dependencies;
  Subscriptions transport;
  PageEntityStateRuntime ha(dependencies, arena, transport, {32, 32, 1024});
  for (bool push : {false, true}) {
    Fixture fixture(limits,
                    std::string("relay_one;;Auto;Auto;") + (push ? "push" : "switch") + ";;internal;;");
    BasicCardDefinition definition;
    TextPool strings;
    assert(prepare_basic_card(fixture.store, 0, definition, &strings, &relays));
    assert(definition.kind == BasicCardKind::LOCAL_RELAY && definition.push_feedback == push &&
           definition.active_color != push && definition.label == "Hall Relay" && definition.unit.empty());
    assert(definition.icon == (push ? "Gesture Tap" : "Power Plug") && definition.icon_on == definition.icon);
    assert(basic_folder_child_indicator(fixture.store, 0) ==
           (push ? std::string_view{} : std::string_view("relay_one")));
    assert(ha.prepare(fixture.store) && relays.prepare(fixture.store));
    ha.activate_prepared(fixture.store);
    relays.activate_prepared(fixture.store);
    assert(ha.memory().active_channels == 0 && transport.bindings.empty());
    for (bool on : {false, true}) {
      provider.on = on;
      page_local_relay_feed().notify("relay_one", on);
      const auto value =
          read_basic_card(definition, {17, 0}, ha, 2, "°C", nullptr, nullptr, nullptr, &relays);
      assert(value.label == "Hall Relay" && value.on == (on && !push) && !value.numeric);
    }
    const auto before = allocations;
    for (int i = 0; i < 1000; ++i)
      read_basic_card(definition, {17, 0}, ha, 2, "°C", nullptr, nullptr, nullptr, &relays);
    assert(allocations == before);
    Fixture custom(limits, std::string("relay_one;Custom;Home;Lightbulb;") + (push ? "push" : "switch") +
                               ";;internal;;");
    assert(prepare_basic_card(custom.store, 0, definition, nullptr, &relays) &&
           definition.label == "Custom" && definition.icon == "Home" &&
           definition.icon_on == (push ? "Home" : "Lightbulb"));
  }
  Fixture unknown(limits, "kitchen_relay_3;;Auto;Auto;switch;;internal;;");
  BasicCardDefinition definition;
  TextPool strings;
  assert(prepare_basic_card(unknown.store, 0, definition, &strings, &relays) &&
         definition.label == "Kitchen Relay 3");
  Fixture empty(limits, ";;Auto;Auto;push;;internal;;");
  assert(prepare_basic_card(empty.store, 0, definition, nullptr, &relays));
  auto translate = [](const char* key) { return std::strcmp(key, "Relay") == 0 ? "Translated Relay" : key; };
  assert(read_basic_card(definition, {17, 0}, ha, 2, "°C", translate, nullptr, nullptr, &relays).label ==
         "Translated Relay");
}

void local_sensor_profile(const DeviceUiLimits& limits) {
  struct Provider : PageLocalSensorProvider {
    size_t capacity() const override {
      return 2;
    }
    uint32_t resolve(std::string_view key, bool text) const override {
      return key == "physical" ? (text ? 1 : 0) : NO_LOCAL_SENSOR;
    }
    bool watch(uint32_t) override {
      return true;
    }
    PageLocalSensorSample read(uint32_t source) const override {
      return {revision, number, source ? text : std::string_view{}};
    }
    float number{21.25f};
    std::string_view text;
    uint64_t revision{1};
  } provider;
  Fixture numeric(limits, "physical;Temperature;;;local;°C;sensor;2;"),
      text(limits, "physical;Ignored;Home;Auto;local;ignored;sensor;text;");
  BasicCardDefinition definition;
  assert(prepare_basic_card(numeric.store, 0, definition));
  assert(definition.kind == BasicCardKind::LOCAL_NUMBER && !definition.large_numbers &&
         !definition.active_color && basic_folder_child_indicator(numeric.store, 0).empty());
  PageCardDependencies dependencies;
  Arena arena;
  Subscriptions transport;
  PageEntityStateRuntime ha(dependencies, arena, transport, {32, 32, 1024});
  PageLocalSensorRuntime local(arena, provider, {32, 8, 1024});
  assert(ha.prepare(numeric.store) && local.prepare(numeric.store));
  ha.activate_prepared(numeric.store);
  local.activate_prepared(numeric.store);
  assert(ha.memory().active_channels == 0 && transport.bindings.empty());
  const ConfigurationHandle handle{17, 0};
  for (int precision : {0, 1, 2}) {
    definition.precision = precision;
    for (float number : {-4.25f, 21.25f, 0.0f, -0.0f, 1.005f, 9999999.0f, std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()}) {
      provider.number = number;
      ++provider.revision;
      local.loop();
      const auto value = read_basic_card(definition, handle, ha, 2, "°C", nullptr, nullptr, &local);
      char expected[32];
      if (!std::isfinite(number))
        std::strcpy(expected, "--");
      else
        std::snprintf(expected, sizeof(expected),
                      precision == 1   ? "%.1f"
                      : precision == 2 ? "%.2f"
                                       : "%.0f",
                      static_cast<double>(number));
      assert(value.numeric && !value.on && value.label == "Temperature" && value.unit == "°C" &&
             std::strcmp(value.number, expected) == 0);
    }
  }
  const auto before = allocations;
  for (int i = 0; i < 1000; ++i) read_basic_card(definition, handle, ha, 2, "°C", nullptr, nullptr, &local);
  assert(allocations == before);
  assert(prepare_basic_card(text.store, 0, definition) && definition.kind == BasicCardKind::LOCAL_TEXT &&
         definition.unit.empty());
  assert(ha.prepare(text.store) && local.prepare(text.store));
  ha.activate_prepared(text.store);
  local.activate_prepared(text.store);
  auto read = [&]() { return read_basic_card(definition, handle, ha, 2, "°C", nullptr, nullptr, &local); };
  assert(read().label == "--");
  for (std::string_view raw : {"Raw ÉTÉ", "unknown", "  Ready: [x]  ", ""}) {
    provider.text = raw;
    ++provider.revision;
    local.loop();
    const auto value = read();
    assert(value.label == raw && !value.numeric && !value.on && !value.formatted_label);
  }
  const char embedded[] = {'A', '\0', 'B'};
  provider.text = {embedded, 3};
  ++provider.revision;
  local.loop();
  assert(read().label == "A");
  const std::string oversized(local.text_capacity() + 1, 'x');
  provider.text = oversized;
  ++provider.revision;
  local.loop();
  assert(read().label == "--");
  ++transport.generation;
  assert(read().label == "--");
  provider.text = "Recovered";
  ++provider.revision;
  local.loop();
  assert(read().label == "Recovered");
  const auto text_allocations = allocations;
  for (int i = 0; i < 1000; ++i) assert(read().label == "Recovered");
  assert(allocations == text_allocations);
}

void option_select_profile(const DeviceUiLimits& limits) {
  for (const char* wire : {"select.room;;Auto;Auto;;;option_select;;",
                           "input_select.room;;Auto;Auto;select.select_option;;action;;"}) {
    Fixture fixture(limits, wire);
    BasicCardDefinition definition;
    assert(prepare_basic_card(fixture.store, 0, definition));
    assert(definition.kind == BasicCardKind::OPTION_SELECT && definition.push_feedback &&
           !definition.active_color && !definition.large_numbers && definition.sensor.empty() &&
           definition.unit.empty());
    assert(basic_folder_child_indicator(fixture.store, 0).empty());
    PageCardDependencies source;
    Arena arena;
    Subscriptions transport;
    PageEntityStateRuntime state(source, arena, transport, {32, 32, 4096});
    assert(state.prepare(fixture.store));
    state.activate_prepared(fixture.store);
    const ConfigurationHandle handle{17, 0};
    const std::string entity(definition.entity);
    auto value = read_basic_card(definition, handle, state);
    assert(value.numeric && !value.on && value.label == entity && std::strcmp(value.text, "--") == 0);
    transport.send(entity.c_str(), "friendly_name", "Room Mode");
    for (const char* raw : {"Night MODE", "garage", u8"Été", "  Raw punctuation: [x]  "}) {
      transport.send(entity.c_str(), "", raw);
      value = read_basic_card(definition, handle, state);
      assert(value.label == "Room Mode" && std::strcmp(value.text, raw) == 0 && !value.on &&
             !value.formatted_label && value.numeric);
    }
    const std::string long_value(HA_STATE_TEXT_MAX_LEN + 30, 'x');
    transport.send(entity.c_str(), "", long_value);
    assert(std::strlen(read_basic_card(definition, handle, state).text) == HA_STATE_TEXT_MAX_LEN);
    for (const char* raw : {"unknown", " UNKNOWN ", "unavailable", "", "  "}) {
      transport.send(entity.c_str(), "", raw);
      assert(std::strcmp(read_basic_card(definition, handle, state).text, "--") == 0);
    }
    const auto before = allocations;
    for (int i = 0; i < 1000; ++i) assert(read_basic_card(definition, handle, state).numeric);
    assert(allocations == before);
    ++transport.generation;
    value = read_basic_card(definition, handle, state);
    assert(value.label == entity && std::strcmp(value.text, "--") == 0);
  }
  Fixture custom(limits, "select.room;Custom;Unused;Unused;;;option_select;;");
  BasicCardDefinition definition;
  assert(prepare_basic_card(custom.store, 0, definition) && definition.label == "Custom");
  Fixture empty(limits, ";;;;;;option_select;;");
  assert(prepare_basic_card(empty.store, 0, definition));
  PageCardDependencies source;
  Arena arena;
  Subscriptions transport;
  PageEntityStateRuntime state(source, arena, transport, {32, 32, 1024});
  assert(state.prepare(empty.store));
  state.activate_prepared(empty.store);
  auto translated = [](const char* key) {
    return std::strcmp(key, "Option") == 0 ? "Translated Option" : key;
  };
  assert(read_basic_card(definition, {17, 0}, state, 2, "°C", translated).label == "Translated Option");
}

void fan_control_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, "fan.room;;Auto;Auto;;;fan_control;;fan_light_entity=light.fan");
  BasicCardDefinition definition;
  assert(prepare_basic_card(fixture.store, 0, definition));
  assert(definition.kind == BasicCardKind::FAN_CONTROL && definition.active_color &&
         definition.push_feedback && definition.icon == "Fan" && definition.icon_on == "Fan" &&
         definition.default_label == std::string_view("Fan") && definition.unit.empty());
  assert(basic_folder_child_indicator(fixture.store, 0) == "fan.room");
  PageCardDependencies source;
  Arena arena;
  Subscriptions transport;
  PageEntityStateRuntime state(source, arena, transport, {32, 32, 2048});
  assert(state.prepare(fixture.store));
  state.activate_prepared(fixture.store);
  const ConfigurationHandle handle{17, 0};
  assert(read_basic_card(definition, handle, state).label == "Fan");
  auto translated = [](const char* key) { return std::strcmp(key, "Fan") == 0 ? "Translated Fan" : key; };
  assert(read_basic_card(definition, handle, state, 2, "°C", translated).label == "Translated Fan");
  transport.send("fan.room", "friendly_name", "Room Fan");
  for (const auto& item : {std::pair<const char*, bool>{"on", true},
                           {"ON", true},
                           {" off ", false},
                           {"unknown", false},
                           {"unavailable", false},
                           {"", false}}) {
    transport.send("fan.room", "", item.first);
    const auto value = read_basic_card(definition, handle, state);
    assert(value.on == item.second && value.label == "Room Fan" && !value.numeric);
    assert(read_basic_indicator(handle, "fan.room", state) == item.second);
  }
  transport.send("fan.room", "", "on");
  const auto before = allocations;
  for (int n = 0; n < 1000; ++n) assert(read_basic_card(definition, handle, state).on);
  assert(allocations == before);
  ++transport.generation;
  assert(!read_basic_card(definition, handle, state).on);
  Fixture custom(limits, "fan.room;Custom;Fan Off;Fan;ignored;ignored;fan_control;2;number_display=actual");
  assert(prepare_basic_card(custom.store, 0, definition));
  assert(definition.label == "Custom" && definition.icon == "Fan Off" && definition.icon_on == "Fan Off");
  assert(state.prepare(custom.store));
  state.activate_prepared(custom.store);
  transport.send("fan.room", "friendly_name", "Ignored name");
  auto value = read_basic_card(definition, handle, state);
  assert(value.label == "Custom" && !value.numeric && value.unit.empty());
}

uint32_t timer_tick = 100;
int64_t timer_epoch = 0;
uint32_t timer_now() {
  return timer_tick;
}
int64_t timer_epoch_now() {
  return timer_epoch;
}
void timer_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, "timer.room;;Unused;Unused;confirm;3;timer;;");
  BasicCardDefinition definition;
  assert(prepare_basic_card(fixture.store, 0, definition));
  assert(definition.kind == BasicCardKind::TIMER && definition.active_color && !definition.large_numbers);
  assert(definition.unit.empty() && definition.icon.empty() &&
         basic_folder_child_indicator(fixture.store, 0) == "timer.room");
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageTimerRuntime timer(arena, subscriptions, timer_now, timer_epoch_now);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {64, 64, 2048}, &timer);
  assert(timer.prepare(fixture.store) && state.prepare(fixture.store));
  timer.activate_prepared(fixture.store);
  state.activate_prepared(fixture.store);
  const auto read = [&]() {
    return read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, nullptr, nullptr, nullptr, nullptr,
                           nullptr, nullptr, nullptr, &timer);
  };
  auto value = read();
  assert(value.label == "Timer" && value.numeric && std::strcmp(value.number, "--:--") == 0);
  subscriptions.send("timer.room", "friendly_name", "Kitchen timer");
  subscriptions.send("timer.room", "duration", "0:05:00");
  subscriptions.send("timer.room", "remaining", "0:01:29");
  subscriptions.send("timer.room", "", "paused");
  value = read();
  assert(value.label == "Kitchen timer" && !value.on && std::strcmp(value.number, "1:29") == 0);
  subscriptions.send("timer.room", "", "active");
  const auto first = read().timer.confirmation_revision;
  assert(read().on && read_basic_indicator({17, 0}, "timer.room", state, BasicFolderMode::TIMER));
  assert(!read_basic_indicator({17, 0}, "timer.room",
                               state));  // Explicit folder entities keep their existing predicate.
  timer_tick += 1000;
  timer.loop();
  assert(std::strcmp(read().number, "1:28") == 0);
  subscriptions.send("timer.room", "", "idle");
  subscriptions.send("timer.room", "", "active");
  assert(read().timer.confirmation_revision !=
         first);  // A state cycle invalidates an old confirmation without a paint.
  const auto before = allocations;
  for (int i = 0; i < 1000; ++i) (void)read();
  assert(allocations == before);
  ++subscriptions.generation;
  timer.loop();
  assert(read().timer.mode == PageTimerMode::UNAVAILABLE);
  {
    Arena small_arena;
    Subscriptions small_subscriptions;
    PageTimerRuntime small_timer(small_arena, small_subscriptions, timer_now, timer_epoch_now);
    PageEntityStateRuntime small_state(dependencies, small_arena, small_subscriptions, {64, 64, 1},
                                       &small_timer);
    assert(small_timer.prepare(fixture.store) && small_state.prepare(fixture.store));
    small_timer.activate_prepared(fixture.store);
    small_state.activate_prepared(fixture.store);
    small_subscriptions.send("timer.room", "", "active");
    assert(read_basic_timer(definition, {17, 0}, small_state, &small_timer).mode ==
           PageTimerMode::UNAVAILABLE);
  }
}
void light_temperature_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, "light.room;;Auto;Other;;5000-6000;light_temperature;color;");
  BasicCardDefinition definition;
  assert(prepare_basic_card(fixture.store, 0, definition));
  assert(definition.kind == BasicCardKind::LIGHT_TEMPERATURE && definition.icon == "Lightbulb" &&
         definition.icon_on == definition.icon && !definition.large_numbers && !definition.active_color &&
         definition.unit.empty());
  assert(basic_folder_child_indicator(fixture.store,0)=="light.room");
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageLightTemperatureRuntime temperature(arena, subscriptions);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {64, 64, 2048}, &temperature);
  assert(temperature.prepare(fixture.store) && state.prepare(fixture.store));
  temperature.activate_prepared(fixture.store);
  state.activate_prepared(fixture.store);
  const auto read = [&]() {
    return read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, nullptr, nullptr, nullptr, nullptr,
                           nullptr, nullptr, &temperature);
  };
  subscriptions.send("light.room", "friendly_name", "Raw light");
  subscriptions.send("light.room", "color_temp_kelvin", "5500");
  assert(read().fill_percent == 0 && read().label == "Raw light");
  subscriptions.send("light.room", "", "on");
  assert(read().fill_percent == 50);
  assert(temperature.begin({17, 0}) && temperature.drag({17, 0}, 80));
  auto value = read();
  assert(value.formatted_label && !value.numeric && std::strcmp(value.text, "5800K") == 0);
  subscriptions.send("light.room", "color_temp_kelvin", "5494");
  value = read();
  assert(value.fill_percent == 49 && std::strcmp(value.text, "5800K") == 0);
  int target;
  assert(temperature.finish({17, 0}, 49, target) && target == 5490);
  assert(!read().formatted_label && read().label == "Raw light");
  subscriptions.send("light.room", "", "off");
  subscriptions.send("light.room", "color_temp_kelvin", "5600");
  assert(read().fill_percent == 0);
  subscriptions.send("light.room", "", "on");
  assert(read().fill_percent == 60);
  const auto before = allocations;
  for (int n = 0; n < 1000; ++n) (void)read();
  assert(allocations == before);
  ++subscriptions.generation;
  temperature.loop();
  assert(!read().light_temperature.available);
  {
    Arena small_arena;
    Subscriptions small_subscriptions;
    PageLightTemperatureRuntime small_temperature(small_arena, small_subscriptions);
    PageEntityStateRuntime small_state(dependencies, small_arena, small_subscriptions, {64, 64, 1},
                                       &small_temperature);
    assert(small_temperature.prepare(fixture.store) && small_state.prepare(fixture.store));
    small_temperature.activate_prepared(fixture.store);
    small_state.activate_prepared(fixture.store);
    small_subscriptions.send("light.room", "", "on");
    small_subscriptions.send("light.room", "color_temp_kelvin", "5500");
    const auto exhausted = read_basic_light_temperature(definition, {17, 0}, small_state, &small_temperature);
    assert(!exhausted.available && !exhausted.gesture && !exhausted.dragging);
  }
}
void temperature_colors() {
  assert(light_temperature::fill_color(1000) == 0xFF8012);
  assert(light_temperature::fill_color(2000) == 0xFF8012);
  assert(light_temperature::fill_color(4250) == 0xDCA689);
  assert(light_temperature::fill_color(5500) == 0xC8BBCA);
  assert(light_temperature::fill_color(6500) == 0xB8CCFF);
  assert(light_temperature::fill_color(std::numeric_limits<int>::max()) == 0xB8CCFF);
  assert(light_temperature::fill_color(std::numeric_limits<int>::min()) == 0xFF8012);
}
void number_slider_profile(const DeviceUiLimits& limits) {
  for (const char* type : {"slider", "light_brightness", "fan_speed", "cover"}) {
    Fixture fixture(limits, std::string("number.room;;Auto;Other;tilt;ignored;") + type + ";2;");
    BasicCardDefinition definition;
    assert(prepare_basic_card(fixture.store, 0, definition) &&
           definition.kind == BasicCardKind::NUMBER_SLIDER);
    assert(!definition.active_color && !definition.large_numbers && definition.unit.empty() &&
           definition.icon_on == definition.icon);
    assert(basic_folder_child_indicator(fixture.store,0)=="number.room");
    Arena arena;
    Subscriptions subscriptions;
    PageCardDependencies dependencies;
    PageNumberSliderRuntime levels(arena, subscriptions);
    PageEntityStateRuntime state(dependencies, arena, subscriptions, {64, 64, 2048}, &levels);
    assert(levels.prepare(fixture.store) && state.prepare(fixture.store));
    levels.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    const auto read = [&]() {
      return read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, nullptr, nullptr, nullptr, nullptr,
                             nullptr, &levels);
    };
    assert(!read().numeric && !read().number_slider.ready);
    subscriptions.send("number.room", "friendly_name", "Raw room");
    subscriptions.send("number.room", "min", "-10");
    subscriptions.send("number.room", "max", "10");
    subscriptions.send("number.room", "step", "0.1");
    subscriptions.send("number.room", "", "5");
    subscriptions.send("number.room", "unit_of_measurement", "kWh");
    auto value = read();
    assert(value.label == "Raw room" && !value.numeric && value.fill_percent == 75);
    assert(levels.begin({17, 0}) && levels.drag({17, 0}, 25));
    value = read();
    assert(value.numeric && value.unit == "kWh" && std::strcmp(value.number, "-7.5") == 0);
    const auto before = allocations;
    for (int i = 0; i < 1000; ++i) (void)read();
    assert(allocations == before);
    for (const auto* missing : {"NoNe", " null ", " UNKNOWN ", "unavailable", ""}) {
      subscriptions.send("number.room", "unit_of_measurement", missing);
      assert(read().unit.empty());
    }
    subscriptions.send("number.room", "step", "0.1");
    assert(!read().numeric && read().number_slider.gesture);
    subscriptions.send("number.room", "step", "0.2");
    assert(!read().number_slider.gesture);
    ++subscriptions.generation;
    levels.loop();
    assert(!read().number_slider.available);
    {
      Arena small_arena;
      Subscriptions small_subscriptions;
      PageNumberSliderRuntime small_levels(small_arena, small_subscriptions);
      PageEntityStateRuntime small_state(dependencies, small_arena, small_subscriptions, {64, 64, 1},
                                         &small_levels);
      assert(small_levels.prepare(fixture.store) && small_state.prepare(fixture.store));
      small_levels.activate_prepared(fixture.store);
      small_state.activate_prepared(fixture.store);
      for (const auto& report : {std::pair{"min", "-10"}, {"max", "10"}, {"step", "0.1"}, {"", "5"}})
        small_subscriptions.send("number.room", report.first, report.second);
      const auto exhausted = read_basic_number_slider(definition, {17, 0}, small_state, &small_levels);
      assert(!exhausted.available && !exhausted.ready && !exhausted.gesture);
    }
  }
}
void percent_slider_profile(const DeviceUiLimits& limits) {
  struct Case {
    const char* entity;
    const char* type;
    const char* mode;
    const char* attribute;
    const char* value;
    int percent;
  };
  for (const auto& item : {Case{"light.room", "light_brightness", "", "brightness", "1", 1},
                           {"fan.room", "fan_speed", "", "percentage", "75.5", 76},
                           {"cover.room", "cover", "", "current_position", "25", 25},
                           {"cover.room", "cover", "tilt", "current_tilt_position", "75", 75},
                           {"light.room", "fan_speed", "", "brightness", "1", 1},
                           {"cover.room", "slider", "", "current_position", "25", 25}}) {
    Fixture fixture(limits, std::string(item.entity) + ";Configured;Auto;Auto;" + item.mode + ";ignored;" +
                                item.type + ";2;");
    BasicCardDefinition definition;
    assert(prepare_basic_card(fixture.store, 0, definition) &&
           definition.kind == BasicCardKind::PERCENT_SLIDER);
    assert(definition.unit.empty() && !definition.active_color && !definition.push_feedback &&
           !definition.large_numbers);
    assert(basic_folder_child_indicator(fixture.store, 0) == item.entity);
    Arena arena;
    Subscriptions subscriptions;
    PageCardDependencies dependencies;
    PagePercentSliderRuntime levels(arena, subscriptions);
    PageEntityStateRuntime state(dependencies, arena, subscriptions, {64, 64, 2048}, &levels);
    assert(levels.prepare(fixture.store) && state.prepare(fixture.store));
    levels.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    auto read = [&]() {
      return read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, nullptr, nullptr, nullptr, nullptr,
                             &levels);
    };
    assert(read().fill_percent == 0 && read().label == "Configured");
    subscriptions.send(item.entity, "", "on");
    assert(read().percent_slider.icon_on && !read().percent_slider.fill_known && read().fill_percent == 0);
    subscriptions.send(item.entity, item.attribute, item.value);
    auto value = read();
    const bool cover = std::string_view(item.entity).substr(0, 6) == "cover.";
    assert(value.percent_slider.percent == item.percent &&
           value.fill_percent == (cover ? 100 - item.percent : item.percent));
    assert(!value.on && !value.numeric && value.unit.empty() && value.label == "Configured");
    assert(levels.preview({17, 0}, 50));
    subscriptions.send(item.entity, item.attribute, "bad");
    assert(read().fill_percent == 50 && read().percent_slider.preview);
    subscriptions.send(item.entity, item.attribute, item.value);
    assert(!read().percent_slider.preview && read().percent_slider.percent == item.percent);
    ++subscriptions.generation;
    levels.loop();
    assert(!read().percent_slider.available && read().fill_percent == 0);
    const auto before = allocations;
    for (int n = 0; n < 1000; ++n) (void)read();
    assert(allocations == before);
    {
      Arena small_arena;
      Subscriptions small_subscriptions;
      PagePercentSliderRuntime small_levels(small_arena, small_subscriptions);
      PageEntityStateRuntime small_state(dependencies, small_arena, small_subscriptions, {64, 64, 1},
                                         &small_levels);
      assert(small_levels.prepare(fixture.store) && small_state.prepare(fixture.store));
      small_levels.activate_prepared(fixture.store);
      small_state.activate_prepared(fixture.store);
      small_subscriptions.send(item.entity, item.attribute, "75");
      const auto exhausted = read_basic_percent_slider(definition, {17, 0}, small_state, &small_levels);
      assert(!exhausted.available && !exhausted.fill_known && !exhausted.icon_on);
    }
  }
}
uint32_t fan_status_tick = 100;
uint32_t fan_status_now() {
  return fan_status_tick;
}
void fan_status_profile(const DeviceUiLimits& limits) {
  for (bool power : {true, false}) {
    fan_status_tick = 100;
    Fixture fixture(limits,
                    std::string("fan.room;;Auto;Auto;;;") + (power ? "fan_switch" : "fan_oscillate") + ";;");
    BasicCardDefinition definition;
    assert(prepare_basic_card(fixture.store, 0, definition));
    assert(definition.kind == (power ? BasicCardKind::FAN_SWITCH : BasicCardKind::FAN_OSCILLATE));
    assert(definition.icon == (power ? "Fan Off" : "Fan") && definition.icon_on == "Fan");
    assert(definition.active_color && definition.push_feedback == !power);
    assert(basic_folder_child_indicator(fixture.store, 0) == "fan.room");
    Arena arena;
    Subscriptions transport;
    PageCardDependencies dependencies;
    PageFanStatusRuntime fan(arena, transport, fan_status_now);
    PageEntityStateRuntime state(dependencies, arena, transport, {32, 32, 2048}, &fan);
    assert(fan.prepare(fixture.store) && state.prepare(fixture.store));
    fan.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    auto read = [&]() {
      return read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, nullptr, nullptr, nullptr, &fan);
    };
    assert(read().label == (power ? "Fan" : "Oscillation"));
    transport.send("fan.room", "friendly_name", "Bedroom");
    assert(read().label == "Bedroom");
    transport.send("fan.room", "", "on");
    transport.send("fan.room", "oscillating", "true");
    assert(read().on && read().label == (power ? "On" : "Oscillating"));
    fan_status_tick += 3000;
    fan.loop();
    assert(read().label == "Bedroom");
    transport.send("fan.room", "friendly_name", "");
    assert(read().label.empty());
    transport.send("fan.room", "oscillating", "unavailable");
    if (!power) assert(read().on && read().label == "Unsupported");
    transport.send("fan.room", "", "unavailable");
    assert(read().label == "Unavailable" && read().on == !power);
    ++transport.generation;
    fan.loop();
    assert(!read().on);
    const auto before = allocations;
    for (int n = 0; n < 1000; ++n) {
      fan.loop();
      (void)read();
    }
    assert(allocations == before);
    {
      Arena small_arena;
      Subscriptions small_transport;
      PageFanStatusRuntime small_fan(small_arena, small_transport, fan_status_now);
      PageEntityStateRuntime small_state(dependencies, small_arena, small_transport,
                                         {32, 32, power ? size_t(1) : size_t(2)}, &small_fan);
      assert(small_fan.prepare(fixture.store) && small_state.prepare(fixture.store));
      small_fan.activate_prepared(fixture.store);
      small_state.activate_prepared(fixture.store);
      small_transport.send("fan.room", "", "on");
      if (!power) small_transport.send("fan.room", "oscillating", "true");
      auto exhausted = read_basic_card(definition, {17, 0}, small_state, 2, "°C", nullptr, nullptr, nullptr,
                                       nullptr, &small_fan);
      assert(!exhausted.on && exhausted.label == (power ? "Unavailable" : "Unsupported"));
    }
  }
}

void fan_mode_profile(const DeviceUiLimits& limits) {
  for (bool direction : {true, false}) {
    Fixture fixture(limits, std::string("fan.room;Ignored;Auto;Auto;sensor.ignored;ignored;") +
                                (direction ? "fan_direction" : "fan_preset") + ";;");
    BasicCardDefinition definition;
    assert(prepare_basic_card(fixture.store, 0, definition));
    assert(definition.kind == (direction ? BasicCardKind::FAN_DIRECTION : BasicCardKind::FAN_PRESET));
    assert(definition.label.empty() && definition.unit.empty() && definition.active_color &&
           definition.push_feedback);
    assert(definition.icon == (direction ? "Swap Horizontal" : "Fan Auto") &&
           definition.icon_on == definition.icon);
    assert(basic_folder_child_indicator(fixture.store, 0) == "fan.room");
    PageCardDependencies source;
    Arena arena;
    Subscriptions transport;
    PageEntityStateRuntime state(source, arena, transport, {32, 32, 4096});
    assert(state.prepare(fixture.store));
    state.activate_prepared(fixture.store);
    const ConfigurationHandle handle{17, 0};
    auto read = [&]() { return read_basic_card(definition, handle, state); };
    assert(read().label == "Unavailable");
    transport.send("fan.room", "", "off");
    assert(read().label == "Unsupported");
    const char* attribute = direction ? "direction" : "preset_mode";
    transport.send("fan.room", "preset_modes", "['presetModes', 'direction']");
    transport.send("fan.room", attribute, direction ? " ReVeRsE " : " sleep_mode ");
    if (!direction) assert(read().label == "Unsupported");
    transport.send("fan.room", "preset_modes", "['sleep_mode', 'auto']");
    auto value = read();
    assert(value.on && value.formatted_label && !value.numeric);
    assert(std::strcmp(value.text, direction ? "Reverse" : "Sleep Mode") == 0);
    assert(!read_basic_indicator(handle, "fan.room", state));  // Folder follows primary power.
    transport.send("fan.room", "", "on");
    assert(read_basic_indicator(handle, "fan.room", state));
    transport.send("fan.room", "", "unavailable");
    assert(read().on && read().label == "Unavailable");
    transport.send("fan.room", "", "on");
    transport.send("fan.room", attribute, direction ? "FORWARD" : " OFF ");
    value = read();
    assert(!value.on);
    if (direction)
      assert(value.formatted_label && std::strcmp(value.text, "Forward") == 0);
    else
      assert(value.label == "Preset");
    transport.send("fan.room", attribute, direction ? "invalid" : "unknown");
    value = read();
    if (direction)
      assert(!value.on && value.label == "Unsupported");
    else
      assert(value.on && std::strcmp(value.text, "Unknown") == 0);
    transport.send("fan.room", attribute, direction ? "reverse" : "auto");
    auto translate = [](const char* key) { return std::strcmp(key, "Auto") == 0 ? "Automatique" : key; };
    value = read_basic_card(definition, handle, state, 2, "°C", translate);
    assert(std::strcmp(value.text, direction ? "Reverse" : "Automatique") == 0);
    const auto before = allocations;
    for (int i = 0; i < 1000; ++i) assert(read().on);
    assert(allocations == before);
    ++transport.generation;
    assert(!read().on && read().label == "Unavailable");
    Fixture custom_mode(limits, std::string("fan.room;Ignored;Custom Glyph;Fan;;;") +
                                    (direction ? "fan_direction" : "fan_preset") + ";;");
    assert(prepare_basic_card(custom_mode.store, 0, definition));
    assert(definition.icon == "Custom Glyph" && definition.icon_on == definition.icon &&
           definition.label.empty());
  }
}

void weather_profile(const DeviceUiLimits &limits) {
  Fixture fixture(limits,"weather.home;Ignored;Custom;Custom;;;weather;0;");BasicCardDefinition definition;assert(prepare_basic_card(fixture.store,0,definition));assert(definition.kind==BasicCardKind::WEATHER&&definition.label.empty()&&definition.unit.empty());PageCardDependencies source;Arena arena;Subscriptions transport;PageEntityStateRuntime state(source,arena,transport,{16,16,1024});assert(state.prepare(fixture.store));state.activate_prepared(fixture.store);
  auto value=read_basic_card(definition,{17,0},state);assert(value.label=="Unavailable");
  transport.send("weather.home","","sunny");value=read_basic_card(definition,{17,0},state);assert(value.label=="Sunny"&&espcontrol::weather::CONDITION_ROWS[value.weather_condition].icon==std::string_view("Weather Sunny"));
  transport.send("weather.home","","partly-cloudy-night");value=read_basic_card(definition,{17,0},state);assert(value.formatted_label&&std::strcmp(value.text,"Partly Cloudy Night")==0&&espcontrol::weather::CONDITION_ROWS[value.weather_condition].icon==std::string_view("Weather Night Cloudy"));
  transport.send("weather.home","","cloudy");auto translate=[](const char *key)->const char *{return std::strcmp(key,"Cloudy")==0?"Translated Cloudy":key;};value=read_basic_card(definition,{17,0},state,2,"°C",translate);assert(value.label=="Translated Cloudy");
  for(const auto raw:{"unknown","unavailable","custom__weather","mdi:weather-sunny"}){transport.send("weather.home","",raw);value=read_basic_card(definition,{17,0},state);assert(!value.numeric&&!value.on);}
  const auto before=allocations;for(size_t i=0;i<1000;++i)value=read_basic_card(definition,{17,0},state);assert(allocations==before);
  ++transport.generation;assert(read_basic_card(definition,{17,0},state).label=="Unavailable");
  Fixture empty(limits,";;;;;;weather;0;");assert(prepare_basic_card(empty.store,0,definition));assert(state.prepare(empty.store));state.activate_prepared(empty.store);value=read_basic_card(definition,{17,0},state);assert(value.label=="Cloudy");
  Fixture forecast(limits,"weather.home;;;;;;weather;daily;");
  if(card_runtime_weather_forecast_supported()){assert(prepare_basic_card(forecast.store,0,definition)&&definition.kind==BasicCardKind::FORECAST&&definition.forecast_view==weather::View::DAILY);}
  else {assert(prepare_basic_card(forecast.store,0,definition));assert(state.prepare(forecast.store));state.activate_prepared(forecast.store);transport.send("weather.home","","sunny");assert(read_basic_card(definition,{17,0},state).label=="Sunny");}
}

}

void media_action_profile(const DeviceUiLimits &limits) {
  Fixture play(limits,"media_player.test;Custom;Auto;Auto;play_pause;;media;state;");
  BasicCardDefinition definition;assert(prepare_basic_card(play.store,0,definition));
  assert(definition.kind==BasicCardKind::MEDIA_ACTION&&definition.media.show_state&&definition.icon=="Play Pause"&&definition.icon_on==definition.icon);
  PageCardDependencies dependencies;Arena arena;Subscriptions subs;
  struct Transport:PageMediaTransport {Subscriptions &subs;explicit Transport(Subscriptions &s):subs(s){}bool connected()const override{return true;}uint32_t connection_generation()const override{return subs.generation;}uint32_t now_ms()const override{return 100;}int64_t epoch_seconds()const override{return 0;}void reannounce()override{}} transport(subs);
  PageMediaStateRuntime media(arena,transport);PageEntityStateRuntime state(dependencies,arena,subs,{32,32,1024},&media);
  assert(state.prepare(play.store));state.activate_prepared(play.store);
  assert(read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).label=="Unknown");
  for (const auto& entry : std::array<std::pair<const char*, const char*>, 7>{{{"playing", "Playing"},
                                                                               {"paused", "Paused"},
                                                                               {"idle", "Idle"},
                                                                               {"off", "Off"},
                                                                               {"unknown", "Unknown"},
                                                                               {"unavailable", "Unavailable"},
                                                                               {" PLAYING ", ""}}}) {
    subs.send("media_player.test","",entry.first);const auto value=read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media);
    assert(value.on==(std::strcmp(entry.first,"playing")==0));
    if(entry.second[0])assert(value.label==entry.second);else assert(value.formatted_label&&std::strcmp(value.text,"Playing")==0);
  }
  subs.send("media_player.test","","playing");const auto before=allocations;
  for(int i=0;i<1000;++i)assert(read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).on);
  assert(allocations==before);
  ++subs.generation;assert(!read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).on&&read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).label=="Unknown");
  for(const auto mode:{"previous","next","play_pause"}) {
    Fixture action(limits,std::string("media_player.test;;;;")+mode+";;media;;");assert(prepare_basic_card(action.store,0,definition));
    assert(state.prepare(action.store));state.activate_prepared(action.store);subs.send("media_player.test","","playing");
    const auto value=read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media);assert(value.on==(std::strcmp(mode,"play_pause")==0));
    assert(value.label==(std::string(mode)=="previous"?"Previous":std::string(mode)=="next"?"Next":"Play/Pause"));
  }
  Fixture playlist(limits,"media_player.test;;;;playlist;;media;;playlist_content_id=spotify%3Aplaylist%3Aabcd,playlist_content_type=playlist");
  assert(!prepare_basic_card(playlist.store,0,definition));TextPool strings;assert(prepare_basic_card(playlist.store,0,definition,&strings));
  assert(definition.media.playlist_id=="spotify:playlist:abcd"&&definition.media.kind==MediaActionKind::PLAYLIST);
  assert(media.prepare(playlist.store)&&state.prepare(playlist.store));media.activate_prepared(playlist.store);state.activate_prepared(playlist.store);
  subs.send("media_player.test","","playing");subs.send("media_player.test","media_content_id","spotify:playlist:ABCD?version=2");
  subs.send("media_player.test","media_content_type","music");assert(read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).on);
  subs.send("media_player.test","media_content_type","video");assert(!read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).on);
  subs.send("media_player.test","media_content_type","unknown");assert(read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).on);
  subs.send("media_player.test","media_content_type","audio");subs.send("media_player.test","media_content_id","spotify:playlist:&#97;bcd");assert(read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).on);
  subs.send("media_player.test","","idle");subs.send("media_player.test","","playing");
  assert(!read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).on);
  // Identical metadata must become fresh even when the packed cache value did not change.
  subs.send("media_player.test","media_content_id","spotify:playlist:&#97;bcd");
  assert(read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).on);
  const auto playlist_before=allocations;for(int i=0;i<1000;++i)assert(read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).on);assert(allocations==playlist_before);
  subs.send("media_player.test","media_content_id","unknown");assert(!read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).on);
  subs.send("media_player.test","media_content_id",std::string(2048,'x'));assert(!read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).on);
  ++subs.generation;assert(!read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).on);
}
void media_position_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, "media_player.room;Custom;Auto;Auto;position;;media;state;");
  BasicCardDefinition definition;
  assert(prepare_basic_card(fixture.store, 0, definition));
  assert(definition.kind == BasicCardKind::MEDIA_POSITION &&
         definition.media.kind == MediaActionKind::POSITION && !definition.push_feedback &&
         !definition.large_numbers);
  PageCardDependencies dependencies;
  Arena arena;
  Subscriptions subs;
  struct Transport : PageMediaTransport {
    Subscriptions& subs;
    uint32_t now = 1000;
    explicit Transport(Subscriptions& s) : subs(s) {}
    bool connected() const override {
      return true;
    }
    uint32_t connection_generation() const override {
      return subs.generation;
    }
    uint32_t now_ms() const override {
      return now;
    }
    int64_t epoch_seconds() const override {
      return 1003;
    }
    void reannounce() override {
      assert(false);
    }
  } transport(subs);
  PageMediaStateRuntime media(arena, transport);
  PageEntityStateRuntime state(dependencies, arena, subs, {32, 32, 1024}, &media);
  assert(media.prepare(fixture.store) && state.prepare(fixture.store));
  media.activate_prepared(fixture.store);
  state.activate_prepared(fixture.store);
  subs.send("media_player.room", "", "playing");
  subs.send("media_player.room", "media_duration", "300");
  subs.send("media_player.room", "media_position", "10");
  subs.send("media_player.room", "media_position_updated_at", "1970-01-01T00:16:40Z");
  auto value = read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, &media);
  assert(value.numeric && value.fill_percent == 4 && std::strcmp(value.number, "0:13") == 0 &&
         value.label == "Playing");
  transport.now += 1000;
  media.loop();
  assert(std::strcmp(read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, &media).number, "0:14") ==
         0);
  subs.send("media_player.room", "", "paused");
  transport.now += 10000;
  media.loop();
  assert(std::strcmp(read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, &media).number, "0:14") ==
         0);
  const auto before = allocations;
  for (int i = 0; i < 1000; ++i)
    assert(read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, &media).numeric);
  assert(allocations == before);
  ++subs.generation;
  value = read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, &media);
  assert(std::strcmp(value.number, "0:00") == 0 && value.fill_percent == 0 && value.label == "Unknown");
}
void bounded_html_metadata() {
  char text[129];
  const std::array<std::pair<const char*, const char*>, 7> cases = {
      {{"A &amp; B", "A & B"},
       {"&#128; &#x97;", u8"€ —"},
       {"&quot;x&apos;", "\"x'"},
       {"&lt;&gt;&nbsp;", u8"<>\u00A0"},
       {"&#0; &#xD800; &bogus;", "&#0; &#xD800; &bogus;"},
       {u8"Ṣade ṣ", u8"Ṣade ṣ"},
       {"&amp;amp;", "&amp;"}}};
  const auto before = allocations;
  for (int i = 0; i < 1000; ++i)
    for (const auto& entry : cases)
      assert(decode_html_entities_bounded(text, sizeof(text), entry.first) == entry.second);
  assert(decode_html_entities_bounded(text, 5, "abc&amp;") == "abc&");
  assert(decode_html_entities_bounded(text, 1, "abc").empty() && text[0] == '\0');
  assert(decode_html_entities_bounded(nullptr, 0, "abc").empty());
  assert(allocations == before);
}
void now_playing_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, "media_player.room;Ignored;Auto;Auto;now_playing;;media;progress;");
  BasicCardDefinition definition;
  assert(prepare_basic_card(fixture.store, 0, definition));
  assert(definition.kind == BasicCardKind::MEDIA_NOW_PLAYING && definition.media.progress &&
         !definition.push_feedback && !definition.large_numbers);
  Fixture alias(limits, std::string("media_player.room;;;;now_playing;;media;;") + MEDIA_COVER_ART_OPTION);
  BasicCardDefinition rejected;
  assert(prepare_basic_card(alias.store, 0, rejected) && rejected.kind == BasicCardKind::MEDIA_COVER_ART);
  Fixture play_pause(limits, "media_player.room;;;;now_playing;;media;play_pause;");
  assert(prepare_basic_card(play_pause.store, 0, rejected) && rejected.media.play_pause_background &&
         rejected.push_feedback && rejected.active_color);
  PageCardDependencies dependencies;
  Arena arena;
  Subscriptions subscriptions;
  struct Transport : PageMediaTransport {
    Subscriptions& subscriptions;
    explicit Transport(Subscriptions& source) : subscriptions(source) {}
    bool connected() const override {
      return true;
    }
    uint32_t connection_generation() const override {
      return subscriptions.generation;
    }
    uint32_t now_ms() const override {
      return 100;
    }
    int64_t epoch_seconds() const override {
      return 1000;
    }
    void reannounce() override {}
  } transport(subscriptions);
  PageMediaStateRuntime media(arena, transport);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {32, 32, 4096}, &media);
  assert(media.prepare(fixture.store) && state.prepare(fixture.store));
  media.activate_prepared(fixture.store);
  state.activate_prepared(fixture.store);
  subscriptions.send("media_player.room", "", "playing");
  subscriptions.send("media_player.room", "media_content_id", "track:1");
  subscriptions.send("media_player.room", "media_title", "Beyonc&eacute;");
  subscriptions.send("media_player.room", "media_artist", "Artist");
  subscriptions.send("media_player.room", "media_duration", "300");
  subscriptions.send("media_player.room", "media_position", "60");
  auto value = read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, &media);
  assert(value.media_title == "Beyonc&eacute;" && value.media_artist == "Artist" &&
         value.fill_percent == 20 && value.on);
  const std::string long_metadata(HA_STATE_TEXT_MAX_LEN + 50, 'x');
  subscriptions.send("media_player.room", "media_title", long_metadata);
  subscriptions.send("media_player.room", "media_artist", long_metadata);
  value = read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, &media);
  assert(value.media_title == std::string_view(long_metadata).substr(0, HA_STATE_TEXT_MAX_LEN) &&
         value.media_artist == std::string_view(long_metadata).substr(0, HA_STATE_TEXT_MAX_LEN));
  subscriptions.send("media_player.room", "media_title", "Beyonc&eacute;");
  subscriptions.send("media_player.room", "media_artist", "Artist");
  // Match legacy metadata: truncate the raw HA value before HTML decoding.
  // A suffix beyond that bound must not change playlist selection.
  std::string encoded;
  for (size_t i = 0; i < 19; ++i) encoded += "&amp;";
  encoded += "a";
  assert(encoded.size() == HA_STATE_TEXT_MAX_LEN);
  const std::string playlist_id = std::string(19, '&') + "a";
  auto playlist = definition;
  playlist.kind = BasicCardKind::MEDIA_ACTION;
  playlist.media.kind = MediaActionKind::PLAYLIST;
  playlist.media.playlist_id = playlist_id;
  playlist.media.playlist_type = "playlist";
  subscriptions.send("media_player.room", "media_content_id", encoded + "ignored suffix");
  subscriptions.send("media_player.room", "media_content_type", "playlist");
  assert(read_basic_card(playlist, {17, 0}, state, 2, "°C", nullptr, &media).on);
  subscriptions.send("media_player.room", "media_content_id", "track:1");
  subscriptions.send("media_player.room", "media_title", "Beyonc&eacute;");
  subscriptions.send("media_player.room", "media_artist", "Artist");
  const auto count = allocations;
  for (size_t i = 0; i < 1000; ++i)
    assert(read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, &media).media_artist == "Artist");
  assert(allocations == count);
  subscriptions.send("media_player.room", "media_content_id", "track:2");
  value = read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, &media);
  assert(value.media_title.empty() && value.media_artist.empty());
  subscriptions.send("media_player.room", "media_title", "New title");
  subscriptions.send("media_player.room", "source", "TV");
  assert(read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, &media).media_external_input);
  subscriptions.send("media_player.room", "", "playing");
  subscriptions.send("media_player.room", "media_title", "New title");
  assert(!read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, &media).media_external_input);
  ++subscriptions.generation;
  value = read_basic_card(definition, {17, 0}, state, 2, "°C", nullptr, &media);
  assert(value.media_title.empty() && value.media_artist.empty() && !value.media_external_input &&
         value.fill_percent == 0);
}

void media_control_profile(const DeviceUiLimits& limits) {
  const auto exercise = [&](const char* config, bool status, bool active) {
    Fixture fixture(limits, config);
    BasicCardDefinition card;
    assert(prepare_basic_card(fixture.store, 0, card) && card.kind == BasicCardKind::MEDIA_CONTROL);
    assert(card.media.volume_number && card.media.volume_max == 60 && card.media.show_state == status);
    Arena arena;
    Subscriptions subscriptions;
    PageCardDependencies dependencies;
    PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 4096});
    assert(state.prepare(fixture.store));
    state.activate_prepared(fixture.store);
    auto value = read_basic_card(card, {17, 0}, state);
    assert(value.numeric && std::strcmp(value.number, "--") == 0 &&
           value.label == (status ? "Unknown" : "Custom"));
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "volume_level", "0.85");
    subscriptions.send("media_player.room", "supported_features", "4");
    value = read_basic_card(card, {17, 0}, state);
    assert(value.on == active && std::strcmp(value.number, "60") == 0 && value.media_volume.reported == 85);
    assert(value.label == (status ? "Playing" : "Custom"));
    for (const auto features : {"1024", "0"}) {
      subscriptions.send("media_player.room", "supported_features", features);
      value = read_basic_card(card, {17, 0}, state);
      assert(value.media_volume.known && std::strcmp(value.number, "85") == 0);
    }
    subscriptions.send("media_player.room", "supported_features", std::string(5000, '9'));
    value = read_basic_card(card, {17, 0}, state);
    assert(value.media_volume.mode == media::VolumeControlMode::READ_ONLY &&
           std::strcmp(value.number, "85") == 0);
    subscriptions.send("media_player.room", "supported_features", "unknown");
    value = read_basic_card(card, {17, 0}, state);
    assert(value.media_volume.mode == media::VolumeControlMode::ABSOLUTE &&
           std::strcmp(value.number, "60") == 0);
    for (const auto invalid : {"nan", "inf", "-inf", "unknown", "None", ""}) {
      subscriptions.send("media_player.room", "volume_level", invalid);
      assert(!read_basic_card(card, {17, 0}, state).media_volume.known);
    }
    for (const auto& entry :
         {std::pair<const char*, const char*>{"1e30", "60"}, {"-1e30", "0"}, {".345tail", "35"}}) {
      subscriptions.send("media_player.room", "volume_level", entry.first);
      assert(std::strcmp(read_basic_card(card, {17, 0}, state).number, entry.second) == 0);
    }
    subscriptions.send("media_player.room", "volume_level", ".5");
    for (const auto raw : {"unknown", "", "unavailable"}) {
      subscriptions.send("media_player.room", "", raw);
      value = read_basic_card(card, {17, 0}, state);
      assert(!value.on && value.media_volume.known && std::strcmp(value.number, "50") == 0);
      if (status) assert(value.label == (std::string_view(raw) == "unavailable" ? "Unavailable" : "Unknown"));
    }
    const auto before = allocations;
    for (size_t i = 0; i < 1000; ++i) assert(read_basic_card(card, {17, 0}, state).media_volume.known);
    assert(allocations == before);
    ++subscriptions.generation;
    assert(!read_basic_card(card, {17, 0}, state).media_volume.known);
  };
  exercise("media_player.room;Custom;Unused Glyph;Auto;volume;;media;;volume_max=60", false, false);
  exercise(
      "media_player.room;Custom;Unused Glyph;Auto;control_modal;;media;;number_display=volume,volume_max=60",
      true, true);
  exercise(
      "media_player.room;Custom;Unused "
      "Glyph;Auto;control_modal;;media;;label_display=label,number_display=volume,volume_max=60",
      false, true);
  Fixture icon(limits, "media_player.room;;;;control_modal;;media;;");
  BasicCardDefinition card;
  assert(prepare_basic_card(icon.store, 0, card) && !card.media.volume_number && card.media.show_state &&
         card.push_feedback);
  assert(std::strcmp(card.default_label, "Media Control") == 0 && card.icon == "Play Pause");
  // Both the existing reader and PageStore canonicalize an empty volume label
  // to Volume before dependency enumeration; it is not a live friendly name.
  Fixture unnamed(limits, "media_player.room;;;;volume;;media;;");
  assert(prepare_basic_card(unnamed.store, 0, card) && card.label == "Volume");
  assert(card.label == parse_cfg("media_player.room;;;;volume;;media;;").label);
}

void speaker_group_profile(const DeviceUiLimits& limits) {
  Fixture fixture(
      limits,
      "media_player.room;;Auto;Auto;speaker_group;;media;;number_display=volume,label_display=status");
  BasicCardDefinition card;
  assert(prepare_basic_card(fixture.store, 0, card));
  assert(card.kind == BasicCardKind::MEDIA_CONTROL && card.media.kind == MediaActionKind::SPEAKER_GROUP &&
         !card.media.volume_number && !card.media.show_state && card.push_feedback && card.active_color);
  assert(card.icon == "Speaker Multiple" && std::strcmp(card.default_label, "Speaker Group") == 0);
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {64, 64, 4096});
  assert(state.prepare(fixture.store));
  state.activate_prepared(fixture.store);
  auto value = read_basic_card(card, {17, 0}, state);
  assert(!value.on && !value.numeric && value.label == "Speaker Group");
  subscriptions.send("media_player.room", "", "paused");
  for (const auto raw :
       {"[]", "['media_player.room']", "['media_player.room','media_player.room']", "['switch.other']"}) {
    subscriptions.send("media_player.room", "group_members", raw);
    assert(!read_basic_card(card, {17, 0}, state).on);
  }
  subscriptions.send("media_player.room", "group_members", "['media_player.other']");
  assert(read_basic_card(card, {17, 0}, state).on);
  subscriptions.send("media_player.room", "", "off");
  assert(read_basic_card(card, {17, 0}, state).on);
  for (const auto raw : {"unavailable", "unknown", "", "  UNKNOWN  "}) {
    subscriptions.send("media_player.room", "", raw);
    assert(!read_basic_card(card, {17, 0}, state).on);
  }
  subscriptions.send("media_player.room", "", "paused");
  const auto before = allocations;
  for (int n = 0; n < 1000; ++n) assert(read_basic_card(card, {17, 0}, state).on);
  assert(allocations == before);
  subscriptions.send("media_player.room", "group_members", std::string(5000, 'x'));
  assert(!read_basic_card(card, {17, 0}, state).on);
  subscriptions.send("media_player.room", "group_members", "['media_player.other']");
  ++subscriptions.generation;
  assert(!read_basic_card(card, {17, 0}, state).on);
  Fixture custom(limits, "media_player.room;Family room;Speaker;Other;speaker_group;;media;;");
  assert(prepare_basic_card(custom.store, 0, card) && card.label == "Family room" && card.icon == "Speaker" &&
         card.icon_on == "Speaker");
}

int main() {
  display_text();
  temperature_colors();
  bounded_html_metadata();
  for (const auto& limits : DEVICE_UI_PROFILES) {
    profile(limits);
    climate_profile(limits);
    status_actions_profile(limits);
    control_profile(limits);
    local_relay_profile(limits);
    local_sensor_profile(limits);
    option_select_profile(limits);
    fan_control_profile(limits);
    fan_mode_profile(limits);
    fan_status_profile(limits);
    percent_slider_profile(limits);
    number_slider_profile(limits);
    light_temperature_profile(limits);
    timer_profile(limits);
    weather_profile(limits);
    media_action_profile(limits);
    media_position_profile(limits);
    now_playing_profile(limits);
    media_control_profile(limits);
    speaker_group_profile(limits);
  }
}
