#include "card_asset_service.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "esphome/core/hal.h"

#ifdef ESP_PLATFORM
#include <esp_random.h>
#endif

namespace espcontrol {
namespace {
CardAssetService *active_card_asset_service = nullptr;
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
  const size_t count = record.count & static_cast<uint8_t>(~RESTORE_SESSION_COMMITTED_FLAG);
  for (size_t index = 0; index < count && index < MAX_STAGED_RESTORE_ASSETS; ++index) {
    update(record.ids[index]);
  }
  return value;
}

uint32_t completed_checksum(const CompletedRestoresRecord &record) {
  uint32_t value = record.magic;
  for (const auto &session : record.sessions) {
    for (unsigned char ch : session) value = (value ^ ch) * 16777619UL;
  }
  return value;
}
}

bool CardAssetService::start() {
  StateLock lock(this);
  if (running_ || (active_card_asset_service != nullptr && active_card_asset_service != this)) {
    return false;
  }
  if (persistence_ == nullptr || !persistence_->ready()) return false;
  active_card_asset_service = this;
  running_ = true;
  load_pending_delete();
  load_restore_session();
  load_completed_restores();
  return true;
}

bool CardAssetService::stop() {
  StateLock lock(this);
  if (!running_) return false;
  clear_card_background_runtime();
  if (active_card_asset_service == this) active_card_asset_service = nullptr;
  running_ = false;
  return true;
}

void CardAssetService::loop() {
  StateLock lock(this);
  if (!running_ || delete_running_) return;
  const uint32_t now = esphome::millis();
  if (!restore_session_.empty() && !restore_recovery_needed_ && !restore_committed_ &&
      now - restore_session_last_activity_ms_ >= RESTORE_SESSION_IDLE_TIMEOUT_MS) {
    restore_recovery_needed_ = true;
  }
  if (pending_delete_id_.empty() && !restore_recovery_needed_) return;
#ifdef ESP_PLATFORM
  if (last_resume_attempt_ != 0 && now - last_resume_attempt_ < 5000) return;
  last_resume_attempt_ = now;
#endif
  if (!pending_delete_id_.empty()) resume_pending_delete();
  if (restore_recovery_needed_ && !delete_running_) {
    if (restore_committed_) finish_committed_restore();
    else recover_abandoned_restore_session();
  }
}

void CardAssetService::set_reference_adapter(CardAssetReferenceAdapter *adapter) {
  StateLock lock(this);
  reference_adapter_ = adapter;
  if (running_ && !pending_delete_id_.empty()) resume_pending_delete();
  if (running_ && restore_recovery_needed_ && !restore_session_.empty()) {
    if (restore_committed_) finish_committed_restore();
    else recover_abandoned_restore_session();
  }
}

void CardAssetService::set_reference_persistence_callback(
    ReferencePersistenceCallback callback, void *context) {
  StateLock lock(this);
  reference_persistence_callback_ = callback;
  reference_persistence_context_ = context;
  if (running_ && !pending_delete_id_.empty()) resume_pending_delete();
}

bool CardAssetService::load_pending_delete() {
  pending_delete_id_.clear();
  if (persistence_ == nullptr || !persistence_->ready()) return false;
  PendingDeleteRecord record{};
  if (!persistence_->load(record)) return true;
  record.id[sizeof(record.id) - 1] = '\0';
  if (record.magic != PENDING_DELETE_MAGIC || record.version != PENDING_DELETE_VERSION ||
      record.checksum != pending_checksum(record.id) ||
      !esphome::card_image_store::CardImageStore::id_valid(record.id)) {
    return true;
  }
  pending_delete_id_ = record.id;
  return true;
}

bool CardAssetService::save_pending_delete(const std::string &id) {
  if (!esphome::card_image_store::CardImageStore::id_valid(id)) return false;
  if (persistence_ == nullptr || !persistence_->ready()) return false;
  PendingDeleteRecord record{};
  std::strncpy(record.id, id.c_str(), sizeof(record.id) - 1);
  record.checksum = pending_checksum(record.id);
  if (!persistence_->save(record) || !persistence_->sync()) return false;
  pending_delete_id_ = id;
  return true;
}

bool CardAssetService::clear_pending_delete() {
  if (persistence_ == nullptr || !persistence_->ready()) return false;
  PendingDeleteRecord record{};
  record.magic = 0;
  record.version = 0;
  if (!persistence_->save(record) || !persistence_->sync()) return false;
  pending_delete_id_.clear();
  return true;
}

