#include "card_image_store.h"
#include "card_asset_service.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

using esphome::card_image_store::CardImageInfo;
using esphome::card_image_store::CardImageStore;
using esphome::card_image_store::CardImageUpload;
using esphome::card_image_store::CARD_IMAGE_FLASH_SECTOR_SIZE;
using esphome::card_image_store::CARD_IMAGE_INDEX_SECTORS;

constexpr size_t PARTITION_SIZE = 2 * 1024 * 1024;
constexpr uint32_t CACHE_MAGIC = 0x4354484D;

struct StoredIndexHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t generation;
  uint32_t count;
  uint32_t crc32;
};

struct FakeFlash {
  esp_partition_t partition{PARTITION_SIZE};
  std::vector<uint8_t> bytes = std::vector<uint8_t>(PARTITION_SIZE, 0xFF);
  std::optional<size_t> fail_write_at;
  std::optional<size_t> fail_erase_at;

  void reset() {
    std::fill(bytes.begin(), bytes.end(), 0xFF);
    fail_write_at.reset();
    fail_erase_at.reset();
  }

  void corrupt(size_t offset) {
    if (offset < bytes.size()) bytes[offset] ^= 0x5A;
  }
};

FakeFlash flash;
uint8_t random_seed = 0;

class TestCardImageStore : public CardImageStore {
 public:
  TestCardImageStore() : CardImageStore() {}
};

class TestReferenceAdapter : public espcontrol::CardAssetReferenceAdapter {
 public:
  bool ready() const override { return is_ready; }
  bool clear_asset_references(const std::string &asset_id) override {
    cleared_id = asset_id;
    if (fail_clear) return false;
    referenced = false;
    return true;
  }
  bool references_asset(const std::string &asset_id) const override {
    return referenced && asset_id == expected_id;
  }

  std::string expected_id;
  std::string cleared_id;
  bool is_ready{true};
  bool fail_clear{false};
  bool referenced{true};
};

void expect(bool condition, const std::string &message) {
  if (condition) return;
  std::cerr << message << '\n';
  std::exit(1);
}

std::vector<uint8_t> jpeg_bytes(size_t size = 1024);

void test_card_asset_service_has_one_application_owner() {
  espcontrol::CardAssetService first;
  espcontrol::CardAssetService second;
  expect(espcontrol::card_asset_service() == nullptr, "asset service should start unregistered");
  expect(first.start(), "application owner should register its asset service");
  expect(espcontrol::card_asset_service() == &first,
         "adapters should resolve the application-owned service");
  expect(!first.start(), "one service cannot be started twice");
  expect(!second.start(), "a second application owner must not replace the active service");
  expect(first.stop(), "active service should stop cleanly");
  expect(espcontrol::card_asset_service() == nullptr, "stopping should remove adapter access");
  expect(!first.stop(), "stopped service cannot be stopped twice");
}

void test_card_asset_service_deletes_only_after_references_persist() {
  flash.reset();
  espcontrol::CardAssetService service;
  expect(service.start(), "asset service should start for deletion transaction");
  const auto bytes = jpeg_bytes();
  CardImageUpload transaction;
  expect(service.begin_upload(bytes.size(), transaction) == ESP_OK,
         "service upload should reserve storage");
  expect(service.write_upload(transaction, bytes.data(), bytes.size()) == ESP_OK,
         "service upload should write the image");
  CardImageInfo image;
  expect(service.commit_upload(transaction, image) == ESP_OK,
         "service upload should commit the image");

  expect(service.delete_with_references(image.id) ==
             espcontrol::CardAssetDeleteResult::REFERENCES_UNAVAILABLE,
         "delete should wait for the configuration adapter");
  CardImageInfo retained;
  expect(service.find(image.id, retained), "unavailable references must retain the image");

  TestReferenceAdapter adapter;
  adapter.expected_id = image.id;
  adapter.fail_clear = true;
  service.set_reference_adapter(&adapter);
  expect(service.delete_with_references(image.id) ==
             espcontrol::CardAssetDeleteResult::PERSISTENCE_FAILED,
         "failed reference persistence should stop deletion");
  expect(service.find(image.id, retained), "failed reference persistence must retain the image");

  adapter.fail_clear = false;
  expect(service.delete_with_references(image.id) == espcontrol::CardAssetDeleteResult::SUCCESS,
         "retry should clear references and delete the image");
  expect(adapter.cleared_id == image.id && !service.find(image.id, retained),
         "the exact image should be erased only after its references clear");
  expect(service.stop(), "asset service should stop after deletion transaction");
}

