#pragma once

#include <algorithm>
#include <cstddef>

#include "configuration_service.h"
#include "panel_config_http_context.h"

namespace espcontrol::configuration {

// The wire representation may be larger than the selected durable slot.
// Exclude both storage headers and respect a smaller borrowed HTTP workspace.
inline size_t panel_config_writable_document_bytes() {
  const auto &context = panel_config_http_context();
  if (!panel_config_http_context_ready() || context.service == nullptr) return 0;
  return std::min(context.document_capacity, context.service->maximum_document_size());
}

}  // namespace espcontrol::configuration
