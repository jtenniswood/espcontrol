#include "card_image_store.h"
#include "card_asset_service.h"
#include "panel_config_asset_references.h"
#include <map>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
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

class FakeAssetPersistence : public espcontrol::CardAssetPersistence {
 public:
  std::map<uint32_t, std::vector<uint8_t>> durable, pending;
  std::optional<size_t> fail_save_after, fail_sync_after;
  bool available{true};
  bool ready() const override { return available; }
  void reset() { durable.clear(); pending.clear(); fail_save_after.reset(); fail_sync_after.reset(); available = true; }
  void reboot() { pending.clear(); fail_save_after.reset(); fail_sync_after.reset(); }
  static bool fails(std::optional<size_t> &after) {
    if (!after) return false;
    if (*after == 0) { after.reset(); return true; }
    --*after;
    return false;
  }
  template<typename T> bool read(uint32_t key, T &record) {
    const auto item = durable.find(key);
    if (item == durable.end() || item->second.size() != sizeof(T)) return false;
    std::memcpy(&record, item->second.data(), sizeof(T));
    return true;
  }
  template<typename T> bool write(uint32_t key, const T &record) {
    if (fails(fail_save_after)) return false;
    const auto *bytes = reinterpret_cast<const uint8_t *>(&record);
    pending[key] = std::vector<uint8_t>(bytes, bytes + sizeof(T));
    return true;
  }
  bool load(espcontrol::PendingDeleteRecord &r) override { return read(espcontrol::PENDING_DELETE_PREFERENCE_KEY, r); }
  bool load(espcontrol::RestoreSessionRecord &r) override { return read(espcontrol::RESTORE_SESSION_PREFERENCE_KEY, r); }
  bool load(espcontrol::CompletedRestoresRecord &r) override { return read(espcontrol::COMPLETED_RESTORES_PREFERENCE_KEY, r); }
  bool save(const espcontrol::PendingDeleteRecord &r) override { return write(espcontrol::PENDING_DELETE_PREFERENCE_KEY, r); }
  bool save(const espcontrol::RestoreSessionRecord &r) override { return write(espcontrol::RESTORE_SESSION_PREFERENCE_KEY, r); }
  bool save(const espcontrol::CompletedRestoresRecord &r) override { return write(espcontrol::COMPLETED_RESTORES_PREFERENCE_KEY, r); }
  bool sync() override {
    if (fails(fail_sync_after)) return false;
    for (const auto &item : pending) durable[item.first] = item.second;
    pending.clear();
    return true;
  }
};
FakeAssetPersistence persistence;

struct FakeFlash {
  esp_partition_t partition{PARTITION_SIZE};
  std::vector<uint8_t> bytes = std::vector<uint8_t>(PARTITION_SIZE, 0xFF);
  std::optional<size_t> fail_write_at;
  std::optional<size_t> fail_erase_at;

  void reset() {
    persistence.reset();
    std::fill(bytes.begin(), bytes.end(), 0xFF);
    fail_write_at.reset();
    fail_erase_at.reset();
  }

  void corrupt(size_t offset) {
    if (offset < bytes.size()) bytes[offset] ^= 0x5A;
  }
};

FakeFlash flash;
uint32_t random_seed = 0x12345678;

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
    if (on_clear) on_clear();
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
  std::function<void()> on_clear{};
};

void expect(bool condition, const std::string &message) {
  if (condition) return;
  std::cerr << message << '\n';
  std::exit(1);
}

std::vector<uint8_t> jpeg_bytes(size_t size = 1024);

void test_card_asset_service_has_one_application_owner() {
  espcontrol::CardAssetService first{&persistence};
  espcontrol::CardAssetService second{&persistence};
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
  espcontrol::CardAssetService service{&persistence};
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
  bool native_persistence_ready = false;
  service.set_reference_persistence_callback(
      [](void *context) { return *static_cast<bool *>(context); },
      &native_persistence_ready);
  expect(service.delete_with_references(image.id) ==
             espcontrol::CardAssetDeleteResult::PERSISTENCE_FAILED,
         "failed native configuration persistence should stop deletion");
  expect(service.find(image.id, retained),
         "failed native configuration persistence must retain the image");

  native_persistence_ready = true;
  expect(service.delete_with_references(image.id) == espcontrol::CardAssetDeleteResult::SUCCESS,
         "retry should persist every configuration source and delete the image");
  expect(adapter.cleared_id == image.id && !service.find(image.id, retained),
         "the exact image should be erased only after its references clear");
  expect(service.stop(), "asset service should stop after deletion transaction");
}

