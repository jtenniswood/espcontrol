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

#include "forecast_reference.h"
#include "position_reference.h"
#include "now_playing_reference.h"
using namespace espcontrol;using namespace espcontrol::pages;using namespace espcontrol::configuration;
static size_t lv_allocations=0,lv_live_bytes=0,cpp_allocations=0;
void *operator new(size_t size){++cpp_allocations;if(void *p=std::malloc(size?size:1))return p;std::abort();}
void *operator new[](size_t size){return ::operator new(size);}
void operator delete(void *p)noexcept{std::free(p);}
void operator delete[](void *p)noexcept{std::free(p);}
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
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
  Fixture(const DeviceUiLimits& limits, std::string_view card) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    assert(writer.append_button(1, reinterpret_cast<const uint8_t*>(card.data()), card.size()) ==
           PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct LocalRelays {};
  Fixture(const DeviceUiLimits& limits, LocalRelays) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {
        ";Relays;Auto;Auto;indicator;;subpage;;", "relay_one;;Auto;Lightbulb;switch;;internal;;",
        "relay_one;Pulse;Auto;Home;push;;internal;;", ";;;;sensor.temp;°C;sensor;1;"};
    for (size_t i = 0; i < 4; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2|I,relay_one,,Auto,Lightbulb,switch,,,|I,relay_one,Pulse,Auto,Home,push,,,";
    assert(writer.append_subpage(1, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    const std::string key = "button_order", order = "1,2,3,4";
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                                 reinterpret_cast<const uint8_t*>(order.data()),
                                 order.size()) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct LocalSensors {};
  Fixture(const DeviceUiLimits& limits, LocalSensors) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Local;Auto;Auto;indicator;;subpage;;",
                                 "physical;Temperature;;;local;°C;sensor;2;",
                                 "physical;Ignored;Home;Auto;local;;sensor;text;",
                                 ";;;;sensor.temp;°C;sensor;1;", "computed;Computed;;;local;V;sensor;2;"};
    for (size_t i = 0; i < 5; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3|S,physical,Ignored,Home,Auto,local,,text,|S,physical,Child,Auto,Auto,local,°C,1,|S,"
        "computed,Computed,Auto,Auto,local,V,2,";
    assert(writer.append_subpage(1, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    const std::string key = "button_order", order = "1,2,3,4,5";
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                                 reinterpret_cast<const uint8_t*>(order.data()),
                                 order.size()) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct OptionSelections {};
  Fixture(const DeviceUiLimits& limits, OptionSelections) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Modes;Auto;Auto;indicator;;subpage;;",
                                 "select.room;;Auto;Auto;;;option_select;;", ";;;;sensor.temp;°C;sensor;1;"};
    for (size_t i = 0; i < 3; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2|S,,Temperature,Auto,Auto,sensor.temp,°C,1,|U,input_select.room,,Auto,Auto,,,,";
    assert(writer.append_subpage(1, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    const std::string key = "button_order", order = "1,2,3";
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                                 reinterpret_cast<const uint8_t*>(order.data()),
                                 order.size()) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct FanStatuses {};
  Fixture(const DeviceUiLimits& limits, FanStatuses) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Modes;Auto;Auto;indicator;;subpage;;",
                                 "fan.room;;Auto;Auto;;;fan_switch;;",
                                 "fan.room;;Auto;Auto;;;fan_oscillate;;"};
    for (size_t i = 0; i < 3; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child = "~B,1,2|B,fan.room,,Auto,Auto,,,,|O,fan.room,,Auto,Auto,,,,";
    assert(writer.append_subpage(1, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    const std::string key = "button_order", order = "1,2,3";
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                                 reinterpret_cast<const uint8_t*>(order.data()),
                                 order.size()) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct PercentSliders {};
  Fixture(const DeviceUiLimits& limits, PercentSliders) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {
        ";Levels;Auto;Auto;indicator;;subpage;;", "fan.room;Fan;Auto;Lightbulb;;;fan_speed;;",
        "light.room;Light;Auto;Auto;;;light_brightness;;", "cover.room;Position;Auto;Auto;;;cover;;",
        "cover.room;Tilt;Auto;Auto;tilt;;cover;;"};
    for (size_t i = 0; i < 5; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3,4|J,fan.room,Fan,Auto,Lightbulb,,,,|V,light.room,Light,Auto,Auto,,,,|C,cover.room,Position,"
        "Auto,Auto,,,,|C,cover.room,Tilt,Auto,Auto,tilt,,,";
    assert(writer.append_subpage(1, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    const std::string key = "button_order", order = "1,2,3,4,5";
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                                 reinterpret_cast<const uint8_t*>(order.data()),
                                 order.size()) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct FanModes {};
  Fixture(const DeviceUiLimits& limits, FanModes) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Modes;Auto;Auto;indicator;;subpage;;",
                                 "fan.room;Ignored;Auto;Auto;;;fan_direction;;",
                                 "fan.room;Ignored;Auto;Auto;;;fan_preset;;"};
    for (size_t i = 0; i < 3; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child = "~B,1,2|E,fan.room,Ignored,Auto,Auto,,,,|Z,fan.room,Ignored,Auto,Auto,,,,";
    assert(writer.append_subpage(1, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    const std::string key = "button_order", order = "1,2,3";
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                                 reinterpret_cast<const uint8_t*>(order.data()),
                                 order.size()) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct MediaControls {};
  Fixture(const DeviceUiLimits& limits, MediaControls) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::array<const char*, 6> cards = {
        ";Controls;Home;Auto;indicator;;subpage;;",
        "media_player.room;;Unused Glyph;Auto;volume;;media;;volume_max=60",
        "media_player.room;Ignored;Unused "
        "Glyph;Auto;control_modal;;media;;number_display=volume,volume_max=60",
        "media_player.room;Ignored;Auto;Auto;control_modal;;media;;",
        "media_player.room;Fixed;Auto;Auto;control_modal;;media;;label_display=label",
        "media_player.room;;Auto;Auto;speaker_group;;media;;number_display=volume,label_display=status"};
    for (uint8_t slot = 1; slot <= cards.size(); ++slot)
      assert(writer.append_button(slot, reinterpret_cast<const uint8_t*>(cards[slot - 1]),
                                  std::strlen(cards[slot - 1])) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3,4|,light.child,Child,Auto,Auto,sensor.temp,W,1,|M,media_player.room,Volume,Auto,Auto,"
        "volume,,,volume_max=60|S,,Temp,Auto,Auto,sensor.temp,°C,1,|M,media_player.room,Controls,Auto,Auto,"
        "control_modal,,,number_display=volume";
    assert(writer.append_subpage(1, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    const std::string key = "button_order", order = "1,2,3,4,5,6";
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                                 reinterpret_cast<const uint8_t*>(order.data()),
                                 order.size()) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  explicit Fixture(const DeviceUiLimits& limits, bool unsupported = false, bool portrait = false,
                   bool stateless = false, bool controls = false, bool weather = false,
                   bool forecasts = false, bool media = false, bool positions = false,
                   bool now_playing = false, bool cover_art = false) {
    PanelConfigWriter writer(document.data(),document.size());assert(writer.begin()==PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),std::strlen(limits.device_profile))==PanelConfigStatus::OK);
    auto button=[&](uint8_t slot,std::string text){assert(writer.append_button(slot,reinterpret_cast<const uint8_t *>(text.data()),text.size())==PanelConfigStatus::OK);};
    if (cover_art) {
      button(1, ";Playback;Home;Auto;indicator;;subpage;;");
      button(2, "media_player.room;Ignored;Unused Glyph;Auto;cover_art;;media;;cover_art_details");
      button(3, "media_player.room;Ignored;Unused Glyph;Auto;now_playing;;media;;media_cover_art");
      button(4, "light.room;Light;Auto;Auto;;;;;");
      const std::string child =
          "~B,1,2,3,4|,light.child,Child,Auto,Auto,,,,|M,media_player.room,Ignored,Auto,Auto,cover_art,,,"
          "cover_art_details|S,,Temp,Auto,Auto,sensor.temp,°C,1,|M,media_player.room,Ignored,Auto,Auto,"
          "now_playing,,,";
      assert(writer.append_subpage(1, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
             PanelConfigStatus::OK);
      const std::string key = "button_order", order = "1,2,3,4";
      assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                                   reinterpret_cast<const uint8_t*>(order.data()),
                                   order.size()) == PanelConfigStatus::OK);
      assert(writer.finish(&size) == PanelConfigStatus::OK);
      assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
             PageStoreStatus::OK);
      return;
    }
    if(now_playing) {
      button(1,";Playback;Home;Auto;indicator;;subpage;;");
      button(2,"media_player.room;Ignored;Unused Glyph;Auto;now_playing;;media;;");
      button(3,"media_player.room;Ignored;Auto;Auto;now_playing;;media;play_pause;");
      button(4,"media_player.room;Ignored;Auto;Auto;now_playing;;media;progress;");
      button(5,";;;;sensor.temp;°C;sensor;1;");button(6,"light.room;Light;Auto;Auto;;;;;");
      const std::string child="~B,1,2,3,4|,light.child,Child,Auto,Auto,,,,|M,media_player.room,Ignored,Auto,Auto,now_playing,,play_pause,|S,,Temp,Auto,Auto,sensor.temp,°C,1,|M,media_player.room,Ignored,Auto,Auto,now_playing,,,";
      assert(writer.append_subpage(1,reinterpret_cast<const uint8_t *>(child.data()),child.size())==PanelConfigStatus::OK);
      const std::string key="button_order",order="1,2,3,4,5,6";
      assert(writer.append_setting(reinterpret_cast<const uint8_t *>(key.data()),key.size(),reinterpret_cast<const uint8_t *>(order.data()),order.size())==PanelConfigStatus::OK);
      assert(writer.finish(&size)==PanelConfigStatus::OK);assert(PageStore::stage_v1(document.data(),size,limits,11,arena.data(),arena.size(),&store)==PageStoreStatus::OK);return;
    }
    if(positions) {
      button(1,";Playback;Home;Auto;indicator;;subpage;;");
      button(2,"media_player.room;Custom;Auto;Auto;position;;media;state;");
      button(3,"media_player.room;;;;play_pause;;media;;");
      button(4,";;;;sensor.temp;°C;sensor;1;");button(5,"cover.room;Blind;Auto;Auto;modal;;cover;0;");
      const std::string child="~B,1,2,3,4|,light.child,Child,Auto,Auto,,,,|M,media_player.room,Seek,Auto,Auto,position,,,|C,cover.room,Blind,Auto,Auto,modal,,,|S,,Temp,Auto,Auto,sensor.temp,°C,1,";
      assert(writer.append_subpage(1,reinterpret_cast<const uint8_t *>(child.data()),child.size())==PanelConfigStatus::OK);
      const std::string key="button_order",order="1,2,3,4,5";
      assert(writer.append_setting(reinterpret_cast<const uint8_t *>(key.data()),key.size(),reinterpret_cast<const uint8_t *>(order.data()),order.size())==PanelConfigStatus::OK);
      assert(writer.finish(&size)==PanelConfigStatus::OK);assert(PageStore::stage_v1(document.data(),size,limits,11,arena.data(),arena.size(),&store)==PageStoreStatus::OK);return;
    }
    if(media) {
      button(1,";Media;Home;Auto;indicator;;subpage;;");
      button(2,"media_player.room;Custom;Auto;Auto;play_pause;;media;state;");
      button(3,"media_player.room;;;;previous;;media;;");
      button(4,"media_player.room;;;;playlist;;media;;playlist_content_id=spotify%3Aplaylist%3Aabcd");
      button(5,"media_player.room;;;;next;;media;;");
      button(6,";;;;sensor.temp;°C;sensor;1;");
      const std::string child="~B,1,2,3,4|M,media_player.room,Next,Auto,Auto,next,,,|M,media_player.room,Play,Auto,Auto,play_pause,,state,|M,media_player.room,Playlist,Auto,Auto,playlist,,,playlist_content_id=spotify%3Aplaylist%3Aabcd|S,,Temp,Auto,Auto,sensor.temp,°C,1,";
      assert(writer.append_subpage(1,reinterpret_cast<const uint8_t *>(child.data()),child.size())==PanelConfigStatus::OK);
      const std::string key="button_order",order="1,2,3,4,5,6";
      assert(writer.append_setting(reinterpret_cast<const uint8_t *>(key.data()),key.size(),reinterpret_cast<const uint8_t *>(order.data()),order.size())==PanelConfigStatus::OK);
      assert(writer.finish(&size)==PanelConfigStatus::OK);
      assert(PageStore::stage_v1(document.data(),size,limits,11,arena.data(),arena.size(),&store)==PageStoreStatus::OK);return;
    }
    if(forecasts) {
      button(1,";Forecasts;Home;Auto;;;subpage;;");button(2,"weather.home;Ignored;Auto;Auto;;;weather;daily;");button(3,"weather.home;Ignored;Auto;Auto;;;weather;tomorrow;");
      const std::string child="~B,1,2|W,weather.home,Ignored,Auto,Auto,,,daily,|W,weather.home,Ignored,Auto,Auto,,,today,";
      assert(writer.append_subpage(1,reinterpret_cast<const uint8_t *>(child.data()),child.size())==PanelConfigStatus::OK);const std::string key="button_order",order="1,2,3";assert(writer.append_setting(reinterpret_cast<const uint8_t *>(key.data()),key.size(),reinterpret_cast<const uint8_t *>(order.data()),order.size())==PanelConfigStatus::OK);assert(writer.finish(&size)==PanelConfigStatus::OK);assert(PageStore::stage_v1(document.data(),size,limits,11,arena.data(),arena.size(),&store)==PageStoreStatus::OK);return;
    }
    if(weather) {
      button(1,";Weather;Home;Auto;;;subpage;;");button(2,"weather.home;Ignored;Auto;Auto;;;weather;0;");
      const std::string child="~B,1,2|W,weather.home,Ignored,Auto,Auto,,,,|W,,Ignored,Auto,Auto,,,,";
      assert(writer.append_subpage(1,reinterpret_cast<const uint8_t *>(child.data()),child.size())==PanelConfigStatus::OK);const std::string key="button_order",order="1,2";assert(writer.append_setting(reinterpret_cast<const uint8_t *>(key.data()),key.size(),reinterpret_cast<const uint8_t *>(order.data()),order.size())==PanelConfigStatus::OK);assert(writer.finish(&size)==PanelConfigStatus::OK);assert(PageStore::stage_v1(document.data(),size,limits,11,arena.data(),arena.size(),&store)==PageStoreStatus::OK);return;
    }
    if(controls) {
      button(1,";Controls;Home;Auto;indicator;;subpage;;");button(2,"cover.room;Blind;Auto;Auto;modal;;cover;0;");button(3,"light.room;Lamp;Auto;Auto;;;light_control;0;");
      button(4,"fan.room;;Auto;Auto;;;fan_control;;");
      const std::string child="~B,1,2,3|LC,light.room,Lamp,Auto,Auto,,,,|C,cover.room,Blind,Auto,Auto,modal,,,|FC,fan.room,,Auto,Auto,,,,";
      assert(writer.append_subpage(1,reinterpret_cast<const uint8_t *>(child.data()),child.size())==PanelConfigStatus::OK);
      const std::string key="button_order",order="1,2,3,4";assert(writer.append_setting(reinterpret_cast<const uint8_t *>(key.data()),key.size(),reinterpret_cast<const uint8_t *>(order.data()),order.size())==PanelConfigStatus::OK);
      assert(writer.finish(&size)==PanelConfigStatus::OK);assert(PageStore::stage_v1(document.data(),size,limits,11,arena.data(),arena.size(),&store)==PageStoreStatus::OK);return;
    }
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
    button(3,unsupported?"media_player.test;;;;control_modal;;media;;":";;Gauge;Gauge;sensor.temp;°C;sensor;1;active_color");
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
  PageRendererRuntime<Slots> *renderer{nullptr};size_t toggles{0},climates{0},commands{0},icon_resolves{0},seeks{0};uint8_t seek_percent{0};float seek_duration{0};PageId seek_page{NO_PAGE};PageBindingToken last;char entity[64]{};
  static const char *icon(void *owner,std::string_view name,std::string_view){++static_cast<Actions *>(owner)->icon_resolves;return name=="Fan"?"FAN":name=="Weather Sunny"?"SUN": name=="Lightbulb"?"ON":name=="Blinds Open"?"OPEN":name=="Blinds"?"CLOSED":"I";}
  static bool valid(void *owner,PageBindingToken t){return static_cast<Actions *>(owner)->renderer->valid(t);}
  static void toggle(void *owner,PageBindingToken t,std::string_view entity){auto &a=*static_cast<Actions *>(owner);++a.toggles;a.last=t;assert(entity.size()<sizeof(a.entity));std::memcpy(a.entity,entity.data(),entity.size());a.entity[entity.size()]='\0';}
  static void open(void *owner,PageBindingToken,PageId page){assert(static_cast<Actions *>(owner)->renderer->open_page(page));}
  static void back(void *owner,PageBindingToken){assert(static_cast<Actions *>(owner)->renderer->back());}
  static void climate(void *owner,PageBindingToken token){auto &a=*static_cast<Actions *>(owner);++a.climates;a.last=token;}
  static void action(void *owner,PageBindingToken token){auto &a=*static_cast<Actions *>(owner);++a.commands;a.last=token;}
  static void seek(void *owner,PageBindingToken token,uint8_t percent,float duration){auto &a=*static_cast<Actions *>(owner);++a.seeks;a.last=token;a.seek_percent=percent;a.seek_duration=duration;if(a.seek_page!=NO_PAGE)assert(a.renderer->open_page(a.seek_page));}
  PageBasicActions callbacks(){return {this,icon,valid,toggle,open,back,climate,action,action,action,action,seek};}
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

template<size_t Slots>void control_profile(const DeviceUiLimits &limits) {
  Fixture fixture(limits,false,false,false,true);Arena arena;Subscriptions subscriptions;PageCardDependencies dependencies;PageEntityStateRuntime state(dependencies,arena,subscriptions,{128,128,8192});
  auto *page=lv_obj_create(lv_screen_active());std::array<lv_coord_t,Slots+1>columns{},rows{};const auto row_count=(Slots+limits.grid_columns-1)/limits.grid_columns;
  for(size_t i=0;i<limits.grid_columns;++i)columns[i]=LV_GRID_FR(1);columns[limits.grid_columns]=LV_GRID_TEMPLATE_LAST;for(size_t i=0;i<row_count;++i)rows[i]=LV_GRID_FR(1);rows[row_count]=LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page,480,480);lv_obj_set_grid_dsc_array(page,columns.data(),rows.data());std::array<PageBasicWidgets,Slots>widgets{};
  for(auto &w:widgets){w.button=lv_button_create(page);w.fill=lv_obj_create(w.button);w.icon=lv_label_create(w.button);w.label=lv_label_create(w.button);w.sensor_container=lv_obj_create(w.button);w.value=lv_label_create(w.sensor_container);w.unit=lv_label_create(w.sensor_container);}
  PageBasicTheme theme;Actions<Slots> actions;
  {auto callbacks=actions.callbacks();callbacks.control=nullptr;PageBasicLvglPort<Slots> missing(arena,state,widgets,{limits.slot_count,limits.grid_columns},theme,callbacks);assert(missing.initialize());assert(!missing.prepare(fixture.store,Slots));}
  {
    Fixture fan_only(limits, std::string_view("fan.room;;Auto;Auto;;;fan_control;;"));
    auto callbacks = actions.callbacks();
    callbacks.control = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                    callbacks);
    assert(missing.initialize());
    assert(!missing.prepare(fan_only.store, Slots));
  }
  {auto incomplete=widgets;incomplete[0].fill=nullptr;PageBasicLvglPort<Slots>missing(arena,state,incomplete,{limits.slot_count,limits.grid_columns},theme,actions.callbacks());assert(missing.initialize());assert(!missing.prepare(fixture.store,Slots));}
  {PageLabelRefresh cache(arena);PageBasicLvglPort<Slots>port(arena,state,widgets,{limits.slot_count,limits.grid_columns},theme,actions.callbacks());assert(port.initialize());std::array<lv_obj_t *,Slots*4>labels{};size_t index=0;for(const auto &w:widgets)for(auto *label:{w.icon,w.label,w.value,w.unit})labels[index++]=label;assert(cache.initialize(labels.data(),labels.size()));PageRendererRuntime<Slots>renderer(limits,arena,port);actions.renderer=&renderer;assert(renderer.prepare(fixture.store)&&state.prepare(fixture.store));renderer.activate_prepared(fixture.store);state.activate_prepared(fixture.store);
    subscriptions.send("fan.room","","on");subscriptions.send("fan.room","friendly_name","Room Fan");subscriptions.send("light.room","","on");subscriptions.send("cover.room","","open");subscriptions.send("cover.room","current_position","25");renderer.update_visible();lv_obj_update_layout(page);lv_refr_now(lv_display_get_default());
    assert(!lv_obj_has_state(widgets[1].button,LV_STATE_CHECKED));assert(lv_obj_has_state(widgets[2].button,LV_STATE_CHECKED));assert(!lv_obj_has_flag(widgets[1].fill,LV_OBJ_FLAG_HIDDEN));assert(lv_obj_get_height(widgets[1].fill)==lv_obj_get_height(widgets[1].button)*75/100);assert(!lv_obj_has_flag(widgets[1].fill,LV_OBJ_FLAG_CLICKABLE));
    lv_obj_set_size(page,600,320);lv_obj_update_layout(page);assert(lv_obj_get_width(widgets[1].fill)==lv_obj_get_width(widgets[1].button)&&lv_obj_get_height(widgets[1].fill)==lv_obj_get_height(widgets[1].button)*75/100);assert(lv_obj_get_y(widgets[1].fill)+lv_obj_get_style_pad_top(widgets[1].button,LV_PART_MAIN)==0);lv_obj_set_size(page,480,480);lv_obj_update_layout(page);
    assert(lv_obj_has_state(widgets[3].button,LV_STATE_CHECKED));
    assert(lv_obj_has_state(widgets[0].button,LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[3].icon),"FAN")==0 &&
           std::strcmp(lv_label_get_text(widgets[3].label),"Room Fan")==0 &&
           lv_obj_has_flag(widgets[3].sensor_container,LV_OBJ_FLAG_HIDDEN));
    click(widgets[3].button);assert(actions.commands==1&&renderer.valid(actions.last));
    const auto fan_token=renderer.binding(3);
    lv_obj_send_event(widgets[3].button,LV_EVENT_PRESSED,nullptr);
    const auto fan_child=fixture.store.card(0)->target;assert(renderer.open_page(fan_child));
    renderer.update_visible();lv_obj_send_event(widgets[3].button,LV_EVENT_CLICKED,nullptr);
    assert(actions.commands==1&&!renderer.valid(fan_token));assert(renderer.back());renderer.update_visible();
    subscriptions.send("fan.room","","off");renderer.update_visible();
    assert(!lv_obj_has_state(widgets[3].button,LV_STATE_CHECKED));
    subscriptions.send("fan.room","","on");renderer.update_visible();
    click(widgets[1].button);assert(actions.commands==2&&renderer.valid(actions.last));
    const auto child=fixture.store.card(0)->target;lv_obj_send_event(widgets[1].button,LV_EVENT_PRESSED,nullptr);assert(renderer.open_page(child));renderer.update_visible();lv_obj_update_layout(page);lv_obj_send_event(widgets[1].button,LV_EVENT_CLICKED,nullptr);assert(actions.commands==2);assert(lv_obj_has_flag(widgets[1].fill,LV_OBJ_FLAG_HIDDEN));assert(!lv_obj_has_flag(widgets[2].fill,LV_OBJ_FLAG_HIDDEN));assert(lv_obj_get_height(widgets[2].fill)==lv_obj_get_height(widgets[2].button)*75/100);
    subscriptions.send("cover.room","current_position","70");subscriptions.send("cover.room","current_position","bad");renderer.update_visible();lv_obj_update_layout(page);assert(lv_obj_get_height(widgets[2].fill)==lv_obj_get_height(widgets[2].button)*30/100);click(widgets[2].button);assert(actions.commands==3);assert(renderer.back());renderer.update_visible();lv_obj_update_layout(page);assert(renderer.open_page(child));renderer.update_visible();assert(renderer.back());renderer.update_visible();lv_obj_update_layout(page);
    const auto cpp=cpp_allocations,reservations=arena.allocations,descriptors=cache.memory().descriptor_adds,retained=lv_live_bytes,channels=subscriptions.bindings.size();
    for(size_t n=0;n<1000;++n){assert(renderer.open_page(child));renderer.update_visible();lv_obj_update_layout(page);assert(renderer.back());renderer.update_visible();lv_obj_update_layout(page);assert(cpp_allocations==cpp&&arena.allocations==reservations&&cache.memory().descriptor_adds==descriptors&&subscriptions.bindings.size()==channels&&lv_live_bytes==retained);}
    ++subscriptions.generation;renderer.update_visible();lv_obj_update_layout(page);assert(lv_obj_has_flag(widgets[1].fill,LV_OBJ_FLAG_HIDDEN)&&!lv_obj_has_state(widgets[2].button,LV_STATE_CHECKED));assert(!lv_obj_has_state(widgets[3].button,LV_STATE_CHECKED));std::printf("%s: light/cover/fan controls 1000 cycles, fixed retained resources, %zu native LVGL bytes including fill bank\n",limits.device_profile,retained);renderer.retire_active();state.retire_active();
  }assert(arena.live==0);lv_obj_delete(page);
}

template <size_t Slots>
void local_relay_profile(const DeviceUiLimits& limits) {
  struct Provider : PageLocalRelayProvider {
    PageLocalRelaySnapshot read(std::string_view) const override {
      return {on, "Hall Relay"};
    }
    bool on{false};
  } provider;
  Fixture fixture(limits, Fixture::LocalRelays{});
  Arena arena;
  Subscriptions transport;
  PageCardDependencies dependencies;
  PageEntityStateRuntime ha(dependencies, arena, transport, {128, 128, 8192});
  PageLocalRelayRuntime relays(arena, provider);
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  const auto row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  for (size_t i = 0; i < row_count; ++i) rows[i] = LV_GRID_FR(1);
  rows[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page, 480, 480);
  lv_obj_set_grid_dsc_array(page, columns.data(), rows.data());
  std::array<PageBasicWidgets, Slots> widgets{};
  for (auto& w : widgets) {
    w.button = lv_button_create(page);
    w.icon = lv_label_create(w.button);
    w.label = lv_label_create(w.button);
    w.sensor_container = lv_obj_create(w.button);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
  }
  PageBasicTheme theme;
  Actions<Slots> actions;
  {
    PageBasicLvglPort<Slots> missing(arena, ha, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     actions.callbacks());
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    auto callbacks = actions.callbacks();
    callbacks.toggle = nullptr;
    PageBasicLvglPort<Slots> missing(arena, ha, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     callbacks, nullptr, nullptr, nullptr, nullptr, &relays);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, ha, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  actions.callbacks(), nullptr, nullptr, nullptr, nullptr, &relays);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[index++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(renderer.prepare(fixture.store) && ha.prepare(fixture.store) && relays.prepare(fixture.store));
    ha.activate_prepared(fixture.store);
    relays.activate_prepared(fixture.store);
    renderer.activate_prepared(fixture.store);
    assert(relays.install(true));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Hall Relay") == 0 &&
           std::strcmp(lv_label_get_text(widgets[2].label), "Pulse") == 0);
    provider.on = true;
    page_local_relay_feed().notify("relay_one", true);
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED) &&
           lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED) &&
           !lv_obj_has_state(widgets[2].button, LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[1].icon), "ON") == 0);
    click(widgets[1].button);
    assert(actions.toggles == 1 && renderer.valid(actions.last) &&
           std::strcmp(actions.entity, "relay_one") == 0);
    click(widgets[2].button);
    assert(actions.toggles == 2 && renderer.valid(actions.last) &&
           fixture.store.field(*fixture.store.card(actions.last.card), CardField::SENSOR) == "push");
    const auto child = fixture.store.card(0)->target;
    const auto old = renderer.binding(1);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.toggles == 2 && !renderer.valid(old));
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Hall Relay") == 0 &&
           lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED) &&
           !lv_obj_has_state(widgets[2].button, LV_STATE_CHECKED));
    provider.on = false;
    page_local_relay_feed().notify("relay_one", false);
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED));
    assert(renderer.back());
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    ++transport.generation;
    provider.on = true;
    page_local_relay_feed().notify("relay_one", true);
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED));
    assert(renderer.open_page(child));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, reservations = arena.allocations, retained = lv_live_bytes,
               channels = transport.bindings.size(), descriptors = cache.memory().descriptor_adds;
    for (int n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reservations && lv_live_bytes == retained &&
             transport.bindings.size() == channels && cache.memory().descriptor_adds == descriptors &&
             relays.memory().active_sources == 1);
    }
    renderer.retire_active();
    ha.retire_active();
    relays.retire_active();
    std::printf(
        "%s: local relays 1000 cycles, pulse/toggle isolation, folder indicators and stable resources\n",
        limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}

