#pragma once

#include <algorithm>
#include <cassert>
#include <vector>
#include "device_ui_limits.h"
#include "panel_config_document_v2.h"
#include "page_store.h"

namespace espcontrol::pages::test {
using namespace espcontrol::configuration;
using K = PanelConfigV2Kind;
using Row = PanelConfigV2Record;
using Bytes = std::vector<uint8_t>;
using Arena = std::vector<uint64_t>;

inline Bytes wire(std::vector<Row> records) {
  Bytes result(32 * 1024); PanelConfigV2Writer writer(result.data(), result.size());
  assert(writer.begin() == PanelConfigStatus::OK);
  // Wire order and IDs differ deliberately from runtime order and indices.
  std::reverse(records.begin(), records.end());
  for (const auto &record : records) assert(writer.append(record) == PanelConfigStatus::OK);
  Arena scratch((writer.workspace_bytes() + 7) / 8); size_t size = 0;
  assert(writer.finish(scratch.data(), scratch.size() * 8, &size) == PanelConfigStatus::OK);
  result.resize(size); return result;
}
inline std::vector<Row> authored(const DeviceUiLimits &limits) {
  Row profile; profile.kind = K::DEVICE_PROFILE; profile.text = limits.device_profile;
  Row root; root.kind = K::PAGE; root.first = root.second = UINT16_MAX; root.third = 71;
  Row page; page.kind = K::PAGE; page.id = 63000; page.second = 61000; page.third = 72;
  Row folder; folder.kind = K::CARD; folder.id = 61000; folder.second = 2;
  folder.field_mask = 1 << 6; folder.fields[6] = 73;
  Row child; child.kind = K::CARD; child.id = 62000; child.first = page.id; child.second = 1;
  child.field_mask = (1 << 0) | (1 << 1) | (1 << 6); child.fields[0] = 65000; child.fields[1] = 74; child.fields[6] = 75;
  Row navigation; navigation.kind = K::NAVIGATION; navigation.id = folder.id; navigation.first = page.id;
  Row entity; entity.kind = K::ENTITY; entity.id = 65000; entity.first = 76;
  std::vector<Row> records{profile, root, page, folder, child, navigation, entity};
  for (auto value : std::array<std::pair<uint16_t, const char *>, 6>{{{71,"2"},{72,"B,1w"},{73,"subpage"},{74,"温度 🌡️"},{75,"sensor"},{76,"sensor.temperature"}}}) {
    Row text; text.kind = K::STRING; text.id = value.first; text.text = value.second; records.push_back(text);
  }
  return records;
}
inline std::vector<Row> authored_nested(const DeviceUiLimits &limits) {
  auto records=authored(limits);
  auto leaf=records[4];leaf.id=60010;leaf.first=1;leaf.fields[1]=77;
  records[4].fields[6]=73; // The first-level child now owns the nested page.
  Row page;page.kind=K::PAGE;page.id=1;page.first=63000;page.second=62000;page.third=78;
  Row navigation;navigation.kind=K::NAVIGATION;navigation.id=62000;navigation.first=1;
  Row label;label.kind=K::STRING;label.id=77;label.text="Nested Leaf";
  Row order;order.kind=K::STRING;order.id=78;order.text="B=Back%20Up:112233,1";
  records.insert(records.end(),{leaf,page,navigation,label,order});
  return records;
}
// More descendant controls than visible shells, plus optional explicit state
// on the intermediate folder. The final leaf has its own HA channel.
inline std::vector<Row> summary_nested(const DeviceUiLimits &limits, bool explicit_nested = false) {
  auto records = authored_nested(limits);
  records[3].field_mask |= (1 << 1) | (1 << 4);
  records[3].fields[1] = 81; records[3].fields[4] = 65003;
  records[4].field_mask |= (1 << 1) | (1 << 4);
  records[4].fields[0] = explicit_nested ? 65001 : 0;
  records[4].fields[1] = 82; records[4].fields[4] = 65003;
  if (!explicit_nested) records[4].field_mask &= ~(1 << 0);
  for (auto &row : records) {
    if (row.kind == K::STRING && row.id == 75) row.text = "light_switch";
    if (row.kind == K::STRING && row.id == 76) row.text = "light.leaf";
    if (row.kind == K::STRING && row.id == 77) row.text = "Leaf";
  }
  for (const auto &value : std::array<std::pair<uint16_t,const char *>,4>{{
      {79,"indicator"},{80,"light.explicit"},{81,"Root"},{82,"Child"}}}) {
    Row row; row.kind=K::STRING; row.id=value.first; row.text=value.second; records.push_back(row);
  }
  Row entity; entity.kind=K::ENTITY; entity.id=65001; entity.first=80; records.push_back(entity);
  entity.id=65003;entity.first=79;records.push_back(entity);
  const auto leaf=*std::find_if(records.begin(),records.end(),[](const Row &row){return row.kind==K::CARD && row.id==60010;});
  for (size_t slot=2; slot<=limits.cards_per_subpage; ++slot) {
    auto nested=leaf; nested.id=60010+slot; nested.second=slot; records.push_back(nested);
    auto direct=leaf; direct.id=62000+slot; direct.first=63000; direct.second=slot; records.push_back(direct);
  }
  Row last_text;last_text.kind=K::STRING;last_text.id=83;last_text.text="light.last";
  Row last_entity;last_entity.kind=K::ENTITY;last_entity.id=65002;last_entity.first=83;
  records.push_back(last_text);records.push_back(last_entity);
  for(auto &row:records)if(row.kind==K::CARD && row.first==1 && row.second==limits.cards_per_subpage)
    row.fields[0]=65002;
  for(auto &row:records)if(row.kind==K::STRING && (row.id==72 || row.id==78))
    row.text="B,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19";
  return records;
}
struct NativeGraphFixture {
  NativeGraphFixture(const DeviceUiLimits &limits, std::vector<Row> records, uint32_t generation=7)
      : document(wire(std::move(records))), arena(128*1024/8) {
    size_t needed=0;
    assert(PageStore::v2_workspace_bytes(document.data(),document.size(),&needed)==PageStoreStatus::OK);
    Arena scratch((needed+7)/8);
    assert(PageStore::stage_v2(document.data(),document.size(),limits,generation,arena.data(),arena.size()*8,
      scratch.data(),needed,&store)==PageStoreStatus::OK);
  }
  Bytes document; Arena arena; PageStore store;
};
}  // namespace espcontrol::pages::test
