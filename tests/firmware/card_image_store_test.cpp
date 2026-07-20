#include "card_image_store.h"

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

void expect(bool condition, const std::string &message) {
  if (condition) return;
  std::cerr << message << '\n';
  std::exit(1);
}

std::vector<uint8_t> jpeg_bytes(size_t size = 1024) {
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

uint32_t index_generation(size_t slot) {
  StoredIndexHeader header{};
  size_t offset = PARTITION_SIZE -
                  (CARD_IMAGE_INDEX_SECTORS - slot) * CARD_IMAGE_FLASH_SECTOR_SIZE;
  std::memcpy(&header, flash.bytes.data() + offset, sizeof(header));
  return header.generation;
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
  test_upload_survives_reboot_and_rename();
  test_interrupted_upload_is_reclaimed();
  test_failed_index_write_rolls_back_upload();
  test_failed_record_erase_stops_upload();
  test_failed_payload_write_is_not_committed();
  test_corrupt_latest_index_recovers_from_records();
  test_recovered_index_stays_ahead_of_surviving_slot();
  test_corrupt_image_payload_is_rejected();
  test_cache_is_confined_to_disposable_region();
  std::cout << "Card image store tests passed.\n";
  return 0;
}