template <size_t Slots>
void local_sensor_profile(const DeviceUiLimits& limits) {
  struct Provider : PageLocalSensorProvider {
    size_t capacity() const override {
      return 2;
    }
    uint32_t resolve(std::string_view key, bool text) const override {
      return key == "physical" ? (text ? 1 : 0) : NO_LOCAL_SENSOR;
    }
    bool watch(uint32_t source) override {
      watched[source] = true;
      return true;
    }
    PageLocalSensorSample read(uint32_t source) const override {
      return {revisions[source], number, source ? text : std::string_view{}};
    }
    size_t callbacks() const override {
      return watched[0] + watched[1];
    }
    float number{23.5f};
    std::string_view text;
    std::array<uint64_t, 2> revisions{{1, 1}};
    std::array<bool, 2> watched{};
  } provider;
  Fixture fixture(limits, Fixture::LocalSensors{});
  Arena arena;
  Subscriptions transport;
  PageCardDependencies dependencies;
  PageEntityStateRuntime ha(dependencies, arena, transport, {128, 128, 8192});
  PageLocalSensorRuntime local(arena, provider, {32, 16, 1024});
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  const auto row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  for (size_t i = 0; i < row_count; ++i) rows[i] = LV_GRID_FR(1);
  rows[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page, 480, 480);
  lv_obj_set_grid_dsc_array(page, columns.data(), rows.data());
  std::array<PageBasicWidgets, Slots> widgets{};
  for (auto& w : widgets) {
    w.button = lv_button_create(page);
    w.icon = lv_label_create(w.button);
    w.label = lv_label_create(w.button);
    w.sensor_container = lv_obj_create(w.button);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
  }
  PageBasicTheme theme;
  Actions<Slots> actions;
  {
    PageBasicLvglPort<Slots> missing(arena, ha, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     actions.callbacks());
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, ha, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  actions.callbacks(), nullptr, nullptr, nullptr, &local);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[index++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(renderer.prepare(fixture.store) && ha.prepare(fixture.store) && local.prepare(fixture.store));
    ha.activate_prepared(fixture.store);
    local.activate_prepared(fixture.store);
    renderer.activate_prepared(fixture.store);
    assert(local.install_manual_feed(true));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "23.50") == 0 &&
           std::strcmp(lv_label_get_text(widgets[1].unit), "°C") == 0 &&
           std::strcmp(lv_label_get_text(widgets[1].label), "Temperature") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "--") == 0 &&
           std::strcmp(lv_label_get_text(widgets[4].value), "--") == 0);
    assert(!lv_obj_has_flag(widgets[2].icon, LV_OBJ_FLAG_HIDDEN) &&
           lv_obj_has_flag(widgets[2].sensor_container, LV_OBJ_FLAG_HIDDEN) &&
           !lv_obj_has_flag(widgets[1].sensor_container, LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(widgets[1].button, LV_OBJ_FLAG_CLICKABLE) &&
           !lv_obj_has_flag(widgets[2].button, LV_OBJ_FLAG_CLICKABLE));
    assert(local.memory().live_bindings == 3 && provider.callbacks() == 2);
    provider.text = "Raw ÉTÉ";
    ++provider.revisions[1];
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "Raw ÉTÉ") == 0);
    assert(page_local_sensor_feed().apply_number("computed", 4.25f));
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[4].value), "4.25") == 0);
    const auto child = fixture.store.card(0)->target;
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Raw ÉTÉ") == 0 &&
           !lv_obj_has_flag(widgets[1].icon, LV_OBJ_FLAG_HIDDEN) &&
           lv_obj_has_flag(widgets[1].sensor_container, LV_OBJ_FLAG_HIDDEN));
    assert(std::strcmp(lv_label_get_text(widgets[2].value), "23.5") == 0 &&
           std::strcmp(lv_label_get_text(widgets[2].label), "Child") == 0);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    ++transport.generation;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "Raw ÉTÉ") == 0 &&
           std::strcmp(lv_label_get_text(widgets[1].value), "23.50") == 0);
    provider.number = std::numeric_limits<float>::infinity();
    ++provider.revisions[0];
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "--") == 0 &&
           std::strcmp(lv_label_get_text(widgets[1].unit), "°C") == 0);
    provider.number = 23.5f;
    ++provider.revisions[0];
    renderer.update_visible();
    const auto cpp = cpp_allocations, reservations = arena.allocations, retained = lv_live_bytes,
               channels = transport.bindings.size(), descriptors = cache.memory().descriptor_adds;
    for (int n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reservations && lv_live_bytes == retained &&
             transport.bindings.size() == channels && cache.memory().descriptor_adds == descriptors &&
             provider.callbacks() == 2 && local.memory().live_bindings == 3);
    }
    provider.text = "";
    ++provider.revisions[1];
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "") == 0);
    renderer.retire_active();
    ha.retire_active();
    local.retire_active();
    assert(!page_local_sensor_feed().apply_number("computed", 1));
    std::printf(
        "%s: local sensors 1000 cycles, source deduplication, manual values and stable retained resources\n",
        limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}

