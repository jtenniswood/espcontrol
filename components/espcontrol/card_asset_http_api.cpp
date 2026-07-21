#ifdef USE_ESP32

#include "card_asset_http_api.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "card_asset_service.h"
#include "../web_server_idf/utils.h"
#include "../web_server_idf/web_server_idf.h"
#include "esphome/core/log.h"

namespace espcontrol::card_asset_http {
namespace {

using esphome::card_image_store::CARD_IMAGE_FORMAT_VERSION;
using esphome::card_image_store::CARD_IMAGE_MAX_BYTES;
using esphome::card_image_store::CardImageInfo;
using esphome::card_image_store::CardImageStore;
using esphome::card_image_store::CardImageUpload;
using esphome::web_server_idf::AsyncWebServerRequest;
using esphome::web_server_idf::query_key_value;
using esphome::web_server_idf::request_get_header;
using esphome::web_server_idf::strcasestr_n;

static const char *const TAG = "card_asset_http";
static constexpr const char *API_PREFIX = "/api/card-images/";

#ifndef HTTPD_409
#define HTTPD_409 "409 Conflict"
#endif
#ifndef HTTPD_503
#define HTTPD_503 "503 Service Unavailable"
#endif

void append_json_string(std::string &out, const char *value) {
  out.push_back('"');
  for (const char *p = value; p != nullptr && *p != '\0'; ++p) {
    switch (*p) {
      case '\\':
      case '"':
        out.push_back('\\');
        out.push_back(*p);
        break;
      case '\n': out.append("\\n"); break;
      case '\r': out.append("\\r"); break;
      case '\t': out.append("\\t"); break;
      default: out.push_back(*p); break;
    }
  }
  out.push_back('"');
}

void send_unavailable(httpd_req_t *request) {
  httpd_resp_set_status(request, HTTPD_503);
  httpd_resp_set_type(request, "text/plain");
  httpd_resp_sendstr(request, "Card image service is unavailable");
}

bool id_valid(const std::string &id) { return CardImageStore::id_valid(id); }

std::string id_from_url(const std::string &url, const char *prefix) {
  std::string rest = url.substr(strlen(prefix));
  if (rest.size() > 4 && rest.compare(rest.size() - 4, 4, ".jpg") == 0) {
    rest.resize(rest.size() - 4);
  }
  return id_valid(rest) ? rest : "";
}

std::string item_json(const CardImageInfo &image) {
  std::string body = "{\"id\":";
  append_json_string(body, image.id.c_str());
  body += ",\"name\":";
  append_json_string(body, image.name.c_str());
  body += ",\"size\":" + std::to_string(image.size);
  body += ",\"url\":\"/card-images/" + image.id + ".jpg\"}";
  return body;
}

std::string list_json() {
  auto *assets = card_asset_service();
  const bool available = assets != nullptr && assets->available();
  const size_t storage_bytes = assets != nullptr ? assets->capacity() : 0;
  const size_t used_bytes = assets != nullptr ? assets->used_bytes() : 0;
  const size_t free_bytes = assets != nullptr ? assets->free_bytes() : 0;
  std::string out = "{\"available\":";
  out += available ? "true" : "false";
  out += ",\"requires_usb_flash\":";
  out += available ? "false" : "true";
  out += ",\"format_version\":" + std::to_string(CARD_IMAGE_FORMAT_VERSION);
  out += ",\"reference_transactions\":true";
  out += ",\"max_active_backgrounds\":";
#ifdef ESPCONTROL_MAX_GRID_SLOTS
  out += std::to_string(ESPCONTROL_MAX_GRID_SLOTS);
#else
  out += "0";
#endif
  out += ",\"storage_bytes\":" + std::to_string(storage_bytes);
  out += ",\"used_bytes\":" + std::to_string(used_bytes);
  out += ",\"free_bytes\":" + std::to_string(free_bytes);
  out += ",\"max_bytes\":" + std::to_string(CARD_IMAGE_MAX_BYTES);
  out += ",\"images\":[";
  const auto images = assets != nullptr ? assets->list() : std::vector<CardImageInfo>{};
  bool first = true;
  for (const auto &image : images) {
    if (!first) out += ',';
    first = false;
    out += item_json(image);
  }
  out += "]}";
  return out;
}

bool handle_get(AsyncWebServerRequest *request) {
  if (request->method() != HTTP_GET) return false;
  char url_buffer[AsyncWebServerRequest::URL_BUF_SIZE];
  std::string url(request->url_to(url_buffer));
  if (url == "/api/card-images") {
    const std::string body = list_json();
    request->send(200, "application/json", body.c_str());
    return true;
  }
  if (url.rfind("/card-images/", 0) != 0) return false;
  const std::string id = id_from_url(url, "/card-images/");
  if (id.empty()) {
    request->send(404, "text/plain", "Not found");
    return true;
  }
  auto *assets = card_asset_service();
  if (assets == nullptr) {
    request->send(503, "text/plain", "Card image service is unavailable");
    return true;
  }
  CardImageInfo image;
  if (!assets->find(id, image)) {
    request->send(404, "text/plain", "Not found");
    return true;
  }
  auto reader = assets->open(id);
  if (!reader) {
    request->send(500, "text/plain", "Image read failed");
    return true;
  }
  httpd_req_t *raw = *request;
  httpd_resp_set_status(raw, HTTPD_200);
  httpd_resp_set_type(raw, "image/jpeg");
  httpd_resp_set_hdr(raw, "Cache-Control", "no-store");
  auto buffer = std::make_unique<char[]>(1024);
  size_t remaining = image.size;
  while (remaining > 0) {
    const size_t chunk = std::min<size_t>(remaining, 1024);
    const int count = reader->read(reinterpret_cast<uint8_t *>(buffer.get()), chunk);
    if (count <= 0) {
      httpd_resp_send_chunk(raw, nullptr, 0);
      return true;
    }
    if (httpd_resp_send_chunk(raw, buffer.get(), static_cast<size_t>(count)) != ESP_OK) return true;
    remaining -= static_cast<size_t>(count);
  }
  reader->end();
  httpd_resp_send_chunk(raw, nullptr, 0);
  return true;
}

bool handle_delete(AsyncWebServerRequest *request) {
  if (request->method() != HTTP_DELETE) return false;
  char url_buffer[AsyncWebServerRequest::URL_BUF_SIZE];
  const std::string url(request->url_to(url_buffer));
  if (url.rfind(API_PREFIX, 0) != 0) return false;
  const std::string id = id_from_url(url, API_PREFIX);
  if (id.empty()) {
    request->send(404, "text/plain", "Not found");
    return true;
  }
  auto *assets = card_asset_service();
  if (assets == nullptr) {
    request->send(503, "text/plain", "Card image service is unavailable");
    return true;
  }
  if (!assets->available()) {
    request->send(503, "text/plain",
                  "Card image storage is unavailable. Reflash this device over USB once to install it.");
    return true;
  }
  const CardAssetDeleteResult result = assets->delete_with_references(id);
  switch (result) {
    case CardAssetDeleteResult::SUCCESS:
      request->send(200, "application/json", "{\"ok\":true}");
      break;
    case CardAssetDeleteResult::NOT_FOUND:
      request->send(404, "text/plain", "Not found");
      break;
    case CardAssetDeleteResult::BUSY:
      request->send(409, "text/plain", "Image deletion is already in progress; try again");
      break;
    case CardAssetDeleteResult::REFERENCES_UNAVAILABLE:
      request->send(503, "text/plain", "Card settings are still starting; try again");
      break;
    case CardAssetDeleteResult::PERSISTENCE_FAILED:
      request->send(500, "text/plain",
                    "Card changes could not be saved, so the image was not deleted.");
      break;
    case CardAssetDeleteResult::STORAGE_FAILED:
      request->send(500, "text/plain", "Image could not be deleted.");
      break;
  }
  return true;
}

esp_err_t handle_upload(httpd_req_t *request) {
  auto content_type = request_get_header(request, "Content-Type");
  if (!content_type.has_value() ||
      strcasestr_n(content_type.value().c_str(), content_type.value().size(), "image/jpeg") == nullptr) {
    httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "JPEG required");
    return ESP_OK;
  }
  if (request->content_len == 0 || request->content_len > CARD_IMAGE_MAX_BYTES) {
    httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Image too large");
    return ESP_OK;
  }
  auto *assets = card_asset_service();
  if (assets == nullptr) {
    send_unavailable(request);
    return ESP_OK;
  }
  if (!assets->available()) {
    httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Card image storage is unavailable. Reflash this device over USB once to install the image storage partition.");
    return ESP_OK;
  }
  CardImageUpload upload;
  esp_err_t error = assets->begin_upload(request->content_len, upload);
  if (error == ESP_ERR_NO_MEM) {
    httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Not enough image storage space");
    return ESP_OK;
  }
  if (error != ESP_OK) {
    httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not start image upload");
    return ESP_OK;
  }
  auto buffer = std::make_unique<char[]>(1024);
  for (size_t remaining = request->content_len; remaining > 0;) {
    const size_t wanted = std::min<size_t>(remaining, 1024);
    const int received = httpd_req_recv(request, buffer.get(), wanted);
    if (received <= 0) {
      assets->abort_upload(upload);
      httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Upload failed");
      return ESP_OK;
    }
    error = assets->write_upload(upload, reinterpret_cast<const uint8_t *>(buffer.get()),
                                 static_cast<size_t>(received));
    if (error != ESP_OK) {
      assets->abort_upload(upload);
      ESP_LOGE(TAG, "Failed to write card image chunk: %s", esp_err_to_name(error));
      httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Upload failed");
      return ESP_OK;
    }
    remaining -= static_cast<size_t>(received);
  }
  CardImageInfo image;
  error = assets->commit_upload(upload, image);
  if (error != ESP_OK) {
    assets->abort_upload(upload);
    ESP_LOGE(TAG, "Failed to commit card image: %s", esp_err_to_name(error));
    httpd_resp_send_err(request,
                        error == ESP_ERR_INVALID_ARG ? HTTPD_400_BAD_REQUEST : HTTPD_500_INTERNAL_SERVER_ERROR,
                        error == ESP_ERR_INVALID_ARG ? "Invalid JPEG" : "Upload failed");
    return ESP_OK;
  }
  const std::string body = item_json(image);
  httpd_resp_set_type(request, "application/json");
  httpd_resp_sendstr(request, body.c_str());
  return ESP_OK;
}

esp_err_t handle_rename(httpd_req_t *request) {
  static constexpr const char *suffix = "/rename";
  std::string url(request->uri);
  const char *query = strchr(url.c_str(), '?');
  if (query != nullptr) url.resize(static_cast<size_t>(query - url.c_str()));
  if (url.rfind(API_PREFIX, 0) != 0 || url.size() <= strlen(API_PREFIX) + strlen(suffix) ||
      url.compare(url.size() - strlen(suffix), strlen(suffix), suffix) != 0) {
    return ESP_ERR_NOT_FOUND;
  }
  const std::string id = url.substr(strlen(API_PREFIX),
                                    url.size() - strlen(API_PREFIX) - strlen(suffix));
  if (!id_valid(id)) {
    httpd_resp_send_err(request, HTTPD_404_NOT_FOUND, "Not found");
    return ESP_OK;
  }
  if (request->content_len > 256) {
    httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Name too long");
    return ESP_OK;
  }
  std::string body(request->content_len, '\0');
  size_t received = 0;
  while (received < request->content_len) {
    const int count = httpd_req_recv(request, &body[received], request->content_len - received);
    if (count <= 0) {
      httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Rename failed");
      return ESP_OK;
    }
    received += static_cast<size_t>(count);
  }
  const auto parsed_name = query_key_value(body.c_str(), body.size(), "name");
  const std::string name = CardImageStore::normalize_name(parsed_name.has_value() ? parsed_name.value() : "");
  auto *assets = card_asset_service();
  if (assets == nullptr) {
    send_unavailable(request);
    return ESP_OK;
  }
  CardImageInfo image;
  const esp_err_t error = assets->rename(id, name, image);
  if (error == ESP_ERR_NOT_FOUND) {
    httpd_resp_send_err(request, HTTPD_404_NOT_FOUND, "Not found");
  } else if (error == ESP_ERR_INVALID_STATE) {
    httpd_resp_set_status(request, HTTPD_409);
    httpd_resp_set_type(request, "text/plain");
    httpd_resp_sendstr(request, "Image is currently in use; try again");
  } else if (error != ESP_OK) {
    ESP_LOGE(TAG, "Failed to update card image name: %s", esp_err_to_name(error));
    httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Rename failed");
  } else {
    const std::string response = item_json(image);
    httpd_resp_set_type(request, "application/json");
    httpd_resp_sendstr(request, response.c_str());
  }
  return ESP_OK;
}

}  // namespace

bool is_shortcut_request(AsyncWebServerRequest *request) {
  if (request->method() != HTTP_GET && request->method() != HTTP_DELETE) return false;
  char url_buffer[AsyncWebServerRequest::URL_BUF_SIZE];
  const std::string url(request->url_to(url_buffer));
  if (request->method() == HTTP_GET) {
    return url == "/api/card-images" || url.rfind(API_PREFIX, 0) == 0 ||
           url.rfind("/card-images/", 0) == 0;
  }
  return url.rfind(API_PREFIX, 0) == 0;
}

bool handle_request(AsyncWebServerRequest *request) {
  return handle_get(request) || handle_delete(request);
}

esp_err_t handle_post(httpd_req_t *request) {
  if (strcmp(request->uri, "/api/card-images") == 0) return handle_upload(request);
  if (strncmp(request->uri, API_PREFIX, strlen(API_PREFIX)) == 0) return handle_rename(request);
  return ESP_ERR_NOT_FOUND;
}

}  // namespace espcontrol::card_asset_http

#endif
