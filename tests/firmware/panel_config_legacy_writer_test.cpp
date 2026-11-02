#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "panel_config_legacy_adapter.h"
#include "panel_config_service_validator.h"
#include "panel_config_compatibility_view.h"
#include "nested_page_fixture.h"

using namespace espcontrol;
using namespace espcontrol::configuration;
using namespace espcontrol::pages::test;

namespace {
size_t calls = 0, fail_at = 0;
std::map<uint8_t*, size_t> live;
uint8_t* allocate(size_t size) {
  if (++calls == fail_at) return nullptr;
  auto* p = static_cast<uint8_t*>(std::malloc(size));
  assert(p);
  live[p] = size;
  return p;
}
void release(uint8_t* p) {
  assert(live.erase(p) == 1);
  std::free(p);
}
struct Backend : StorageBackend {
  std::array<Bytes, 2> slots{Bytes(32768, 255), Bytes(32768, 255)};
  size_t writes = 0;
  bool fail = false;
  size_t slot_capacity() const override { return slots[0].size(); }
  bool read(uint8_t slot, size_t offset, uint8_t* data, size_t size) override {
    if (slot >= 2 || offset > slots[slot].size() || size > slots[slot].size() - offset) return false;
    std::memcpy(data, slots[slot].data() + offset, size);
    return true;
  }
  bool write(uint8_t slot, size_t offset, const uint8_t* data, size_t size) override {
    if (fail || slot >= 2 || offset > slots[slot].size() || size > slots[slot].size() - offset) return false;
    std::memcpy(slots[slot].data() + offset, data, size);
    ++writes;
    return true;
  }
  bool sync() override { return true; }
};
struct Text : PanelConfigTextValue {
  std::string state;
  size_t writes = 0;
  bool fail_publish = false;
  explicit Text(std::string value) : state(std::move(value)) {}
  const std::string& value() const override { return state; }
  bool set_value(const char* value, size_t size) override { state.assign(value, size); ++writes; return true; }
  bool publish_value(const char* value, size_t size) override {
    if (fail_publish) return false;
    state.assign(value, size); return true;
  }
};
struct Runtime : ConfigurationRuntimeAdapter {
  size_t activations = 0;
  uint16_t version = 0;
  Bytes active;
  bool apply(uint16_t format, const uint8_t* data, size_t size) override {
    version = format; active.assign(data, data + size); ++activations; return true;
  }
};
// Exercise the next policy through the real validator, without enabling it in
// the production release contract or weakening replacement admission.
struct FutureWriter : ConfigurationDocumentValidator {
  PanelConfigDocumentValidator content{allocate, release};
  bool supports_version(uint16_t v) const override { return content.supports_version(v); }
  uint16_t write_version() const override { return 2; }
  bool supports_write_version(uint16_t v) const override { return v == 2; }
  bool checks_replacements(uint16_t v) const override { return content.checks_replacements(v); }
  bool replacement_needs_document(uint16_t current, uint16_t incoming) const override { return content.replacement_needs_document(current, incoming); }
  ServiceStatus replacement_status(uint16_t current, const uint8_t* data, size_t size, uint16_t incoming) const override { return content.replacement_status(current, data, size, incoming); }
  bool validate(uint16_t v, const uint8_t* data, size_t size) const override { return content.validate(v, data, size); }
  ServiceStatus validation_status(uint16_t v, const uint8_t* data, size_t size) const override { return content.validation_status(v, data, size); }
};
struct Fixture {
  Backend backend;
  ConfigurationStore store{backend};
  Text order{"1"}, color{"123456"}, button{"light.room;Room;Auto;Auto;;;;;"};
  PanelConfigTextBindings bindings;
  PanelConfigLegacyAdapter legacy{bindings, allocate, release};
  Runtime runtime;
  PanelConfigDocumentValidator stable{allocate, release};
  FutureWriter future;
  ConfigurationService service;
  explicit Fixture(const DeviceUiLimits& limits, bool next = true)
      : service(store, legacy, next ? static_cast<const ConfigurationDocumentValidator*>(&future) : &stable,
                nullptr, 0, next ? LegacyConfigurationMode::READ_IMPORT_ONLY : LegacyConfigurationMode::DUAL_WRITE) {
    bindings.set_device_profile(limits.device_profile);
    bindings.set_button_order(&order);
    bindings.set_button_on_color(&color);
    assert(bindings.set_button(1, &button, {}));
    service.set_buffer_allocator(allocate, release);
    service.set_runtime_adapter(&runtime);
  }
};
Bytes read(Fixture& f, ServiceLoadResult* result = nullptr) {
  Bytes bytes(32768);
  const auto loaded = f.service.load(bytes.data(), bytes.size());
  assert(loaded.ok());
  if (result) *result = loaded;
  bytes.resize(loaded.document_size);
  return bytes;
}
std::map<std::string, std::string> records(const Bytes& bytes, uint16_t version) {
  PanelConfigCompatibilityView view(allocate, release);
  assert(view.load(version, bytes.data(), bytes.size()) == PanelConfigRepresentationStatus::OK);
  PanelConfigReader reader(view.data(), view.size());
  assert(reader.validate() == PanelConfigStatus::OK);
  assert(reader.begin() == PanelConfigStatus::OK);
  PanelConfigRecord row;
  std::map<std::string, std::string> values;
  while (reader.next(&row) == PanelConfigStatus::OK) {
    const auto key = std::to_string(static_cast<int>(row.type)) + ":" + std::to_string(row.slot) + ":" +
        (row.key_size ? std::string(reinterpret_cast<const char*>(row.key), row.key_size) : "");
    values[key] = std::string(reinterpret_cast<const char*>(row.value), row.value_size);
  }
  return values;
}
void bootstrap(const DeviceUiLimits& limits, bool next) {
  Fixture f(limits, next);
  Bytes bytes(32768);
  const auto boot = f.service.load_and_apply_runtime(bytes.data(), bytes.size());
  if (!boot.ok()) std::fprintf(stderr, "bootstrap format %u status %u\n", next ? 2u : 1u, static_cast<unsigned>(boot.status));
  assert(boot.imported_legacy());
  assert(boot.document_version == (next ? 2 : 1));
  assert(f.runtime.activations == 1 && f.runtime.version == boot.document_version);
  assert(f.color.writes == 0 && f.order.writes == 0 && f.button.writes == 0);
  const auto saved = read(f);
  assert(saved == f.runtime.active);
  const auto writes = f.backend.writes;
  const auto unchanged = f.service.synchronize_legacy_runtime(bytes.data(), bytes.size());
  assert(unchanged.ok() && unchanged.generation == boot.generation);
  assert(f.backend.writes == writes && f.runtime.activations == 1 && read(f) == saved);
  f.color.state = "ABCDEF";
  const auto edited = f.service.synchronize_legacy_runtime(bytes.data(), bytes.size());
  assert(edited.ok() && edited.document_version == boot.document_version && edited.generation == boot.generation + 1);
  assert(f.runtime.activations == 2);
  auto expected = records(saved, boot.document_version);
  expected[std::to_string(static_cast<int>(PanelConfigRecordType::SETTING)) + ":0:button_on_color"] = "ABCDEF";
  assert(records(read(f), boot.document_version) == expected);
  if (next) assert(f.color.writes == 0 && f.order.writes == 0 && f.button.writes == 0);
  assert(live.empty());
}

Bytes native_flat(const DeviceUiLimits& limits) {
  Bytes bytes(8192);
  PanelConfigWriter writer(bytes.data(), bytes.size());
  assert(writer.begin() == PanelConfigStatus::OK);
  assert(writer.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile), std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
  auto setting = [&](const char* key, const char* value) {
    assert(writer.append_setting(reinterpret_cast<const uint8_t*>(key), std::strlen(key), reinterpret_cast<const uint8_t*>(value), std::strlen(value)) == PanelConfigStatus::OK);
  };
  setting("button_order", "1");
  setting("button_on_color", "123456");
  setting("future_setting", "opaque;value:with,delimiters");
  for (uint8_t slot : {1, 32}) {
    const std::string card = slot == 1 ? "light.room;Room;Auto;Auto;;;;;" : ";Saved;Home;Auto;;;subpage;;";
    assert(writer.append_button(slot, reinterpret_cast<const uint8_t*>(card.data()), card.size()) == PanelConfigStatus::OK);
  }
  const std::string child = "~B=Parent:112233;S|sensor.inactive|Saved|Auto|Auto||||opaque";
  assert(writer.append_subpage(32, reinterpret_cast<const uint8_t*>(child.data()), child.size()) == PanelConfigStatus::OK);
  size_t size = 0;
  assert(writer.finish(&size) == PanelConfigStatus::OK);
  bytes.resize(size);
  PanelConfigRepresentation converted(allocate, release);
  assert(converted.convert(1, 2, bytes.data(), bytes.size(), 32768) == PanelConfigRepresentationStatus::OK);
  return Bytes(converted.data(), converted.data() + converted.size());
}

void preserve_flat_native(const DeviceUiLimits& limits) {
  Fixture f(limits);
  const auto original = native_flat(limits);
  const auto saved = f.service.save(2, original.data(), original.size());
  assert(saved.ok());
  Bytes output(32768);
  const auto writes = f.backend.writes;
  const auto activations = f.runtime.activations;
  auto synced = f.service.synchronize_legacy_runtime(output.data(), output.size());
  assert(synced.ok() && synced.generation == saved.generation);
  assert(read(f) == original && f.backend.writes == writes && f.runtime.activations == activations);
  f.color.state = "AA5500";
  f.backend.fail = true;
  synced = f.service.synchronize_legacy_runtime(output.data(), output.size());
  assert(synced.status == ServiceStatus::STORE_FAILED && read(f) == original);
  assert(f.runtime.activations == activations && f.runtime.active == original);
  f.backend.fail = false;
  synced = f.service.synchronize_legacy_runtime(output.data(), output.size());
  assert(synced.ok() && synced.document_version == 2 && synced.generation == saved.generation + 1);
  auto expected = records(original, 2);
  expected[std::to_string(static_cast<int>(PanelConfigRecordType::SETTING)) + ":0:button_on_color"] = "AA5500";
  const auto edited = read(f);
  assert(records(edited, 2) == expected);
  assert(f.runtime.active == edited);
  assert(f.order.writes == 0 && f.color.writes == 0 && f.button.writes == 0);
  // A fresh service instance reads the native snapshot; it must not import
  // stale preferences or rewrite native IDs/order during boot.
  f.order.state = "stale"; f.color.state = "stale"; f.button.state = "stale";
  ConfigurationService reboot(f.store, f.legacy, &f.future, nullptr, 0, LegacyConfigurationMode::READ_IMPORT_ONLY);
  reboot.set_buffer_allocator(allocate, release);
  reboot.set_runtime_adapter(&f.runtime);
  const auto writes_after = f.backend.writes;
  const auto boot = reboot.load_and_apply_runtime(output.data(), output.size());
  assert(boot.ok() && boot.document_version == 2 && boot.generation == synced.generation);
  assert(read(f) == edited && f.runtime.active == edited && f.backend.writes == writes_after);
  assert(f.color.state == "AA5500" && f.order.state == "1");
  assert(live.empty());
}

void unchanged_arbitrary_ids(const DeviceUiLimits& limits) {
  Fixture f(limits);
  const auto original = wire(authored(limits));
  assert(f.service.save(2, original.data(), original.size()).ok());
  const auto writes = f.backend.writes;
  const auto activations = f.runtime.activations;
  Bytes output(32768);
  assert(f.service.synchronize_legacy_runtime(output.data(), output.size()).ok());
  assert(read(f) == original && f.backend.writes == writes && f.runtime.activations == activations);
  assert(live.empty());
}

void allocation_failures(const DeviceUiLimits& limits) {
  Fixture f(limits);
  Bytes output(32768);
  calls = 0;
  const auto loaded = f.legacy.load_for_write(2, output.data(), output.size());
  assert(loaded.status == LegacyStatus::OK && loaded.document_version == 2 && live.empty());
  const auto allocation_count = calls;
  assert(allocation_count >= 2);
  for (size_t failure = 1; failure <= allocation_count; ++failure) {
    calls = 0; fail_at = failure;
    const auto failed = f.legacy.load_for_write(2, output.data(), output.size());
    fail_at = 0;
    assert(failed.status == LegacyStatus::ALLOCATION_FAILED && live.empty());
    assert(f.backend.writes == 0 && f.runtime.activations == 0 && f.color.writes == 0);
    assert(f.legacy.load_for_write(2, output.data(), output.size()).status == LegacyStatus::OK);
    assert(live.empty());
  }
  // The v1 writer keeps the allocation-free adapter import path.
  calls = 0;
  assert(f.legacy.load_for_write(1, output.data(), output.size()).status == LegacyStatus::OK && calls == 0);
  const auto legacy = f.legacy.load(output.data(), output.size());
  assert(legacy.status == LegacyStatus::OK);
  const auto too_small = f.legacy.load_for_write(2, output.data(), legacy.document_size);
  assert(too_small.status == LegacyStatus::BUFFER_TOO_SMALL && live.empty());
  // Service-level failure while converting the first import leaves native
  // storage empty, allowing an ordinary retry rather than committing v1.
  calls = 0; fail_at = 2; // first allocation is service read scratch
  const auto failed_boot = f.service.load_and_apply_runtime(output.data(), output.size());
  fail_at = 0;
  assert(failed_boot.status == ServiceStatus::ALLOCATION_FAILED);
  assert(f.backend.writes == 0 && f.runtime.activations == 0 && live.empty());
  assert(f.service.load_and_apply_runtime(output.data(), output.size()).imported_legacy());
  assert(live.empty());
}

void legacy_source_still_fenced(const DeviceUiLimits& limits) {
  Fixture f(limits);
  const auto nested = wire(authored_nested(limits));
  const auto saved = f.service.save(2, nested.data(), nested.size());
  assert(saved.durable());
  const auto writes = f.backend.writes;
  const auto activations = f.runtime.activations;
  const auto flattened = native_flat(limits);
  const auto rejected = f.service.save(2, flattened.data(), flattened.size(), {1});
  assert(rejected.status == ServiceStatus::INCOMPATIBLE_REPLACEMENT);
  assert(f.backend.writes == writes && f.runtime.activations == activations && read(f) == nested);
  Bytes output(32768);
  const auto shadow = f.service.refresh_legacy_shadow(output.data(), output.size());
  assert(shadow.status == ServiceStatus::UNSUPPORTED_VERSION);
  assert(f.backend.writes == writes && f.runtime.activations == activations && read(f) == nested);
  assert(live.empty());
}

void merge_allocation_failures(const DeviceUiLimits& limits) {
  const auto original = native_flat(limits);
  auto expected = records(original, 2);
  expected[std::to_string(static_cast<int>(PanelConfigRecordType::SETTING)) + ":0:button_on_color"] = "AA5500";
  Bytes output(32768);
  size_t allocation_count = 0;
  {
    Fixture f(limits);
    assert(f.service.save(2, original.data(), original.size()).ok());
    f.color.state = "AA5500";
    calls = 0;
    assert(f.service.synchronize_legacy_runtime(output.data(), output.size()).ok());
    allocation_count = calls;
    assert(allocation_count > 4 && live.empty());
  }
  for (size_t failure = 1; failure <= allocation_count; ++failure) {
    Fixture f(limits);
    const auto saved = f.service.save(2, original.data(), original.size());
    assert(saved.ok());
    f.color.state = "AA5500";
    calls = 0; fail_at = failure;
    const auto failed = f.service.synchronize_legacy_runtime(output.data(), output.size());
    fail_at = 0;
    assert(failed.status == ServiceStatus::ALLOCATION_FAILED || failed.status == ServiceStatus::LEGACY_MIRROR_FAILED);
    assert(live.empty());
    ServiceLoadResult current;
    const auto document = read(f, &current);
    assert(f.runtime.active == document);
    if (current.generation == saved.generation) {
      assert(document == original && f.runtime.activations == 1);
      assert(f.service.synchronize_legacy_runtime(output.data(), output.size()).ok());
    } else {
      // A publication failure follows a completed durable/runtime swap. It
      // is repaired from that generation, never by committing the edit twice.
      assert(current.generation == saved.generation + 1 && f.runtime.activations == 2);
      assert(records(document, 2) == expected);
      assert(f.service.retry_compatibility(output.data(), output.size()).ok());
    }
    const auto recovered = read(f, &current);
    assert(current.generation == saved.generation + 1 && records(recovered, 2) == expected);
    assert(f.runtime.active == recovered && live.empty());
    assert(f.order.writes == 0 && f.color.writes == 0 && f.button.writes == 0);
  }
}

std::vector<Row> nested_globals(const DeviceUiLimits& limits, const char* color = "77AA11") {
  auto rows = authored_nested(limits);
  Row key; key.kind = K::STRING; key.id = 88; key.text = "button_on_color";
  Row value; value.kind = K::STRING; value.id = 89; value.text = color;
  Row setting; setting.kind = K::SETTING; setting.id = key.id; setting.first = value.id;
  rows.insert(rows.end(), {key, value, setting});
  return rows;
}

void native_global_publication(const DeviceUiLimits& limits) {
  Fixture f(limits);
  Text child{"old child"};
  std::array<PanelConfigTextValue*, PanelConfigTextBindings::MAX_SUBPAGE_CHUNKS> chunks{};
  chunks[0] = &child;
  assert(f.bindings.set_button(1, &f.button, chunks));
  const auto original = wire(nested_globals(limits));
  const auto saved = f.service.save(2, original.data(), original.size());
  assert(saved.ok());
  assert(f.order.state == "2" && f.color.state == "77AA11");
  assert(f.service.compatibility_state().publication == CompatibilityStatus::NATIVE_ONLY);
  assert(!f.service.compatibility_state().retry_pending());
  assert(f.button.state == "light.room;Room;Auto;Auto;;;;;" && child.state == "old child");
  assert(f.order.writes == 0 && f.color.writes == 0 && f.button.writes == 0 && child.writes == 0);
  const auto writes = f.backend.writes;
  Bytes output(32768);
  // The native graph is authoritative. Repair does not rewrite storage or
  // pretend that the old per-card/subpage text fields contain the graph.
  assert(f.service.retry_compatibility(output.data(), output.size()).ok());
  assert(f.backend.writes == writes && read(f) == original);
  f.order.state = "stale"; f.color.state = "stale";
  ConfigurationService reboot(f.store, f.legacy, &f.future, nullptr, 0, LegacyConfigurationMode::READ_IMPORT_ONLY);
  reboot.set_buffer_allocator(allocate, release);
  reboot.set_runtime_adapter(&f.runtime);
  const auto boot = reboot.load_and_apply_runtime(output.data(), output.size());
  assert(boot.ok() && boot.generation == saved.generation);
  assert(f.order.state == "2" && f.color.state == "77AA11" && child.state == "old child");
  assert(f.backend.writes == writes && read(f) == original);
  // Colour changes arrive as complete native documents through the existing
  // shared web API. They must succeed and retain all nested/inactive records.
  const auto recolored = wire(nested_globals(limits, "AABBCC"));
  const auto edited = reboot.save_if_generation(saved.generation, 2, recolored.data(), recolored.size());
  assert(edited.ok() && edited.generation == saved.generation + 1);
  assert(f.color.state == "AABBCC" && f.order.state == "2" && read(f) == recolored);
  const auto activations = f.runtime.activations;
  const auto writes_after = f.backend.writes;
  f.button.state = "light.old;Unsafe legacy edit";
  f.order.state = "stale"; f.color.state = "stale";
  assert(reboot.synchronize_legacy_runtime(output.data(), output.size()).ok());
  assert(f.order.state == "2" && f.color.state == "AABBCC");
  assert(read(f) == recolored && f.backend.writes == writes_after && f.runtime.activations == activations);
  assert(f.legacy.mirror_status(2, recolored.data(), recolored.size()) == CompatibilityStatus::NOT_REPRESENTABLE);
  assert(f.order.writes == 0 && f.color.writes == 0 && f.button.writes == 0 && child.writes == 0);
  // Releases still promising a complete downgrade mirror continue to reject
  // this layout before any durable publication.
  ConfigurationService dual(f.store, f.legacy, &f.future);
  dual.set_runtime_adapter(&f.runtime);
  dual.set_buffer_allocator(allocate, release);
  assert(dual.save(2, recolored.data(), recolored.size()).status == ServiceStatus::CAPACITY_EXCEEDED);
  assert(f.backend.writes == writes_after && f.runtime.activations == activations && read(f) == recolored);
  assert(live.empty());
}

void native_global_publication_failures(const DeviceUiLimits& limits) {
  const auto original = wire(nested_globals(limits));
  Fixture f(limits);
  calls = 0;
  assert(f.legacy.publish_status(2, original.data(), original.size()) == CompatibilityStatus::NATIVE_ONLY);
  const auto count = calls;
  assert(count > 0 && live.empty());
  for (size_t failure = 1; failure <= count; ++failure) {
    f.order.state = "old order"; f.color.state = "old color";
    calls = 0; fail_at = failure;
    const auto status = f.legacy.publish_status(2, original.data(), original.size());
    fail_at = 0;
    assert(status == CompatibilityStatus::RETRYABLE_FAILURE && live.empty());
    assert(f.order.state == "old order" && f.color.state == "old color");
    assert(f.legacy.publish_status(2, original.data(), original.size()) == CompatibilityStatus::NATIVE_ONLY);
  }
  f.order.state = "old order"; f.color.state = "old color";
  auto wrong_profile = nested_globals(limits);
  wrong_profile[0].text = "wrong-device";
  const auto wrong = wire(wrong_profile);
  assert(f.legacy.publish_status(2, wrong.data(), wrong.size()) == CompatibilityStatus::NOT_REPRESENTABLE);
  auto inconsistent = nested_globals(limits);
  Row key; key.kind = K::STRING; key.id = 92; key.text = "button_order";
  Row value; value.kind = K::STRING; value.id = 93; value.text = "1";
  Row setting; setting.kind = K::SETTING; setting.id = key.id; setting.first = value.id;
  inconsistent.insert(inconsistent.end(), {key, value, setting});
  const auto conflict = wire(inconsistent);
  assert(f.legacy.publish_status(2, conflict.data(), conflict.size()) == CompatibilityStatus::NOT_REPRESENTABLE);
  auto corrupt = original; corrupt[0] = 'X';
  assert(f.legacy.publish_status(2, corrupt.data(), corrupt.size()) == CompatibilityStatus::NOT_REPRESENTABLE);
  assert(f.order.state == "old order" && f.color.state == "old color");
  assert(f.backend.writes == 0 && f.runtime.activations == 0 && live.empty());
  const auto no_color = wire(authored_nested(limits));
  assert(f.legacy.publish_status(2, no_color.data(), no_color.size()) == CompatibilityStatus::NATIVE_ONLY);
  assert(f.order.state == "2" && f.color.state.empty());
  assert(f.order.writes == 0 && f.color.writes == 0 && f.button.writes == 0);

  for (bool order_failure : {false, true}) {
    Fixture p(limits);
    auto& failed_field = order_failure ? p.order : p.color;
    failed_field.fail_publish = true;
    const auto saved = p.service.save(2, original.data(), original.size());
    assert(saved.status == ServiceStatus::LEGACY_MIRROR_FAILED && saved.durable());
    assert(p.runtime.active == original && p.runtime.activations == 1);
    assert(p.service.compatibility_state().retry_pending());
    const auto writes = p.backend.writes;
    Bytes output(32768);
    assert(p.service.retry_compatibility(output.data(), output.size()).status == ServiceStatus::LEGACY_MIRROR_FAILED);
    failed_field.fail_publish = false;
    const auto repaired = p.service.retry_compatibility(output.data(), output.size());
    assert(repaired.ok() && repaired.generation == saved.generation);
    assert(p.service.compatibility_state().publication == CompatibilityStatus::NATIVE_ONLY);
    assert(!p.service.compatibility_state().retry_pending());
    assert(p.order.state == "2" && p.color.state == "77AA11");
    assert(p.backend.writes == writes && p.runtime.activations == 1 && read(p) == original);
    failed_field.state = "stale"; failed_field.fail_publish = true;
    assert(p.service.synchronize_legacy_runtime(output.data(), output.size()).status == ServiceStatus::LEGACY_MIRROR_FAILED);
    assert(p.service.compatibility_state().retry_pending());
    failed_field.fail_publish = false;
    assert(p.service.retry_compatibility(output.data(), output.size()).ok());
    assert(p.backend.writes == writes && p.runtime.activations == 1 && read(p) == original);
    assert(p.order.writes == 0 && p.color.writes == 0 && p.button.writes == 0 && live.empty());
  }
}
}

int main(int argc, char**) {
  if (argc > 1) {
    bootstrap(DEVICE_UI_PROFILES[0], true);
  } else {
    for (const auto& limits : DEVICE_UI_PROFILES) {
      bootstrap(limits, false); bootstrap(limits, true);
      preserve_flat_native(limits); unchanged_arbitrary_ids(limits);
      allocation_failures(limits); legacy_source_still_fenced(limits);
      native_global_publication(limits);
    }
    merge_allocation_failures(DEVICE_UI_PROFILES[0]);
    native_global_publication_failures(DEVICE_UI_PROFILES[0]);
  }
  assert(live.empty());
}