template <size_t Slots>
void option_select_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::OptionSelections{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192});
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  const auto row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  for (size_t i = 0; i < row_count; ++i) rows[i] = LV_GRID_FR(1);
  rows[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page, 480, 480);
  lv_obj_set_grid_dsc_array(page, columns.data(), rows.data());
  std::array<PageBasicWidgets, Slots> widgets{};
  for (auto& w : widgets) {
    w.button = lv_button_create(page);
    w.icon = lv_label_create(w.button);
    w.label = lv_label_create(w.button);
    w.sensor_container = lv_obj_create(w.button);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
    lv_obj_set_width(w.sensor_container, 120);
    lv_obj_set_width(w.value, 75);
    lv_label_set_long_mode(w.value, LV_LABEL_LONG_CLIP);
  }
  lv_font_t option_font = *LV_FONT_DEFAULT;
  option_font.line_height += 2;
  PageBasicTheme theme;
  theme.option_select_font = &option_font;
  theme.sensor_font = LV_FONT_DEFAULT;
  theme.sensor_surface = 0x123456;
  Actions<Slots> actions;
  {
    auto callbacks = actions.callbacks();
    callbacks.control = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     callbacks);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  actions.callbacks());
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[index++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(renderer.prepare(fixture.store) && state.prepare(fixture.store));
    renderer.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "--") == 0 &&
           std::strcmp(lv_label_get_text(widgets[1].label), "select.room") == 0);
    subscriptions.send("select.room", "", "Night MODE");
    subscriptions.send("select.room", "friendly_name", "Room Mode");
    subscriptions.send("sensor.temp", "", "21.5");
    subscriptions.send("input_select.room", "", "  Guest [Room]  ");
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "Night MODE") == 0 &&
           std::strcmp(lv_label_get_text(widgets[1].label), "Room Mode") == 0);
    assert(lv_obj_get_style_text_font(widgets[1].value, LV_PART_MAIN) == &option_font &&
           lv_obj_get_style_width(widgets[1].value, LV_PART_MAIN) == lv_pct(100) &&
           lv_label_get_long_mode(widgets[1].value) == LV_LABEL_LONG_DOT);
    assert(lv_obj_has_flag(widgets[1].icon, LV_OBJ_FLAG_HIDDEN) &&
           !lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED));
    click(widgets[1].button);
    assert(actions.commands == 1 && renderer.valid(actions.last));
    const auto child = fixture.store.card(0)->target;
    const auto old = renderer.binding(1);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 1 && !renderer.valid(old));
    assert(lv_obj_get_style_text_font(widgets[1].value, LV_PART_MAIN) == LV_FONT_DEFAULT &&
           lv_obj_get_style_width(widgets[1].value, LV_PART_MAIN) == 75 &&
           lv_obj_get_style_width(widgets[1].sensor_container, LV_PART_MAIN) == 120 &&
           lv_label_get_long_mode(widgets[1].value) == LV_LABEL_LONG_CLIP);
    assert(std::strcmp(lv_label_get_text(widgets[2].value), "  Guest [Room]  ") == 0);
    click(widgets[2].button);
    assert(actions.commands == 2 && renderer.valid(actions.last));
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, reservations = arena.allocations, retained = lv_live_bytes,
               channels = subscriptions.bindings.size(), descriptors = cache.memory().descriptor_adds;
    for (int n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reservations && lv_live_bytes == retained &&
             subscriptions.bindings.size() == channels && cache.memory().descriptor_adds == descriptors);
    }
    ++subscriptions.generation;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "--") == 0);
    renderer.retire_active();
    state.retire_active();
    std::printf("%s: option-select 1000 cycles, stable retained resources and restored value layout\n",
                limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}

uint32_t fan_status_tick = 100;
uint32_t fan_status_now() {
  return fan_status_tick;
}
template <size_t Slots>
void fan_status_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::FanStatuses{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageFanStatusRuntime fan(arena, subscriptions, fan_status_now);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &fan);
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  const auto row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  for (size_t i = 0; i < row_count; ++i) rows[i] = LV_GRID_FR(1);
  rows[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page, 480, 480);
  lv_obj_set_grid_dsc_array(page, columns.data(), rows.data());
  std::array<PageBasicWidgets, Slots> widgets{};
  for (auto& w : widgets) {
    w.button = lv_button_create(page);
    w.icon = lv_label_create(w.button);
    w.label = lv_label_create(w.button);
    w.sensor_container = lv_obj_create(w.button);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
  }
  PageBasicTheme theme;
  Actions<Slots> actions;
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  actions.callbacks(), nullptr, nullptr, nullptr, nullptr, nullptr, &fan);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[index++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(fan.prepare(fixture.store) && renderer.prepare(fixture.store) && state.prepare(fixture.store));
    fan.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    renderer.activate_prepared(fixture.store);
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Fan") == 0);
    click(widgets[1].button);
    assert(actions.commands == 0);
    subscriptions.send("fan.room", "friendly_name", "Bedroom");
    subscriptions.send("fan.room", "", "on");
    subscriptions.send("fan.room", "oscillating", "true");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED) &&
           lv_obj_has_state(widgets[2].button, LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "On") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "Oscillating") == 0);
    click(widgets[1].button);
    click(widgets[2].button);
    assert(actions.commands == 2 && renderer.valid(actions.last));
    const auto child = fixture.store.card(0)->target;
    const auto old = renderer.binding(1);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 2 && !renderer.valid(old));
    fan_status_tick += 2999;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "On") == 0);
    subscriptions.send("fan.room", "", "ON");
    fan_status_tick += 1;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Bedroom") == 0);
    subscriptions.send("fan.room", "oscillating", "bad");
    renderer.update_visible();
    click(widgets[2].button);
    assert(actions.commands == 2);
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "Unsupported") == 0);
    subscriptions.send("fan.room", "oscillating", "true");
    assert(renderer.back());
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    lv_obj_update_layout(page);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, reserved = arena.allocations, retained = lv_live_bytes,
               channels = subscriptions.bindings.size(), descriptors = cache.memory().descriptor_adds;
    for (size_t n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reserved && lv_live_bytes == retained &&
             subscriptions.bindings.size() == channels && cache.memory().descriptor_adds == descriptors);
    }
    subscriptions.send("fan.room", "", "off");
    renderer.update_visible();
    assert(renderer.open_page(child));
    fan_status_tick += 3000;
    renderer.update_visible();  // First paint after expiry is forced, not a quiet poll.
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Bedroom") == 0);
    subscriptions.send("fan.room", "", "on");
    renderer.update_visible();
    assert(renderer.back());
    ++subscriptions.generation;
    renderer.update_visible();  // First forced paint must already be disconnected.

    assert(!lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Unavailable") == 0);
    click(widgets[1].button);
    assert(actions.commands == 2);
    renderer.retire_active();
    state.retire_active();
    fan.retire_active();
    std::printf(
        "%s: fan switch/oscillation 1000 cycles, hidden status history and stable retained resources\n",
        limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}

template <size_t Slots>
void fan_mode_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::FanModes{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192});
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  const auto row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  for (size_t i = 0; i < row_count; ++i) rows[i] = LV_GRID_FR(1);
  rows[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page, 480, 480);
  lv_obj_set_grid_dsc_array(page, columns.data(), rows.data());
  std::array<PageBasicWidgets, Slots> widgets{};
  for (auto& w : widgets) {
    w.button = lv_button_create(page);
    w.icon = lv_label_create(w.button);
    w.label = lv_label_create(w.button);
    w.sensor_container = lv_obj_create(w.button);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
  }
  PageBasicTheme theme;
  Actions<Slots> actions;
  for (bool direction : {true, false}) {
    Fixture one(limits,
                std::string("fan.room;;Auto;Auto;;;") + (direction ? "fan_direction" : "fan_preset") + ";;");
    auto callbacks = actions.callbacks();
    if (direction)
      callbacks.action = nullptr;
    else
      callbacks.control = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     callbacks);
    assert(missing.initialize() && !missing.prepare(one.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  actions.callbacks());
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[index++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(renderer.prepare(fixture.store) && state.prepare(fixture.store));
    renderer.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Unavailable") == 0);
    subscriptions.send("fan.room", "", "off");
    subscriptions.send("fan.room", "direction", "reverse");
    subscriptions.send("fan.room", "preset_mode", "sleep_mode");
    subscriptions.send("fan.room", "preset_modes", "['auto','sleep_mode']");
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED) &&
           lv_obj_has_state(widgets[2].button, LV_STATE_CHECKED));
    assert(!lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Reverse") == 0 &&
           std::strcmp(lv_label_get_text(widgets[2].label), "Sleep Mode") == 0);
    click(widgets[1].button);
    assert(actions.commands == 1 && renderer.valid(actions.last));
    click(widgets[2].button);
    assert(actions.commands == 2 && renderer.valid(actions.last));
    const auto child = fixture.store.card(0)->target;
    for (size_t slot : {size_t(1), size_t(2)}) {
      const auto token = renderer.binding(slot);
      lv_obj_send_event(widgets[slot].button, LV_EVENT_PRESSED, nullptr);
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_send_event(widgets[slot].button, LV_EVENT_CLICKED, nullptr);
      assert(actions.commands == 2 && !renderer.valid(token));
      assert(renderer.back());
      renderer.update_visible();
    }
    subscriptions.send("fan.room", "", "on");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, reserved = arena.allocations, retained = lv_live_bytes,
               channels = subscriptions.bindings.size(), descriptors = cache.memory().descriptor_adds,
               resolves = actions.icon_resolves;
    for (size_t n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reserved && lv_live_bytes == retained &&
             subscriptions.bindings.size() == channels && cache.memory().descriptor_adds == descriptors &&
             actions.icon_resolves == resolves);
    }
    ++subscriptions.generation;
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED) &&
           std::strcmp(lv_label_get_text(widgets[1].label), "Unavailable") == 0);
    renderer.retire_active();
    state.retire_active();
    std::printf("%s: fan direction/preset 1000 cycles, stable retained resources\n", limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}

template<size_t Slots>void weather_profile(const DeviceUiLimits &limits) {
  Fixture fixture(limits,false,false,false,false,true);Arena arena;Subscriptions subscriptions;PageCardDependencies dependencies;PageEntityStateRuntime state(dependencies,arena,subscriptions,{128,128,8192});auto *page=lv_obj_create(lv_screen_active());std::array<lv_coord_t,Slots+1>columns{},rows{};const auto row_count=(Slots+limits.grid_columns-1)/limits.grid_columns;
  for(size_t i=0;i<limits.grid_columns;++i)columns[i]=LV_GRID_FR(1);columns[limits.grid_columns]=LV_GRID_TEMPLATE_LAST;for(size_t i=0;i<row_count;++i)rows[i]=LV_GRID_FR(1);rows[row_count]=LV_GRID_TEMPLATE_LAST;lv_obj_set_size(page,480,480);lv_obj_set_grid_dsc_array(page,columns.data(),rows.data());std::array<PageBasicWidgets,Slots>widgets{};
  for(auto &w:widgets){w.button=lv_button_create(page);w.icon=lv_label_create(w.button);w.label=lv_label_create(w.button);w.sensor_container=lv_obj_create(w.button);w.value=lv_label_create(w.sensor_container);w.unit=lv_label_create(w.sensor_container);}
  PageBasicTheme theme;Actions<Slots> actions;
  {PageLabelRefresh cache(arena);PageBasicLvglPort<Slots>port(arena,state,widgets,{limits.slot_count,limits.grid_columns},theme,actions.callbacks());assert(port.initialize());std::array<lv_obj_t *,Slots*4>labels{};size_t index=0;for(const auto &w:widgets)for(auto *label:{w.icon,w.label,w.value,w.unit})labels[index++]=label;assert(cache.initialize(labels.data(),labels.size()));PageRendererRuntime<Slots>renderer(limits,arena,port);actions.renderer=&renderer;assert(renderer.prepare(fixture.store)&&state.prepare(fixture.store));renderer.activate_prepared(fixture.store);state.activate_prepared(fixture.store);renderer.update_visible();assert(std::strcmp(lv_label_get_text(widgets[1].label),"Unavailable")==0);subscriptions.send("weather.home","","sunny");renderer.update_visible();lv_obj_update_layout(page);lv_refr_now(lv_display_get_default());assert(std::strcmp(lv_label_get_text(widgets[1].label),"Sunny")==0&&std::strcmp(lv_label_get_text(widgets[1].icon),"SUN")==0&&!lv_obj_has_flag(widgets[1].button,LV_OBJ_FLAG_CLICKABLE));
    const auto child=fixture.store.card(0)->target;assert(renderer.open_page(child));renderer.update_visible();lv_obj_update_layout(page);assert(std::strcmp(lv_label_get_text(widgets[2].label),"Cloudy")==0);assert(renderer.back());renderer.update_visible();lv_obj_update_layout(page);
    const auto cpp=cpp_allocations,reservations=arena.allocations,descriptors=cache.memory().descriptor_adds,retained=lv_live_bytes,resolves=actions.icon_resolves,channels=subscriptions.bindings.size();
    for(size_t n=0;n<1000;++n){subscriptions.send("weather.home","",n%2?"partly-cloudy-night":"sunny");assert(renderer.open_page(child));renderer.update_visible();lv_obj_update_layout(page);assert(renderer.back());renderer.update_visible();lv_obj_update_layout(page);assert(cpp_allocations==cpp&&arena.allocations==reservations&&cache.memory().descriptor_adds==descriptors&&subscriptions.bindings.size()==channels&&lv_live_bytes==retained&&actions.icon_resolves==resolves);}
    ++subscriptions.generation;renderer.update_visible();assert(std::strcmp(lv_label_get_text(widgets[1].label),"Unavailable")==0);renderer.retire_active();state.retire_active();std::printf("%s: current weather 1000 cycles, fixed C++/arena/labels/glyph resolution and retained LVGL bytes\n",limits.device_profile);
  }assert(arena.live==0);lv_obj_delete(page);
}

