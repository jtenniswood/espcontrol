#pragma once

#include <algorithm>
#include <cstring>

#include "configuration_buffer.h"
#include "panel_config_import.h"

namespace espcontrol::configuration {

// These are transient PSRAM ceilings, independent of the durable v1 slot size.
// A failed conversion never changes the stored document or its generation.
constexpr size_t PANEL_CONFIG_V2_HTTP_MAX_BYTES = 512 * 1024;
constexpr size_t PANEL_CONFIG_CONVERSION_MAX_WORKSPACE = 512 * 1024;
constexpr const char *PANEL_CONFIG_VERSION_HEADER = "X-Panel-Config-Version";

inline bool panel_config_requested_version(const char *header, uint16_t *version) {
  if (!version) return false;
  if (!header || std::strcmp(header, "1") == 0) { *version = 1; return true; }
  if (std::strcmp(header, "2") == 0) { *version = 2; return true; }
  return false;
}

enum class PanelConfigRepresentationStatus : uint8_t {
  OK, INVALID_DOCUMENT, NOT_REPRESENTABLE, TOO_LARGE, UNAVAILABLE,
};

// Own only the converted response/request until the synchronous HTTP operation
// finishes. Scratch is released before returning, including on every failure.
class PanelConfigRepresentation {
 public:
  PanelConfigRepresentation(ConfigurationBuffer::Allocate allocate,
                            ConfigurationBuffer::Release release)
      : output_(allocate, release), allocate_(allocate), release_(release) {}

  const uint8_t *data() const { return data_; }
  size_t size() const { return size_; }

  PanelConfigRepresentationStatus convert(uint16_t from, uint16_t to,
      const uint8_t *document, size_t size, size_t output_ceiling) {
    using S = PanelConfigRepresentationStatus;
    // A caller may reuse the owner, but must not borrow its prior output as
    // the next input: preserve that allocation rather than freeing live input.
    if (data_ && panel_config_v2_detail::overlap(data_, size_, document, size))
      return S::INVALID_DOCUMENT;
    output_.reset(); data_ = nullptr; size_ = 0;
    if (!document || !size || !output_ceiling || from == to ||
        (from != 1 && from != 2) || (to != 1 && to != 2))
      return S::INVALID_DOCUMENT;
    size_t output_bytes = std::min(output_ceiling, PANEL_CONFIG_V2_HTTP_MAX_BYTES), workspace_bytes = 0;
    if (from == 1) {
      PanelConfigImportBounds bounds;
      if (panel_config_v1_to_v2_bounds(document, size, &bounds) != PanelConfigImportStatus::OK)
        return S::INVALID_DOCUMENT;
      output_bytes = std::min({output_bytes, bounds.output_bytes, PANEL_CONFIG_V2_HTTP_MAX_BYTES});
      workspace_bytes = std::min(bounds.workspace_bytes, PANEL_CONFIG_CONVERSION_MAX_WORKSPACE);
    } else {
      if (size > PANEL_CONFIG_V2_HTTP_MAX_BYTES) return S::TOO_LARGE;
      if (panel_config_v2_to_v1_workspace_bytes(document, size, &workspace_bytes) != PanelConfigProjectionStatus::OK)
        return S::INVALID_DOCUMENT;
      if (workspace_bytes > PANEL_CONFIG_CONVERSION_MAX_WORKSPACE) return S::TOO_LARGE;
    }
    ConfigurationBuffer scratch(allocate_, release_);
    auto *workspace = scratch.allocate(workspace_bytes);
    if (!workspace) return S::UNAVAILABLE;
    // A dictionary-compressed v2 input can expand, but ordinary compatibility
    // reads should not reserve the entire 512 KiB ceiling. Grow private output
    // geometrically only when needed, releasing each failed attempt first.
    size_t attempt_bytes = from == 2 ? std::min(output_bytes, size) : output_bytes;
    while (true) {
      auto *output = output_.allocate(attempt_bytes);
      if (!output) return S::UNAVAILABLE;
      size_t written = 0;
      const auto status = from == 1
          ? panel_config_v1_to_v2(document, size, output, attempt_bytes, workspace, workspace_bytes, &written)
          : panel_config_v2_to_v1(document, size, output, attempt_bytes, workspace, workspace_bytes, &written);
      if (status == PanelConfigProjectionStatus::OK) {
        data_ = output; size_ = written; return S::OK;
      }
      output_.reset();
      if (status == PanelConfigProjectionStatus::BUFFER_TOO_SMALL && attempt_bytes < output_bytes) {
        attempt_bytes = std::min(output_bytes, attempt_bytes * 2);
        continue;
      }
      if (status == PanelConfigProjectionStatus::BUFFER_TOO_SMALL) return S::TOO_LARGE;
      if (status == PanelConfigProjectionStatus::NOT_REPRESENTABLE) return S::NOT_REPRESENTABLE;
      return S::INVALID_DOCUMENT;
    }
  }

 private:
  ConfigurationBuffer output_;
  ConfigurationBuffer::Allocate allocate_;
  ConfigurationBuffer::Release release_;
  const uint8_t *data_{nullptr};
  size_t size_{0};
};

}  // namespace espcontrol::configuration
