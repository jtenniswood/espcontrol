#ifdef USE_ESP32

#include <array>
#include <cstdarg>
#include <memory>
#include <cstring>
#include <cctype>
#include <cinttypes>

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/defines.h"

#include "esp_tls_crypto.h"
#ifdef USE_WEBSERVER_AUTH_DIGEST
#include <esp_random.h>
#include <esp_rom_md5.h>
#endif
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "utils.h"
#include "web_server_idf.h"

#ifdef USE_WEBSERVER_OTA
#include <multipart_parser.h>
#include "multipart.h"  // For parse_multipart_boundary and other utils
#endif

#ifdef USE_WEBSERVER
#include "esphome/components/web_server/web_server.h"
#include "esphome/components/web_server/list_entities.h"
#endif  // USE_WEBSERVER

// Include socket headers after Arduino headers to avoid IPADDR_NONE/INADDR_NONE macro conflicts
#include <cerrno>
#include <sys/socket.h>

namespace esphome::web_server_idf {

#ifndef HTTPD_409
#define HTTPD_409 "409 Conflict"
#endif

#define CRLF_STR "\r\n"
#define CRLF_LEN (sizeof(CRLF_STR) - 1)

static const char *const TAG = "web_server_idf";

// Global instance to avoid guard variable (saves 8 bytes)
// This is initialized at program startup before any threads
namespace {
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
DefaultHeaders default_headers_instance;
}  // namespace

DefaultHeaders &DefaultHeaders::Instance() { return default_headers_instance; }

namespace {
#ifdef ESPHOME_PROJECT_NAME
static constexpr const char *ESPCONTROL_PROJECT_NAME = ESPHOME_PROJECT_NAME;
#else
static constexpr const char *ESPCONTROL_PROJECT_NAME = "";
#endif

#ifdef ESPHOME_PROJECT_VERSION
static constexpr const char *ESPCONTROL_PROJECT_VERSION = ESPHOME_PROJECT_VERSION;
#else
static constexpr const char *ESPCONTROL_PROJECT_VERSION = "";
#endif

#ifdef ESPCONTROL_DEVICE_SLUG
static constexpr const char *ESPCONTROL_DEVICE_PROFILE = ESPCONTROL_DEVICE_SLUG;
#else
static constexpr const char *ESPCONTROL_DEVICE_PROFILE = "";
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
      case '\n':
        out.append("\\n");
        break;
      case '\r':
        out.append("\\r");
        break;
      case '\t':
        out.append("\\t");
        break;
      default:
        out.push_back(*p);
        break;
    }
  }
  out.push_back('"');
}

std::string firmware_version_json() {
  std::string out;
  out.reserve(128);
  out.append("{\"project_name\":");
  append_json_string(out, ESPCONTROL_PROJECT_NAME);
  out.append(",\"project_version\":");
  append_json_string(out, ESPCONTROL_PROJECT_VERSION);
  out.append(",\"firmware_version\":");
  append_json_string(out, ESPCONTROL_PROJECT_VERSION);
  out.append(",\"device_slug\":");
  append_json_string(out, ESPCONTROL_DEVICE_PROFILE);
  out.append(",\"version\":");
  append_json_string(out, ESPCONTROL_PROJECT_VERSION);
  out.push_back('}');
  return out;
}

bool handle_firmware_version_request(AsyncWebServerRequest *request) {
  if (request->method() != HTTP_GET) {
    return false;
  }
  char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
  StringRef url = request->url_to(url_buf);
  if (url != "/espcontrol/version" && url != "/espcontrol/version.json") {
    return false;
  }
  std::string body = firmware_version_json();
  request->send(200, "application/json", body.c_str());
  return true;
}

void apply_no_cache_headers(httpd_req_t *req) {
  httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  httpd_resp_set_hdr(req, "Pragma", "no-cache");
  httpd_resp_set_hdr(req, "Expires", "0");
}

// Non-blocking send function to prevent watchdog timeouts when TCP buffers are full
/**
 * Sends data on a socket in non-blocking mode.
 *
 * @param hd      HTTP server handle (unused).
 * @param sockfd  Socket file descriptor.
 * @param buf     Buffer to send.
 * @param buf_len Length of buffer.
 * @param flags   Flags for send().
 * @return
 *   - Number of bytes sent on success.
 *   - HTTPD_SOCK_ERR_INVALID if buf is nullptr.
 *   - HTTPD_SOCK_ERR_TIMEOUT if the send buffer is full (EAGAIN/EWOULDBLOCK).
 *   - HTTPD_SOCK_ERR_FAIL for other errors.
 */
int nonblocking_send(httpd_handle_t hd, int sockfd, const char *buf, size_t buf_len, int flags) {
  if (buf == nullptr) {
    return HTTPD_SOCK_ERR_INVALID;
  }

  // Use MSG_DONTWAIT to prevent blocking when TCP send buffer is full
  int ret = send(sockfd, buf, buf_len, flags | MSG_DONTWAIT);
  if (ret < 0) {
    const int err = errno;
    if (err == EAGAIN || err == EWOULDBLOCK) {
      // Buffer full - retry later
      return HTTPD_SOCK_ERR_TIMEOUT;
    }
    // Real error
    ESP_LOGD(TAG, "send error: errno %d", err);
    return HTTPD_SOCK_ERR_FAIL;
  }
  return ret;
}
}  // namespace

void AsyncWebServer::safe_close_with_shutdown(httpd_handle_t hd, int sockfd) {
  // CRITICAL: Shut down receive BEFORE closing to prevent lwIP race conditions
  //
  // The race condition occurs because close() initiates lwIP teardown while
  // the TCP/IP thread can still receive packets, causing assertions when
  // recv_tcp() sees partially-torn-down state.
  //
  // By shutting down receive first, we tell lwIP to stop accepting new data BEFORE
  // the teardown begins, eliminating the race window. We only shutdown RD (not RDWR)
  // to allow the FIN packet to be sent cleanly during close().
  //
  // Note: This function may be called with an already-closed socket if the network
  // stack closed it. In that case, shutdown() will fail but close() is safe to call.
  //
  // See: https://github.com/esphome/esphome-webserver/issues/163

  // Attempt shutdown - ignore errors as socket may already be closed
  shutdown(sockfd, SHUT_RD);

  // Always close - safe even if socket is already closed by network stack
  close(sockfd);
}

void AsyncWebServer::end() {
  if (this->server_) {
    httpd_stop(this->server_);
    this->server_ = nullptr;
  }
}