template<size_t Slots>void media_action_profile(const DeviceUiLimits &limits) {
  Fixture fixture(limits,false,false,false,false,false,false,true);
  Arena arena;Subscriptions subscriptions;PageCardDependencies dependencies;
  struct Transport:PageMediaTransport {Subscriptions &subs;explicit Transport(Subscriptions &s):subs(s){}bool connected()const override{return true;}uint32_t connection_generation()const override{return subs.generation;}uint32_t now_ms()const override{return 100;}int64_t epoch_seconds()const override{return 0;}void reannounce()override{}} transport(subscriptions);
  PageMediaStateRuntime media(arena,transport);PageEntityStateRuntime state(dependencies,arena,subscriptions,{128,128,8192},&media);
  auto *page=lv_obj_create(lv_screen_active());std::array<lv_coord_t,Slots+1>columns{},rows{};
  const auto row_count=(Slots+limits.grid_columns-1)/limits.grid_columns;
  for(size_t i=0;i<limits.grid_columns;++i)columns[i]=LV_GRID_FR(1);columns[limits.grid_columns]=LV_GRID_TEMPLATE_LAST;
  for(size_t i=0;i<row_count;++i)rows[i]=LV_GRID_FR(1);rows[row_count]=LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page,480,480);lv_obj_set_grid_dsc_array(page,columns.data(),rows.data());
  std::array<PageBasicWidgets,Slots>widgets{};
  for(auto &w:widgets){w.button=lv_button_create(page);w.icon=lv_label_create(w.button);w.label=lv_label_create(w.button);w.sensor_container=lv_obj_create(w.button);w.value=lv_label_create(w.sensor_container);w.unit=lv_label_create(w.sensor_container);}
  PageBasicTheme theme;Actions<Slots> actions;
  {auto callbacks=actions.callbacks();callbacks.media=nullptr;PageBasicLvglPort<Slots>missing(arena,state,widgets,{limits.slot_count,limits.grid_columns},theme,callbacks,nullptr,&media);assert(missing.initialize());assert(!missing.prepare(fixture.store,Slots));}
  {
    PageLabelRefresh cache(arena);PageBasicLvglPort<Slots>port(arena,state,widgets,{limits.slot_count,limits.grid_columns},theme,actions.callbacks(),nullptr,&media);assert(port.initialize());
    std::array<lv_obj_t *,Slots*4>labels{};size_t index=0;for(const auto &w:widgets)for(auto *label:{w.icon,w.label,w.value,w.unit})labels[index++]=label;
    assert(cache.initialize(labels.data(),labels.size()));
    PageRendererRuntime<Slots>renderer(limits,arena,port);actions.renderer=&renderer;
    assert(media.prepare(fixture.store)&&renderer.prepare(fixture.store)&&state.prepare(fixture.store));media.activate_prepared(fixture.store);renderer.activate_prepared(fixture.store);state.activate_prepared(fixture.store);
    subscriptions.send("media_player.room","","playing");subscriptions.send("media_player.room","media_content_id","spotify:playlist:ABCD?version=2");subscriptions.send("media_player.room","media_content_type","music");subscriptions.send("sensor.temp","","21.4");
    renderer.update_visible();lv_obj_update_layout(page);lv_refr_now(lv_display_get_default());
    assert(lv_obj_has_state(widgets[0].button,LV_STATE_CHECKED)&&lv_obj_has_state(widgets[1].button,LV_STATE_CHECKED)&&lv_obj_has_state(widgets[3].button,LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[1].label),"Playing")==0);
    assert(!lv_obj_has_state(widgets[2].button,LV_STATE_CHECKED)&&!lv_obj_has_state(widgets[4].button,LV_STATE_CHECKED));
    lv_obj_send_event(widgets[2].button,LV_EVENT_PRESSED,nullptr);assert(actions.commands==1&&renderer.valid(actions.last));
    lv_obj_send_event(widgets[2].button,LV_EVENT_RELEASED,nullptr);lv_obj_send_event(widgets[2].button,LV_EVENT_CLICKED,nullptr);assert(actions.commands==1);
    click(widgets[4].button);assert(actions.commands==2);click(widgets[3].button);assert(actions.commands==3);
    const auto child=fixture.store.card(0)->target;
    lv_obj_send_event(widgets[1].button,LV_EVENT_PRESSED,nullptr);assert(renderer.open_page(child));renderer.update_visible();lv_obj_send_event(widgets[1].button,LV_EVENT_CLICKED,nullptr);assert(actions.commands==3);
    click(widgets[1].button);assert(actions.commands==4);assert(renderer.back());renderer.update_visible();
    subscriptions.send("media_player.room","media_content_id","other-playlist");assert(renderer.open_page(child));renderer.update_visible();assert(!lv_obj_has_state(widgets[3].button,LV_STATE_CHECKED));assert(renderer.back());renderer.update_visible();
    subscriptions.send("media_player.room","media_content_id","spotify:playlist:abcd");renderer.update_visible();
    assert(renderer.open_page(child));renderer.update_visible();assert(renderer.back());renderer.update_visible();lv_obj_update_layout(page);
    subscriptions.send("media_player.room","","idle");renderer.update_visible();subscriptions.send("media_player.room","","playing");renderer.update_visible();
    assert(!lv_obj_has_state(widgets[3].button,LV_STATE_CHECKED));
    subscriptions.send("media_player.room","media_content_id","spotify:playlist:abcd");assert(port.state_changed());renderer.update_visible();
    assert(lv_obj_has_state(widgets[3].button,LV_STATE_CHECKED));
    const auto cpp=cpp_allocations,reservations=arena.allocations,descriptors=cache.memory().descriptor_adds,retained=lv_live_bytes,channels=subscriptions.bindings.size();
    for(size_t n=0;n<1000;++n){assert(renderer.open_page(child));renderer.update_visible();lv_obj_update_layout(page);assert(renderer.back());renderer.update_visible();lv_obj_update_layout(page);assert(cpp_allocations==cpp&&arena.allocations==reservations&&cache.memory().descriptor_adds==descriptors&&subscriptions.bindings.size()==channels&&lv_live_bytes==retained);}
    ++subscriptions.generation;renderer.update_visible();assert(!lv_obj_has_state(widgets[1].button,LV_STATE_CHECKED)&&!lv_obj_has_state(widgets[3].button,LV_STATE_CHECKED));assert(std::strcmp(lv_label_get_text(widgets[1].label),"Unknown")==0);
    std::printf("%s: media actions/playlist, 1000 cycles, stable retained resources\n",limits.device_profile);
    renderer.retire_active();state.retire_active();media.retire_active();
  }
  assert(arena.live==0);lv_obj_delete(page);
}

