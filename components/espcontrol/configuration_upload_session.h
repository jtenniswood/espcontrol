#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace espcontrol::configuration {

enum class UploadStatus : uint8_t {
  OK,
  INVALID_ARGUMENT,
  TOO_LARGE,
  NO_ACTIVE_UPLOAD,
  STALE_UPLOAD,
  OUT_OF_ORDER,
  INCOMPLETE,
};

// Owns one bounded, sequential candidate upload. A newer begin() replaces any
// incomplete candidate, which naturally coalesces rapid browser edits without
// retaining stale revisions or chunk queues.
class ConfigurationUploadSession {
 public:
  ConfigurationUploadSession(uint8_t *buffer, size_t capacity)
      : buffer_(buffer), capacity_(capacity) {}

  UploadStatus begin(uint32_t transaction, uint32_t expected_revision,
                     uint16_t document_version, size_t document_size) {
    if (transaction == 0 || (capacity_ > 0 && buffer_ == nullptr)) {
      return UploadStatus::INVALID_ARGUMENT;
    }
    if (document_size > capacity_) return UploadStatus::TOO_LARGE;
    transaction_ = transaction;
    expected_revision_ = expected_revision;
    document_version_ = document_version;
    document_size_ = document_size;
    received_size_ = 0;
    active_ = true;
    return UploadStatus::OK;
  }

  UploadStatus append(uint32_t transaction, size_t offset,
                      const uint8_t *data, size_t size) {
    if (!active_) return UploadStatus::NO_ACTIVE_UPLOAD;
    if (transaction != transaction_) return UploadStatus::STALE_UPLOAD;
    if (offset != received_size_) return UploadStatus::OUT_OF_ORDER;
    if (size > document_size_ - received_size_ ||
        (size > 0 && data == nullptr)) {
      return UploadStatus::INVALID_ARGUMENT;
    }
    if (size > 0) std::memcpy(buffer_ + received_size_, data, size);
    received_size_ += size;
    return UploadStatus::OK;
  }

  UploadStatus inspect(uint32_t transaction) const {
    if (!active_) return UploadStatus::NO_ACTIVE_UPLOAD;
    if (transaction != transaction_) return UploadStatus::STALE_UPLOAD;
    return received_size_ == document_size_ ? UploadStatus::OK
                                            : UploadStatus::INCOMPLETE;
  }

  void finish(uint32_t transaction) {
    if (active_ && transaction == transaction_) active_ = false;
  }

  const uint8_t *document() const { return buffer_; }
  size_t document_size() const { return document_size_; }
  size_t received_size() const { return received_size_; }
  uint32_t transaction() const { return transaction_; }
  uint32_t expected_revision() const { return expected_revision_; }
  uint16_t document_version() const { return document_version_; }
  bool active() const { return active_; }

 private:
  uint8_t *buffer_{nullptr};
  size_t capacity_{0};
  uint32_t transaction_{0};
  uint32_t expected_revision_{0};
  uint16_t document_version_{0};
  size_t document_size_{0};
  size_t received_size_{0};
  bool active_{false};
};

}  // namespace espcontrol::configuration
