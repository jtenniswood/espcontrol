#include "page_basic_lvgl_port.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "forecast_reference.h"
#include "now_playing_reference.h"
#include "page_card_dependencies.h"
#include "page_device_command_policy.h"
#include "page_entity_observer_set.h"
#include "page_grid_input.h"
#include "page_label_refresh.h"
#include "page_lvgl_dispatch.h"
#include "page_media_command_policy.h"
#include "page_modal_gestures.h"
#include "page_pooled_qr_tile_view.h"
#include "page_runtime_resources_group.h"
#include "page_slider_command_policy.h"
#include "page_timer_command_policy.h"
#include "panel_config_document.h"
#include "position_reference.h"
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
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>("button_order"), 12,
                                 reinterpret_cast<const uint8_t*>("1"), 1) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct DeviceCommands {};
  Fixture(const DeviceUiLimits& limits, DeviceCommands) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {
        "fan.room;Power;Auto;Auto;;;fan_switch;;",        "fan.room;Oscillate;Auto;Auto;;;fan_oscillate;;",
        "fan.room;Direction;Auto;Auto;;;fan_direction;;", "cover.room;Open;Auto;Auto;open;;cover;;",
        "cover.room;Cover;Auto;Auto;toggle;;cover;;",     ";Devices;Auto;Auto;;;subpage;;"};
    for (size_t i = 0; i < 6; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3,4,5|garage,cover.room,Garage,Auto,Auto,,,,|gate,cover.gate,Gate,Auto,Auto,open,,,|lock,"
        "lock.room,Lock,Auto,Auto,,,,|vacuum,vacuum.room,Vacuum,Auto,Auto,start_stop,,,|lawn_mower,lawn_"
        "mower.room,Mower,Auto,Auto,pause_resume,,,";
    assert(writer.append_subpage(6, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>("button_order"), 12,
                                 reinterpret_cast<const uint8_t*>("1,2,3,4,5,6"),
                                 11) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct DeferredMedia {};
  Fixture(const DeviceUiLimits& limits, DeferredMedia) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string rows[] = {"media_player.room;Play;Auto;Auto;play_pause;;media;;",
                                "media_player.room;Previous;Auto;Auto;previous;;media;;",
                                "media_player.room;Next;Auto;Auto;next;;media;;",
                                "media_player.room;Playlist;Auto;Auto;playlist;;media;;playlist_content_id="
                                "spotify%3Aplaylist%3Atest,playlist_player_source=Living%20Room",
                                "media_player.room;Position;Auto;Auto;position;;media;;",
                                ";Media;Auto;Auto;;;subpage;;"};
    for (size_t i = 0; i < 6; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(rows[i].data()), rows[i].size()) ==
             PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2|M,media_player.room,Progress,Auto,Auto,now_playing,,progress,|M,media_player.room,Play,Auto,"
        "Auto,now_playing,,play_pause,";
    assert(writer.append_subpage(6, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>("button_order"), 12,
                                 reinterpret_cast<const uint8_t*>("1,2,3,4,5,6"),
                                 11) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct ScreenLocks {};
  Fixture(const DeviceUiLimits& limits, ScreenLocks) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Lock page;Auto;Auto;;;subpage;;", ";Ignore;Ignore;Ignore;;;screen_lock;;",
                                 ";Ignore;Ignore;Ignore;;;screen_lock;;screensaver_on_lock"};
    for (size_t i = 0; i < 3; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2|screen_lock,,Ignore,Ignore,Ignore,,,,|screen_lock,,Ignore,Ignore,Ignore,,,,screensaver_on_"
        "lock";
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
  struct InputEpochs {};
  Fixture(const DeviceUiLimits& limits, InputEpochs) {
    PanelConfigWriter w(document.data(), document.size());
    assert(w.begin() == PanelConfigStatus::OK);
    assert(w.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                   std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Controls;Auto;Auto;indicator;;subpage;;",
                                 "switch.input;Switch;Auto;Auto;;;;;",
                                 "script.input;Run;Auto;Auto;script.turn_on;;action;;",
                                 "ble_keyboard;Local;Auto;Auto;local;;action;;",
                                 "https://example.test/hook;Hook;Auto;Auto;;;webhook;;",
                                 ";Saver;Auto;Auto;;;screensaver;;"};
    for (size_t i = 0; i < 6; ++i)
      assert(w.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()), cards[i].size()) ==
             PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3,4,5|light_control,light.input,Light,Auto,Auto,,,,|cover,cover.input,Cover,Auto,Auto,modal,,"
        ",|fan_control,fan.input,Fan,Auto,Auto,,,,|fan_direction,fan.input,Direction,Auto,Auto,,,,|option_"
        "select,select.input,Option,Auto,Auto,,,,";
    assert(w.append_subpage(1, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    const std::string key = "button_order", order = "1,2,3,4,5,6";
    assert(w.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                            reinterpret_cast<const uint8_t*>(order.data()),
                            order.size()) == PanelConfigStatus::OK);
    assert(w.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct Confirmations {};
  Fixture(const DeviceUiLimits& limits, Confirmations) {
    PanelConfigWriter w(document.data(), document.size());
    assert(w.begin() == PanelConfigStatus::OK);
    assert(w.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                   std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Confirm;Auto;Auto;indicator;;subpage;;",
                                 "switch.confirm;On;Auto;Auto;;;;;confirm_on",
                                 "switch.confirm;Off;Auto;Auto;;;;;confirm_off",
                                 "script.confirm;Script;Auto;Auto;script.turn_on;;action;;confirm_on",
                                 "cover.confirm;Open;Auto;Auto;open;;garage;;confirm_on",
                                 "cover.confirm;Garage;Auto;Auto;;;garage;;confirm_on,confirm_off"};
    for (size_t i = 0; i < 6; ++i)
      assert(w.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()), cards[i].size()) ==
             PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3|,switch.confirm,Child,Auto,Auto,,,,confirm_on%2Cconfirm_off|R,cover.confirm,Child "
        "Garage,Auto,Auto,,,,confirm_on%2Cconfirm_off|action,script.confirm,Child "
        "Script,Auto,Auto,script.turn_on,,,confirm_on";
    assert(w.append_subpage(1, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    const std::string key = "button_order", order = "1,2,3,4,5,6";
    assert(w.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                            reinterpret_cast<const uint8_t*>(order.data()),
                            order.size()) == PanelConfigStatus::OK);
    assert(w.finish(&size) == PanelConfigStatus::OK);
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
  struct AlarmCards {};
  Fixture(const DeviceUiLimits& limits, AlarmCards) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {
        ";Alarms;Home;Lightbulb;indicator;;subpage;;subpage_kind=alarm,pin_arm=0,pin_disarm=0,label_display="
        "name,icon_display=static",
        "alarm_control_panel.home;;Auto;Lightbulb;;;alarm;;",
        "alarm_control_panel.home;Configured;Power;Lightbulb;;;alarm;;label_display=name,icon_display=static",
        "alarm_control_panel.home;;Power;Lightbulb;away;;alarm_action;;pin_arm=0",
        "alarm_control_panel.home;;Auto;Auto;disarm;;alarm_action;;",
        ";Temperature;Auto;Auto;sensor.temperature;C;sensor;1;"};
    for (size_t i = 0; i < 6; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3|S,,Temperature,Auto,Auto,sensor.temperature,C,1,|Y,alarm_control_panel.home,,Home,"
        "Lightbulb,,,,label_display=name%2Cicon_display=static%2Cpin_arm=0%2Cpin_disarm=0|AA,alarm_control_"
        "panel.only,,Auto,Auto,home,,,";
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
  struct ClockCards {};
  Fixture(const DeviceUiLimits& limits, ClockCards) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Clocks;Auto;Auto;indicator;;subpage;;", ";Ignored;Power;Auto;;;clock;;",
                                 "Asia/Kathmandu;Ignored;Power;Auto;;;timezone;;",
                                 "Unknown/Long_City_With_Underscores;Ignored;Power;Auto;;;timezone;;"};
    for (size_t n = 0; n < 4; ++n)
      assert(writer.append_button(n + 1, reinterpret_cast<const uint8_t*>(cards[n].data()),
                                  cards[n].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3|S,,Temperature,Auto,Auto,sensor.temperature,C,1,|CK,,Ignored,Auto,Auto,,,,|T,Pacific/"
        "Chatham,Ignored,Auto,Auto,,,,";
    assert(writer.append_subpage(1, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    const std::string key = "button_order", order = limits.grid_columns >= 3 ? "1,2w,0,3,4" : "1,0,2w,0,3,4";
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                                 reinterpret_cast<const uint8_t*>(order.data()),
                                 order.size()) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct NumberSize {
    const char* order;
    const char* type;
    const char* options;
  };
  Fixture(const DeviceUiLimits& limits, NumberSize spec) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string type = spec.type;
    const std::string card = type == "action"
                                 ? std::string(
                                       "script.scene;Room;Auto;Auto;script.turn_on;;action;;state_entity="
                                       "sensor.temperature,state_precision=1,state_unit=C,") +
                                       spec.options
                                 : std::string("climate.room;Room;Auto;Auto;sensor.temperature;C;") +
                                       (type == "duration" ? "sensor;time;" : type + ";1;") + spec.options;
    assert(writer.append_button(1, reinterpret_cast<const uint8_t*>(card.data()), card.size()) ==
           PanelConfigStatus::OK);
    const std::string key = "button_order", order = spec.order;
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                                 reinterpret_cast<const uint8_t*>(order.data()),
                                 order.size()) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct ClockSize {
    const char* order;
    const char* type;
    const char* options;
  };
  Fixture(const DeviceUiLimits& limits, ClockSize spec) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string card =
        std::string("Pacific/Chatham;Ignored;Power;Auto;;;") + spec.type + ";;" + spec.options;
    assert(writer.append_button(1, reinterpret_cast<const uint8_t*>(card.data()), card.size()) ==
           PanelConfigStatus::OK);
    const std::string key = "button_order", order = spec.order;
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                                 reinterpret_cast<const uint8_t*>(order.data()),
                                 order.size()) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct CalendarCards {};
  Fixture(const DeviceUiLimits& limits, CalendarCards) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Dates;Auto;Auto;indicator;;subpage;;", ";Ignored;Power;Auto;;;calendar;;",
                                 ";Ignored;Power;Auto;;;calendar;datetime;",
                                 ";;Auto;Auto;sensor.temperature;C;sensor;1;"};
    for (size_t n = 0; n < 4; ++n)
      assert(writer.append_button(n + 1, reinterpret_cast<const uint8_t*>(cards[n].data()),
                                  cards[n].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3|D,sensor.other,Ignored,Auto,Auto,,,,|D,,Ignored,Auto,Auto,,,datetime,|S,,Temperature,Auto,"
        "Auto,sensor.temperature,C,1,";
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
  struct CalendarSize {
    const char* order;
    bool disabled;
  };
  Fixture(const DeviceUiLimits& limits, CalendarSize size_spec) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string card = std::string(";Ignored;Power;Auto;;;calendar;datetime;") +
                             (size_spec.disabled ? "large_numbers=off" : "large_numbers");
    assert(writer.append_button(1, reinterpret_cast<const uint8_t*>(card.data()), card.size()) ==
           PanelConfigStatus::OK);
    const std::string key = "button_order", order = size_spec.order;
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                                 reinterpret_cast<const uint8_t*>(order.data()),
                                 order.size()) == PanelConfigStatus::OK);
    assert(writer.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(document.data(), size, limits, 11, arena.data(), arena.size(), &store) ==
           PageStoreStatus::OK);
  }
  struct CleaningCards {};
  Fixture(const DeviceUiLimits& limits, CleaningCards) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Cleaning;Auto;Auto;indicator;;subpage;;",
                                 "vacuum.room;;Robot Vacuum Off;Power;start_stop;;vacuum;;",
                                 "vacuum.room;;Robot Vacuum Variant Off;Auto;status;;vacuum;;",
                                 "lawn_mower.room;;Lawnmower;Auto;pause_resume;;lawn_mower;;",
                                 "lawn_mower.room;;Power;Auto;status;;lawn_mower;;",
                                 "vacuum.room;Dock;Vacuum Outline;Auto;dock;;vacuum;;"};
    for (size_t n = 0; n < 6; ++n)
      assert(writer.append_button(n + 1, reinterpret_cast<const uint8_t*>(cards[n].data()),
                                  cards[n].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3,4,5|VC,vacuum.room,Start "
        "Dock,Auto,Auto,start_dock,,,|LM,lawn_mower.room,Dock,Auto,Auto,dock,,,|VC,vacuum.room,Area,"
        "Auto,Auto,clean_area,zone_1,,|S,,Sensor,Auto,Auto,sensor.temperature,C,1,|VC,vacuum.room,Locate,"
        "Auto,"
        "Auto,locate,,,";
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
  struct AccessCards {};
  Fixture(const DeviceUiLimits& limits, AccessCards) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Access;Auto;Auto;indicator;;subpage;;",
                                 "cover.garage;;Auto;Auto;;;garage;;",
                                 "cover.gate;;Auto;Auto;;;gate;;",
                                 "lock.room;;Auto;Auto;;;lock;;",
                                 "cover.garage;;Auto;Lightbulb;open;;garage;;label_display=status",
                                 "cover.gate;Gate Stop;Auto;Auto;stop;;gate;;"};
    for (size_t n = 0; n < 6; ++n)
      assert(writer.append_button(n + 1, reinterpret_cast<const uint8_t*>(cards[n].data()),
                                  cards[n].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3,4,5|R,cover.garage,,Auto,Auto,,,,|GT,cover.gate,,Auto,Auto,,,,|K,lock.room,,Auto,Auto,,,,|"
        "K,lock.room,,Auto,Auto,unlock,,,|R,cover.garage,,Auto,Auto,open,,,";
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
  struct CoverToggles {};
  Fixture(const DeviceUiLimits& limits, CoverToggles) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Modes;Auto;Auto;indicator;;subpage;;",
                                 "cover.room;;Auto;Auto;toggle;;cover;;",
                                 "cover.room;Configured;Lightbulb;Blinds Open;toggle;;cover;;"};
    for (size_t i = 0; i < 3; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2|C,cover.room,,Auto,Auto,toggle,,,|C,cover.room,Configured,Lightbulb,Blinds Open,toggle,,,";
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
  struct CoverCommands {};
  Fixture(const DeviceUiLimits& limits, CoverCommands) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {
        ";Commands;Auto;Auto;indicator;;subpage;;", "cover.room;Open;Auto;Other;open;;cover;;",
        "cover.room;Close;Auto;Other;close;;cover;;", "cover.room;Stop;Auto;Other;stop;;cover;;",
        "cover.room;Position;Auto;Other;set_position;67;cover;;"};
    for (size_t i = 0; i < 5; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3|C,cover.room,Open,Auto,Other,open,,,|C,cover.room,Stop,Auto,Other,stop,,,|S,,Temp,Auto,"
        "Auto,sensor.temp,°C,1,";
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
  struct Timers {};
  Fixture(const DeviceUiLimits& limits, Timers) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Timers;Auto;Auto;indicator;;subpage;;",
                                 "timer.room;;Unused;Unused;confirm;1;timer;;",
                                 "timer.room;Plain;Unused;Unused;;;timer;;", ";;;;sensor.temp;°C;sensor;1;"};
    for (size_t i = 0; i < 4; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3|S,,Temp,Auto,Auto,sensor.temp,°C,1,|TM,timer.room,Child,Unused,Unused,confirm,1,,|,light."
        "child,Child,Auto,Auto,,,,";
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
  struct LightTemperatures {};
  Fixture(const DeviceUiLimits& limits, LightTemperatures) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {";Temperatures;Auto;Auto;indicator;;subpage;;",
                                 "light.room;;Auto;Other;;5000-6000;light_temperature;color;",
                                 "light.room;Fixed;Auto;Other;;2000-6500;light_temperature;;",
                                 ";;;;sensor.temp;°C;sensor;1;", "fan.room;Fan;Auto;Auto;;;fan_speed;;"};
    for (size_t i = 0; i < 5; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3,4|,light.child,Child,Auto,Auto,,,,|N,light.room,Child "
        "temperature,Auto,Auto,,3000-7000,color,|S,,Temp,Auto,Auto,sensor.temp,°C,1,|J,fan.room,Fan,Auto,"
        "Auto,,,,";
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
  struct NumberSliders {};
  Fixture(const DeviceUiLimits& limits, NumberSliders) {
    PanelConfigWriter writer(document.data(), document.size());
    assert(writer.begin() == PanelConfigStatus::OK);
    assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                        std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string cards[] = {
        ";Levels;Auto;Auto;indicator;;subpage;;", "number.room;;Auto;Lightbulb;;;fan_speed;;",
        "input_number.room;Light;Auto;Auto;;;light_brightness;;", "number.other;Position;Auto;Auto;;;cover;;",
        "number.other;Tilt;Auto;Auto;tilt;;cover;;"};
    for (size_t i = 0; i < 5; ++i)
      assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                  cards[i].size()) == PanelConfigStatus::OK);
    const std::string child =
        "~B,1,2,3,4|J,number.room,Fan,Auto,Lightbulb,,,,|V,input_number.room,Light,Auto,Auto,,,,|C,number."
        "other,Position,"
        "Auto,Auto,,,,|J,fan.mixed,Fan,Auto,Auto,,,,";
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
  PageBasicLvglPort<Slots>* basic_port{nullptr};
  PageMediaSeekProof seek_proof;
  bool admitted{false};
  PageCardCommands* deferred{nullptr};
  PageEntityStateRuntime* command_state{nullptr};
  PageRendererRuntime<Slots> *renderer{nullptr};size_t toggles{0},climates{0},commands{0},icon_resolves{0},seeks{0};uint8_t seek_percent{0};float seek_duration{0};PageId seek_page{NO_PAGE};PageBindingToken last;char entity[64]{};
  static const char* icon(void* owner, std::string_view name, std::string_view) {
    ++static_cast<Actions*>(owner)->icon_resolves;
    return name == "Robot Vacuum"               ? "VAC_NORMAL"
           : name == "Robot Vacuum Alert"       ? "VAC_ALERT"
           : name == "Robot Vacuum Off"         ? "VAC_OFF"
           : name == "Robot Vacuum Variant"     ? "VAC_DOCK"
           : name == "Robot Vacuum Variant Off" ? "VAC_OFF"
           : name == "Robot Mower"              ? "MOWER"
           : name == "Robot Mower Outline"      ? "MOWER_OFF"
           : name == "Vacuum Outline"           ? "VAC_AREA"
           : name == "Power"                    ? "POWER"
           : name == "Garage"                   ? "GARAGE_CLOSED"
           : name == "Garage Open"              ? "GARAGE_OPEN"
           : name == "Gate"                     ? "GATE_CLOSED"
           : name == "Gate Open"                ? "GATE_OPEN"
           : name == "Lock"                     ? "LOCKED"
           : name == "Lock Open"                ? "UNLOCKED"
           : name == "Fan"                      ? "FAN"
           : name == "Weather Sunny"            ? "SUN"
           : name == "Lightbulb"                ? "ON"
           : name == "Blinds Open"              ? "OPEN"
           : name == "Blinds"                   ? "CLOSED"
                                                : "I";
  }
  static bool valid(void* owner, PageBindingToken t) {
    auto& actions = *static_cast<Actions*>(owner);
    return actions.renderer->valid(t) && (!actions.basic_port || actions.basic_port->input_enabled());
  }
  static void toggle(void *owner,PageBindingToken t,std::string_view entity){auto &a=*static_cast<Actions *>(owner);++a.toggles;a.last=t;assert(entity.size()<sizeof(a.entity));std::memcpy(a.entity,entity.data(),entity.size());a.entity[entity.size()]='\0';}
  static void open(void *owner,PageBindingToken,PageId page){assert(static_cast<Actions *>(owner)->renderer->open_page(page));}
  static void back(void *owner,PageBindingToken){assert(static_cast<Actions *>(owner)->renderer->back());}
  static void climate(void *owner,PageBindingToken token){auto &a=*static_cast<Actions *>(owner);++a.climates;a.last=token;}
  static void action(void *owner,PageBindingToken token){auto &a=*static_cast<Actions *>(owner);++a.commands;a.last=token;}
  static void media(void* owner, PageBindingToken token, uint32_t) { action(owner, token); }
  static void seek(void* owner, PageBindingToken token, PageMediaSeekProof proof, uint8_t percent,
                   float duration) {
    auto& a = *static_cast<Actions*>(owner);
    ++a.seeks;
    a.last = token;
    a.seek_percent = percent;
    a.seek_duration = duration;
    a.seek_proof = proof;
    // This legacy geometry fixture models an already accepted transport send.
    if (a.basic_port && a.basic_port->preview_seek(token, proof, percent, duration))
      a.renderer->update_visible();
    if (a.seek_page != NO_PAGE) assert(a.renderer->open_page(a.seek_page));
  }
  bool locked{false}, lock_screensaver{false};
  size_t locks{0};
  size_t confirmations{0};
  uint32_t confirmation_connection{0};
  bool confirmation_current{false};
  PageCoverCommandSnapshot cover_snapshot;
  PageAccessActionSnapshot access_snapshot;
  const char* cleaning_service{nullptr};
  PageTimerAction timer_action{PageTimerAction::NONE};
  bool timer_accept{true};
  double numeric_target{0};
  number_slider::Metadata numeric_metadata;
  size_t paint_reads{0};
  static bool bound(void* owner, PageBindingToken t) {
    auto& actions = *static_cast<Actions*>(owner);
    const bool result = actions.renderer->bound(t);
    if (result && !actions.renderer->valid(t)) ++actions.paint_reads;
    return result;
  }
  PageBasicActions callbacks() {
    auto result =
        PageBasicActions{this, icon, valid, toggle, open, back, climate, action, action, action, media, seek};
    result.bound = bound;
    result.screen_locked = [](void* owner) { return static_cast<Actions*>(owner)->locked; };
    result.screen_lock = [](void* owner, PageBindingToken token, bool saver) {
      auto& a = *static_cast<Actions*>(owner);
      ++a.locks;
      a.last = token;
      a.lock_screensaver = saver;
    };

    return result;
  }
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
    subscriptions.send("fan.room", "", "on");
    subscriptions.send("fan.room", "friendly_name", "Room Fan");
    subscriptions.send("fan.room", "", "off");
    subscriptions.send("light.room", "", "off");
    subscriptions.send("cover.room", "", "closed");
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    subscriptions.send("light.room", "", "on");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    subscriptions.send("light.room", "", "off");
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    subscriptions.send("cover.room", "", "open");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    subscriptions.send("cover.room", "", "closed");
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    subscriptions.send("cover.room", "", "unavailable");
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    subscriptions.send("fan.room", "", "on");
    subscriptions.send("light.room", "", "on");
    subscriptions.send("cover.room", "", "open");
    subscriptions.send("cover.room", "current_position", "25");
    renderer.update_visible();
    lv_obj_update_layout(page);
    lv_refr_now(lv_display_get_default());
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
void cover_toggle_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::CoverToggles{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageCoverToggleRuntime cover(arena, subscriptions, fan_status_now);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &cover);
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
  auto callbacks = actions.callbacks();
  callbacks.toggle = [](void* owner, PageBindingToken token, std::string_view entity) {
    auto& action = *static_cast<Actions<Slots>*>(owner);
    Actions<Slots>::toggle(owner, token, entity);
    if (action.seek_page != NO_PAGE) assert(action.renderer->open_page(action.seek_page));
  };
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                  nullptr, nullptr, nullptr, &cover);
    assert(port.initialize());
    {
      auto absent = callbacks;
      absent.toggle = nullptr;
      PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                       absent, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                       nullptr, nullptr, nullptr, &cover);
      assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
    }
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[index++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    PageRuntimeResourcesGroup<3> resources({&cover, &renderer, &state});
    PageStoreRuntime publication(limits, arena, 32768, &resources);
    assert(publication.prepare(PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(), fixture.size));
    assert(publication.activate_prepared(0, PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(),
                                         fixture.size));
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Cover") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[1].icon), "CLOSED") == 0);
    subscriptions.send("cover.room", "", "closed");
    subscriptions.send("cover.room", "friendly_name", "Bedroom");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED));
    assert(!lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Bedroom") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "Configured") == 0);
    subscriptions.send("cover.room", "", "opening");
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED));
    assert(lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Opening") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[1].icon), "OPEN") == 0);
    fan_status_tick += 10000;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Opening") == 0);
    subscriptions.send("cover.room", "", "open");
    renderer.update_visible();
    click(widgets[1].button);
    assert(actions.toggles == 1 && renderer.valid(actions.last));
    assert(lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED));  // Existing optimistic feedback.
    const auto child = fixture.store.card(0)->target;
    const auto old = renderer.binding(1);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.toggles == 1 && !renderer.valid(old));
    fan_status_tick += 3000;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Bedroom") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "Configured") == 0);
    subscriptions.send("cover.room", "friendly_name", "Latest");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Latest") == 0);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    ++subscriptions.generation;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "--") == 0);
    subscriptions.send("cover.room", "", "closed");
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.toggles == 1);
    click(widgets[1].button);
    assert(actions.toggles == 2);
    subscriptions.send("cover.room", "", std::string(10000, 'x'));
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Unavailable") == 0);
    click(widgets[1].button);
    assert(actions.toggles == 2);
    subscriptions.send("cover.room", "", "closed");
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.suspend() && renderer.resume());
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.toggles == 2);
    actions.seek_page = child;
    click(widgets[1].button);
    actions.seek_page = NO_PAGE;
    assert(actions.toggles == 3 && !renderer.valid(actions.last));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, reserved = arena.allocations, retained = lv_live_bytes,
               channels = subscriptions.bindings.size(), descriptors = cache.memory().descriptor_adds;
    for (int n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reserved && lv_live_bytes == retained &&
             subscriptions.bindings.size() == channels && cache.memory().descriptor_adds == descriptors);
    }
    renderer.retire_active();
    state.retire_active();
    cover.retire_active();
    std::printf("%s: cover toggles 1000 cycles, hidden history, stale input and stable resources\n",
                limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}

