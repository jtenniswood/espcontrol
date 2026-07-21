#include "card_asset_service.h"

#include <cstring>

#ifdef ESP_PLATFORM
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"
#endif

namespace espcontrol {
namespace {
CardAssetService *active_card_asset_service = nullptr;

#ifdef ESP_PLATFORM
constexpr uint32_t PENDING_DELETE_MAGIC = 0x43414444;
constexpr uint8_t PENDING_DELETE_VERSION = 1;
constexpr uint32_t PENDING_DELETE_PREFERENCE_KEY = 0x5D19A62C;

struct PendingDeleteRecord {
  uint32_t magic{PENDING_DELETE_MAGIC};
  uint8_t version{PENDING_DELETE_VERSION};
  char id[41]{};
  uint32_t checksum{0};
};

uint32_t pending_checksum(const char *id) {
  uint32_t value = 2166136261UL;
  value = (value ^ PENDING_DELETE_MAGIC) * 16777619UL;
  value = (value ^ PENDING_DELETE_VERSION) * 16777619UL;
  for (const unsigned char *cursor = reinterpret_cast<const unsigned char *>(id);
       cursor != nullptr && *cursor != '\0'; ++cursor) {
    value = (value ^ *cursor) * 16777619UL;
  }
  return value;
}
#endif
}

bool CardAssetService::start() {
  if (running_ || (active_card_asset_service != nullptr && active_card_asset_service != this)) {
    return false;
  }
  active_card_asset_service = this;
  running_ = true;
  load_pending_delete();
  return true;
}

bool CardAssetService::stop() {
  if (!running_) return false;
  clear_card_background_runtime();
  if (active_card_asset_service == this) active_card_asset_service = nullptr;
  running_ = false;
  return true;
}

void CardAssetService::loop() {
  if (!running_ || pending_delete_id_.empty() || delete_running_) return;
#ifdef ESP_PLATFORM
  const uint32_t now = esphome::millis();
  if (last_resume_attempt_ != 0 && now - last_resume_attempt_ < 5000) return;
  last_resume_attempt_ = now;
#endif
  resume_pending_delete();
}

void CardAssetService::set_reference_adapter(CardAssetReferenceAdapter *adapter) {
  reference_adapter_ = adapter;
  if (running_ && !pending_delete_id_.empty()) resume_pending_delete();
}

bool CardAssetService::load_pending_delete() {
  pending_delete_id_.clear();
#ifdef ESP_PLATFORM
  if (esphome::global_preferences == nullptr) return false;
  pending_delete_preference_ =
      esphome::global_preferences->make_preference<PendingDeleteRecord>(
          PENDING_DELETE_PREFERENCE_KEY, true);
  PendingDeleteRecord record{};
  if (!pending_delete_preference_.load(&record)) return true;
  record.id[sizeof(record.id) - 1] = '\0';
  if (record.magic != PENDING_DELETE_MAGIC || record.version != PENDING_DELETE_VERSION ||
      record.checksum != pending_checksum(record.id) ||
      !esphome::card_image_store::CardImageStore::id_valid(record.id)) {
    return true;
  }
  pending_delete_id_ = record.id;
#endif
  return true;
}

bool CardAssetService::save_pending_delete(const std::string &id) {
  if (!esphome::card_image_store::CardImageStore::id_valid(id)) return false;
#ifdef ESP_PLATFORM
  if (esphome::global_preferences == nullptr) return false;
  PendingDeleteRecord record{};
  std::strncpy(record.id, id.c_str(), sizeof(record.id) - 1);
  record.checksum = pending_checksum(record.id);
  if (!pending_delete_preference_.save(&record) || !esphome::global_preferences->sync()) return false;
#endif
  pending_delete_id_ = id;
  return true;
}

bool CardAssetService::clear_pending_delete() {
#ifdef ESP_PLATFORM
  if (esphome::global_preferences == nullptr) return false;
  PendingDeleteRecord record{};
  record.magic = 0;
  record.version = 0;
  if (!pending_delete_preference_.save(&record) || !esphome::global_preferences->sync()) return false;
#endif
  pending_delete_id_.clear();
  return true;
}

CardAssetDeleteResult CardAssetService::delete_with_references(const std::string &id) {
  if (delete_running_) return CardAssetDeleteResult::BUSY;
  esphome::card_image_store::CardImageInfo image;
  if (!store_.find(id, image)) return CardAssetDeleteResult::NOT_FOUND;
  if (!pending_delete_id_.empty() && pending_delete_id_ != id) return CardAssetDeleteResult::BUSY;
  if (reference_adapter_ == nullptr || !reference_adapter_->ready()) {
    return CardAssetDeleteResult::REFERENCES_UNAVAILABLE;
  }
  if (pending_delete_id_.empty() && !save_pending_delete(id)) {
    return CardAssetDeleteResult::PERSISTENCE_FAILED;
  }
  return resume_pending_delete();
}

CardAssetDeleteResult CardAssetService::resume_pending_delete() {
  if (pending_delete_id_.empty()) return CardAssetDeleteResult::SUCCESS;
  if (delete_running_) return CardAssetDeleteResult::BUSY;
  if (reference_adapter_ == nullptr || !reference_adapter_->ready()) {
    return CardAssetDeleteResult::REFERENCES_UNAVAILABLE;
  }

  delete_running_ = true;
  const std::string id = pending_delete_id_;
  esphome::card_image_store::CardImageInfo image;
  if (!store_.find(id, image)) {
    const bool cleared = clear_pending_delete();
    delete_running_ = false;
    return cleared ? CardAssetDeleteResult::SUCCESS : CardAssetDeleteResult::PERSISTENCE_FAILED;
  }
  if (!reference_adapter_->clear_asset_references(id) ||
      reference_adapter_->references_asset(id)) {
    delete_running_ = false;
    return CardAssetDeleteResult::PERSISTENCE_FAILED;
  }
  const esp_err_t error = store_.erase(id);
  if (error != ESP_OK) {
    delete_running_ = false;
    return error == ESP_ERR_INVALID_STATE ? CardAssetDeleteResult::BUSY
                                          : CardAssetDeleteResult::STORAGE_FAILED;
  }
  const bool cleared = clear_pending_delete();
  delete_running_ = false;
  return cleared ? CardAssetDeleteResult::SUCCESS : CardAssetDeleteResult::PERSISTENCE_FAILED;
}

CardAssetService *card_asset_service() { return active_card_asset_service; }

}  // namespace espcontrol
