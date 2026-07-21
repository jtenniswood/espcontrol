#pragma once

#ifdef USE_WEBSERVER

#include <cstddef>
#include <cstdint>

#include "configuration_document_api.h"
#include "configuration_entity_document.h"
#include "configuration_upload_session.h"
#include "esphome/components/web_server_idf/web_server_idf.h"

namespace espcontrol::configuration {

// Revisioned, chunked configuration transport. POST bodies stay below the
// ESP-IDF request-header limit and candidates are assembled sequentially into
// one fixed PSRAM buffer before validation and commit.
class ConfigurationHttpHandler final
    : public esphome::web_server_idf::AsyncWebHandler {
 public:
  static constexpr size_t MAX_CHUNK_SIZE = 384;

  ConfigurationHttpHandler(ConfigurationDocumentApi &api,
                           const EntityConfigurationAdapter &validator,
                           uint8_t *snapshot_buffer,
                           size_t snapshot_capacity,
                           uint8_t *upload_buffer, size_t upload_capacity)
      : api_(api),
        validator_(validator),
        snapshot_buffer_(snapshot_buffer),
        snapshot_capacity_(snapshot_capacity),
        upload_(upload_buffer, upload_capacity) {}

  bool canHandle(
      esphome::web_server_idf::AsyncWebServerRequest *request) const override;
  void handleRequest(
      esphome::web_server_idf::AsyncWebServerRequest *request) override;

 private:
  void handle_snapshot(
      esphome::web_server_idf::AsyncWebServerRequest *request);
  void handle_begin(esphome::web_server_idf::AsyncWebServerRequest *request);
  void handle_chunk(esphome::web_server_idf::AsyncWebServerRequest *request);
  void handle_commit(esphome::web_server_idf::AsyncWebServerRequest *request);

  ConfigurationDocumentApi &api_;
  const EntityConfigurationAdapter &validator_;
  uint8_t *snapshot_buffer_{nullptr};
  size_t snapshot_capacity_{0};
  ConfigurationUploadSession upload_;
  uint8_t decoded_chunk_[MAX_CHUNK_SIZE]{};
};

}  // namespace espcontrol::configuration

#endif  // USE_WEBSERVER