void AsyncWebServer::begin() {
  if (this->server_) {
    this->end();
  }
  // The ESPControl web UI exposes many internal configuration entities. Larger
  // P4 panels can overflow the ESP-IDF default while serving entity details.
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.stack_size = 16384;
  // Keep browser bursts from opening several web sessions at once. The config
  // UI fetches details sequentially, so two client sockets are enough and leave
  // more internal heap available for LVGL/display work on P4 panels.
  config.max_open_sockets = 5;
  config.backlog_conn = 2;
  config.server_port = this->port_;
  config.uri_match_fn = [](const char * /*unused*/, const char * /*unused*/, size_t /*unused*/) { return true; };
  // Always enable LRU purging to handle socket exhaustion gracefully.
  // When max sockets is reached, the oldest connection is closed to make room for new ones.
  // This prevents "httpd_accept_conn: error in accept (23)" errors.
  // See: https://github.com/esphome/esphome/issues/12464
  config.lru_purge_enable = true;
  // Use custom close function that shuts down before closing to prevent lwIP race conditions
  config.close_fn = AsyncWebServer::safe_close_with_shutdown;
  if (httpd_start(&this->server_, &config) == ESP_OK) {
    global_async_web_server() = this;
    const httpd_uri_t handler_get = {
        .uri = "",
        .method = HTTP_GET,
        .handler = AsyncWebServer::request_handler,
        .user_ctx = this,
    };
    httpd_register_uri_handler(this->server_, &handler_get);

    const httpd_uri_t handler_post = {
        .uri = "",
        .method = HTTP_POST,
        .handler = AsyncWebServer::request_post_handler,
        .user_ctx = this,
    };
    httpd_register_uri_handler(this->server_, &handler_post);

    const httpd_uri_t handler_options = {
        .uri = "",
        .method = HTTP_OPTIONS,
        .handler = AsyncWebServer::request_handler,
        .user_ctx = this,
    };
    httpd_register_uri_handler(this->server_, &handler_options);
  }
}

esp_err_t AsyncWebServer::request_post_handler(httpd_req_t *r) {
  ESP_LOGVV(TAG, "Enter AsyncWebServer::request_post_handler. uri=%s", r->uri);
  auto content_type = request_get_header(r, "Content-Type");

  if (!request_has_header(r, "Content-Length")) {
    ESP_LOGW(TAG, "Content length is required for post: %s", r->uri);
    httpd_resp_send_err(r, HTTPD_411_LENGTH_REQUIRED, nullptr);
    return ESP_OK;
  }

  if (content_type.has_value()) {
    const char *content_type_char = content_type.value().c_str();

    // Check most common case first
    size_t content_type_len = strlen(content_type_char);
    if (strcasestr_n(content_type_char, content_type_len, "application/x-www-form-urlencoded") != nullptr) {
      // Normal form data - proceed with regular handling
#ifdef USE_WEBSERVER_OTA
    } else if (strcasestr_n(content_type_char, content_type_len, "multipart/form-data") != nullptr) {
      auto *server = static_cast<AsyncWebServer *>(r->user_ctx);
      return server->handle_multipart_upload_(r, content_type_char);
#endif
    } else {
      ESP_LOGW(TAG, "Unsupported content type for POST: %s", content_type_char);
      // fallback to get handler to support backward compatibility
      return AsyncWebServer::request_handler(r);
    }
  }

  // Handle regular form data
  if (r->content_len > CONFIG_HTTPD_MAX_REQ_HDR_LEN) {
    ESP_LOGW(TAG, "Request size is to big: %zu", r->content_len);
    httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, nullptr);
    return ESP_FAIL;
  }

  std::string post_query;
  if (r->content_len > 0) {
    post_query.resize(r->content_len);
    size_t received = 0;
    while (received < r->content_len) {
      const int ret = httpd_req_recv(r, &post_query[received], r->content_len - received);
      if (ret <= 0) {  // 0 return value indicates connection closed
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
          httpd_resp_send_err(r, HTTPD_408_REQ_TIMEOUT, nullptr);
          return ESP_ERR_TIMEOUT;
        }
        httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, nullptr);
        return ESP_FAIL;
      }
      received += static_cast<size_t>(ret);
    }
    if (received != r->content_len) {
      ESP_LOGW(TAG, "Incomplete POST body for %s: got %zu of %zu bytes", r->uri, received, r->content_len);
      httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, nullptr);
      return ESP_FAIL;
    }
  }

  AsyncWebServerRequest req(r, std::move(post_query));
  return static_cast<AsyncWebServer *>(r->user_ctx)->request_handler_(&req);
}

esp_err_t AsyncWebServer::request_handler(httpd_req_t *r) {
  ESP_LOGVV(TAG, "Enter AsyncWebServer::request_handler. method=%u, uri=%s", r->method, r->uri);
  AsyncWebServerRequest req(r);
  return static_cast<AsyncWebServer *>(r->user_ctx)->request_handler_(&req);
}

esp_err_t AsyncWebServer::request_handler_(AsyncWebServerRequest *request) const {
  if (handle_firmware_version_request(request)) {
    return ESP_OK;
  }
  for (auto *handler : this->handlers_) {
    if (handler->canHandle(request)) {
      // At now process only basic requests.
      // OTA requires multipart request support and handleUpload for it
      handler->handleRequest(request);
      return ESP_OK;
    }
  }
  if (this->on_not_found_) {
    this->on_not_found_(request);
    return ESP_OK;
  }
  return ESP_ERR_NOT_FOUND;
}

AsyncWebServerRequest::~AsyncWebServerRequest() {
  delete this->rsp_;
  for (auto *param : this->params_) {
    delete param;  // NOLINT(cppcoreguidelines-owning-memory)
  }
}

bool AsyncWebServerRequest::hasHeader(const char *name) const { return request_has_header(*this, name); }

optional<std::string> AsyncWebServerRequest::get_header(const char *name) const {
  return request_get_header(*this, name);
}

StringRef AsyncWebServerRequest::url_to(std::span<char, URL_BUF_SIZE> buffer) const {
  const char *uri = this->req_->uri;
  const char *query_start = strchr(uri, '?');
  size_t uri_len = query_start ? static_cast<size_t>(query_start - uri) : strlen(uri);
  size_t copy_len = std::min(uri_len, URL_BUF_SIZE - 1);
  memcpy(buffer.data(), uri, copy_len);
  buffer[copy_len] = '\0';
  // Decode URL-encoded characters in-place (e.g., %20 -> space)
  size_t decoded_len = url_decode(buffer.data());
  return StringRef(buffer.data(), decoded_len);
}

