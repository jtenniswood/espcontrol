// Diagnostic benchmark; no timing thresholds, and no device-memory claim.
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "page_entity_state_runtime.h"
#include "panel_config_document.h"

using namespace espcontrol;using namespace espcontrol::pages;using namespace espcontrol::configuration;
namespace {
constexpr size_t ATTRIBUTES=24,QUERIES=20000,UPDATES=2000;
struct Arena:PageArenaAllocator {void *allocate(size_t size)override{return std::malloc(size);}void release(void *p,size_t)override{std::free(p);}};
struct Source:PageDependencySource {
  bool enumerate(const PageStore &store,PageDependencySink &sink)const override {
    for(CardId card=0;card<store.card_count();++card){const auto entity="climate.synthetic_"+std::to_string(card);for(size_t attr=0;attr<ATTRIBUTES;++attr)if(!sink.add(card,entity,"field_"+std::to_string(attr)))return false;}return true;
  }
};
struct Subscriptions:PageEntitySubscriptions {
  struct Entry{void *owner;uint32_t revision,channel;Deliver deliver;};std::vector<Entry>entries;
  bool subscribe(const char *,const char *,void *owner,uint32_t revision,uint32_t channel,Deliver deliver)override{entries.push_back({owner,revision,channel,deliver});return true;}
  void release(void *owner)override{entries.erase(std::remove_if(entries.begin(),entries.end(),[owner](const auto &e){return e.owner==owner;}),entries.end());}
  uint32_t connection_generation()const override{return 1;}
};
void run(const DeviceUiLimits &limits) {
  std::vector<uint8_t>document(65536),storage(131072);PageStore store;size_t size=0;
  PanelConfigWriter writer(document.data(),document.size());assert(writer.begin()==PanelConfigStatus::OK);
  assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(limits.device_profile),std::strlen(limits.device_profile))==PanelConfigStatus::OK);
  for(uint8_t root=1;root<=limits.slot_count;++root) {
    const std::string parent=";Room;Home;Auto;;;subpage;;";
    assert(writer.append_button(root,reinterpret_cast<const uint8_t *>(parent.data()),parent.size())==PanelConfigStatus::OK);
    std::string children="~B";
    for(size_t child=1;child<limits.slot_count;++child)children+=","+std::to_string(child);
    for(size_t child=1;child<limits.slot_count;++child)children+="|S,,Value,Auto,Auto,sensor.synthetic_"+std::to_string(root)+"_"+std::to_string(child)+",,1,";
    assert(writer.append_subpage(root,reinterpret_cast<const uint8_t *>(children.data()),children.size())==PanelConfigStatus::OK);
  }
  assert(writer.finish(&size)==PanelConfigStatus::OK);
  assert(PageStore::stage_v1(document.data(),size,limits,17,storage.data(),storage.size(),&store)==PageStoreStatus::OK);
  Arena arena;Source source;Subscriptions subscriptions;const auto count=store.card_count()*ATTRIBUTES;subscriptions.entries.reserve(count);
  PageEntityStateRuntime state(source,arena,subscriptions,{count,count,count*4});assert(state.prepare(store));state.activate_prepared(17);
  for(const auto &entry:subscriptions.entries)entry.deliver(entry.owner,entry.revision,entry.channel,"10.0");
  const ConfigurationHandle last{17,static_cast<CardId>(store.card_count()-1)};volatile size_t checksum=0;
  const auto query_start=std::chrono::steady_clock::now();
  for(size_t i=0;i<QUERIES;++i){checksum+=state.dependency_count(last);checksum+=state.dependency(last,ATTRIBUTES-1).value.size();}
  const auto query_ns=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-query_start).count();
  const auto update_start=std::chrono::steady_clock::now();const auto &first=subscriptions.entries[0];
  for(size_t i=0;i<UPDATES;++i)first.deliver(first.owner,first.revision,first.channel,i%2?"10.0":"11.0");
  const auto update_ns=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-update_start).count();
  assert(checksum==QUERIES*(ATTRIBUTES+4));assert(state.memory().active_state_bytes==count*4);
  assert(state.dependency(last,ATTRIBUTES-1).value=="10.0");const auto memory=state.memory();
  std::printf("{\"profile\":\"%s\",\"cards\":%zu,\"edges\":%zu,\"fixed_bytes\":%zu,\"arena_bytes\":%zu,\"queries\":%zu,\"query_ns\":%lld,\"updates\":%zu,\"update_ns\":%lld}\n",limits.device_profile,store.card_count(),count,sizeof(state),memory.active_arena_bytes,QUERIES,static_cast<long long>(query_ns),UPDATES,static_cast<long long>(update_ns));
}
}
int main(){for(const auto &limits:DEVICE_UI_PROFILES)if(std::strcmp(limits.device_profile,"guition-esp32-s3-4848s040")==0||std::strcmp(limits.device_profile,"guition-esp32-p4-jc8012p4a1-v3")==0)run(limits);}