template <size_t Slots>
void access_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::AccessCards{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageAccessStateRuntime access(arena, subscriptions, fan_status_now);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &access);
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
  auto callbacks = actions.callbacks();
  callbacks.access = [](void* owner, PageBindingToken token, PageAccessActionSnapshot snapshot) {
    auto& action = *static_cast<Actions<Slots>*>(owner);
    ++action.commands;
    action.last = token;
    action.access_snapshot = snapshot;
    if (action.seek_page != NO_PAGE) assert(action.renderer->open_page(action.seek_page));
  };
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                  nullptr, nullptr, nullptr, nullptr, &access);
    assert(port.initialize());
    {
      auto absent = callbacks;
      absent.access = nullptr;
      PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                       absent, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                       nullptr, nullptr, nullptr, nullptr, &access);
      assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
    }
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[index++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    PageRuntimeResourcesGroup<3> resources({&access, &renderer, &state});
    PageStoreRuntime publication(limits, arena, 32768, &resources);
    assert(publication.prepare(PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(), fixture.size));
    assert(publication.activate_prepared(0, PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(),
                                         fixture.size));
    renderer.update_visible();
    assert(access.memory().histories == 4);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Garage Door") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[4].label), "--") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[4].icon), "GARAGE_OPEN") == 0);
    assert(lv_obj_has_flag(widgets[2].button, LV_OBJ_FLAG_CLICKABLE));
    subscriptions.send("cover.garage", "", "closed");
    subscriptions.send("cover.gate", "", "closed");
    subscriptions.send("lock.room", "", "locked");
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[4].label), "Closed") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[4].icon), "GARAGE_CLOSED") == 0);
    subscriptions.send("cover.garage", "friendly_name", "Garage name");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Garage name") == 0);
    click(widgets[3].button);
    assert(actions.commands == 1 && actions.access_snapshot.locked);
    subscriptions.send("lock.room", "", "unlocked");
    renderer.update_visible();
    click(widgets[3].button);
    assert(actions.commands == 2 && !actions.access_snapshot.locked);
    assert(lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    subscriptions.send("lock.room", "", "locked");
    const auto gate_background = lv_color_to_u32(lv_obj_get_style_bg_color(widgets[2].button, LV_PART_MAIN));
    subscriptions.send("cover.gate", "", "unavailable");
    renderer.update_visible();
    assert(lv_color_to_u32(lv_obj_get_style_bg_color(widgets[2].button, LV_PART_MAIN)) == gate_background);
    assert(lv_color_to_u32(lv_obj_get_style_text_color(widgets[2].label, LV_PART_MAIN)) ==
           lv_color_to_u32(lv_color_hex(theme.disabled_text)));
    theme.disabled_text = 0x123456;
    ++theme.revision;
    renderer.update_visible();
    assert(lv_color_to_u32(lv_obj_get_style_text_color(widgets[2].label, LV_PART_MAIN)) ==
           lv_color_to_u32(lv_color_hex(theme.disabled_text)));
    assert(lv_obj_has_state(widgets[2].button, LV_STATE_DISABLED) &&
           lv_obj_has_state(widgets[2].label, LV_STATE_DISABLED));
    assert(!lv_obj_has_flag(widgets[2].button, LV_OBJ_FLAG_CLICKABLE));
    click(widgets[2].button);
    click(widgets[5].button);
    assert(actions.commands == 2);
    subscriptions.send("cover.gate", "", "closing");
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[2].button, LV_STATE_DISABLED));
    assert(lv_obj_has_state(widgets[2].button, LV_STATE_CHECKED) &&
           lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    click(widgets[5].button);
    assert(actions.commands == 3);
    subscriptions.send("cover.gate", "", "closed");
    subscriptions.send("cover.garage", "", "opening");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Opening") == 0);
    fan_status_tick += 10000;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Opening") == 0);
    subscriptions.send("cover.garage", "", "open");
    renderer.update_visible();
    const auto child = fixture.store.card(0)->target;
    subscriptions.send("cover.gate", "", "unavailable");
    renderer.update_visible();
    const auto old = renderer.binding(1);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 3 && !renderer.valid(old));
    assert(!lv_obj_has_state(widgets[5].button, LV_STATE_DISABLED) &&
           !lv_obj_has_state(widgets[5].label, LV_STATE_DISABLED));
    assert(lv_obj_has_flag(widgets[5].button, LV_OBJ_FLAG_CLICKABLE));
    fan_status_tick += 3000;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Garage name") == 0);
    click(widgets[4].button);
    assert(actions.commands == 4);  // Direct lock command has no history.
    assert(renderer.back());
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[4].label), "Open") ==
           0);  // Persistent history did not expire.
    lv_obj_send_event(widgets[3].button, LV_EVENT_PRESSED, nullptr);
    ++subscriptions.generation;
    renderer.update_visible();
    for (const auto slot : {size_t(2), size_t(5)}) {
      assert(lv_obj_has_state(widgets[slot].button, LV_STATE_DISABLED));
      assert(!lv_obj_has_flag(widgets[slot].button, LV_OBJ_FLAG_CLICKABLE));
      click(widgets[slot].button);
    }
    assert(actions.commands == 4);
    subscriptions.send("cover.gate", "friendly_name", "Waiting");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[2].button, LV_STATE_DISABLED));
    subscriptions.send("cover.gate", "", "closed");
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[2].button, LV_STATE_DISABLED) &&
           lv_obj_has_flag(widgets[5].button, LV_OBJ_FLAG_CLICKABLE));
    subscriptions.send("lock.room", "", "locked");
    lv_obj_send_event(widgets[3].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 4);
    subscriptions.send("cover.garage", "", std::string(10000, 'x'));
    renderer.update_visible();
    click(widgets[1].button);
    assert(actions.commands == 4);
    subscriptions.send("cover.garage", "", "closed");
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.suspend() && renderer.resume());
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 4);
    actions.seek_page = child;
    click(widgets[1].button);
    actions.seek_page = NO_PAGE;
    assert(actions.commands == 5 && !renderer.valid(actions.last));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, reserved = arena.allocations, retained = lv_live_bytes,
               channels = subscriptions.bindings.size(), descriptors = cache.memory().descriptor_adds;
    for (int n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reserved && lv_live_bytes == retained &&
             subscriptions.bindings.size() == channels && cache.memory().descriptor_adds == descriptors);
    }
    renderer.retire_active();
    state.retire_active();
    access.retire_active();
    std::printf("%s: access cards 1000 cycles, independent status, availability and stable resources\n",
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
void cover_command_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::CoverCommands{});
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
  callbacks.cover_command = [](void* owner, PageBindingToken token, PageCoverCommandSnapshot command) {
    auto& action = *static_cast<Actions<Slots>*>(owner);
    ++action.commands;
    action.last = token;
    action.cover_snapshot = command;
    if (action.seek_page != NO_PAGE) assert(action.renderer->open_page(action.seek_page));
  };
  {
    auto absent = callbacks;
    absent.cover_command = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     absent);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t used = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[used++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    PageRuntimeResourcesGroup<2> resources({&renderer, &state});
    PageStoreRuntime publication(limits, arena, 32768, &resources);
    assert(publication.prepare(PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(), fixture.size));
    assert(publication.activate_prepared(0, PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(),
                                         fixture.size));
    renderer.update_visible();
    lv_obj_update_layout(page);
    for (size_t slot = 1; slot <= 4; ++slot) {
      assert(!lv_obj_has_state(widgets[slot].button, LV_STATE_CHECKED) &&
             lv_obj_has_flag(widgets[slot].sensor_container, LV_OBJ_FLAG_HIDDEN));
      assert(std::strcmp(lv_label_get_text(widgets[slot].icon), "CLOSED") == 0);
      click(widgets[slot].button);
      assert(actions.commands == slot && !actions.cover_snapshot.known && !actions.cover_snapshot.tilt);
    }
    subscriptions.send("cover.room", "supported_features", "16");
    renderer.update_visible();
    click(widgets[1].button);
    assert(actions.commands == 5 && actions.cover_snapshot.known && actions.cover_snapshot.tilt);
    click(widgets[2].button);
    click(widgets[3].button);
    click(widgets[4].button);
    assert(actions.commands == 5);
    subscriptions.send("cover.room", "supported_features", "64");
    click(widgets[3].button);
    assert(actions.commands == 6 && actions.cover_snapshot.tilt);
    assert(fixture.store.field(*fixture.store.card(actions.last.card), CardField::SENSOR) == "stop");
    subscriptions.send("cover.room", "supported_features", "128");
    click(widgets[4].button);
    assert(actions.commands == 7 && actions.cover_snapshot.tilt);
    assert(fixture.store.field(*fixture.store.card(actions.last.card), CardField::UNIT) == "67");
    subscriptions.send("cover.room", "supported_features", "255");
    click(widgets[1].button);
    assert(actions.commands == 8 && !actions.cover_snapshot.tilt);  // Prefer normal when both exist.
    subscriptions.send("cover.room", "supported_features", "0");
    click(widgets[1].button);
    assert(actions.commands == 8);
    subscriptions.send("cover.room", "supported_features", "unknown");
    click(widgets[1].button);
    assert(actions.commands == 9 && !actions.cover_snapshot.known && !actions.cover_snapshot.tilt);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    ++subscriptions.generation;
    subscriptions.send("cover.room", "supported_features", "16");
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 9);
    renderer.update_visible();
    subscriptions.send("cover.room", "supported_features", std::string(10000, '9'));
    click(widgets[1].button);
    assert(actions.commands == 9);
    subscriptions.send("cover.room", "supported_features", "255");
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));  // Command children never contribute.
    const auto child = fixture.store.card(0)->target;
    const auto root = renderer.binding(1);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 9 && !renderer.valid(root));
    click(widgets[1].button);
    assert(actions.commands == 10 && renderer.valid(actions.last));
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.suspend() && renderer.resume());
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 10);
    actions.seek_page = child;
    click(widgets[1].button);
    actions.seek_page = NO_PAGE;
    assert(actions.commands == 11 && !renderer.valid(actions.last));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, retained = lv_live_bytes, reservations = arena.allocations,
               channels = subscriptions.bindings.size(), descriptors = cache.memory().descriptor_adds;
    for (int i = 0; i < 1000; ++i) {
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
    std::printf("%s: cover commands 1000 cycles, capabilities, tilt fallback and stable resources\n",
                limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}
template <size_t Slots>
void cleaning_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::CleaningCards{});
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
    w.fill = lv_obj_create(w.button);
    w.slider = lv_slider_create(w.button);
  }
  for (size_t slot : {size_t(1), size_t(3)}) {
    lv_obj_set_style_pad_left(widgets[slot].button, 7, LV_PART_MAIN);
    lv_obj_set_style_pad_right(widgets[slot].button, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_top(widgets[slot].button, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(widgets[slot].button, 9, LV_PART_MAIN);
  }
  const auto plain_label_align = lv_obj_get_style_align(widgets[4].label, LV_PART_MAIN);
  const auto plain_label_width = lv_obj_get_style_width(widgets[4].label, LV_PART_MAIN);
  PageBasicTheme theme;
  Actions<Slots> actions;
  auto callbacks = actions.callbacks();
  callbacks.cleaning = [](void* owner, PageBindingToken token, const char* service) {
    auto& action = *static_cast<Actions<Slots>*>(owner);
    ++action.commands;
    action.last = token;
    action.cleaning_service = service;
    if (action.seek_page != NO_PAGE) assert(action.renderer->open_page(action.seek_page));
  };
  {
    auto absent = callbacks;
    absent.cleaning = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     absent);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t used = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[used++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    PageRuntimeResourcesGroup<2> resources({&renderer, &state});
    PageStoreRuntime publication(limits, arena, 32768, &resources);
    assert(publication.prepare(PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(), fixture.size));
    assert(publication.activate_prepared(0, PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(),
                                         fixture.size));
    renderer.update_visible();
    assert(!lv_obj_has_flag(widgets[2].button, LV_OBJ_FLAG_CLICKABLE) &&
           !lv_obj_has_flag(widgets[4].button, LV_OBJ_FLAG_CLICKABLE));
    assert(std::strcmp(lv_label_get_text(widgets[1].icon), "VAC_OFF") == 0);
    click(widgets[2].button);
    assert(actions.commands == 0);
    click(widgets[1].button);
    assert(actions.commands == 1 && std::strcmp(actions.cleaning_service, "vacuum.start") == 0);
    subscriptions.send("vacuum.room", "", "cleaning");
    subscriptions.send("lawn_mower.room", "", "docked");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED) &&
           !lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[1].icon), "VAC_NORMAL") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "Cleaning") == 0);
    lv_obj_update_layout(page);
    lv_area_t content, label_bounds;
    lv_obj_get_content_coords(widgets[1].button, &content);
    lv_obj_get_coords(widgets[1].label, &label_bounds);
    assert(label_bounds.x1 == content.x1 && label_bounds.x2 == content.x2 && label_bounds.y2 == content.y2);
    assert(std::strcmp(lv_label_get_text(widgets[5].icon), "VAC_AREA") ==
           0);  // Direct Dock never adopts shared live state.
    click(widgets[1].button);
    assert(actions.commands == 2 && std::strcmp(actions.cleaning_service, "vacuum.stop") == 0);
    subscriptions.send("lawn_mower.room", "", "mowing");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED) &&
           !lv_obj_has_state(widgets[3].button, LV_STATE_CHECKED));
    assert(std::strcmp(lv_label_get_text(widgets[4].label), "Mowing") == 0 &&
           std::strcmp(lv_label_get_text(widgets[4].icon), "POWER") == 0);
    click(widgets[3].button);
    assert(actions.commands == 3 && std::strcmp(actions.cleaning_service, "lawn_mower.pause") == 0);
    subscriptions.send("lawn_mower.room", "", "unknown");
    subscriptions.send("vacuum.room", "", "unknown");
    renderer.update_visible();
    click(widgets[3].button);
    assert(actions.commands == 3);
    click(widgets[1].button);
    assert(actions.commands == 4 && std::strcmp(actions.cleaning_service, "vacuum.start") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "Unavailable") == 0);
    const auto child = fixture.store.card(0)->target;
    const auto old = renderer.binding(1);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 4 && !renderer.valid(old));
    assert(lv_obj_get_style_align(widgets[4].label, LV_PART_MAIN) == plain_label_align);
    assert(lv_obj_get_style_width(widgets[4].label, LV_PART_MAIN) == plain_label_width);
    click(widgets[1].button);
    assert(actions.commands == 4);  // Start/Dock has no safe service for unavailable state.
    subscriptions.send("vacuum.room", "", "docked");
    renderer.update_visible();
    click(widgets[1].button);
    assert(actions.commands == 5 && std::strcmp(actions.cleaning_service, "vacuum.start") == 0);
    subscriptions.send("vacuum.room", "", "paused");
    subscriptions.send("lawn_mower.room", "", "docked");
    renderer.update_visible();
    click(widgets[2].button);
    assert(actions.commands == 6 && std::strcmp(actions.cleaning_service, "lawn_mower.dock") == 0);
    click(widgets[3].button);
    assert(actions.commands == 7 && std::strcmp(actions.cleaning_service, "vacuum.clean_area") == 0);
    const auto* area = publication.active().card(actions.last.card);
    assert(area && publication.active().field(*area, CardField::UNIT) == "zone_1");
    subscriptions.send("vacuum.room", "", std::string(10000, 'x'));
    renderer.update_visible();
    click(widgets[1].button);
    assert(actions.commands == 7);
    click(widgets[5].button);
    assert(actions.commands == 8 && std::strcmp(actions.cleaning_service, "vacuum.locate") ==
                                        0);  // Aggregate-only state is not a direct command dependency.
    subscriptions.send("vacuum.room", "", "cleaning");
    renderer.update_visible();
    lv_obj_send_event(widgets[2].button, LV_EVENT_PRESSED, nullptr);
    ++subscriptions.generation;
    subscriptions.send("vacuum.room", "", "paused");
    subscriptions.send("lawn_mower.room", "", "mowing");
    renderer.update_visible();
    lv_obj_send_event(widgets[2].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 8);
    lv_obj_send_event(widgets[2].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.suspend() && renderer.resume());
    renderer.update_visible();
    lv_obj_send_event(widgets[2].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 8);
    assert(renderer.back());
    renderer.update_visible();
    actions.seek_page = child;
    click(widgets[1].button);
    actions.seek_page = NO_PAGE;
    assert(actions.commands == 9 && !renderer.valid(actions.last));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, reserved = arena.allocations, retained = lv_live_bytes,
               channels = subscriptions.bindings.size(), descriptors = cache.memory().descriptor_adds;
    for (int n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reserved && lv_live_bytes == retained &&
             subscriptions.bindings.size() == channels && cache.memory().descriptor_adds == descriptors);
    }
    renderer.retire_active();
    state.retire_active();
    std::printf("%s: vacuum/mower cards 1000 cycles, state policies, direct commands and stable resources\n",
                limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}

PageTimezoneSelection test_timezone_resolve(void*, std::string_view option, time_t) {
  const auto id = option.substr(0, option.find(" ("));
  return {id, id == "Asia/Kathmandu" ? "GMT-5:45" : id == "Pacific/Chatham" ? "GMT-12:45" : "UTC0"};
}
const lv_font_t* calendar_large_font();
template <size_t Slots>
void clock_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::ClockCards{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageTimezoneRuntime timezone({nullptr, test_timezone_resolve});
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
  theme.sensor_font = &lv_font_montserrat_14;
  theme.large_sensor_font = calendar_large_font();
  const auto original_align = lv_obj_get_style_align(widgets[1].sensor_container, LV_PART_MAIN);
  Actions<Slots> actions;
  {
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     actions.callbacks());
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  actions.callbacks(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &timezone);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t used = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[used++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    PageRuntimeResourcesGroup<3> resources({&renderer, &timezone, &state});
    PageStoreRuntime publication(limits, arena, 32768, &resources);
    assert(publication.prepare(PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(), fixture.size));
    assert(publication.activate_prepared(0, PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(),
                                         fixture.size));
    renderer.update_visible();
    const auto widget_for = [&](CardId id) -> const PageBasicWidgets& {
      for (size_t i = 0; i < Slots; ++i)
        if (renderer.binding(i).binding && !renderer.binding(i).back && renderer.binding(i).card == id)
          return widgets[i];
      assert(false);
      return widgets[0];
    };
    const auto& root_clock = widget_for(1);
    const auto& root_world = widget_for(2);
    const auto& root_city = widget_for(3);
    assert(timezone.memory().active_cards == 5 && subscriptions.bindings.size() == 1);
    assert(std::strcmp(lv_label_get_text(root_clock.value), "--:--") == 0);
    assert(std::strcmp(lv_label_get_text(root_clock.label), "") == 0);
    assert(std::strcmp(lv_label_get_text(root_world.label), "Kathmandu") == 0);
    assert(!lv_obj_has_flag(root_clock.button, LV_OBJ_FLAG_CLICKABLE) &&
           lv_obj_has_flag(root_clock.icon, LV_OBJ_FLAG_HIDDEN));
    click(root_clock.button);
    click(root_world.button);
    assert(actions.commands == 0 && actions.toggles == 0);
    assert(timezone.update(true, 1792281900, "UTC", true));
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(root_clock.value), "12:05") == 0);
    assert(std::strcmp(lv_label_get_text(root_world.value), "5:50") == 0);
    assert(std::strcmp(lv_label_get_text(root_city.label), "Long City With Underscores") == 0);
    assert(lv_obj_get_style_align(root_clock.sensor_container, LV_PART_MAIN) == LV_ALIGN_LEFT_MID);
    const auto child = fixture.store.card(0)->target;
    assert(renderer.open_page(child));
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[3].label), "Chatham") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[3].value), "12:50") == 0);
    assert(lv_obj_get_style_align(widget_for(limits.slot_count).sensor_container, LV_PART_MAIN) ==
           original_align);
    assert(lv_obj_get_style_align(widget_for(limits.slot_count + 1).sensor_container, LV_PART_MAIN) ==
           original_align);
    assert(!lv_obj_has_flag(widget_for(limits.slot_count).label, LV_OBJ_FLAG_HIDDEN));
    assert(timezone.update(true, 1792281900, "Pacific/Chatham", false));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(root_clock.value), "12:50") == 0);
    assert(!lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    ++subscriptions.generation;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(root_clock.value), "12:50") == 0);
    assert(timezone.update(false, 0, "UTC", false));
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(root_world.value), "--:--") == 0 &&
           std::strcmp(lv_label_get_text(root_world.label), "Kathmandu") == 0);
    assert(timezone.update(true, 1792367940, "UTC", false));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, reserved = arena.allocations, retained = lv_live_bytes,
               channels = subscriptions.bindings.size(), descriptors = cache.memory().descriptor_adds;
    for (int n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      assert(timezone.update(true, 1792281600 + n * 60, n % 2 ? "UTC" : "Pacific/Chatham", (n % 2) != 0));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reserved && lv_live_bytes == retained &&
             subscriptions.bindings.size() == channels && cache.memory().descriptor_adds == descriptors);
    }
    renderer.retire_active();
    timezone.retire_active();
    state.retire_active();
    std::printf("%s: clock/world-clock 1000 cycles, hidden updates and stable resources\n",
                limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}

