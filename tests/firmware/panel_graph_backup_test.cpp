#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "nested_page_fixture.h"
#include "panel_graph_backup.h"

using namespace espcontrol;
using namespace espcontrol::configuration;
using namespace espcontrol::pages::test;

Bytes graph_backup_fixture(const DeviceUiLimits& limits) {
  auto records = authored_nested(limits);
  Row options; options.kind = K::STRING; options.id = 81;
  const std::string large(3500, 'x'); options.text = large;
  records.push_back(options);
  Row label; label.kind = K::STRING; label.id = 82;
  const std::string escaped_label(500, '\n'); label.text = escaped_label;
  records.push_back(label);
  records[3].field_mask |= (1 << 1) | (1 << 8);
  records[3].fields[1] = 82; records[3].fields[8] = 81;
  const auto original_folder = records[3], original_mid = records[4];
  const auto original_leaf = *std::find_if(records.begin(), records.end(),
      [](const Row& row) { return row.kind == K::CARD && row.id == 60010; });
  auto inactive = original_folder; inactive.id = 60001; inactive.second = 32;
  auto middle = original_mid; middle.id = 60002; middle.first = 50000;
  auto leaf = original_leaf; leaf.id = 60003; leaf.first = 50001;
  leaf.field_mask |= 1 << 8; leaf.fields[8] = 81;
  records.insert(records.end(), {inactive, middle, leaf});
  Row page; page.kind = K::PAGE; page.id = 50000; page.second = inactive.id; page.third = 72;
  Row nav; nav.kind = K::NAVIGATION; nav.id = inactive.id; nav.first = page.id;
  records.insert(records.end(), {page, nav});
  page.id = 50001; page.first = 50000; page.second = middle.id; page.third = 78;
  nav.id = middle.id; nav.first = page.id;
  records.insert(records.end(), {page, nav});
  return wire(std::move(records));
}

Bytes flat_backup_fixture(const DeviceUiLimits& limits) {
  const auto* profile = reinterpret_cast<const uint8_t*>(limits.device_profile);
  Bytes source(8192);
  PanelConfigWriter writer(source.data(), source.size());
  assert(writer.begin() == PanelConfigStatus::OK);
  assert(writer.append_device_profile(profile, std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
  const std::string root = "~light.kitchen,Kitchen%3B%20%22table%22%0A%C2%B0,Lightbulb,Auto,,,,,";
  const std::string children = "~B=Return%20Home:334455,1|S,,Room,Auto,Auto,sensor.room,%C2%B0C,1,";
  assert(writer.append_button(1, reinterpret_cast<const uint8_t*>(root.data()), root.size()) == PanelConfigStatus::OK);
  assert(writer.append_subpage(1, reinterpret_cast<const uint8_t*>(children.data()), children.size()) == PanelConfigStatus::OK);
  size_t size = 0;
  assert(writer.finish(&size) == PanelConfigStatus::OK);
  PanelConfigImportBounds bounds;
  assert(panel_config_v1_to_v2_bounds(source.data(), size, &bounds) == PanelConfigImportStatus::OK);
  Arena scratch((bounds.workspace_bytes + 7) / 8);
  Bytes result(bounds.output_bytes);
  assert(panel_config_v1_to_v2(source.data(), size, result.data(), result.size(), scratch.data(),
                              scratch.size() * 8, &size) == PanelConfigImportStatus::OK);
  result.resize(size);
  return result;
}

int main(int argc, char** argv) {
  const auto& selected = DEVICE_UI_PROFILES[8];
  const std::string argument = argc == 2 ? argv[1] : "";
  const bool flat = argument == "--flat-native" || argument == "--flat-fixture";
  const auto fixture = flat ? flat_backup_fixture(selected) : graph_backup_fixture(selected);
  if (argument == "--native" || argument == "--flat-native") {
    std::cout.write(reinterpret_cast<const char*>(fixture.data()), fixture.size());
    return 0;
  }
  for (const auto& limits : DEVICE_UI_PROFILES) {
    const auto document = flat ? flat_backup_fixture(limits) : graph_backup_fixture(limits);
    espcontrol::backup::PanelGraphBackupView view(document.data(), document.size());
    size_t needed = 0;
    assert(view.workspace_bytes(&needed) == PanelConfigStatus::OK && needed > 0);
    Arena scratch((needed + 7) / 8);
    std::string result;
    auto sink = [&](const std::string& part) { assert(part.size() <= 1024); result += part; return true; };
    assert(!view.write("{}", "{}", "", "", sink) && result.empty());
    assert(view.prepare(scratch.data(), needed - 1, limits.slot_count, limits.device_profile) == PanelConfigStatus::BUFFER_TOO_SMALL);
    assert(!view.write("{}", "{}", "", "", sink) && result.empty());
    assert(view.prepare(scratch.data(), needed, 0, limits.device_profile) == PanelConfigStatus::INVALID_ARGUMENT);
    assert(view.prepare(scratch.data(), needed, 33, limits.device_profile) == PanelConfigStatus::INVALID_ARGUMENT);
    assert(view.prepare(scratch.data(), needed, limits.slot_count, "other-panel") == PanelConfigStatus::INVALID_DOCUMENT);
    assert(!view.write("{}", "{}", "", "", sink) && result.empty());
    assert(view.prepare(scratch.data(), needed, limits.slot_count, limits.device_profile) == PanelConfigStatus::OK);
    const std::string settings = R"({"language":"de","clock_bar":false})";
    const std::string screen = R"({"brightness_day":42,"theme_mode":"Light"})";
    const std::string identity = R"({"version":1,"name":"Kitchen","hostname":"kitchen-ab1234","mac_suffix":"ab1234"})";
    assert(view.write(settings, screen, identity, "2026-11-02T12:00:00Z", sink));
    assert(result.find("\"version\":3") != std::string::npos);
    assert(result.find("\"document_version\":2") != std::string::npos);
    if (!flat) assert(result.find("\"type\":\"subpage\"") != std::string::npos);
    if ((argument == "--fixture" || argument == "--flat-fixture") && &limits == &selected) {
      std::cout << result;
      return 0;
    }
    // Every sink failure must stop immediately; callers must not terminate a
    // truncated HTTP response as if a complete backup had been delivered.
    size_t total = 0;
    auto counting = [&](const std::string&) { ++total; return true; };
    assert(view.write("{}", "{}", "", "", counting));
    for (size_t fail = 0; fail < total; ++fail) {
      size_t calls = 0;
      auto broken = [&](const std::string&) { return calls++ != fail; };
      assert(!view.write("{}", "{}", "", "", broken) && calls == fail + 1);
    }
    auto corrupted = document;
    corrupted[0] = 'X';
    espcontrol::backup::PanelGraphBackupView invalid(corrupted.data(), corrupted.size());
    result.clear();
    assert(invalid.workspace_bytes(&needed) == PanelConfigStatus::INVALID_DOCUMENT);
    assert(!invalid.write("{}", "{}", "", "", sink) && result.empty());
  }
  // Base64 chunks stay bounded for large native documents and all padding
  // lengths, without keeping their expanded representation in memory.
  for (const size_t length : {size_t{0}, size_t{1}, size_t{2}, size_t{3}, size_t{4097}}) {
    std::vector<uint8_t> bytes(length, 0xff);
    size_t encoded = 0;
    auto sink = [&](const std::string& part) { assert(part.size() <= 1024); encoded += part.size(); return true; };
    assert(espcontrol::backup::write_backup_base64(bytes.data(), bytes.size(), sink));
    assert(encoded == 4 * ((length + 2) / 3));
  }
}