void test_card_asset_service_stages_restore_until_commit_or_rollback() {
  flash.reset();
  espcontrol::CardAssetService service;
  expect(service.start(), "asset service should start for restore staging");
  TestReferenceAdapter adapter;
  adapter.referenced = false;
  service.set_reference_adapter(&adapter);

  const std::string rollback_session = service.begin_restore_session();
  expect(!rollback_session.empty(), "restore should create a durable session token");
  const auto bytes = jpeg_bytes();
  CardImageUpload rollback_upload;
  expect(service.begin_upload(bytes.size(), rollback_upload) == ESP_OK,
         "staged upload should reserve storage");
  expect(service.write_upload(rollback_upload, bytes.data(), bytes.size()) == ESP_OK,
         "staged upload should write bytes");
  CardImageInfo rollback_image;
  expect(service.commit_upload(rollback_upload, rollback_image) == ESP_OK,
         "staged upload should commit its image record");
  expect(service.stage_restored_asset(rollback_session, rollback_image.id),
         "restore session should durably track its new image");
  expect(service.rollback_restore_session(rollback_session) ==
             espcontrol::CardAssetRestoreResult::SUCCESS,
         "rollback should remove every staged image");
  CardImageInfo found;
  expect(!service.find(rollback_image.id, found), "rolled-back image should not remain visible");

  const std::string commit_session = service.begin_restore_session();
  CardImageUpload commit_upload;
  expect(service.begin_upload(bytes.size(), commit_upload) == ESP_OK,
         "second staged upload should reserve storage");
  expect(service.write_upload(commit_upload, bytes.data(), bytes.size()) == ESP_OK,
         "second staged upload should write bytes");
  CardImageInfo commit_image;
  expect(service.commit_upload(commit_upload, commit_image) == ESP_OK,
         "second staged upload should commit its image record");
  expect(service.stage_restored_asset(commit_session, commit_image.id),
         "commit session should track its image");
  expect(service.commit_restore_session(commit_session) ==
             espcontrol::CardAssetRestoreResult::SUCCESS,
         "commit should make the staged restore permanent");
  expect(service.find(commit_image.id, found), "committed restore image should remain visible");
  expect(service.stop(), "asset service should stop after restore staging");
}

void test_card_asset_service_stages_every_indexed_image() {
  flash.reset();
  espcontrol::CardAssetService service;
  expect(service.start(), "asset service should start for a large restore");
  const std::string session = service.begin_restore_session();
  expect(!session.empty(), "large restore should create a durable session token");

  for (size_t index = 0;
       index < esphome::card_image_store::CARD_IMAGE_INDEX_MAX_RECORDS; ++index) {
    char id[41];
    std::snprintf(id, sizeof(id), "restored-image-%02zu", index);
    expect(service.stage_restored_asset(session, id),
           "restore journal must accept every image the persistent index can hold");
  }
  expect(!service.stage_restored_asset(session, "one-image-past-index-capacity"),
         "restore journal should stop at the persistent image index capacity");

  expect(service.commit_restore_session(session) ==
             espcontrol::CardAssetRestoreResult::SUCCESS,
         "large restore should commit after every image is tracked");
  expect(service.stop(), "asset service should stop after the large restore");
}

void test_card_asset_service_retries_restore_recovery_without_pending_delete() {
  flash.reset();
  espcontrol::CardAssetService service;
  expect(service.start(), "asset service should start for restore recovery");
  TestReferenceAdapter adapter;
  adapter.is_ready = false;
  adapter.referenced = false;
  service.set_reference_adapter(&adapter);

  const std::string session = service.begin_restore_session();
  const auto bytes = jpeg_bytes();
  CardImageUpload upload;
  expect(service.begin_upload(bytes.size(), upload) == ESP_OK,
         "recovery fixture should reserve image storage");
  expect(service.write_upload(upload, bytes.data(), bytes.size()) == ESP_OK,
         "recovery fixture should write image bytes");
  CardImageInfo image;
  expect(service.commit_upload(upload, image) == ESP_OK,
         "recovery fixture should commit the staged image");
  expect(service.stage_restored_asset(session, image.id),
         "recovery fixture should track the staged image");
  expect(service.rollback_restore_session(session) ==
             espcontrol::CardAssetRestoreResult::ROLLBACK_FAILED,
         "unavailable references should leave durable rollback work pending");

  adapter.is_ready = true;
  service.loop();
  CardImageInfo found;
  expect(!service.find(image.id, found),
         "service loop should retry restore rollback when references become ready");
  expect(!service.begin_restore_session().empty(),
         "successful retry should clear the old session for later restores");
  expect(service.stop(), "asset service should stop after restore recovery");
}