void AsyncWebServerRequest::redirect(const std::string &url) {
  httpd_resp_set_status(*this, "302 Found");
  httpd_resp_set_hdr(*this, "Location", url.c_str());
  httpd_resp_set_hdr(*this, "Connection", "close");
  apply_no_cache_headers(*this);
  httpd_resp_send(*this, nullptr, 0);
}

void AsyncWebServerRequest::init_response_(AsyncWebServerResponse *rsp, int code, const char *content_type) {
  // Set status code - use constants for common codes, default to 500 for unknown codes
  const char *status;
  switch (code) {
    case 200:
      status = HTTPD_200;
      break;
    case 404:
      status = HTTPD_404;
      break;
    case 409:
      status = HTTPD_409;
      break;
    default:
      status = HTTPD_500;
      break;
  }
  httpd_resp_set_status(*this, status);

  if (content_type && *content_type) {
    httpd_resp_set_type(*this, content_type);
  }
  httpd_resp_set_hdr(*this, "Accept-Ranges", "none");
  apply_no_cache_headers(*this);

  for (const auto &header : DefaultHeaders::Instance().headers_) {
    httpd_resp_set_hdr(*this, header.name, header.value);
  }

  delete this->rsp_;
  this->rsp_ = rsp;
}

#ifdef USE_WEBSERVER_AUTH