void test_card_asset_service_reserves_idle_image_before_clearing_references() {
  flash.reset();
  espcontrol::CardAssetService service{&persistence};
  expect(service.start(), "asset service should start for reader-safe deletion");
  const auto bytes = jpeg_bytes();
  CardImageUpload upload;
  expect(service.begin_upload(bytes.size(), upload) == ESP_OK,
         "reader-safe deletion fixture should reserve storage");
  expect(service.write_upload(upload, bytes.data(), bytes.size()) == ESP_OK,
         "reader-safe deletion fixture should write the image");
  CardImageInfo image;
  expect(service.commit_upload(upload, image) == ESP_OK,
         "reader-safe deletion fixture should commit the image");

  TestReferenceAdapter adapter;
  adapter.expected_id = image.id;
  service.set_reference_adapter(&adapter);
  auto reader = service.open(image.id);
  expect(reader != nullptr, "reader-safe deletion fixture should hold an active reader");
  expect(service.delete_with_references(image.id) == espcontrol::CardAssetDeleteResult::BUSY,
         "active readers should make deletion busy before references are cleared");
  expect(adapter.cleared_id.empty() && adapter.referenced,
         "busy deletion must leave every card reference intact");

  reader->end();
  std::shared_ptr<esphome::http_request::HttpContainer> racing_reader;
  adapter.on_clear = [&]() { racing_reader = service.open(image.id); };
  expect(service.delete_with_references(image.id) == espcontrol::CardAssetDeleteResult::SUCCESS,
         "idle image should delete after its references persist");
  expect(racing_reader == nullptr,
         "erase reservation should reject readers that race with reference clearing");
  CardImageInfo retained;
  expect(!service.find(image.id, retained), "reserved deletion should erase the selected image");
  expect(service.stop(), "asset service should stop after reader-safe deletion");
}

