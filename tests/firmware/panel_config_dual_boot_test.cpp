#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "page_store_runtime.h"
#include "panel_config_legacy_adapter.h"
#include "panel_config_runtime_adapter.h"
#include "panel_config_service_validator.h"

using namespace espcontrol;
using namespace espcontrol::configuration;
using namespace espcontrol::pages;
using Bytes=std::vector<uint8_t>;
using Row=PanelConfigV2Record;
using Kind=PanelConfigV2Kind;
namespace {
size_t calls=0,fail_at=0,live=0,peak=0;
std::map<uint8_t *,size_t> allocations;
uint8_t *allocate(size_t size) {
  if(++calls==fail_at)return nullptr;
  auto *value=static_cast<uint8_t *>(std::malloc(size));assert(value);
  allocations[value]=size;live+=size;peak=std::max(peak,live);return value;
}
void release(uint8_t *value) {assert(allocations.count(value));live-=allocations.at(value);allocations.erase(value);std::free(value);}
Bytes native(const DeviceUiLimits &limits,bool maximum) {
  Bytes bytes(32768);PanelConfigV2Writer writer(bytes.data(),bytes.size());
  assert(writer.begin()==PanelConfigStatus::OK);
  auto append=[&](const Row &row){assert(writer.append(row)==PanelConfigStatus::OK);};
  auto text=[&](uint16_t id,const std::string &value){Row row;row.kind=Kind::STRING;row.id=id;row.text=value;append(row);};
  Row row;row.kind=Kind::DEVICE_PROFILE;row.text=limits.device_profile;append(row);
  text(1,"subpage");text(2,"sensor");text(3,"Room");text(4,"1");text(5,"button_order");text(6,"B,1");
  text(7,"future_native_setting");text(8,std::string("opaque\0",7)+"温度🌡️");text(9,"button_on_color");
  row={};row.kind=Kind::PAGE;row.first=row.second=65535;row.third=4;append(row);
  for(size_t p=1;p<=(maximum?limits.slot_count:size_t{1});++p) {
    row={};row.kind=Kind::CARD;row.id=p;row.second=p;row.field_mask=(1<<1)|(1<<6);row.fields[1]=3;row.fields[6]=1;append(row);
    row={};row.kind=Kind::PAGE;row.id=10000+p;row.second=p;row.third=6;append(row);
    row={};row.kind=Kind::NAVIGATION;row.id=p;row.first=10000+p;append(row);
    for(size_t c=1;c<=(maximum?limits.cards_per_subpage:size_t{1});++c) {
      const uint16_t id=p*20+c;const auto suffix=std::to_string(p)+"_"+std::to_string(c);
      text(2000+id,"sensor.room_"+suffix);text(3000+id,"Zone "+suffix+" 🌡️");
      row={};row.kind=Kind::ENTITY;row.id=1000+id;row.first=2000+id;append(row);
      row={};row.kind=Kind::CARD;row.id=100+id;row.first=10000+p;row.second=c;row.field_mask=(1<<0)|(1<<1)|(1<<6);
      row.fields[0]=1000+id;row.fields[1]=3000+id;row.fields[6]=2;append(row);
    }
  }
  // Explicit empty and missing are distinct source records. A no-op callback
  // from compatibility publication must not normalize either into a rewrite.
  if(!maximum){row={};row.kind=Kind::CARD;row.id=2;row.second=2;row.flags=PANEL_CONFIG_V2_RAW_SOURCE;append(row);}
  row={};row.kind=Kind::SETTING;row.id=5;row.first=4;append(row);
  row={};row.kind=Kind::SETTING;row.id=7;row.first=8;append(row);
  row={};row.kind=Kind::SETTING;row.id=9;append(row);
  text(60000,"Dormant");text(60001,"unknown_future");text(60002,"sensor.hidden");
  row={};row.kind=Kind::CARD;row.id=32;row.second=32;row.field_mask=(1<<1)|(1<<6);row.fields[1]=60000;row.fields[6]=60001;append(row);
  row={};row.kind=Kind::PAGE;row.id=60000;row.second=32;row.third=6;append(row);
  row={};row.kind=Kind::ENTITY;row.id=60000;row.first=60002;append(row);
  row={};row.kind=Kind::CARD;row.id=60000;row.first=60000;row.second=1;row.field_mask=(1<<0)|(1<<1)|(1<<6);
  row.fields[0]=60000;row.fields[1]=60000;row.fields[6]=2;append(row);
  std::vector<uint64_t> index((writer.workspace_bytes()+7)/8);size_t size=0;
  assert(writer.finish(index.data(),index.size()*8,&size)==PanelConfigStatus::OK);bytes.resize(size);return bytes;
}
struct Backend final:StorageBackend {
  std::array<Bytes,2> slots{Bytes(32768,255),Bytes(32768,255)};size_t writes=0;bool fail=false;
  size_t slot_capacity()const override{return slots[0].size();}
  bool read(uint8_t slot,size_t offset,uint8_t *out,size_t size)override {
    if(slot>=2||offset>slot_capacity()||size>slot_capacity()-offset)return false;
    std::memcpy(out,slots[slot].data()+offset,size);return true;
  }
  bool write(uint8_t slot,size_t offset,const uint8_t *in,size_t size)override {
    if(fail||slot>=2||offset>slot_capacity()||size>slot_capacity()-offset)return false;
    ++writes;std::memcpy(slots[slot].data()+offset,in,size);return true;
  }
  bool sync()override{return !fail;}
};
struct Arena final:PageArenaAllocator {
  bool fail=false;size_t calls=0,fail_at=0;
  void *allocate(size_t size)override{return (++calls==fail_at||fail)?nullptr:std::malloc(size);}
  void release(void *value,size_t)override{std::free(value);}
};
struct Text final:PanelConfigTextValue {
  std::string state;size_t persistent=0,published=0;bool fail=false;
  const std::string &value()const override{return state;}
  bool set_value(const char *value,size_t size)override {if(fail||size>255)return false;state.assign(value,size);++persistent;return true;}
  bool publish_value(const char *value,size_t size)override {if(fail)return false;state.assign(value,size);++published;return true;}
  bool can_persist_value(size_t size)const override{return !fail&&size<=255;}
};
struct Fixture {
  Backend backend;ConfigurationStore store{backend};Text order,color;
  std::array<Text,32> buttons;std::array<std::array<Text,8>,32> chunks;
  PanelConfigTextBindings bindings;PanelConfigLegacyAdapter legacy{bindings,allocate,release};
  PanelConfigDocumentValidator validator{allocate,release};ConfigurationService service{store,legacy,&validator};
  Arena arena;PageStoreRuntime runtime;
  explicit Fixture(const DeviceUiLimits &limits):runtime(limits,arena,128*1024) {
    bindings.set_device_profile(limits.device_profile);bindings.set_button_order(&order);bindings.set_button_on_color(&color);
    for(size_t i=0;i<limits.slot_count;++i) {
      std::array<PanelConfigTextValue *,8> pointers{};
      for(size_t c=0;c<limits.legacy_subpage_chunks;++c)pointers[c]=&chunks[i][c];
      assert(bindings.set_button(i+1,&buttons[i],pointers));
    }
    service.set_buffer_allocator(allocate,release);service.set_runtime_adapter(&runtime);
  }
  void seed(const Bytes &document) {
    // Seed the actual service ECDO envelope, not a fake bare-codec store.
    Bytes encoded{'E','C','D','O',2,0,8,0};encoded.insert(encoded.end(),document.begin(),document.end());
    assert(store.commit(encoded.data(),encoded.size()).ok());
  }
  void exact(const Bytes &document,uint32_t generation,uint16_t version) {
    Bytes out(32768);const auto loaded=service.load(out.data(),out.size());
    assert(loaded.ok()&&loaded.generation==generation&&loaded.document_version==version&&loaded.document_size==document.size());
    assert(std::equal(document.begin(),document.end(),out.begin()));
  }
};
std::map<std::string,std::string> records(const uint8_t *document,size_t size) {
  PanelConfigReader reader(document,size);assert(reader.begin()==PanelConfigStatus::OK);PanelConfigRecord row;
  std::map<std::string,std::string> result;
  while(reader.next(&row)==PanelConfigStatus::OK) {
    auto key=std::to_string(int(row.type))+":"+std::to_string(row.slot)+":";
    if(row.key_size)key.append(reinterpret_cast<const char *>(row.key),row.key_size);
    result[key]=std::string(reinterpret_cast<const char *>(row.value),row.value_size);
  }
  return result;
}
void profiles() {
  for(const auto &limits:DEVICE_UI_PROFILES)for(bool maximum:{false,true}) {
    Fixture fixture(limits);const auto source=native(limits,maximum);fixture.seed(source);const auto writes=fixture.backend.writes;
    Bytes restored(32768);const auto loaded=fixture.service.load_and_apply_runtime(restored.data(),restored.size());
    assert(loaded.ok()&&loaded.document_version==2&&loaded.generation==1);
    assert(fixture.backend.writes==writes&&fixture.runtime.active().source_version()==2&&fixture.runtime.active().generation()==1);
    assert(fixture.runtime.active().page(0)->runtime_card_count==limits.slot_count);
    assert(!fixture.runtime.active().runtime_card(31));
    assert(fixture.runtime.active().field(*fixture.runtime.active().card(31),CardField::LABEL)=="Dormant");
    assert(fixture.buttons[0].published>0&&fixture.buttons[0].persistent==0);
    PanelConfigRuntimeAdapter standalone(fixture.bindings,allocate,release);
    assert(standalone.apply(2,source.data(),source.size()));
    fixture.exact(source,1,2);
    assert(fixture.service.save(2,source.data(),source.size()).status==ServiceStatus::UNSUPPORTED_VERSION);
    assert(fixture.service.save_if_generation(1,2,source.data(),source.size()).status==ServiceStatus::UNSUPPORTED_VERSION);
    assert(fixture.backend.writes==writes);
    const auto unchanged=fixture.service.synchronize_legacy_runtime(restored.data(),restored.size());
    assert(unchanged.ok()&&unchanged.document_version==2&&unchanged.generation==1&&fixture.backend.writes==writes);
    assert(fixture.legacy.mirror(2,source.data(),source.size()));
    const auto shadow=fixture.service.refresh_legacy_shadow(restored.data(),restored.size());
    assert(shadow.status==ServiceStatus::UNSUPPORTED_VERSION&&fixture.backend.writes==writes);
    fixture.exact(source,1,2);
    PanelConfigRepresentation projected(allocate,release);
    assert(projected.convert(2,1,source.data(),source.size(),32768)==PanelConfigRepresentationStatus::OK);
    const auto original_records=records(projected.data(),projected.size());
    fixture.order.state="1w";
    const auto edited=fixture.service.synchronize_legacy_runtime(restored.data(),restored.size());
    assert(edited.ok()&&edited.document_version==1&&edited.generation==2&&fixture.runtime.active().source_version()==1);
    auto expected=original_records;expected["4:0:button_order"]="1w";
    assert(records(restored.data(),edited.document_size)==expected);
    const Bytes final(restored.begin(),restored.begin()+edited.document_size);fixture.exact(final,2,1);
    assert(allocations.size()==1); // Only this explicitly scoped projection.
  }
  assert(allocations.empty());
}
void unavailable_and_publication() {
  const auto &limits=DEVICE_UI_PROFILES[8];
  for(size_t fail=1;fail<=2;++fail) {
    Fixture fixture(limits);const auto source=native(limits,false);fixture.seed(source);const auto writes=fixture.backend.writes;
    Bytes restored(32768);fail_at=calls+fail;
    const auto loaded=fixture.service.load_and_apply_runtime(restored.data(),restored.size());fail_at=0;
    assert(loaded.status==ServiceStatus::ALLOCATION_FAILED&&fixture.backend.writes==writes&&fixture.runtime.active().source_size()==0&&allocations.empty());
    assert(fixture.service.load_and_apply_runtime(restored.data(),restored.size()).ok());fixture.exact(source,1,2);
  }
  // The real v2 runtime's scratch, staging and compact arena reservations
  // must report retryable memory pressure through the service, not bad data.
  for(size_t failure=1;failure<=3;++failure) {
    Fixture f(limits);const auto doc=native(limits,false);f.seed(doc);
    const auto writes=f.backend.writes;Bytes output(32768);
    f.arena.fail_at=f.arena.calls+failure;
    assert(f.service.load_and_apply_runtime(output.data(),output.size()).status==ServiceStatus::ALLOCATION_FAILED);
    assert(f.runtime.preparation_allocation_failed()&&f.runtime.active().source_size()==0&&f.backend.writes==writes&&allocations.empty());
    f.arena.fail_at=0;
    assert(f.service.load_and_apply_runtime(output.data(),output.size()).ok());
    assert(!f.runtime.preparation_allocation_failed()&&f.runtime.active().generation()==1&&f.backend.writes==writes);f.exact(doc,1,2);
  }
  Fixture fixture(limits);const auto source=native(limits,false);fixture.seed(source);Bytes restored(32768);
  fixture.order.fail=true;
  assert(fixture.service.load_and_apply_runtime(restored.data(),restored.size()).status==ServiceStatus::LEGACY_MIRROR_FAILED);
  assert(fixture.runtime.active().source_version()==2&&fixture.runtime.active().generation()==1);
  fixture.order.state="stale";
  assert(fixture.service.synchronize_legacy_runtime(restored.data(),restored.size()).status==ServiceStatus::LEGACY_MIRROR_FAILED);
  fixture.exact(source,1,2);fixture.order.fail=false;
  assert(fixture.service.load_and_apply_runtime(restored.data(),restored.size()).ok());
  fixture.order.state="1w";const auto writes=fixture.backend.writes;
  // Load document, validate its graph, obtain merge buffer, then fail the
  // compatibility projector's scratch allocation. No save has begun.
  fail_at=calls+4;
  assert(fixture.service.synchronize_legacy_runtime(restored.data(),restored.size()).status==ServiceStatus::ALLOCATION_FAILED);fail_at=0;
  assert(fixture.backend.writes==writes);fixture.exact(source,1,2);
  assert(fixture.service.synchronize_legacy_runtime(restored.data(),restored.size()).ok());
}
void imports_cannot_enable_v2_writes() {
  struct Import final:LegacyConfigurationAdapter {
    Bytes document;
    LegacyLoadResult load(uint8_t *out,size_t capacity)override {
      assert(capacity>=document.size());std::copy(document.begin(),document.end(),out);
      return {LegacyStatus::OK,2,document.size()};
    }
    bool mirror(uint16_t,const uint8_t *,size_t)override {assert(false);return false;}
  } legacy;
  legacy.document=native(DEVICE_UI_PROFILES[8],false);
  Backend backend;ConfigurationStore store(backend);PanelConfigDocumentValidator validator(allocate,release);
  ConfigurationService service(store,legacy,&validator);Bytes output(32768);
  assert(validator.supports_version(2)&&!validator.supports_write_version(2));
  assert(service.load(output.data(),output.size()).status==ServiceStatus::UNSUPPORTED_VERSION);
  assert(service.refresh_legacy_shadow(output.data(),output.size()).status==ServiceStatus::UNSUPPORTED_VERSION);
  assert(!backend.writes&&allocations.empty());
}
} // namespace
int main() {
  profiles();unavailable_and_publication();imports_cannot_enable_v2_writes();assert(allocations.empty());
  std::cout<<"PASS actual ECDO v2 boot, v1 write policy, all nine ordinary/maximum profiles, inactive/empty preservation, compatibility fencing and allocation failures; transient service/compatibility peak="<<peak<<" bytes (host, excludes runtime arenas)\n";
}