struct PointerInput {
  lv_point_t point{0, 0};
  lv_indev_state_t state{LV_INDEV_STATE_RELEASED};
  static void read(lv_indev_t* device, lv_indev_data_t* data) {
    auto& input = *static_cast<PointerInput*>(lv_indev_get_user_data(device));
    data->point = input.point;
    data->state = input.state;
  }
};
void pointer(lv_indev_t* device, PointerInput& input, lv_obj_t* slider, int percent, bool pressed) {
  lv_area_t area;
  lv_obj_get_coords(slider, &area);
  input.point = {area.x1 + (lv_area_get_width(&area) - 1) * percent / 100, (area.y1 + area.y2) / 2};
  input.state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  lv_indev_read(device);
}
void position_geometry_reference(const PageBasicWidgets& widgets, uint8_t percent, lv_indev_t* device,
                                 PointerInput& input, uint8_t expected_touch) {
  lv_obj_update_layout(widgets.button);
  lv_area_t root_area, actual_slider, actual_fill;
  lv_obj_get_coords(widgets.button, &root_area);
  lv_obj_get_coords(widgets.slider, &actual_slider);
  lv_obj_get_coords(widgets.fill, &actual_fill);
  auto* root = lv_button_create(lv_layer_top());
  lv_obj_set_pos(root, root_area.x1, root_area.y1);
  lv_obj_set_size(root, lv_area_get_width(&root_area), lv_area_get_height(&root_area));
  const auto radius = lv_obj_get_style_radius(widgets.button, LV_PART_MAIN);
  lv_obj_set_style_radius(root, radius, LV_PART_MAIN);
  auto* slider = legacy_position_reference::setup_slider_widget(root, 0x212121, true);
  auto* fill = lv_obj_get_child(root, 0);
  lv_obj_update_layout(root);
  legacy_position_reference::slider_fit_to_button(slider, root, true);
  legacy_position_reference::slider_update_fill(fill, root, percent, true, false, radius, true);
  lv_obj_update_layout(root);
  lv_area_t reference_slider, reference_fill;
  lv_obj_get_coords(slider, &reference_slider);
  lv_obj_get_coords(fill, &reference_fill);
  if (std::memcmp(&actual_slider, &reference_slider, sizeof(actual_slider)) ||
      std::memcmp(&actual_fill, &reference_fill, sizeof(actual_fill)))
    std::fprintf(stderr, "geometry: slider %d,%d,%d,%d vs %d,%d,%d,%d fill %d,%d,%d,%d vs %d,%d,%d,%d\n",
                 actual_slider.x1, actual_slider.y1, actual_slider.x2, actual_slider.y2, reference_slider.x1,
                 reference_slider.y1, reference_slider.x2, reference_slider.y2, actual_fill.x1,
                 actual_fill.y1, actual_fill.x2, actual_fill.y2, reference_fill.x1, reference_fill.y1,
                 reference_fill.x2, reference_fill.y2);
  assert(std::memcmp(&actual_slider, &reference_slider, sizeof(actual_slider)) == 0 &&
         std::memcmp(&actual_fill, &reference_fill, sizeof(actual_fill)) == 0);
  if (expected_touch != 255) {
    pointer(device, input, slider, 90, true);
    pointer(device, input, slider, 90, false);
    assert(lv_slider_get_value(slider) == expected_touch);
  }
  lv_obj_delete(root);
}
void vertical_pointer(lv_indev_t* device, PointerInput& input, lv_obj_t* slider, int percent, bool pressed) {
  lv_area_t area;
  lv_obj_get_coords(slider, &area);
  input.point = {(area.x1 + area.x2) / 2, area.y2 - (lv_area_get_height(&area) - 1) * percent / 100};
  input.state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  lv_indev_read(device);
}
void percent_geometry_reference(const PageBasicWidgets& widgets, uint8_t percent, bool inverted) {
  lv_obj_update_layout(widgets.button);
  lv_area_t root_area, actual_slider, actual_fill;
  lv_obj_get_coords(widgets.button, &root_area);
  lv_obj_get_coords(widgets.slider, &actual_slider);
  lv_obj_get_coords(widgets.fill, &actual_fill);
  auto* root = lv_button_create(lv_layer_top());
  lv_obj_set_pos(root, root_area.x1, root_area.y1);
  lv_obj_set_size(root, lv_area_get_width(&root_area), lv_area_get_height(&root_area));
  const auto radius = lv_obj_get_style_radius(widgets.button, LV_PART_MAIN);
  lv_obj_set_style_radius(root, radius, LV_PART_MAIN);
  auto* slider = legacy_position_reference::setup_slider_widget(root, 0x212121, false);
  auto* fill = lv_obj_get_child(root, 0);
  lv_obj_update_layout(root);
  legacy_position_reference::slider_fit_to_button(slider, root, false);
  legacy_position_reference::slider_update_fill(fill, root, percent, false, inverted, radius, false);
  lv_obj_update_layout(root);
  lv_area_t expected_slider, expected_fill;
  lv_obj_get_coords(slider, &expected_slider);
  lv_obj_get_coords(fill, &expected_fill);
  if (std::memcmp(&actual_slider, &expected_slider, sizeof(actual_slider)) ||
      std::memcmp(&actual_fill, &expected_fill, sizeof(actual_fill)))
    std::fprintf(
        stderr, "percent geometry mismatch slider %d,%d,%d,%d / %d,%d,%d,%d fill %d,%d,%d,%d / %d,%d,%d,%d\n",
        actual_slider.x1, actual_slider.y1, actual_slider.x2, actual_slider.y2, expected_slider.x1,
        expected_slider.y1, expected_slider.x2, expected_slider.y2, actual_fill.x1, actual_fill.y1,
        actual_fill.x2, actual_fill.y2, expected_fill.x1, expected_fill.y1, expected_fill.x2,
        expected_fill.y2);
  assert(std::memcmp(&actual_slider, &expected_slider, sizeof(actual_slider)) == 0 &&
         std::memcmp(&actual_fill, &expected_fill, sizeof(actual_fill)) == 0);
  lv_obj_delete(root);
}
template <size_t Slots>
void percent_slider_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::PercentSliders{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PagePercentSliderRuntime levels(arena, subscriptions);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &levels);
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  const auto row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  for (size_t i = 0; i < row_count; ++i) rows[i] = LV_GRID_FR(1);
  rows[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page, 480, 480);
  lv_obj_set_grid_dsc_array(page, columns.data(), rows.data());
  std::array<PageBasicWidgets, Slots> widgets{};
  for (auto& w : widgets) {
    w.button = lv_button_create(page);
    w.icon = lv_label_create(w.button);
    w.label = lv_label_create(w.button);
    w.sensor_container = lv_obj_create(w.button);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
    w.fill = lv_obj_create(w.button);
    w.slider = lv_slider_create(w.button);
  }
  for (size_t slot : {size_t(1), size_t(3)}) {
    lv_obj_set_style_pad_left(widgets[slot].button, 7, LV_PART_MAIN);
    lv_obj_set_style_pad_right(widgets[slot].button, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_top(widgets[slot].button, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(widgets[slot].button, 9, LV_PART_MAIN);
  }
  PageBasicTheme theme;
  Actions<Slots> actions;
  auto callbacks = actions.callbacks();
  callbacks.percent_slider = [](void* owner, PageBindingToken token, int percent) {
    auto& action = *static_cast<Actions<Slots>*>(owner);
    ++action.seeks;
    action.seek_percent = static_cast<uint8_t>(percent);
    action.last = token;
    if (action.seek_page != NO_PAGE) assert(action.renderer->open_page(action.seek_page));
  };
  {
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     callbacks);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    auto absent = callbacks;
    absent.percent_slider = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     absent, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &levels);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    auto incomplete = widgets;
    incomplete[1].slider = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, incomplete, {limits.slot_count, limits.grid_columns},
                                     theme, callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                     &levels);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  PointerInput input;
  auto* device = lv_indev_create();
  lv_indev_set_type(device, LV_INDEV_TYPE_POINTER);
  lv_indev_set_display(device, lv_display_get_default());
  lv_indev_set_user_data(device, &input);
  lv_indev_set_read_cb(device, PointerInput::read);
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &levels);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t used = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[used++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(levels.prepare(fixture.store) && renderer.prepare(fixture.store) && state.prepare(fixture.store));
    levels.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    renderer.activate_prepared(fixture.store);
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(lv_obj_get_height(widgets[3].fill) == 0);  // No fictitious initial closed-cover fill.
    subscriptions.send("fan.room", "", "off");
    subscriptions.send("light.room", "", "off");
    subscriptions.send("cover.room", "", "closed");
    subscriptions.send("fan.room", "percentage", "25");
    subscriptions.send("light.room", "brightness", "128");
    subscriptions.send("cover.room", "current_position", "20");
    subscriptions.send("cover.room", "current_tilt_position", "70");
    renderer.update_visible();
    lv_obj_update_layout(page);
    for (size_t slot = 1; slot <= 4; ++slot)
      assert(!lv_obj_has_state(widgets[slot].button, LV_STATE_CHECKED));
    assert(lv_slider_get_value(widgets[1].slider) == 25 && lv_slider_get_value(widgets[2].slider) == 50);
    {
      const auto original=lv_obj_get_style_bg_color(widgets[1].fill,LV_PART_MAIN);
      lv_obj_set_style_bg_color(widgets[1].fill,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
      lv_obj_set_style_text_color(widgets[1].button,lv_color_hex(0xFFFFFF),LV_PART_MAIN);
      lv_layer_t layer{};lv_draw_label_dsc_t descriptor;lv_draw_label_dsc_init(&descriptor);
      descriptor.base.layer=&layer;descriptor.color=lv_color_hex(0xFFFFFF);
      lv_draw_task_t task{};task.type=LV_DRAW_TASK_TYPE_LABEL;task.draw_dsc=&descriptor;
      lv_obj_get_coords(widgets[1].fill,&task.clip_area);task.area=task.clip_area;layer._clip_area=task.clip_area;
      lv_obj_send_event(widgets[1].label,LV_EVENT_DRAW_TASK_ADDED,&task);
      assert(lv_color_eq(descriptor.color,lv_color_hex(0x212121))); // Dark text over the bright fill.
      task.clip_area.x1=task.clip_area.x2+1;task.clip_area.x2=task.clip_area.x1+2;
      descriptor.color=lv_color_hex(0x212121);
      lv_obj_send_event(widgets[1].label,LV_EVENT_DRAW_TASK_ADDED,&task);
      assert(lv_color_eq(descriptor.color,lv_color_hex(0xFFFFFF))); // Base text outside the fill.
      lv_obj_set_style_bg_color(widgets[1].fill,original,LV_PART_MAIN);
    }
    percent_geometry_reference(widgets[1], 25, false);
    percent_geometry_reference(widgets[2], 50, false);
    percent_geometry_reference(widgets[3], 80, true);
    percent_geometry_reference(widgets[4], 30, true);
    assert(std::strcmp(lv_label_get_text(widgets[3].icon), "OPEN") == 0);
    subscriptions.send("fan.room", "", "on");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    vertical_pointer(device, input, widgets[1].slider, 0, true);
    vertical_pointer(device, input, widgets[1].slider, 0, false);
    assert(actions.seeks == 1 && actions.seek_percent == 0 && renderer.valid(actions.last));
    vertical_pointer(device, input, widgets[1].slider, 100, true);
    vertical_pointer(device, input, widgets[1].slider, 100, false);
    assert(actions.seeks == 2 && actions.seek_percent == 100);
    // A repeated report still updates the physical thumb during an active
    // drag. A later pointer sample then wins, matching the legacy callbacks.
    vertical_pointer(device, input, widgets[1].slider, 75, true);
    subscriptions.send("fan.room", "percentage", "25");
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[1].slider) == 25);
    subscriptions.send("fan.room", "percentage", "25");
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[1].slider) == 25);
    lv_obj_send_event(widgets[1].slider, LV_EVENT_PRESS_LOST, nullptr);
    vertical_pointer(device, input, widgets[1].slider, 75, false);
    assert(actions.seeks == 2);
    vertical_pointer(device, input, widgets[1].slider, 100, true);
    vertical_pointer(device, input, widgets[1].slider, 100, false);
    assert(actions.seeks == 3);
    const auto root_binding = renderer.binding(1);
    assert(levels.read({11, root_binding.card}).preview);
    const auto child = fixture.store.card(0)->target;
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(lv_slider_get_value(widgets[1].slider) == 25);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(lv_slider_get_value(widgets[1].slider) == 100);
    subscriptions.send("fan.room", "percentage", "25");
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[1].slider) == 25 && !levels.read({11, root_binding.card}).preview);
    vertical_pointer(device, input, widgets[1].slider, 75, true);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    vertical_pointer(device, input, widgets[1].slider, 75, false);
    assert(actions.seeks == 3 && !renderer.valid(root_binding));
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    vertical_pointer(device, input, widgets[1].slider, 75, true);
    ++subscriptions.generation;
    renderer.update_visible();
    vertical_pointer(device, input, widgets[1].slider, 75, false);
    assert(actions.seeks == 3 && !lv_obj_has_flag(widgets[1].slider, LV_OBJ_FLAG_CLICKABLE));
    subscriptions.send("fan.room", "", "on");
    subscriptions.send("fan.room", "percentage", "25");
    renderer.update_visible();
    lv_obj_update_layout(page);
    actions.seek_page = child;
    vertical_pointer(device, input, widgets[1].slider, 100, true);
    vertical_pointer(device, input, widgets[1].slider, 100, false);
    actions.seek_page = NO_PAGE;
    assert(actions.seeks == 4 && !renderer.valid(actions.last));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    vertical_pointer(device, input, widgets[1].slider, 75, true);
    assert(renderer.suspend() && renderer.resume());
    renderer.update_visible();
    vertical_pointer(device, input, widgets[1].slider, 75, false);
    assert(actions.seeks == 4);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, retained = lv_live_bytes, reservations = arena.allocations,
               channels = subscriptions.bindings.size(), descriptors = cache.memory().descriptor_adds;
    for (int n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && lv_live_bytes == retained && arena.allocations == reservations &&
             subscriptions.bindings.size() == channels && cache.memory().descriptor_adds == descriptors);
    }
    renderer.retire_active();
    state.retire_active();
    levels.retire_active();
    std::printf(
        "%s: percent sliders 1000 cycles, legacy geometry, vertical endpoints and stable retained "
        "resources\n",
        limits.device_profile);
  }
  assert(arena.live == 0);
  lv_indev_delete(device);
  lv_obj_delete(page);
}
template <size_t Slots>
void media_position_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, false, false, false, false, false, false, false, true);
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  struct Transport : PageMediaTransport {
    Subscriptions& subs;
    uint32_t now = 100;
    bool online = true;
    explicit Transport(Subscriptions& s) : subs(s) {}
    bool connected() const override {
      return online;
    }
    uint32_t connection_generation() const override {
      return subs.generation;
    }
    uint32_t now_ms() const override {
      return now;
    }
    int64_t epoch_seconds() const override {
      return 1000;
    }
    void reannounce() override {
      ++requests;
    }
    size_t requests = 0;
  } transport(subscriptions);
  PageMediaStateRuntime media(arena, transport);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &media);
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  const auto row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  for (size_t i = 0; i < row_count; ++i) rows[i] = LV_GRID_FR(1);
  rows[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page, 480, 480);
  lv_obj_set_grid_dsc_array(page, columns.data(), rows.data());
  std::array<PageBasicWidgets, Slots> widgets{};
  for (auto& w : widgets) {
    w.button = lv_button_create(page);
    w.fill = lv_obj_create(w.button);
    w.icon = lv_label_create(w.button);
    w.label = lv_label_create(w.button);
    lv_obj_align(w.label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    w.sensor_container = lv_obj_create(w.button);
    lv_obj_set_style_bg_opa(w.sensor_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(w.sensor_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(w.sensor_container, 0, LV_PART_MAIN);
    lv_obj_set_size(w.sensor_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
    w.slider = lv_slider_create(w.button);
  }
  PointerInput input;
  auto* device = lv_indev_create();
  lv_indev_set_type(device, LV_INDEV_TYPE_POINTER);
  lv_indev_set_display(device, lv_display_get_default());
  lv_indev_set_user_data(device, &input);
  lv_indev_set_read_cb(device, PointerInput::read);
  lv_timer_pause(lv_indev_get_read_timer(device));
  PageBasicTheme theme;
  theme.sensor_font = &lv_font_montserrat_14;
  theme.media_surface = 0x101010;
  Actions<Slots> actions;
  {
    auto callbacks = actions.callbacks();
    callbacks.seek = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     callbacks, nullptr, &media);
    assert(missing.initialize());
    assert(!missing.prepare(fixture.store, Slots));
  }
  {
    auto incomplete = widgets;
    incomplete[0].slider = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, incomplete, {limits.slot_count, limits.grid_columns},
                                     theme, actions.callbacks(), nullptr, &media);
    assert(missing.initialize());
    assert(!missing.prepare(fixture.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  actions.callbacks(), nullptr, &media);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[index++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(media.prepare(fixture.store) && renderer.prepare(fixture.store) && state.prepare(fixture.store));
    media.activate_prepared(fixture.store);
    renderer.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "media_duration", "300");
    subscriptions.send("media_player.room", "media_position", "60");
    subscriptions.send("cover.room", "current_position", "25");
    renderer.update_visible();
    lv_obj_update_layout(page);
    lv_refr_now(lv_display_get_default());
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "1:00") == 0 &&
           std::strcmp(lv_label_get_text(widgets[1].label), "Playing") == 0 &&
           lv_obj_has_flag(widgets[1].icon, LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_width(widgets[1].slider) > lv_obj_get_height(widgets[1].slider));
    pointer(device, input, widgets[1].slider, 90, true);
    assert(actions.seeks == 0);
    pointer(device, input, widgets[1].slider, 90, false);
    assert(actions.seeks == 1 && actions.seek_percent >= 88 && actions.seek_percent <= 92 &&
           actions.seek_duration == 300);
    const auto percent = actions.seek_percent;
    position_geometry_reference(widgets[1], percent, device, input, percent);
    subscriptions.send("media_player.room", "media_position", "61");
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[1].slider) >= 88);
    subscriptions.send("media_player.room", "media_position", std::to_string(300 * percent / 100));
    renderer.update_visible();
    subscriptions.send("media_player.room", "", "paused");
    renderer.update_visible();
    pointer(device, input, widgets[1].slider, 50, true);
    pointer(device, input, widgets[1].slider, 50, false);
    assert(actions.seeks == 2);
    transport.now += 3001;
    assert(port.state_changed());
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[1].slider) >= 88);
    pointer(device, input, widgets[1].slider, 50, true);
    transport.online = false;
    ++subscriptions.generation;
    media.loop();
    renderer.update_visible();
    transport.online = true;
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "media_duration", "300");
    subscriptions.send("media_player.room", "media_position", "60");
    renderer.update_visible();
    lv_indev_read(device);
    assert(actions.seeks == 2);
    pointer(device, input, widgets[1].slider, 50, false);
    assert(actions.seeks == 2);
    pointer(device, input, widgets[1].slider, 90, true);
    pointer(device, input, widgets[1].slider, 90, false);
    assert(actions.seeks == 3);
    const auto child = fixture.store.card(0)->target;
    actions.seek_page = child;
    pointer(device, input, widgets[1].slider, 20, true);
    pointer(device, input, widgets[1].slider, 20, false);
    actions.seek_page = NO_PAGE;
    assert(actions.seeks == 4 && !renderer.valid(actions.last));
    lv_obj_send_event(widgets[1].slider, LV_EVENT_RELEASED, nullptr);
    assert(actions.seeks == 4);
    renderer.update_visible();
    assert(lv_obj_has_flag(widgets[1].slider, LV_OBJ_FLAG_HIDDEN) &&
           !lv_obj_has_flag(widgets[2].slider, LV_OBJ_FLAG_HIDDEN));
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(renderer.open_page(child));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    // A takeover must cancel the physical pointer until it releases, even if
    // the same media card becomes visible again immediately.
    pointer(device, input, widgets[1].slider, 70, true);
    assert(renderer.suspend() && renderer.resume());
    renderer.update_visible();
    pointer(device, input, widgets[1].slider, 70, false);
    assert(actions.seeks == 4);
    subscriptions.send("media_player.room", "media_duration", "3.4e38");
    renderer.update_visible();
    assert(!lv_obj_has_flag(widgets[1].slider, LV_OBJ_FLAG_CLICKABLE));
    pointer(device, input, widgets[1].slider, 70, true);
    pointer(device, input, widgets[1].slider, 70, false);
    assert(actions.seeks == 4);
    subscriptions.send("media_player.room", "media_duration", "300");
    subscriptions.send("media_player.room", "", "paused");
    subscriptions.send("media_player.room", "media_position", "0");
    renderer.update_visible();
    position_geometry_reference(widgets[1], 0, device, input, 255);
    assert(lv_color_to_u32(lv_obj_get_style_bg_color(widgets[1].button, LV_PART_MAIN)) ==
           lv_color_to_u32(lv_color_hex(theme.media_surface)));
    assert(lv_color_to_u32(lv_obj_get_style_bg_color(widgets[1].fill, LV_PART_MAIN)) ==
           lv_color_to_u32(lv_color_hex(theme.surface)));
    lv_obj_set_size(page, 600, 320);
    lv_obj_update_layout(page);
    position_geometry_reference(widgets[1], 0, device, input, 255);
    lv_obj_set_size(page, 480, 480);
    lv_obj_update_layout(page);
    lv_indev_reset(device, nullptr);
    const auto cpp = cpp_allocations, reservations = arena.allocations,
               descriptors = cache.memory().descriptor_adds, retained = lv_live_bytes,
               channels = subscriptions.bindings.size();
    for (size_t n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reservations &&
             cache.memory().descriptor_adds == descriptors && subscriptions.bindings.size() == channels &&
             lv_live_bytes == retained);
    }
    assert(transport.requests == 0);
    std::printf(
        "%s: media position, real pointer/legacy geometry parity, 1000 cycles, stable retained resources\n",
        limits.device_profile);
    renderer.retire_active();
    state.retire_active();
    media.retire_active();
  }
  assert(arena.live == 0);
  lv_indev_delete(device);
  lv_obj_delete(page);
}