std::vector<uint8_t> jpeg_bytes(size_t size) {
  expect(size >= 4, "JPEG fixture must include start and end markers");
  std::vector<uint8_t> bytes(size, 0x42);
  bytes[0] = 0xFF;
  bytes[1] = 0xD8;
  bytes[size - 2] = 0xFF;
  bytes[size - 1] = 0xD9;
  return bytes;
}

CardImageInfo upload(TestCardImageStore &store, size_t size = 1024) {
  auto bytes = jpeg_bytes(size);
  CardImageUpload transaction;
  expect(store.begin_upload(bytes.size(), transaction) == ESP_OK, "upload should reserve storage");
  expect(store.write_upload(transaction, bytes.data(), bytes.size()) == ESP_OK,
         "upload should write JPEG bytes");
  CardImageInfo info;
  expect(store.commit_upload(transaction, info) == ESP_OK, "upload should commit its index entry");
  return info;
}

size_t newest_index_offset() {
  return PARTITION_SIZE - CARD_IMAGE_FLASH_SECTOR_SIZE;
}

size_t index_slot_offset(size_t slot) {
  return PARTITION_SIZE -
         (CARD_IMAGE_INDEX_SECTORS - slot) * CARD_IMAGE_FLASH_SECTOR_SIZE;
}

uint32_t index_generation(size_t slot) {
  StoredIndexHeader header{};
  size_t offset = PARTITION_SIZE -
                  (CARD_IMAGE_INDEX_SECTORS - slot) * CARD_IMAGE_FLASH_SECTOR_SIZE;
  std::memcpy(&header, flash.bytes.data() + offset, sizeof(header));
  return header.generation;
}

size_t next_index_offset() {
  return index_generation(0) > index_generation(1)
           ? index_slot_offset(1) : index_slot_offset(0);
}

void write_legacy_index(size_t slot, uint32_t generation,
                        const std::vector<uint32_t> &offsets) {
  std::vector<uint8_t> sector(CARD_IMAGE_FLASH_SECTOR_SIZE, 0xFF);
  auto *header = reinterpret_cast<StoredIndexHeader *>(sector.data());
  header->magic = 0x43494E58;
  header->version = 1;
  header->generation = generation;
  header->count = static_cast<uint32_t>(offsets.size());
  auto *stored_offsets = reinterpret_cast<uint32_t *>(sector.data() + sizeof(*header));
  std::memcpy(stored_offsets, offsets.data(), offsets.size() * sizeof(uint32_t));
  uint32_t crc = 0xFFFFFFFFu;
  auto update = [&crc](const uint8_t *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
      crc ^= data[i];
      for (int bit = 0; bit < 8; bit++) {
        crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320u : 0u);
      }
    }
  };
  update(reinterpret_cast<const uint8_t *>(&header->generation), sizeof(header->generation));
  update(reinterpret_cast<const uint8_t *>(&header->count), sizeof(header->count));
  update(reinterpret_cast<const uint8_t *>(stored_offsets), offsets.size() * sizeof(uint32_t));
  header->crc32 = crc ^ 0xFFFFFFFFu;
  std::memcpy(flash.bytes.data() + index_slot_offset(slot), sector.data(), sector.size());
}

size_t data_capacity() {
  return PARTITION_SIZE - CARD_IMAGE_INDEX_SECTORS * CARD_IMAGE_FLASH_SECTOR_SIZE;
}

size_t find_magic(uint32_t magic) {
  for (size_t offset = 0; offset + sizeof(magic) <= data_capacity();
       offset += CARD_IMAGE_FLASH_SECTOR_SIZE) {
    uint32_t candidate = 0;
    std::memcpy(&candidate, flash.bytes.data() + offset, sizeof(candidate));
    if (candidate == magic) return offset;
  }
  return static_cast<size_t>(-1);
}