uint32_t alarm_card_tick = 100;
uint32_t alarm_card_now() {
  return alarm_card_tick;
}
template <size_t Slots>
void alarm_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::AlarmCards{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageAlarmStateRuntime alarm(arena, subscriptions, alarm_card_now);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &alarm);
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
  theme.surface = 0x111111;
  theme.sensor_surface = 0x222222;
  theme.alarm_surface = 0x333333;
  theme.active = 0xFFE066;
  Actions<Slots> actions;
  auto callbacks = actions.callbacks();
  callbacks.alarm = [](void* owner, PageBindingToken token) {
    auto& a = *static_cast<Actions<Slots>*>(owner);
    ++a.commands;
    a.last = token;
    if (a.seek_page != NO_PAGE) assert(a.renderer->open_page(a.seek_page));
  };
  callbacks.icon = [](void* owner, std::string_view name, std::string_view domain) -> const char* {
    auto& a = *static_cast<Actions<Slots>*>(owner);
    const char* glyph = name == "Security"        ? "ALARM_INITIAL"
                        : name == "Alarm"         ? "ALARM_NORMAL"
                        : name == "Shield Home"   ? "ALARM_HOME"
                        : name == "Shield Lock"   ? "ALARM_AWAY"
                        : name == "Weather Night" ? "ALARM_NIGHT"
                        : name == "Airplane"      ? "ALARM_VACATION"
                        : name == "Shield Off"    ? "ALARM_DISARM"
                        : name == "Alarm Light"   ? "ALARM_TRIGGER"
                        : name == "Home"          ? "STATIC_HOME"
                                                  : nullptr;
    if (glyph) {
      ++a.icon_resolves;
      return glyph;
    }
    return Actions<Slots>::icon(owner, name, domain);
  };
  {
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     callbacks);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    auto absent = callbacks;
    absent.alarm = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     absent, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                     nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &alarm);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &alarm);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t used = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[used++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    PageRuntimeResourcesGroup<3> resources({&renderer, &alarm, &state});
    PageStoreRuntime publication(limits, arena, 32768, &resources);
    assert(publication.prepare(PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(), fixture.size));
    assert(publication.activate_prepared(0, PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(),
                                         fixture.size));
    renderer.update_visible();

    if (alarm.memory().active_sources != 3 || subscriptions.bindings.size() != 8)
      std::fprintf(stderr, "alarm fixture %s: sources=%zu channels=%zu\n", limits.device_profile,
                   alarm.memory().active_sources, subscriptions.bindings.size());
    assert(alarm.memory().active_sources == 3 && subscriptions.bindings.size() == 8);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "--") == 0 &&
           std::strcmp(lv_label_get_text(widgets[1].icon), "ALARM_INITIAL") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "Configured") == 0 &&
           std::strcmp(lv_label_get_text(widgets[2].icon), "POWER") == 0);
    assert(lv_obj_has_flag(widgets[1].sensor_container, LV_OBJ_FLAG_HIDDEN));
    subscriptions.send("alarm_control_panel.home", "arm_mode", "");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].icon), "ALARM_NORMAL") == 0);
    subscriptions.send("alarm_control_panel.home", "friendly_name", "House Alarm");
    subscriptions.send("sensor.temperature", "", "22.5");
    subscriptions.send("alarm_control_panel.home", "", "disarmed");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Disarmed") == 0 &&
           lv_obj_has_state(widgets[4].button, LV_STATE_CHECKED));
    const auto color = [](uint32_t rgb) { return lv_color_to_u32(lv_color_hex(rgb)); };
    assert(lv_color_to_u32(lv_obj_get_style_bg_color(widgets[1].button, LV_PART_MAIN)) ==
           color(theme.alarm_surface));
    assert(lv_color_to_u32(lv_obj_get_style_bg_color(widgets[3].button, LV_PART_MAIN)) ==
           color(theme.surface));
    lv_obj_add_state(widgets[1].button, LV_STATE_PRESSED);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(lv_color_to_u32(lv_obj_get_style_bg_color(widgets[1].button, LV_PART_MAIN)) ==
           color(theme.active));
    assert(lv_color_to_u32(lv_obj_get_style_text_color(widgets[1].label, LV_PART_MAIN)) ==
           color(display_text_color_for_bg(theme.active)));
    lv_obj_clear_state(widgets[1].button, LV_STATE_PRESSED);
    lv_obj_send_event(widgets[1].button, LV_EVENT_RELEASED, nullptr);
    assert(lv_color_to_u32(lv_obj_get_style_text_color(widgets[1].label, LV_PART_MAIN)) ==
           color(display_text_color_for_bg(theme.alarm_surface)));
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 1);
    click(widgets[3].button);
    assert(actions.commands == 2 && !lv_obj_has_state(widgets[3].button, LV_STATE_CHECKED));
    subscriptions.send("alarm_control_panel.home", "", "arming");
    subscriptions.send("alarm_control_panel.home", "arm_mode", "home");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].icon), "ALARM_HOME") == 0 &&
           std::strcmp(lv_label_get_text(widgets[2].label), "Arming") == 0);
    subscriptions.send("alarm_control_panel.home", "", "armed_away");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[3].button, LV_STATE_CHECKED) &&
           std::strcmp(lv_label_get_text(widgets[3].icon), "POWER") == 0);
    alarm_card_tick += 3000;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "Configured") == 0);
    const auto child = fixture.store.card(0)->target;
    assert(renderer.open_page(child));
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "22.5") == 0 &&
           std::strcmp(lv_label_get_text(widgets[2].icon), "STATIC_HOME") == 0 &&
           std::strcmp(lv_label_get_text(widgets[2].label), "House Alarm") == 0);
    lv_obj_add_state(widgets[1].button, LV_STATE_PRESSED);
    assert(lv_color_to_u32(lv_obj_get_style_bg_color(widgets[1].button, LV_PART_MAIN)) ==
           color(theme.sensor_surface));
    lv_obj_clear_state(widgets[1].button, LV_STATE_PRESSED);
    subscriptions.send("alarm_control_panel.only", "", "on");
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    assert(!lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));  // Action children do not contribute.
    subscriptions.send("alarm_control_panel.home", "", "on");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    subscriptions.send("alarm_control_panel.home", "", "triggered");
    renderer.update_visible();
    assert(alarm.takeover().valid && std::strcmp(lv_label_get_text(widgets[1].icon), "ALARM_TRIGGER") == 0);
    assert(lv_color_to_u32(lv_obj_get_style_bg_color(widgets[1].button, LV_PART_MAIN)) == color(0xC62828));
    assert(renderer.open_page(child));
    renderer.update_visible();
    assert(alarm.takeover().valid);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_send_event(widgets[3].button, LV_EVENT_PRESSED, nullptr);
    ++subscriptions.generation;
    subscriptions.send("alarm_control_panel.home", "", "disarmed");
    lv_obj_send_event(widgets[3].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 2);
    renderer.update_visible();
    subscriptions.send("alarm_control_panel.home", "", "unknown");
    renderer.update_visible();
    click(widgets[3].button);
    assert(actions.commands == 2);
    click(widgets[1].button);
    assert(actions.commands == 3);  // Readonly panel remains available.
    lv_obj_send_event(widgets[2].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_send_event(widgets[2].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 3);
    subscriptions.send("alarm_control_panel.home", "", "disarmed");
    renderer.update_visible();
    actions.seek_page = 0;
    click(widgets[2].button);
    assert(actions.commands == 4 && renderer.active_page() == 0);
    actions.seek_page = NO_PAGE;
    theme.surface = 0xFFFFFF;
    theme.alarm_surface = 0xB0B0B0;
    theme.text = 0x333333;
    ++theme.revision;
    renderer.update_visible();
    assert(lv_color_to_u32(lv_obj_get_style_text_color(widgets[1].label, LV_PART_MAIN)) ==
           color(display_text_color_for_bg(theme.alarm_surface)));
    const char* states[] = {
        "disarmed", "armed_away", "armed_home", "armed_night", "armed_vacation", "armed_custom_bypass",
        "arming",   "pending",    "triggered",  "unknown"};
    for (const auto* value : states) {
      subscriptions.send("alarm_control_panel.home", "", value);
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
    }
    const auto cpp = cpp_allocations, reserved = arena.allocations, retained = lv_live_bytes,
               channels = subscriptions.bindings.size(), descriptors = cache.memory().descriptor_adds,
               glyphs = actions.icon_resolves;
    for (int n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      alarm_card_tick += 100;
      subscriptions.send("alarm_control_panel.home", "", states[n % std::size(states)]);
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reserved && lv_live_bytes == retained &&
             subscriptions.bindings.size() == channels && cache.memory().descriptor_adds == descriptors &&
             actions.icon_resolves == glyphs);
    }
    assert(port.memory().text_truncations == 0);
    renderer.retire_active();
    alarm.retire_active();
    state.retire_active();
    std::printf(
        "%s: alarm status/actions 1000 cycles, captured intents, palette/reuse and stable resources\n",
        limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}

template <size_t Slots>
void calendar_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::CalendarCards{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageCalendarRuntime calendar;
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &calendar);
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
  {
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     actions.callbacks());
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  actions.callbacks(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &calendar);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t used = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[used++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    PageRuntimeResourcesGroup<3> resources({&renderer, &calendar, &state});
    PageStoreRuntime publication(limits, arena, 32768, &resources);
    assert(publication.prepare(PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(), fixture.size));
    assert(publication.activate_prepared(0, PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(),
                                         fixture.size));
    renderer.update_visible();
    // Two calendar sources, plus the numeric sensor state and its friendly name.
    assert(calendar.memory().active_sources == 2 && subscriptions.bindings.size() == 4);
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "--") == 0 &&
           std::strcmp(lv_label_get_text(widgets[1].label), "Date") == 0);
    assert(!lv_obj_has_flag(widgets[1].button, LV_OBJ_FLAG_CLICKABLE) &&
           lv_obj_has_flag(widgets[1].icon, LV_OBJ_FLAG_HIDDEN));
    click(widgets[1].button);
    click(widgets[2].button);
    assert(actions.commands == 0 && actions.toggles == 0);
    subscriptions.send("sensor.date", "", "2026-10-29");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "29") == 0 &&
           std::strcmp(lv_label_get_text(widgets[1].label), "October") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[2].value), "--") == 0);
    calendar.time(true, 30, 11, 0, 5, true);
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "30") == 0 &&
           std::strcmp(lv_label_get_text(widgets[2].value), "12:05") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "30 November") == 0);
    const auto child = fixture.store.card(0)->target;
    assert(renderer.open_page(child));
    renderer.update_visible();
    subscriptions.send("sensor.other", "", "2026-12-31");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "31") == 0);
    assert(renderer.back());
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "31 December") == 0);
    assert(!lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED));
    ++subscriptions.generation;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "31") == 0);
    calendar.time(false, 0, 0, 0, 0, false);
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[2].value), "--") == 0 &&
           std::strcmp(lv_label_get_text(widgets[1].value), "31") == 0);
    calendar.time(true, 31, 12, 23, 59, false);
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, reserved = arena.allocations, retained = lv_live_bytes,
               channels = subscriptions.bindings.size(), descriptors = cache.memory().descriptor_adds;
    for (int n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      calendar.time(true, 31, 12, n % 24, n % 60, (n % 2) != 0);
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp_allocations == cpp && arena.allocations == reserved && lv_live_bytes == retained &&
             subscriptions.bindings.size() == channels && cache.memory().descriptor_adds == descriptors);
    }
    renderer.retire_active();
    calendar.retire_active();
    state.retire_active();
    std::printf("%s: calendar/date-time 1000 cycles, shared hidden history and stable resources\n",
                limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}

const lv_font_t* calendar_large_font() {
  static lv_font_t font = lv_font_montserrat_14;
  font.line_height = 28;
  return &font;
}
template <size_t Slots>
void calendar_size_profile(const DeviceUiLimits& limits) {
  for (const auto* order : {"1", "1w", "1b", "1q"})
    for (bool disabled : {false, true}) {
      if (std::strcmp(order, "1q") == 0 && (limits.grid_columns < 3 || (Slots / limits.grid_columns) < 3))
        continue;
      Fixture fixture(limits, Fixture::CalendarSize{order, disabled});
      Arena arena;
      Subscriptions subscriptions;
      PageCardDependencies deps;
      PageCalendarRuntime calendar;
      PageEntityStateRuntime state(deps, arena, subscriptions, {64, 64, 2048}, &calendar);
      auto* page = lv_obj_create(lv_screen_active());
      std::array<lv_coord_t, Slots + 1> columns{}, rows{};
      for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
      columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
      const auto count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
      for (size_t i = 0; i < count; ++i) rows[i] = LV_GRID_FR(1);
      rows[count] = LV_GRID_TEMPLATE_LAST;
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
      theme.sensor_font = &lv_font_montserrat_14;
      theme.large_sensor_font = calendar_large_font();
      theme.surface = 0x111111;
      theme.sensor_surface = 0x222222;
      Actions<Slots> actions;
      {
        PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                      actions.callbacks(), nullptr, nullptr, nullptr, nullptr, nullptr,
                                      nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                      &calendar);
        assert(port.initialize());
        PageRendererRuntime<Slots> renderer(limits, arena, port);
        actions.renderer = &renderer;
        assert(renderer.prepare(fixture.store) && calendar.prepare(fixture.store) &&
               state.prepare(fixture.store));
        calendar.activate_prepared(fixture.store);
        state.activate_prepared(fixture.store);
        renderer.activate_prepared(fixture.store);
        calendar.time(true, 31, 12, 23, 59, false);
        renderer.update_visible();
        const auto* expected =
            !disabled && std::strcmp(order, "1b") == 0 ? calendar_large_font() : theme.sensor_font;
        assert(lv_obj_get_style_text_font(widgets[0].value, LV_PART_MAIN) == expected);
        assert(!lv_obj_has_flag(widgets[0].label, LV_OBJ_FLAG_HIDDEN));
        assert(lv_color_to_u32(lv_obj_get_style_bg_color(widgets[0].button, LV_PART_MAIN)) ==
               lv_color_to_u32(lv_color_hex(theme.sensor_surface)));
        renderer.retire_active();
        calendar.retire_active();
        state.retire_active();
      }
      assert(arena.live == 0);
      lv_obj_delete(page);
    }
}

template <size_t Slots>
void clock_size_profile(const DeviceUiLimits& limits) {
  for (const auto* order : {"1", "1w", "1b", "1q"})
    for (const auto* type : {"clock", "timezone"})
      for (const auto* options : {"", "large_numbers", "large_numbers=off", "large_numbers,center_clock"}) {
        if (std::strcmp(order, "1q") == 0 && (limits.grid_columns < 3 || (Slots / limits.grid_columns) < 3))
          continue;
        Fixture fixture(limits, Fixture::ClockSize{order, type, options});
        Arena arena;
        Subscriptions subscriptions;
        PageCardDependencies deps;
        PageTimezoneRuntime timezone({nullptr, test_timezone_resolve});
        PageEntityStateRuntime state(deps, arena, subscriptions, {64, 64, 2048});
        auto* page = lv_obj_create(lv_screen_active());
        std::array<lv_coord_t, Slots + 1> columns{}, rows{};
        for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
        columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
        const auto count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
        for (size_t i = 0; i < count; ++i) rows[i] = LV_GRID_FR(1);
        rows[count] = LV_GRID_TEMPLATE_LAST;
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
        theme.sensor_font = &lv_font_montserrat_14;
        theme.large_sensor_font = calendar_large_font();
        theme.surface = 0x111111;
        theme.sensor_surface = 0x222222;
        Actions<Slots> actions;
        {
          PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns},
                                        theme, actions.callbacks(), nullptr, nullptr, nullptr, nullptr,
                                        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                        nullptr, nullptr, &timezone);
          assert(port.initialize());
          PageRendererRuntime<Slots> renderer(limits, arena, port);
          actions.renderer = &renderer;
          assert(renderer.prepare(fixture.store) && timezone.prepare(fixture.store) &&
                 state.prepare(fixture.store));
          timezone.activate_prepared(fixture.store);
          state.activate_prepared(fixture.store);
          renderer.activate_prepared(fixture.store);
          assert(timezone.update(true, 1792367940, "UTC", false));
          renderer.update_visible();
          const bool disabled = std::strcmp(options, "large_numbers=off") == 0;
          const bool square = std::strcmp(order, "1b") == 0, wide = std::strcmp(order, "1w") == 0,
                     extra = std::strcmp(order, "1q") == 0;
          const bool clock = std::strcmp(type, "clock") == 0;
          const bool large =
              !disabled && (square || (clock && (wide || extra || std::strstr(options, "large_numbers"))));
          const auto* expected = large ? calendar_large_font() : theme.sensor_font;
          assert(lv_obj_get_style_text_font(widgets[0].value, LV_PART_MAIN) == expected);
          assert(lv_obj_has_flag(widgets[0].label, LV_OBJ_FLAG_HIDDEN) ==
                 (large && clock && (wide || extra)));
          if (large && clock && (wide || extra))
            assert(lv_obj_get_style_align(widgets[0].sensor_container, LV_PART_MAIN) ==
                   (extra && std::strstr(options, "center_clock") ? LV_ALIGN_CENTER : LV_ALIGN_LEFT_MID));
          assert(lv_color_to_u32(lv_obj_get_style_bg_color(widgets[0].button, LV_PART_MAIN)) ==
                 lv_color_to_u32(lv_color_hex(theme.sensor_surface)));
          if (large) {
            theme.large_sensor_font = nullptr;
            ++theme.revision;
            renderer.update_visible();
            assert(lv_obj_get_style_text_font(widgets[0].value, LV_PART_MAIN) == theme.sensor_font);
            assert(!lv_obj_has_flag(widgets[0].label, LV_OBJ_FLAG_HIDDEN));
            theme.large_sensor_font = calendar_large_font();
            ++theme.revision;
            renderer.update_visible();
            assert(lv_obj_get_style_text_font(widgets[0].value, LV_PART_MAIN) == expected);
          }
          renderer.retire_active();
          timezone.retire_active();
          state.retire_active();
        }
        assert(arena.live == 0);
        lv_obj_delete(page);
      }
}
template <size_t Slots>
void number_size_profile(const DeviceUiLimits& limits) {
  for (const auto* order : {"1", "1w", "1b", "1q"})
    for (const auto* type : {"sensor", "duration", "climate", "action", "subpage"})
      for (const auto* options : {"", "large_numbers", "large_numbers=off"}) {
        if (std::strcmp(order, "1q") == 0 && (limits.grid_columns < 3 || (Slots / limits.grid_columns) < 3))
          continue;
        Fixture fixture(limits, Fixture::NumberSize{order, type, options});
        Arena arena;
        Subscriptions subscriptions;
        PageCardDependencies deps;
        PageEntityStateRuntime state(deps, arena, subscriptions, {64, 64, 2048});
        auto* page = lv_obj_create(lv_screen_active());
        std::array<lv_coord_t, Slots + 1> columns{}, rows{};
        for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
        columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
        const auto count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
        for (size_t i = 0; i < count; ++i) rows[i] = LV_GRID_FR(1);
        rows[count] = LV_GRID_TEMPLATE_LAST;
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
        theme.sensor_font = &lv_font_montserrat_14;
        theme.large_sensor_font = calendar_large_font();
        theme.surface = 0x111111;
        theme.sensor_surface = 0x222222;
        Actions<Slots> actions;
        {
          PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns},
                                        theme, actions.callbacks(), nullptr, nullptr, nullptr, nullptr,
                                        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                        nullptr, nullptr);
          assert(port.initialize());
          PageRendererRuntime<Slots> renderer(limits, arena, port);
          actions.renderer = &renderer;
          assert(renderer.prepare(fixture.store) && state.prepare(fixture.store));
          state.activate_prepared(fixture.store);
          renderer.activate_prepared(fixture.store);
          renderer.update_visible();
          const bool disabled =
              std::strcmp(options, "large_numbers=off") == 0 && std::strcmp(type, "duration") != 0;
          const bool square = std::strcmp(order, "1b") == 0;
          const bool climate = std::strcmp(type, "climate") == 0;
          const bool eligible =
              climate || std::strcmp(type, "sensor") == 0 || std::strcmp(type, "duration") == 0;
          const bool large =
              eligible && !disabled && (square || (climate && std::strcmp(options, "large_numbers") == 0));
          const auto* expected = large ? calendar_large_font() : theme.sensor_font;
          if (lv_obj_get_style_text_font(widgets[0].value, LV_PART_MAIN) != expected)
            std::fprintf(stderr, "font parity failed: %s type=%s order=%s options=%s expected-large=%d\n",
                         limits.device_profile, type, order, options, large);
          assert(lv_obj_get_style_text_font(widgets[0].value, LV_PART_MAIN) == expected);
          assert(!lv_obj_has_flag(widgets[0].label, LV_OBJ_FLAG_HIDDEN));
          for (int percent : {-25, 30}) {
            theme.large_sensor_unit_offset_percent = percent;
            ++theme.revision;
            renderer.update_visible();
            assert(lv_obj_get_style_translate_y(widgets[0].unit, LV_PART_MAIN) ==
                   (large ? 28 * percent / 100 : 0));
          }
          if (large) {
            theme.large_sensor_font = nullptr;
            ++theme.revision;
            renderer.update_visible();
            assert(lv_obj_get_style_text_font(widgets[0].value, LV_PART_MAIN) == theme.sensor_font);
            assert(!lv_obj_has_flag(widgets[0].label, LV_OBJ_FLAG_HIDDEN));
            theme.large_sensor_font = calendar_large_font();
            ++theme.revision;
            renderer.update_visible();
            assert(lv_obj_get_style_text_font(widgets[0].value, LV_PART_MAIN) == expected);
          }
          renderer.retire_active();
          state.retire_active();
        }
        assert(arena.live == 0);
        lv_obj_delete(page);
      }
}

