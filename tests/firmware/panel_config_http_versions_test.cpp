#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <map>
#include <thread>
#include <vector>

#include "page_store_runtime.h"
#include "panel_config_read_endpoint.h"
#include "panel_config_write_endpoint.h"
#include "panel_config_service_validator.h"

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
Bytes document(const DeviceUiLimits &limits,bool maximum=false) {
  Bytes bytes(40*1024); PanelConfigWriter writer(bytes.data(),bytes.size());
  assert(writer.begin()==PanelConfigStatus::OK);
  assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),std::strlen(limits.device_profile))==PanelConfigStatus::OK);
  const size_t roots=maximum?limits.slot_count:1;
  for(size_t slot=1;slot<=roots;++slot) {
    const std::string folder=";Room;Home;Auto;indicator;;subpage;;";
    assert(writer.append_button(slot,reinterpret_cast<const uint8_t *>(folder.data()),folder.size())==PanelConfigStatus::OK);
    std::string children="~B=Parent%3A%20Home:FF8C00";
    for(size_t child=1;child<=(maximum?limits.cards_per_subpage:1);++child)
      children+="|S,sensor.room_"+std::to_string(slot)+"_"+std::to_string(child)+",Temp%20%C2%B0C,Auto,Auto,,%C2%B0C,1,";
    assert(writer.append_subpage(slot,reinterpret_cast<const uint8_t *>(children.data()),children.size())==PanelConfigStatus::OK);
  }
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
  bool fail=false;
  size_t slot_capacity()const override{return slots[0].size();}
  bool read(uint8_t slot,size_t offset,uint8_t *data,size_t size)override {
    if(slot>=2||offset>slots[slot].size()||size>slots[slot].size()-offset)return false;
    std::memcpy(data,slots[slot].data()+offset,size);return true;
  }
  bool write(uint8_t slot,size_t offset,const uint8_t *data,size_t size)override {
    if(fail||slot>=2||offset>slots[slot].size()||size>slots[slot].size()-offset)return false;
    std::memcpy(slots[slot].data()+offset,data,size);return true;
  }
  bool sync()override{return !fail;}
};
struct Legacy:LegacyConfigurationAdapter {
  bool mirror_ok=true;
  LegacyLoadResult load(uint8_t *,size_t)override{return {};}
  bool mirror(uint16_t version,const uint8_t *,size_t)override{assert(version==1);return mirror_ok;}
};
struct Arena:PageArenaAllocator {
  bool fail=false;
  void *allocate(size_t size)override{return fail?nullptr:std::malloc(size);}
  void release(void *p,size_t)override{std::free(p);}
};
struct Fixture {
  Backend backend;ConfigurationStore store{backend};Legacy legacy;PanelConfigDocumentValidator validator;
  ConfigurationService service{store,legacy,&validator};ConfigurationMainLoop queue;Arena arena;
  PageStoreRuntime runtime;PanelConfigReadHandler reader;PanelConfigWriteHandler writer;
  explicit Fixture(const DeviceUiLimits &limits):runtime(limits,arena,128*1024) {
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
}
int main() {
  profiles_and_cas();rejection_and_failure();authentication_and_request_limits();converted_owner_boundaries();
  assert(allocations.empty()&&live_bytes==0&&peak_bytes<3*PANEL_CONFIG_V2_HTTP_MAX_BYTES);
  std::printf("HTTP v1/v2: all nine maximum profiles, shared-generation CAS, request/allocator/storage/runtime rejection; host HTTP allocation peak=%zu bytes; retained=%zu bytes\n",peak_bytes,live_bytes);
}
