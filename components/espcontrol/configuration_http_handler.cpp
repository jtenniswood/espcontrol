#include "configuration_http_handler.h"

#ifdef USE_WEBSERVER

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

#include "esp_http_server.h"

namespace espcontrol::configuration {
namespace {

constexpr char SNAPSHOT_PATH[] = "/espcontrol/configuration";
constexpr char BEGIN_PATH[] = "/espcontrol/configuration/begin";
constexpr char CHUNK_PATH[] = "/espcontrol/configuration/chunk";
constexpr char COMMIT_PATH[] = "/espcontrol/configuration/commit";

bool is_path(esphome::web_server_idf::AsyncWebServerRequest *request,
             const char *expected) {
  char buffer[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
  return request->url_to(buffer) == expected;
}

bool parse_unsigned(const std::string &value, uint64_t maximum,
                    uint64_t *output) {
  if (value.empty() || output == nullptr || value[0] == '-') return false;
  errno = 0;
  char *end = nullptr;
  const unsigned long long parsed = std::strtoull(value.c_str(), &end, 10);
  if (errno != 0 || end != value.c_str() + value.size() || parsed > maximum) {
    return false;
  }
  *output = parsed;
  return true;
}

int hex_nibble(char value) {
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'a' && value <= 'f') return value - 'a' + 10;
  if (value >= 'A' && value <= 'F') return value - 'A' + 10;
  return -1;
}

void send_json(esphome::web_server_idf::AsyncWebServerRequest *request,
               int status, const char *body) {
  const char *response_status = HTTPD_500;
  switch (status) {
    case 200:
      response_status = HTTPD_200;
      break;
    case 400:
      response_status = HTTPD_400;
      break;
    case 404:
      response_status = HTTPD_404;
      break;
    case 409:
      response_status = "409 Conflict";
      break;
    case 413:
      response_status = "413 Content Too Large";
      break;
    case 503:
      response_status = "503 Service Unavailable";
      break;
  }
  httpd_resp_set_status(*request, response_status);
  httpd_resp_set_type(*request, "application/json");
  httpd_resp_set_hdr(*request, "Cache-Control", "no-store");
  httpd_resp_send(*request, body, HTTPD_RESP_USE_STRLEN);
}

void send_upload_error(
    esphome::web_server_idf::AsyncWebServerRequest *request,
    UploadStatus status) {
  switch (status) {
    case UploadStatus::TOO_LARGE:
      send_json(request, 413, "{\"status\":\"too_large\"}");
      break;
    case UploadStatus::STALE_UPLOAD:
      send_json(request, 409, "{\"status\":\"stale_upload\"}");
      break;
    case UploadStatus::NO_ACTIVE_UPLOAD:
      send_json(request, 409, "{\"status\":\"no_active_upload\"}");
      break;
    case UploadStatus::OUT_OF_ORDER:
      send_json(request, 409, "{\"status\":\"out_of_order\"}");
      break;
    case UploadStatus::INCOMPLETE:
      send_json(request, 409, "{\"status\":\"incomplete\"}");
      break;
    default:
      send_json(request, 400, "{\"status\":\"invalid\"}");
      break;
  }
}

}  // namespace

bool ConfigurationHttpHandler::canHandle(
    esphome::web_server_idf::AsyncWebServerRequest *request) const {
  if (request->method() == HTTP_GET) return is_path(request, SNAPSHOT_PATH);
  if (request->method() != HTTP_POST) return false;
  return is_path(request, BEGIN_PATH) || is_path(request, CHUNK_PATH) ||
         is_path(request, COMMIT_PATH);
}

void ConfigurationHttpHandler::handleRequest(
    esphome::web_server_idf::AsyncWebServerRequest *request) {
  if (request->method() == HTTP_GET) {
    handle_snapshot(request);
  } else if (is_path(request, BEGIN_PATH)) {
    handle_begin(request);
  } else if (is_path(request, CHUNK_PATH)) {
    handle_chunk(request);
  } else {
    handle_commit(request);
  }
}

void ConfigurationHttpHandler::handle_snapshot(
    esphome::web_server_idf::AsyncWebServerRequest *request) {
  const DocumentSnapshot snapshot =
      api_.snapshot(snapshot_buffer_, snapshot_capacity_);
  if (!snapshot.ok()) {
    send_json(request,
              snapshot.status == DocumentApiStatus::EMPTY ? 404 : 503,
              snapshot.status == DocumentApiStatus::EMPTY
                  ? "{\"status\":\"empty\"}"
                  : "{\"status\":\"unavailable\"}");
    return;
  }

  char revision[16];
  char version[8];
  std::snprintf(revision, sizeof(revision), "%u",
                static_cast<unsigned>(snapshot.revision));
  std::snprintf(version, sizeof(version), "%u",
                static_cast<unsigned>(snapshot.document_version));
  httpd_resp_set_type(*request, "application/octet-stream");
  httpd_resp_set_hdr(*request, "Cache-Control", "no-store");
  httpd_resp_set_hdr(*request, "X-EspControl-Revision", revision);
  httpd_resp_set_hdr(*request, "X-EspControl-Document-Version", version);
  httpd_resp_send(*request,
                  reinterpret_cast<const char *>(snapshot_buffer_),
                  snapshot.document_size);
}

void ConfigurationHttpHandler::handle_begin(
    esphome::web_server_idf::AsyncWebServerRequest *request) {
  uint64_t transaction = 0;
  uint64_t revision = 0;
  uint64_t version = 0;
  uint64_t size = 0;
  if (!parse_unsigned(request->arg("transaction"),
                      std::numeric_limits<uint32_t>::max(), &transaction) ||
      !parse_unsigned(request->arg("expected_revision"),
                      std::numeric_limits<uint32_t>::max(), &revision) ||
      !parse_unsigned(request->arg("document_version"),
                      std::numeric_limits<uint16_t>::max(), &version) ||
      !parse_unsigned(request->arg("size"), api_.maximum_document_size(),
                      &size)) {
    send_upload_error(request, UploadStatus::INVALID_ARGUMENT);
    return;
  }
  const UploadStatus status = upload_.begin(
      static_cast<uint32_t>(transaction), static_cast<uint32_t>(revision),
      static_cast<uint16_t>(version), static_cast<size_t>(size));
  if (status != UploadStatus::OK) {
    send_upload_error(request, status);
    return;
  }
  send_json(request, 200, "{\"status\":\"ready\"}");
}

void ConfigurationHttpHandler::handle_chunk(
    esphome::web_server_idf::AsyncWebServerRequest *request) {
  uint64_t transaction = 0;
  uint64_t offset = 0;
  const std::string encoded = request->arg("data");
  if (!parse_unsigned(request->arg("transaction"),
                      std::numeric_limits<uint32_t>::max(), &transaction) ||
      !parse_unsigned(request->arg("offset"),
                      api_.maximum_document_size(), &offset) ||
      encoded.size() % 2 != 0 ||
      encoded.size() > sizeof(decoded_chunk_) * 2) {
    send_upload_error(request, UploadStatus::INVALID_ARGUMENT);
    return;
  }

  const size_t decoded_size = encoded.size() / 2;
  for (size_t index = 0; index < decoded_size; ++index) {
    const int high = hex_nibble(encoded[index * 2]);
    const int low = hex_nibble(encoded[index * 2 + 1]);
    if (high < 0 || low < 0) {
      send_upload_error(request, UploadStatus::INVALID_ARGUMENT);
      return;
    }
    decoded_chunk_[index] = static_cast<uint8_t>((high << 4) | low);
  }

  const UploadStatus status = upload_.append(
      static_cast<uint32_t>(transaction), static_cast<size_t>(offset),
      decoded_chunk_, decoded_size);
  if (status != UploadStatus::OK) {
    send_upload_error(request, status);
    return;
  }
  httpd_resp_set_status(*request, HTTPD_204);
  httpd_resp_set_hdr(*request, "Cache-Control", "no-store");
  httpd_resp_send(*request, nullptr, 0);
}

void ConfigurationHttpHandler::handle_commit(
    esphome::web_server_idf::AsyncWebServerRequest *request) {
  uint64_t transaction = 0;
  if (!parse_unsigned(request->arg("transaction"),
                      std::numeric_limits<uint32_t>::max(), &transaction)) {
    send_upload_error(request, UploadStatus::INVALID_ARGUMENT);
    return;
  }
  const uint32_t transaction_id = static_cast<uint32_t>(transaction);
  const UploadStatus upload_status = upload_.inspect(transaction_id);
  if (upload_status != UploadStatus::OK) {
    send_upload_error(request, upload_status);
    return;
  }
  if (!validator_.validate(upload_.document_version(), upload_.document(),
                           upload_.document_size())) {
    upload_.finish(transaction_id);
    send_json(request, 400, "{\"status\":\"invalid_document\"}");
    return;
  }

  const DocumentSave saved = api_.replace(
      upload_.expected_revision(), upload_.document_version(),
      upload_.document(), upload_.document_size());
  upload_.finish(transaction_id);
  char response[128];
  std::snprintf(response, sizeof(response),
                "{\"status\":\"%s\",\"revision\":%u}",
                saved.ok() ? "ok" :
                saved.status == DocumentApiStatus::CONFLICT ? "conflict" :
                                                             "unavailable",
                static_cast<unsigned>(saved.revision));
  if (saved.ok()) {
    committed_revision_.store(saved.revision);
    send_json(request, 200, response);
  } else if (saved.status == DocumentApiStatus::CONFLICT) {
    send_json(request, 409, response);
  } else if (saved.status == DocumentApiStatus::TOO_LARGE) {
    send_json(request, 413, response);
  } else if (saved.status == DocumentApiStatus::INVALID) {
    send_json(request, 400, response);
  } else {
    send_json(request, 503, response);
  }
}

}  // namespace espcontrol::configuration

#endif  // USE_WEBSERVER