void now_playing_geometry(const PageBasicWidgets& widgets, const PageBasicTheme& theme, bool progress,
                          bool play_pause, std::string_view title, std::string_view artist,
                          uint8_t row_span = 1, uint8_t column_span = 1, bool cover = false) {
  lv_obj_update_layout(widgets.button);
  lv_area_t button_area, title_area, artist_area;
  lv_obj_get_coords(widgets.button, &button_area);
  lv_obj_get_coords(widgets.icon, &title_area);
  lv_obj_get_coords(widgets.label, &artist_area);
  auto* root = lv_button_create(lv_layer_top());
  lv_obj_set_pos(root, button_area.x1, button_area.y1);
  lv_obj_set_size(root, lv_area_get_width(&button_area), lv_area_get_height(&button_area));
  legacy_now_playing_reference::CardPadding padding{
      lv_obj_get_style_pad_left(widgets.button, LV_PART_MAIN),
      lv_obj_get_style_pad_top(widgets.button, LV_PART_MAIN),
      lv_obj_get_style_pad_right(widgets.button, LV_PART_MAIN),
      lv_obj_get_style_pad_bottom(widgets.button, LV_PART_MAIN)};
  lv_obj_set_style_border_width(root, lv_obj_get_style_border_width(widgets.button, LV_PART_MAIN),
                                LV_PART_MAIN);
  lv_obj_set_style_pad_left(root, progress ? 0 : padding.left, LV_PART_MAIN);
  lv_obj_set_style_pad_top(root, progress ? 0 : padding.top, LV_PART_MAIN);
  lv_obj_set_style_pad_right(root, progress ? 0 : padding.right, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(root, progress ? 0 : padding.bottom, LV_PART_MAIN);
  auto* first = lv_label_create(root);
  auto* second = lv_label_create(root);
  lv_obj_set_style_text_font(second, theme.label_font, LV_PART_MAIN);
  for (auto* label : {first, second}) {
    lv_obj_set_style_transform_scale_x(label, theme.media_text_scale_x, LV_PART_MAIN);
    lv_obj_set_style_transform_scale_y(label, theme.media_text_scale_y, LV_PART_MAIN);
  }
  legacy_now_playing_reference::setup_media_now_playing_layout(
      root, nullptr, first, second, theme.media_title_font,
      progress ? padding : legacy_now_playing_reference::CardPadding{},
      cover           ? (row_span == 3 && column_span == 3                      ? 5
                         : row_span == 1 || (row_span == 2 && column_span == 2) ? 2
                                                                                : 0)
      : row_span == 1 ? 2
                      : 0,
      play_pause, progress ? padding.left : 0, false);
  lv_label_set_text(first, std::string(title).c_str());
  lv_label_set_text(second, std::string(artist).c_str());
  lv_obj_update_layout(root);
  if (cover && row_span >= 2 && column_span >= 2) {
    lv_obj_align_to(second, first, LV_ALIGN_OUT_BOTTOM_LEFT, 0, std::max<int32_t>(0, padding.top / 2));
    lv_obj_update_layout(root);
  }
  lv_area_t expected_title, expected_artist;
  lv_obj_get_coords(first, &expected_title);
  lv_obj_get_coords(second, &expected_artist);
  if (std::memcmp(&title_area, &expected_title, sizeof(title_area)) ||
      std::memcmp(&artist_area, &expected_artist, sizeof(artist_area)))
    std::fprintf(
        stderr,
        "now geometry progress=%d title %d,%d,%d,%d vs %d,%d,%d,%d artist %d,%d,%d,%d vs %d,%d,%d,%d\n",
        progress, title_area.x1, title_area.y1, title_area.x2, title_area.y2, expected_title.x1,
        expected_title.y1, expected_title.x2, expected_title.y2, artist_area.x1, artist_area.y1,
        artist_area.x2, artist_area.y2, expected_artist.x1, expected_artist.y1, expected_artist.x2,
        expected_artist.y2);
  assert(std::memcmp(&title_area, &expected_title, sizeof(title_area)) == 0 &&
         std::memcmp(&artist_area, &expected_artist, sizeof(artist_area)) == 0);
  assert(lv_obj_get_style_text_font(widgets.icon, LV_PART_MAIN) == theme.media_title_font &&
         lv_obj_get_style_text_font(widgets.label, LV_PART_MAIN) == theme.label_font);
  assert(lv_obj_get_style_transform_scale_x(widgets.icon, LV_PART_MAIN) == theme.media_text_scale_x);
  lv_obj_delete(root);
}
template <size_t Slots>
void media_control_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::MediaControls{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  struct Transport final : PageMediaTransport {
    Subscriptions& subscriptions;
    uint32_t now{100};
    bool online{true};
    explicit Transport(Subscriptions& value) : subscriptions(value) {}
    bool connected() const override {
      return online;
    }
    uint32_t connection_generation() const override {
      return subscriptions.generation;
    }
    uint32_t now_ms() const override {
      return now;
    }
    int64_t epoch_seconds() const override {
      return 1000;
    }
    void reannounce() override {}
  } transport(subscriptions);
  PageMediaStateRuntime media(arena, transport);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &media);
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  const auto row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  for (size_t i = 0; i < row_count; ++i) rows[i] = LV_GRID_FR(1);
  rows[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page, 480, 480);
  lv_obj_set_grid_dsc_array(page, columns.data(), rows.data());
  lv_font_t icon_font = lv_font_montserrat_14;
  icon_font.line_height = 26;
  std::array<PageBasicWidgets, Slots> widgets{};
  for (auto& w : widgets) {
    w.button = lv_button_create(page);
    lv_obj_set_style_pad_all(w.button, 8, 0);
    w.icon = lv_label_create(w.button);
    lv_obj_set_style_text_font(w.icon, &icon_font, 0);
    lv_obj_align(w.icon, LV_ALIGN_TOP_LEFT, 0, 0);
    w.label = lv_label_create(w.button);
    lv_obj_align(w.label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    w.sensor_container = lv_obj_create(w.button);
    lv_obj_align(w.sensor_container, LV_ALIGN_CENTER, 3, 4);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
  }
  PageBasicTheme theme;
  theme.sensor_font = &lv_font_montserrat_14;
  theme.label_font = &lv_font_montserrat_14;
  theme.icon_font = &icon_font;
  Actions<Slots> actions;
  auto callbacks = actions.callbacks();
  callbacks.media_control = Actions<Slots>::action;
  callbacks.media_volume = Actions<Slots>::action;
  callbacks.icon = [](void* owner, std::string_view name, std::string_view domain) -> const char* {
    return name == "Unused Glyph" ? nullptr : Actions<Slots>::icon(owner, name, domain);
  };
  for (int missing = 0; missing < 3; ++missing) {
    auto hooks = callbacks;
    if (missing == 0) hooks.media_volume = nullptr;
    if (missing == 1) hooks.media_control = nullptr;
    PageBasicLvglPort<Slots> invalid(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     hooks, nullptr, missing == 2 ? nullptr : &media);
    assert(invalid.initialize() && !invalid.prepare(fixture.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks, nullptr, &media);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[index++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(media.prepare(fixture.store) && renderer.prepare(fixture.store) && state.prepare(fixture.store));
    media.activate_prepared(fixture.store);
    renderer.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "--") == 0);
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "volume_level", ".85");
    subscriptions.send("media_player.room", "supported_features", "4");
    subscriptions.send("light.child", "", "on");
    subscriptions.send("sensor.temp", "", "21.4");
    renderer.update_visible();
    lv_obj_update_layout(page);
    for (size_t slot : {1, 2}) {
      const auto& w = widgets[slot];
      assert(lv_obj_has_flag(w.icon, LV_OBJ_FLAG_HIDDEN) &&
             !lv_obj_has_flag(w.sensor_container, LV_OBJ_FLAG_HIDDEN));
      assert(std::strcmp(lv_label_get_text(w.value), "60") == 0 &&
             lv_obj_get_style_align(w.sensor_container, LV_PART_MAIN) == LV_ALIGN_TOP_LEFT);
    }
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Volume") == 0);
    assert(!lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED) &&
           lv_obj_has_state(widgets[2].button, LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "Playing") == 0 &&
           std::strcmp(lv_label_get_text(widgets[4].label), "Fixed") == 0);
    assert(!lv_obj_has_flag(widgets[3].icon, LV_OBJ_FLAG_HIDDEN) &&
           lv_obj_has_flag(widgets[3].sensor_container, LV_OBJ_FLAG_HIDDEN));
    click(widgets[1].button);
    assert(actions.commands == 1 && actions.last.card == renderer.binding(1).card);
    click(widgets[3].button);
    assert(actions.commands == 2 && actions.last.card == renderer.binding(3).card);
    const auto group_binding = renderer.binding(5);
    assert(std::strcmp(lv_label_get_text(widgets[5].label), "Speaker Group") == 0 &&
           !lv_obj_has_flag(widgets[5].icon, LV_OBJ_FLAG_HIDDEN) &&
           lv_obj_has_flag(widgets[5].sensor_container, LV_OBJ_FLAG_HIDDEN));
    assert(!port.preview_volume(group_binding, 30));
    assert(!port.preview_group(renderer.binding(1), true));
    subscriptions.send("media_player.room", "group_members", "['media_player.room']");
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[5].button, LV_STATE_CHECKED));
    assert(port.preview_group(group_binding, true));
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[5].button, LV_STATE_CHECKED));
    transport.now += 20000;
    assert(!port.state_changed());
    subscriptions.send("media_player.room", "shuffle", "on");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[5].button, LV_STATE_CHECKED));
    subscriptions.send("media_player.room", "group_members", "['media_player.room']");
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[5].button, LV_STATE_CHECKED));
    subscriptions.send("media_player.room", "group_members", "['media_player.other']");
    renderer.update_visible();
    assert(port.preview_group(group_binding, false));
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[5].button, LV_STATE_CHECKED));
    subscriptions.send("media_player.room", "shuffle", "off");
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[5].button, LV_STATE_CHECKED));
    subscriptions.send("media_player.room", "group_members", "['media_player.other','media_player.third']");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[5].button, LV_STATE_CHECKED));

    subscriptions.send("media_player.room", "group_members", "['media_player.room','media_player.room']");
    renderer.update_visible();assert(!lv_obj_has_state(widgets[5].button,LV_STATE_CHECKED));
    subscriptions.send("media_player.room", "group_members", "['media_player.other']");
    subscriptions.send("media_player.room", "", "paused");renderer.update_visible();
    assert(lv_obj_has_state(widgets[5].button,LV_STATE_CHECKED)&&!lv_obj_has_state(widgets[2].button,LV_STATE_CHECKED));
    click(widgets[5].button);assert(actions.commands==3&&actions.last.card==group_binding.card);
    subscriptions.send("media_player.room", "", "unavailable");renderer.update_visible();assert(!lv_obj_has_state(widgets[5].button,LV_STATE_CHECKED));
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "volume_level", ".2");
    renderer.update_visible();
    const auto origin = renderer.binding(1);
    assert(port.preview_volume(origin, 50));
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "50") == 0);
    subscriptions.send("media_player.room", "volume_level", ".25");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "50") == 0);
    transport.now += 1499;
    assert(!port.state_changed());
    ++transport.now;
    assert(port.state_changed());
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "25") == 0);
    assert(port.preview_volume(origin, 50));
    renderer.update_visible();
    subscriptions.send("media_player.room", "volume_level", ".5");
    renderer.update_visible();
    transport.now += 1500;
    assert(!port.state_changed());
    assert(port.preview_volume(origin, 60));
    subscriptions.send("media_player.room", "supported_features", "1024");
    subscriptions.send("media_player.room", "volume_level", ".85");
    renderer.update_visible();
    assert(!port.preview_volume(origin, 30) && std::strcmp(lv_label_get_text(widgets[1].value), "85") == 0);
    subscriptions.send("media_player.room", "supported_features", "0");
    renderer.update_visible();
    assert(!port.preview_volume(origin, 30));
    subscriptions.send("media_player.room", "supported_features", "4");
    renderer.update_visible();
    assert(port.preview_volume(origin, 50));
    ++subscriptions.generation;
    media.loop();
    renderer.update_visible();
    assert(!port.preview_volume(origin, 40));
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "--") == 0);
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "volume_level", ".2");
    subscriptions.send("media_player.room", "supported_features", "4");
    renderer.update_visible();
    assert(port.preview_volume(origin, 50));
    transport.online = false;
    media.loop();
    renderer.update_visible();
    assert(!port.preview_volume(origin, 40));
    transport.online = true;
    subscriptions.send("media_player.room", "", "playing");
    renderer.update_visible();
    const auto child = fixture.store.card(0)->target;
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    lv_obj_send_event(widgets[5].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    lv_obj_send_event(widgets[5].button, LV_EVENT_CLICKED, nullptr);
    assert(renderer.active_page()==child);assert(!port.preview_group(group_binding,true));
    assert(actions.commands == 3 && !port.preview_volume(origin, 30));
    assert(lv_obj_get_style_align(widgets[1].sensor_container, LV_PART_MAIN) == LV_ALIGN_CENTER &&
           lv_obj_get_style_x(widgets[1].sensor_container, LV_PART_MAIN) == 3);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, reservations = arena.allocations, retained = lv_live_bytes,
               descriptors = cache.memory().descriptor_adds, channels = subscriptions.bindings.size();
    for (size_t n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reservations && lv_live_bytes == retained &&
             cache.memory().descriptor_adds == descriptors && subscriptions.bindings.size() == channels);
    }
    // A durable replacement revokes the old card and any pending panel preview.
    const auto retired = renderer.binding(1);
    assert(port.preview_volume(retired, 30));
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    Fixture ordinary(limits);
    PageStore replacement;
    assert(PageStore::stage_v1(ordinary.document.data(), ordinary.size, limits, 12, ordinary.arena.data(),
                               ordinary.arena.size(), &replacement) == PageStoreStatus::OK);
    assert(media.prepare(replacement) && renderer.prepare(replacement) && state.prepare(replacement));
    renderer.activate_prepared(replacement);
    media.activate_prepared(replacement);
    state.activate_prepared(replacement);
    assert(!port.preview_volume(retired, 40));
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 3);
    renderer.update_visible();
    lv_obj_update_layout(page);
    renderer.retire_active();
    state.retire_active();
    media.retire_active();
    std::printf(
        "%s: media volume/control cards and speaker groups, pending acknowledgements/modal hooks, 1000 cycles, stable retained "
        "resources\n",
        limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}
template <size_t Slots>
void cover_art_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, false, false, false, false, false, false, false, false, false, true);
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  struct Transport final : PageMediaTransport {
    Subscriptions& subscriptions;
    explicit Transport(Subscriptions& value) : subscriptions(value) {}
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
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &media);
  struct View final : PageCoverArtView {
    std::array<PageBindingToken, Slots> tokens{};
    std::array<uint16_t, 16 * 16> pixels{};
    lv_image_dsc_t descriptor{};
    uint64_t asset{1}, changes{0};
    size_t bindings{0}, retired{0};
    bool available{false};
    View() {
      pixels.fill(0xf800);
      descriptor.header.magic = LV_IMAGE_HEADER_MAGIC;
      descriptor.header.cf = LV_COLOR_FORMAT_RGB565;
      descriptor.header.w = 16;
      descriptor.header.h = 16;
      descriptor.header.stride = 32;
      descriptor.data = reinterpret_cast<const uint8_t*>(pixels.data());
      descriptor.data_size = pixels.size() * 2;
    }
    bool ready() const override {
      return true;
    }
    void bind(PageBindingToken token) override {
      tokens[token.shell] = token;
      ++bindings;
    }
    void unbind(PageBindingToken token) override {
      tokens[token.shell] = {};
      ++retired;
    }
    void size(PageBindingToken, uint16_t width, uint16_t height) override {
      assert(width && height);
    }
    PageCoverArtSurface surface(PageBindingToken token) override {
      if (!available || token.shell >= Slots || !token.binding ||
          tokens[token.shell].configuration != token.configuration ||
          tokens[token.shell].binding != token.binding)
        return {};
      return {&descriptor, asset, 0x440000};
    }
    uint64_t revision() const override {
      return changes;
    }
  } view;
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  const auto row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  for (size_t i = 0; i < row_count; ++i) rows[i] = LV_GRID_FR(1);
  rows[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page, 480, 480);
  lv_obj_set_grid_dsc_array(page, columns.data(), rows.data());
  lv_font_t icon_font = lv_font_montserrat_14;
  icon_font.line_height = 26;
  lv_font_t title_font = lv_font_montserrat_14;
  title_font.line_height = 20;
  lv_font_t artist_font = lv_font_montserrat_14;
  artist_font.line_height = 18;
  std::array<PageBasicWidgets, Slots> widgets{};
  for (auto& w : widgets) {
    w.button = lv_button_create(page);
    lv_obj_set_style_pad_all(w.button, 8, 0);
    w.forecast = lv_obj_create(w.button);
    w.icon = lv_label_create(w.button);
    lv_obj_align(w.icon, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(w.icon, &icon_font, 0);
    w.label = lv_label_create(w.button);
    lv_obj_align(w.label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    w.sensor_container = lv_obj_create(w.button);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
  }
  PageBasicTheme theme;
  theme.media_title_font = &lv_font_montserrat_14;
  theme.label_font = &lv_font_montserrat_14;
  theme.cover_title_font = &title_font;
  theme.cover_artist_font = &artist_font;
  theme.media_control_title_font = &title_font;
  theme.media_control_artist_font = &artist_font;
  theme.icon_font = &icon_font;
  theme.media_text_scale_x = 280;
  theme.translate = [](const char* key) -> const char* {
    return std::strcmp(key, "Source") == 0 ? "SRC" : std::strcmp(key, "Idle") == 0 ? "IDLE" : key;
  };
  Actions<Slots> actions;
  auto callbacks = actions.callbacks();
  callbacks.media_control = Actions<Slots>::action;
  {
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     callbacks, nullptr, &media);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks, nullptr, &media, &view);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[index++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(media.prepare(fixture.store) && renderer.prepare(fixture.store) && state.prepare(fixture.store));
    media.activate_prepared(fixture.store);
    renderer.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(lv_obj_has_flag(widgets[1].forecast, LV_OBJ_FLAG_HIDDEN));
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "media_content_id", "track:1");
    subscriptions.send("media_player.room", "media_title", "A &amp; B");
    subscriptions.send("media_player.room", "media_artist", "ART");
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(std::strcmp(lv_label_get_text(widgets[1].icon), "A & B") == 0);
    assert(!lv_obj_has_flag(widgets[1].icon,LV_OBJ_FLAG_HIDDEN)&&!lv_obj_has_flag(widgets[1].label,LV_OBJ_FLAG_HIDDEN));
    now_playing_geometry(widgets[1], theme, false, true, "A & B", "ART", 1, 1, true);
    assert(lv_obj_has_flag(widgets[2].icon, LV_OBJ_FLAG_HIDDEN) &&
           lv_obj_has_flag(widgets[2].label, LV_OBJ_FLAG_HIDDEN));
    click(widgets[1].button);
    assert(actions.commands == 1);
    view.available = true;
    ++view.changes;
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(!lv_obj_has_flag(widgets[1].forecast, LV_OBJ_FLAG_HIDDEN));
    assert((lv_color_to_u32(lv_obj_get_style_text_color(widgets[1].icon, LV_PART_MAIN)) & 0xffffff) ==
           0xffffff);
    lv_refr_now(lv_display_get_default());
    // A completion delivered by a draw-task callback supersedes this frame.
    // Its old image may already be queued; its tint must not use the new asset.
    struct DrawReplacement { View* view; bool changed{false}; size_t images{0},tints{0}; } draw{&view};
    const auto replace=[](lv_event_t* event){
      auto& draw=*static_cast<DrawReplacement*>(lv_event_get_user_data(event));
      const auto type=lv_draw_task_get_type(lv_event_get_draw_task(event));
      if(type==LV_DRAW_TASK_TYPE_IMAGE){
        ++draw.images;
        if(!draw.changed){draw.changed=true;++draw.view->asset;++draw.view->changes;}
      } else if(type==LV_DRAW_TASK_TYPE_FILL)++draw.tints;
    };
    lv_obj_add_flag(widgets[1].forecast,LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    assert(lv_obj_add_event_cb(widgets[1].forecast,replace,LV_EVENT_DRAW_TASK_ADDED,&draw));
    lv_obj_invalidate(widgets[1].forecast);lv_refr_now(lv_display_get_default());
    assert(draw.changed&&draw.images>0&&draw.tints==0);
    renderer.update_visible();lv_obj_invalidate(widgets[1].forecast);lv_refr_now(lv_display_get_default());
    assert(draw.tints>0);
    lv_obj_remove_event_cb_with_user_data(widgets[1].forecast,replace,&draw);
    lv_obj_clear_flag(widgets[1].forecast,LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    for (bool compact_portrait : {false, true}) {
      theme.compact_portrait = compact_portrait;
      ++theme.revision;
      PageCellLayout large{};
      large.visible = true;
      large.card = renderer.binding(1).card;
      large.column_span = 2;
      large.row_span = 2;
      port.bind(1, fixture.store, large, {}, renderer.binding(1));
      renderer.refresh();
      renderer.update_visible();
      lv_obj_update_layout(page);
      auto expected = theme;
      expected.media_title_font = compact_portrait ? &title_font : &artist_font;
      expected.label_font = compact_portrait ? &artist_font : &lv_font_montserrat_14;
      now_playing_geometry(widgets[1], expected, false, true, "A & B", "ART", 2, 2, true);
    }
    const auto child = fixture.store.card(0)->target;
    assert(renderer.open_page(child));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    subscriptions.send("media_player.room", "source", "TV &amp; Radio");  // Not an external input.
    renderer.update_visible();
    subscriptions.send("media_player.room", "source", "TV");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[2].icon), "TV") == 0 &&
           std::strcmp(lv_label_get_text(widgets[2].label), "SRC") == 0);
    view.available = false;
    ++view.changes;
    renderer.update_visible();
    assert(lv_obj_has_flag(widgets[1].forecast, LV_OBJ_FLAG_HIDDEN));
    assert((lv_color_to_u32(lv_obj_get_style_text_color(widgets[1].icon, LV_PART_MAIN)) & 0xffffff) ==
           theme.text);
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "media_content_id", "");
    subscriptions.send("media_player.room", "media_title", "");
    subscriptions.send("media_player.room", "media_artist", "");
    subscriptions.send("media_player.room", "entity_picture", "");
    subscriptions.send("media_player.room", "entity_picture_local", "");
    subscriptions.send("media_player.room", "source", "");
    subscriptions.send("media_player.room", "", "idle");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "IDLE") == 0 &&
           !lv_obj_has_flag(widgets[1].icon, LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_style_text_font(widgets[1].icon, LV_PART_MAIN) == &icon_font);
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "media_title", "Back");
    subscriptions.send("media_player.room", "media_artist", "ART");
    view.available = true;
    ++view.changes;
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Child") == 0 &&
           lv_obj_get_style_text_font(widgets[1].icon, LV_PART_MAIN) == &icon_font);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, reservations = arena.allocations, retained = lv_live_bytes,
               descriptors = cache.memory().descriptor_adds, channels = subscriptions.bindings.size();
    for (size_t n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reservations && lv_live_bytes == retained &&
             cache.memory().descriptor_adds == descriptors && subscriptions.bindings.size() == channels);
    }
    renderer.retire_active();
    for (const auto& token : view.tokens) assert(!token.binding);
    state.retire_active();
    media.retire_active();
    lv_refr_now(lv_display_get_default());
    std::printf("%s: cover artwork, reusable image surface/labels, 1000 cycles, stable retained resources\n",
                limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}
