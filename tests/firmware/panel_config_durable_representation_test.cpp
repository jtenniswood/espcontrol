#include <cassert>
#include <cstdlib>
#include <map>
#include <vector>
#include "durable_format_fixture.h"
#include "nested_page_fixture.h"
#include "panel_config_durable_representation.h"

using namespace espcontrol::configuration;
using espcontrol::configuration::test::large_flat_document;
using namespace espcontrol::pages::test;
namespace {
using S = PanelConfigRepresentationStatus;
size_t calls = 0, fail_at = 0;
std::map<uint8_t *, size_t> live;
uint8_t *allocate(size_t size) {
  if (++calls == fail_at) return nullptr;
  auto *p = static_cast<uint8_t *>(std::malloc(size)); assert(p); live[p] = size; return p;
}
void release(uint8_t *p) { assert(live.erase(p) == 1); std::free(p); }
Bytes to_v2(const Bytes &v1) {
  PanelConfigRepresentation converted(allocate, release);
  assert(converted.convert(1, 2, v1.data(), v1.size(), PANEL_CONFIG_V2_HTTP_MAX_BYTES) == S::OK);
  return Bytes(converted.data(), converted.data() + converted.size());
}
std::vector<Row> rows(const Bytes &bytes) {
  PanelConfigV2Reader reader(bytes.data(), bytes.size()); size_t needed = 0;
  assert(reader.workspace_bytes(&needed) == PanelConfigStatus::OK);
  Arena scratch((needed + 7) / 8);
  assert(reader.validate(scratch.data(), scratch.size() * 8) == PanelConfigStatus::OK);
  std::vector<Row> result(reader.record_count());
  for (size_t i = 0; i < result.size(); ++i) assert(reader.record(i, &result[i]));
  return result;
}
Bytes encode(const std::vector<Row> &records) {
  Bytes bytes(128 * 1024); PanelConfigV2Writer writer(bytes.data(), bytes.size());
  assert(writer.begin() == PanelConfigStatus::OK);
  for (const auto &r : records) assert(writer.append(r) == PanelConfigStatus::OK);
  Arena scratch((writer.workspace_bytes() + 7) / 8); size_t size = 0;
  assert(writer.finish(scratch.data(), scratch.size() * 8, &size) == PanelConfigStatus::OK);
  bytes.resize(size); return bytes;
}
std::map<std::string, std::string> values(const Bytes &bytes) {
  PanelConfigReader reader(bytes.data(), bytes.size()); assert(reader.validate() == PanelConfigStatus::OK);
  assert(reader.begin() == PanelConfigStatus::OK); PanelConfigRecord r;
  std::map<std::string, std::string> result;
  while (reader.next(&r) == PanelConfigStatus::OK)
    result[std::to_string(int(r.type)) + ':' + std::to_string(r.slot) + ':' +
        std::string(reinterpret_cast<const char *>(r.key), r.key_size)] =
        std::string(reinterpret_cast<const char *>(r.value), r.value_size);
  return result;
}
void fallback(size_t roots, size_t capacity) {
  const auto original = large_flat_document(roots), v2 = to_v2(original);
  assert(original.size() <= capacity && v2.size() > capacity);
  for (uint16_t source : {1, 2}) {
    const auto &input = source == 1 ? original : v2;
    PanelConfigDurableRepresentation selected(allocate, release);
    assert(selected.select(source, 2, input.data(), input.size(), capacity) == S::OK);
    assert(selected.version() == 1 && selected.size() <= capacity);
    const Bytes result(selected.data(), selected.data() + selected.size());
    assert(values(result) == values(original));
    if (source == 1) assert(result == original);  // Exact native-v1 backup/import bytes.
    assert(selected.select(1, 2, selected.data(), selected.size(), capacity) == S::INVALID_DOCUMENT);
  }
  assert(live.empty());
}
void identity_and_provenance() {
  const auto original = large_flat_document(), v2 = to_v2(original);
  constexpr size_t capacity = 40932;
  auto records = rows(v2);
  // Record order and dictionary numbers do not change graph identities.
  for (auto &r : records) {
    auto shift = [](uint16_t &id) { if (id) id += 2000; };
    if (r.kind == K::STRING) shift(r.id);
    else if (r.kind == K::ENTITY || r.kind == K::SETTING) { shift(r.id); shift(r.first); }
    else if (r.kind == K::PAGE) shift(r.third);
    else if (r.kind == K::CARD) { for (auto &id : r.fields) shift(id); shift(r.raw_source); }
  }
  std::reverse(records.begin(), records.end());
  const auto reordered = encode(records);
  {
    PanelConfigDurableRepresentation selected(allocate, release);
    assert(selected.select(2, 2, reordered.data(), reordered.size(), capacity) == S::OK);
    assert(selected.version() == 1);
    assert(values(Bytes(selected.data(), selected.data() + selected.size())) == values(original));
  }
  for (unsigned mutation = 0; mutation < 3; ++mutation) {
    records = rows(v2);
    if (mutation == 0) {
      auto card = std::find_if(records.begin(), records.end(), [](const Row &r) { return r.kind == K::CARD && r.first; });
      assert(card != records.end()); card->id = 60000;
    } else if (mutation == 1) {
      auto card = std::find_if(records.begin(), records.end(), [](const Row &r) { return r.kind == K::CARD && r.first; });
      card->flags = 0; card->raw_source = 0;
    } else {
      Row extra; extra.kind = K::STRING; extra.id = 60000; extra.text = "unreferenced retained text";
      records.push_back(extra);
    }
    const auto changed = encode(records);
    PanelConfigDurableRepresentation selected(allocate, release);
    assert(selected.select(2, 2, changed.data(), changed.size(), capacity) == S::TOO_LARGE);
    assert(!selected.data() && selected.size() == 0 && selected.version() == 0);
  }
  assert(live.empty());
}
void failures_and_preference() {
  const auto original = large_flat_document(), v2 = to_v2(original);
  const auto preserved = v2;
  size_t allocation_count = 0;
  {
    PanelConfigDurableRepresentation selected(allocate, release); calls = 0;
    assert(selected.select(2, 2, v2.data(), v2.size(), 40932) == S::OK); allocation_count = calls;
  }
  assert(allocation_count > 3 && live.empty());
  for (size_t n = 1; n <= allocation_count; ++n) {
    PanelConfigDurableRepresentation selected(allocate, release); calls = 0; fail_at = n;
    assert(selected.select(2, 2, v2.data(), v2.size(), 40932) == S::UNAVAILABLE); fail_at = 0;
    assert(!selected.data() && v2 == preserved);
  }
  {
    PanelConfigDurableRepresentation selected(allocate, release); calls = 0;
    assert(selected.select(2, 2, v2.data(), v2.size(), v2.size()) == S::OK);
    assert(selected.version() == 2 && selected.data() == v2.data() && calls == 0);
  }
  {
    PanelConfigDurableRepresentation selected(allocate, release);
    assert(selected.select(2, 1, v2.data(), v2.size(), 40932) == S::OK && selected.version() == 1);
  }
  {
    const auto nested = wire(authored_nested(espcontrol::DEVICE_UI_PROFILES[8]));
    PanelConfigDurableRepresentation selected(allocate, release);
    assert(selected.select(2, 2, nested.data(), nested.size(), nested.size() - 1) == S::TOO_LARGE);
    assert(!selected.data());
  }
  assert(live.empty());
}
}  // namespace
int main() {
  fallback(20, 40932); fallback(12, 16356);
  identity_and_provenance(); failures_and_preference(); assert(live.empty());
}