void test_upload_survives_reboot_and_rename() {
  flash.reset();
  std::string id;
  {
    TestCardImageStore store;
    CardImageInfo uploaded = upload(store);
    id = uploaded.id;
    CardImageInfo renamed;
    expect(store.rename(id, "Kitchen scene", renamed) == ESP_OK, "rename should succeed");
    expect(renamed.name == "Kitchen scene", "rename should update metadata");
  }
  {
    TestCardImageStore rebooted;
    auto images = rebooted.list();
    expect(images.size() == 1, "committed image should survive reboot");
    expect(images[0].id == id && images[0].name == "Kitchen scene",
           "reboot should preserve image identity and name");
    auto reader = rebooted.open(id);
    expect(reader != nullptr, "committed image should be readable");
    auto expected = jpeg_bytes();
    std::vector<uint8_t> actual(expected.size());
    expect(reader->read(actual.data(), actual.size()) == static_cast<int>(actual.size()),
           "reader should return the complete image");
    reader->end();
    expect(actual == expected, "reader should preserve original JPEG bytes");
  }
}

void test_interrupted_upload_is_reclaimed() {
  flash.reset();
  size_t abandoned_offset = 0;
  {
    TestCardImageStore store;
    auto bytes = jpeg_bytes();
    CardImageUpload transaction;
    expect(store.begin_upload(bytes.size(), transaction) == ESP_OK, "upload should begin");
    abandoned_offset = transaction.offset;
    expect(store.write_upload(transaction, bytes.data(), bytes.size() / 2) == ESP_OK,
           "partial upload should write its received bytes");
  }
  {
    TestCardImageStore rebooted;
    expect(rebooted.list().empty(), "uncommitted upload should not appear after reboot");
    CardImageUpload retry;
    expect(rebooted.begin_upload(1024, retry) == ESP_OK, "abandoned space should be reusable");
    expect(retry.offset == abandoned_offset, "retry should reclaim the abandoned record location");
    rebooted.abort_upload(retry);
  }
}

void test_failed_index_write_rolls_back_upload() {
  flash.reset();
  {
    TestCardImageStore store;
    auto bytes = jpeg_bytes();
    CardImageUpload transaction;
    expect(store.begin_upload(bytes.size(), transaction) == ESP_OK, "upload should begin");
    expect(store.write_upload(transaction, bytes.data(), bytes.size()) == ESP_OK,
           "upload body should be written");
    flash.fail_write_at = newest_index_offset();
    CardImageInfo ignored;
    expect(store.commit_upload(transaction, ignored) == ESP_FAIL,
           "failed journal write should fail the upload commit");
    flash.fail_write_at.reset();
  }
  TestCardImageStore rebooted;
  expect(rebooted.list().empty(), "failed commit must not leave a visible image");
}

void test_failed_journal_erase_retains_original_name() {
  flash.reset();
  std::string id;
  {
    TestCardImageStore store;
    CardImageInfo uploaded = upload(store);
    id = uploaded.id;
    flash.fail_erase_at = next_index_offset();
    CardImageInfo ignored;
    expect(store.rename(id, "Unsafe rename", ignored) == ESP_FAIL,
           "journal erase failure should fail rename");
    flash.fail_erase_at.reset();
    expect(store.list()[0].name == id, "failed erase should retain the in-memory name");
  }
  TestCardImageStore rebooted;
  expect(rebooted.list()[0].name == id, "failed erase should retain the persisted name");
}

void test_failed_journal_write_retains_original_name() {
  flash.reset();
  std::string id;
  {
    TestCardImageStore store;
    CardImageInfo uploaded = upload(store);
    id = uploaded.id;
    flash.fail_write_at = next_index_offset();
    CardImageInfo ignored;
    expect(store.rename(id, "Unsafe rename", ignored) == ESP_FAIL,
           "journal write failure should fail rename");
    flash.fail_write_at.reset();
    expect(store.list()[0].name == id, "failed write should retain the in-memory name");
  }
  TestCardImageStore rebooted;
  expect(rebooted.list()[0].name == id, "failed write should retain the persisted name");
}

void test_rename_survives_one_damaged_journal_slot() {
  flash.reset();
  std::string id;
  {
    TestCardImageStore store;
    id = upload(store).id;
    CardImageInfo renamed;
    expect(store.rename(id, "Journal name", renamed) == ESP_OK, "rename should commit metadata");
  }
  flash.corrupt(index_slot_offset(1));
  TestCardImageStore rebooted;
  auto images = rebooted.list();
  expect(images.size() == 1 && images[0].name == "Journal name",
         "the surviving journal slot should preserve rename metadata");
}