uint32_t timer_tick = 100;
int64_t timer_epoch = 0;
uint32_t timer_now() {
  return timer_tick;
}
int64_t timer_epoch_now() {
  return timer_epoch;
}
size_t timer_count() {
  size_t count = 0;
  lv_timer_t* timer = nullptr;
  while ((timer = lv_timer_get_next(timer))) ++count;
  return count;
}
// Exercise actual pointer callbacks through the shared deferred command owner.
template <size_t Slots>
struct DeferredGridPort final : PageCardCommandPort {
  PageCardControlPolicy& policy;
  PageRendererRuntime<Slots>* renderer{nullptr};
  bool accepted{true};
  size_t sends{0}, previews{0};
  PageBasicLvglPort<Slots>* view{nullptr};
  PageEntityStateRuntime* graph{nullptr};
  std::function<void(const PageCardCommandSnapshot&)> on_send;
  PageCardCommandKind last{PageCardCommandKind::NONE};
  explicit DeferredGridPort(PageCardControlPolicy& p) : policy(p) {}
  bool ready() const override { return !page_lvgl_dispatching(); }
  bool valid(PageBindingToken t) const override {
    return renderer && renderer->valid(t) && (!view || view->input_enabled());
  }
  bool connected() const override { return true; }
  bool service_available() const override { return true; }
  void dispatch(const PageCardCommandSnapshot&) override { assert(false); }
  bool admit_control(const PageStore& s, PageBindingToken t, PageCardCommandKind k,
                     PageCardCommandValue& v) override {
    return policy.admit(s, t, k, v);
  }
  bool valid_control(const PageStore& s, PageBindingToken t, PageCardCommandKind k,
                     const PageCardCommandValue& v) override {
    return policy.valid(s, t, k, v);
  }
  void discard_control(PageBindingToken t, PageCardCommandKind k, const PageCardCommandValue& v) override {
    policy.discard(t, k, v);
  }
  void accepted_control(PageBindingToken t, PageCardCommandKind k, const PageCardCommandValue& v) override {
    policy.accepted(t, k, v);
    if (view && k == PageCardCommandKind::DEVICE_ACTION &&
        view->preview_device(t, graph->connection_generation(), v.write_revision))
      ++previews;
    if (view && k == PageCardCommandKind::MEDIA_SEEK &&
        view->preview_seek(t, {graph->connection_generation(), v.write_revision},
                           static_cast<uint8_t>(v.integer), static_cast<float>(v.number)))
      ++previews;
  }
  bool dispatch_control(const PageCardCommandSnapshot& s) override {
    assert(ready());
    ++sends;
    last = s.kind;
    if (on_send) on_send(s);
    return accepted;
  }
};
template <size_t Slots>
void deferred_device_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::DeviceCommands{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageFanStatusRuntime fan(arena, subscriptions, fan_status_now);
  PageAccessStateRuntime access(arena, subscriptions, fan_status_now);
  PageCoverToggleRuntime cover(arena, subscriptions, fan_status_now);
  PageEntityObserverSet<3> observers;
  assert(observers.add(fan) && observers.add(access) && observers.add(cover));
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &observers);
  PageDeviceCommandPolicy policy(state, fan, access);
  DeferredGridPort<Slots> sender(policy);
  PageCardCommands commands(state, sender);
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  const auto count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < count; ++i) rows[i] = LV_GRID_FR(1);
  rows[count] = LV_GRID_TEMPLATE_LAST;
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
  PointerInput input;
  auto* device = lv_indev_create();
  lv_indev_set_type(device, LV_INDEV_TYPE_POINTER);
  lv_indev_set_display(device, lv_display_get_default());
  lv_indev_set_user_data(device, &input);
  lv_indev_set_read_cb(device, PointerInput::read);
  lv_timer_pause(lv_indev_get_read_timer(device));
  {
    PageBasicTheme theme;
    Actions<Slots> actions;
    actions.deferred = &commands;
    auto callbacks = actions.callbacks();
    callbacks.action = nullptr;
    callbacks.toggle = nullptr;
    callbacks.device = [](void* owner, PageBindingToken token, uint32_t connection) {
      auto& a = *static_cast<Actions<Slots>*>(owner);
      const auto before = cpp_allocations;
      a.admitted = a.deferred->request_device(token, connection);
      ++a.commands;
      assert(cpp_allocations == before && a.deferred->loop() == PageCardCommandResult::NONE);
    };
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, &fan, nullptr,
                                  nullptr, nullptr, nullptr, &cover, &access);
    assert(port.initialize());
    actions.basic_port = &port;
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t used = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[used++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    sender.renderer = &renderer;
    sender.view = &port;
    sender.graph = &state;
    PageRuntimeResourcesGroup<6> resources({&renderer, &commands, &fan, &access, &cover, &state});
    assert(resources.prepare(fixture.store));
    resources.activate_prepared(fixture.store);
    const auto report = [&] {
      subscriptions.send("fan.room", "", "on");
      subscriptions.send("fan.room", "oscillating", "false");
      subscriptions.send("fan.room", "direction", "forward");
      subscriptions.send("cover.room", "", "closed");
      subscriptions.send("cover.room", "supported_features", "1");
      subscriptions.send("cover.gate", "", "closed");
      subscriptions.send("lock.room", "", "locked");
      subscriptions.send("vacuum.room", "", "cleaning");
      subscriptions.send("lawn_mower.room", "", "mowing");
    };
    report();
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto child = fixture.store.card(5)->target;
    const auto reservations = arena.allocations, registrations = subscriptions.bindings.size();
    assert(renderer.valid(renderer.binding(0)));
    click(widgets[0].button);
    assert(commands.memory().pending == 1);
    commands.cancel();
    const auto press = [&](size_t slot) {
      pointer(device, input, widgets[slot].button, 50, true);
      pointer(device, input, widgets[slot].button, 50, false);
    };
    const auto drain = [&] {
      for (size_t i = 0; i < 5; ++i) assert(commands.loop() == PageCardCommandResult::DISPATCHED);
    };
    size_t retained = 0;
    for (size_t cycle = 0; cycle < 1000; ++cycle) {
      const auto sent = sender.sends;
      for (size_t i = 0; i < 5; ++i) press(i);
      assert(actions.admitted && commands.memory().pending == 5 && sender.sends == sent);
      drain();
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      for (size_t i = 1; i < 6; ++i) press(i);
      assert(actions.admitted && commands.memory().pending == 5 && sender.sends == sent + 5);
      drain();
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(sender.sends == sent + 10 && arena.allocations == reservations &&
             subscriptions.bindings.size() == registrations);
      if (!cycle)
        retained = lv_live_bytes;
      else
        assert(lv_live_bytes == retained);
    }
    // Pending and rejected sends cannot paint optimistic cover feedback.
    lv_obj_remove_state(widgets[4].button, LV_STATE_CHECKED);
    sender.accepted = false;
    press(4);
    assert(!lv_obj_has_state(widgets[4].button, LV_STATE_CHECKED));
    assert(commands.loop() == PageCardCommandResult::FAILED &&
           !lv_obj_has_state(widgets[4].button, LV_STATE_CHECKED));
    sender.accepted = true;
    press(4);
    assert(!lv_obj_has_state(widgets[4].button, LV_STATE_CHECKED));
    assert(commands.loop() == PageCardCommandResult::DISPATCHED &&
           lv_obj_has_state(widgets[4].button, LV_STATE_CHECKED));
    const auto origin = renderer.binding(4);
    const auto primary = state.read_attribute({origin.configuration, origin.card}, "cover.room", {});
    port.set_input_covered(true); assert(port.settle_input());
    assert(!Actions<Slots>::valid(&actions, origin) && renderer.bound(origin));
    assert(port.preview_device(origin, subscriptions.generation, primary.revision));
    port.set_input_covered(false);
    press(0);
    subscriptions.send("fan.room", "", "off");
    assert(commands.loop() == PageCardCommandResult::CANCELLED);
    report();
    press(0);
    assert(renderer.open_page(child));
    assert(commands.loop() == PageCardCommandResult::CANCELLED);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_send_event(widgets[0].button, LV_EVENT_PRESSED, nullptr);
    ++subscriptions.generation;
    renderer.update_visible();
    report();
    renderer.update_visible();
    const auto requested = actions.commands;
    lv_obj_send_event(widgets[0].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == requested);
    press(0);
    ++subscriptions.generation;
    assert(commands.loop() == PageCardCommandResult::CANCELLED);
    report();
    renderer.update_visible();
    press(4);
    sender.on_send = [&](const PageCardCommandSnapshot&) { assert(renderer.open_page(child)); };
    const auto previews = sender.previews;
    assert(commands.loop() == PageCardCommandResult::DISPATCHED && sender.previews == previews);
    sender.on_send = {};
    assert(renderer.back());
    renderer.update_visible();
    assert(resources.prepare(fixture.store));
    resources.activate_prepared(fixture.store);
    assert(commands.memory().pending == 0);
    resources.retire_active();
  }
  lv_indev_delete(device);
  lv_obj_delete(page);
  assert(arena.live == 0);
}
template <size_t Slots>
void deferred_media_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::DeferredMedia{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  struct Transport : PageMediaTransport {
    Subscriptions& subscriptions;
    explicit Transport(Subscriptions& s) : subscriptions(s) {}
    bool connected() const override { return true; }
    uint32_t connection_generation() const override { return subscriptions.generation; }
    uint32_t now_ms() const override { return 100; }
    int64_t epoch_seconds() const override { return 0; }
    void reannounce() override {}
  } transport(subscriptions);
  PageMediaStateRuntime media(arena, transport);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &media);
  PageMediaCommandPolicy policy(state, media);
  DeferredGridPort<Slots> sender(policy);
  PageCardCommands commands(state, sender);
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  const auto count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < count; ++i) rows[i] = LV_GRID_FR(1);
  rows[count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page, 480, 480);
  lv_obj_set_grid_dsc_array(page, columns.data(), rows.data());
  std::array<PageBasicWidgets, Slots> widgets{};
  for (auto& w : widgets) {
    w.button = lv_button_create(page);
    w.fill = lv_obj_create(w.button);
    w.icon = lv_label_create(w.button);
    w.label = lv_label_create(w.button);
    w.sensor_container = lv_obj_create(w.button);
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
  {
    PageBasicTheme theme;
    theme.media_title_font = &lv_font_montserrat_14;
    Actions<Slots> actions;
    actions.deferred = &commands;
    auto callbacks = actions.callbacks();
    callbacks.media = [](void* owner, PageBindingToken token, uint32_t connection) {
      auto& a = *static_cast<Actions<Slots>*>(owner);
      const auto before = cpp_allocations;
      a.admitted = a.deferred->request_media(token, connection);
      assert(cpp_allocations == before && a.deferred->loop() == PageCardCommandResult::NONE);
    };
    callbacks.seek = [](void* owner, PageBindingToken token, PageMediaSeekProof proof, uint8_t percent,
                        float duration) {
      auto& a = *static_cast<Actions<Slots>*>(owner);
      const auto before = cpp_allocations;
      ++a.seeks;
      a.last = token;
      a.seek_proof = proof;
      a.seek_percent = percent;
      a.seek_duration = duration;
      a.admitted = a.deferred->request_seek(token, proof, percent, duration);
      assert(cpp_allocations == before && a.deferred->loop() == PageCardCommandResult::NONE);
    };
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks, nullptr, &media);
    assert(port.initialize());
    actions.basic_port = &port;
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t used = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[used++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    PageGridInput<Slots> grid_input(port, renderer, commands);
    actions.renderer = &renderer;
    sender.renderer = &renderer;
    sender.view = &port;
    sender.graph = &state;
    PageRuntimeResourcesGroup<4> resources({&renderer, &commands, &media, &state});
    assert(resources.prepare(fixture.store));
    resources.activate_prepared(fixture.store);
    subscriptions.send("media_player.room", "", "playing");
    subscriptions.send("media_player.room", "media_duration", "300");
    subscriptions.send("media_player.room", "media_position", "60");
    subscriptions.send("media_player.room", "media_content_id", "track-a");
    renderer.update_visible();
    lv_obj_update_layout(page);
    auto release = [&](size_t slot, int percent) {
      pointer(device, input, widgets[slot].slider, percent, true);
      pointer(device, input, widgets[slot].slider, percent, false);
    };
    const auto child = fixture.store.card(5)->target;
    const auto reservations = arena.allocations, registrations = subscriptions.bindings.size();
    size_t retained = 0;
    for (size_t cycle = 0; cycle < 1000; ++cycle) {
      const auto sent = sender.sends;
      for (size_t slot : {size_t(1), size_t(2)}) {
        lv_obj_send_event(widgets[slot].button, LV_EVENT_PRESSED, nullptr);
        assert(commands.memory().pending == slot && sender.sends == sent);
        lv_obj_send_event(widgets[slot].button, LV_EVENT_RELEASED, nullptr);
        lv_obj_send_event(widgets[slot].button, LV_EVENT_CLICKED, nullptr);
        assert(commands.memory().pending == slot);
      }
      click(widgets[0].button);
      click(widgets[3].button);
      release(4, 65);
      assert(actions.admitted && commands.memory().pending == 5 && sender.sends == sent);
      assert(lv_slider_get_value(widgets[4].slider) == 20);  // Queue admission is not preview acceptance.
      for (int i = 0; i < 5; ++i) assert(commands.loop() == PageCardCommandResult::DISPATCHED);
      renderer.update_visible();
      assert(lv_slider_get_value(widgets[4].slider) == actions.seek_percent);
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      release(1, 70);
      click(widgets[2].button);
      assert(commands.memory().pending == 2);
      assert(commands.loop() == PageCardCommandResult::DISPATCHED &&
             commands.loop() == PageCardCommandResult::DISPATCHED);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(sender.sends == sent + 7 && arena.allocations == reservations &&
             subscriptions.bindings.size() == registrations);
      if (!cycle)
        retained = lv_live_bytes;
      else
        assert(lv_live_bytes == retained);
    }
    const auto covered_origin = renderer.binding(4);
    pointer(device, input, widgets[4].slider, 80, true);
    grid_input.cover(true);
    assert(!port.input_enabled() && renderer.bound(covered_origin));
    grid_input.cover(false);  // Cover/uncover between maintenance ticks cannot revive this drag.
    assert(!port.input_enabled());
    pointer(device, input, widgets[4].slider, 85, false);
    assert(commands.memory().pending == 0);
    assert(grid_input.settle() && port.input_enabled() && renderer.bound(covered_origin));
    pointer(device, input, widgets[4].slider, 85, false);
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[4].slider) == 20);
    release(4, 70);
    assert(commands.loop() == PageCardCommandResult::DISPATCHED);
    renderer.update_visible();
    const auto accepted_percent = lv_slider_get_value(widgets[4].slider);
    grid_input.cover(true);
    assert(grid_input.settle() && !port.input_enabled());
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[4].slider) == accepted_percent);
    // An accepted modal edit may update its bound origin while grid input is blocked.
    assert(!Actions<Slots>::valid(&actions, covered_origin) &&
           Actions<Slots>::bound(&actions, covered_origin));
    assert(port.preview_seek(covered_origin, actions.seek_proof, 60, 300));
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[4].slider) == 60);
    grid_input.cover(false);
    click(widgets[0].button);
    assert(commands.memory().pending == 1);
    grid_input.cover(true);
    assert(commands.memory().pending == 0);
    grid_input.cover(false);
    assert(grid_input.settle());
    assert(grid_input.valid(covered_origin) && grid_input.bound(covered_origin));
    const auto sends_before = sender.sends;
    for (size_t cycle = 0; cycle < 1000; ++cycle) {
      grid_input.cover(false);
      lv_obj_send_event(widgets[0].button, LV_EVENT_PRESSED, nullptr);
      grid_input.cover(true);
      grid_input.cover(false);
      lv_obj_send_event(widgets[0].button, LV_EVENT_CLICKED, nullptr);
      assert(!port.input_enabled() && commands.memory().pending == 0);
      assert(grid_input.settle() && port.input_enabled());
    }
    assert(sender.sends == sends_before && renderer.bound(covered_origin));
    // Blocking is scalar-only during draw; collecting input is deferred.
    struct CoverDraw {
      PageGridInput<Slots>* input;
      bool fired{false};
    } draw{&grid_input};
    lv_obj_add_event_cb(
        widgets[0].button,
        [](lv_event_t* event) {
          auto& state = *static_cast<CoverDraw*>(lv_event_get_user_data(event));
          state.input->cover(true);
          state.fired = true;
          assert(!state.input->settle());
        },
        LV_EVENT_DRAW_MAIN, &draw);
    lv_obj_invalidate(widgets[0].button);
    lv_refr_now(lv_display_get_default());
    assert(draw.fired && !port.input_enabled());
    lv_obj_remove_event_cb_with_user_data(widgets[0].button, nullptr, &draw);
    grid_input.cover(false);
    assert(grid_input.settle());
    sender.accepted = false;
    release(4, 80);
    assert(commands.loop() == PageCardCommandResult::FAILED);
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[4].slider) == 20);
    sender.accepted = true;
    for (int i = 0; i < 8; ++i) click(widgets[0].button);
    release(4, 80);
    assert(!actions.admitted && commands.memory().pending == 8);
    commands.cancel();
    const auto seeks = actions.seeks;
    pointer(device, input, widgets[4].slider, 50, true);
    subscriptions.send("media_player.room", "media_content_id", "track-b");
    subscriptions.send("media_player.room", "media_content_id", "track-a");
    renderer.update_visible();
    pointer(device, input, widgets[4].slider, 50, false);
    assert(actions.seeks == seeks && commands.memory().pending == 0);
    release(4, 75);
    subscriptions.send("media_player.room", "media_content_id", "track-b");
    subscriptions.send("media_player.room", "media_content_id", "track-a");
    assert(commands.loop() == PageCardCommandResult::CANCELLED);
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[4].slider) == 20);
    pointer(device, input, widgets[4].slider, 65, true);
    subscriptions.send("media_player.room", "media_content_id", "track-a");
    renderer.update_visible();
    pointer(device, input, widgets[4].slider, 65, false);
    assert(commands.loop() == PageCardCommandResult::DISPATCHED);
    renderer.update_visible();
    // An accepted send cannot install an old seek hint onto a new track.
    release(4, 70);
    const auto previews = sender.previews;
    sender.on_send = [&](const PageCardCommandSnapshot&) {
      subscriptions.send("media_player.room", "media_content_id", "track-b");
    };
    assert(commands.loop() == PageCardCommandResult::DISPATCHED && sender.previews == previews);
    sender.on_send = {};
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[4].slider) == 20);
    // An old accepted seek must not overwrite a newer physical drag.
    release(4, 65);
    pointer(device, input, widgets[4].slider, 45, true);
    const auto dragged = lv_slider_get_value(widgets[4].slider);
    assert(commands.loop() == PageCardCommandResult::DISPATCHED);
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[4].slider) == dragged);
    pointer(device, input, widgets[4].slider, 45, false);
    assert(commands.loop() == PageCardCommandResult::DISPATCHED);
    release(4, 60);
    assert(renderer.suspend());
    assert(commands.loop() == PageCardCommandResult::CANCELLED);
    assert(renderer.resume());
    renderer.update_visible();
    assert(commands.memory().pending == 0);
    const std::string large(8193, 'x');
    subscriptions.send("media_player.room", "media_content_id", large);
    renderer.update_visible();
    assert(!lv_obj_has_flag(widgets[4].slider, LV_OBJ_FLAG_CLICKABLE));
    subscriptions.send("media_player.room", "media_content_id", "track-a");
    renderer.update_visible();
    assert(lv_obj_has_flag(widgets[4].slider, LV_OBJ_FLAG_CLICKABLE));
    resources.retire_active();
  }
  lv_indev_delete(device);
  lv_obj_delete(page);
  assert(arena.live == 0);
  std::printf("%s: 1000 deferred root/child media cycles, real seek touches and track-change guards\n",
              limits.device_profile);
}
template <size_t Slots>
void timer_profile(const DeviceUiLimits& limits) {
  timer_tick = 100;
  timer_epoch = 0;
  Fixture fixture(limits, Fixture::Timers{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageTimerRuntime timer(arena, subscriptions, timer_now, timer_epoch_now);
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &timer);
  PageTimerCommandPolicy policy(state, timer);
  DeferredGridPort<Slots> sender(policy);
  PageCardCommands commands(state, sender);
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
  actions.deferred = &commands;
  actions.command_state = &state;
  auto callbacks = actions.callbacks();
  callbacks.timer = [](void* owner, PageBindingToken token, PageTimerAction command,
                       PageTimerCommandProof proof) {
    auto& action = *static_cast<Actions<Slots>*>(owner);
    ++action.commands;
    action.last = token;
    action.timer_action = command;
    const auto before = cpp_allocations;
    assert(action.deferred->request_timer(token, command, proof));
    assert(cpp_allocations == before && action.deferred->loop() == PageCardCommandResult::NONE);
    if (action.seek_page != NO_PAGE) assert(action.renderer->open_page(action.seek_page));
  };
  {
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     callbacks);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    auto absent = callbacks;
    absent.timer = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     absent, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                     nullptr, nullptr, &timer);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                  nullptr, nullptr, &timer);
    assert(port.initialize());
    actions.basic_port = &port;
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t used = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[used++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    PageGridInput<Slots> grid_input(port, renderer, commands);
    actions.renderer = &renderer;
    sender.renderer = &renderer;
    sender.view = &port;
    PageRuntimeResourcesGroup<4> resources({&renderer, &commands, &timer, &state});
    auto tap = [&](lv_obj_t* button) {
      const auto sends = sender.sends;
      click(button);
      assert(sender.sends == sends);
      sender.accepted = actions.timer_accept;
      const auto pending = commands.memory().pending;
      const auto result = commands.loop();
      assert(pending == 0 ? result == PageCardCommandResult::NONE
                          : result == (sender.accepted ? PageCardCommandResult::DISPATCHED
                                                       : PageCardCommandResult::FAILED) ||
                                result == PageCardCommandResult::CANCELLED);
    };
    PageStoreRuntime publication(limits, arena, 32768, &resources);
    assert(publication.prepare(PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(), fixture.size));
    assert(publication.activate_prepared(0, PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(),
                                         fixture.size));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "--:--") == 0);
    tap(widgets[1].button);
    assert(actions.commands == 0);
    auto report = [&]() {
      subscriptions.send("timer.room", "friendly_name", "Kitchen timer");
      subscriptions.send("timer.room", "duration", "0:05:00");
      subscriptions.send("timer.room", "remaining", "0:00:01");
    };
    report();
    subscriptions.send("timer.room", "", "idle");
    renderer.update_visible();
    assert(lv_obj_has_flag(widgets[1].icon, LV_OBJ_FLAG_HIDDEN) &&
           !lv_obj_has_flag(widgets[1].sensor_container, LV_OBJ_FLAG_HIDDEN));
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "5:00") == 0);
    tap(widgets[1].button);
    assert(actions.commands == 1 && actions.timer_action == PageTimerAction::START);
    subscriptions.send("timer.room", "", "active");
    renderer.update_visible();
    assert(lv_obj_has_state(widgets[0].button, LV_STATE_CHECKED) &&
           lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED));
    const auto timers = timer_count();
    tap(widgets[1].button);
    assert(actions.commands == 1 && std::strcmp(lv_label_get_text(widgets[1].label), "Confirm") == 0 &&
           timer_count() == timers);
    subscriptions.send("timer.room", "friendly_name", "Changed while confirming");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Confirm") == 0);
    tap(widgets[1].button);
    assert(actions.commands == 2 && actions.timer_action == PageTimerAction::CANCEL);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Changed while confirming") == 0);
    subscriptions.send("timer.room", "friendly_name", "Kitchen timer");
    renderer.update_visible();
    subscriptions.send("timer.room", "", "idle");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "5:00") == 0 &&
           std::strcmp(lv_label_get_text(widgets[2].value), "0:00") == 0);
    timer_tick += 5000;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[2].value), "5:00") == 0);
    subscriptions.send("timer.room", "", "active");
    renderer.update_visible();
    tap(widgets[1].button);
    timer_tick += 1001;
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Kitchen timer") == 0);
    tap(widgets[1].button);
    assert(actions.commands == 2);
    tap(widgets[1].button);
    assert(actions.commands == 3);
    tap(widgets[1].button);
    assert(actions.commands == 3);
    subscriptions.send("timer.room", "", "idle");
    subscriptions.send("timer.room", "", "active");  // No intervening paint.
    tap(widgets[1].button);
    assert(actions.commands == 3);
    tap(widgets[1].button);
    assert(actions.commands == 4);
    const auto child = fixture.store.card(0)->target;
    const auto root = renderer.binding(1);
    tap(widgets[1].button);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    tap(widgets[2].button);
    assert(actions.commands == 4 && std::strcmp(lv_label_get_text(widgets[2].label), "Confirm") == 0);
    assert(renderer.back());
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Kitchen timer") == 0);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    ++subscriptions.generation;
    report();
    subscriptions.send("timer.room", "", "active");
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 4);
    renderer.update_visible();
    tap(widgets[1].button);
    assert(renderer.suspend() && renderer.resume());
    renderer.update_visible();
    tap(widgets[1].button);
    assert(actions.commands == 4);
    assert(renderer.open_page(child));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 4 && !renderer.valid(root));
    assert(renderer.back());
    renderer.update_visible();
    actions.seek_page = child;
    tap(widgets[2].button);
    actions.seek_page = NO_PAGE;
    assert(actions.commands == 5 && !renderer.valid(actions.last));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    actions.timer_accept = false;
    subscriptions.send("timer.room", "remaining", "0:00:01");
    tap(widgets[2].button);
    actions.timer_accept = true;
    subscriptions.send("timer.room", "", "idle");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[2].value), "0:00") ==
           0);  // Rejected cancel retains natural-finish grace.
    subscriptions.send("timer.room", "", "active");
    renderer.update_visible();
    // Confirmation cycles retain neither a timer nor a callback, even repeatedly.
    const auto reservations = arena.allocations, channels = subscriptions.bindings.size();
    for (int i = 0; i < 1000; ++i) {
      tap(widgets[1].button);
      tap(widgets[1].button);
      assert(timer_count() == timers);
    }
    assert(arena.allocations == reservations && subscriptions.bindings.size() == channels);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto retained = lv_live_bytes, descriptors = cache.memory().descriptor_adds;
    for (int i = 0; i < 1000; ++i) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(lv_live_bytes == retained && arena.allocations == reservations &&
             subscriptions.bindings.size() == channels && cache.memory().descriptor_adds == descriptors &&
             timer_count() == timers);
    }
    subscriptions.send("timer.room", "", "idle");
    subscriptions.send("timer.room", "", "active");
    renderer.update_visible();
    const auto before_cover = actions.commands;
    tap(widgets[1].button);
    assert(actions.commands == before_cover &&
           std::strcmp(lv_label_get_text(widgets[1].label), "Confirm") == 0);
    const auto origin = renderer.binding(1);
    grid_input.cover(true);
    grid_input.cover(false);
    assert(grid_input.settle());
    renderer.update_visible();
    assert(grid_input.bound(origin) && grid_input.valid(origin));
    tap(widgets[1].button);
    assert(actions.commands == before_cover);
    tap(widgets[1].button);
    assert(actions.commands == before_cover + 1);
    resources.retire_active();
    std::printf(
        "%s: deferred timer cards 1000 cycles, countdowns, bounded confirmation and stable resources\n",
        limits.device_profile);
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
}
template <size_t Slots>
void light_temperature_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::LightTemperatures{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageLightTemperatureRuntime temperature(arena, subscriptions);
  PagePercentSliderRuntime percentages(arena, subscriptions);
  PageEntityObserverSet<2> observers;
  assert(observers.add(temperature) && observers.add(percentages));
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &observers);
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
  theme.red_percent = 50;
  Actions<Slots> actions;
  auto callbacks = actions.callbacks();
  callbacks.percent_slider = [](void*, PageBindingToken, int) {};
  callbacks.light_temperature = [](void* owner, PageBindingToken token, int kelvin) {
    auto& action = *static_cast<Actions<Slots>*>(owner);
    ++action.seeks;
    action.last = token;
    action.numeric_target = kelvin;
    if (action.seek_page != NO_PAGE) assert(action.renderer->open_page(action.seek_page));
  };
  {
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     callbacks);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    auto absent = callbacks;
    absent.light_temperature = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     absent, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                     &percentages, nullptr, &temperature);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    auto incomplete = widgets;
    incomplete[1].fill = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, incomplete, {limits.slot_count, limits.grid_columns},
                                     theme, callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                     &percentages, nullptr, &temperature);
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
                                  callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                  &percentages, nullptr, &temperature);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t used = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[used++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    PageRuntimeResourcesGroup<4> resources({&renderer, &temperature, &percentages, &state});
    PageStoreRuntime publication(limits, arena, 32768, &resources);
    assert(publication.prepare(PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(), fixture.size));
    assert(publication.activate_prepared(0, PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(),
                                         fixture.size));
    renderer.update_visible();
    lv_obj_update_layout(page);
    subscriptions.send("light.room", "friendly_name", "Temperature room");
    subscriptions.send("light.room", "color_temp_kelvin", "5500");
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[1].slider) == 0);
    subscriptions.send("light.room", "", "on");
    subscriptions.send("fan.room", "", "off");
    subscriptions.send("fan.room", "percentage", "25");
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(lv_obj_has_state(widgets[0].button,LV_STATE_CHECKED));
    assert(lv_slider_get_value(widgets[1].slider) == 50 && lv_slider_get_value(widgets[2].slider) == 77);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Temperature room") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[2].label), "Fixed") == 0);
    assert(lv_color_eq(lv_obj_get_style_bg_color(widgets[1].fill, LV_PART_MAIN), lv_color_hex(0xC8BBCA)));
    assert(lv_color_eq(lv_obj_get_style_bg_color(widgets[2].fill, LV_PART_MAIN), lv_color_hex(theme.active)));
    percent_geometry_reference(widgets[1], 50, false);
    percent_geometry_reference(widgets[2], 77, false);
    for (size_t slot : {size_t(1), size_t(2)}) {
      lv_area_t content, icon, label;
      lv_obj_get_content_coords(widgets[slot].button, &content);
      lv_obj_get_coords(widgets[slot].icon, &icon);
      lv_obj_get_coords(widgets[slot].label, &label);
      assert(icon.x1 == content.x1 && icon.y1 == content.y1 && label.x1 == content.x1 &&
             label.y2 == content.y2);
      assert(!lv_obj_has_state(widgets[slot].button, LV_STATE_CHECKED));
      assert(lv_obj_has_flag(widgets[slot].sensor_container, LV_OBJ_FLAG_HIDDEN));
    }
    {
      const auto old = lv_obj_get_style_bg_color(widgets[1].fill, LV_PART_MAIN);
      lv_obj_set_style_bg_color(widgets[1].fill, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
      lv_layer_t layer{};
      lv_draw_label_dsc_t descriptor;
      lv_draw_label_dsc_init(&descriptor);
      descriptor.base.layer = &layer;
      descriptor.color = lv_color_hex(0xFFFFFF);
      lv_draw_task_t task{};
      task.type = LV_DRAW_TASK_TYPE_LABEL;
      task.draw_dsc = &descriptor;
      lv_obj_get_coords(widgets[1].fill, &task.clip_area);
      task.area = task.clip_area;
      layer._clip_area = task.clip_area;
      lv_obj_send_event(widgets[1].label, LV_EVENT_DRAW_TASK_ADDED, &task);
      assert(lv_color_eq(descriptor.color, lv_color_hex(0x212121)));
      lv_obj_set_style_bg_color(widgets[1].fill, old, LV_PART_MAIN);
    }
    vertical_pointer(device, input, widgets[1].slider, 0, true);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "5000K") == 0);
    vertical_pointer(device, input, widgets[1].slider, 0, false);
    assert(actions.seeks == 1 && actions.numeric_target == 5000);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Temperature room") == 0);
    vertical_pointer(device, input, widgets[1].slider, 100, true);
    vertical_pointer(device, input, widgets[1].slider, 100, false);
    assert(actions.seeks == 2 && actions.numeric_target == 6000);
    const auto root_binding = renderer.binding(1);
    const auto child = fixture.store.card(0)->target;
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(lv_label_get_long_mode(widgets[1].icon) ==
           LV_LABEL_LONG_WRAP);  // Temperature clipping was restored.
    assert(lv_slider_get_value(widgets[2].slider) == 62);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(lv_slider_get_value(widgets[1].slider) == 100);
    subscriptions.send("light.room", "", "off");
    subscriptions.send("light.room", "color_temp_kelvin", "5500");
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[1].slider) == 0);
    assert(!lv_obj_has_state(widgets[0].button,LV_STATE_CHECKED));
    vertical_pointer(device, input, widgets[1].slider, 0, true);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Temperature room") ==
           0);  // No invented overlay.
    vertical_pointer(device, input, widgets[1].slider, 0, false);
    assert(actions.seeks == 3);
    subscriptions.send("light.room", "", "on");
    renderer.update_visible();
    vertical_pointer(device, input, widgets[1].slider, 75, true);
    char preview[64];
    std::snprintf(preview, sizeof(preview), "%s", lv_label_get_text(widgets[1].label));
    subscriptions.send("light.room", "color_temp_kelvin", "5494");
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[1].slider) == 49 &&
           std::strcmp(lv_label_get_text(widgets[1].label), preview) == 0);
    subscriptions.send("light.room", "friendly_name", "Renamed light");
    renderer.update_visible();
    assert(std::strcmp(lv_label_get_text(widgets[1].label), preview) == 0);
    vertical_pointer(device, input, widgets[1].slider, 75, false);
    assert(actions.seeks == 4);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Renamed light") == 0);
    vertical_pointer(device, input, widgets[1].slider, 75, true);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    vertical_pointer(device, input, widgets[1].slider, 75, false);
    assert(actions.seeks == 4 && !renderer.valid(root_binding));
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    vertical_pointer(device, input, widgets[1].slider, 75, true);
    ++subscriptions.generation;
    renderer.update_visible();
    vertical_pointer(device, input, widgets[1].slider, 75, false);
    assert(actions.seeks == 4);
    subscriptions.send("light.room", "", "on");
    subscriptions.send("light.room", "color_temp_kelvin", "5500");
    renderer.update_visible();
    vertical_pointer(device, input, widgets[1].slider, 75, true);
    assert(renderer.suspend() && renderer.resume());
    renderer.update_visible();
    vertical_pointer(device, input, widgets[1].slider, 75, false);
    assert(actions.seeks == 4);
    actions.seek_page = child;
    vertical_pointer(device, input, widgets[1].slider, 100, true);
    vertical_pointer(device, input, widgets[1].slider, 100, false);
    actions.seek_page = NO_PAGE;
    assert(actions.seeks == 5 && !renderer.valid(actions.last));
    renderer.update_visible();
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
    temperature.retire_active();
    percentages.retire_active();
    std::printf("%s: light temperature 1000 cycles, Kelvin colors, drag labels and stable resources\n",
                limits.device_profile);
  }
  assert(arena.live == 0);
  lv_indev_delete(device);
  lv_obj_delete(page);
}
template <size_t Slots>
void deferred_slider_profile(const DeviceUiLimits& limits) {
  const char* cards[] = {"fan.speed;Speed;Auto;Auto;;;slider;;",
                         "input_number.level;Level;Auto;Auto;;;slider;;",
                         "light.temp;Temperature;Auto;Auto;;2000-6500;light_temperature;;"};
  for (size_t kind = 0; kind < 3; ++kind) {
    Fixture fixture(limits, std::string_view(cards[kind]));
    Arena arena;
    Subscriptions subscriptions;
    PageCardDependencies dependencies;
    PagePercentSliderRuntime percentages(arena, subscriptions);
    PageNumberSliderRuntime numbers(arena, subscriptions);
    PageLightTemperatureRuntime temperature(arena, subscriptions);
    PageEntityObserverSet<3> observers;
    assert(observers.add(percentages) && observers.add(numbers) && observers.add(temperature));
    PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &observers);
    PageSliderCommandPolicy policy(state, percentages, numbers, temperature);
    DeferredGridPort<Slots> sender(policy);
    PageCardCommands commands(state, sender);
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
    PointerInput input;
    auto* device = lv_indev_create();
    lv_indev_set_type(device, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(device, lv_display_get_default());
    lv_indev_set_user_data(device, &input);
    lv_indev_set_read_cb(device, PointerInput::read);
    {
      PageBasicTheme theme;
      Actions<Slots> actions;
      actions.deferred = &commands;
      actions.command_state = &state;
      auto callbacks = actions.callbacks();
      callbacks.toggle = [](void* owner, PageBindingToken t, std::string_view) {
        auto& a = *static_cast<Actions<Slots>*>(owner);
        const auto before = cpp_allocations;
        assert(a.deferred->request_slider_toggle(t, a.command_state->connection_generation()));
        assert(cpp_allocations == before && a.deferred->loop() == PageCardCommandResult::NONE);
      };
      callbacks.percent_slider = [](void* owner, PageBindingToken t, int value) {
        auto& a = *static_cast<Actions<Slots>*>(owner);
        const auto before = cpp_allocations;
        assert(a.deferred->request_percent(t, a.command_state->connection_generation(), value));
        assert(cpp_allocations == before && a.deferred->loop() == PageCardCommandResult::NONE);
      };
      callbacks.number_slider = [](void* owner, PageBindingToken t, double value,
                                   const number_slider::Metadata& m) {
        auto& a = *static_cast<Actions<Slots>*>(owner);
        const auto before = cpp_allocations;
        assert(a.deferred->request_number(t, a.command_state->connection_generation(), value, m));
        assert(cpp_allocations == before && a.deferred->loop() == PageCardCommandResult::NONE);
      };
      callbacks.light_temperature = [](void* owner, PageBindingToken t, int value) {
        auto& a = *static_cast<Actions<Slots>*>(owner);
        const auto before = cpp_allocations;
        assert(a.deferred->request_temperature(t, a.command_state->connection_generation(), value));
        assert(cpp_allocations == before && a.deferred->loop() == PageCardCommandResult::NONE);
      };
      PageLabelRefresh cache(arena);
      PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                    callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                    &percentages, &numbers, &temperature);
      assert(port.initialize());
      actions.basic_port = &port;
      std::array<lv_obj_t*, Slots * 4> labels{};
      size_t used = 0;
      for (const auto& w : widgets)
        for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[used++] = label;
      assert(cache.initialize(labels.data(), labels.size()));
      PageRendererRuntime<Slots> renderer(limits, arena, port);
      PageGridInput<Slots> grid_input(port, renderer, commands);
      actions.renderer = &renderer;
      sender.renderer = &renderer;
      sender.view = &port;
      PageRuntimeResourcesGroup<6> resources(
          {&renderer, &commands, &percentages, &numbers, &temperature, &state});
      assert(resources.prepare(fixture.store));
      resources.activate_prepared(fixture.store);
      subscriptions.send("fan.speed", "", "on");
      subscriptions.send("fan.speed", "percentage", "42");
      subscriptions.send("input_number.level", "", "12");
      subscriptions.send("input_number.level", "min", "0");
      subscriptions.send("input_number.level", "max", "20");
      subscriptions.send("input_number.level", "step", "0.5");
      subscriptions.send("light.temp", "", "on");
      subscriptions.send("light.temp", "color_temp_kelvin", "3200");
      renderer.update_visible();
      lv_obj_update_layout(page);
      const auto allocations = arena.allocations, registrations = subscriptions.bindings.size();
      const auto expected = kind == 0   ? PageCardCommandKind::PERCENT
                            : kind == 1 ? PageCardCommandKind::NUMBER
                                        : PageCardCommandKind::TEMPERATURE;
      auto release = [&]() {
        vertical_pointer(device, input, widgets[0].slider, 50, true);
        vertical_pointer(device, input, widgets[0].slider, 65, false);
        assert(commands.memory().pending == 1);
      };
      for (size_t cycle = 0; cycle < 1000; ++cycle) {
        release();
        assert(sender.sends == cycle);
        assert(commands.loop() == PageCardCommandResult::DISPATCHED && sender.last == expected);
        renderer.update_visible();
        lv_obj_update_layout(page);
        assert(commands.memory().pending == 0 && subscriptions.bindings.size() == registrations);
        assert(arena.allocations == allocations);
      }
      sender.accepted = false;
      release();
      if (kind == 2) subscriptions.send("light.temp", "", "on");
      assert(commands.loop() == PageCardCommandResult::FAILED);
      renderer.update_visible();
      const ConfigurationHandle handle{fixture.store.generation(), 0};
      const int restored = kind == 0   ? percentages.read(handle).percent
                           : kind == 1 ? numbers.read(handle).position
                                       : temperature.read(handle).percent;
      assert(lv_slider_get_value(widgets[0].slider) == restored);
      assert(kind != 0 || restored == 42);
      assert(kind != 1 || restored == 24);
      assert(kind != 2 || temperature.read(handle).display_kelvin == 3200);
      release();
      assert(renderer.suspend());
      assert(commands.loop() == PageCardCommandResult::CANCELLED);
      assert(renderer.resume());
      renderer.update_visible();
      assert(commands.memory().pending == 0 && sender.sends == 1001);
      sender.accepted = true;
      const auto origin = renderer.binding(0);
      const auto sent = sender.sends;
      const auto current = [&] {
        return kind == 0   ? percentages.read(handle).percent
               : kind == 1 ? numbers.read(handle).position
                           : temperature.read(handle).percent;
      };
      const auto reported = current();
      vertical_pointer(device, input, widgets[0].slider, 80, true);
      grid_input.cover(true);
      assert(!grid_input.valid(origin) && grid_input.bound(origin) && commands.memory().pending == 0);
      grid_input.cover(false);
      vertical_pointer(device, input, widgets[0].slider, 85, false);
      assert(commands.memory().pending == 0 && !port.input_enabled());
      assert(grid_input.settle());
      vertical_pointer(device, input, widgets[0].slider, 85, false);
      renderer.update_visible();
      assert(current() == reported && !numbers.read(handle).gesture && !temperature.read(handle).gesture);
      assert(lv_slider_get_value(widgets[0].slider) == reported && grid_input.valid(origin));
      // Released-but-unsent previews roll back before a fast cover/uncover can dispatch them.
      release();
      grid_input.cover(true);
      assert(commands.memory().pending == 0);
      grid_input.cover(false);
      assert(grid_input.settle());
      renderer.update_visible();
      assert(commands.loop() == PageCardCommandResult::NONE && sender.sends == sent && current() == reported);
      release();
      assert(commands.loop() == PageCardCommandResult::DISPATCHED);
      const auto accepted = current();
      grid_input.cover(true);
      assert(grid_input.settle());
      grid_input.cover(false);
      renderer.update_visible();
      assert(current() == accepted && grid_input.bound(origin) && arena.allocations == allocations);
      resources.retire_active();
    }
    lv_indev_delete(device);
    lv_obj_delete(page);
    assert(arena.live == 0);
  }
  std::printf("%s: deferred grid sliders, 1000 real pointer releases per kind, failure/cover rollback\n",
              limits.device_profile);
}
template <size_t Slots>
void number_slider_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::NumberSliders{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageNumberSliderRuntime levels(arena, subscriptions);
  PagePercentSliderRuntime percentages(arena, subscriptions);
  PageEntityObserverSet<2> observers;
  assert(observers.add(levels) && observers.add(percentages));
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &observers);
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
  callbacks.percent_slider = [](void*, PageBindingToken, int) {};
  callbacks.number_slider = [](void* owner, PageBindingToken token, double target,
                               const number_slider::Metadata& metadata) {
    auto& action = *static_cast<Actions<Slots>*>(owner);
    ++action.seeks;
    action.last = token;
    action.numeric_target = target;
    action.numeric_metadata = metadata;
    if (action.seek_page != NO_PAGE) assert(action.renderer->open_page(action.seek_page));
  };
  {
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     callbacks);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    auto absent = callbacks;
    absent.number_slider = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     absent, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                     &percentages, &levels);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    auto incomplete = widgets;
    incomplete[1].slider = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, incomplete, {limits.slot_count, limits.grid_columns},
                                     theme, callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                     &percentages, &levels);
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
                                  callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                  &percentages, &levels);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t used = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[used++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    PageRuntimeResourcesGroup<4> resources({&renderer, &levels, &percentages, &state});
    PageStoreRuntime publication(limits, arena, 32768, &resources);
    assert(publication.prepare(PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(), fixture.size));
    assert(publication.activate_prepared(0, PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(),
                                         fixture.size));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(!lv_obj_has_flag(widgets[1].slider, LV_OBJ_FLAG_CLICKABLE));
    auto report = [&]() {
      for (const auto* entity : {"number.room", "input_number.room", "number.other"}) {
        subscriptions.send(entity, "min", "-10");
        subscriptions.send(entity, "max", "10");
        subscriptions.send(entity, "step", "0.1");
        subscriptions.send(entity, "", "5");
        subscriptions.send(entity, "unit_of_measurement", "kWh");
      }
    };
    report();
    subscriptions.send("fan.mixed", "", "on");
    subscriptions.send("fan.mixed", "percentage", "25");
    subscriptions.send("number.room", "friendly_name", "Room range");
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "Room range") == 0);
    for (size_t slot = 1; slot <= 4; ++slot) {
      assert(lv_slider_get_max_value(widgets[slot].slider) == 200 &&
             lv_slider_get_value(widgets[slot].slider) == 150);
      assert(!lv_obj_has_state(widgets[slot].button, LV_STATE_CHECKED));
      assert(lv_obj_has_flag(widgets[slot].sensor_container, LV_OBJ_FLAG_HIDDEN));
      percent_geometry_reference(widgets[slot], 75, false);
    }
    vertical_pointer(device, input, widgets[1].slider, 0, true);
    assert(!lv_obj_has_flag(widgets[1].sensor_container, LV_OBJ_FLAG_HIDDEN));
    assert(std::strcmp(lv_label_get_text(widgets[1].value), "-10.0") == 0);
    assert(std::strcmp(lv_label_get_text(widgets[1].unit), "kWh") == 0);
    vertical_pointer(device, input, widgets[1].slider, 0, false);
    assert(actions.seeks == 1 && actions.numeric_target == -10 && actions.numeric_metadata.step == 0.1);
    assert(lv_obj_has_flag(widgets[1].sensor_container, LV_OBJ_FLAG_HIDDEN));
    vertical_pointer(device, input, widgets[1].slider, 100, true);
    vertical_pointer(device, input, widgets[1].slider, 100, false);
    assert(actions.seeks == 2 && actions.numeric_target == 10);
    const auto root_binding = renderer.binding(1);
    const auto child = fixture.store.card(0)->target;
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(lv_slider_get_value(widgets[1].slider) == 150);
    assert(lv_slider_get_max_value(widgets[4].slider) == 100 && lv_slider_get_value(widgets[4].slider) == 25);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(lv_slider_get_value(widgets[1].slider) == 200);  // Released root preview survives navigation.
    subscriptions.send("number.room", "", "5");
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[1].slider) == 150);
    subscriptions.send("input_number.room", "", "7");
    vertical_pointer(device, input, widgets[1].slider, 75, true);
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[2].slider) == 170);
    const auto thumb = lv_slider_get_value(widgets[1].slider);
    subscriptions.send("number.room", "", "5");
    renderer.update_visible();
    assert(lv_slider_get_value(widgets[1].slider) == thumb);  // State preserves numeric drag.
    subscriptions.send("number.room", "step", "0.1");
    renderer.update_visible();
    assert(lv_obj_has_flag(widgets[1].sensor_container, LV_OBJ_FLAG_HIDDEN));
    vertical_pointer(device, input, widgets[1].slider, 75, false);
    assert(actions.seeks == 3);  // Equivalent metadata keeps the physical release valid.
    vertical_pointer(device, input, widgets[1].slider, 75, true);
    subscriptions.send("number.room", "step", "0.2");
    renderer.update_visible();
    vertical_pointer(device, input, widgets[1].slider, 75, false);
    assert(actions.seeks == 3);
    report();
    renderer.update_visible();
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
    assert(actions.seeks == 3);
    assert(!lv_obj_has_flag(widgets[1].slider, LV_OBJ_FLAG_CLICKABLE));
    report();
    renderer.update_visible();
    vertical_pointer(device, input, widgets[1].slider, 75, true);
    assert(renderer.suspend() && renderer.resume());
    renderer.update_visible();
    vertical_pointer(device, input, widgets[1].slider, 75, false);
    assert(actions.seeks == 3);
    actions.seek_page = child;
    vertical_pointer(device, input, widgets[1].slider, 100, true);
    vertical_pointer(device, input, widgets[1].slider, 100, false);
    actions.seek_page = NO_PAGE;
    assert(actions.seeks == 4 && !renderer.valid(actions.last));
    renderer.update_visible();
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    subscriptions.send("fan.mixed","","off");subscriptions.send("number.room","","1");
    renderer.update_visible();assert(lv_obj_has_state(widgets[0].button,LV_STATE_CHECKED));
    subscriptions.send("number.room","","5");renderer.update_visible();
    assert(!lv_obj_has_state(widgets[0].button,LV_STATE_CHECKED));
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
    percentages.retire_active();
    std::printf("%s: number sliders 1000 cycles, full ranges, gesture cancellation and stable resources\n",
                limits.device_profile);
  }
  assert(arena.live == 0);
  lv_indev_delete(device);
  lv_obj_delete(page);
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
    PageRuntimeResourcesGroup<3> resources({&renderer, &levels, &state});
    PageStoreRuntime publication(limits, arena, 32768, &resources);
    assert(publication.prepare(PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(), fixture.size));
    assert(publication.activate_prepared(0, PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(),
                                         fixture.size));
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
    assert(levels.read({0, root_binding.card}).preview);
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
    assert(lv_slider_get_value(widgets[1].slider) == 25 && !levels.read({0, root_binding.card}).preview);
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
    actions.basic_port = &port;
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
    actions.basic_port = &port;
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
    port.set_input_covered(true);
    assert(port.settle_input());
    assert(!Actions<Slots>::valid(&actions, group_binding) && renderer.bound(group_binding));
    assert(port.preview_group(group_binding, true));
    port.set_input_covered(false);
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
    port.set_input_covered(true);
    assert(port.settle_input());
    assert(!Actions<Slots>::valid(&actions, origin) && renderer.bound(origin));
    assert(port.preview_volume(origin, 50));
    port.set_input_covered(false);
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
    // Accepted preview updates after reconnect must not refresh an older press.
    for (size_t slot : {1u, 5u}) {
      const auto commands = actions.commands;
      lv_obj_send_event(widgets[slot].button, LV_EVENT_PRESSED, nullptr);
      ++subscriptions.generation;
      media.loop();
      state.invalidate_disconnected_state();
      subscriptions.send("media_player.room", "", "playing");
      subscriptions.send("media_player.room", "volume_level", ".2");
      subscriptions.send("media_player.room", "supported_features", "4");
      subscriptions.send("media_player.room", "group_members", "['media_player.room']");
      renderer.update_visible();
      assert(slot == 1 ? port.preview_volume(renderer.binding(slot), 30)
                       : port.preview_group(renderer.binding(slot), true));
      lv_obj_send_event(widgets[slot].button, LV_EVENT_CLICKED, nullptr);
      assert(actions.commands == commands);
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
template <size_t Slots, size_t Images>
void image_tile_profile(const DeviceUiLimits& limits) {
  struct Document {
    std::array<uint8_t, 8192> bytes{};
    alignas(std::max_align_t) std::array<uint8_t, 32768> arena{};
    size_t size{0};
    PageStore store;
    explicit Document(const DeviceUiLimits& limits) {
      PanelConfigWriter w(bytes.data(), bytes.size());
      assert(w.begin() == PanelConfigStatus::OK);
      assert(w.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                     std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
      const auto add = [&](uint8_t slot, const std::string& config) {
        assert(w.append_button(slot, reinterpret_cast<const uint8_t*>(config.data()), config.size()) ==
               PanelConfigStatus::OK);
      };
      add(1, ";Rooms;Home;Auto;indicator;;subpage;;");
      add(2,
          "camera.front;;Auto;Auto;;;image;;image_modal_refresh_mode=activity,image_modal_refresh_trigger="
          "binary_sensor.motion");
      add(3, "camera.other;Configured;Lightbulb;Auto;;;image;;image_label,image_icon,image_modal_mode=fit");
      add(4, "image.still;Still;Auto;Auto;;;image;;image_label");
      add(5, ";;;;;;image;;");
      add(6, "sensor.unsupported;;;;;;image;;");
      const std::string child =
          "~B,1,2|image,camera.front,Child,Auto,Auto,,,,image_label%2Cimage_icon|,light.child,Light,Auto,"
          "Auto,,,,";
      assert(w.append_subpage(1, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
             PanelConfigStatus::OK);
      std::string order;
      for (size_t i = 1; i <= limits.slot_count; ++i) {
        if (i > 1) order += ",";
        order += std::to_string(i);
      }
      const std::string key = "button_order";
      assert(w.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                              reinterpret_cast<const uint8_t*>(order.data()),
                              order.size()) == PanelConfigStatus::OK);
      assert(w.finish(&size) == PanelConfigStatus::OK);
      assert(PageStore::stage_v1(bytes.data(), size, limits, 17, arena.data(), arena.size(), &store) ==
             PageStoreStatus::OK);
    }
  } document(limits);
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageEntityObserverSet<1> observers;
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192}, &observers);
  Actions<Slots> actions;
  struct Origin final : PageImageOrigin {
    Actions<Slots>& actions;
    Subscriptions& subscriptions;
    Origin(Actions<Slots>& a, Subscriptions& s) : actions(a), subscriptions(s) {}
    bool valid(PageBindingToken t) const override {
      return actions.renderer && actions.renderer->bound(t);
    }
    bool connected() const override {
      return true;
    }
    uint32_t connection_generation() const override {
      return subscriptions.generation;
    }
  } origin(actions, subscriptions);
  struct Requests final : PageImageRequests {
    struct Job {
      void* owner{nullptr};
      Receive receive{nullptr};
      uint64_t request{0};
      size_t slot{0};
      void reply() const {
        if (receive) receive(owner, slot, request, true, false);
      }
    };
    std::array<Job, Images> jobs{};
    size_t starts{0}, releases{0};
    bool start(size_t slot, uint64_t request, std::string_view, PageImageOptions, void* owner,
               Receive receive) override {
      ++starts;
      jobs[slot] = {owner, receive, request, slot};
      return true;
    }
    void cancel(size_t slot) override {
      jobs[slot].reply();
    }
    void release_surface(size_t) override {
      ++releases;
    }
  } requests;
  PageImageRuntime<Images> images(requests, origin);
  struct Environment final : PageImageTileEnvironment {
    std::string_view base_url() const override {
      return "https://ha";
    }
    uint32_t endpoint_generation() const override {
      return 1;
    }
    bool request_allowed(uint16_t, uint16_t) const override {
      return true;
    }
    bool visible(PageBindingToken) const override {
      return true;
    }
    uint16_t maximum_source_side() const override {
      return 400;
    }
  } environment;
  PageImageTileRuntime<Images> tiles(state, images, environment, fan_status_now);
  assert(observers.add(tiles));
  struct View final : PageImageTileView {
    PageImageTileRuntime<Images>& tiles;
    std::array<uint16_t, 16 * 16> pixels{};
    lv_image_dsc_t descriptor{};
    explicit View(PageImageTileRuntime<Images>& t) : tiles(t) {
      pixels.fill(0xf800);
      descriptor.header.magic = LV_IMAGE_HEADER_MAGIC;
      descriptor.header.cf = LV_COLOR_FORMAT_RGB565;
      descriptor.header.w = descriptor.header.h = 16;
      descriptor.header.stride = 32;
      descriptor.data = reinterpret_cast<const uint8_t*>(pixels.data());
      descriptor.data_size = pixels.size() * 2;
    }
    bool ready() const override {
      return true;
    }
    void bind(PageBindingToken token, PageImageTileSource source) override {
      tiles.bind(token, source);
    }
    void unbind(PageBindingToken token) override {
      tiles.unbind(token);
    }
    void size(PageBindingToken token, uint16_t w, uint16_t h, uint32_t bg) override {
      tiles.size(token, w, h, bg);
    }
    PageImageTileSurface surface(PageBindingToken token) override {
      const auto p = tiles.snapshot(token);
      return {p, p.asset ? &descriptor : nullptr};
    }
    uint64_t revision() const override {
      return tiles.revision();
    }
  } view(tiles);
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  const size_t row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < row_count; ++i) rows[i] = LV_GRID_FR(1);
  rows[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_size(page, 480, 480);
  lv_obj_set_grid_dsc_array(page, columns.data(), rows.data());
  std::array<PageBasicWidgets, Slots> widgets{};
  for (auto& w : widgets) {
    w.button = lv_button_create(page);
    lv_obj_set_style_pad_all(w.button, 8, 0);
    w.forecast = lv_obj_create(w.button);
    w.icon = lv_label_create(w.button);
    w.label = lv_label_create(w.button);
    lv_obj_align(w.label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    w.sensor_container = lv_obj_create(w.button);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
  }
  PageBasicTheme theme;
  theme.sensor_font = theme.icon_font = theme.label_font = &lv_font_montserrat_14;
  theme.sensor_surface = 0xF5F5F5;
  theme.image_loading_surface = 0xFFFFFF;
  theme.text = 0x333333;
  theme.translate = [](const char* key) -> const char* {
    return !std::strcmp(key, "Loading") ? "LOADING" : !std::strcmp(key, "Unavailable") ? "UNAVAILABLE" : key;
  };
  auto callbacks = actions.callbacks();
  callbacks.image = Actions<Slots>::action;
  {
    PageLabelRefresh cache(arena);
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                  &view);
    assert(port.initialize());
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t label_index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[label_index++] = label;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(renderer.prepare(document.store) && state.prepare(document.store));
    renderer.activate_prepared(document.store);
    state.activate_prepared(document.store);
    renderer.update_visible();
    lv_obj_update_layout(page);
    renderer.refresh();
    renderer.update_visible();
    const auto front = renderer.binding(1), other = renderer.binding(2);
    const auto slot = tiles.find(front);
    assert(slot < Images);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "LOADING") == 0 &&
           !lv_obj_has_flag(widgets[1].forecast, LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_style_bg_opa(widgets[1].forecast, LV_PART_MAIN) == LV_OPA_COVER);
    if (Images == 2) assert(!std::strcmp(lv_label_get_text(widgets[3].label), "Too many"));
    assert(!std::strcmp(lv_label_get_text(widgets[4].label), "Configure"));
    assert(!std::strcmp(lv_label_get_text(widgets[5].label), "UNAVAILABLE"));
    assert((lv_color_to_u32(lv_obj_get_style_bg_color(widgets[1].forecast, LV_PART_MAIN)) & 0xFFFFFF) ==
           0xFFFFFF);
    assert((lv_color_to_u32(lv_obj_get_style_text_color(widgets[1].label, LV_PART_MAIN)) & 0xFFFFFF) ==
           0x333333);
    click(widgets[1].button);
    assert(actions.commands == 0);
    subscriptions.send("camera.front", "access_token", "test-token");
    subscriptions.send("camera.other", "access_token", "test-token");
    subscriptions.send("camera.front", "friendly_name", "Front Camera");
    images.loop();
    tiles.loop();
    renderer.update_visible();
    assert(requests.starts == 2);
    for (const auto& job : requests.jobs) job.reply();
    tiles.loop();
    renderer.update_visible();
    lv_obj_update_layout(page);
    assert(!lv_obj_has_flag(widgets[1].forecast, LV_OBJ_FLAG_HIDDEN) &&
           lv_obj_has_flag(widgets[1].icon, LV_OBJ_FLAG_HIDDEN) &&
           lv_obj_has_flag(widgets[1].label, LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(widgets[2].forecast, LV_OBJ_FLAG_HIDDEN) &&
           !lv_obj_has_flag(widgets[2].icon, LV_OBJ_FLAG_HIDDEN) &&
           !lv_obj_has_flag(widgets[2].label, LV_OBJ_FLAG_HIDDEN));
    assert(!std::strcmp(lv_label_get_text(widgets[2].label), "Configured"));
    assert(lv_obj_get_width(widgets[1].forecast) == lv_obj_get_width(widgets[1].button));
    assert(lv_obj_get_style_text_color(widgets[2].label, LV_PART_MAIN).red == 255);
    assert(actions.paint_reads > 0);
    // A graph report can replace ready policy from a draw-task callback. Never
    // queue this frame's label shadow after its image has become unavailable.
    struct DrawChange {
      Subscriptions* subscriptions;
      bool changed{false};
      size_t images{0}, shadows{0};
    } draw{&subscriptions};
    const auto change = [](lv_event_t* event) {
      auto& draw = *static_cast<DrawChange*>(lv_event_get_user_data(event));
      const auto type = lv_draw_task_get_type(lv_event_get_draw_task(event));
      if (type == LV_DRAW_TASK_TYPE_IMAGE) {
        ++draw.images;
        if (!draw.changed) {
          draw.changed = true;
          draw.subscriptions->send("camera.other", "", "unavailable");
        }
      } else if (type == LV_DRAW_TASK_TYPE_LABEL)
        ++draw.shadows;
    };
    lv_obj_add_flag(widgets[2].forecast, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    assert(lv_obj_add_event_cb(widgets[2].forecast, change, LV_EVENT_DRAW_TASK_ADDED, &draw));
    lv_obj_invalidate(widgets[2].forecast);
    lv_refr_now(lv_display_get_default());
    assert(draw.changed && draw.images > 0 && draw.shadows == 0);
    lv_obj_remove_event_cb_with_user_data(widgets[2].forecast, change, &draw);
    lv_obj_clear_flag(widgets[2].forecast, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    subscriptions.send("camera.other", "", "idle");
    tiles.loop();
    for (const auto& job : requests.jobs) job.reply();
    tiles.loop();
    renderer.update_visible();
    click(widgets[1].button);
    assert(actions.commands == 1);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    ++subscriptions.generation;
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 1);
    subscriptions.send("camera.front", "access_token", "new-token");
    subscriptions.send("camera.other", "access_token", "new-token");
    images.loop();
    tiles.loop();
    for (const auto& job : requests.jobs) job.reply();
    tiles.loop();
    renderer.update_visible();
    // Draw ownership is valid while the input gate is closed during paint.
    lv_refr_now(lv_display_get_default());
    const auto child = document.store.card(0)->target;
    assert(child != NO_PAGE);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == 1);
    assert(tiles.find(renderer.binding(1)) < Images);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    // Warm both banks and draw caches before comparing retained resources.
    for (size_t n = 0; n < 2; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
    }
    const auto cpp = cpp_allocations, lv = lv_live_bytes, owner_bytes = arena.live,
               registrations = subscriptions.bindings.size(), starts = requests.starts;
    for (size_t n = 0; n < 1000; ++n) {
      assert(renderer.open_page(child));
      renderer.update_visible();
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
    }
    assert(cpp_allocations == cpp && lv_live_bytes == lv && arena.live == owner_bytes &&
           subscriptions.bindings.size() == registrations && requests.starts == starts);
    images.loop();
    tiles.loop();
    for (const auto& job : requests.jobs) job.reply();
    tiles.loop();
    renderer.update_visible();
    subscriptions.send("camera.front", "", "unavailable");
    tiles.loop();
    renderer.update_visible();
    assert(!lv_obj_has_flag(widgets[1].forecast, LV_OBJ_FLAG_HIDDEN) &&
           !std::strcmp(lv_label_get_text(widgets[1].label), "UNAVAILABLE") &&
           tiles.snapshot(renderer.binding(1)).asset == 0);
    // Old descriptors can be invalid before the next renderer paint. The draw
    // callback must reject policy/asset changes before touching their bytes.
    lv_obj_invalidate(widgets[2].forecast);
    renderer.suspend();
    view.descriptor.data = nullptr;
    lv_refr_now(lv_display_get_default());
    assert(!tiles.snapshot(other).bound);
    renderer.retire_active();
    state.retire_active();
    std::printf(
        "%s: image tiles, shared %zu-image pool, reconnect, deferred cleanup and 1000 stable navigation "
        "cycles passed\n",
        limits.device_profile, Images);
  }
  observers.remove(tiles);
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
    actions.basic_port = &port;
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
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    requests.online = false;
    ++requests.connection;
    ++subscriptions.generation;
    state.invalidate_disconnected_state();
    forecasts.loop();
    renderer.update_visible();
    const auto before_cached_open = actions.commands;
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == before_cached_open + 1);
    assert(std::strcmp(lv_label_get_text(widgets[1].label), "") == 0);
    lv_obj_set_size(page, 600, 320);
    lv_obj_update_layout(page);
    lv_refr_now(lv_display_get_default());
    renderer.retire_active();
    state.retire_active();
    forecasts.retire_active();
    forecasts.loop();
    std::printf(
        "%s: forecast 1000 cycles, fixed frames/canvases, 0 C++/arena/label/glyph/request additions\n",
        limits.device_profile);
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
template <size_t Slots>
void qr_tile_profile(const DeviceUiLimits& limits) {
  struct Document {
    std::array<uint8_t, 8192> bytes{};
    alignas(std::max_align_t) std::array<uint8_t, 32768> arena{};
    PageStore store;
    size_t size{0};
    explicit Document(const DeviceUiLimits& limits) {
      PanelConfigWriter writer(bytes.data(), bytes.size());
      assert(writer.begin() == PanelConfigStatus::OK);
      assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                          std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
      const std::string cards[] = {
          ";Rooms;Auto;Auto;indicator;;subpage;;",
          "switch.guest;Tile;Auto;Auto;;;wifi_qr_card;;ssid64=R3Vlc3Q,security=open,card_off_color=336699",
          "switch.guest;Connect;Auto;Auto;;;wifi_qr;;ssid64=R3Vlc3Q,security=open,wifi_tabs=qr|guest",
          "switch.guest;Invalid;Auto;Auto;;;wifi_qr_card;;ssid64=bad,security=WPA",
          "light.room;Lamp;Auto;Auto;;;;",
          ";Custom;Auto;Auto;;;wifi_qr;;ssid64=R3Vlc3Q,security=open"};
      for (size_t i = 0; i < 6; ++i)
        assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i].data()),
                                    cards[i].size()) == PanelConfigStatus::OK);
      const std::string child =
          "~B,1,2|wifi_qr_card,switch.guest,Child,Auto,Auto,,,,ssid64%3DR3Vlc3Q%2Csecurity%3Dopen|wifi_qr,"
          "switch.guest,Connect,Auto,Auto,,,,ssid64%3DR3Vlc3Q%2Csecurity%3Dopen";
      assert(writer.append_subpage(1, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
             PanelConfigStatus::OK);
      std::string order;
      for (size_t i = 1; i <= limits.slot_count; ++i) {
        if (i > 1) order += ",";
        order += std::to_string(i);
      }
      const std::string key = "button_order";
      assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                                   reinterpret_cast<const uint8_t*>(order.data()),
                                   order.size()) == PanelConfigStatus::OK);
      assert(writer.finish(&size) == PanelConfigStatus::OK);
      assert(PageStore::stage_v1(bytes.data(), size, limits, 17, arena.data(), arena.size(), &store) ==
             PageStoreStatus::OK);
    }
  } document(limits);
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {64, 64, 4096});
  Actions<Slots> actions;
  struct Origin final : PageQrOrigin {
    Actions<Slots>& actions;
    explicit Origin(Actions<Slots>& a) : actions(a) {}
    bool valid(PageBindingToken token) const override {
      return actions.renderer && actions.renderer->bound(token);
    }
  } origin(actions);
  // The real encoder/pixel parity is tested separately. This deterministic
  // module pattern makes card placement and draw admission directly observable.
  struct Encoder final : PageQrEncoder {
    size_t calls{0};
    int side{0};
    bool compact{false};
    bool encode(std::string_view payload, int size, bool small, PageQrMatrix& matrix) override {
      ++calls;
      side = size;
      compact = small;
      assert(payload == "WIFI:T:nopass;S:Guest;H:false;;");
      matrix.clear();
      matrix.layout = page_qr_layout(1, size, small);
      for (unsigned y = 0; y < matrix.layout.modules; ++y)
        for (unsigned x = 0; x < matrix.layout.modules; ++x) matrix.dark(x, y, (x + y) % 2 == 0);
      return matrix.layout.valid();
    }
  } encoder;
  PageLvglQrFrames frames(arena);
  PageQrRuntime<> qrs(arena, encoder, frames, origin, Slots);
  assert(qrs.initialize());
  PagePooledQrTileView view(qrs, frames);
  auto* page = lv_obj_create(lv_screen_active());
  lv_obj_set_size(page, 480, 480);
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  const size_t row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  for (size_t i = 0; i < row_count; ++i) rows[i] = LV_GRID_FR(1);
  rows[row_count] = LV_GRID_TEMPLATE_LAST;
  lv_obj_set_grid_dsc_array(page, columns.data(), rows.data());
  std::array<PageBasicWidgets, Slots> widgets{};
  for (auto& w : widgets) {
    w.button = lv_button_create(page);
    lv_obj_set_style_pad_all(w.button, 8, 0);
    w.forecast = lv_obj_create(w.button);
    w.icon = lv_label_create(w.button);
    w.label = lv_label_create(w.button);
    w.sensor_container = lv_obj_create(w.button);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
  }
  PageBasicTheme theme;
  theme.sensor_font = theme.icon_font = theme.label_font = &lv_font_montserrat_14;
  theme.translate = [](const char* key) -> const char* {
    return !std::strcmp(key, "Connect") ? "CONNECT" : key;
  };
  auto callbacks = actions.callbacks();
  callbacks.wifi_qr = Actions<Slots>::action;
  const auto make_port = [&](PageBasicActions cb, PageQrTileView* tiles) {
    return new PageBasicLvglPort<Slots>(arena, state, widgets, {limits.slot_count, limits.grid_columns},
                                        theme, cb, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                        nullptr, nullptr, nullptr, tiles);
  };
  {
    auto* missing = make_port(callbacks, nullptr);
    assert(missing->initialize() && !missing->prepare(document.store, Slots));
    delete missing;
    auto bad = callbacks;
    bad.wifi_qr = nullptr;
    missing = make_port(bad, &view);
    assert(missing->initialize() && !missing->prepare(document.store, Slots));
    delete missing;
    auto no_bound = callbacks;
    no_bound.bound = nullptr;
    missing = make_port(no_bound, &view);
    assert(missing->initialize() && !missing->prepare(document.store, Slots));
    delete missing;
    PageLvglQrFrames short_frames(arena);
    PageQrRuntime<> short_owner(arena, encoder, short_frames, origin, Slots - 1);
    PagePooledQrTileView short_view(short_owner, short_frames);
    missing = make_port(callbacks, &short_view);
    assert(missing->initialize() && !missing->prepare(document.store, Slots));
    delete missing;
    assert(short_owner.initialize());
    missing = make_port(callbacks, &short_view);
    assert(missing->initialize() && !missing->prepare(document.store, Slots));
    delete missing;
    assert(short_owner.shutdown());
    auto* canvas = widgets[0].forecast;
    widgets[0].forecast = nullptr;
    missing = make_port(callbacks, &view);
    assert(missing->initialize() && !missing->prepare(document.store, Slots));
    delete missing;
    widgets[0].forecast = canvas;
  }
  {
    std::unique_ptr<PageBasicLvglPort<Slots>> port(make_port(callbacks, &view));
    assert(port->initialize());
    PageLabelRefresh cache(arena);
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t label = 0;
    for (const auto& w : widgets)
      for (auto* item : {w.icon, w.label, w.value, w.unit}) labels[label++] = item;
    assert(cache.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, *port);
    actions.renderer = &renderer;
    assert(renderer.prepare(document.store) && state.prepare(document.store));
    const auto first_cpp = cpp_allocations, first_lv = lv_allocations;
    renderer.activate_prepared(document.store);
    assert(cpp_allocations == first_cpp && lv_allocations == first_lv);
    state.activate_prepared(document.store);
    const auto pump = [&] {
      renderer.update_visible();
      lv_obj_update_layout(page);
      qrs.loop();
      renderer.update_visible();
      lv_obj_update_layout(page);
    };
    pump();
    auto front = renderer.binding(1);
    assert(qrs.snapshot(1, front).ready && encoder.compact);
    assert(encoder.side ==
           std::min<int>(lv_obj_get_width(widgets[1].button), lv_obj_get_height(widgets[1].button) - 6));
    assert(lv_obj_has_flag(widgets[1].icon, LV_OBJ_FLAG_HIDDEN) &&
           lv_obj_has_flag(widgets[1].label, LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(widgets[1].sensor_container, LV_OBJ_FLAG_HIDDEN) &&
           !lv_obj_has_flag(widgets[1].forecast, LV_OBJ_FLAG_HIDDEN));
    assert(!qrs.snapshot(3, renderer.binding(3)).ready);
    assert(!std::strcmp(lv_label_get_text(widgets[2].label), "CONNECT") &&
           !std::strcmp(lv_label_get_text(widgets[5].label), "Custom"));
    assert(!lv_obj_has_flag(widgets[2].icon, LV_OBJ_FLAG_HIDDEN) &&
           lv_obj_has_flag(widgets[2].forecast, LV_OBJ_FLAG_HIDDEN));
    for (auto selection : {LV_STATE_DEFAULT, LV_STATE_PRESSED, LV_STATE_CHECKED,
                           static_cast<lv_state_t>(LV_STATE_CHECKED | LV_STATE_PRESSED)}) {
      lv_obj_add_state(widgets[1].button, selection);
      assert((lv_color_to_u32(lv_obj_get_style_bg_color(widgets[1].button, LV_PART_MAIN)) & 0xFFFFFF) ==
             0xFFFFFF);
      lv_obj_clear_state(widgets[1].button, selection);
    }
    const auto commands = actions.commands;
    click(widgets[1].button);
    click(widgets[2].button);
    click(widgets[3].button);
    click(widgets[5].button);
    assert(actions.commands == commands + 3 && actions.toggles == 0);
    subscriptions.send("switch.guest", "", "on");
    pump();
    assert(!lv_obj_has_state(widgets[1].button, LV_STATE_CHECKED) &&
           !lv_obj_has_state(widgets[2].button, LV_STATE_CHECKED));
    ++subscriptions.generation;
    pump();
    click(widgets[2].button);
    assert(actions.commands == commands + 4 && actions.toggles == 0);
    const auto child = document.store.card(0)->target;
    assert(child != NO_PAGE && renderer.open_page(child));
    pump();
    assert(qrs.snapshot(1, renderer.binding(1)).ready && !qrs.snapshot(1, front).ready);
    click(widgets[1].button);
    click(widgets[2].button);
    assert(actions.commands == commands + 6 && actions.toggles == 0);
    assert(renderer.back());
    pump();
    for (unsigned n = 0; n < 10; ++n) {
      assert(renderer.open_page(child));
      pump();
      assert(renderer.back());
      pump();
    }
    const auto bytes = arena.live, allocs = arena.allocations, cpp = cpp_allocations, lv = lv_live_bytes;
    const auto registrations = subscriptions.bindings.size();
    for (unsigned n = 0; n < 1000; ++n) {
      const auto before = cpp_allocations, before_lv = lv_allocations;
      assert(renderer.open_page(child));
      assert(cpp_allocations == before && lv_allocations == before_lv);
      pump();
      assert(renderer.back());
      pump();
      assert(arena.live == bytes && arena.allocations == allocs && cpp_allocations == cpp &&
             lv_live_bytes == lv);
      assert(subscriptions.bindings.size() == registrations);
    }
    // Sizing a spanning card retains the standard encoder policy.
    PageCellLayout large{};
    large.visible = true;
    large.card = renderer.binding(1).card;
    large.row_span = large.column_span = 2;
    port->bind(1, document.store, large, {}, renderer.binding(1));
    renderer.refresh();
    pump();
    assert(!encoder.compact && encoder.side == std::min<int>(lv_obj_get_width(widgets[1].button),
                                                             lv_obj_get_height(widgets[1].button) - 12));
    lv_obj_invalidate(page);
    lv_refr_now(lv_display_get_default());
    const auto geometry = page_qr_layout(1, encoder.side, false);
    lv_area_t box;
    lv_obj_get_coords(widgets[1].button, &box);
    const int x = box.x1 + (lv_obj_get_width(widgets[1].button) - geometry.side) / 2;
    const int y = box.y1 + (lv_obj_get_height(widgets[1].button) - geometry.side) / 2;
    assert(x >= 0 && y >= 0 && x + geometry.side <= 480 && y + geometry.side <= 480);
    assert(framebuffer[(y + geometry.margin) * 480 + x + geometry.margin] == 0);
    assert(framebuffer[y * 480 + x] == 0xFFFF);
    assert(!view.draw(renderer.binding(1), qrs.snapshot(1, renderer.binding(1)).asset + 1,
                      widgets[1].forecast, nullptr));
    // A press from a retired view cannot open a replacement card.
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    pump();
    const auto old_commands = actions.commands;
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == old_commands);
    const auto previous = renderer.binding(1);
    alignas(std::max_align_t) std::array<uint8_t, 32768> new_arena{};
    PageStore replacement;
    assert(PageStore::stage_v1(document.bytes.data(), document.size, limits, 0, new_arena.data(),
                               new_arena.size(), &replacement) == PageStoreStatus::OK);
    assert(renderer.prepare(replacement) && state.prepare(replacement));
    renderer.discard_prepared();
    state.discard_prepared();
    assert(qrs.snapshot(1, previous).ready);
    assert(renderer.prepare(replacement) && state.prepare(replacement));
    renderer.activate_prepared(replacement);
    state.activate_prepared(replacement);
    document.arena.fill(0xDA);
    pump();
    assert(renderer.binding(1).configuration == 0 && qrs.snapshot(1, renderer.binding(1)).ready);
    assert(!qrs.snapshot(1, previous).ready);
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.commands == old_commands);
    renderer.retire_active();
    state.retire_active();
    qrs.loop();
  }
  assert(qrs.shutdown() && arena.live == 0);
  lv_obj_delete(page);
  std::printf(
      "%s: QR cards/buttons: 1000 navigation cycles, guarded modal taps, white/compact/spanning layout and "
      "stable resources\n",
      limits.device_profile);
}

