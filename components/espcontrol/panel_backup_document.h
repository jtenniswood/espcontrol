#pragma once

#include "configuration_buffer.h"
#include "panel_backup.h"
#include "panel_graph_backup.h"

namespace espcontrol::backup {
enum class PanelBackupDocumentStatus { OK, UNSUPPORTED_VERSION, INVALID_DOCUMENT, ALLOCATION_FAILED };

// Request-local ownership of graph validation scratch. Preflight is complete
// before HTTP headers; both formats retain their original native bytes.
class PanelBackupDocumentView {
 public:
  PanelBackupDocumentView(const uint8_t* document, size_t size,
      configuration::ConfigurationBuffer::Allocate allocate = configuration::ConfigurationBuffer::allocate_host,
      configuration::ConfigurationBuffer::Release release = configuration::ConfigurationBuffer::release_host)
      : document_(document), size_(size), graph_(document, size), workspace_(allocate, release) {}

  PanelBackupDocumentStatus prepare(uint16_t version, size_t slots, std::string_view expected_profile) {
    using namespace configuration;
    ready_ = false;
    workspace_.reset();
    if (version != 1 && version != 2) return PanelBackupDocumentStatus::UNSUPPORTED_VERSION;
    if (!slots || slots > PANEL_CONFIG_MAX_SLOT_COUNT || expected_profile.empty())
      return PanelBackupDocumentStatus::INVALID_DOCUMENT;
    if (version == 1) {
      if (!valid_backup_document(document_, size_, slots)) return PanelBackupDocumentStatus::INVALID_DOCUMENT;
      PanelConfigReader reader(document_, size_);
      PanelConfigRecord record;
      bool matched = false;
      reader.begin();
      while (reader.next(&record) == PanelConfigStatus::OK)
        if (record.type == PanelConfigRecordType::DEVICE_PROFILE)
          matched = std::string_view(reinterpret_cast<const char*>(record.value), record.value_size) == expected_profile;
      if (!matched) return PanelBackupDocumentStatus::INVALID_DOCUMENT;
    } else {
      size_t bytes = 0;
      if (graph_.workspace_bytes(&bytes) != PanelConfigStatus::OK)
        return PanelBackupDocumentStatus::INVALID_DOCUMENT;
      auto* scratch = workspace_.allocate(bytes);
      if (!scratch) return PanelBackupDocumentStatus::ALLOCATION_FAILED;
      if (graph_.prepare(scratch, bytes, slots, expected_profile) != PanelConfigStatus::OK) {
        workspace_.reset();
        return PanelBackupDocumentStatus::INVALID_DOCUMENT;
      }
    }
    version_ = version;
    slots_ = slots;
    ready_ = true;
    return PanelBackupDocumentStatus::OK;
  }

  template<typename Sink>
  bool write(const std::string& settings, const std::string& screen,
             const std::string& identity, const std::string& exported_at, Sink& sink) const {
    if (!ready_) return false;
    return version_ == 2 ? graph_.write(settings, screen, identity, exported_at, sink)
        : write_backup(document_, size_, slots_, settings, screen, identity, exported_at, sink);
  }

 private:
  const uint8_t* document_;
  size_t size_;
  PanelGraphBackupView graph_;
  configuration::ConfigurationBuffer workspace_;
  uint16_t version_{0};
  size_t slots_{0};
  bool ready_{false};
};
}  // namespace espcontrol::backup