#ifdef USE_WEBSERVER_AUTH_DIGEST
namespace {

static constexpr const char *DIGEST_REALM = "Login Required";
static constexpr const char *DIGEST_QOP = "auth";
static constexpr size_t DIGEST_VALUE_LENGTH = 32;
static constexpr size_t DIGEST_CNONCE_MAX_LENGTH = 64;
static constexpr size_t DIGEST_NONCE_SLOTS = 8;
static constexpr size_t DIGEST_CNONCE_SLOTS = 4;
static constexpr uint32_t DIGEST_NONCE_LIFETIME_MS = 5 * 60 * 1000;

struct DigestCnonceState {
  std::array<char, DIGEST_CNONCE_MAX_LENGTH + 1> value{};
  uint32_t last_nonce_count{};
  uint64_t used_nonce_counts{};
};

struct DigestNonceState {
  std::array<char, DIGEST_VALUE_LENGTH + 1> nonce{};
  std::array<char, DIGEST_VALUE_LENGTH + 1> opaque{};
  uint32_t issued_at{};
  std::array<DigestCnonceState, DIGEST_CNONCE_SLOTS> cnonces{};
};

// Retain a small bounded set of challenges so simultaneous browser requests
// can authenticate without allowing captured Authorization headers to replay.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::array<DigestNonceState, DIGEST_NONCE_SLOTS> digest_nonce_states;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
size_t next_digest_nonce_slot = 0;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
portMUX_TYPE digest_auth_lock = portMUX_INITIALIZER_UNLOCKED;

bool digest_ref_equals(StringRef value, const char *expected) {
  const size_t expected_length = strlen(expected);
  return value.size() == expected_length && memcmp(value.c_str(), expected, expected_length) == 0;
}

bool digest_ref_equals_buffer(StringRef value, const char *expected, size_t expected_length) {
  return value.size() == expected_length && memcmp(value.c_str(), expected, expected_length) == 0;
}

bool parse_nonce_count(StringRef value, uint32_t *nonce_count) {
  if (value.size() != 8)
    return false;

  uint32_t result = 0;
  for (size_t i = 0; i < value.size(); i++) {
    const char ch = value.c_str()[i];
    uint8_t digit;
    if (ch >= '0' && ch <= '9') {
      digit = static_cast<uint8_t>(ch - '0');
    } else if (ch >= 'a' && ch <= 'f') {
      digit = static_cast<uint8_t>(ch - 'a' + 10);
    } else if (ch >= 'A' && ch <= 'F') {
      digit = static_cast<uint8_t>(ch - 'A' + 10);
    } else {
      return false;
    }
    result = (result << 4) | digit;
  }
  if (result == 0)
    return false;
  *nonce_count = result;
  return true;
}

bool digest_nonce_expired(const DigestNonceState &state, uint32_t now) {
  return state.nonce[0] == '\0' || static_cast<uint32_t>(now - state.issued_at) > DIGEST_NONCE_LIFETIME_MS;
}

bool digest_challenge_is_valid(StringRef nonce, StringRef opaque) {
  const uint32_t now = millis();
  bool valid = false;
  portENTER_CRITICAL(&digest_auth_lock);
  for (const auto &state : digest_nonce_states) {
    if (!digest_nonce_expired(state, now) &&
        digest_ref_equals_buffer(nonce, state.nonce.data(), DIGEST_VALUE_LENGTH) &&
        digest_ref_equals_buffer(opaque, state.opaque.data(), DIGEST_VALUE_LENGTH)) {
      valid = true;
      break;
    }
  }
  portEXIT_CRITICAL(&digest_auth_lock);
  return valid;
}

bool accept_nonce_count(DigestCnonceState *state, uint32_t nonce_count) {
  if (nonce_count > state->last_nonce_count) {
    const uint32_t advance = nonce_count - state->last_nonce_count;
    state->used_nonce_counts = advance >= 64 ? 1 : (state->used_nonce_counts << advance) | 1;
    state->last_nonce_count = nonce_count;
    return true;
  }

  // Permit parallel requests to arrive slightly out of order, but never accept
  // the same count twice or a count outside the bounded replay window.
  const uint32_t distance = state->last_nonce_count - nonce_count;
  if (distance >= 64)
    return false;
  const uint64_t count_mask = uint64_t{1} << distance;
  if ((state->used_nonce_counts & count_mask) != 0)
    return false;
  state->used_nonce_counts |= count_mask;
  return true;
}

bool accept_digest_nonce_count(StringRef nonce, StringRef opaque, StringRef cnonce, uint32_t nonce_count) {
  if (cnonce.size() == 0 || cnonce.size() > DIGEST_CNONCE_MAX_LENGTH)
    return false;

  const uint32_t now = millis();
  bool accepted = false;
  portENTER_CRITICAL(&digest_auth_lock);
  for (auto &state : digest_nonce_states) {
    if (digest_nonce_expired(state, now) ||
        !digest_ref_equals_buffer(nonce, state.nonce.data(), DIGEST_VALUE_LENGTH) ||
        !digest_ref_equals_buffer(opaque, state.opaque.data(), DIGEST_VALUE_LENGTH)) {
      continue;
    }

    DigestCnonceState *free_state = nullptr;
    bool found_cnonce = false;
    for (auto &cnonce_state : state.cnonces) {
      if (cnonce_state.value[0] == '\0') {
        if (free_state == nullptr)
          free_state = &cnonce_state;
        continue;
      }
      if (!digest_ref_equals_buffer(cnonce, cnonce_state.value.data(), strlen(cnonce_state.value.data())))
        continue;
      found_cnonce = true;
      accepted = accept_nonce_count(&cnonce_state, nonce_count);
      break;
    }

    if (!found_cnonce && free_state != nullptr) {
      memcpy(free_state->value.data(), cnonce.c_str(), cnonce.size());
      free_state->value[cnonce.size()] = '\0';
      free_state->last_nonce_count = nonce_count;
      free_state->used_nonce_counts = 1;
      accepted = true;
    }
    break;
  }
  portEXIT_CRITICAL(&digest_auth_lock);
  return accepted;
}

void retain_digest_challenge(const char *nonce, const char *opaque) {
  const uint32_t now = millis();
  portENTER_CRITICAL(&digest_auth_lock);
  auto &state = digest_nonce_states[next_digest_nonce_slot];
  state = DigestNonceState{};
  memcpy(state.nonce.data(), nonce, DIGEST_VALUE_LENGTH + 1);
  memcpy(state.opaque.data(), opaque, DIGEST_VALUE_LENGTH + 1);
  state.issued_at = now;
  next_digest_nonce_slot = (next_digest_nonce_slot + 1) % DIGEST_NONCE_SLOTS;
  portEXIT_CRITICAL(&digest_auth_lock);
}

void bytes_to_hex(const uint8_t *data, size_t len, char *out) {
  static const char HEX[] = "0123456789abcdef";
  for (size_t i = 0; i < len; i++) {
    out[i * 2] = HEX[data[i] >> 4];
    out[i * 2 + 1] = HEX[data[i] & 0x0f];
  }
  out[len * 2] = '\0';
}

StringRef digest_param(StringRef params, const char *key) {
  size_t key_len = strlen(key);
  const char *base = params.c_str();
  size_t n = params.size();
  size_t i = 0;
  while (i < n) {
    while (i < n && (base[i] == ' ' || base[i] == ','))
      i++;
    size_t name_start = i;
    while (i < n && base[i] != '=' && base[i] != ',')
      i++;
    if (i >= n)
      break;
    if (base[i] == ',')
      continue;
    size_t name_len = i - name_start;
    while (name_len > 0 && base[name_start + name_len - 1] == ' ')
      name_len--;
    i++;
    const char *val_start;
    size_t val_len;
    if (i < n && base[i] == '"') {
      i++;
      val_start = base + i;
      while (i < n && base[i] != '"')
        i++;
      val_len = (base + i) - val_start;
      if (i < n)
        i++;
    } else {
      val_start = base + i;
      while (i < n && base[i] != ',')
        i++;
      val_len = (base + i) - val_start;
    }
    if (name_len == key_len && memcmp(base + name_start, key, key_len) == 0)
      return StringRef(val_start, val_len);
    while (i < n && base[i] != ',')
      i++;
  }
  return StringRef();
}

enum class DigestAuthResult : uint8_t { FAILED, STALE, AUTHENTICATED };

DigestAuthResult check_digest_auth(const char *username, const char *password, const std::string &header,
                                   const char *method, const char *request_uri) {
  const size_t prefix_len = sizeof("Digest ") - 1;
  StringRef params(header.c_str() + prefix_len, header.size() - prefix_len);

  if (digest_param(params, "username") != username)
    return DigestAuthResult::FAILED;

  StringRef realm = digest_param(params, "realm");
  StringRef nonce = digest_param(params, "nonce");
  StringRef uri = digest_param(params, "uri");
  StringRef qop = digest_param(params, "qop");
  StringRef nc = digest_param(params, "nc");
  StringRef cnonce = digest_param(params, "cnonce");
  StringRef opaque = digest_param(params, "opaque");
  StringRef algorithm = digest_param(params, "algorithm");
  StringRef response = digest_param(params, "response");
  uint32_t nonce_count;
  if (!digest_ref_equals(realm, DIGEST_REALM) || !digest_ref_equals(qop, DIGEST_QOP) ||
      !digest_ref_equals(uri, request_uri) || (algorithm.size() != 0 && !digest_ref_equals(algorithm, "MD5")) ||
      response.size() != DIGEST_VALUE_LENGTH || cnonce.size() == 0 || cnonce.size() > DIGEST_CNONCE_MAX_LENGTH ||
      !parse_nonce_count(nc, &nonce_count)) {
    return DigestAuthResult::FAILED;
  }
  if (!digest_challenge_is_valid(nonce, opaque))
    return DigestAuthResult::STALE;

  md5_context_t ctx;
  uint8_t digest[16];

  char ha1[33];
  esp_rom_md5_init(&ctx);
  esp_rom_md5_update(&ctx, username, strlen(username));
  esp_rom_md5_update(&ctx, ":", 1);
  esp_rom_md5_update(&ctx, DIGEST_REALM, strlen(DIGEST_REALM));
  esp_rom_md5_update(&ctx, ":", 1);
  esp_rom_md5_update(&ctx, password, strlen(password));
  esp_rom_md5_final(digest, &ctx);
  bytes_to_hex(digest, sizeof(digest), ha1);

  char ha2[33];
  esp_rom_md5_init(&ctx);
  esp_rom_md5_update(&ctx, method, strlen(method));
  esp_rom_md5_update(&ctx, ":", 1);
  esp_rom_md5_update(&ctx, request_uri, strlen(request_uri));
  esp_rom_md5_final(digest, &ctx);
  bytes_to_hex(digest, sizeof(digest), ha2);

  char expected[33];
  esp_rom_md5_init(&ctx);
  esp_rom_md5_update(&ctx, ha1, 32);
  esp_rom_md5_update(&ctx, ":", 1);
  esp_rom_md5_update(&ctx, nonce.c_str(), nonce.size());
  esp_rom_md5_update(&ctx, ":", 1);
  esp_rom_md5_update(&ctx, nc.c_str(), nc.size());
  esp_rom_md5_update(&ctx, ":", 1);
  esp_rom_md5_update(&ctx, cnonce.c_str(), cnonce.size());
  esp_rom_md5_update(&ctx, ":", 1);
  esp_rom_md5_update(&ctx, DIGEST_QOP, strlen(DIGEST_QOP));
  esp_rom_md5_update(&ctx, ":", 1);
  esp_rom_md5_update(&ctx, ha2, 32);
  esp_rom_md5_final(digest, &ctx);
  bytes_to_hex(digest, sizeof(digest), expected);

  uint8_t result = 0;
  for (size_t i = 0; i < 32; i++)
    result |= static_cast<uint8_t>(expected[i] ^ response[i]);
  if (result != 0)
    return DigestAuthResult::FAILED;
  if (!accept_digest_nonce_count(nonce, opaque, cnonce, nonce_count))
    return DigestAuthResult::STALE;
  return DigestAuthResult::AUTHENTICATED;
}

}  // namespace
#endif  // USE_WEBSERVER_AUTH_DIGEST