template <size_t Slots>
void confirmation_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::Confirmations{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies deps;
  PageAccessStateRuntime access(arena, subscriptions, fan_status_now);
  PageEntityStateRuntime state(deps, arena, subscriptions, {128, 128, 8192}, &access);
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  const size_t row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
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
  auto callbacks = actions.callbacks();
  callbacks.access = [](void* owner, PageBindingToken token, PageAccessActionSnapshot) {
    auto& a = *static_cast<Actions<Slots>*>(owner);
    ++a.commands;
    a.last = token;
  };
  callbacks.confirmation = [](void* owner, PageBindingToken token, uint32_t connection, bool current) {
    auto& a = *static_cast<Actions<Slots>*>(owner);
    ++a.confirmations;
    a.last = token;
    a.confirmation_connection = connection;
    a.confirmation_current = current;
    if (a.seek_page != NO_PAGE) assert(a.renderer->open_page(a.seek_page));
  };
  {
    auto absent = callbacks;
    absent.confirmation = nullptr;
    PageBasicLvglPort<Slots> missing(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                     absent, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                     nullptr, nullptr, nullptr, nullptr, &access);
    assert(missing.initialize() && !missing.prepare(fixture.store, Slots));
  }
  {
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                  nullptr, nullptr, nullptr, nullptr, &access);
    assert(port.initialize());
    PageLabelRefresh refresh(arena);
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[index++] = label;
    assert(refresh.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    PageRuntimeResourcesGroup<3> resources({&access, &renderer, &state});
    PageStoreRuntime publication(limits, arena, 32768, &resources);
    const auto prepare = [&] {
      return publication.prepare(PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(), fixture.size);
    };
    const auto activate = [&](uint32_t generation) {
      return publication.activate_prepared(generation, PANEL_CONFIG_DOCUMENT_VERSION, fixture.document.data(),
                                           fixture.size);
    };
    assert(prepare());
    const auto activation_cpp = cpp_allocations, activation_lv = lv_allocations;
    assert(activate(0));
    assert(activation_cpp == cpp_allocations && activation_lv == lv_allocations);
    subscriptions.send("switch.confirm", "", "off");
    subscriptions.send("cover.confirm", "", "closed");
    renderer.update_visible();
    lv_obj_update_layout(page);
    lv_refr_now(lv_display_get_default());
    const auto tap = [&](size_t slot) {
      const auto cpp = cpp_allocations, reservations = arena.allocations;
      click(widgets[slot].button);
      // LVGL allocates transient style-state/animation work for native input.
      // Admission allocates no C++/arena storage; settled view retention is
      // checked across the complete navigation cycle below.
      assert(cpp == cpp_allocations && reservations == arena.allocations);
    };
    tap(1);
    assert(actions.confirmations == 1 && !actions.confirmation_current && actions.last.configuration == 0 &&
           actions.confirmation_connection == subscriptions.generation && actions.toggles == 0);
    tap(2);
    assert(actions.toggles == 1 && actions.confirmations == 1);  // Off-only flag does not block turning on.
    subscriptions.send("switch.confirm", "", "on");
    renderer.update_visible();
    tap(1);
    assert(actions.toggles == 2);
    tap(2);
    assert(actions.confirmations == 2 && actions.confirmation_current && actions.toggles == 2);
    tap(3);
    assert(actions.confirmations == 3 && !actions.confirmation_current && actions.commands == 0);
    tap(4);
    assert(actions.confirmations == 4 && !actions.confirmation_current && actions.commands == 0);
    tap(5);
    assert(actions.confirmations == 5 && !actions.confirmation_current && actions.commands == 0);
    subscriptions.send("cover.confirm", "", "open");
    renderer.update_visible();
    tap(5);
    assert(actions.confirmation_current && actions.confirmations == 6);
    // Reconnection cannot adopt a press into the new epoch, even with fresh state before click.
    const auto old_count = actions.confirmations;
    lv_obj_send_event(widgets[2].button, LV_EVENT_PRESSED, nullptr);
    ++subscriptions.generation;
    state.invalidate_disconnected_state();
    subscriptions.send("switch.confirm", "", "on");
    subscriptions.send("cover.confirm", "", "closed");
    renderer.update_visible();
    lv_obj_send_event(widgets[2].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.confirmations == old_count);
    tap(2);
    assert(actions.confirmations == old_count + 1 &&
           actions.confirmation_connection == subscriptions.generation);
    const auto confirmed_before_unknown = actions.confirmations, toggled_before_unknown = actions.toggles;
    subscriptions.send("switch.confirm", "", "unknown");
    renderer.update_visible();
    tap(1);
    tap(2);
    assert(actions.confirmations == confirmed_before_unknown && actions.toggles == toggled_before_unknown);
    subscriptions.send("switch.confirm", "", "on");
    renderer.update_visible();
    const auto child = fixture.store.card(0)->target;
    assert(child != NO_PAGE);
    actions.seek_page = child;
    tap(2);
    actions.seek_page = NO_PAGE;
    assert(!renderer.valid(actions.last));
    renderer.update_visible();
    const auto after_navigation = actions.confirmations;
    lv_obj_send_event(widgets[2].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.confirmations == after_navigation);
    tap(1);
    tap(2);
    tap(3);
    assert(actions.confirmations == after_navigation + 3);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    subscriptions.send("switch.confirm", "", "off");
    subscriptions.send("cover.confirm", "", "closed");
    renderer.update_visible();
    const auto cpp = cpp_allocations, lv_bytes = lv_live_bytes, meta = arena.live,
               reservations = arena.allocations, registrations = subscriptions.bindings.size();
    const auto objects = lv_obj_get_child_count(page);
    for (int i = 0; i < 1000; ++i) {
      tap(1);
      tap(3);
      tap(4);
      tap(5);
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      tap(1);
      tap(2);
      tap(3);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp == cpp_allocations && lv_bytes == lv_live_bytes && meta == arena.live &&
             reservations == arena.allocations && registrations == subscriptions.bindings.size() &&
             objects == lv_obj_get_child_count(page));
    }
    // A discarded prepare leaves both current bindings and their flags live.
    const auto original = renderer.binding(1);
    assert(prepare());
    publication.discard_prepared();
    assert(renderer.valid(original));
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(prepare());
    assert(activate(1));
    const auto before_late = actions.confirmations;
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.confirmations == before_late && !renderer.valid(original));
    renderer.update_visible();
    tap(1);
    assert(actions.confirmations == before_late + 1 && actions.last.configuration == 1);
    renderer.retire_active();
    state.retire_active();
    access.retire_active();
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
  std::printf(
      "%s: confirmation routing, 1000 navigation cycles, captured press epochs and stable resources\n",
      limits.device_profile);
}

