#include <array>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "page_card_dependencies.h"
#include "button_grid_card_runtime.h"
#include "page_store_goldens.h"
#include "panel_config_document.h"

using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
namespace {
struct Document {
  Document(const DeviceUiLimits &limits) : limits(limits), writer(data.data(), data.size()) {
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),
        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    std::string order;
    for (size_t slot=1;slot<=limits.slot_count;++slot) {
      if (!order.empty()) order+=",";
      order+=std::to_string(slot);
    }
    const std::string key="button_order";
    assert(writer.append_setting(reinterpret_cast<const uint8_t *>(key.data()),key.size(),
        reinterpret_cast<const uint8_t *>(order.data()),order.size())==PanelConfigStatus::OK);
  }
  void button(uint8_t slot, const std::string &text) {
    assert(writer.append_button(slot,reinterpret_cast<const uint8_t *>(text.data()),text.size())==PanelConfigStatus::OK);
  }
  void child(uint8_t slot, const std::string &text) {
    assert(writer.append_subpage(slot,reinterpret_cast<const uint8_t *>(text.data()),text.size())==PanelConfigStatus::OK);
  }
  PageStore finish() {
    size_t bytes=0; assert(writer.finish(&bytes)==PanelConfigStatus::OK); PageStore store;
    assert(PageStore::stage_v1(data.data(),bytes,limits,1,arena.data(),arena.size(),&store)==PageStoreStatus::OK);
    return store;
  }
  const DeviceUiLimits &limits;
  std::array<uint8_t,65536> data{};
  alignas(std::max_align_t) std::array<uint8_t,262144> arena{};
  PanelConfigWriter writer;
};
struct Sink final : PageDependencySink {
  struct Entry { CardId card; std::string entity,attribute; };
  bool add(CardId card, std::string_view entity, std::string_view attribute) override {
    if (remaining==0) return false;
    if (remaining>0) --remaining;
    entries.push_back({card,std::string(entity),std::string(attribute)}); return true;
  }
  std::set<std::string> pairs(CardId card=0) const {
    std::set<std::string> result;
    for (const auto &entry:entries) if (entry.card==card) result.insert(entry.entity+"/"+entry.attribute);
    return result;
  }
  int remaining{-1};std::vector<Entry> entries;
};
std::set<std::string> declarations(const std::string &config,bool low_heap=false) {
  Document doc(DEVICE_UI_PROFILES[8]);doc.button(1,config);auto store=doc.finish();Sink sink;
  assert(PageCardDependencies({low_heap}).enumerate(store,sink));return sink.pairs();
}
void sensor_fields_and_modes() {
  assert(declarations("sensor.unused;Name;Auto;Auto;;;sensor;1;").empty());
  assert(declarations(";Name;Auto;Auto;sensor.temperature;°C;sensor;1;")==std::set<std::string>{"sensor.temperature/"});
  assert((declarations(";;Auto;Auto;sensor.temperature;°C;sensor;1;")==
      std::set<std::string>{"sensor.temperature/","sensor.temperature/friendly_name"}));
  assert(declarations(";;Auto;Auto;sensor.message;;sensor;text;")==std::set<std::string>{"sensor.message/"});
  assert((declarations(";Time;Auto;Auto;sensor.elapsed;;sensor;time;")==
      std::set<std::string>{"sensor.elapsed/","sensor.elapsed/unit_of_measurement"}));
  assert(declarations(";Time;Auto;Auto;sensor.elapsed;;sensor;time;time_unit=minutes")==std::set<std::string>{"sensor.elapsed/"});
  assert(declarations("uptime;Local;Auto;Auto;local;;sensor;1;").empty());
  assert((declarations("light.kitchen;;Auto;Auto;sensor.power;W;;1;")==
      std::set<std::string>{"light.kitchen/","light.kitchen/friendly_name","sensor.power/"}));
  assert(declarations(";;Auto;Auto;sensor.power;W;;1;").empty());
}
void actions_and_numeric_controls() {
  assert(declarations("script.run;Run;Auto;Auto;script.turn_on;;action;;").empty());
  assert(declarations("script.run;Run;Auto;Auto;script.turn_on;;action;;state_entity=sensor.status,state_precision=text")==
      std::set<std::string>{"sensor.status/"});
  assert((declarations("input_select.mode;;Auto;Auto;input_select.select_option;;action;;")==
      std::set<std::string>{"input_select.mode/","input_select.mode/options","input_select.mode/friendly_name"}));
  assert((declarations("number.volume;Level;Auto;Auto;;;slider;;")==
      std::set<std::string>{"number.volume/","number.volume/min","number.volume/max","number.volume/step","number.volume/unit_of_measurement"}));
  assert((declarations("cover.blind;Tilt;Auto;Auto;tilt;;cover;;")==
      std::set<std::string>{"cover.blind/","cover.blind/current_tilt_position"}));
  assert((declarations("cover.blind;Open;Auto;Auto;open;;cover;;")==
      std::set<std::string>{"cover.blind/supported_features"}));
  assert((declarations("cover.blind;Blind;Auto;Auto;modal;;cover;;")==
      std::set<std::string>{"cover.blind/","cover.blind/current_position","cover.blind/current_tilt_position","cover.blind/supported_features"}));
  assert(declarations("cover.blind;;Auto;Auto;modal;;cover;;").count("cover.blind/friendly_name")==1);
  assert((declarations("light.kitchen;Light;Auto;Auto;;;light_control;;")==
      std::set<std::string>{"light.kitchen/","light.kitchen/brightness","light.kitchen/color_temp_kelvin",
          "light.kitchen/supported_color_modes","light.kitchen/friendly_name"}));
}
void complex_and_external_sources() {
  auto climate=declarations("climate.room;Room;Auto;Auto;;;climate_control;;");
  assert(climate.size()==20);for (const auto *attribute:{"","hvac_action","current_temperature","hvac_modes","fan_mode","swing_horizontal_mode"})
    assert(climate.count(std::string("climate.room/")+attribute)==1);
  auto media=declarations("media_player.room;Room;Auto;Auto;control_modal;;media;;",true);
  assert(media.count("sensor.speaker_group/data")==1);assert(media.count("media_player.room/media_position")==0);
  assert(media.count("media_player.room/group_members")==1);assert(media.count("media_player.room/shuffle")==1);
  for (bool low_heap : {false, true}) {
    for (const auto* mode : {"cover_art", "now_playing"}) {
      const auto cover=declarations(std::string("media_player.room;Room;Auto;Auto;")+mode+
          ";;media;;media_cover_art,cover_art_secondary_entity=media_player.tv",low_heap);
      for (const auto* entity : {"media_player.room", "media_player.tv"})
        for (const auto* attribute :
             {"", "source", "media_title", "media_artist", "media_content_id", "media_content_type",
              "media_position", "media_duration", "media_position_updated_at", "entity_picture",
              "entity_picture_local", "access_token"})
          assert(cover.count(std::string(entity)+"/"+attribute)==1);
      assert(cover.count("sensor.speaker_group/data")==1);
    }
  }
  auto full=declarations("media_player.room;Room;Auto;Auto;control_modal;;media;;",false);
  assert(full.count("media_player.room/media_position")==1);assert(full.count("media_player.room/friendly_name")==1);
  assert(declarations(";Empty;Auto;Auto;control_modal;;media;;").empty());
  assert((declarations("camera.door;Door;Auto;Auto;;;image;;image_modal_refresh_mode=activity,image_modal_refresh_trigger=binary_sensor.motion")==
      std::set<std::string>{"camera.door/","camera.door/entity_picture","camera.door/access_token","binary_sensor.motion/"}));
  assert(declarations("https://example.invalid/image;URL;Auto;Auto;;;image;;").empty());
  assert(declarations("switch.guest;WiFi;Auto;Auto;;;wifi_qr;;wifi_tabs=guest")==std::set<std::string>{"switch.guest/"});
  assert(declarations("switch.guest;WiFi;Auto;Auto;;;wifi_qr;;wifi_tabs=qr").empty());
  assert(declarations("weather.home;Weather;Auto;Auto;;;weather;tomorrow;").empty());
  assert(declarations("weather.home;Weather;Auto;Auto;;;weather;;")==std::set<std::string>{"weather.home/"});
  const auto fan=declarations("fan.room;Fan;Auto;Auto;;;fan_control;;fan_light_entity=light.room");
  assert(fan.size()==10);assert(fan.count("fan.room/preset_modes")==1);assert(fan.count("light.room/")==1);
  assert((declarations("timer.cooking;Timer;Auto;Auto;;;timer;;")==
      std::set<std::string>{"timer.cooking/","timer.cooking/duration","timer.cooking/remaining","timer.cooking/finishes_at"}));
  assert(declarations("cover.garage;Open;Auto;Auto;open;;garage;;").empty());
  assert(declarations("cover.gate;Open;Auto;Auto;open;;gate;;")==std::set<std::string>{"cover.gate/"});
  assert(declarations("lock.front;Lock;Auto;Auto;lock;;lock;;").empty());
}
void folder_and_hidden_state() {
  assert((declarations("climate.room;Room;Home;Auto;indicator;;subpage;;subpage_kind=climate")==
      std::set<std::string>{"climate.room/","climate.room/hvac_action"}));
  Document doc(DEVICE_UI_PROFILES[8]);doc.button(1,";Media;Home;Auto;indicator;;subpage;;");
  doc.child(1,"~B,1|M,media_player.room,Next,Auto,Auto,next,,,,");
  auto store=doc.finish();Sink sink;assert(PageCardDependencies().enumerate(store,sink));
  const auto child=store.page(store.card(0)->target)->first_card;
  assert(sink.pairs(child)==std::set<std::string>{"media_player.room/"});
  Document inactive(DEVICE_UI_PROFILES[8]);inactive.button(1,"light.room;Room;Auto;Auto;;;;;");
  inactive.child(1,"~B,1|S,,Hidden,Auto,Auto,sensor.hidden,,1,");
  auto hidden=inactive.finish();Sink ignored;assert(PageCardDependencies().enumerate(hidden,ignored));
  for (const auto &entry:ignored.entries)assert(entry.entity!="sensor.hidden");
  Document alarm(DEVICE_UI_PROFILES[8]);alarm.button(1,"alarm_control_panel.home;Alarm;Home;Auto;indicator;;subpage;;subpage_kind=alarm");
  alarm.child(1,"~B,1|AA,,Arm,Auto,Auto,away,,,,");
  auto inherited=alarm.finish();Sink a;assert(PageCardDependencies().enumerate(inherited,a));
  const auto alarm_child=inherited.page(inherited.card(0)->target)->first_card;
  assert(a.pairs(alarm_child).count("alarm_control_panel.home/")==1);
  Document nested(DEVICE_UI_PROFILES[8]);nested.button(1,";Folder;Home;Auto;indicator;;subpage;;");
  nested.child(1,"B,1|:Nested:Home:Auto:indicator::subpage::");
  const auto unsupported=nested.finish();Sink rejected;
  assert(!PageCardDependencies().enumerate(unsupported,rejected));
}
void fixtures_and_failure_propagation() {
  for (const auto &limits:DEVICE_UI_PROFILES) {
    for (const auto &fixture:PAGE_STORE_GOLDENS) {
      Document doc(limits);doc.button(1,fixture.input);auto store=doc.finish();Sink sink;
      const bool known = cards::context_for(fixture.expected[6],fixture.expected[4]).known;
      const bool prepared = PageCardDependencies().enumerate(store,sink);
      if (prepared != known) std::cerr << "Dependency mismatch: " << fixture.input << '\n';
      assert(prepared == known);
      if (sink.entries.empty()) continue;
      Sink failed;failed.remaining=0;assert(!PageCardDependencies().enumerate(store,failed));
    }
  }
}
struct Arena final:PageArenaAllocator {
  void *allocate(size_t size) override {auto *p=std::malloc(size);if(p)++live;return p;}
  void release(void *p,size_t) override {assert(live>0);--live;std::free(p);}
  size_t live{0};
};
struct Native final:PageEntitySubscriptions {
  bool subscribe(const char *,const char *,void *,uint32_t,uint32_t,Deliver) override {++live;return true;}
  void release(void *) override {live=0;}
  uint32_t connection_generation()const override{return 1;}
  size_t live{0};
};
void maximum_live_sensor_dependencies() {
  for(const auto &limits:DEVICE_UI_PROFILES) {
    Document doc(limits);
    for(size_t parent=1;parent<=limits.slot_count;++parent) {
      doc.button(parent,";Room;Home;Auto;indicator;;subpage;;");
      std::string subpage="~B";
      for(size_t child=1;child<=limits.cards_per_subpage;++child) subpage+=","+std::to_string(child);
      for(size_t child=1;child<=limits.cards_per_subpage;++child) {
        const std::string entity="sensor.room_"+std::to_string(parent)+"_"+std::to_string(child);
        subpage+="|S,"+entity+",Zone,Auto,Auto,"+entity+",C,1,";
      }
      doc.child(parent,subpage);
    }
    const auto store=doc.finish();const auto channels=limits.max_child_cards();
    PageCardDependencies source;Arena arena;Native native;
    {
      PageEntityStateRuntime state(source,arena,native,{channels,channels,channels*8});
      assert(state.prepare(store));assert(native.live==channels);state.activate_prepared(1);
      assert(state.memory().active_channels==channels);assert(state.memory().active_dependencies==channels);
      assert(state.memory().active_state_bytes==0);assert(arena.live==1);
      for(PageId page=1;page<store.page_count();++page) {
        for(size_t child=0;child<store.page(page)->card_count;++child) {
          const auto id=static_cast<CardId>(store.page(page)->first_card+child);
          assert(state.dependency_count({1,id})==1);
        }
      }
      std::cout<<limits.device_profile<<": distinct live sensor channels="<<channels
          <<" state arena="<<state.memory().active_arena_bytes<<'\n';
    }
    assert(arena.live==0);assert(native.live==0);
    PageEntityStateRuntime bounded(source,arena,native,{channels,channels-1,channels*8});
    assert(!bounded.prepare(store));assert(arena.live==0);assert(native.live==0);
  }
}
}  // namespace
int main() {
  sensor_fields_and_modes();actions_and_numeric_controls();complex_and_external_sources();
  folder_and_hidden_state();fixtures_and_failure_propagation();maximum_live_sensor_dependencies();
}
