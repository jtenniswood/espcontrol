#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "configuration_service.h"

namespace espcontrol::configuration {

enum class DocumentApiStatus : uint8_t {
  OK,
  EMPTY,
  CONFLICT,
  INVALID,
  TOO_LARGE,
  UNAVAILABLE,
};

struct DocumentSnapshot {
  DocumentApiStatus status{DocumentApiStatus::EMPTY};
  uint32_t revision{0};
  uint16_t document_version{CURRENT_CONFIGURATION_DOCUMENT_VERSION};
  size_t document_size{0};

  bool ok() const { return status == DocumentApiStatus::OK; }
};

struct DocumentSave {
  DocumentApiStatus status{DocumentApiStatus::UNAVAILABLE};
  uint32_t revision{0};
  uint16_t document_version{CURRENT_CONFIGURATION_DOCUMENT_VERSION};
  size_t document_size{0};
  bool changed{false};

  bool ok() const { return status == DocumentApiStatus::OK; }
};

// Transport-neutral boundary used by the HTTP handler and firmware runtime.
// Documents are opaque here: parsing and CardGraph validation occur before a
// candidate revision is activated, while this class owns durable revisions.
class ConfigurationDocumentApi {
 public:
  explicit ConfigurationDocumentApi(ConfigurationService &service)
      : service_(service) {}

  DocumentSnapshot snapshot(uint8_t *output, size_t output_capacity) {
    if (busy_.test_and_set(std::memory_order_acquire)) {
      return {DocumentApiStatus::UNAVAILABLE};
    }
    const ServiceLoadResult loaded = service_.load(output, output_capacity);
    const DocumentSnapshot result{
        map_load_status(loaded.status), loaded.generation,
        loaded.document_version, loaded.document_size};
    busy_.clear(std::memory_order_release);
    return result;
  }

  DocumentSave replace(uint32_t expected_revision, uint16_t document_version,
                       const uint8_t *document, size_t document_size) {
    if (busy_.test_and_set(std::memory_order_acquire)) {
      return {DocumentApiStatus::UNAVAILABLE};
    }
    const ServiceSaveResult saved = service_.save_if_revision(
        expected_revision, document_version, document, document_size);
    const DocumentSave result{
        map_save_status(saved.status), saved.generation,
        saved.document_version, saved.document_size, saved.ok()};
    busy_.clear(std::memory_order_release);
    return result;
  }

  // Captures compatibility entities and folds them into the durable document
  // while holding the same non-blocking lock used by browser transactions.
  DocumentSave synchronize(LegacyConfigurationAdapter &source,
                           uint8_t *capture, size_t capture_capacity) {
    if (busy_.test_and_set(std::memory_order_acquire)) {
      return {DocumentApiStatus::UNAVAILABLE};
    }
    const LegacyLoadResult current = source.load(capture, capture_capacity);
    if (current.status != LegacyStatus::OK) {
      const DocumentApiStatus status =
          current.status == LegacyStatus::EMPTY
              ? DocumentApiStatus::EMPTY
              : current.status == LegacyStatus::BUFFER_TOO_SMALL
                    ? DocumentApiStatus::TOO_LARGE
                    : DocumentApiStatus::UNAVAILABLE;
      busy_.clear(std::memory_order_release);
      return {status, 0, current.document_version, current.document_size};
    }
    bool changed = false;
    const ServiceSaveResult saved = service_.save_current_if_changed(
        capture, current.document_size, &changed);
    const DocumentSave result{
        map_save_status(saved.status), saved.generation,
        saved.document_version, saved.document_size, changed};
    busy_.clear(std::memory_order_release);
    return result;
  }

  size_t maximum_document_size() const {
    return service_.maximum_document_size();
  }

 private:
  static DocumentApiStatus map_load_status(ServiceStatus status) {
    switch (status) {
      case ServiceStatus::OK:
      case ServiceStatus::IMPORTED_LEGACY:
        return DocumentApiStatus::OK;
      case ServiceStatus::EMPTY:
        return DocumentApiStatus::EMPTY;
      case ServiceStatus::INVALID_ARGUMENT:
      case ServiceStatus::UNSUPPORTED_VERSION:
      case ServiceStatus::INVALID_DOCUMENT:
        return DocumentApiStatus::INVALID;
      case ServiceStatus::BUFFER_TOO_SMALL:
        return DocumentApiStatus::TOO_LARGE;
      default:
        return DocumentApiStatus::UNAVAILABLE;
    }
  }

  static DocumentApiStatus map_save_status(ServiceStatus status) {
    switch (status) {
      case ServiceStatus::OK:
        return DocumentApiStatus::OK;
      case ServiceStatus::REVISION_CONFLICT:
        return DocumentApiStatus::CONFLICT;
      case ServiceStatus::INVALID_ARGUMENT:
      case ServiceStatus::UNSUPPORTED_VERSION:
      case ServiceStatus::INVALID_DOCUMENT:
        return DocumentApiStatus::INVALID;
      case ServiceStatus::BUFFER_TOO_SMALL:
        return DocumentApiStatus::TOO_LARGE;
      default:
        return DocumentApiStatus::UNAVAILABLE;
    }
  }

  ConfigurationService &service_;
  std::atomic_flag busy_ = ATOMIC_FLAG_INIT;
};

}  // namespace espcontrol::configuration