CardAssetDeleteResult CardAssetService::delete_with_references(const std::string &id) {
  StateLock lock(this);
  if (delete_running_) return CardAssetDeleteResult::BUSY;
  esphome::card_image_store::CardImageInfo image;
  if (!store_.find(id, image)) return CardAssetDeleteResult::NOT_FOUND;
  if (!pending_delete_id_.empty() && pending_delete_id_ != id) return CardAssetDeleteResult::BUSY;
  if (reference_adapter_ == nullptr || !reference_adapter_->ready()) {
    return CardAssetDeleteResult::REFERENCES_UNAVAILABLE;
  }
  if (pending_delete_id_.empty()) {
    const esp_err_t reserve_error = store_.reserve_erase(id);
    if (reserve_error != ESP_OK) {
      return reserve_error == ESP_ERR_INVALID_STATE ? CardAssetDeleteResult::BUSY
                                                    : CardAssetDeleteResult::STORAGE_FAILED;
    }
    if (!save_pending_delete(id)) {
      store_.cancel_erase(id);
      return CardAssetDeleteResult::PERSISTENCE_FAILED;
    }
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
  const esp_err_t reserve_error = store_.reserve_erase(id);
  if (reserve_error != ESP_OK) {
    delete_running_ = false;
    return reserve_error == ESP_ERR_INVALID_STATE ? CardAssetDeleteResult::BUSY
                                                  : CardAssetDeleteResult::STORAGE_FAILED;
  }
  if (!reference_adapter_->clear_asset_references(id) ||
      (reference_persistence_callback_ != nullptr &&
       !reference_persistence_callback_(reference_persistence_context_)) ||
      reference_adapter_->references_asset(id)) {
    store_.cancel_erase(id);
    delete_running_ = false;
    return CardAssetDeleteResult::PERSISTENCE_FAILED;
  }
  const esp_err_t error = store_.erase(id);
  if (error != ESP_OK) {
    store_.cancel_erase(id);
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
  restore_committed_ = false;
  restore_session_last_activity_ms_ = 0;
  if (persistence_ == nullptr || !persistence_->ready()) return false;
  auto record = std::make_unique<RestoreSessionRecord>();
  if (!persistence_->load(*record)) return true;
  record->session[sizeof(record->session) - 1] = '\0';
  for (auto &id : record->ids) id[sizeof(id) - 1] = '\0';
  const size_t count = record->count & static_cast<uint8_t>(~RESTORE_SESSION_COMMITTED_FLAG);
  if (record->magic != RESTORE_SESSION_MAGIC || record->version != RESTORE_SESSION_VERSION ||
      count > MAX_STAGED_RESTORE_ASSETS ||
      record->checksum != restore_checksum(*record) || record->session[0] == '\0') {
    return true;
  }
  for (size_t index = 0; index < count; ++index) {
    if (!esphome::card_image_store::CardImageStore::id_valid(record->ids[index])) return true;
  }
  restore_session_ = record->session;
  restore_committed_ = (record->count & RESTORE_SESSION_COMMITTED_FLAG) != 0;
  for (size_t index = 0; index < count; ++index) staged_restore_ids_.emplace_back(record->ids[index]);
  restore_recovery_needed_ = true;
  return true;
}

bool CardAssetService::save_restore_session() {
  if (persistence_ == nullptr || !persistence_->ready() || restore_session_.empty() ||
      staged_restore_ids_.size() > MAX_STAGED_RESTORE_ASSETS) {
    return false;
  }
  auto record = std::make_unique<RestoreSessionRecord>();
  record->count = static_cast<uint8_t>(staged_restore_ids_.size()) |
                  (restore_committed_ ? RESTORE_SESSION_COMMITTED_FLAG : 0);
  std::strncpy(record->session, restore_session_.c_str(), sizeof(record->session) - 1);
  for (size_t index = 0; index < staged_restore_ids_.size(); ++index) {
    std::strncpy(record->ids[index], staged_restore_ids_[index].c_str(),
                 sizeof(record->ids[index]) - 1);
  }
  record->checksum = restore_checksum(*record);
  if (!persistence_->save(*record) ||
      !persistence_->sync()) return false;
  return true;
}

bool CardAssetService::clear_restore_session() {
  if (persistence_ == nullptr || !persistence_->ready()) return false;
  auto record = std::make_unique<RestoreSessionRecord>();
  record->magic = 0;
  record->version = 0;
  if (!persistence_->save(*record) ||
      !persistence_->sync()) return false;
  restore_session_.clear();
  staged_restore_ids_.clear();
  restore_recovery_needed_ = false;
  restore_committed_ = false;
  restore_session_last_activity_ms_ = 0;
  return true;
}

std::string CardAssetService::begin_restore_session() {
  StateLock lock(this);
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
  restore_committed_ = false;
  restore_session_last_activity_ms_ = esphome::millis();
  if (!save_restore_session()) {
    restore_session_.clear();
    return "";
  }
  return restore_session_;
}

bool CardAssetService::stage_restored_asset(const std::string &session, const std::string &id) {
  StateLock lock(this);
  if (session.empty() || session != restore_session_ || restore_committed_ || restore_recovery_needed_ ||
      !esphome::card_image_store::CardImageStore::id_valid(id) ||
      staged_restore_ids_.size() >= MAX_STAGED_RESTORE_ASSETS) {
    return false;
  }
  if (std::find(staged_restore_ids_.begin(), staged_restore_ids_.end(), id) !=
      staged_restore_ids_.end()) {
    restore_session_last_activity_ms_ = esphome::millis();
    return true;
  }
  staged_restore_ids_.push_back(id);
  if (save_restore_session()) {
    restore_session_last_activity_ms_ = esphome::millis();
    return true;
  }
  staged_restore_ids_.pop_back();
  return false;
}

void CardAssetService::unstage_restored_asset(const std::string &session, const std::string &id) {
  StateLock lock(this);
  if (session != restore_session_) return;
  const auto item = std::find(staged_restore_ids_.begin(), staged_restore_ids_.end(), id);
  if (item == staged_restore_ids_.end()) return;
  const auto previous = staged_restore_ids_;
  staged_restore_ids_.erase(item);
  if (save_restore_session()) restore_session_last_activity_ms_ = esphome::millis();
  else staged_restore_ids_ = previous;
}

void CardAssetService::load_completed_restores() {
  completed_restore_sessions_.clear();
  CompletedRestoresRecord record{};
  if (!persistence_->load(record) || record.magic != COMPLETED_RESTORES_MAGIC ||
      record.checksum != completed_checksum(record)) return;
  for (const auto &session : record.sessions) {
    if (session[16] != '\0') return;
  }
  for (const auto &session : record.sessions) {
    if (session[0] != '\0') completed_restore_sessions_.emplace_back(session);
  }
}

bool CardAssetService::finish_committed_restore() {
  if (std::find(completed_restore_sessions_.begin(), completed_restore_sessions_.end(),
                restore_session_) == completed_restore_sessions_.end()) {
    auto completed = completed_restore_sessions_;
    completed.insert(completed.begin(), restore_session_);
    if (completed.size() > COMPLETED_RESTORE_CAPACITY) completed.resize(COMPLETED_RESTORE_CAPACITY);
    CompletedRestoresRecord record{};
    for (size_t i = 0; i < completed.size(); ++i)
      std::strncpy(record.sessions[i], completed[i].c_str(), sizeof(record.sessions[i]) - 1);
    record.checksum = completed_checksum(record);
    if (!persistence_->save(record) || !persistence_->sync()) return false;
    completed_restore_sessions_ = completed;
  }
  return clear_restore_session();
}

CardAssetRestoreResult CardAssetService::commit_restore_session(const std::string &session) {
  StateLock lock(this);
  if (!session.empty() && std::find(completed_restore_sessions_.begin(),
      completed_restore_sessions_.end(), session) != completed_restore_sessions_.end()) {
    return CardAssetRestoreResult::SUCCESS;
  }
  if (session.empty() || session != restore_session_) return CardAssetRestoreResult::INVALID_SESSION;
  if (!restore_committed_) {
    restore_committed_ = true;
    if (!save_restore_session()) {
      restore_committed_ = false;
      return CardAssetRestoreResult::PERSISTENCE_FAILED;
    }
  }
  // Persist a bounded receipt before clearing the deployed session journal.
  // If either operation fails, boot/loop recovery finishes this same commit.
  restore_recovery_needed_ = true;
  return finish_committed_restore() ? CardAssetRestoreResult::SUCCESS
                                    : CardAssetRestoreResult::PERSISTENCE_FAILED;
}

CardAssetRestoreResult CardAssetService::rollback_restore_session(const std::string &session) {
  StateLock lock(this);
  if (session.empty() || session != restore_session_) return CardAssetRestoreResult::INVALID_SESSION;
  if (restore_committed_) return CardAssetRestoreResult::INVALID_SESSION;
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

void CardAssetService::recover_abandoned_restore_session() {
  if (restore_session_.empty() || reference_adapter_ == nullptr ||
      !reference_adapter_->ready()) return;
  for (const auto &id : staged_restore_ids_) {
    // A referenced staged image means configuration persistence completed but
    // the commit response was lost. Preserve it rather than breaking the
    // durable configuration during timeout or reboot recovery.
    const auto reference = recovery_reference_callback_ != nullptr
        ? recovery_reference_callback_(recovery_reference_context_, id)
        : CardAssetReferenceState::USE_LEGACY;
    if (reference == CardAssetReferenceState::UNAVAILABLE) return;
    if (reference == CardAssetReferenceState::REFERENCED ||
        (reference == CardAssetReferenceState::USE_LEGACY &&
         reference_adapter_->references_asset(id))) continue;
    esphome::card_image_store::CardImageInfo image;
    if (!store_.find(id, image)) continue;
    if (store_.erase(id) != ESP_OK) return;
  }
  clear_restore_session();
}

CardAssetService *card_asset_service() { return active_card_asset_service; }

}  // namespace espcontrol
