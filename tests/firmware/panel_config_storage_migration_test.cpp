#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "configuration_service.h"
#include "panel_config_espidf_storage.h"
#include "panel_config_storage_migration.h"
#include "panel_flash_layout.h"

using namespace espcontrol::configuration;
namespace {
constexpr size_t NVS_CAPACITY = 1024;
constexpr size_t FLASH_CAPACITY = 2048;
const esp_partition_t partition{espcontrol::PANEL_DATA_PARTITION_BYTES};
std::vector<uint8_t> flash(partition.size, 0xFF);
std::map<std::string, std::vector<uint8_t>> nvs;
bool namespace_exists = false;
bool fail_nvs_open = false;
bool fail_read = false;
int fail_mutation = -1;
int mutations = 0;
int nvs_writes = 0;
bool corrupt_publication = false;

struct Legacy final : LegacyConfigurationAdapter {
  int loads = 0;
  LegacyLoadResult load(uint8_t *output, size_t) override {
    ++loads;
    output[0] = 'x';
    return {LegacyStatus::OK, 1, 1};
  }
  bool mirror(uint16_t, const uint8_t *, size_t) override { return false; }
};

void reset() {
  flash.assign(partition.size, 0xFF);
  std::fill(flash.end() - espcontrol::PANEL_IDENTITY_STORAGE_BYTES, flash.end(), 0xA5);
  nvs.clear();
  namespace_exists = false;
  fail_nvs_open = fail_read = corrupt_publication = false;
  fail_mutation = -1;
  mutations = nvs_writes = 0;
}

void save_document(bool on_flash, const std::vector<uint8_t> &document,
                   size_t nvs_capacity = NVS_CAPACITY) {
  EspIdfPanelConfigBlobStorage blobs;
  assert(on_flash ? blobs.begin_card_images_partition(FLASH_CAPACITY) : blobs.begin());
  BufferedBlobStorageBackend<FLASH_CAPACITY> backend(blobs);
  std::array<uint8_t, FLASH_CAPACITY * 2> memory{};
  assert(backend.begin(memory.data(), memory.size(), on_flash ? FLASH_CAPACITY : nvs_capacity));
  ConfigurationStore store(backend);
  Legacy legacy;
  ConfigurationService service(store, legacy);
  assert(service.save_current(document.data(), document.size()).durable());
}

StoreStatus migrate() {
  EspIdfPanelConfigBlobStorage source, destination;
  source.begin_read_only();
  assert(destination.begin_card_images_partition(FLASH_CAPACITY));
  std::array<uint8_t, FLASH_CAPACITY * 4> memory{};
  const auto original = nvs;
  const int original_writes = nvs_writes;
  auto result = migrate_panel_config_storage<FLASH_CAPACITY>(
      source, destination, FLASH_CAPACITY, memory.data(), memory.size());
  assert(nvs == original && nvs_writes == original_writes);
  assert(std::all_of(flash.end() - espcontrol::PANEL_IDENTITY_STORAGE_BYTES,
                     flash.end(), [](uint8_t byte) { return byte == 0xA5; }));
  return result;
}

void expect_document(const std::vector<uint8_t> &document, bool expect_import = false) {
  EspIdfPanelConfigBlobStorage destination;
  assert(destination.begin_card_images_partition(FLASH_CAPACITY));
  BufferedBlobStorageBackend<FLASH_CAPACITY> backend(destination);
  std::array<uint8_t, FLASH_CAPACITY * 2> memory{};
  assert(backend.begin(memory.data(), memory.size()));
  ConfigurationStore store(backend);
  Legacy legacy;
  ConfigurationService service(store, legacy);
  std::array<uint8_t, FLASH_CAPACITY> output{};
  const auto loaded = service.load(output.data(), output.size());
  assert(loaded.ok() && loaded.document_size == document.size());
  assert(std::memcmp(output.data(), document.data(), document.size()) == 0);
  assert(legacy.loads == (expect_import ? 1 : 0));
}
}  // namespace

esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle) {
  assert(std::string(name) == "espcontrol_cfg");
  if (fail_nvs_open) return ESP_FAIL;
  if (!namespace_exists && mode == NVS_READONLY) return ESP_ERR_NVS_NOT_FOUND;
  namespace_exists = true;
  *handle = mode == NVS_READONLY ? 1 : 2;
  return ESP_OK;
}
void nvs_close(nvs_handle_t) {}
esp_err_t nvs_get_blob(nvs_handle_t, const char *key, void *output, size_t *size) {
  const auto found = nvs.find(key);
  if (found == nvs.end()) return ESP_ERR_NVS_NOT_FOUND;
  if (output != nullptr) {
    if (*size < found->second.size()) return ESP_FAIL;
    std::memcpy(output, found->second.data(), found->second.size());
  }
  *size = found->second.size();
  return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *data, size_t size) {
  assert(handle == 2);
  ++nvs_writes;
  const auto *bytes = static_cast<const uint8_t *>(data);
  nvs[key] = std::vector<uint8_t>(bytes, bytes + size);
  return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t handle) { assert(handle == 2); return ESP_OK; }
