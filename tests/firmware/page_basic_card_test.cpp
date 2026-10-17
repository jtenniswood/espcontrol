#include <array>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

#include "bounded_display_text.h"
#include "page_basic_card.h"
#include "page_card_dependencies.h"
#include "panel_config_document.h"

using namespace espcontrol;using namespace espcontrol::pages;using namespace espcontrol::configuration;
static size_t allocations=0;
void *operator new(size_t bytes){++allocations;if(void *p=std::malloc(bytes?bytes:1))return p;std::abort();}
void *operator new[](size_t bytes){return ::operator new(bytes);}
void operator delete(void *p)noexcept{std::free(p);}
void operator delete[](void *p)noexcept{std::free(p);}
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
  ++subs.generation;assert(!read_basic_card(definition,{17,0},state).numeric);
  for(const auto &text:{std::string("light.test;;;;;;;;confirm_off"),std::string(";;;;sensor.temp;;sensor;text;"),std::string(";;;;sensor.temp;;sensor;time;"),std::string("media_player.test;;;;control;;media;;"),std::string(";;;;indicator;;subpage;;")}) {
    Fixture unsupported(limits,text);assert(!prepare_basic_card(unsupported.store,0,definition));
  }
  Fixture toggle(limits,"light.test;;Lightbulb;Lightbulb;sensor.temp;%;;2;");assert(prepare_basic_card(toggle.store,0,definition));assert(definition.kind==BasicCardKind::TOGGLE);
  assert(state.prepare(toggle.store));state.activate_prepared(toggle.store);subs.send("light.test",""," ON ");subs.send("sensor.temp","","45.25");
  value=read_basic_card(definition,{17,0},state);assert(value.on&&value.numeric&&std::strcmp(value.number,"45.25")==0);
  assert(!read_basic_card(definition,{18,0},state).numeric);
}
}
int main(){display_text();for(const auto &limits:DEVICE_UI_PROFILES)profile(limits);}