template <size_t Slots>
void input_epoch_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::InputEpochs{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies deps;
  PageEntityStateRuntime state(deps, arena, subscriptions, {128, 128, 8192});
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  const size_t row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
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
    w.sensor_container = lv_obj_create(w.button);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
  }
  PageBasicTheme theme;
  Actions<Slots> actions;
  {
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  actions.callbacks());
    assert(port.initialize());
    PageLabelRefresh refresh(arena);
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t index = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[index++] = label;
    assert(refresh.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(renderer.prepare(fixture.store) && state.prepare(fixture.store));
    renderer.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    const auto report = [&] {
      subscriptions.send("switch.input", "", "off");
      subscriptions.send("light.input", "", "off");
      subscriptions.send("cover.input", "", "closed");
      subscriptions.send("fan.input", "", "on");
      subscriptions.send("fan.input", "supported_features", "63");
      subscriptions.send("fan.input", "direction", "forward");
      subscriptions.send("select.input", "", "Home");
      subscriptions.send("select.input", "options", "Home,Away");
    };
    const auto reconnect = [&] {
      ++subscriptions.generation;
      state.invalidate_disconnected_state();
      report();
      renderer.update_visible();
    };
    report();
    renderer.update_visible();
    lv_obj_update_layout(page);
    lv_refr_now(lv_display_get_default());
    for (size_t slot : {1u, 2u}) {
      const auto toggles = actions.toggles, commands = actions.commands;
      lv_obj_send_event(widgets[slot].button, LV_EVENT_PRESSED, nullptr);
      reconnect();
      lv_obj_send_event(widgets[slot].button, LV_EVENT_CLICKED, nullptr);
      assert(actions.toggles == toggles && actions.commands == commands);
      click(widgets[slot].button);
      assert(actions.toggles + actions.commands == toggles + commands + 1);
    }
    for (size_t slot : {3u, 4u, 5u}) {
      const auto commands = actions.commands;
      lv_obj_send_event(widgets[slot].button, LV_EVENT_PRESSED, nullptr);
      reconnect();
      lv_obj_send_event(widgets[slot].button, LV_EVENT_CLICKED, nullptr);
      assert(actions.commands == commands + 1);
    }
    // Local navigation also remains independent of HA connection churn.
    const auto child = fixture.store.card(0)->target;
    assert(child != NO_PAGE);
    lv_obj_send_event(widgets[0].button, LV_EVENT_PRESSED, nullptr);
    reconnect();
    lv_obj_send_event(widgets[0].button, LV_EVENT_CLICKED, nullptr);
    renderer.update_visible();
    assert(renderer.binding(0).page == child);
    for (size_t slot = 1; slot <= 5; ++slot) {
      const auto commands = actions.commands, climates = actions.climates;
      lv_obj_send_event(widgets[slot].button, LV_EVENT_PRESSED, nullptr);
      reconnect();
      lv_obj_send_event(widgets[slot].button, LV_EVENT_CLICKED, nullptr);
      assert(actions.commands == commands && actions.climates == climates);
      click(widgets[slot].button);
      assert(actions.commands + actions.climates == commands + climates + 1);
    }
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, meta = arena.allocations, retained = arena.live,
               lv_retained = lv_live_bytes, registrations = subscriptions.bindings.size();
    for (int i = 0; i < 1000; ++i) {
      const auto commands = actions.commands, toggles = actions.toggles;
      lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
      reconnect();
      lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
      lv_obj_send_event(widgets[2].button, LV_EVENT_PRESSED, nullptr);
      reconnect();
      lv_obj_send_event(widgets[2].button, LV_EVENT_CLICKED, nullptr);
      assert(actions.commands == commands && actions.toggles == toggles);
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      assert(cpp == cpp_allocations && meta == arena.allocations && retained == arena.live &&
             lv_retained == lv_live_bytes && registrations == subscriptions.bindings.size());
    }
    renderer.retire_active();
    state.retire_active();
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
  std::printf(
      "%s: input epochs, 1000 reconnect/navigation cycles, HA stale clicks rejected, local input retained\n",
      limits.device_profile);
}

