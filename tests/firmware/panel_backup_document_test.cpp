#include <cassert>
#include <cstdlib>
#include <string>

#include "nested_page_fixture.h"
#include "panel_backup_document.h"

using namespace espcontrol;
using namespace espcontrol::configuration;
using namespace espcontrol::backup;
using namespace espcontrol::pages::test;

namespace {
size_t allocations = 0;
bool fail = false;
uint8_t* allocate(size_t bytes) {
  if (fail) return nullptr;
  auto* result = static_cast<uint8_t*>(std::malloc(bytes));
  assert(result);
  ++allocations;
  return result;
}
void release(uint8_t* bytes) {
  assert(allocations);
  --allocations;
  std::free(bytes);
}
Bytes v1(const DeviceUiLimits& limits, bool inactive = false) {
  Bytes result(1024);
  PanelConfigWriter writer(result.data(), result.size());
  assert(writer.begin() == PanelConfigStatus::OK);
  assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                       std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
  const std::string card = "light.room;Room;Auto;Auto;;;;;";
  assert(writer.append_button(1, reinterpret_cast<const uint8_t*>(card.data()), card.size()) == PanelConfigStatus::OK);
  if (inactive)
    assert(writer.append_button(32, reinterpret_cast<const uint8_t*>(card.data()), card.size()) == PanelConfigStatus::OK);
  size_t size = 0;
  assert(writer.finish(&size) == PanelConfigStatus::OK);
  result.resize(size);
  return result;
}
}

int main() {
  const auto& limits = DEVICE_UI_PROFILES[8];
  {
    const auto document = v1(limits, true);
    PanelBackupDocumentView view(document.data(), document.size(), allocate, release);
    assert(view.prepare(1, limits.slot_count, limits.device_profile) == PanelBackupDocumentStatus::OK);
    std::string result;
    auto sink = [&](const std::string& part) { result += part; return true; };
    assert(view.write("{}", "{}", "", "", sink));
    assert(!allocations);
  }
  for (uint16_t version : {1, 2}) {
    const auto document = version == 1 ? v1(limits) : wire(authored_nested(limits));
    std::string response;
    auto sink = [&](const std::string& part) { response += part; return true; };
    {
      PanelBackupDocumentView view(document.data(), document.size(), allocate, release);
      assert(!view.write("{}", "{}", "", "", sink) && response.empty());
      assert(view.prepare(3, limits.slot_count, limits.device_profile) == PanelBackupDocumentStatus::UNSUPPORTED_VERSION);
      assert(view.prepare(version, 0, limits.device_profile) == PanelBackupDocumentStatus::INVALID_DOCUMENT);
      assert(view.prepare(version, 33, limits.device_profile) == PanelBackupDocumentStatus::INVALID_DOCUMENT);
      assert(view.prepare(version, limits.slot_count, "wrong-device") == PanelBackupDocumentStatus::INVALID_DOCUMENT);
      assert(allocations == 0 && !view.write("{}", "{}", "", "", sink) && response.empty());
      fail = true;
      const auto failure = view.prepare(version, limits.slot_count, limits.device_profile);
      fail = false;
      if (version == 2) {
        assert(failure == PanelBackupDocumentStatus::ALLOCATION_FAILED);
        assert(!view.write("{}", "{}", "", "", sink) && response.empty() && !allocations);
      } else {
        assert(failure == PanelBackupDocumentStatus::OK && !allocations);
      }
      assert(view.prepare(version, limits.slot_count, limits.device_profile) == PanelBackupDocumentStatus::OK);
      assert(allocations == (version == 2 ? 1u : 0u));
      assert(view.write("{}", "{}", "", "", sink));
      assert(response.find(version == 2 ? "\"version\":3" : "\"version\":2") != std::string::npos);
      auto disconnected = [](const std::string&) { return false; };
      assert(!view.write("{}", "{}", "", "", disconnected));
      // Failed re-preparation releases its private index and cannot expose a
      // previously successful graph through a stale writer state.
      assert(view.prepare(version, limits.slot_count, "wrong-device") == PanelBackupDocumentStatus::INVALID_DOCUMENT);
      response.clear();
      assert(!allocations && !view.write("{}", "{}", "", "", sink) && response.empty());
    }
    assert(!allocations);
    auto invalid = document;
    invalid[0] = 'X';
    PanelBackupDocumentView bad(invalid.data(), invalid.size(), allocate, release);
    assert(bad.prepare(version, limits.slot_count, limits.device_profile) == PanelBackupDocumentStatus::INVALID_DOCUMENT);
    assert(!allocations);
  }
}