bool AsyncWebServerRequest::authenticate(const char *username, const char *password) const {
  if (username == nullptr || password == nullptr || *username == 0) {
    return true;
  }
  auto auth = this->get_header("Authorization");
  if (!auth.has_value()) {
    return false;
  }

  auto *auth_str = auth.value().c_str();

#ifdef USE_WEBSERVER_AUTH_DIGEST
  const auto auth_prefix_len = sizeof("Digest ") - 1;
  if (strncmp("Digest ", auth_str, auth_prefix_len) != 0) {
    ESP_LOGW(TAG, "Only Digest authorization supported");
    return false;
  }
  const auto result =
      check_digest_auth(username, password, auth.value(), http_method_str(this->method()), this->req_->uri);
  this->digest_nonce_stale_ = result == DigestAuthResult::STALE;
  return result == DigestAuthResult::AUTHENTICATED;
#else
  const auto auth_prefix_len = sizeof("Basic ") - 1;
  if (strncmp("Basic ", auth_str, auth_prefix_len) != 0) {
    ESP_LOGW(TAG, "Only Basic authorization supported");
    return false;
  }

  // Build user:pass in stack buffer to avoid heap allocation
  constexpr size_t max_user_info_len = 256;
  char user_info[max_user_info_len];
  size_t user_len = strlen(username);
  size_t pass_len = strlen(password);
  size_t user_info_len = user_len + 1 + pass_len;

  if (user_info_len >= max_user_info_len) {
    ESP_LOGW(TAG, "Credentials too long for authentication");
    return false;
  }

  memcpy(user_info, username, user_len);
  user_info[user_len] = ':';
  memcpy(user_info + user_len + 1, password, pass_len);
  user_info[user_info_len] = '\0';

  // Base64 output size is ceil(input_len * 4/3) + 1, with input bounded to 256 bytes
  // max output is ceil(256 * 4/3) + 1 = 343 bytes, use 350 for safety
  constexpr size_t max_digest_len = 350;
  char digest[max_digest_len];
  size_t out;
  esp_crypto_base64_encode(reinterpret_cast<uint8_t *>(digest), max_digest_len, &out,
                           reinterpret_cast<const uint8_t *>(user_info), user_info_len);

  // Constant-time comparison to avoid timing side channels.
  // No early return on length mismatch — the length difference is folded
  // into the accumulator so any mismatch is rejected.
  const char *provided = auth_str + auth_prefix_len;
  size_t digest_len = out;  // length from esp_crypto_base64_encode
  // Derive provided_len from the already-sized std::string rather than
  // rescanning with strlen (avoids attacker-controlled scan length).
  size_t provided_len = auth.value().size() - auth_prefix_len;
  // Use full-width XOR so any bit difference in the lengths is preserved
  // (uint8_t truncation would miss differences in higher bytes, e.g.
  // digest_len vs digest_len + 256).
  volatile size_t result = digest_len ^ provided_len;
  // Iterate over the expected digest length only — the full-width length
  // XOR above already rejects any length mismatch, and bounding the loop
  // prevents a long Authorization header from forcing extra work.
  for (size_t i = 0; i < digest_len; i++) {
    char provided_ch = (i < provided_len) ? provided[i] : 0;
    result |= static_cast<uint8_t>(digest[i] ^ provided_ch);
  }
  return result == 0;
#endif  // USE_WEBSERVER_AUTH_DIGEST
}

void AsyncWebServerRequest::requestAuthentication() const {
  httpd_resp_set_hdr(*this, "Connection", "keep-alive");
#ifdef USE_WEBSERVER_AUTH_DIGEST
  uint8_t random_bytes[16];
  char nonce[33];
  char opaque[33];
  char header[192];
  esp_fill_random(random_bytes, sizeof(random_bytes));
  bytes_to_hex(random_bytes, sizeof(random_bytes), nonce);
  esp_fill_random(random_bytes, sizeof(random_bytes));
  bytes_to_hex(random_bytes, sizeof(random_bytes), opaque);
  retain_digest_challenge(nonce, opaque);
  snprintf(header, sizeof(header),
           R"(Digest realm="Login Required", qop="auth", algorithm=MD5, nonce="%s", opaque="%s"%s)", nonce,
           opaque, this->digest_nonce_stale_ ? ", stale=true" : "");
  httpd_resp_set_hdr(*this, "WWW-Authenticate", header);
#else
  httpd_resp_set_hdr(*this, "WWW-Authenticate", "Basic realm=\"Login Required\"");
#endif  // USE_WEBSERVER_AUTH_DIGEST
  httpd_resp_send_err(*this, HTTPD_401_UNAUTHORIZED, nullptr);
}
#endif  // USE_WEBSERVER_AUTH

AsyncWebParameter *AsyncWebServerRequest::getParam(const char *name) {
  // Check cache first - only successful lookups are cached
  for (auto *param : this->params_) {
    if (param->name() == name) {
      return param;
    }
  }

  // Look up value from query strings
  auto val = this->find_query_value_(name);

  // Don't cache misses to avoid wasting memory when handlers check for
  // optional parameters that don't exist in the request
  if (!val.has_value()) {
    return nullptr;
  }

  auto *param = new AsyncWebParameter(name, val.value());  // NOLINT(cppcoreguidelines-owning-memory)
  this->params_.push_back(param);
  return param;
}

/// Search post_query then URL query with a callback.
/// Returns first truthy result, or value-initialized default.
/// URL query is accessed directly from req->uri (same pattern as url_to()).
template<typename Func>
static auto search_query_sources(httpd_req_t *req, const std::string &post_query, const char *name, Func func)
    -> decltype(func(nullptr, size_t{0}, name)) {
  if (!post_query.empty()) {
    auto result = func(post_query.c_str(), post_query.size(), name);
    if (result) {
      return result;
    }
  }
  // Use httpd API for query length, then access string directly from URI.
  // http_parser identifies components by offset/length without modifying the URI string.
  // This is the same pattern used by url_to().
  auto len = httpd_req_get_url_query_len(req);
  if (len == 0) {
    return {};
  }
  const char *query = strchr(req->uri, '?');
  if (query == nullptr) {
    return {};
  }
  query++;  // skip '?'
  return func(query, len, name);
}