void test_legacy_index_migrates_on_rename() {
  flash.reset();
  std::string id;
  size_t offset = 0;
  {
    TestCardImageStore store;
    CardImageInfo uploaded = upload(store);
    id = uploaded.id;
    offset = uploaded.offset;
  }
  write_legacy_index(0, 10, {static_cast<uint32_t>(offset)});
  write_legacy_index(1, 11, {static_cast<uint32_t>(offset)});
  {
    TestCardImageStore migrated;
    expect(migrated.list()[0].name == id, "legacy index should fall back to the image header name");
    CardImageInfo renamed;
    expect(migrated.rename(id, "Migrated name", renamed) == ESP_OK,
           "legacy index should migrate when metadata changes");
  }
  TestCardImageStore rebooted;
  expect(rebooted.list()[0].name == "Migrated name",
         "the migrated metadata journal should survive reboot");
}

void test_failed_record_erase_stops_upload() {
  flash.reset();
  TestCardImageStore store;
  expect(store.list().empty(), "fresh store should initialise its journal");
  flash.fail_erase_at = 0;
  CardImageUpload transaction;
  expect(store.begin_upload(1024, transaction) == ESP_FAIL,
         "failed data-sector erase should stop the upload before writing");
  flash.fail_erase_at.reset();
  expect(store.list().empty(), "failed reservation must not create an image entry");
}

void test_failed_payload_write_is_not_committed() {
  flash.reset();
  {
    TestCardImageStore store;
    auto bytes = jpeg_bytes();
    CardImageUpload transaction;
    expect(store.begin_upload(bytes.size(), transaction) == ESP_OK, "upload should begin");
    flash.fail_write_at = transaction.offset + 128;
    expect(store.write_upload(transaction, bytes.data(), bytes.size()) == ESP_FAIL,
           "failed payload write should be reported");
    flash.fail_write_at.reset();
    store.abort_upload(transaction);
  }
  TestCardImageStore rebooted;
  expect(rebooted.list().empty(), "failed payload write must not survive reboot");
}

void test_corrupt_latest_index_recovers_from_records() {
  flash.reset();
  std::string id;
  {
    TestCardImageStore store;
    id = upload(store).id;
  }
  flash.corrupt(newest_index_offset());
  TestCardImageStore rebooted;
  auto images = rebooted.list();
  expect(images.size() == 1 && images[0].id == id,
         "degraded journal should rebuild from committed image records");
}

void test_recovered_index_stays_ahead_of_surviving_slot() {
  flash.reset();
  std::string id;
  {
    TestCardImageStore store;
    id = upload(store).id;
  }
  expect(index_generation(1) == 2, "latest index fixture should use generation 2");
  flash.corrupt(PARTITION_SIZE - CARD_IMAGE_INDEX_SECTORS * CARD_IMAGE_FLASH_SECTOR_SIZE);

  {
    TestCardImageStore recovered;
    auto images = recovered.list();
    expect(images.size() == 1 && images[0].id == id,
           "degraded journal should recover the committed image");
  }

  expect(index_generation(0) == 3,
         "recovered index should be newer than the surviving journal slot");
  TestCardImageStore rebooted;
  auto images = rebooted.list();
  expect(images.size() == 1 && images[0].id == id,
         "the recovered index should remain authoritative after another reboot");
}

void test_corrupt_image_payload_is_rejected() {
  flash.reset();
  CardImageInfo uploaded;
  {
    TestCardImageStore store;
    uploaded = upload(store);
  }
  flash.corrupt(uploaded.offset + 128);
  TestCardImageStore rebooted;
  expect(rebooted.list().empty(), "CRC-corrupt image should be excluded during recovery");
}

void test_cache_is_confined_to_disposable_region() {
  flash.reset();
  TestCardImageStore store;
  CardImageInfo source = upload(store);
  constexpr uint16_t width = 16;
  constexpr uint16_t height = 16;
  std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 2, 0x7B);
  expect(store.write_rgb565_cache(source.id, source.crc32, width, height,
                                  pixels.data(), pixels.size()) == ESP_OK,
         "decoded cache should be stored");
  size_t offset = find_magic(CACHE_MAGIC);
  size_t cache_start = esphome::card_image_store::layout::cache_region_start(
      data_capacity(), CARD_IMAGE_FLASH_SECTOR_SIZE);
  expect(offset != static_cast<size_t>(-1), "cache record should exist in flash");
  expect(offset >= cache_start, "cache record must stay inside the disposable region");
}