void test_card_asset_service_stages_restore_until_commit_or_rollback() {
  flash.reset();
  espcontrol::CardAssetService service{&persistence};
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
  espcontrol::CardAssetService service{&persistence};
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
  espcontrol::CardAssetService service{&persistence};
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

void test_card_asset_service_rolls_back_abandoned_restore() {
  flash.reset();
  espcontrol::CardAssetService service{&persistence};
  expect(service.start(), "asset service should start for restore timeout recovery");
  TestReferenceAdapter adapter;
  adapter.referenced = false;
  service.set_reference_adapter(&adapter);

  const std::string session = service.begin_restore_session();
  const auto bytes = jpeg_bytes();
  CardImageUpload upload;
  expect(service.begin_upload(bytes.size(), upload) == ESP_OK,
         "timeout fixture should reserve image storage");
  expect(service.write_upload(upload, bytes.data(), bytes.size()) == ESP_OK,
         "timeout fixture should write image bytes");
  CardImageInfo image;
  expect(service.commit_upload(upload, image) == ESP_OK,
         "timeout fixture should commit the staged image");
  expect(service.stage_restored_asset(session, image.id),
         "timeout fixture should track the staged image");

  for (uint32_t elapsed = 0;
       elapsed <= espcontrol::CardAssetService::RESTORE_SESSION_IDLE_TIMEOUT_MS; ++elapsed) {
    service.loop();
  }
  CardImageInfo found;
  expect(!service.find(image.id, found),
         "an abandoned restore should roll back after its inactivity timeout");
  const std::string referenced_session = service.begin_restore_session();
  expect(!referenced_session.empty(),
         "timeout recovery should make the service available for another restore");
  CardImageUpload referenced_upload;
  expect(service.begin_upload(bytes.size(), referenced_upload) == ESP_OK,
         "referenced timeout fixture should reserve image storage");
  expect(service.write_upload(referenced_upload, bytes.data(), bytes.size()) == ESP_OK,
         "referenced timeout fixture should write image bytes");
  CardImageInfo referenced_image;
  expect(service.commit_upload(referenced_upload, referenced_image) == ESP_OK,
         "referenced timeout fixture should commit the staged image");
  adapter.referenced = true;
  adapter.expected_id = referenced_image.id;
  expect(service.stage_restored_asset(referenced_session, referenced_image.id),
         "referenced timeout fixture should track the staged image");
  for (uint32_t elapsed = 0;
       elapsed <= espcontrol::CardAssetService::RESTORE_SESSION_IDLE_TIMEOUT_MS; ++elapsed) {
    service.loop();
  }
  expect(service.find(referenced_image.id, found),
         "timeout recovery must retain an image referenced by durable configuration");
  expect(service.stop(), "asset service should stop after timeout recovery");
}

CardImageInfo stage_fixture(espcontrol::CardAssetService &service, const std::string &session) {
  const auto bytes = jpeg_bytes();
  CardImageUpload upload;
  expect(service.begin_upload(bytes.size(), upload) == ESP_OK, "reserve staged fixture");
  expect(service.stage_restored_asset(session, upload.id), "journal before image publication");
  expect(service.write_upload(upload, bytes.data(), bytes.size()) == ESP_OK, "write staged fixture");
  CardImageInfo image;
  expect(service.commit_upload(upload, image) == ESP_OK, "publish staged fixture");
  return image;
}

struct NativeReferences {
  bool ready{false};
  std::vector<uint8_t> document;
  static espcontrol::CardAssetReferenceState check(void *context, const std::string &id) {
    auto &self = *static_cast<NativeReferences *>(context);
    bool referenced = false;
    if (!self.ready || !espcontrol::panel_config_references_asset(
        self.document.data(), self.document.size(), id, referenced)) {
      return espcontrol::CardAssetReferenceState::UNAVAILABLE;
    }
    return referenced ? espcontrol::CardAssetReferenceState::REFERENCED
                      : espcontrol::CardAssetReferenceState::UNREFERENCED;
  }
  void set(const std::string &main_id, const std::string &subpage_id) {
    using namespace espcontrol::configuration;
    document.resize(2048);
    PanelConfigWriter writer(document.data(), document.size());
    expect(writer.begin() == PanelConfigStatus::OK, "begin native reference fixture");
    const std::string profile = "test-panel";
    expect(writer.append_device_profile(reinterpret_cast<const uint8_t *>(profile.data()), profile.size()) == PanelConfigStatus::OK,
           "native fixture profile");
    const std::string button = "light.a;A;;;;;light;;bg_image=" + main_id;
    const std::string subpage = "1|light.b;B;;;;;light;;bg_image=" + subpage_id;
    expect(writer.append_button(1, reinterpret_cast<const uint8_t *>(button.data()), button.size()) == PanelConfigStatus::OK,
           "native main reference");
    expect(writer.append_subpage(1, reinterpret_cast<const uint8_t *>(subpage.data()), subpage.size()) == PanelConfigStatus::OK,
           "native subpage reference");
    size_t size = 0;
    expect(writer.finish(&size) == PanelConfigStatus::OK, "finish native fixture");
    document.resize(size);
  }
};

void test_persistent_recovery_uses_native_document_after_startup() {
  flash.reset();
  std::string session;
  CardImageInfo main_image, subpage_image, orphan;
  {
    espcontrol::CardAssetService service{&persistence};
    expect(service.start(), "start persistent restore");
    session = service.begin_restore_session();
    main_image = stage_fixture(service, session);
    subpage_image = stage_fixture(service, session);
    orphan = stage_fixture(service, session);
    service.stop();
  }
  persistence.reboot();
  // New process/service, persisted flash and stale legacy values: this is the
  // save-then-reboot window before the browser commits its image session.
  NativeReferences native;
  native.set(main_image.id, subpage_image.id);
  espcontrol::CardAssetService rebooted{&persistence};
  rebooted.set_recovery_reference_callback(NativeReferences::check, &native);
  expect(rebooted.start(), "reload deployed restore journal");
  TestReferenceAdapter legacy;
  legacy.referenced = false;
  rebooted.set_reference_adapter(&legacy);
  CardImageInfo found;
  expect(rebooted.find(orphan.id, found), "no recovery before native startup finishes");
  native.ready = true;
  // Unreadable authoritative data must also keep all assets recoverable.
  native.document[0] = 0;
  rebooted.loop();
  expect(rebooted.find(orphan.id, found), "invalid native document cannot authorize erase");
  native.set(main_image.id, subpage_image.id);
  rebooted.loop();
  expect(rebooted.find(main_image.id, found), "saved main-card image survives stale legacy mirror");
  expect(rebooted.find(subpage_image.id, found), "saved subpage image survives stale legacy mirror");
  expect(!rebooted.find(orphan.id, found), "only unreferenced staged image is reclaimed");
  expect(!rebooted.begin_restore_session().empty(), "recovery clears session for next restore");
  rebooted.stop();
}

void test_persistent_commit_interruption_boundaries() {
  using espcontrol::CardAssetRestoreResult;
  for (bool fail_sync : {false, true}) {
    for (size_t boundary = 0; boundary < 3; ++boundary) {
      flash.reset();
      std::string session;
      CardImageInfo image;
      {
        espcontrol::CardAssetService service{&persistence};
        expect(service.start(), "start commit interruption fixture");
        session = service.begin_restore_session();
        image = stage_fixture(service, session);
        // Commit marker, completion receipt, then active-journal clearing.
        (fail_sync ? persistence.fail_sync_after : persistence.fail_save_after) = boundary;
        expect(service.commit_restore_session(session) == CardAssetRestoreResult::PERSISTENCE_FAILED,
               "every write/sync boundary reports persistence failure");
        service.stop();
      }
      persistence.reboot();
      espcontrol::CardAssetService rebooted{&persistence};
      expect(rebooted.start(), "reload interrupted commit");
      expect(rebooted.commit_restore_session(session) == CardAssetRestoreResult::SUCCESS,
             "retry completes same session after reboot");
      rebooted.loop();
      CardImageInfo found;
      expect(rebooted.find(image.id, found), "commit retry never deletes the committed image");
      rebooted.stop();
      persistence.reboot();
      espcontrol::CardAssetService retried{&persistence};
      expect(retried.start(), "load completed receipt");
      expect(retried.commit_restore_session(session) == CardAssetRestoreResult::SUCCESS,
             "lost success response can be retried across another reboot");
      expect(retried.rollback_restore_session(session) == CardAssetRestoreResult::INVALID_SESSION,
             "completed restore cannot be rolled back");
      const auto next = retried.begin_restore_session();
      expect(!next.empty(), "completed cleanup does not block next restore");
      expect(retried.commit_restore_session(session) == CardAssetRestoreResult::SUCCESS,
             "retry previous commit does not disturb the active session");
      expect(retried.commit_restore_session(next) == CardAssetRestoreResult::SUCCESS,
             "new session still commits");
      retried.stop();
    }
  }
}

void test_persistent_recovery_finishes_committed_cleanup() {
  flash.reset();
  std::string session;
  CardImageInfo image;
  {
    espcontrol::CardAssetService service{&persistence};
    expect(service.start(), "start committed cleanup fixture");
    session = service.begin_restore_session();
    image = stage_fixture(service, session);
    persistence.fail_save_after = 1;
    expect(service.commit_restore_session(session) == espcontrol::CardAssetRestoreResult::PERSISTENCE_FAILED,
           "receipt failure leaves committed journal");
    service.stop();
  }
  persistence.reboot();
  espcontrol::CardAssetService rebooted{&persistence};
  expect(rebooted.start(), "reload committed journal");
  rebooted.loop();
  CardImageInfo found;
  expect(rebooted.find(image.id, found), "committed cleanup never needs legacy references");
  expect(rebooted.commit_restore_session(session) == espcontrol::CardAssetRestoreResult::SUCCESS,
         "loop recovery saves retry receipt");
  for (size_t i = 0; i < espcontrol::COMPLETED_RESTORE_CAPACITY; ++i) {
    const auto next = rebooted.begin_restore_session();
    expect(!next.empty(), "bounded history permits new session");
    expect(rebooted.commit_restore_session(next) == espcontrol::CardAssetRestoreResult::SUCCESS,
           "commit bounded history fixture");
  }
  expect(rebooted.commit_restore_session(session) == espcontrol::CardAssetRestoreResult::INVALID_SESSION,
         "old receipts expire after bounded number of completions");
  rebooted.stop();
}

void test_persistence_failures_before_publication() {
  for (bool fail_sync : {false, true}) {
    flash.reset();
    espcontrol::CardAssetService service{&persistence};
    expect(service.start(), "start persistence-failure fixture");
    (fail_sync ? persistence.fail_sync_after : persistence.fail_save_after) = 0;
    expect(service.begin_restore_session().empty(), "failed begin must not return a session");
    const auto session = service.begin_restore_session();
    expect(!session.empty(), "failed begin can be retried");
    (fail_sync ? persistence.fail_sync_after : persistence.fail_save_after) = 0;
    expect(!service.stage_restored_asset(session, "not-published"), "failed staging must not authorize publication");
    service.stop();
  }
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

void test_overlapping_uploads_reserve_distinct_flash_spans() {
  flash.reset();
  TestCardImageStore store;
  auto first_bytes = jpeg_bytes();
  auto second_bytes = jpeg_bytes();
  std::fill(first_bytes.begin() + 2, first_bytes.end() - 2, 0x31);
  std::fill(second_bytes.begin() + 2, second_bytes.end() - 2, 0x72);

  CardImageUpload first;
  CardImageUpload second;
  expect(store.begin_upload(first_bytes.size(), first) == ESP_OK,
         "first concurrent upload should reserve storage");
  expect(store.begin_upload(second_bytes.size(), second) == ESP_OK,
         "second concurrent upload should reserve storage");
  expect(first.offset != second.offset,
         "overlapping uploads must reserve distinct flash spans");

  const size_t half = first_bytes.size() / 2;
  expect(store.write_upload(first, first_bytes.data(), half) == ESP_OK,
         "first concurrent upload should write its first chunk");
  expect(store.write_upload(second, second_bytes.data(), second_bytes.size()) == ESP_OK,
         "second concurrent upload should write while the first remains open");
  expect(store.write_upload(first, first_bytes.data() + half,
                            first_bytes.size() - half) == ESP_OK,
         "first concurrent upload should finish after the second");

  CardImageInfo first_image;
  CardImageInfo second_image;
  expect(store.commit_upload(second, second_image) == ESP_OK,
         "second concurrent upload should commit independently");
  expect(store.commit_upload(first, first_image) == ESP_OK,
         "first concurrent upload should commit independently");

  for (const auto &fixture : std::vector<std::pair<CardImageInfo, std::vector<uint8_t>>>{
           {first_image, first_bytes}, {second_image, second_bytes}}) {
    auto reader = store.open(fixture.first.id);
    expect(reader != nullptr, "concurrent upload should remain readable");
    std::vector<uint8_t> actual(fixture.second.size());
    expect(reader->read(actual.data(), actual.size()) == static_cast<int>(actual.size()),
           "concurrent upload reader should return the complete image");
    reader->end();
    expect(actual == fixture.second,
           "concurrent upload should preserve only its own JPEG bytes");
  }
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

void test_failed_delete_index_write_retains_image() {
  flash.reset();
  std::string id;
  {
    TestCardImageStore store;
    CardImageInfo uploaded = upload(store);
    id = uploaded.id;
    flash.fail_write_at = next_index_offset();
    expect(store.erase(id) == ESP_FAIL,
           "failed journal write should fail image deletion");
    flash.fail_write_at.reset();
    CardImageInfo retained;
    expect(store.find(id, retained),
           "failed deletion persistence must retain the in-memory image");
  }
  TestCardImageStore rebooted;
  CardImageInfo retained;
  expect(rebooted.find(id, retained),
         "failed deletion persistence must retain the image after reboot");
}

void test_name_normalization_preserves_utf8_boundaries() {
  const std::string glyph = "\xE7\x8C\xAB";
  std::string input;
  for (size_t index = 0; index < 14; ++index) input += glyph;
  const std::string normalized = CardImageStore::normalize_name(input);
  expect(normalized.size() == 39,
         "UTF-8 names should stop before a code point crosses the byte limit");
  expect(normalized == input.substr(0, 39),
         "UTF-8 name truncation should preserve complete code points");
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

void test_older_index_preserves_names_when_newest_record_is_corrupt() {
  flash.reset();
  std::string surviving_id;
  CardImageInfo newest_image;
  {
    TestCardImageStore store;
    surviving_id = upload(store).id;
    CardImageInfo renamed;
    expect(store.rename(surviving_id, "Older journal name", renamed) == ESP_OK,
           "rename should be present in the older journal slot");
    newest_image = upload(store);
  }

  flash.corrupt(newest_image.offset + 128);
  TestCardImageStore rebooted;
  auto images = rebooted.list();
  expect(images.size() == 1 && images[0].id == surviving_id,
         "the older valid index should load when the newest references corrupt data");
  expect(images[0].name == "Older journal name",
         "falling back to the older index should preserve renamed image metadata");
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

void test_upload_evicts_cache_when_index_is_full() {
  flash.reset();
  TestCardImageStore store;
  std::vector<CardImageInfo> images;
  for (size_t index = 0;
       index + 1 < esphome::card_image_store::CARD_IMAGE_INDEX_MAX_RECORDS; ++index) {
    images.push_back(upload(store));
  }

  constexpr uint16_t width = 16;
  constexpr uint16_t height = 16;
  std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 2, 0x29);
  expect(store.write_rgb565_cache(images.front().id, images.front().crc32, width, height,
                                  pixels.data(), pixels.size()) == ESP_OK,
         "cache fixture should fill the final persistent index entry");

  CardImageInfo added = upload(store);
  expect(!added.id.empty(), "JPEG upload should evict a disposable cache when the index is full");
  expect(store.list().size() == esphome::card_image_store::CARD_IMAGE_INDEX_MAX_RECORDS,
         "full index should retain every source image after cache eviction");
  expect(find_magic(CACHE_MAGIC) == static_cast<size_t>(-1),
         "index-space eviction should erase the disposable cache record");
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
  for (size_t i = 0; i < size; i++) {
    random_seed = random_seed * 1664525U + 1013904223U;
    bytes[i] = static_cast<uint8_t>(random_seed >> 24);
  }
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
  test_persistent_recovery_uses_native_document_after_startup();
  test_persistent_commit_interruption_boundaries();
  test_persistent_recovery_finishes_committed_cleanup();
  test_persistence_failures_before_publication();
  persistence.reset();
  test_card_asset_service_has_one_application_owner();
  test_card_asset_service_deletes_only_after_references_persist();
  test_card_asset_service_reserves_idle_image_before_clearing_references();
  test_card_asset_service_stages_restore_until_commit_or_rollback();
  test_card_asset_service_stages_every_indexed_image();
  test_card_asset_service_retries_restore_recovery_without_pending_delete();
  test_card_asset_service_rolls_back_abandoned_restore();
  test_overlapping_uploads_reserve_distinct_flash_spans();
  test_upload_survives_reboot_and_rename();
  test_interrupted_upload_is_reclaimed();
  test_failed_index_write_rolls_back_upload();
  test_failed_journal_erase_retains_original_name();
  test_failed_journal_write_retains_original_name();
  test_failed_delete_index_write_retains_image();
  test_name_normalization_preserves_utf8_boundaries();
  test_rename_survives_one_damaged_journal_slot();
  test_older_index_preserves_names_when_newest_record_is_corrupt();
  test_legacy_index_migrates_on_rename();
  test_failed_record_erase_stops_upload();
  test_failed_payload_write_is_not_committed();
  test_corrupt_latest_index_recovers_from_records();
  test_recovered_index_stays_ahead_of_surviving_slot();
  test_corrupt_image_payload_is_rejected();
  test_cache_is_confined_to_disposable_region();
  test_cache_requires_its_source_image();
  test_upload_evicts_cache_when_index_is_full();
  std::cout << "Card image store tests passed.\n";
  return 0;
}
