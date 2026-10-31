"""Compile extracted candidate producer bodies: native/profile vs standalone routing."""
from pathlib import Path
import argparse,datetime,hashlib,importlib.util,json,os,re,shlex,subprocess,sys,tempfile
os.umask(0o077)
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root',type=Path,default=Path(__file__).resolve().parents[2])
parser.add_argument('--report',type=Path)
args=parser.parse_args();root=args.source_root.resolve();src=root;headers=root/'components/espcontrol';tests=root/'tests/firmware'
temporary=tempfile.TemporaryDirectory(prefix='espcontrol-pooled-producers-');directory=Path(temporary.name)
spec=importlib.util.spec_from_file_location('extract',tests/'generate_media_lifecycle_integration.py');extract=importlib.util.module_from_spec(spec);spec.loader.exec_module(extract)
def struct(file,name):
 text=(headers/file).read_text();m=re.search(r'^struct '+name+r' \{[\s\S]*?^\};',text,re.M);assert m;return m.group()
dt=(headers/'button_grid_datetime_cards.h').read_text();weather=(headers/'button_grid_weather_forecast.h').read_text();weather=weather[weather.index('#else\n\nstruct WeatherForecastCardRef'):];cfg=(headers/'button_grid_config.h').read_text()
functions=[]
for text,names in [(dt,['calendar_date_state','timezone_card_state','refresh_calendar_cards','update_calendar_cards','update_calendar_cards_time','update_timezone_cards']),(weather,['refresh_weather_forecast_card_visuals','weather_forecast_cancel_pending_requests','weather_forecast_cancel_stale_requests','weather_forecast_send_next_queued','refresh_weather_forecast_cards']),(cfg,['refresh_temperature_unit_labels'])]:
 for name in names:functions.append(extract.definition(text,name)[1])