template <size_t Slots>
void now_playing_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, false, false, false, false, false, false, false, false, true);
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  struct Transport : PageMediaTransport {
    Subscriptions& subscriptions;
    uint32_t now = 100;
    explicit Transport(Subscriptions& source) : subscriptions(source) {}
    bool connected() const override {
      return true;
    }
    uint32_t connection_generation() const override {
      return subscriptions.generation;
    }
    uint32_t now_ms() const override {
      return now;
    }
    int64_t epoch_seconds() const override {
      return 1000;
    }
    void reannounce() override {
      ++requests;
    }
    size_t requests = 0;
  } transport(subscriptions);
  PageMediaStateRuntime media(arena, transport);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &media);
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  const auto row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  for (size_t i = 0; i < row_count; ++i) rows[i] = LV_GRID_FR(1);
  rows[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page, 480, 480);
  lv_obj_set_grid_dsc_array(page, columns.data(), rows.data());
  std::array<PageBasicWidgets, Slots> widgets{};
  lv_font_t tall_title = lv_font_montserrat_14;
  tall_title.line_height = 20;
  lv_font_t icon_font = lv_font_montserrat_14;
  icon_font.line_height = 26;
  for (auto& w : widgets) {
    w.button = lv_button_create(page);
    lv_obj_set_style_pad_all(w.button, 8, LV_PART_MAIN);
    w.fill = lv_obj_create(w.button);
    w.icon = lv_label_create(w.button);
    lv_obj_align(w.icon, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(w.icon, &icon_font, LV_PART_MAIN);
    lv_obj_set_style_transform_scale_x(w.icon, 288, LV_PART_MAIN);
    w.label = lv_label_create(w.button);
    lv_obj_align(w.label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_transform_scale_x(w.label, 280, LV_PART_MAIN);
    w.sensor_container = lv_obj_create(w.button);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
    w.slider = lv_slider_create(w.button);
  }
  PageBasicTheme theme;
  theme.media_title_font = &lv_font_montserrat_14;
  theme.label_font = &lv_font_montserrat_14;
  theme.media_text_scale_x = 280;
  theme.translate = [](const char* key) -> const char* {
    return std::strcmp(key, "Source") == 0 ? "SRC" : key;
  };
  Actions<Slots> actions;
  auto media_callbacks=actions.callbacks();
  media_callbacks.icon=[](void* owner,std::string_view name,std::string_view domain)->const char* {
    return name=="Unused Glyph"?nullptr:Actions<Slots>::icon(owner,name,domain);
  };
  {
    auto invalid_theme = theme;
    invalid_theme.media_title_font = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns},
                                     invalid_theme, actions.callbacks(), nullptr, &media);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  for (int missing_action = 0; missing_action < 2; ++missing_action) {
    auto callbacks = actions.callbacks();
    if (missing_action)
      callbacks.media = nullptr;
    else
      callbacks.seek = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     callbacks, nullptr, &media);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  media_callbacks, nullptr, &media);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[index++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(media.prepare(fixture.store) && renderer.prepare(fixture.store) && state.prepare(fixture.store));
    media.activate_prepared(fixture.store);
    renderer.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "media_content_id", "provider:track:1");
    subscriptions.send("media_player.room", "media_title", u8"Ｓａｄｅ 🌧️");
    subscriptions.send("media_player.room", "media_artist", "ART");
    subscriptions.send("media_player.room", "media_duration", "300");
    subscriptions.send("media_player.room", "media_position", "60");
    subscriptions.send("sensor.temp", "", "21.4");
    renderer.update_visible();
    lv_obj_update_layout(page);
    lv_refr_now(lv_display_get_default());
    for (size_t slot = 1; slot <= 3; ++slot) {
      assert(std::strcmp(lv_label_get_text(widgets[slot].icon), "Sade") == 0);
      assert(std::strcmp(lv_label_get_text(widgets[slot].label), "ART") == 0);
      now_playing_geometry(widgets[slot], theme, slot == 3, slot == 2, "Sade", "ART");
    }
    assert(!lv_obj_has_flag(widgets[1].button, LV_OBJ_FLAG_CLICKABLE) &&
           lv_obj_has_flag(widgets[2].button, LV_OBJ_FLAG_CLICKABLE) &&
           !lv_obj_has_flag(widgets[3].button, LV_OBJ_FLAG_CLICKABLE));
    click(widgets[1].button);
    assert(actions.commands == 0);
    click(widgets[2].button);
    assert(actions.commands == 1);
    PointerInput input;
    auto* device = lv_indev_create();
    lv_indev_set_type(device, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(device, lv_display_get_default());
    lv_indev_set_user_data(device, &input);
    lv_indev_set_read_cb(device, PointerInput::read);
    lv_timer_pause(lv_indev_get_read_timer(device));
    pointer(device, input, widgets[3].slider, 90, true);
    pointer(device, input, widgets[3].slider, 90, false);
    assert(actions.seeks == 1 && actions.seek_percent >= 88 &&
           std::strcmp(lv_label_get_text(widgets[3].icon), "Sade") == 0);
    pointer(device, input, widgets[3].slider, 50, true);
    ++subscriptions.generation;
    renderer.update_visible();
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "media_duration", "300");
    subscriptions.send("media_player.room", "media_position", "60");
    subscriptions.send("media_player.room", "media_title", u8"Ｓａｄｅ 🌧️");
    subscriptions.send("media_player.room", "media_artist", "ART");
    renderer.update_visible();
    lv_indev_read(device);
    pointer(device, input, widgets[3].slider, 50, false);
    assert(actions.seeks == 1);
    pointer(device, input, widgets[3].slider, 70, true);
    assert(renderer.suspend() && renderer.resume());
    renderer.update_visible();
    lv_indev_read(device);
    pointer(device, input, widgets[3].slider, 70, false);
    assert(actions.seeks == 1);
    actions.seek_page = fixture.store.card(0)->target;
    pointer(device, input, widgets[3].slider, 30, true);
    pointer(device, input, widgets[3].slider, 30, false);
    actions.seek_page = NO_PAGE;
    assert(actions.seeks == 2 && !renderer.valid(actions.last));
    assert(renderer.back());
    renderer.update_visible();
    subscriptions.send("media_player.room", "media_artist", "");
    subscriptions.send("media_player.room", "source", "TV");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "SRC") == 0 &&
           !lv_obj_has_flag(widgets[1].label, LV_OBJ_FLAG_HIDDEN));
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "media_title", u8"Ｓａｄｅ 🌧️");
    renderer.update_visible();
    assert(lv_obj_has_flag(widgets[1].label, LV_OBJ_FLAG_HIDDEN));
    const auto child = fixture.store.card(0)->target;
    lv_obj_send_event(widgets[2].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_send_event(widgets[2].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 1);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Child") == 0 &&
           !lv_obj_has_flag(widgets[1].label, LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_style_text_font(widgets[1].icon, LV_PART_MAIN) == &icon_font &&
           lv_obj_get_style_transform_scale_x(widgets[1].icon, LV_PART_MAIN) == 288);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    lv_indev_reset(device, nullptr);
    subscriptions.send("media_player.room", "media_artist", "ART");
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(renderer.open_page(child));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const std::string long_title =
        "A long title with many words that wraps into several lines when the card is narrow";
    subscriptions.send("media_player.room", "media_title", long_title);
    subscriptions.send("media_player.room", "media_artist", "ART");
    theme.media_title_font = &tall_title;
    theme.media_text_scale_x = 320;
    ++theme.revision;
    renderer.update_visible();
    lv_obj_set_size(page, 600, 320);
    lv_obj_update_layout(page);
    assert(port.state_changed());
    renderer.update_visible();
    lv_obj_update_layout(page);
    for (size_t slot = 1; slot <= 3; ++slot)
      now_playing_geometry(widgets[slot], theme, slot == 3, slot == 2, long_title, "ART");
    // The prepared port also supports multi-row titles without a line limit.
    PageCellLayout large{};
    large.visible = true;
    large.card = renderer.binding(1).card;
    large.column = 0;
    large.row = 0;
    large.column_span = 1;
    large.row_span = 2;
    port.bind(1, fixture.store, large, {}, renderer.binding(1));
    renderer.refresh();
    renderer.update_visible();
    lv_obj_update_layout(page);
    now_playing_geometry(widgets[1], theme, false, false, long_title, "ART", 2);
    assert(renderer.open_page(child));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_set_size(page, 480, 480);
    lv_obj_update_layout(page);
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, reservations = arena.allocations,
               descriptors = cache.memory().descriptor_adds, retained = lv_live_bytes,
               channels = subscriptions.bindings.size();
    for (size_t n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reservations &&
             cache.memory().descriptor_adds == descriptors && subscriptions.bindings.size() == channels &&
             lv_live_bytes == retained);
    }
    ++subscriptions.generation;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].icon), "--") == 0 &&
           lv_obj_has_flag(widgets[1].label, LV_OBJ_FLAG_HIDDEN) &&
           !lv_obj_has_state(widgets[2].button, LV_STATE_CHECKED));
    // Replace a large-title arena with an ordinary one, then return. No label
    // may retain the released title storage or its media-specific geometry.
    Fixture ordinary(limits);
    PageStore replacement;
    assert(PageStore::stage_v1(ordinary.document.data(), ordinary.size, limits, 12, ordinary.arena.data(),
                               ordinary.arena.size(), &replacement) == PageStoreStatus::OK);
    assert(media.prepare(replacement) && renderer.prepare(replacement) && state.prepare(replacement));
    renderer.activate_prepared(replacement);
    media.activate_prepared(replacement);
    state.activate_prepared(replacement);
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(lv_obj_get_style_text_font(widgets[1].icon, LV_PART_MAIN) == &icon_font);
    PageStore returned;
    assert(PageStore::stage_v1(fixture.document.data(), fixture.size, limits, 13, fixture.arena.data(),
                               fixture.arena.size(), &returned) == PageStoreStatus::OK);
    assert(media.prepare(returned) && renderer.prepare(returned) && state.prepare(returned));
    renderer.activate_prepared(returned);
    media.activate_prepared(returned);
    state.activate_prepared(returned);
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "media_title", "Back");
    subscriptions.send("media_player.room", "media_artist", "ART");
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(std::strcmp(lv_label_get_text(widgets[1].icon), "Back") == 0);
    lv_indev_delete(device);
    renderer.retire_active();
    state.retire_active();
    media.retire_active();
    std::printf("%s: Now Playing, layout/label reuse/touch, 1000 cycles, stable retained resources\n",
                limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}