template <size_t Slots>
void screen_lock_profile(const DeviceUiLimits& limits) {
  Fixture fixture(limits, Fixture::ScreenLocks{});
  Arena arena;
  Subscriptions subscriptions;
  PageCardDependencies dependencies;
  PageEntityStateRuntime state(dependencies, arena, subscriptions, {128, 128, 8192});
  auto* page = lv_obj_create(lv_screen_active());
  std::array<lv_coord_t, Slots + 1> columns{}, rows{};
  for (size_t i = 0; i < limits.grid_columns; ++i) columns[i] = LV_GRID_FR(1);
  columns[limits.grid_columns] = LV_GRID_TEMPLATE_LAST;
  const size_t row_count = (Slots + limits.grid_columns - 1) / limits.grid_columns;
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
  theme.translate = [](const char* key) {
    return std::strcmp(key, "Screen Locked") == 0     ? "Touchscreen secured with the shared PIN"
           : std::strcmp(key, "Screen Unlocked") == 0 ? "Touchscreen ready for input"
                                                      : key;
  };
  Actions<Slots> actions;
  auto callbacks = actions.callbacks();
  for (bool missing_state : {false, true}) {
    auto incomplete = callbacks;
    if (missing_state)
      incomplete.screen_locked = nullptr;
    else
      incomplete.screen_lock = nullptr;
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  incomplete);
    assert(port.initialize() && !port.prepare(fixture.store, Slots));
  }
  {
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme,
                                  callbacks);
    assert(port.initialize());
    PageLabelRefresh refresh(arena);
    std::array<lv_obj_t*, Slots * 4> labels{};
    size_t at = 0;
    for (const auto& w : widgets)
      for (auto* label : {w.icon, w.label, w.value, w.unit}) labels[at++] = label;
    assert(refresh.initialize(labels.data(), labels.size()));
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(renderer.prepare(fixture.store) && state.prepare(fixture.store));
    renderer.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto verify = [&](size_t slot) {
      if (std::strcmp(lv_label_get_text(widgets[slot].icon), actions.locked ? "LOCKED" : "UNLOCKED") != 0)
        std::fprintf(stderr, "PIN projection: slot %zu, locked %d, icon '%s', label '%s', page %u\n", slot,
                     actions.locked, lv_label_get_text(widgets[slot].icon),
                     lv_label_get_text(widgets[slot].label), unsigned(renderer.active_page()));
      assert(std::strcmp(lv_label_get_text(widgets[slot].icon), actions.locked ? "LOCKED" : "UNLOCKED") == 0);
      assert(std::strcmp(lv_label_get_text(widgets[slot].label),
                         theme.translate(actions.locked ? "Screen Locked" : "Screen Unlocked")) == 0);
      assert(lv_obj_has_state(widgets[slot].button, LV_STATE_CHECKED) == actions.locked);
      assert(lv_obj_has_flag(widgets[slot].sensor_container, LV_OBJ_FLAG_HIDDEN));
    };
    const auto tap = [&](size_t slot, bool saver) {
      const auto count = actions.locks;
      lv_obj_send_event(widgets[slot].button, LV_EVENT_PRESSED, nullptr);
      lv_obj_send_event(widgets[slot].button, LV_EVENT_CLICKED, nullptr);
      if (actions.locks != count + 1 || actions.lock_screensaver != saver || !renderer.valid(actions.last))
        std::fprintf(stderr,
                     "PIN input: page %u slot %zu count %zu -> %zu saver %d expected %d valid %d card %u\n",
                     unsigned(renderer.active_page()), slot, count, actions.locks, actions.lock_screensaver,
                     saver, renderer.valid(actions.last), unsigned(renderer.binding(slot).card));
      assert(actions.locks == count + 1 && actions.lock_screensaver == saver && renderer.valid(actions.last));
    };
    verify(1);
    const auto unpressed = actions.locks;
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESS_LOST, nullptr);
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.locks == unpressed);
    tap(1, false);
    tap(2, true);
    actions.locked = true;
    renderer.update_visible();
    verify(1);
    actions.locked = false;
    renderer.update_visible();
    verify(1);
    // HA reconnect does not invalidate an entirely local PIN-lock press.
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    ++subscriptions.generation;
    state.invalidate_disconnected_state();
    const auto reconnect_count = actions.locks;
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.locks == reconnect_count + 1);
    const auto child = fixture.store.card(0)->target;
    assert(child != NO_PAGE);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.open_page(child));
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto stale = actions.locks;
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.locks == stale);
    verify(1);
    tap(1, false);
    // Preserve the existing subpage normalizer, which strips this local flag.
    assert(fixture.store.field(*fixture.store.card(renderer.binding(2).card), CardField::OPTIONS).empty());
    tap(2, false);
    assert(renderer.back());
    renderer.update_visible();
    lv_obj_update_layout(page);
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.suspend());
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.locks == stale + 2);
    assert(renderer.resume());
    renderer.update_visible();
    lv_obj_update_layout(page);
    const auto cpp = cpp_allocations, reservations = arena.allocations, retained = arena.live,
               lv_bytes = lv_live_bytes;
    const auto objects = lv_obj_get_child_count(page);
    const auto descriptors = refresh.memory().descriptor_adds;
    assert(subscriptions.bindings.empty());
    for (int i = 0; i < 1000; ++i) {
      actions.locked = (i % 2) == 0;
      renderer.update_visible();
      verify(1);
      tap(2, true);
      assert(renderer.open_page(child));
      renderer.update_visible();
      lv_obj_update_layout(page);
      verify(1);
      tap(1, false);
      assert(renderer.back());
      renderer.update_visible();
      lv_obj_update_layout(page);
      verify(1);
      assert(cpp_allocations == cpp && arena.allocations == reservations && arena.live == retained &&
             lv_live_bytes == lv_bytes && lv_obj_get_child_count(page) == objects &&
             refresh.memory().descriptor_adds == descriptors && subscriptions.bindings.empty());
    }
    // Replacement invalidates held input; a new binding reads shared state again.
    lv_obj_send_event(widgets[1].button, LV_EVENT_PRESSED, nullptr);
    assert(renderer.prepare(fixture.store) && state.prepare(fixture.store));
    const auto before_replace = actions.locks;
    renderer.activate_prepared(fixture.store);
    state.activate_prepared(fixture.store);
    lv_obj_send_event(widgets[1].button, LV_EVENT_CLICKED, nullptr);
    assert(actions.locks == before_replace);
    renderer.update_visible();
    verify(1);
    tap(1, false);
    renderer.retire_active();
    state.retire_active();
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
  std::printf("%s: Screen Lock, 1000 navigation/shared PIN cycles, local press guards and stable resources\n",
              limits.device_profile);
}