nav='struct Navigation final : espcontrol::ApplicationSceneNavigation {\n int paints=0, forecasts=0, units=0;\n void refresh_content() override {++paints;}\n void refresh_weather_forecasts() override {++forecasts;}\n void refresh_temperature_unit(bool fahrenheit) override {assert(fahrenheit);++units;}\n bool navigation_ready() const override { return true; }\n bool grid_configured() const override { return true; }\n int subpage_slot() const override { return 0; }\n std::string_view subpage_label() const override { return {}; }\n bool critical_active() const override { return false; }\n bool can_open_modal(std::string_view) const override { return true; }\n bool open_modal(std::string_view) override { return true; }\n bool can_open_subpage(std::string_view) const override { return true; }\n bool open_subpage(std::string_view) override { return true; }\n bool close_subpage() override { return true; }\n bool navigate(std::string_view) override { return true; }\n bool home() override { return true; }\n void close_modal() override {}\n bool open_settings() override { return true; }\n void close_settings() override {}\n void display_takeover() override {}\n void refresh_chrome() override {}\n};\n'
prefix=r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <memory>
#include <string>
#include <vector>
#include "page_scene_selection.h"
#include "calendar_state_events.h"
#include "timezone_state_events.h"
using lv_obj_t=int;
static int legacy_calls=0;
static int calendar_count=1,zone_count=1;
static void legacy() {++legacy_calls;}
'''
prefix+='\n'.join(struct('button_grid_datetime_cards.h',n) for n in ('CalendarCardRef','CalendarDateState','TimezoneCardRef','TimezoneCardState'))
prefix+=r'''
CalendarCardRef* calendar_card_refs(){legacy();static CalendarCardRef r{};return &r;}
int& calendar_card_count(){legacy();return calendar_count;}
void compact_calendar_card_refs(){legacy();}
void apply_calendar_card_text(const CalendarCardRef&,const CalendarDateState&){legacy();}
TimezoneCardRef* timezone_card_refs(){legacy();static TimezoneCardRef r{};return &r;}
int& timezone_card_count(){legacy();return zone_count;}
void compact_timezone_card_refs(){legacy();}
void apply_timezone_card_text(const TimezoneCardRef&,bool,time_t,const std::string&,bool){legacy();}
struct WeatherForecastCardRef {std::string entity_id;bool valid=false;float high=0,low=0;std::string source_unit;};
WeatherForecastCardRef* weather_forecast_card_refs(){legacy();static WeatherForecastCardRef r{};return &r;}
int weather_forecast_card_count(){legacy();return 0;}
bool weather_forecast_card_ref_ready(const WeatherForecastCardRef&){legacy();return true;}
void apply_weather_forecast_card_text(const WeatherForecastCardRef&,bool,float,float,const std::string&){legacy();}
struct Source {bool loading=false,stale=false;struct{int count=0;}forecast;std::string entity_id;};
std::vector<std::shared_ptr<Source>>& weather_daily_sources(){legacy();static std::vector<std::shared_ptr<Source>> s;return s;}
void weather_daily_refresh_visuals(){legacy();}
void notify_dashboard_content_changed(){legacy();}
void weather_forecast_schedule_visual_refresh(){legacy();}
uint32_t& weather_forecast_action_ready_ms(){legacy();static uint32_t t=0;return t;}
void weather_forecast_clear_queue(){legacy();}
void weather_forecast_clear_retries(){legacy();}
struct WeatherForecastPendingRequest {uint32_t call_id=0,started_ms=0;std::string entity_id,day;};
[[maybe_unused]] constexpr int WEATHER_FORECAST_PENDING_MAX=1;
[[maybe_unused]] constexpr uint32_t WEATHER_FORECAST_REQUEST_TIMEOUT_MS=60000;
WeatherForecastPendingRequest* weather_forecast_pending_requests(){legacy();static WeatherForecastPendingRequest r[1];return r;}
void ha_cancel_action_response_callback(uint32_t,const char*){legacy();}
void apply_weather_forecast_unavailable_for_entity(const std::string&){legacy();}
bool weather_forecast_schedule_retry(const std::string&,const std::string&,const char*){legacy();return false;}
namespace esphome {uint32_t millis(){return 0;}}
#define ESP_LOGW(...)
bool weather_forecast_actions_ready(){legacy();return false;}
bool weather_forecast_any_pending(){legacy();return false;}
void weather_forecast_enqueue_due_retries(){legacy();}
bool weather_forecast_dequeue(std::string&,std::string&){legacy();return false;}
void request_weather_forecast_entity(const std::string&,const std::string&){legacy();}
void weather_forecast_enqueue(const std::string&,const std::string&){legacy();}
struct ClimateControlCtx {};
ClimateControlCtx** climate_control_refs(){legacy();static ClimateControlCtx c;static ClimateControlCtx* p=&c;return &p;}
int climate_control_ref_count(){legacy();return 1;}
void climate_update_card(ClimateControlCtx*){legacy();}
void climate_control_set_modal_value(ClimateControlCtx*){legacy();}
bool display_temperature_uses_fahrenheit(){return true;}
'''
suffix=r'''
static int dates=0,times=0,zones=0;
int main(){
#ifdef EXPECT_WEATHER_DISABLED
 const int expected_forecasts=0;
#else
 const int expected_forecasts=1;
#endif
#ifdef EXPECT_NATIVE
 static_assert(espcontrol::pooled_application_selected());
#else
 assert(!espcontrol::pooled_application_selected());
#endif
 Navigation nav;espcontrol::application_scene_navigation()=&nav;
 espcontrol::calendar_state_events()={&nav,[](void*,bool,int,int){++dates;},[](void*,bool,int,int,int,int,bool){++times;}};
 espcontrol::timezone_state_events()={&nav,[](void*,bool,time_t,std::string_view,bool){++zones;return true;}};
 update_calendar_cards(true,19,8);update_calendar_cards_time(true,19,8,10,35,true);update_timezone_cards(true,1234,"UTC",true);
 assert(dates==1&&times==1&&zones==1);
 const auto& date=calendar_date_state();const auto& zone=timezone_card_state();
 assert(date.date_valid&&date.day==19&&date.month==8&&date.time_valid&&date.hour==10&&date.minute==35&&date.use_12h);
 assert(zone.valid&&zone.epoch==1234&&zone.active_timezone=="UTC"&&zone.use_12h);
 refresh_calendar_cards();refresh_weather_forecast_card_visuals();refresh_temperature_unit_labels();refresh_weather_forecast_cards();
 weather_forecast_cancel_pending_requests();assert(!weather_forecast_cancel_stale_requests());weather_forecast_send_next_queued();
#ifdef EXPECT_NATIVE
 assert(legacy_calls==0);assert(nav.paints==2&&nav.forecasts==expected_forecasts&&nav.units==1);
 espcontrol::application_scene_navigation()=nullptr;
 refresh_calendar_cards();refresh_weather_forecast_card_visuals();refresh_temperature_unit_labels();refresh_weather_forecast_cards();
 weather_forecast_cancel_pending_requests();assert(!weather_forecast_cancel_stale_requests());weather_forecast_send_next_queued();
 update_calendar_cards(false,0,0);update_timezone_cards(false,0,"UTC",false);
 assert(legacy_calls==0&&dates==2&&zones==2); // Boot without a bank still retains producer state/events.
#else
 assert(legacy_calls>0&&nav.paints==0&&nav.forecasts==0&&nav.units==0);
 const auto previous=legacy_calls;espcontrol::select_pooled_application();
 refresh_calendar_cards();refresh_weather_forecast_card_visuals();refresh_temperature_unit_labels();refresh_weather_forecast_cards();
 assert(legacy_calls==previous&&nav.paints==2&&nav.forecasts==expected_forecasts&&nav.units==1);
#endif
 espcontrol::application_scene_navigation()=nullptr;
}
'''
body='\n'.join(functions)
disabled_source=(headers/'button_grid_weather_forecast.h').read_text();disabled_source=disabled_source[:disabled_source.index('#else\n\nstruct WeatherForecastCardRef')]
disabled_functions=[]
for text,names in [(dt,['calendar_date_state','timezone_card_state','refresh_calendar_cards','update_calendar_cards','update_calendar_cards_time','update_timezone_cards']),(disabled_source,['refresh_weather_forecast_card_visuals','weather_forecast_cancel_pending_requests','weather_forecast_cancel_stale_requests','weather_forecast_send_next_queued','refresh_weather_forecast_cards']),(cfg,['refresh_temperature_unit_labels'])]:
 for name in names:disabled_functions.append(extract.definition(text,name)[1])
variants=[('standalone','',body,True),('forecast-disabled-native','ESPCONTROL_UI_PROFILE_GUITION_ESP32_S3_4848S040','\n'.join(disabled_functions),True)];flags=re.findall(r'#(?:el)?if defined\((ESPCONTROL_UI_PROFILE_[A-Z0-9_]+)\)\n#define ESPCONTROL_DEVICE_UI_SLOT_COUNT (\d+)',(headers/'device_ui_limits_generated.h').read_text());assert len(flags)==9
for flag,slots in flags:variants.append((flag.removeprefix('ESPCONTROL_UI_PROFILE_').lower(),flag,body,True))
mutations={
 'missing-temperature-return':body.replace('      scene->refresh_temperature_unit(display_temperature_uses_fahrenheit());\n    return;','      scene->refresh_temperature_unit(display_temperature_uses_fahrenheit());'),
 'missing-date-event':body.replace('  if (events.date) events.date(events.owner, valid, day, month);','  (void) events;'),
 'missing-forecast-forwarding':body.replace('if (auto *scene = espcontrol::application_scene_navigation()) scene->refresh_weather_forecasts();','(void) espcontrol::application_scene_navigation();')}
for name,text in mutations.items():assert text!=body;variants.append((name,'ESPCONTROL_UI_PROFILE_GUITION_ESP32_S3_4848S040',text,False))
rows=[]
for name,flag,text,expected in variants:
 cpp=directory/(name+'.cpp');cpp.write_text(prefix+nav+text+suffix);exe=directory/name;command=shlex.split(os.environ.get('CXX','c++'))+['-std=c++17','-Wall','-Wextra','-Werror','-Wno-unused-function','-UNDEBUG','-I'+str(headers),str(cpp),'-o',str(exe)]
 if flag:command+=['-D'+flag,'-DEXPECT_NATIVE=1']
 if name=='forecast-disabled-native':command+=['-DESPCONTROL_DISABLE_WEATHER_FORECAST=1','-DEXPECT_WEATHER_DISABLED=1']
 compiled=subprocess.run(command,capture_output=True,text=True);(directory/(name+'.compile.log')).write_text(compiled.stdout+compiled.stderr);assert compiled.returncode==0,(name,compiled.stderr)
 result=subprocess.run([str(exe)],capture_output=True,text=True,timeout=10);(directory/(name+'.run.log')).write_text(result.stdout+result.stderr);assert (result.returncode==0)==expected,(name,result.returncode,result.stderr);rows.append({'name':name,'status':'passed' if expected else 'unsafe_mutation_rejected','physical_execution':False})
result={'status':'extracted_actual_candidate_producer_routing_passed','cases':rows,'source_hashes':{str(p.relative_to(src)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [headers/'page_scene_selection.h',headers/'button_grid_datetime_cards.h',headers/'button_grid_config.h',headers/'button_grid_weather_forecast.h']},'production_modified':False,'CXX':os.environ.get('CXX','c++'),'scope':'Actual complete extracted candidate function bodies, lightweight legacy/LVGL sinks. Central date/time state and events, explicit calendar repaint, forecast/temperature dispatch, no legacy traversal inprofile native/absent-bank, standalonelegacy/manual-native semantics. Missingtemperature-return/dateevent/forecastforward mutations rejected. This isnotfulltargetELF removal orphysicalparity.','at':datetime.datetime.now(datetime.timezone.utc).isoformat()}
if args.report:
 assert not args.report.exists();args.report.write_text(json.dumps(result,indent=2)+'\n');args.report.chmod(0o600)
print('PASS',len(rows),'actual producer routing/profile/standalone/disabled/mutation cases.')
temporary.cleanup()