optional<std::string> AsyncWebServerRequest::find_query_value_(const char *name) const {
  return search_query_sources(this->req_, this->post_query_, name,
                              [](const char *q, size_t len, const char *k) { return query_key_value(q, len, k); });
}

bool AsyncWebServerRequest::hasArg(const char *name) {
  return search_query_sources(this->req_, this->post_query_, name, query_has_key);
}

std::string AsyncWebServerRequest::arg(const char *name) {
  auto val = this->find_query_value_(name);
  if (val.has_value()) {
    return std::move(val.value());
  }
  return {};
}

void AsyncWebServerResponse::addHeader(const char *name, const char *value) {
  httpd_resp_set_hdr(*this->req_, name, value);
}

void AsyncResponseStream::print(float value) {
  // Use stack buffer to avoid temporary string allocation
  // Size: sign (1) + digits (10) + decimal (1) + precision (6) + exponent (5) + null (1) = 24, use 32 for safety
  char buf[32];
  int len = snprintf(buf, sizeof(buf), "%f", value);
  this->content_.append(buf, len);
}

void AsyncResponseStream::printf(const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  const int length = vsnprintf(nullptr, 0, fmt, args);
  va_end(args);
  if (length < 0) {
    return;
  }

  std::string str;
  str.resize(length + 1);

  va_start(args, fmt);
  vsnprintf(str.data(), str.size(), fmt, args);
  va_end(args);
  str.resize(length);

  this->print(str);
}

#ifdef USE_WEBSERVER
AsyncEventSource::~AsyncEventSource() {
  for (auto *ses : this->sessions_) {
    delete ses;  // NOLINT(cppcoreguidelines-owning-memory)
  }
}

void AsyncEventSource::handleRequest(AsyncWebServerRequest *request) {
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,clang-analyzer-cplusplus.NewDeleteLeaks)
  auto *rsp = new AsyncEventSourceResponse(request, this, this->web_server_);
  if (this->on_connect_) {
    this->on_connect_(rsp);
  }
  this->sessions_.push_back(rsp);
  // Wake up WebServer::loop() to drain deferred event queues for this client.
  // Safe from httpd task context via the pending_enable_loop_ flag.
  this->web_server_->enable_loop_soon_any_context();
}

bool AsyncEventSource::loop() {
  // Clean up dead sessions safely
  // This follows the ESP-IDF pattern where free_ctx marks resources as dead
  // and the main loop handles the actual cleanup to avoid race conditions
  for (size_t i = 0; i < this->sessions_.size();) {
    auto *ses = this->sessions_[i];
    // If the session has a dead socket (marked by destroy callback)
    if (ses->fd_.load() == 0) {
      ESP_LOGD(TAG, "Removing dead event source session");
      delete ses;  // NOLINT(cppcoreguidelines-owning-memory)
      // Remove by swapping with last element (O(1) removal, order doesn't matter for sessions)
      this->sessions_[i] = this->sessions_.back();
      this->sessions_.pop_back();
    } else {
      ses->loop();
      ++i;
    }
  }
  return !this->sessions_.empty();
}

void AsyncEventSource::try_send_nodefer(const char *message, size_t message_len, const char *event, uint32_t id,
                                        uint32_t reconnect) {
  for (auto *ses : this->sessions_) {
    if (ses->fd_.load() != 0) {  // Skip dead sessions
      ses->try_send_nodefer(message, message_len, event, id, reconnect);
    }
  }
}

void AsyncEventSource::deferrable_send_state(void *source, const char *event_type,
                                             message_generator_t *message_generator) {
  // Skip if no connected clients to avoid unnecessary processing
  if (this->empty())
    return;
  for (auto *ses : this->sessions_) {
    if (ses->fd_.load() != 0) {  // Skip dead sessions
      ses->deferrable_send_state(source, event_type, message_generator);
    }
  }
}

AsyncEventSourceResponse::AsyncEventSourceResponse(const AsyncWebServerRequest *request,
                                                   esphome::web_server_idf::AsyncEventSource *server,
                                                   esphome::web_server::WebServer *ws)
    : server_(server), web_server_(ws), entities_iterator_(ws, server) {
  httpd_req_t *req = *request;

  httpd_resp_set_status(req, HTTPD_200);
  httpd_resp_set_type(req, "text/event-stream");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
  httpd_resp_set_hdr(req, "Connection", "keep-alive");

  for (const auto &header : DefaultHeaders::Instance().headers_) {
    httpd_resp_set_hdr(req, header.name, header.value);
  }

  httpd_resp_send_chunk(req, CRLF_STR, CRLF_LEN);

  req->sess_ctx = this;
  req->free_ctx = AsyncEventSourceResponse::destroy;

  this->hd_ = req->handle;
  this->fd_.store(httpd_req_to_sockfd(req));

  // Use non-blocking send to prevent watchdog timeouts when TCP buffers are full
  httpd_sess_set_send_override(this->hd_, this->fd_.load(), nonblocking_send);

  // Configure reconnect timeout and send config
  // this should always go through since the tcp send buffer is empty on connect
  auto message = ws->get_config_json();
  this->try_send_nodefer(message.c_str(), message.size(), "ping", millis(), 30000);

#ifdef USE_WEBSERVER_SORTING
  for (auto &group : ws->sorting_groups_) {
    // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks) false positive with ArduinoJson
    json::JsonBuilder builder;
    JsonObject root = builder.root();
    root["name"] = group.second.name;
    root["sorting_weight"] = group.second.weight;
    message = builder.serialize();
    // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

    // a (very) large number of these should be able to be queued initially without defer
    // since the only thing in the send buffer at this point is the initial ping/config
    this->try_send_nodefer(message.c_str(), message.size(), "sorting_group");
  }
#endif

  this->entities_iterator_.begin(ws->include_internal_);

  // just dump them all up-front and take advantage of the deferred queue
  //     on second thought that takes too long, but leaving the commented code here for debug purposes
  // while(!this->entities_iterator_.completed()) {
  //  this->entities_iterator_.advance();
  //}
}

void AsyncEventSourceResponse::destroy(void *ptr) {
  auto *rsp = static_cast<AsyncEventSourceResponse *>(ptr);
  int fd = rsp->fd_.exchange(0);  // Atomically get and clear fd
  ESP_LOGD(TAG, "Event source connection closed (fd: %d)", fd);
  // Mark as dead - will be cleaned up in the main loop
  // Note: We don't delete or remove from set here to avoid race conditions
  // httpd will call our custom close_fn (safe_close_with_shutdown) which handles
  // shutdown() before close() to prevent lwIP race conditions
}