struct ForecastRequests:PageForecastRequests {
  void *owner=nullptr;Receive receive=nullptr;PageForecastToken token;uint32_t now=100,connection=1;bool online=true;size_t starts=0;
  bool connected()const override{return online;}uint32_t connection_generation()const override{return connection;}uint32_t now_ms()const override{return now;}
  bool start(void *pointer,PageForecastToken next,std::string_view,Receive callback)override{++starts;owner=pointer;token=next;receive=callback;return true;}
  void cancel(void *,PageForecastToken)override{receive=nullptr;}
  void reply(){auto callback=receive;receive=nullptr;assert(callback);callback(owner,token,"WF1|2026-10-17|2026-10-18|°C|mm|km/h\n2026-10-17|sunny|20|10|0|0|1|50\n2026-10-18|rainy|18|9|20|1|2|75\n2026-10-19|cloudy|21|9|20|1|2|75\n",true);}
};
static std::array<uint16_t,480*480>framebuffer{};
void forecast_pixel_compare(const PageBasicWidgets &widgets,PageForecastSnapshot snapshot,weather::View view,const PageBasicTheme &theme) {
  lv_refr_now(lv_display_get_default());const auto expected=framebuffer;
  lv_obj_add_flag(widgets.forecast,LV_OBJ_FLAG_HIDDEN);
  forecast_reference::WeatherDailySource source;source.forecast=snapshot.forecast;source.loading=snapshot.loading;source.stale=snapshot.stale;
  forecast_reference::WeatherDailyCard reference;reference.owner=widgets.button;reference.source=&source;reference.view=view;reference.caption=widgets.label;reference.placeholder_icon=widgets.icon;
  reference.font=lv_obj_get_style_text_font(widgets.label,LV_PART_MAIN);reference.temperature_font=theme.forecast_small_font?theme.forecast_small_font:reference.font;reference.icon_font=theme.forecast_icon_font?theme.forecast_icon_font:lv_obj_get_style_text_font(widgets.icon,LV_PART_MAIN);
  reference.root=lv_obj_create(widgets.button);lv_obj_set_style_bg_opa(reference.root,LV_OPA_TRANSP,LV_PART_MAIN);lv_obj_set_style_border_width(reference.root,0,LV_PART_MAIN);lv_obj_set_style_pad_all(reference.root,0,LV_PART_MAIN);lv_obj_clear_flag(reference.root,LV_OBJ_FLAG_CLICKABLE);lv_obj_clear_flag(reference.root,LV_OBJ_FLAG_SCROLLABLE);
  forecast_reference::scale_x=theme.forecast_scale_x;forecast_reference::scale_y=theme.forecast_scale_y;forecast_reference::translate=theme.translate;forecast_reference::convert=theme.forecast_temperature;
  forecast_reference::weather_daily_render_card(&reference);lv_obj_update_layout(widgets.button);lv_obj_invalidate(widgets.button);lv_refr_now(lv_display_get_default());
  size_t mismatches=0;for(size_t pixel=0;pixel<framebuffer.size();++pixel)if(framebuffer[pixel]!=expected[pixel])++mismatches;
  if(mismatches)std::fprintf(stderr,"Forecast pixel mismatch: %zu pixels, scale %d/%d, view %u\n",mismatches,theme.forecast_scale_x,theme.forecast_scale_y,unsigned(view));
  assert(mismatches==0);lv_obj_delete(reference.root);lv_obj_clear_flag(widgets.forecast,LV_OBJ_FLAG_HIDDEN);lv_obj_invalidate(widgets.button);
}
template<size_t Slots>void forecast_profile(const DeviceUiLimits &limits) {
  Fixture fixture(limits,false,false,false,false,false,true);Arena arena;Subscriptions subscriptions;PageCardDependencies dependencies;PageEntityStateRuntime state(dependencies,arena,subscriptions,{128,128,8192});ForecastRequests requests;PageForecastRuntime forecasts(arena,requests);auto *page=lv_obj_create(lv_screen_active());std::array<lv_coord_t,Slots+1>columns{},rows{};const auto row_count=(Slots+limits.grid_columns-1)/limits.grid_columns;
  for(size_t i=0;i<limits.grid_columns;++i)columns[i]=LV_GRID_FR(1);columns[limits.grid_columns]=LV_GRID_TEMPLATE_LAST;for(size_t i=0;i<row_count;++i)rows[i]=LV_GRID_FR(1);rows[row_count]=LV_GRID_TEMPLATE_LAST;lv_obj_set_size(page,480,480);lv_obj_set_grid_dsc_array(page,columns.data(),rows.data());std::array<PageBasicWidgets,Slots>widgets{};
  for(auto &w:widgets){w.button=lv_button_create(page);w.forecast=lv_obj_create(w.button);w.icon=lv_label_create(w.button);w.label=lv_label_create(w.button);w.sensor_container=lv_obj_create(w.button);w.value=lv_label_create(w.sensor_container);w.unit=lv_label_create(w.sensor_container);}
  PageBasicTheme theme;theme.forecast_small_font=&lv_font_montserrat_14;theme.forecast_icon_font=&lv_font_montserrat_14;Actions<Slots> actions;
  {PageBasicLvglPort<Slots>missing(arena,state,widgets,{limits.slot_count,limits.grid_columns},theme,actions.callbacks());assert(missing.initialize()&&!missing.prepare(fixture.store,Slots));}
  {PageLabelRefresh cache(arena);PageBasicLvglPort<Slots>port(arena,state,widgets,{limits.slot_count,limits.grid_columns},theme,actions.callbacks(),&forecasts);assert(port.initialize());std::array<lv_obj_t *,Slots*4>labels{};size_t index=0;for(const auto &w:widgets)for(auto *label:{w.icon,w.label,w.value,w.unit})labels[index++]=label;assert(cache.initialize(labels.data(),labels.size()));PageRendererRuntime<Slots>renderer(limits,arena,port);actions.renderer=&renderer;assert(renderer.prepare(fixture.store)&&state.prepare(fixture.store)&&forecasts.prepare(fixture.store));renderer.activate_prepared(fixture.store);state.activate_prepared(fixture.store);forecasts.activate_prepared(fixture.store);renderer.update_visible();lv_obj_update_layout(page);assert(std::strcmp(lv_label_get_text(widgets[1].label),"Loading")==0&&lv_obj_has_flag(widgets[1].forecast,LV_OBJ_FLAG_HIDDEN));requests.now+=PageForecastRuntime::READY_DELAY_MS;forecasts.loop();requests.reply();renderer.update_visible();lv_obj_update_layout(page);lv_refr_now(lv_display_get_default());assert(lv_obj_has_flag(widgets[1].icon,LV_OBJ_FLAG_HIDDEN)&&!lv_obj_has_flag(widgets[1].forecast,LV_OBJ_FLAG_HIDDEN)&&lv_obj_get_child_count(widgets[1].forecast)==0);assert(port.memory().forecast_frames==2);click(widgets[1].button);assert(actions.commands==1);
    PageForecastSnapshot pixels;assert(forecasts.snapshot({11,1},"weather.home",pixels));forecast_pixel_compare(widgets[1],pixels,weather::View::DAILY,theme);++theme.revision;renderer.update_visible();
    theme.forecast_scale_x=280;++theme.revision;renderer.update_visible();lv_obj_update_layout(page);forecast_pixel_compare(widgets[1],pixels,weather::View::DAILY,theme);theme.forecast_scale_x=256;theme.forecast_scale_y=280;++theme.revision;renderer.update_visible();lv_obj_update_layout(page);forecast_pixel_compare(widgets[1],pixels,weather::View::DAILY,theme);theme.forecast_scale_y=256;++theme.revision;renderer.update_visible();
    lv_font_t large_icon=lv_font_montserrat_14;large_icon.line_height=128;theme.forecast_icon_font=&large_icon;theme.forecast_scale_x=280;++theme.revision;renderer.update_visible();lv_obj_update_layout(page);forecast_pixel_compare(widgets[1],pixels,weather::View::DAILY,theme);theme.forecast_icon_font=&lv_font_montserrat_14;theme.forecast_scale_x=256;++theme.revision;renderer.update_visible();
    const auto child=fixture.store.card(0)->target;assert(renderer.open_page(child));renderer.update_visible();lv_obj_update_layout(page);assert(renderer.back());renderer.update_visible();lv_obj_update_layout(page);
    const auto cpp=cpp_allocations,reservations=arena.allocations,descriptors=cache.memory().descriptor_adds,retained=lv_live_bytes,resolves=actions.icon_resolves,starts=requests.starts;
    for(size_t n=0;n<1000;++n){assert(renderer.open_page(child));renderer.update_visible();lv_obj_update_layout(page);assert(renderer.back());renderer.update_visible();lv_obj_update_layout(page);assert(cpp_allocations==cpp&&arena.allocations==reservations&&cache.memory().descriptor_adds==descriptors&&lv_live_bytes==retained&&actions.icon_resolves==resolves&&requests.starts==starts);}
    requests.online=false;++requests.connection;forecasts.loop();renderer.update_visible();assert(std::strcmp(lv_label_get_text(widgets[1].label),"Last forecast")==0);lv_obj_set_size(page,600,320);lv_obj_update_layout(page);lv_refr_now(lv_display_get_default());renderer.retire_active();state.retire_active();forecasts.retire_active();forecasts.loop();std::printf("%s: forecast 1000 cycles, fixed frames/canvases, 0 C++/arena/label/glyph/request additions\n",limits.device_profile);
  }assert(arena.live==0);lv_obj_delete(page);
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
int main() {
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  lv_init();
  auto* display = lv_display_create(480, 480);
  assert(display);
  static std::array<uint16_t, 480 * 32> draw_buffer{};
  lv_display_set_buffers(display, draw_buffer.data(), nullptr, sizeof(draw_buffer),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, [](lv_display_t* d, const lv_area_t* area, uint8_t* data) {
    const auto* pixels = reinterpret_cast<uint16_t*>(data);
    size_t index = 0;
    for (int y = area->y1; y <= area->y2; ++y)
      for (int x = area->x1; x <= area->x2; ++x) {
        if (x >= 0 && x < 480 && y >= 0 && y < 480) framebuffer[y * 480 + x] = pixels[index];
        ++index;
      }
    lv_display_flush_ready(d);
  });
  for (const auto& limits : DEVICE_UI_PROFILES) {
    const char* filter = std::getenv("ESPCONTROL_TEST_DEVICE");
    if (filter && std::strcmp(filter, limits.device_profile) != 0) continue;
    const char* family = std::getenv("ESPCONTROL_TEST_FAMILY");
    if (family && std::strcmp(family, "percent_sliders") == 0) {
      switch (limits.slot_count) {
        case 6:
          percent_slider_profile<6>(limits);
          break;
        case 9:
          percent_slider_profile<9>(limits);
          break;
        case 15:
          percent_slider_profile<15>(limits);
          break;
        case 20:
          percent_slider_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "fan_status") == 0) {
      switch (limits.slot_count) {
        case 6:
          fan_status_profile<6>(limits);
          percent_slider_profile<6>(limits);
          break;
        case 9:
          fan_status_profile<9>(limits);
          percent_slider_profile<9>(limits);
          break;
        case 15:
          fan_status_profile<15>(limits);
          percent_slider_profile<15>(limits);
          break;
        case 20:
          fan_status_profile<20>(limits);
          percent_slider_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "local_relays") == 0) {
      switch (limits.slot_count) {
        case 6:
          local_relay_profile<6>(limits);
          break;
        case 9:
          local_relay_profile<9>(limits);
          break;
        case 15:
          local_relay_profile<15>(limits);
          break;
        case 20:
          local_relay_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "local_sensors") == 0) {
      switch (limits.slot_count) {
        case 6:
          local_sensor_profile<6>(limits);
          local_relay_profile<6>(limits);
          break;
        case 9:
          local_sensor_profile<9>(limits);
          local_relay_profile<9>(limits);
          break;
        case 15:
          local_sensor_profile<15>(limits);
          local_relay_profile<15>(limits);
          break;
        case 20:
          local_sensor_profile<20>(limits);
          local_relay_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "option_select") == 0) {
      switch (limits.slot_count) {
        case 6:
          option_select_profile<6>(limits);
          local_sensor_profile<6>(limits);
          local_relay_profile<6>(limits);
          break;
        case 9:
          option_select_profile<9>(limits);
          local_sensor_profile<9>(limits);
          local_relay_profile<9>(limits);
          break;
        case 15:
          option_select_profile<15>(limits);
          local_sensor_profile<15>(limits);
          local_relay_profile<15>(limits);
          break;
        case 20:
          option_select_profile<20>(limits);
          local_sensor_profile<20>(limits);
          local_relay_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "fan_modes") == 0) {
      switch (limits.slot_count) {
        case 6:
          fan_mode_profile<6>(limits);
          fan_status_profile<6>(limits);
          percent_slider_profile<6>(limits);
          option_select_profile<6>(limits);
          local_sensor_profile<6>(limits);
          local_relay_profile<6>(limits);
          break;
        case 9:
          fan_mode_profile<9>(limits);
          fan_status_profile<9>(limits);
          percent_slider_profile<9>(limits);
          option_select_profile<9>(limits);
          local_sensor_profile<9>(limits);
          local_relay_profile<9>(limits);
          break;
        case 15:
          fan_mode_profile<15>(limits);
          fan_status_profile<15>(limits);
          percent_slider_profile<15>(limits);
          option_select_profile<15>(limits);
          local_sensor_profile<15>(limits);
          local_relay_profile<15>(limits);
          break;
        case 20:
          fan_mode_profile<20>(limits);
          fan_status_profile<20>(limits);
          percent_slider_profile<20>(limits);
          option_select_profile<20>(limits);
          local_sensor_profile<20>(limits);
          local_relay_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "controls") == 0) {
      switch (limits.slot_count) {
        case 6: control_profile<6>(limits); break;
        case 9: control_profile<9>(limits); break;
        case 15: control_profile<15>(limits); break;
        case 20: control_profile<20>(limits); break;
        default: assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "media_controls") == 0) {
      switch (limits.slot_count) {
        case 6: media_control_profile<6>(limits); break;
        case 9: media_control_profile<9>(limits); break;
        case 15: media_control_profile<15>(limits); break;
        case 20: media_control_profile<20>(limits); break;
        default: assert(false);
      }
      continue;
    }
    switch (limits.slot_count) {
      case 6:
        profile<6>(limits);
        status_action_profile<6>(limits);
        control_profile<6>(limits);
        fan_mode_profile<6>(limits);
        fan_status_profile<6>(limits);
        percent_slider_profile<6>(limits);
        option_select_profile<6>(limits);
        local_sensor_profile<6>(limits);
        local_relay_profile<6>(limits);
        weather_profile<6>(limits);
        forecast_profile<6>(limits);
        media_action_profile<6>(limits);
        media_position_profile<6>(limits);
        now_playing_profile<6>(limits);
        cover_art_profile<6>(limits);
        media_control_profile<6>(limits);
        break;
      case 9:
        profile<9>(limits);
        status_action_profile<9>(limits);
        control_profile<9>(limits);
        fan_mode_profile<9>(limits);
        fan_status_profile<9>(limits);
        percent_slider_profile<9>(limits);
        option_select_profile<9>(limits);
        local_sensor_profile<9>(limits);
        local_relay_profile<9>(limits);
        weather_profile<9>(limits);
        forecast_profile<9>(limits);
        media_action_profile<9>(limits);
        media_position_profile<9>(limits);
        now_playing_profile<9>(limits);
        cover_art_profile<9>(limits);
        media_control_profile<9>(limits);
        break;
      case 15:
        profile<15>(limits);
        status_action_profile<15>(limits);
        control_profile<15>(limits);
        fan_mode_profile<15>(limits);
        fan_status_profile<15>(limits);
        percent_slider_profile<15>(limits);
        option_select_profile<15>(limits);
        local_sensor_profile<15>(limits);
        local_relay_profile<15>(limits);
        weather_profile<15>(limits);
        forecast_profile<15>(limits);
        media_action_profile<15>(limits);
        media_position_profile<15>(limits);
        now_playing_profile<15>(limits);
        cover_art_profile<15>(limits);
        media_control_profile<15>(limits);
        break;
      case 20:
        profile<20>(limits);
        status_action_profile<20>(limits);
        control_profile<20>(limits);
        fan_mode_profile<20>(limits);
        fan_status_profile<20>(limits);
        percent_slider_profile<20>(limits);
        option_select_profile<20>(limits);
        local_sensor_profile<20>(limits);
        local_relay_profile<20>(limits);
        weather_profile<20>(limits);
        forecast_profile<20>(limits);
        media_action_profile<20>(limits);
        media_position_profile<20>(limits);
        now_playing_profile<20>(limits);
        cover_art_profile<20>(limits);
        media_control_profile<20>(limits);
        break;
      default:
        std::abort();
    }
  }
  modal_gesture_cycles();
  lv_display_delete(display);
  lv_deinit();
  assert(lv_live_bytes == 0);
}
