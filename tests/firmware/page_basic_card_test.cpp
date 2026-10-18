#include <array>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

#include "bounded_display_text.h"
#include "button_grid_subpage_parser.h"
#include "page_basic_card.h"
#include "page_card_dependencies.h"
#include "weather_condition.h"
#include "panel_config_document.h"

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
  for(const auto &text:{std::string("light.test;;;;;;;;confirm_off"),std::string("media_player.test;;;;control_modal;;media;;")}) {
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
  for(const auto &pair:{std::pair<const char *,int>{"closed",100},{"closing",100},{"open",0},{"opening",0},{"unknown",0}}){transport.send("cover.room","",pair.first);value=read_basic_card(definition,{17,0},state);assert(value.closed_percent==pair.second);assert(value.on==(std::strcmp(pair.first,"open")==0||std::strcmp(pair.first,"opening")==0));}
  const char packed[]={'4','2','.','5','9'};transport.send("cover.room","current_position",{packed,4});value=read_basic_card(definition,{17,0},state);assert(value.on&&value.closed_percent==57);
  for(const auto invalid:{"unavailable","unknown","nan","inf","bad",""}){transport.send("cover.room","current_position",invalid);value=read_basic_card(definition,{17,0},state);assert(value.closed_percent==57&&value.on);}
  // Valid history is kept even while the page is hidden and no presenter runs.
  transport.send("cover.room","current_position","71");transport.send("cover.room","current_position","nan");value=read_basic_card(definition,{17,0},state);assert(value.closed_percent==29);
  for(const auto &pair:{std::pair<const char *,int>{"0",100},{"100",0},{"-1e30",100},{"1e30",0},{"25prefix",75}}){transport.send("cover.room","current_position",pair.first);value=read_basic_card(definition,{17,0},state);assert(value.closed_percent==pair.second);}
  const auto before=allocations;for(int i=0;i<1000;++i){value=read_basic_card(definition,{17,0},state);assert(value.closed_percent==75);}assert(allocations==before);
  ++transport.generation;value=read_basic_card(definition,{17,0},state);assert(value.closed_percent==0&&!value.on);transport.send("cover.room","","closed");value=read_basic_card(definition,{17,0},state);assert(value.closed_percent==100);
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
  struct Transport:PageMediaTransport {Subscriptions &subs;explicit Transport(Subscriptions &s):subs(s){}bool connected()const override{return true;}uint32_t connection_generation()const override{return subs.generation;}void reannounce()override{}} transport(subs);
  PageMediaStateRuntime media(arena,transport);PageEntityStateRuntime state(dependencies,arena,subs,{32,32,1024},&media);
  assert(state.prepare(play.store));state.activate_prepared(play.store);
  assert(read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).label=="Unknown");
  for(const auto &entry:std::array<std::pair<const char *,const char *>,7>{{{"playing","Playing"},{"paused","Paused"},{"idle","Idle"},{"off","Off"},{"unknown","Unavailable"},{"unavailable","Unavailable"},{" PLAYING ",""}}}) {
    subs.send("media_player.test","",entry.first);const auto value=read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media);
    assert(value.on==(std::strcmp(entry.first,"playing")==0));
    if(entry.second[0])assert(value.label==entry.second);else assert(value.formatted_label&&std::strcmp(value.text,"Playing")==0);
  }
  subs.send("media_player.test","","playing");const auto before=allocations;
  for(int i=0;i<1000;++i)assert(read_basic_card(definition,{17,0},state,2,"°C",nullptr,&media).on);assert(allocations==before);
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
void bounded_html_metadata() {
  char text[129];
  const std::array<std::pair<const char *,const char *>,7> cases={{{"A &amp; B","A & B"},{"&#128; &#x97;",u8"€ —"},{"&quot;x&apos;","\"x'"},{"&lt;&gt;&nbsp;",u8"<>\u00A0"},{"&#0; &#xD800; &bogus;","&#0; &#xD800; &bogus;"},{u8"Ṣade ṣ",u8"Ṣade ṣ"},{"&amp;amp;","&amp;"}}};
  const auto before=allocations;
  for(int i=0;i<1000;++i)for(const auto &entry:cases)assert(decode_html_entities_bounded(text,sizeof(text),entry.first)==entry.second);
  assert(decode_html_entities_bounded(text,5,"abc&amp;")=="abc&");
  assert(decode_html_entities_bounded(text,1,"abc").empty()&&text[0]=='\0');
  assert(decode_html_entities_bounded(nullptr,0,"abc").empty());
  assert(allocations==before);
}
int main(){display_text();bounded_html_metadata();for(const auto &limits:DEVICE_UI_PROFILES){profile(limits);climate_profile(limits);status_actions_profile(limits);control_profile(limits);weather_profile(limits);media_action_profile(limits);}}
