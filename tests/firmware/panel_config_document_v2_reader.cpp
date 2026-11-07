#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include "panel_config_document_v2.h"
using namespace espcontrol::configuration;
static bool forbid_allocation = false;
void *operator new(size_t n) {
  if (forbid_allocation) std::abort();
  if (void *p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
void *operator new[](size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, size_t) noexcept { std::free(p); }
void operator delete[](void *p, size_t) noexcept { std::free(p); }
struct NoAllocation {
  NoAllocation() { assert(!forbid_allocation); forbid_allocation = true; }
  ~NoAllocation() { forbid_allocation = false; }
};

int main(int argc, char **argv) {
  std::vector<uint8_t> input(std::istreambuf_iterator<char>(std::cin), {});
  PanelConfigV2Reader reader(input.data(), input.size());
  size_t bytes = 0;
  PanelConfigStatus status;
  { NoAllocation guard; status = reader.workspace_bytes(&bytes); }
  std::vector<uint64_t> storage((bytes + 7) / 8 + 1);
  if (status == PanelConfigStatus::OK) {
    void *workspace = storage.data();
    size_t capacity = bytes;
    const std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "short") --capacity;
    if (mode == "unaligned") workspace = reinterpret_cast<uint8_t *>(workspace) + 1;
    if (mode == "overlap") workspace = input.data();
    NoAllocation guard;
    status = reader.validate(workspace, capacity);
  }
  const std::string mode = argc > 1 ? argv[1] : "";
  if (status == PanelConfigStatus::OK && mode.rfind("writer-", 0) == 0) {
    std::vector<uint8_t> output(mode == "writer-small-output" ? 16 : input.size() + 128);
    std::vector<uint64_t> scratch((bytes + 8) / 8 + 1);
    PanelConfigV2Writer writer(output.data(), output.size());
    auto written = writer.begin();
    for (size_t n = 0; n < reader.record_count() && written == PanelConfigStatus::OK; ++n) {
      PanelConfigV2Record record;
      assert(reader.record(n, &record));
      if (mode == "writer-dangling" && record.kind == PanelConfigV2Kind::ENTITY) record.first = 600;
      if (mode == "writer-field-mask" && record.kind == PanelConfigV2Kind::CARD) record.fields[8] = 123;
      if (mode == "writer-hidden-raw" && record.kind == PanelConfigV2Kind::CARD && !record.flags) record.raw_source = 12;
      if (mode == "writer-reserved-field" && record.kind == PanelConfigV2Kind::STRING) record.second = 12;
      { NoAllocation guard; written = writer.append(record); }
      if (mode == "writer-duplicate" && record.kind == PanelConfigV2Kind::STRING && record.id == 1 && written == PanelConfigStatus::OK)
        { NoAllocation guard; written = writer.append(record); }
    }
    size_t size = 12345;
    if (written == PanelConfigStatus::OK) {
      NoAllocation guard;
      written = writer.finish(scratch.data(), mode == "writer-short-index" ? 0 : scratch.size() * 8, &size);
      if (written != PanelConfigStatus::OK) assert(size == 0);
    }
    if (written != PanelConfigStatus::OK) {
      // Even an ignored failure cannot expose a valid document marker.
      assert(output[0] != 'E');
      std::cout << unsigned(written) << '\n';
      return 2;
    }
    if (mode != "writer-roundtrip") return 3;
    std::cout.write(reinterpret_cast<const char *>(output.data()), size);
    return 0;
  }
  std::cout << unsigned(status) << ' ' << bytes << '\n';
  if (status != PanelConfigStatus::OK) {
    PanelConfigV2Record record;
    assert(reader.record_count() == 0 && !reader.record(PanelConfigV2Kind::PAGE, 0, &record));
    assert(reader.string(1).empty() && reader.entity(1).empty());
    return 2;
  }
  assert(!reader.record(PanelConfigV2Kind::PAGE, 0, nullptr));
  assert(reader.workspace_bytes(nullptr) == PanelConfigStatus::INVALID_ARGUMENT);
  for (size_t n = 0; n < reader.record_count(); ++n) {
    PanelConfigV2Record r;
    assert(reader.record(n, &r));
    std::cout << unsigned(r.kind) << ' ' << r.id << ' ' << r.first << ' '
              << r.second << ' ' << r.third << ' ' << r.field_mask << ' '
              << unsigned(r.flags) << ' ' << r.raw_source;
    for (auto id : r.fields) std::cout << ' ' << id;
    std::cout << ' ';
    static constexpr char hex[] = "0123456789abcdef";
    for (unsigned char c : r.text) std::cout << hex[c >> 4] << hex[c & 15];
    std::cout << '\n';
    PanelConfigV2Record found;
    assert(reader.record(r.kind, r.id, &found) && found.id == r.id);
    if (r.kind == PanelConfigV2Kind::STRING) assert(reader.string(r.id) == r.text);
    if (r.kind == PanelConfigV2Kind::ENTITY)
      assert(!reader.entity(r.id).empty() && reader.entity(r.id) == reader.string(r.first));
  }
  if (argc > 1 && std::string(argv[1]) == "revalidate") {
    input[14] = 1;
    assert(reader.validate(storage.data(), bytes) == PanelConfigStatus::INVALID_DOCUMENT);
    PanelConfigV2Record r;
    assert(!reader.record(PanelConfigV2Kind::PAGE, 0, &r) && reader.record_count() == 0);
  }
  assert(reader.string(0).empty() && reader.entity(0).empty());
}