void test_cache_requires_its_source_image() {
  flash.reset();
  TestCardImageStore store;
  CardImageInfo source = upload(store);
  constexpr uint16_t width = 16;
  constexpr uint16_t height = 16;
  std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 2, 0x4C);

  expect(store.write_rgb565_cache(source.id, source.crc32 ^ 1, width, height,
                                  pixels.data(), pixels.size()) == ESP_ERR_INVALID_STATE,
         "cache should reject a checksum from an outdated source image");
  expect(store.erase(source.id) == ESP_OK, "source image should be deletable before delayed cache write");
  expect(store.write_rgb565_cache(source.id, source.crc32, width, height,
                                  pixels.data(), pixels.size()) == ESP_ERR_NOT_FOUND,
         "delayed cache write should reject a deleted source image");
  expect(find_magic(CACHE_MAGIC) == static_cast<size_t>(-1),
         "rejected delayed cache write must not consume flash storage");
}

}  // namespace

const esp_partition_t *esp_partition_find_first(int, esp_partition_subtype_t,
                                                 const char *) {
  return &flash.partition;
}

esp_err_t esp_partition_read(const esp_partition_t *, size_t offset,
                             void *destination, size_t size) {
  if (offset > flash.bytes.size() || size > flash.bytes.size() - offset) return ESP_FAIL;
  std::memcpy(destination, flash.bytes.data() + offset, size);
  return ESP_OK;
}

esp_err_t esp_partition_write(const esp_partition_t *, size_t offset,
                              const void *source, size_t size) {
  if (flash.fail_write_at && offset == *flash.fail_write_at) return ESP_FAIL;
  if (offset > flash.bytes.size() || size > flash.bytes.size() - offset) return ESP_FAIL;
  const auto *input = static_cast<const uint8_t *>(source);
  for (size_t i = 0; i < size; i++) {
    if ((flash.bytes[offset + i] & input[i]) != input[i]) return ESP_FAIL;
  }
  for (size_t i = 0; i < size; i++) flash.bytes[offset + i] &= input[i];
  return ESP_OK;
}

esp_err_t esp_partition_erase_range(const esp_partition_t *, size_t offset,
                                    size_t size) {
  if (flash.fail_erase_at && offset == *flash.fail_erase_at) return ESP_FAIL;
  if (offset % CARD_IMAGE_FLASH_SECTOR_SIZE != 0 ||
      size % CARD_IMAGE_FLASH_SECTOR_SIZE != 0 ||
      offset > flash.bytes.size() || size > flash.bytes.size() - offset) {
    return ESP_FAIL;
  }
  std::fill(flash.bytes.begin() + offset, flash.bytes.begin() + offset + size, 0xFF);
  return ESP_OK;
}

void esp_fill_random(void *buffer, size_t size) {
  auto *bytes = static_cast<uint8_t *>(buffer);
  for (size_t i = 0; i < size; i++) bytes[i] = ++random_seed;
}

uint32_t esp_rom_crc32_le(uint32_t seed, const uint8_t *data, uint32_t size) {
  uint32_t crc = seed ^ 0xFFFFFFFFu;
  for (uint32_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320u : 0u);
  }
  return crc ^ 0xFFFFFFFFu;
}

int main() {
  test_card_asset_service_has_one_application_owner();
  test_card_asset_service_deletes_only_after_references_persist();
  test_card_asset_service_stages_restore_until_commit_or_rollback();
  test_card_asset_service_stages_every_indexed_image();
  test_card_asset_service_retries_restore_recovery_without_pending_delete();
  test_upload_survives_reboot_and_rename();
  test_interrupted_upload_is_reclaimed();
  test_failed_index_write_rolls_back_upload();
  test_failed_journal_erase_retains_original_name();
  test_failed_journal_write_retains_original_name();
  test_rename_survives_one_damaged_journal_slot();
  test_legacy_index_migrates_on_rename();
  test_failed_record_erase_stops_upload();
  test_failed_payload_write_is_not_committed();
  test_corrupt_latest_index_recovers_from_records();
  test_recovered_index_stays_ahead_of_surviving_slot();
  test_corrupt_image_payload_is_rejected();
  test_cache_is_confined_to_disposable_region();
  test_cache_requires_its_source_image();
  std::cout << "Card image store tests passed.\n";
  return 0;
}