// helper for allowing only unique entries in the queue
void AsyncEventSourceResponse::deq_push_back_with_dedup_(void *source, message_generator_t *message_generator) {
  DeferredEvent item(source, message_generator);

  // Use range-based for loop instead of std::find_if to reduce template instantiation overhead and binary size
  for (auto &event : this->deferred_queue_) {
    if (event == item) {
      return;  // Already in queue, no need to update since items are equal
    }
  }
  this->deferred_queue_.push_back(item);
}

void AsyncEventSourceResponse::process_deferred_queue_() {
  while (!deferred_queue_.empty()) {
    DeferredEvent &de = deferred_queue_.front();
    auto message = de.message_generator_(web_server_, de.source_);
    if (this->try_send_nodefer(message.c_str(), message.size(), "state")) {
      // O(n) but memory efficiency is more important than speed here which is why std::vector was chosen
      deferred_queue_.erase(deferred_queue_.begin());
    } else {
      break;
    }
  }
}

void AsyncEventSourceResponse::process_buffer_() {
  if (event_buffer_.empty()) {
    return;
  }
  if (event_bytes_sent_ == event_buffer_.size()) {
    event_buffer_.resize(0);
    event_bytes_sent_ = 0;
    return;
  }

  size_t remaining = event_buffer_.size() - event_bytes_sent_;
  int bytes_sent =
      httpd_socket_send(this->hd_, this->fd_.load(), event_buffer_.c_str() + event_bytes_sent_, remaining, 0);
  if (bytes_sent == HTTPD_SOCK_ERR_TIMEOUT) {
    // EAGAIN/EWOULDBLOCK - socket buffer full, try again later
    // NOTE: Similar logic exists in web_server/web_server.cpp in DeferredUpdateEventSource::process_deferred_queue_()
    // The implementations differ due to platform-specific APIs (HTTPD_SOCK_ERR_TIMEOUT vs DISCARDED, fd_.store(0) vs
    // close()), but the failure counting and timeout logic should be kept in sync. If you change this logic, also
    // update the Arduino implementation.
    this->consecutive_send_failures_++;
    if (this->consecutive_send_failures_ >= MAX_CONSECUTIVE_SEND_FAILURES) {
      // Too many failures, connection is likely dead
      ESP_LOGW(TAG, "Closing stuck EventSource connection after %" PRIu16 " failed sends",
               this->consecutive_send_failures_);
      this->fd_.store(0);  // Mark for cleanup
      this->deferred_queue_.clear();
    }
    return;
  }
  if (bytes_sent == HTTPD_SOCK_ERR_FAIL) {
    // Real socket error - connection will be closed by httpd and destroy callback will be called
    return;
  }
  if (bytes_sent <= 0) {
    // Unexpected error or zero bytes sent
    ESP_LOGW(TAG, "Unexpected send result: %d", bytes_sent);
    return;
  }

  // Successful send - reset failure counter
  this->consecutive_send_failures_ = 0;
  event_bytes_sent_ += bytes_sent;

  // Log partial sends for debugging
  if (event_bytes_sent_ < event_buffer_.size()) {
    ESP_LOGV(TAG, "Partial send: %d/%zu bytes (total: %zu/%zu)", bytes_sent, remaining, event_bytes_sent_,
             event_buffer_.size());
  }

  if (event_bytes_sent_ == event_buffer_.size()) {
    event_buffer_.resize(0);
    event_bytes_sent_ = 0;
  }
}

void AsyncEventSourceResponse::loop() {
  process_buffer_();
  process_deferred_queue_();
  if (!this->entities_iterator_.completed())
    this->entities_iterator_.advance();
}

bool AsyncEventSourceResponse::try_send_nodefer(const char *message, size_t message_len, const char *event, uint32_t id,
                                                uint32_t reconnect) {
  if (this->fd_.load() == 0) {
    return false;
  }

  process_buffer_();
  if (!event_buffer_.empty()) {
    // there is still pending event data to send first
    return false;
  }

  // 8 spaces are standing in for the hexidecimal chunk length to print later
  const char chunk_len_header[] = "        " CRLF_STR;
  const int chunk_len_header_len = sizeof(chunk_len_header) - 1;

  event_buffer_.append(chunk_len_header);

  // Use stack buffer for formatting numeric fields to avoid temporary string allocations
  // Size: "retry: " (7) + max uint32 (10 digits) + CRLF (2) + null (1) = 20 bytes, use 32 for safety
  constexpr size_t num_buf_size = 32;
  char num_buf[num_buf_size];

  if (reconnect) {
    int len = snprintf(num_buf, num_buf_size, "retry: %" PRIu32 CRLF_STR, reconnect);
    event_buffer_.append(num_buf, len);
  }

  if (id) {
    int len = snprintf(num_buf, num_buf_size, "id: %" PRIu32 CRLF_STR, id);
    event_buffer_.append(num_buf, len);
  }

  if (event && *event) {
    event_buffer_.append("event: ", sizeof("event: ") - 1);
    event_buffer_.append(event);
    event_buffer_.append(CRLF_STR, CRLF_LEN);
  }

  // Match ESPAsyncWebServer: null message means no data lines and no terminating blank line
  if (message) {
    // SSE spec requires each line of a multi-line message to have its own "data:" prefix
    // Handle \n, \r, and \r\n line endings (matching ESPAsyncWebServer behavior)

    // Fast path: check if message contains any newlines at all
    // Most SSE messages (JSON state updates) have no newlines
    const char *first_n = static_cast<const char *>(memchr(message, '\n', message_len));
    const char *first_r = static_cast<const char *>(memchr(message, '\r', message_len));

    if (first_n == nullptr && first_r == nullptr) {
      // No newlines - fast path (most common case)
      event_buffer_.append("data: ", sizeof("data: ") - 1);
      event_buffer_.append(message, message_len);
      event_buffer_.append(CRLF_STR CRLF_STR, CRLF_LEN * 2);  // data line + blank line terminator
    } else {
      // Has newlines - handle multi-line message
      const char *line_start = message;
      const char *msg_end = message + message_len;

      // Reuse the first search results
      const char *next_n = first_n;
      const char *next_r = first_r;

      while (line_start <= msg_end) {
        const char *line_end;
        const char *next_line;

        if (next_n == nullptr && next_r == nullptr) {
          // No more line breaks - output remaining text as final line
          event_buffer_.append("data: ", sizeof("data: ") - 1);
          event_buffer_.append(line_start, msg_end - line_start);
          event_buffer_.append(CRLF_STR, CRLF_LEN);
          break;
        }

        // Determine line ending type and next line start
        if (next_n != nullptr && next_r != nullptr) {
          if (next_r + 1 == next_n) {
            // \r\n sequence
            line_end = next_r;
            next_line = next_n + 1;
          } else {
            // Mixed \n and \r - use whichever comes first
            line_end = (next_r < next_n) ? next_r : next_n;
            next_line = line_end + 1;
          }
        } else if (next_n != nullptr) {
          // Unix LF
          line_end = next_n;
          next_line = next_n + 1;
        } else {
          // Old Mac CR
          line_end = next_r;
          next_line = next_r + 1;
        }

        // Output this line
        event_buffer_.append("data: ", sizeof("data: ") - 1);
        event_buffer_.append(line_start, line_end - line_start);
        event_buffer_.append(CRLF_STR, CRLF_LEN);

        line_start = next_line;

        // Check if we've consumed all content
        if (line_start >= msg_end) {
          break;
        }

        // Search for next newlines only in remaining string
        next_n = static_cast<const char *>(memchr(line_start, '\n', msg_end - line_start));
        next_r = static_cast<const char *>(memchr(line_start, '\r', msg_end - line_start));
      }

      // Terminate message with blank line
      event_buffer_.append(CRLF_STR, CRLF_LEN);
    }
  }

  if (event_buffer_.size() == static_cast<size_t>(chunk_len_header_len)) {
    // Nothing was added, reset buffer
    event_buffer_.resize(0);
    return true;
  }

  event_buffer_.append(CRLF_STR, CRLF_LEN);

  // chunk length header itself and the final chunk terminating CRLF are not counted as part of the chunk
  int chunk_len = event_buffer_.size() - CRLF_LEN - chunk_len_header_len;
  char chunk_len_str[9];
  snprintf(chunk_len_str, 9, "%08x", chunk_len);
  std::memcpy(&event_buffer_[0], chunk_len_str, 8);

  event_bytes_sent_ = 0;
  process_buffer_();

  return true;
}

