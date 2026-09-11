#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace espcontrol::configuration {

class ConfigurationService;

struct PanelConfigHttpContext {
  ConfigurationService *service{nullptr};
  uint8_t *document{nullptr};
  size_t document_capacity{0};
  const char *username{nullptr};
  const char *password{nullptr};
  std::atomic<bool> ready{false};
  std::atomic<bool> initialization_complete{false};
};

inline PanelConfigHttpContext &panel_config_http_context() {
  static PanelConfigHttpContext context;
  return context;
}

inline void bind_panel_config_http_context(ConfigurationService &service,
                                           uint8_t *document,
                                           size_t document_capacity,
                                           const char *username,
                                           const char *password) {
  PanelConfigHttpContext &context = panel_config_http_context();
  context.ready.store(false, std::memory_order_release);
  context.service = &service;
  context.document = document;
  context.document_capacity = document_capacity;
  context.username = username;
  context.password = password;
  context.ready.store(document != nullptr && document_capacity != 0,
                      std::memory_order_release);
}

inline bool panel_config_http_context_ready() {
  return panel_config_http_context().ready.load(std::memory_order_acquire);
}

// Capabilities must not report a permanent legacy fallback while the deferred
// native setup is still running. Once setup completes, an unavailable context
// intentionally advertises the normal legacy-only capability response.
inline void set_panel_config_http_context_initialization_complete(bool complete) {
  panel_config_http_context().initialization_complete.store(
      complete, std::memory_order_release);
}

inline bool panel_config_http_context_initialization_complete() {
  return panel_config_http_context().initialization_complete.load(
      std::memory_order_acquire);
}

}  // namespace espcontrol::configuration