template<size_t Slots>
void on_pattern_profile(const DeviceUiLimits& limits) {
  std::array<uint8_t, 8192> document{};
  alignas(std::max_align_t) std::array<uint8_t, 65536> storage{};
  PageStore store;
  PanelConfigWriter writer(document.data(), document.size());
  assert(writer.begin() == PanelConfigStatus::OK);
  assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                      std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
  const char* cards[] = {
      "switch.pattern;Pattern;Auto;Auto;;;;;on_pattern=stripes",
      ";Pages;Auto;Auto;;;subpage;;",
      "switch.plain;Plain;Auto;Auto;;;;;",
      "switch.dark;Dark;Auto;Auto;;;;;card_active_color=custom,card_on_color=336699,on_pattern=stripes",
      "switch.pale;Pale;Auto;Auto;;;;;card_active_color=custom,card_on_color=EEDDCC,on_pattern=stripes",
      "switch.invalid;Plain;Auto;Auto;;;;;on_pattern=invalid"};
  for (size_t i = 0; i < 6; ++i)
    assert(writer.append_button(i + 1, reinterpret_cast<const uint8_t*>(cards[i]), std::strlen(cards[i])) ==
           PanelConfigStatus::OK);
  const char* child = "~B,1|S,,Value,Auto,Auto,sensor.pattern,C,1,";
  assert(writer.append_subpage(2, reinterpret_cast<const uint8_t*>(child), std::strlen(child)) == PanelConfigStatus::OK);
  assert(writer.append_setting(reinterpret_cast<const uint8_t*>("button_order"), 12,
                               reinterpret_cast<const uint8_t*>("1,2,3,4,5,6"), 11) == PanelConfigStatus::OK);
  size_t size = 0;
  assert(writer.finish(&size) == PanelConfigStatus::OK);
  assert(PageStore::stage_v1(document.data(), size, limits, 17, storage.data(), storage.size(), &store) == PageStoreStatus::OK);
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
    w.fill = lv_obj_create(w.button);
    w.icon = lv_label_create(w.button);
    w.label = lv_label_create(w.button);
    w.sensor_container = lv_obj_create(w.button);
    w.value = lv_label_create(w.sensor_container);
    w.unit = lv_label_create(w.sensor_container);
  }
  PageBasicTheme theme;
  theme.red_percent = 80; theme.green_percent = 90; theme.blue_percent = 70;
  Actions<Slots> actions;
  {
    PageBasicLvglPort<Slots> port(arena, state, widgets, {limits.slot_count, limits.grid_columns}, theme, actions.callbacks());
    assert(port.initialize());
    PageRendererRuntime<Slots> renderer(limits, arena, port);
    actions.renderer = &renderer;
    assert(renderer.prepare(store) && state.prepare(store));
    renderer.activate_prepared(store);
    state.activate_prepared(store);
    for (const auto* entity : {"switch.pattern", "switch.plain", "switch.dark", "switch.pale", "switch.invalid"})
      subscriptions.send(entity, "", "on");
    const auto verify = [&](size_t slot, bool pattern, uint32_t color) {
      auto* button = widgets[slot].button;
      assert(lv_obj_has_state(button, LV_STATE_CHECKED));
      for (const bool pressed : {false, true}) {
        if (pressed) lv_obj_add_state(button, LV_STATE_PRESSED);
        assert(lv_obj_get_style_bg_grad_dir(button, LV_PART_MAIN) == (pattern ? LV_GRAD_DIR_HOR : LV_GRAD_DIR_NONE));
        if (pattern)
          assert(lv_color_to_u32(lv_obj_get_style_bg_grad_color(button, LV_PART_MAIN)) ==
                 lv_color_to_u32(lv_color_hex(card_pattern_highlight_color(color))));
        lv_obj_clear_state(button, LV_STATE_PRESSED);
      }
    };
    const auto home = [&] {
      renderer.update_visible();
      lv_obj_update_layout(page);
      verify(0, true, theme.active);
      verify(2, false, theme.active);
      verify(3, true, correct_display_color(0x336699, 80, 90, 70));
      verify(4, true, correct_display_color(0xEEDDCC, 80, 90, 70));
      verify(5, false, theme.active);
    };
    const auto cycle = [&] {
      assert(renderer.open_page(store.card(1)->target));
      renderer.update_visible();
      lv_obj_update_layout(page);
      auto* back = widgets[0].button;
      lv_obj_add_state(back, static_cast<lv_state_t>(LV_STATE_CHECKED | LV_STATE_PRESSED));
      assert(lv_obj_get_style_bg_grad_dir(back, LV_PART_MAIN) == LV_GRAD_DIR_NONE);
      lv_obj_clear_state(back, static_cast<lv_state_t>(LV_STATE_CHECKED | LV_STATE_PRESSED));
      assert(renderer.back());
      home();
    };
    home();
    cycle();
    const auto cpp = cpp_allocations, metadata = arena.allocations, retained = arena.live,
               widget_bytes = lv_live_bytes, registrations = subscriptions.bindings.size();
    for (int i = 0; i < 1000; ++i) cycle();
    assert(cpp == cpp_allocations && metadata == arena.allocations && retained == arena.live &&
           widget_bytes == lv_live_bytes && registrations == subscriptions.bindings.size());
    renderer.retire_active();
    state.retire_active();
  }
  assert(arena.live == 0);
  lv_obj_delete(page);
  std::printf("%s: on-state patterns, corrected colours, plain/Back reuse, 1000 bounded navigation cycles\n", limits.device_profile);
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
    if (!family || std::strcmp(family, "on_pattern") == 0) {
      switch (limits.slot_count) {
        case 6: on_pattern_profile<6>(limits); break;
        case 9: on_pattern_profile<9>(limits); break;
        case 15: on_pattern_profile<15>(limits); break;
        case 20: on_pattern_profile<20>(limits); break;
        default: assert(false);
      }
      if (family) continue;
    }
    if (family && std::strcmp(family, "media_parity") == 0) {
      switch (limits.slot_count) {
        case 6:
          media_action_profile<6>(limits);
          media_position_profile<6>(limits);
          now_playing_profile<6>(limits);
          break;
        case 9:
          media_action_profile<9>(limits);
          media_position_profile<9>(limits);
          now_playing_profile<9>(limits);
          break;
        case 15:
          media_action_profile<15>(limits);
          media_position_profile<15>(limits);
          now_playing_profile<15>(limits);
          break;
        case 20:
          media_action_profile<20>(limits);
          media_position_profile<20>(limits);
          now_playing_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (!family || std::strcmp(family, "device_commands") == 0) {
      switch (limits.slot_count) {
        case 6:
          deferred_device_profile<6>(limits);
          break;
        case 9:
          deferred_device_profile<9>(limits);
          break;
        case 15:
          deferred_device_profile<15>(limits);
          break;
        case 20:
          deferred_device_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      if (family) continue;
    }
    if (!family || std::strcmp(family, "media_commands") == 0) {
      switch (limits.slot_count) {
        case 6:
          deferred_media_profile<6>(limits);
          break;
        case 9:
          deferred_media_profile<9>(limits);
          break;
        case 15:
          deferred_media_profile<15>(limits);
          break;
        case 20:
          deferred_media_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      if (family) continue;
    }
    if (!family || std::strcmp(family, "slider_commands") == 0) {
      switch(limits.slot_count) {
        case 6: deferred_slider_profile<6>(limits); break;
        case 9: deferred_slider_profile<9>(limits); break;
        case 15: deferred_slider_profile<15>(limits); break;
        case 20: deferred_slider_profile<20>(limits); break;
        default: assert(false);
      }
      if(family) continue;
    }
    if (family && std::strcmp(family, "screen_lock") == 0) {
      switch (limits.slot_count) {
        case 6:
          screen_lock_profile<6>(limits);
          break;
        case 9:
          screen_lock_profile<9>(limits);
          break;
        case 15:
          screen_lock_profile<15>(limits);
          break;
        case 20:
          screen_lock_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "input_epochs") == 0) {
      switch (limits.slot_count) {
        case 6:
          input_epoch_profile<6>(limits);
          break;
        case 9:
          input_epoch_profile<9>(limits);
          break;
        case 15:
          input_epoch_profile<15>(limits);
          break;
        case 20:
          input_epoch_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "forecast") == 0) {
      switch (limits.slot_count) {
        case 6:
          forecast_profile<6>(limits);
          break;
        case 9:
          forecast_profile<9>(limits);
          break;
        case 15:
          forecast_profile<15>(limits);
          break;
        case 20:
          forecast_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "confirmation") == 0) {
      switch (limits.slot_count) {
        case 6:
          confirmation_profile<6>(limits);
          break;
        case 9:
          confirmation_profile<9>(limits);
          break;
        case 15:
          confirmation_profile<15>(limits);
          break;
        case 20:
          confirmation_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "qr") == 0) {
      switch (limits.slot_count) {
        case 6:
          qr_tile_profile<6>(limits);
          break;
        case 9:
          qr_tile_profile<9>(limits);
          break;
        case 15:
          qr_tile_profile<15>(limits);
          break;
        case 20:
          qr_tile_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "images") == 0) {
      switch (limits.slot_count) {
        case 6:
          image_tile_profile<6, 6>(limits);
          break;
        case 9:
          if (limits.image_slot_count == 2)
            image_tile_profile<9, 2>(limits);
          else
            image_tile_profile<9, 6>(limits);
          break;
        case 15:
          image_tile_profile<15, 6>(limits);
          break;
        case 20:
          image_tile_profile<20, 6>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "alarm") == 0) {
      switch (limits.slot_count) {
        case 6:
          alarm_profile<6>(limits);
          break;
        case 9:
          alarm_profile<9>(limits);
          break;
        case 15:
          alarm_profile<15>(limits);
          break;
        case 20:
          alarm_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "number-layout") == 0) {
      switch (limits.slot_count) {
        case 6:
          number_size_profile<6>(limits);
          break;
        case 9:
          number_size_profile<9>(limits);
          break;
        case 15:
          number_size_profile<15>(limits);
          break;
        case 20:
          number_size_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      std::printf("%s: numeric font/unit-offset parity passed\n", limits.device_profile);
      continue;
    }
    if (family && std::strcmp(family, "clock") == 0) {
      switch (limits.slot_count) {
        case 6:
          clock_profile<6>(limits);
          clock_size_profile<6>(limits);
          break;
        case 9:
          clock_profile<9>(limits);
          clock_size_profile<9>(limits);
          break;
        case 15:
          clock_profile<15>(limits);
          clock_size_profile<15>(limits);
          break;
        case 20:
          clock_profile<20>(limits);
          clock_size_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "calendar") == 0) {
      switch (limits.slot_count) {
        case 6:
          calendar_profile<6>(limits);
          calendar_size_profile<6>(limits);
          break;
        case 9:
          calendar_profile<9>(limits);
          calendar_size_profile<9>(limits);
          break;
        case 15:
          calendar_profile<15>(limits);
          calendar_size_profile<15>(limits);
          break;
        case 20:
          calendar_profile<20>(limits);
          calendar_size_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "cleaning") == 0) {
      switch (limits.slot_count) {
        case 6:
          cleaning_profile<6>(limits);
          break;
        case 9:
          cleaning_profile<9>(limits);
          break;
        case 15:
          cleaning_profile<15>(limits);
          break;
        case 20:
          cleaning_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "access") == 0) {
      switch (limits.slot_count) {
        case 6:
          access_profile<6>(limits);
          break;
        case 9:
          access_profile<9>(limits);
          break;
        case 15:
          access_profile<15>(limits);
          break;
        case 20:
          access_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "cover_toggles") == 0) {
      switch (limits.slot_count) {
        case 6:
          cover_toggle_profile<6>(limits);
          break;
        case 9:
          cover_toggle_profile<9>(limits);
          break;
        case 15:
          cover_toggle_profile<15>(limits);
          break;
        case 20:
          cover_toggle_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "cover_commands") == 0) {
      switch (limits.slot_count) {
        case 6:
          cover_command_profile<6>(limits);
          break;
        case 9:
          cover_command_profile<9>(limits);
          break;
        case 15:
          cover_command_profile<15>(limits);
          break;
        case 20:
          cover_command_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "folder_indicators") == 0) {
      switch (limits.slot_count) {
        case 6:
          control_profile<6>(limits);
          break;
        case 9:
          control_profile<9>(limits);
          break;
        case 15:
          control_profile<15>(limits);
          break;
        case 20:
          control_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "timers") == 0) {
      switch (limits.slot_count) {
        case 6:
          timer_profile<6>(limits);
          break;
        case 9:
          timer_profile<9>(limits);
          break;
        case 15:
          timer_profile<15>(limits);
          break;
        case 20:
          timer_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "light_temperature") == 0) {
      switch (limits.slot_count) {
        case 6:
          light_temperature_profile<6>(limits);
          break;
        case 9:
          light_temperature_profile<9>(limits);
          break;
        case 15:
          light_temperature_profile<15>(limits);
          break;
        case 20:
          light_temperature_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
    if (family && std::strcmp(family, "number_sliders") == 0) {
      switch (limits.slot_count) {
        case 6:
          number_slider_profile<6>(limits);
          break;
        case 9:
          number_slider_profile<9>(limits);
          break;
        case 15:
          number_slider_profile<15>(limits);
          break;
        case 20:
          number_slider_profile<20>(limits);
          break;
        default:
          assert(false);
      }
      continue;
    }
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
          number_slider_profile<6>(limits);
          option_select_profile<6>(limits);
          local_sensor_profile<6>(limits);
          local_relay_profile<6>(limits);
          break;
        case 9:
          fan_mode_profile<9>(limits);
          fan_status_profile<9>(limits);
          percent_slider_profile<9>(limits);
          number_slider_profile<9>(limits);
          option_select_profile<9>(limits);
          local_sensor_profile<9>(limits);
          local_relay_profile<9>(limits);
          break;
        case 15:
          fan_mode_profile<15>(limits);
          fan_status_profile<15>(limits);
          percent_slider_profile<15>(limits);
          number_slider_profile<15>(limits);
          option_select_profile<15>(limits);
          local_sensor_profile<15>(limits);
          local_relay_profile<15>(limits);
          break;
        case 20:
          fan_mode_profile<20>(limits);
          fan_status_profile<20>(limits);
          percent_slider_profile<20>(limits);
          number_slider_profile<20>(limits);
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
        input_epoch_profile<6>(limits);
        screen_lock_profile<6>(limits);
        confirmation_profile<6>(limits);
        qr_tile_profile<6>(limits);
        status_action_profile<6>(limits);
        control_profile<6>(limits);
        cover_command_profile<6>(limits);
        cover_toggle_profile<6>(limits);
        access_profile<6>(limits);
        cleaning_profile<6>(limits);
        calendar_profile<6>(limits);
        calendar_size_profile<6>(limits);
        clock_profile<6>(limits);
        clock_size_profile<6>(limits);
        number_size_profile<6>(limits);
        alarm_profile<6>(limits);
        fan_mode_profile<6>(limits);
        fan_status_profile<6>(limits);
        percent_slider_profile<6>(limits);
        number_slider_profile<6>(limits);
        light_temperature_profile<6>(limits);
        timer_profile<6>(limits);
        option_select_profile<6>(limits);
        local_sensor_profile<6>(limits);
        local_relay_profile<6>(limits);
        weather_profile<6>(limits);
        forecast_profile<6>(limits);
        media_action_profile<6>(limits);
        media_position_profile<6>(limits);
        now_playing_profile<6>(limits);
        cover_art_profile<6>(limits);
        image_tile_profile<6, 6>(limits);
        media_control_profile<6>(limits);
        break;
      case 9:
        profile<9>(limits);
        input_epoch_profile<9>(limits);
        screen_lock_profile<9>(limits);
        confirmation_profile<9>(limits);
        qr_tile_profile<9>(limits);
        status_action_profile<9>(limits);
        control_profile<9>(limits);
        cover_command_profile<9>(limits);
        cover_toggle_profile<9>(limits);
        access_profile<9>(limits);
        cleaning_profile<9>(limits);
        calendar_profile<9>(limits);
        calendar_size_profile<9>(limits);
        clock_profile<9>(limits);
        clock_size_profile<9>(limits);
        number_size_profile<9>(limits);
        alarm_profile<9>(limits);
        fan_mode_profile<9>(limits);
        fan_status_profile<9>(limits);
        percent_slider_profile<9>(limits);
        number_slider_profile<9>(limits);
        light_temperature_profile<9>(limits);
        timer_profile<9>(limits);
        option_select_profile<9>(limits);
        local_sensor_profile<9>(limits);
        local_relay_profile<9>(limits);
        weather_profile<9>(limits);
        forecast_profile<9>(limits);
        media_action_profile<9>(limits);
        media_position_profile<9>(limits);
        now_playing_profile<9>(limits);
        cover_art_profile<9>(limits);
        if (limits.image_slot_count == 2)
          image_tile_profile<9, 2>(limits);
        else
          image_tile_profile<9, 6>(limits);
        media_control_profile<9>(limits);
        break;
      case 15:
        profile<15>(limits);
        input_epoch_profile<15>(limits);
        screen_lock_profile<15>(limits);
        confirmation_profile<15>(limits);
        qr_tile_profile<15>(limits);
        status_action_profile<15>(limits);
        control_profile<15>(limits);
        cover_command_profile<15>(limits);
        cover_toggle_profile<15>(limits);
        access_profile<15>(limits);
        cleaning_profile<15>(limits);
        calendar_profile<15>(limits);
        calendar_size_profile<15>(limits);
        clock_profile<15>(limits);
        clock_size_profile<15>(limits);
        number_size_profile<15>(limits);
        alarm_profile<15>(limits);
        fan_mode_profile<15>(limits);
        fan_status_profile<15>(limits);
        percent_slider_profile<15>(limits);
        number_slider_profile<15>(limits);
        light_temperature_profile<15>(limits);
        timer_profile<15>(limits);
        option_select_profile<15>(limits);
        local_sensor_profile<15>(limits);
        local_relay_profile<15>(limits);
        weather_profile<15>(limits);
        forecast_profile<15>(limits);
        media_action_profile<15>(limits);
        media_position_profile<15>(limits);
        now_playing_profile<15>(limits);
        cover_art_profile<15>(limits);
        image_tile_profile<15, 6>(limits);
        media_control_profile<15>(limits);
        break;
      case 20:
        profile<20>(limits);
        input_epoch_profile<20>(limits);
        screen_lock_profile<20>(limits);
        confirmation_profile<20>(limits);
        qr_tile_profile<20>(limits);
        status_action_profile<20>(limits);
        control_profile<20>(limits);
        cover_command_profile<20>(limits);
        cover_toggle_profile<20>(limits);
        access_profile<20>(limits);
        cleaning_profile<20>(limits);
        calendar_profile<20>(limits);
        calendar_size_profile<20>(limits);
        clock_profile<20>(limits);
        clock_size_profile<20>(limits);
        number_size_profile<20>(limits);
        alarm_profile<20>(limits);
        fan_mode_profile<20>(limits);
        fan_status_profile<20>(limits);
        percent_slider_profile<20>(limits);
        number_slider_profile<20>(limits);
        light_temperature_profile<20>(limits);
        timer_profile<20>(limits);
        option_select_profile<20>(limits);
        local_sensor_profile<20>(limits);
        local_relay_profile<20>(limits);
        weather_profile<20>(limits);
        forecast_profile<20>(limits);
        media_action_profile<20>(limits);
        media_position_profile<20>(limits);
        now_playing_profile<20>(limits);
        cover_art_profile<20>(limits);
        image_tile_profile<20, 6>(limits);
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
