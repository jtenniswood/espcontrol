#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../card_image_store/card_image_store.h"

#ifdef ESP_PLATFORM
#include "esphome/core/preferences.h"
#endif

namespace espcontrol {

class CardAssetReferenceAdapter {
 public:
  virtual ~CardAssetReferenceAdapter() = default;
  virtual bool ready() const = 0;
  virtual bool clear_asset_references(const std::string &asset_id) = 0;
  virtual bool references_asset(const std::string &asset_id) const = 0;
};

enum class CardAssetDeleteResult {
  SUCCESS,
  NOT_FOUND,
  BUSY,
  REFERENCES_UNAVAILABLE,
  PERSISTENCE_FAILED,
  STORAGE_FAILED,
};

enum class CardAssetRestoreResult {
  SUCCESS,
  INVALID_SESSION,
  PERSISTENCE_FAILED,
  ROLLBACK_FAILED,
};

// Application-owned boundary for card-image persistence. HTTP and rendering
// adapters depend on this service rather than constructing or locating their
// own store.
class CardAssetService {
 public:
  ~CardAssetService() = default;

  bool start();
  bool stop();
  void loop();
  bool running() const { return running_; }

  void set_reference_adapter(CardAssetReferenceAdapter *adapter);
  bool supports_reference_transactions() const {
    return reference_adapter_ != nullptr && reference_adapter_->ready();
  }
  CardAssetDeleteResult delete_with_references(const std::string &id);
  std::string begin_restore_session();
  bool stage_restored_asset(const std::string &session, const std::string &id);
  void unstage_restored_asset(const std::string &session, const std::string &id);
  CardAssetRestoreResult commit_restore_session(const std::string &session);
  CardAssetRestoreResult rollback_restore_session(const std::string &session);

  bool available() { return store_.available(); }
  size_t capacity() { return store_.capacity(); }
  size_t used_bytes() { return store_.used_bytes(); }
  size_t free_bytes() { return store_.free_bytes(); }
  std::vector<esphome::card_image_store::CardImageInfo> list() { return store_.list(); }
  bool find(const std::string &id, esphome::card_image_store::CardImageInfo &out) {
    return store_.find(id, out);
  }

  esp_err_t begin_upload(size_t size, esphome::card_image_store::CardImageUpload &upload) {
    return store_.begin_upload(size, upload);
  }
  esp_err_t write_upload(esphome::card_image_store::CardImageUpload &upload,
                         const uint8_t *data, size_t size) {
    return store_.write_upload(upload, data, size);
  }
  esp_err_t commit_upload(esphome::card_image_store::CardImageUpload &upload,
                          esphome::card_image_store::CardImageInfo &out) {
    return store_.commit_upload(upload, out);
  }
  void abort_upload(esphome::card_image_store::CardImageUpload &upload) {
    store_.abort_upload(upload);
  }
  esp_err_t rename(const std::string &id, const std::string &name,
                   esphome::card_image_store::CardImageInfo &out) {
    return store_.rename(id, name, out);
  }
  esp_err_t erase(const std::string &id) { return store_.erase(id); }

  bool read_rgb565_cache(const std::string &id, uint32_t source_crc32,
                         uint16_t width, uint16_t height, uint8_t *buffer,
                         size_t size) {
    return store_.read_rgb565_cache(id, source_crc32, width, height, buffer, size);
  }
  esp_err_t write_rgb565_cache(const std::string &id, uint32_t source_crc32,
                               uint16_t width, uint16_t height,
                               const uint8_t *buffer, size_t size) {
    return store_.write_rgb565_cache(id, source_crc32, width, height, buffer, size);
  }
  std::shared_ptr<esphome::http_request::HttpContainer> open(const std::string &id) {
    return store_.open(id);
  }

  template<typename Runtime>
  Runtime &ensure_card_background_runtime(void (*shutdown)(Runtime &) = nullptr) {
    if (!card_background_runtime_) {
      card_background_runtime_ = std::make_unique<RuntimeHolder<Runtime>>(shutdown);
    }
    return static_cast<RuntimeHolder<Runtime> *>(card_background_runtime_.get())->state;
  }

  void clear_card_background_runtime() {
    if (card_background_runtime_) card_background_runtime_->shutdown();
    card_background_runtime_.reset();
  }

 private:
  struct RuntimeHolderBase {
    virtual ~RuntimeHolderBase() = default;
    virtual void shutdown() = 0;
  };

  template<typename Runtime> struct RuntimeHolder final : RuntimeHolderBase {
    explicit RuntimeHolder(void (*shutdown_callback)(Runtime &))
        : shutdown_callback(shutdown_callback) {}
    void shutdown() override {
      if (shutdown_callback != nullptr) shutdown_callback(state);
    }
    Runtime state{};
    void (*shutdown_callback)(Runtime &) = nullptr;
  };

  esphome::card_image_store::CardImageStore store_{};
  std::unique_ptr<RuntimeHolderBase> card_background_runtime_{};
  CardAssetReferenceAdapter *reference_adapter_{nullptr};
  std::string pending_delete_id_{};
  std::string restore_session_{};
  std::vector<std::string> staged_restore_ids_{};
  bool restore_recovery_needed_{false};
  bool delete_running_{false};
  bool running_{false};

  bool load_pending_delete();
  bool save_pending_delete(const std::string &id);
  bool clear_pending_delete();
  CardAssetDeleteResult resume_pending_delete();
  bool load_restore_session();
  bool save_restore_session();
  bool clear_restore_session();

#ifdef ESP_PLATFORM
  uint32_t last_resume_attempt_{0};
  esphome::ESPPreferenceObject pending_delete_preference_{};
  esphome::ESPPreferenceObject restore_session_preference_{};
#endif
};

// Adapters use this narrow registry to reach the service owned by
// EspControlApp. A null result means application setup has not completed.
CardAssetService *card_asset_service();

}  // namespace espcontrol