const esp_partition_t *esp_partition_find_first(int, int, const char *label) {
  assert(std::string(label) == "card_images");
  return &partition;
}
esp_err_t esp_partition_read(const esp_partition_t *, size_t offset, void *output, size_t size) {
  if (fail_read) return ESP_FAIL;
  assert(offset + size <= flash.size());
  std::memcpy(output, flash.data() + offset, size);
  return ESP_OK;
}
esp_err_t esp_partition_write(const esp_partition_t *, size_t offset, const void *data, size_t size) {
  const bool interrupted = mutations++ == fail_mutation;
  assert(offset + size <= flash.size());
  const auto *bytes = static_cast<const uint8_t *>(data);
  for (size_t i = 0; i < (interrupted ? size / 2 : size); ++i) {
    assert((flash[offset + i] & bytes[i]) == bytes[i]);
    flash[offset + i] &= bytes[i];
  }
  if (interrupted) return ESP_FAIL;
  // Simulate a device acknowledging the commit marker but corrupting payload.
  if (corrupt_publication && offset == 0 && size == sizeof(uint32_t))
    flash[16 + CONFIGURATION_ENVELOPE_HEADER_SIZE + CONFIGURATION_DOCUMENT_HEADER_SIZE] ^= 1;
  return ESP_OK;
}
esp_err_t esp_partition_erase_range(const esp_partition_t *, size_t offset, size_t size) {
  if (mutations++ == fail_mutation) return ESP_FAIL;
  assert(offset % 4096 == 0 && size % 4096 == 0 && offset + size <= flash.size());
  std::memset(flash.data() + offset, 0xFF, size);
  return ESP_OK;
}

int main() {
  const std::vector<uint8_t> latest(500, 'n');
  const std::vector<uint8_t> older(400, 'o');
  reset();
  save_document(false, older);
  save_document(false, latest);
  assert(migrate() == StoreStatus::OK);
  expect_document(latest); // Failed mirroring must never restore stale text.
  const int migration_mutations = mutations;
  assert(migrate() == StoreStatus::OK && mutations == migration_mutations);
  expect_document(latest);

  // A later edit on flash remains authoritative even though NVS is retained.
  save_document(true, older);
  fail_nvs_open = true;
  assert(migrate() == StoreStatus::OK);
  expect_document(older);

  // The new capacity supports two maximum-sized saves followed by a reboot.
  const std::vector<uint8_t> maximum(
      FLASH_CAPACITY - CONFIGURATION_ENVELOPE_HEADER_SIZE - CONFIGURATION_DOCUMENT_HEADER_SIZE, 'm');
  fail_nvs_open = false;
  save_document(true, maximum);
  save_document(true, maximum);
  assert(migrate() == StoreStatus::OK);
  expect_document(maximum);

  // Older NVS documents can exceed the current bounded NVS write capacity.
  reset();
  const std::vector<uint8_t> large_original(1500, 'l');
  save_document(false, large_original, FLASH_CAPACITY);
  assert(migrate() == StoreStatus::OK);
  expect_document(large_original);

  // Reboot at every erase/write boundary; retry from the untouched NVS copy.
  for (int failure = 0; failure < migration_mutations; ++failure) {
    reset();
    save_document(false, older);
    save_document(false, latest);
    fail_mutation = failure;
    const auto status = migrate();
    assert(status != StoreStatus::OK && status != StoreStatus::EMPTY);
    fail_mutation = -1;
    assert(migrate() == StoreStatus::OK);
    expect_document(latest);
  }

  reset();
  save_document(false, latest);
  corrupt_publication = true;
  assert(migrate() == StoreStatus::VERIFY_FAILED);
  corrupt_publication = false;
  assert(migrate() == StoreStatus::OK);
  expect_document(latest);

  reset();
  save_document(false, latest);
  fail_read = true;
  assert(migrate() == StoreStatus::READ_FAILED && mutations == 0);
  fail_read = false;
  fail_nvs_open = true;
  assert(migrate() == StoreStatus::READ_FAILED && mutations == 0);

  // Fresh installs and an existing empty namespace still use legacy import.
  reset();
  assert(migrate() == StoreStatus::EMPTY && !namespace_exists && mutations == 0);
  expect_document({'x'}, true);
  reset();
  namespace_exists = true;
  assert(migrate() == StoreStatus::EMPTY && mutations == 0);

  // A torn newest NVS slot must fall back to its last checksummed generation.
  reset();
  save_document(false, older);
  save_document(false, latest);
  nvs["slot_b"].back() ^= 1;
  assert(migrate() == StoreStatus::OK);
  expect_document(older);
}