void AsyncEventSourceResponse::deferrable_send_state(void *source, const char *event_type,
                                                     message_generator_t *message_generator) {
  // allow all json "details_all" to go through before publishing bare state events, this avoids unnamed entries showing
  // up in the web GUI and reduces event load during initial connect
  if (!this->entities_iterator_.completed() && 0 != strcmp(event_type, "state_detail_all"))
    return;

  if (source == nullptr)
    return;
  if (event_type == nullptr)
    return;
  if (message_generator == nullptr)
    return;

  if (0 != strcmp(event_type, "state_detail_all") && 0 != strcmp(event_type, "state")) {
    ESP_LOGE(TAG, "Can't defer non-state event");
  }

  process_buffer_();
  process_deferred_queue_();

  if (!event_buffer_.empty() || !deferred_queue_.empty()) {
    // outgoing event buffer or deferred queue still not empty which means downstream tcp send buffer full, no point
    // trying to send first
    deq_push_back_with_dedup_(source, message_generator);
  } else {
    auto message = message_generator(web_server_, source);
    if (!this->try_send_nodefer(message.c_str(), message.size(), "state")) {
      deq_push_back_with_dedup_(source, message_generator);
    }
  }
}
#endif

#ifdef USE_WEBSERVER_OTA
esp_err_t AsyncWebServer::handle_multipart_upload_(httpd_req_t *r, const char *content_type) {
  static constexpr size_t MULTIPART_CHUNK_SIZE = 1460;       // Match Arduino AsyncWebServer buffer size
  static constexpr size_t YIELD_INTERVAL_BYTES = 16 * 1024;  // Yield every 16KB to prevent watchdog

  // Parse boundary and create reader
  const char *boundary_start;
  size_t boundary_len;
  if (!parse_multipart_boundary(content_type, &boundary_start, &boundary_len)) {
    ESP_LOGE(TAG, "Failed to parse multipart boundary");
    httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, nullptr);
    return ESP_FAIL;
  }

  AsyncWebServerRequest req(r);
  AsyncWebHandler *handler = nullptr;
  for (auto *h : this->handlers_) {
    if (h->canHandle(&req)) {
      handler = h;
      break;
    }
  }

  if (!handler) {
    ESP_LOGW(TAG, "No handler found for OTA request");
    httpd_resp_send_err(r, HTTPD_404_NOT_FOUND, nullptr);
    return ESP_OK;
  }

  // Upload state
  std::string filename;
  size_t index = 0;
  // Create reader on heap to reduce stack usage
  auto reader = std::make_unique<MultipartReader>("--" + std::string(boundary_start, boundary_len));

  // Configure callbacks
  reader->set_data_callback([&](const uint8_t *data, size_t len) {
    if (!reader->has_file() || !len)
      return;

    if (filename.empty()) {
      filename = reader->get_current_part().filename;
      ESP_LOGV(TAG, "Processing file: '%s'", filename.c_str());
      handler->handleUpload(&req, filename, 0, nullptr, 0, false);  // Start
    }

    handler->handleUpload(&req, filename, index, const_cast<uint8_t *>(data), len, false);
    index += len;
  });

  reader->set_part_complete_callback([&]() {
    if (index > 0) {
      handler->handleUpload(&req, filename, index, nullptr, 0, true);  // End
      filename.clear();
      index = 0;
    }
  });

  // Use heap buffer - 1460 bytes is too large for the httpd task stack
  auto buffer = std::make_unique_for_overwrite<char[]>(MULTIPART_CHUNK_SIZE);
  size_t bytes_since_yield = 0;

  for (size_t remaining = r->content_len; remaining > 0;) {
    int recv_len = httpd_req_recv(r, buffer.get(), std::min(remaining, MULTIPART_CHUNK_SIZE));

    if (recv_len <= 0) {
      httpd_resp_send_err(r, recv_len == HTTPD_SOCK_ERR_TIMEOUT ? HTTPD_408_REQ_TIMEOUT : HTTPD_400_BAD_REQUEST,
                          nullptr);
      return recv_len == HTTPD_SOCK_ERR_TIMEOUT ? ESP_ERR_TIMEOUT : ESP_FAIL;
    }

    if (reader->parse(buffer.get(), recv_len) != static_cast<size_t>(recv_len)) {
      ESP_LOGW(TAG, "Multipart parser error");
      httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, nullptr);
      return ESP_FAIL;
    }

    remaining -= recv_len;
    bytes_since_yield += recv_len;

    if (bytes_since_yield > YIELD_INTERVAL_BYTES) {
      vTaskDelay(1);
      bytes_since_yield = 0;
    }
  }

  handler->handleRequest(&req);
  return ESP_OK;
}
#endif  // USE_WEBSERVER_OTA

}  // namespace esphome::web_server_idf

#endif  // !defined(USE_ESP32)
