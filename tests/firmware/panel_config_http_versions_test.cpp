#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "page_store_runtime.h"
#include "page_renderer_runtime.h"
#include "page_runtime_resources_group.h"
#include "panel_config_read_endpoint.h"
#include "panel_config_write_endpoint.h"
#include "panel_config_service_validator.h"
#include "panel_config_legacy_adapter.h"

using namespace espcontrol;
using namespace espcontrol::configuration;
using namespace espcontrol::pages;
namespace {
using Bytes = std::vector<uint8_t>;
size_t allocation_calls=0, fail_at=0, live_bytes=0, peak_bytes=0;
std::map<uint8_t *,size_t> allocations;
uint8_t *allocate(size_t size) {
  if (++allocation_calls==fail_at) return nullptr;
  auto *p=static_cast<uint8_t *>(std::malloc(size)); assert(p);
  allocations[p]=size; live_bytes+=size; peak_bytes=std::max(peak_bytes,live_bytes); return p;
}
void release(uint8_t *p) {
  assert(allocations.count(p)); live_bytes-=allocations.at(p); allocations.erase(p); std::free(p);
}
std::map<std::string,std::string> records(const Bytes &input) {
  PanelConfigReader reader(input.data(),input.size()); assert(reader.validate()==PanelConfigStatus::OK);
  assert(reader.begin()==PanelConfigStatus::OK); PanelConfigRecord row;
  std::map<std::string,std::string> result;
  while(reader.next(&row)==PanelConfigStatus::OK) {
    const std::string key=std::to_string(static_cast<int>(row.type))+":"+std::to_string(row.slot)+":"+
      (row.key_size?std::string(reinterpret_cast<const char *>(row.key),row.key_size):"");
    result[key]=std::string(reinterpret_cast<const char *>(row.value),row.value_size);
  }
  return result;
}
Bytes document(const DeviceUiLimits &limits,bool maximum=false,size_t child_override=0) {
  Bytes bytes(40*1024); PanelConfigWriter writer(bytes.data(),bytes.size());
  assert(writer.begin()==PanelConfigStatus::OK);
  assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),std::strlen(limits.device_profile))==PanelConfigStatus::OK);
  const size_t roots=maximum?limits.slot_count:1;
  for(size_t slot=1;slot<=roots;++slot) {
    const std::string folder=";Room;Home;Auto;indicator;;subpage;;";
    assert(writer.append_button(slot,reinterpret_cast<const uint8_t *>(folder.data()),folder.size())==PanelConfigStatus::OK);
    std::string children="~B=Parent%3A%20Home:FF8C00";
    const size_t child_count=child_override?child_override:maximum?limits.cards_per_subpage:1;
    for(size_t child=1;child<=child_count;++child)children+=","+std::to_string(child);
    for(size_t child=1;child<=child_count;++child)
      children+="|S,sensor.room_"+std::to_string(slot)+"_"+std::to_string(child)+",Temp%20%C2%B0C,Auto,Auto,,%C2%B0C,1,";
    assert(writer.append_subpage(slot,reinterpret_cast<const uint8_t *>(children.data()),children.size())==PanelConfigStatus::OK);
  }
  std::string order;
  for(size_t slot=1;slot<=roots;++slot){if(slot>1)order+=",";order+=std::to_string(slot);}
  assert(writer.append_setting(reinterpret_cast<const uint8_t*>("button_order"),12,reinterpret_cast<const uint8_t*>(order.data()),order.size())==PanelConfigStatus::OK);
  const std::string opaque="~future.dormant,Retained,Auto,Auto,,,unknown_future,,opaque=%00";
  assert(writer.append_button(32,reinterpret_cast<const uint8_t *>(opaque.data()),opaque.size())==PanelConfigStatus::OK);
  const std::string key="unknown_setting",value="\xEF\xBB\xBF" "unicode \xF0\x9F\x8C\x8D";
  assert(writer.append_setting(reinterpret_cast<const uint8_t *>(key.data()),key.size(),reinterpret_cast<const uint8_t *>(value.data()),value.size())==PanelConfigStatus::OK);
  size_t size=0;assert(writer.finish(&size)==PanelConfigStatus::OK);bytes.resize(size);return bytes;
}
Bytes nested_document() {
  Bytes bytes(4096);alignas(8)std::array<uint8_t,4096> scratch{};
  PanelConfigV2Writer writer(bytes.data(),bytes.size());assert(writer.begin()==PanelConfigStatus::OK);
  PanelConfigV2Record r;r.kind=PanelConfigV2Kind::DEVICE_PROFILE;r.text=DEVICE_UI_PROFILES[8].device_profile;
  assert(writer.append(r)==PanelConfigStatus::OK);
  r={};r.kind=PanelConfigV2Kind::STRING;r.id=1;r.text="subpage";assert(writer.append(r)==PanelConfigStatus::OK);
  for(uint16_t page=0;page<3;++page) {
    r={};r.kind=PanelConfigV2Kind::PAGE;r.id=page;r.first=page?page-1:65535;r.second=page?page:65535;
    assert(writer.append(r)==PanelConfigStatus::OK);
    if(page==2)break;
    r={};r.kind=PanelConfigV2Kind::CARD;r.id=page+1;r.first=page;r.second=1;r.field_mask=1<<6;r.fields[6]=1;
    assert(writer.append(r)==PanelConfigStatus::OK);
    r={};r.kind=PanelConfigV2Kind::NAVIGATION;r.id=page+1;r.first=page+1;assert(writer.append(r)==PanelConfigStatus::OK);
  }
  size_t size=0;assert(writer.finish(scratch.data(),scratch.size(),&size)==PanelConfigStatus::OK);
  bytes.resize(size);return bytes;
}
Bytes disagreeing_raw_source(const Bytes &source) {
  std::vector<uint64_t> index(source.size());
  PanelConfigV2Reader reader(source.data(),source.size());assert(reader.validate(index.data(),index.size()*8)==PanelConfigStatus::OK);
  Bytes output(source.size()+32);PanelConfigV2Writer writer(output.data(),output.size());assert(writer.begin()==PanelConfigStatus::OK);
  PanelConfigV2Record row;
  for(size_t i=0;i<reader.record_count();++i) {
    assert(reader.record(i,&row));
    if(row.kind==PanelConfigV2Kind::CARD&&row.id==1) {row.field_mask|=1<<1;row.fields[1]=row.fields[6];}
    assert(writer.append(row)==PanelConfigStatus::OK);
  }
  size_t size=0;assert(writer.finish(index.data(),index.size()*8,&size)==PanelConfigStatus::OK);
  output.resize(size);return output;
}
struct Backend:StorageBackend {
  std::array<Bytes,2> slots{Bytes(65536,255),Bytes(65536,255)};
  bool fail=false;size_t writes=0;
  size_t slot_capacity()const override{return slots[0].size();}
  bool read(uint8_t slot,size_t offset,uint8_t *data,size_t size)override {
    if(slot>=2||offset>slots[slot].size()||size>slots[slot].size()-offset)return false;
    std::memcpy(data,slots[slot].data()+offset,size);return true;
  }
  bool write(uint8_t slot,size_t offset,const uint8_t *data,size_t size)override {
    if(fail||slot>=2||offset>slots[slot].size()||size>slots[slot].size()-offset)return false;
    std::memcpy(slots[slot].data()+offset,data,size);++writes;return true;
  }
  bool sync()override{return !fail;}
};
struct Legacy:LegacyConfigurationAdapter {
  struct Text:PanelConfigTextValue {
    size_t maximum=255,writes=0;std::string state;
    const std::string& value()const override{return state;}
    bool can_persist_value(size_t size)const override{return size<=maximum;}
    bool set_value(const char* value,size_t size)override {assert(size<=maximum);state.assign(value,size);++writes;return true;}
    bool publish_value(const char* value,size_t size)override {state.assign(value,size);return true;}
  };
  Text order,color;std::array<Text,32> buttons;std::array<std::array<Text,8>,32> chunks;
  PanelConfigTextBindings bindings;PanelConfigLegacyAdapter adapter{bindings,allocate,release};
  bool mirror_ok=true;size_t mirrors=0,publications=0;
  explicit Legacy(const DeviceUiLimits& limits) {
    order.maximum=120;color.maximum=6;
    bindings.set_device_profile(limits.device_profile);bindings.set_button_order(&order);bindings.set_button_on_color(&color);
    for(size_t i=0;i<limits.slot_count;++i) {
      std::array<PanelConfigTextValue*,8> pointers{};
      for(size_t c=0;c<limits.legacy_subpage_chunks;++c)pointers[c]=&chunks[i][c];
      assert(bindings.set_button(i+1,&buttons[i],pointers));
    }
  }
  LegacyLoadResult load(uint8_t *,size_t)override{return {};}
  CompatibilityStatus admit_write(uint16_t version,const uint8_t* data,size_t size)override{return adapter.admit_write(version,data,size);}
  bool publish(uint16_t version,const uint8_t* data,size_t size)override{++publications;return adapter.publish(version,data,size);}
  bool mirror(uint16_t version,const uint8_t* data,size_t size)override{assert(version==1);++mirrors;return mirror_ok&&adapter.mirror(version,data,size);}
  size_t preference_writes()const {
    size_t count=order.writes+color.writes;
    for(const auto& text:buttons)count+=text.writes;
    for(const auto& row:chunks)for(const auto& text:row)count+=text.writes;
    return count;
  }
};
struct Arena:PageArenaAllocator {
  bool fail=false;size_t calls=0;
  void *allocate(size_t size)override{++calls;return fail?nullptr:std::malloc(size);}
  void release(void *p,size_t)override{std::free(p);}
};
struct RendererPort:PageRendererPort {
  const DeviceUiLimits& limits;
  explicit RendererPort(const DeviceUiLimits& value):limits(value){}
  PageViewGeometry geometry(const PageStore&)const override{return {limits.slot_count,limits.grid_columns};}
  bool prepare(const PageStore&,size_t)override{return true;}
  void discard_prepared()override{}
  void activate_prepared(const PageStore&)override{}
  void retire_active()override{}
  void unbind(size_t,PageBindingToken)override{}
  void bind(size_t,const PageStore&,const PageCellLayout&,PageBackStyleView,PageBindingToken)override{}
  void update(size_t,PageBindingToken)override{}
  void suspend()override{}
  bool state_changed()const override{return false;}
};
std::unique_ptr<PageRuntimeResources> renderer(const DeviceUiLimits& limits,Arena& arena,RendererPort& port) {
  switch(limits.slot_count) {
    case 6:return std::make_unique<PageRendererRuntime<6>>(limits,arena,port);
    case 9:return std::make_unique<PageRendererRuntime<9>>(limits,arena,port);
    case 15:return std::make_unique<PageRendererRuntime<15>>(limits,arena,port);
    case 20:return std::make_unique<PageRendererRuntime<20>>(limits,arena,port);
    default:assert(false);return {};
  }
}
struct Fixture {
  Backend backend;ConfigurationStore store{backend};Legacy legacy;PanelConfigDocumentValidator validator{allocate,release};
  ConfigurationService service{store,legacy,&validator};ConfigurationMainLoop queue;Arena arena;
  RendererPort port;std::unique_ptr<PageRuntimeResources> render_resources;PageRuntimeResourcesGroup<1> resources;
  PageStoreRuntime runtime;PanelConfigReadHandler reader;PanelConfigWriteHandler writer;
  explicit Fixture(const DeviceUiLimits &limits):legacy(limits),port(limits),render_resources(renderer(limits,arena,port)),
      resources({{render_resources.get()}}),runtime(limits,arena,128*1024,&resources) {
    service.set_runtime_adapter(&runtime);
    bind_panel_config_http_context(service,queue,40*1024,allocate,release,"user","password");
    const auto source=document(limits);assert(service.save_if_generation(0,1,source.data(),source.size()).ok());
  }
  ~Fixture(){queue.stop();panel_config_http_context().ready=false;assert(allocations.empty());}
  void run(esphome::web_server_idf::AsyncWebHandler &handler,esphome::web_server_idf::AsyncWebServerRequest &request) {
    std::atomic<bool>done{false};
    std::thread http([&]{handler.handleRequest(&request);done=true;});
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    while(!done&&std::chrono::steady_clock::now()<deadline){queue.process(service);std::this_thread::yield();}
    assert(done);http.join();
  }
  httpd_req_t get(const char *version=nullptr) {
    httpd_req_t raw;if(version)raw.headers[PANEL_CONFIG_VERSION_HEADER]=version;
    esphome::web_server_idf::AsyncWebServerRequest request(raw);assert(reader.canHandle(&request));run(reader,request);return raw;
  }
  httpd_req_t put(const Bytes &bytes,const char *version,const char *etag,bool complete=true) {
    httpd_req_t raw;raw.method=HTTP_PUT;raw.length=bytes.size();
    raw.headers["Content-Type"]="application/vnd.espcontrol.panel-config";
    if(version)raw.headers[PANEL_CONFIG_VERSION_HEADER]=version;
    if(etag)raw.headers["If-Match"]=etag;
    esphome::web_server_idf::AsyncWebServerRequest request(raw);assert(writer.canHandle(&request));
    if(writer.canReceiveBody(&request)) {
      const size_t first=std::min(size_t(17),bytes.size());
      writer.handleBody(&request,const_cast<uint8_t *>(bytes.data()),first,0,bytes.size());
      if(complete)writer.handleBody(&request,const_cast<uint8_t *>(bytes.data()+first),bytes.size()-first,first,bytes.size());
      run(writer,request);
    }
    return raw;
  }
  void seed_v2(const Bytes &document) {
    seed(2,document);
  }
  void seed(uint8_t version,const Bytes &document) {
    Bytes encoded{'E','C','D','O',version,0,8,0};encoded.insert(encoded.end(),document.begin(),document.end());
    assert(store.commit(encoded.data(),encoded.size()).ok());
  }
};
int status(const httpd_req_t &request){return std::stoi(request.status);}
void profiles_and_cas() {
  for(const auto &limits:DEVICE_UI_PROFILES) {
    Fixture f(limits);
    const auto original=f.get();assert(status(original)==200&&original.sent_headers.at(PANEL_CONFIG_VERSION_HEADER)=="1");
    const auto v2=f.get("2");assert(status(v2)==200&&v2.sent_headers.at(PANEL_CONFIG_VERSION_HEADER)=="2");
    assert(v2.sent_headers.at("Vary")==PANEL_CONFIG_VERSION_HEADER&&v2.sent_headers.at("X-Panel-Config-Durable-Version")=="1");
    assert(original.sent_headers.at("X-Panel-Config-Generation")==v2.sent_headers.at("X-Panel-Config-Generation"));
    assert(status(f.put(v2.response,nullptr,"\"1\""))==400); // Explicit opt-in is required.
    const auto saved=f.put(v2.response,"2","\"v2-1\"");assert(status(saved)==204&&saved.sent_headers.at("ETag")=="\"v2-2\"");
    assert(saved.sent_headers.at(PANEL_CONFIG_VERSION_HEADER)=="2"&&saved.sent_headers.at("X-Panel-Config-Durable-Version")=="1");
    assert(f.runtime.active().generation()==2&&records(f.get().response)==records(original.response));
    assert(status(f.put(original.response,"1","\"1\""))==409);
    const auto conflict=f.put(v2.response,"2","\"v2-1\"");assert(status(conflict)==409&&conflict.sent_headers.at("ETag")=="\"v2-2\"");
    assert(f.get("2").sent_headers.at("ETag")=="\"v2-2\"");
    const auto maximum=document(limits,true);
    PanelConfigRepresentation converted(allocate,release);
    assert(converted.convert(1,2,maximum.data(),maximum.size(),PANEL_CONFIG_V2_HTTP_MAX_BYTES)==PanelConfigRepresentationStatus::OK);
    const Bytes max_v2(converted.data(),converted.data()+converted.size());
    assert(status(f.put(max_v2,"2","\"v2-2\""))==204);
    assert(records(f.get().response)==records(maximum));
    assert(f.runtime.active().page(0)->runtime_card_count==limits.slot_count&&!f.runtime.active().runtime_card(31));
  }
  assert(allocations.empty());
}
void profile_capacity_write_limits() {
  for(const auto& limits:DEVICE_UI_PROFILES) {
    Fixture f(limits);const auto original=f.get();const auto too_many=document(limits,false,limits.cards_per_subpage+1);
    assert(status(f.put(too_many,"1","\"1\""))==413);
    assert(f.get().response==original.response&&f.runtime.active().generation()==1);
    PanelConfigRepresentation converted(allocate,release);
    assert(converted.convert(1,2,too_many.data(),too_many.size(),PANEL_CONFIG_V2_HTTP_MAX_BYTES)==PanelConfigRepresentationStatus::OK);
    const Bytes v2(converted.data(),converted.data()+converted.size());
    assert(status(f.put(v2,"2","\"v2-1\""))==413);
    assert(f.get().response==original.response&&f.runtime.active().generation()==1);
  }
}
Bytes replace_record(const Bytes& source,PanelConfigRecordType type,uint8_t slot,const std::string& key,const std::string& value) {
  Bytes output(40*1024);PanelConfigWriter writer(output.data(),output.size());assert(writer.begin()==PanelConfigStatus::OK);
  PanelConfigReader reader(source.data(),source.size());assert(reader.begin()==PanelConfigStatus::OK);PanelConfigRecord row;bool replaced=false;
  auto append=[&](PanelConfigRecordType kind,uint8_t index,const uint8_t* name,size_t name_size,const uint8_t* data,size_t size) {
    const auto status=kind==PanelConfigRecordType::DEVICE_PROFILE?writer.append_device_profile(data,size):
      kind==PanelConfigRecordType::BUTTON?writer.append_button(index,data,size):
      kind==PanelConfigRecordType::SUBPAGE?writer.append_subpage(index,data,size):writer.append_setting(name,name_size,data,size);
    assert(status==PanelConfigStatus::OK);
  };
  while(reader.next(&row)==PanelConfigStatus::OK) {
    const bool match=row.type==type&&row.slot==slot&&row.key_size==key.size()&&(!row.key_size||std::memcmp(row.key,key.data(),key.size())==0);
    if(match){append(type,slot,reinterpret_cast<const uint8_t*>(key.data()),key.size(),reinterpret_cast<const uint8_t*>(value.data()),value.size());replaced=true;}
    else append(row.type,row.slot,row.key,row.key_size,row.value,row.value_size);
  }
  if(!replaced)append(type,slot,reinterpret_cast<const uint8_t*>(key.data()),key.size(),reinterpret_cast<const uint8_t*>(value.data()),value.size());
  size_t size=0;assert(writer.finish(&size)==PanelConfigStatus::OK);output.resize(size);return output;
}
void legacy_fixed_write_limits() {
  for(const auto& limits:DEVICE_UI_PROFILES)for(uint16_t version:{1,2}) {
    Fixture f(limits);const auto original=f.get();const auto source=original.response;uint32_t generation=1;
    auto put=[&](const Bytes& bytes) {
      const auto etag=(version==1?std::string("\""):std::string("\"v2-"))+std::to_string(generation)+"\"";
      if(version==1)return f.put(bytes,"1",etag.c_str());
      PanelConfigRepresentation converted(allocate,release);
      assert(converted.convert(1,2,bytes.data(),bytes.size(),PANEL_CONFIG_V2_HTTP_MAX_BYTES)==PanelConfigRepresentationStatus::OK);
      const Bytes v2(converted.data(),converted.data()+converted.size());return f.put(v2,"2",etag.c_str());
    };
    const auto writes=f.backend.writes,arenas=f.arena.calls,mirrors=f.legacy.mirrors,published=f.legacy.publications,preferences=f.legacy.preference_writes();
    auto reject=[&](const Bytes& bytes) {
      const auto result=put(bytes);assert(status(result)==413&&result.response.size()>0);
      assert(f.backend.writes==writes&&f.arena.calls==arenas&&f.legacy.mirrors==mirrors&&f.legacy.publications==published&&f.legacy.preference_writes()==preferences);
      const auto unchanged=f.get();assert(unchanged.response==source&&unchanged.sent_headers.at("ETag")=="\"1\""&&f.runtime.active().generation()==1);
    };
    reject(replace_record(source,PanelConfigRecordType::BUTTON,1,"",std::string(256,'x')));
    reject(replace_record(source,PanelConfigRecordType::SETTING,0,"button_order",std::string(121,'x')));
    reject(replace_record(source,PanelConfigRecordType::SETTING,0,"button_on_color","1234567"));
    const size_t ceiling=limits.legacy_subpage_byte_ceiling();
    reject(replace_record(source,PanelConfigRecordType::SUBPAGE,1,"",std::string("~B|,switch.one,")+std::string(ceiling+1-15,'x')));
    const std::string prefix="~B|,switch.one,";assert(prefix.size()==15);
    // Four-byte glyphs cannot straddle the deployed 255-byte preference fields:
    // first chunk holds 15 ASCII bytes +60 glyphs; each later chunk 63 glyphs.
    const size_t glyphs=60+63*(limits.legacy_subpage_chunks-1);
    std::string unicode=prefix;
    for(size_t i=0;i<=glyphs;++i)unicode+="🌡";
    assert(unicode.size()<=ceiling);
    reject(replace_record(source,PanelConfigRecordType::SUBPAGE,1,"",unicode));
    unicode=prefix;for(size_t i=0;i<glyphs;++i)unicode+="🌡";
    const auto legal=replace_record(source,PanelConfigRecordType::SUBPAGE,1,"",unicode);
    assert(status(put(legal))==204&&f.runtime.active().generation()==++generation);
    auto accepted=legal;
    const std::string card_prefix="light.one;",card_suffix=";Auto;Auto;;;;;";
    const auto root=card_prefix+std::string(255-card_prefix.size()-card_suffix.size(),'x')+card_suffix;
    assert(root.size()==255);
    accepted=replace_record(accepted,PanelConfigRecordType::BUTTON,1,"",root);
    assert(status(put(accepted))==204&&f.runtime.active().generation()==++generation);
    accepted=replace_record(accepted,PanelConfigRecordType::SETTING,0,"button_order","1"+std::string(119,','));
    assert(status(put(accepted))==204&&f.runtime.active().generation()==++generation);
    accepted=replace_record(accepted,PanelConfigRecordType::SETTING,0,"button_on_color","A1B2C3");
    assert(status(put(accepted))==204&&f.runtime.active().generation()==++generation);
    const std::string key="native.future";
    accepted=replace_record(accepted,PanelConfigRecordType::SETTING,0,key,std::string(PANEL_CONFIG_MAX_RECORD_BODY_BYTES-1-key.size(),'x'));
    assert(status(put(accepted))==204&&f.runtime.active().generation()==++generation);
    assert(records(f.get().response)==records(accepted));
  }
}
void historical_capacity_reads_and_child_folder_writes() {
  for(const auto& limits:DEVICE_UI_PROFILES)for(uint8_t stored_version:{1,2}) {
    Fixture f(limits);
    const auto historical=document(limits,false,limits.cards_per_subpage+1);
    PanelConfigRepresentation converted(allocate,release);
    assert(converted.convert(1,2,historical.data(),historical.size(),PANEL_CONFIG_V2_HTTP_MAX_BYTES)==PanelConfigRepresentationStatus::OK);
    const Bytes v2(converted.data(),converted.data()+converted.size());
    const auto& stored=stored_version==1?historical:v2;
    f.seed(stored_version,stored);
    const auto writes=f.backend.writes;
    Bytes boot(40*1024);
    assert(f.service.load_and_apply_runtime(boot.data(),boot.size()).ok());
    assert(f.runtime.active().generation()==2&&f.runtime.active().source_size()==stored.size());
    assert(std::memcmp(f.runtime.active().source_document(),stored.data(),stored.size())==0);
    assert(f.backend.writes==writes&&records(f.get().response)==records(historical));
    assert(f.get("2").response==v2);
    assert(status(f.put(historical,"1","\"2\""))==413);
    assert(status(f.put(v2,"2","\"v2-2\""))==413);
    assert(f.backend.writes==writes&&f.runtime.active().generation()==2);
  }
  for(const auto& limits:DEVICE_UI_PROFILES)for(uint16_t version:{1,2}) {
    Fixture f(limits);const auto original=f.get().response;
    const auto child_folder=replace_record(original,PanelConfigRecordType::SUBPAGE,1,"","B,1|:Nested:Auto:Auto:::subpage::");
    PanelConfigRepresentation converted(allocate,release);
    assert(converted.convert(1,2,child_folder.data(),child_folder.size(),PANEL_CONFIG_V2_HTTP_MAX_BYTES)==PanelConfigRepresentationStatus::OK);
    const Bytes v2(converted.data(),converted.data()+converted.size());
    const auto writes=f.backend.writes,preferences=f.legacy.preference_writes();
    const auto rejected=version==1?f.put(child_folder,"1","\"1\""):f.put(v2,"2","\"v2-1\"");
    assert(status(rejected)==413&&f.backend.writes==writes&&f.legacy.preference_writes()==preferences);
    assert(f.runtime.active().generation()==1&&f.get().response==original);
  }
}
void shared_image_write_limits() {
  for(const auto& limits:DEVICE_UI_PROFILES)for(uint16_t version:{1,2}) {
    Fixture f(limits);const auto original=f.get();auto source=original.response;
    const size_t root_images=std::min(size_t(limits.image_slot_count+1),size_t(limits.slot_count-1));
    std::string order="1";
    for(size_t i=0;i<root_images;++i) {
      const std::string value=i%2?"media_player.art;Art;Auto;Auto;cover_art;;media;;":"camera.image;Image;Auto;Auto;;;image;;";
      source=replace_record(source,PanelConfigRecordType::BUTTON,i+2,"",value);order+=","+std::to_string(i+2);
    }
    source=replace_record(source,PanelConfigRecordType::SETTING,0,"button_order",order);
    const size_t child_images=limits.image_slot_count+1-root_images;
    std::string child="B";
    for(size_t i=0;i<child_images;++i)child+=","+std::to_string(i+1);
    for(size_t i=0;i<child_images;++i)child+="|camera.child_"+std::to_string(i)+":Child:Auto:Auto:::image::";
    source=replace_record(source,PanelConfigRecordType::SUBPAGE,1,"",child);
    auto put=[&](const Bytes& bytes) {
      if(version==1)return f.put(bytes,"1","\"1\"");
      PanelConfigRepresentation converted(allocate,release);
      assert(converted.convert(1,2,bytes.data(),bytes.size(),PANEL_CONFIG_V2_HTTP_MAX_BYTES)==PanelConfigRepresentationStatus::OK);
      return f.put(Bytes(converted.data(),converted.data()+converted.size()),"2","\"v2-1\"");
    };
    const auto writes=f.backend.writes,preferences=f.legacy.preference_writes(),publications=f.legacy.publications;
    assert(status(put(source))==413&&f.runtime.active().generation()==1&&f.backend.writes==writes);
    assert(f.get().response==original.response&&f.legacy.preference_writes()==preferences&&f.legacy.publications==publications);
    // Retain the extra definition while removing it from executable layouts.
    // This permits editing inactive data without allocating another image slot.
    if(child_images) {
      child="B,1";
      for(size_t i=0;i<child_images;++i)child+="|camera.child_"+std::to_string(i)+":Child:Auto:Auto:::image::";
      source=replace_record(source,PanelConfigRecordType::SUBPAGE,1,"",child);
    } else {
      order="1";for(size_t i=0;i<root_images-1;++i)order+=","+std::to_string(i+2);
      source=replace_record(source,PanelConfigRecordType::SETTING,0,"button_order",order);
    }
    assert(status(put(source))==204&&f.runtime.active().generation()==2&&records(f.get().response)==records(source));
  }
}
void authentication_and_request_limits() {
  Fixture f(DEVICE_UI_PROFILES[8]);
  const auto calls=allocation_calls;
  for(int method:{HTTP_GET,HTTP_PUT}) {
    httpd_req_t raw;raw.method=method;raw.authenticated=false;
    esphome::web_server_idf::AsyncWebServerRequest request(raw);
    if(method==HTTP_GET)f.run(f.reader,request);else assert(!f.writer.canReceiveBody(&request));
    assert(status(raw)==401&&allocation_calls==calls);
  }
  for(const auto &[version,size]:std::array<std::pair<const char *,size_t>,2>{{{"1",40*1024+1},{"2",PANEL_CONFIG_V2_HTTP_MAX_BYTES+1}}}) {
    httpd_req_t raw;raw.method=HTTP_PUT;raw.length=size;
    raw.headers["Content-Type"]="application/vnd.espcontrol.panel-config";
    raw.headers[PANEL_CONFIG_VERSION_HEADER]=version;raw.headers["If-Match"]=std::strcmp(version,"2")==0?"\"v2-1\"":"\"1\"";
    esphome::web_server_idf::AsyncWebServerRequest request(raw);
    assert(!f.writer.canReceiveBody(&request)&&status(raw)==400&&allocation_calls==calls);
  }
  panel_config_http_context().ready=false;
  assert(status(f.get("2"))==503);
  const auto source=document(DEVICE_UI_PROFILES[8]);assert(status(f.put(source,"2","\"v2-1\""))==503);
  panel_config_http_context().ready=true;
  assert(allocation_calls==calls&&f.writer.maximumBodySize()==PANEL_CONFIG_V2_HTTP_MAX_BYTES);
}
void converted_owner_boundaries() {
  const auto source=document(DEVICE_UI_PROFILES[8]);
  PanelConfigRepresentation converted(allocate,release);
  assert(converted.convert(1,2,source.data(),source.size(),PANEL_CONFIG_V2_HTTP_MAX_BYTES)==PanelConfigRepresentationStatus::OK);
  const Bytes v2(converted.data(),converted.data()+converted.size());
  const auto *borrowed=converted.data();const size_t borrowed_size=converted.size();
  assert(converted.convert(2,1,borrowed,borrowed_size,40*1024)==PanelConfigRepresentationStatus::INVALID_DOCUMENT);
  assert(converted.data()==borrowed&&converted.size()==borrowed_size&&Bytes(borrowed,borrowed+borrowed_size)==v2);
  assert(converted.convert(2,1,v2.data(),v2.size(),16)==PanelConfigRepresentationStatus::TOO_LARGE);
  assert(!converted.data()&&!converted.size()&&allocations.empty());
}
void rejection_and_failure() {
  Fixture f(DEVICE_UI_PROFILES[8]);const auto source=f.get().response;const auto v2=f.get("2").response;
  for(const char *version:{"", "3", "02", " 2", "2 "}) {
    assert(status(f.get(version))==406);assert(status(f.put(v2,version,"\"1\""))==415);
  }
  assert(status(f.put(v2,"2",nullptr))==428);
  assert(status(f.put(v2,"2","\"v2-1\"",false))==400);
  assert(status(f.put(nested_document(),"2","\"v2-1\""))==400);
  assert(status(f.put(disagreeing_raw_source(v2),"2","\"v2-1\""))==400);
  auto malformed=v2;malformed[0]=0;assert(status(f.put(malformed,"2","\"v2-1\""))==400);
  for(size_t failure=1;failure<=3;++failure) {
    fail_at=allocation_calls+failure;assert(status(f.get("2"))==503);fail_at=0;assert(allocations.empty());
    fail_at=allocation_calls+failure;assert(status(f.put(v2,"2","\"v2-1\""))==503);fail_at=0;assert(allocations.empty());
    assert(f.runtime.active().generation()==1&&f.get().response==source);
  }
  f.arena.fail=true;assert(status(f.put(v2,"2","\"v2-1\""))==503);f.arena.fail=false;
  f.backend.fail=true;assert(status(f.put(v2,"2","\"v2-1\""))==500);f.backend.fail=false;
  assert(f.runtime.active().generation()==1&&f.get().response==source);
  f.legacy.mirror_ok=false;const auto warning=f.put(v2,"2","\"v2-1\"");
  assert(status(warning)==202&&warning.sent_headers.at("X-Panel-Config-Legacy-Mirror")=="failed");
  assert(f.runtime.active().generation()==2&&records(f.get().response)==records(source));
  // A fresh boot reader still sees durable v1 after a v2 request.
  Bytes loaded(40*1024);const auto boot=f.service.load(loaded.data(),loaded.size());
  assert(boot.ok()&&boot.document_version==1&&boot.generation==2);
}
void stored_v2_reads_and_replacements() {
  for(const auto &limits:DEVICE_UI_PROFILES) {
    Fixture fixture(limits);const auto original=fixture.get().response;
    Bytes source;
    {
      PanelConfigRepresentation imported(allocate,release);
      assert(imported.convert(1,2,original.data(),original.size(),PANEL_CONFIG_V2_HTTP_MAX_BYTES)==PanelConfigRepresentationStatus::OK);
      source.assign(imported.data(),imported.data()+imported.size());
    }
    fixture.seed_v2(source);
    const auto v2=fixture.get("2");assert(status(v2)==200&&v2.response==source);
    assert(v2.sent_headers.at("X-Panel-Config-Durable-Version")=="2"&&v2.sent_headers.at("ETag")=="\"v2-2\"");
    const auto v1=fixture.get();assert(status(v1)==200&&records(v1.response)==records(original));
    assert(v1.sent_headers.at("X-Panel-Config-Durable-Version")=="2"&&v1.sent_headers.at(PANEL_CONFIG_VERSION_HEADER)=="1");
    assert(v1.sent_headers.at("ETag")=="\"2\"");
    const auto stale=fixture.put(source,"2","\"v2-1\"");
    assert(status(stale)==409&&!stale.sent_headers.count("X-Panel-Config-Durable-Version"));
    assert(fixture.get("2").response==source);
    Bytes output(40*1024);const auto boot=fixture.service.load_and_apply_runtime(output.data(),output.size());
    assert(boot.ok()&&boot.document_version==2&&boot.generation==2&&fixture.runtime.active().source_version()==2);
    for(size_t failed=1;failed<=4;++failed) {
      fail_at=allocation_calls+failed;assert(status(fixture.get("1"))==503);fail_at=0;
      assert(allocations.empty()&&fixture.get("2").response==source);
    }
    const auto saved=fixture.put(source,"2","\"v2-2\"");assert(status(saved)==204);
    assert(saved.sent_headers.at("X-Panel-Config-Durable-Version")=="1");
    assert(fixture.runtime.active().source_version()==1&&fixture.runtime.active().generation()==3);
    assert(status(fixture.put(v1.response,"1","\"2\""))==409);
    assert(status(fixture.put(source,"2","\"v2-2\""))==409);
  }
  Fixture future(DEVICE_UI_PROFILES[8]);const auto source=nested_document();future.seed_v2(source);
  assert(future.get("2").response==source&&status(future.get("1"))==422);
  Bytes output(40*1024);assert(future.service.load_and_apply_runtime(output.data(),output.size()).status==ServiceStatus::RUNTIME_PREPARE_FAILED);
  assert(future.runtime.active().generation()==1&&future.get("2").response==source);
}
void compressed_compatibility_growth() {
  Bytes input(32768);PanelConfigV2Writer writer(input.data(),input.size());assert(writer.begin()==PanelConfigStatus::OK);
  auto append=[&](const PanelConfigV2Record &row){assert(writer.append(row)==PanelConfigStatus::OK);};
  PanelConfigV2Record row;row.kind=PanelConfigV2Kind::DEVICE_PROFILE;row.text=DEVICE_UI_PROFILES[6].device_profile;append(row);
  for(auto value:std::array<std::pair<uint16_t,const char *>,4>{{
      {1,"subpage"},{2,"sensor"},{3,"A shared label with Unicode 温度 and repeated descriptive text"},{4,"sensor.shared"}}}) {
    row={};row.kind=PanelConfigV2Kind::STRING;row.id=value.first;row.text=value.second;append(row);
  }
  row={};row.kind=PanelConfigV2Kind::ENTITY;row.id=1;row.first=4;append(row);
  row={};row.kind=PanelConfigV2Kind::PAGE;row.first=row.second=65535;append(row);
  for(uint16_t page=1;page<=20;++page) {
    row={};row.kind=PanelConfigV2Kind::CARD;row.id=page;row.second=page;row.field_mask=1<<6;row.fields[6]=1;append(row);
    row={};row.kind=PanelConfigV2Kind::PAGE;row.id=page;row.second=page;append(row);
    row={};row.kind=PanelConfigV2Kind::NAVIGATION;row.id=page;row.first=page;append(row);
    for(uint16_t child=1;child<=19;++child) {
      row={};row.kind=PanelConfigV2Kind::CARD;row.id=100+page*20+child;row.first=page;row.second=child;
      row.field_mask=(1<<0)|(1<<1)|(1<<6);row.fields[0]=1;row.fields[1]=3;row.fields[6]=2;append(row);
    }
  }
  std::vector<uint64_t> workspace((writer.workspace_bytes()+7)/8);size_t used=0;
  assert(writer.finish(workspace.data(),workspace.size()*8,&used)==PanelConfigStatus::OK);input.resize(used);
  const auto before=allocation_calls;size_t total=0;
  {
    PanelConfigRepresentation converted(allocate,release);
    assert(converted.convert(2,1,input.data(),input.size(),65536)==PanelConfigRepresentationStatus::OK);
    assert(converted.size()>input.size()*2&&converted.size()<65536);
    PanelConfigReader reader(converted.data(),converted.size());assert(reader.validate()==PanelConfigStatus::OK);
    total=allocation_calls-before;assert(total>=3&&allocations.size()==1);
  }
  assert(allocations.empty());
  const auto original=input;
  for(size_t failure=1;failure<=total;++failure) {
    PanelConfigRepresentation converted(allocate,release);fail_at=allocation_calls+failure;
    assert(converted.convert(2,1,input.data(),input.size(),65536)==PanelConfigRepresentationStatus::UNAVAILABLE);fail_at=0;
    assert(!converted.data()&&!converted.size()&&allocations.empty()&&input==original);
  }
}
}
int main() {
  historical_capacity_reads_and_child_folder_writes();
  profile_capacity_write_limits();
  legacy_fixed_write_limits();
  shared_image_write_limits();
  stored_v2_reads_and_replacements();compressed_compatibility_growth();
  profiles_and_cas();rejection_and_failure();authentication_and_request_limits();converted_owner_boundaries();
  assert(allocations.empty()&&live_bytes==0&&peak_bytes<3*PANEL_CONFIG_V2_HTTP_MAX_BYTES);
  std::printf("HTTP v1/v2: all nine maximum profiles, shared-generation CAS, request/allocator/storage/runtime rejection; host HTTP allocation peak=%zu bytes; retained=%zu bytes\n",peak_bytes,live_bytes);
}
