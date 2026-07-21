#include "card_asset_service.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#ifdef ESP_PLATFORM
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"
#include <esp_random.h>
#endif

namespace espcontrol {
namespace {
CardAssetService *active_card_asset_service = nullptr;
constexpr size_t MAX_STAGED_RESTORE_ASSETS = 40;

#ifdef ESP_PLATFORM
constexpr uint32_t PENDING_DELETE_MAGIC = 0x43414444;
constexpr uint8_t PENDING_DELETE_VERSION = 1;
constexpr uint32_t PENDING_DELETE_PREFERENCE_KEY = 0x5D19A62C;
constexpr uint32_t RESTORE_SESSION_MAGIC = 0x43415253;
constexpr uint8_t RESTORE_SESSION_VERSION = 1;
constexpr uint32_t RESTORE_SESSION_PREFERENCE_KEY = 0x5D19A62D;

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

struct RestoreSessionRecord {
  uint32_t magic{RESTORE_SESSION_MAGIC};
  uint8_t version{RESTORE_SESSION_VERSION};
  uint8_t count{0};
  char session[17]{};
  char ids[MAX_STAGED_RESTORE_ASSETS][41]{};
  uint32_t checksum{0};
};

uint32_t restore_checksum(const RestoreSessionRecord &record) {
  uint32_t value = 2166136261UL;
  const auto update = [&value](const char *text) {
    for (const unsigned char *cursor = reinterpret_cast<const unsigned char *>(text);
         cursor != nullptr && *cursor != '\0'; ++cursor) {
      value = (value ^ *cursor) * 16777619UL;
    }
  };
  value = (value ^ record.magic) * 16777619UL;
  value = (value ^ record.version) * 16777619UL;
  value = (value ^ record.count) * 16777619UL;
  update(record.session);
  for (size_t index = 0; index < record.count && index < MAX_STAGED_RESTORE_ASSETS; ++index) {
    update(record.ids[index]);
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
  load_restore_session();
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
  if (restore_recovery_needed_ && !delete_running_) {
    rollback_restore_session(restore_session_);
  }
}

void CardAssetService::set_reference_adapter(CardAssetReferenceAdapter *adapter) {
  reference_adapter_ = adapter;
  if (running_ && !pending_delete_id_.empty()) resume_pending_delete();
  if (running_ && restore_recovery_needed_ && !restore_session_.empty()) {
    rollback_restore_session(restore_session_);
  }
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

bool CardAssetService::load_restore_session() {
  restore_session_.clear();
  staged_restore_ids_.clear();
  restore_recovery_needed_ = false;
#ifdef ESP_PLATFORM
  if (esphome::global_preferences == nullptr) return false;
  restore_session_preference_ =
      esphome::global_preferences->make_preference<RestoreSessionRecord>(
          RESTORE_SESSION_PREFERENCE_KEY, true);
  RestoreSessionRecord record{};
  if (!restore_session_preference_.load(&record)) return true;
  record.session[sizeof(record.session) - 1] = '\0';
  for (auto &id : record.ids) id[sizeof(id) - 1] = '\0';
  if (record.magic != RESTORE_SESSION_MAGIC || record.version != RESTORE_SESSION_VERSION ||
      record.count > MAX_STAGED_RESTORE_ASSETS ||
      record.checksum != restore_checksum(record) || record.session[0] == '\0') {
    return true;
  }
  restore_session_ = record.session;
  for (size_t index = 0; index < record.count; ++index) {
    if (!esphome::card_image_store::CardImageStore::id_valid(record.ids[index])) return true;
    staged_restore_ids_.emplace_back(record.ids[index]);
  }
  restore_recovery_needed_ = true;
#endif
  return true;
}

bool CardAssetService::save_restore_session() {
#ifdef ESP_PLATFORM
  if (esphome::global_preferences == nullptr || restore_session_.empty() ||
      staged_restore_ids_.size() > MAX_STAGED_RESTORE_ASSETS) {
    return false;
  }
  RestoreSessionRecord record{};
  record.count = static_cast<uint8_t>(staged_restore_ids_.size());
  std::strncpy(record.session, restore_session_.c_str(), sizeof(record.session) - 1);
  for (size_t index = 0; index < staged_restore_ids_.size(); ++index) {
    std::strncpy(record.ids[index], staged_restore_ids_[index].c_str(),
                 sizeof(record.ids[index]) - 1);
  }
  record.checksum = restore_checksum(record);
  if (!restore_session_preference_.save(&record) || !esphome::global_preferences->sync()) return false;
#endif
  return true;
}

bool CardAssetService::clear_restore_session() {
#ifdef ESP_PLATFORM
  if (esphome::global_preferences == nullptr) return false;
  RestoreSessionRecord record{};
  record.magic = 0;
  record.version = 0;
  if (!restore_session_preference_.save(&record) || !esphome::global_preferences->sync()) return false;
#endif
  restore_session_.clear();
  staged_restore_ids_.clear();
  restore_recovery_needed_ = false;
  return true;
}

std::string CardAssetService::begin_restore_session() {
  if (!restore_session_.empty()) return "";
  char token[17];
#ifdef ESP_PLATFORM
  std::snprintf(token, sizeof(token), "%08lx%08lx",
                static_cast<unsigned long>(esp_random()),
                static_cast<unsigned long>(esp_random()));
#else
  static uint32_t next_session = 0;
  std::snprintf(token, sizeof(token), "%016lx", static_cast<unsigned long>(++next_session));
#endif
  restore_session_ = token;
  staged_restore_ids_.clear();
  restore_recovery_needed_ = false;
  if (!save_restore_session()) {
    restore_session_.clear();
    return "";
  }
  return restore_session_;
}

bool CardAssetService::stage_restored_asset(const std::string &session, const std::string &id) {
  if (session.empty() || session != restore_session_ ||
      !esphome::card_image_store::CardImageStore::id_valid(id) ||
      staged_restore_ids_.size() >= MAX_STAGED_RESTORE_ASSETS) {
    return false;
  }
  if (std::find(staged_restore_ids_.begin(), staged_restore_ids_.end(), id) !=
      staged_restore_ids_.end()) {
    return true;
  }
  staged_restore_ids_.push_back(id);
  if (save_restore_session()) return true;
  staged_restore_ids_.pop_back();
  return false;
}

void CardAssetService::unstage_restored_asset(const std::string &session, const std::string &id) {
  if (session != restore_session_) return;
  const auto item = std::find(staged_restore_ids_.begin(), staged_restore_ids_.end(), id);
  if (item == staged_restore_ids_.end()) return;
  staged_restore_ids_.erase(item);
  save_restore_session();
}

CardAssetRestoreResult CardAssetService::commit_restore_session(const std::string &session) {
  if (session.empty() || session != restore_session_) return CardAssetRestoreResult::INVALID_SESSION;
  return clear_restore_session() ? CardAssetRestoreResult::SUCCESS
                                 : CardAssetRestoreResult::PERSISTENCE_FAILED;
}

CardAssetRestoreResult CardAssetService::rollback_restore_session(const std::string &session) {
  if (session.empty() || session != restore_session_) return CardAssetRestoreResult::INVALID_SESSION;
  restore_recovery_needed_ = true;
  for (const auto &id : staged_restore_ids_) {
    esphome::card_image_store::CardImageInfo image;
    if (!store_.find(id, image)) continue;
    const CardAssetDeleteResult result = delete_with_references(id);
    if (result != CardAssetDeleteResult::SUCCESS && result != CardAssetDeleteResult::NOT_FOUND) {
      return CardAssetRestoreResult::ROLLBACK_FAILED;
    }
  }
  return clear_restore_session() ? CardAssetRestoreResult::SUCCESS
                                 : CardAssetRestoreResult::PERSISTENCE_FAILED;
}

CardAssetService *card_asset_service() { return active_card_asset_service; }

}  // namespace espcontrol
