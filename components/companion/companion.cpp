#include "companion.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include "../espcontrol/companion_controls.h"

#include <esp_random.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>
#include <mbedtls/x509_crt.h>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

namespace esphome::companion {

static const char *const TAG = "companion";
static CompanionService *global_companion_service = nullptr;
static constexpr uint32_t PAIRING_WINDOW_MS = 5 * 60 * 1000;
static constexpr uint32_t RETRY_DELAY_MS = 30 * 1000;
static constexpr char PAIRING_ALPHABET[] = "ABCDEFGHJKLMNPQRSTUVWXYZ";

static std::vector<std::string> split(const std::string &value, char separator) {
  std::vector<std::string> parts;
  std::stringstream stream(value);
  std::string part;
  while (std::getline(stream, part, separator)) parts.push_back(part);
  return parts;
}

static bool safe_field(const std::string &value, size_t limit) {
  if (value.empty() || value.size() > limit) return false;
  return std::all_of(value.begin(), value.end(), [](unsigned char byte) {
    return byte >= 0x20 && byte <= 0x7e && byte != '|' && byte != ',';
  });
}

static std::string hex(const uint8_t *value, size_t length) {
  static constexpr char digits[] = "0123456789abcdef";
  std::string result;
  result.reserve(length * 2);
  for (size_t i = 0; i < length; i++) {
    result.push_back(digits[value[i] >> 4]);
    result.push_back(digits[value[i] & 15]);
  }
  return result;
}

static bool constant_time_equal(const std::string &left, const std::string &right) {
  if (left.size() != right.size()) return false;
  unsigned char different = 0;
  for (size_t i = 0; i < left.size(); i++) different |= left[i] ^ right[i];
  return different == 0;
}

void CompanionService::setup() {
  global_companion_service = this;
  this->preferences_ = global_preferences->make_preference<CompanionIdentityPreference>(fnv1a_hash("companion_identity"));
  this->preferences_.load(&this->identity_);
  if (!this->ensure_identity_()) {
    ESP_LOGE(TAG, "Could not create the panel TLS identity");
    this->mark_failed();
    return;
  }
  register_companion_action_sender([this](const std::string &action, const std::string &request) {
    return this->invoke_(action, request);
  });
  register_companion_actions_endpoint();
  if (!this->start_server_()) this->mark_failed();
}

void CompanionService::loop() {
  if (!this->pairing_code_.empty() && static_cast<int32_t>(millis() - this->pairing_expires_at_) >= 0) {
    this->pairing_code_.clear();
    this->pairing_expires_at_ = 0;
  }
  companion_refresh_cards_if_requested();
}

void CompanionService::dump_config() {
  uint8_t fingerprint[32]{};
  mbedtls_sha256(this->identity_.certificate, this->identity_.certificate_len, fingerprint, 0);
  ESP_LOGCONFIG(TAG, "Companion:\n  Secure WebSocket port: %u\n  Device certificate: %s\n  Paired Mac: %s",
                this->port_, hex(fingerprint, sizeof(fingerprint)).c_str(), this->identity_.paired ? "yes" : "no");
}

bool CompanionService::ensure_identity_() {
  if (this->identity_.version == 1 && this->identity_.certificate_len && this->identity_.private_key_len) return true;

  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context drbg;
  mbedtls_pk_context key;
  mbedtls_x509write_cert certificate;
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&drbg);
  mbedtls_pk_init(&key);
  mbedtls_x509write_crt_init(&certificate);
  unsigned char personalization[] = "espcontrol-companion";
  bool success = false;
  do {
    if (mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy,
                              personalization, sizeof(personalization)) != 0) break;
    if (mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)) != 0) break;
    if (mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(key), mbedtls_ctr_drbg_random, &drbg) != 0) break;
    mbedtls_x509write_crt_set_version(&certificate, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&certificate, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_subject_key(&certificate, &key);
    mbedtls_x509write_crt_set_issuer_key(&certificate, &key);
    if (mbedtls_x509write_crt_set_subject_name(&certificate, "CN=EspControl Companion") != 0) break;
    if (mbedtls_x509write_crt_set_issuer_name(&certificate, "CN=EspControl Companion") != 0) break;
    unsigned char serial[] = {static_cast<unsigned char>(esp_random()), static_cast<unsigned char>(esp_random()),
                              static_cast<unsigned char>(esp_random()), static_cast<unsigned char>(esp_random())};
    if (mbedtls_x509write_crt_set_serial_raw(&certificate, serial, sizeof(serial)) != 0) break;
    if (mbedtls_x509write_crt_set_validity(&certificate, "20260101000000", "20360101000000") != 0) break;
    if (mbedtls_x509write_crt_set_basic_constraints(&certificate, 0, -1) != 0) break;
    if (mbedtls_x509write_crt_set_key_usage(&certificate, MBEDTLS_X509_KU_DIGITAL_SIGNATURE | MBEDTLS_X509_KU_KEY_ENCIPHERMENT) != 0) break;

    std::array<uint8_t, 1024> certificate_buffer{};
    const int certificate_length = mbedtls_x509write_crt_der(&certificate, certificate_buffer.data(), certificate_buffer.size(),
                                                               mbedtls_ctr_drbg_random, &drbg);
    if (certificate_length <= 0 || certificate_length > static_cast<int>(sizeof(this->identity_.certificate))) break;
    std::array<uint8_t, 384> key_buffer{};
    const int key_length = mbedtls_pk_write_key_der(&key, key_buffer.data(), key_buffer.size());
    if (key_length <= 0 || key_length > static_cast<int>(sizeof(this->identity_.private_key))) break;
    this->identity_ = {};
    this->identity_.version = 1;
    this->identity_.certificate_len = certificate_length;
    this->identity_.private_key_len = key_length;
    std::memcpy(this->identity_.certificate, certificate_buffer.data() + certificate_buffer.size() - certificate_length, certificate_length);
    std::memcpy(this->identity_.private_key, key_buffer.data() + key_buffer.size() - key_length, key_length);
    success = this->preferences_.save(&this->identity_);
  } while (false);
  mbedtls_x509write_crt_free(&certificate);
  mbedtls_pk_free(&key);
  mbedtls_ctr_drbg_free(&drbg);
  mbedtls_entropy_free(&entropy);
  return success;
}

bool CompanionService::start_server_() {
  httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
  config.httpd.max_open_sockets = 2;
  config.httpd.close_fn = &CompanionService::session_close_;
  config.port_secure = this->port_;
  config.httpd.global_user_ctx = this;
  config.servercert = this->identity_.certificate;
  config.servercert_len = this->identity_.certificate_len;
  config.prvtkey_pem = this->identity_.private_key;
  config.prvtkey_len = this->identity_.private_key_len;
  if (httpd_ssl_start(&this->server_, &config) != ESP_OK) return false;
  const httpd_uri_t websocket = {
      .uri = "/companion/v1", .method = HTTP_GET, .handler = &CompanionService::websocket_handler_, .user_ctx = this,
      .is_websocket = true, .handle_ws_control_frames = true};
  return httpd_register_uri_handler(this->server_, &websocket) == ESP_OK;
}

void CompanionService::session_close_(httpd_handle_t server, int socket_fd) {
  (void) server;
  if (global_companion_service &&
      socket_fd == global_companion_service->authenticated_socket_) {
    global_companion_service->set_connected_(false);
  }
  shutdown(socket_fd, SHUT_RD);
  close(socket_fd);
}

esp_err_t CompanionService::websocket_handler_(httpd_req_t *request) {
  auto *service = static_cast<CompanionService *>(request->user_ctx);
  return service ? service->handle_websocket_(request) : ESP_FAIL;
}

esp_err_t CompanionService::handle_websocket_(httpd_req_t *request) {
  const int socket_fd = httpd_req_to_sockfd(request);
  if (request->method == HTTP_GET) {
    this->send_(socket_fd, "HELLO|1");
    return ESP_OK;
  }
  httpd_ws_frame_t frame{};
  if (httpd_ws_recv_frame(request, &frame, 0) != ESP_OK || frame.len > 1024) return ESP_FAIL;
  std::vector<uint8_t> payload(frame.len + 1, 0);
  frame.payload = payload.data();
  if (httpd_ws_recv_frame(request, &frame, frame.len) != ESP_OK) return ESP_FAIL;
  if (frame.type == HTTPD_WS_TYPE_CLOSE) {
    if (socket_fd == this->authenticated_socket_) this->set_connected_(false);
    return ESP_OK;
  }
  if (frame.type != HTTPD_WS_TYPE_TEXT) return ESP_OK;
  this->handle_message_(socket_fd, reinterpret_cast<const char *>(payload.data()));
  return ESP_OK;
}

bool CompanionService::authenticate_(const std::vector<std::string> &parts) {
  // AUTH|sequence|nonce|hmac-sha256(AUTH|sequence|nonce). Sequence is strictly
  // increasing, so a captured authenticated frame cannot be replayed.
  if (!this->identity_.paired || parts.size() != 4 || parts[0] != "AUTH") return false;
  if (!safe_field(parts[1], 10) || !safe_field(parts[2], 96) || parts[3].size() != 64) return false;
  const uint32_t sequence = strtoul(parts[1].c_str(), nullptr, 10);
  if (sequence <= this->last_sequence_) return false;
  std::string signed_message = parts[0] + "|" + parts[1] + "|" + parts[2];
  uint8_t digest[32]{};
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), this->identity_.credential, sizeof(this->identity_.credential),
                  reinterpret_cast<const unsigned char *>(signed_message.data()), signed_message.size(), digest);
  if (!constant_time_equal(hex(digest, sizeof(digest)), parts[3])) return false;
  this->last_sequence_ = sequence;
  return true;
}

std::string CompanionService::pairing_verification_code() const {
  uint8_t fingerprint[32]{};
  mbedtls_sha256(this->identity_.certificate, this->identity_.certificate_len,
                fingerprint, 0);
  std::string code = hex(fingerprint, 6);
  code.insert(8, "-");
  code.insert(4, "-");
  return code;
}

void CompanionService::handle_message_(int socket_fd, const std::string &message) {
  const auto parts = split(message, '|');
  if (parts.empty()) return;
  if (parts[0] == "AUTH") {
    if (!this->authenticate_(parts)) {
      this->send_(socket_fd, "ERROR|authentication_failed");
      return;
    }
    if (this->authenticated_socket_ != -1 && this->authenticated_socket_ != socket_fd) httpd_sess_trigger_close(this->server_, this->authenticated_socket_);
    this->authenticated_socket_ = socket_fd;
    this->set_connected_(true);
    this->send_(socket_fd, "AUTHENTICATED|1");
    this->publish_catalogue_();
    return;
  }
  if (parts[0] == "PAIR") {
    // Pairing is permitted only during the physical five-minute window. The
    // Mac receives a fresh credential only inside this TLS connection, then
    // stores it in Keychain and pins this certificate's fingerprint.
    if (!this->pairing_active() || millis() < this->next_attempt_at_ || parts.size() != 2 || !constant_time_equal(parts[1], this->pairing_code_)) {
      this->failed_attempts_++;
      this->next_attempt_at_ = millis() + RETRY_DELAY_MS;
      this->send_(socket_fd, "ERROR|pairing_failed");
      return;
    }
    const int previous_socket = this->authenticated_socket_;
    this->set_connected_(false);
    if (previous_socket >= 0 && previous_socket != socket_fd)
      httpd_sess_trigger_close(this->server_, previous_socket);
    for (auto &byte : this->identity_.credential) byte = static_cast<uint8_t>(esp_random());
    this->identity_.paired = 1;
    this->last_sequence_ = 0;
    this->preferences_.save(&this->identity_);
    this->pairing_code_.clear();
    this->pairing_expires_at_ = 0;
    this->failed_attempts_ = 0;
    this->send_(socket_fd, "PAIRED|" + hex(this->identity_.credential, sizeof(this->identity_.credential)));
    return;
  }
  if (socket_fd != this->authenticated_socket_) {
    this->send_(socket_fd, "ERROR|authenticate_first");
    return;
  }
  if (parts[0] == "CATALOG" && (parts.size() == 1 || parts.size() == 2)) {
    const auto catalogue = parts.size() == 2 ? split(parts[1], ',') : std::vector<std::string>{};
    std::vector<CompanionAction> actions;
    for (const auto &entry : catalogue) {
      const auto item = split(entry, ':');
      if (item.size() == 2 && safe_field(item[0], 96) && safe_field(item[1], 96)) actions.push_back({item[0], item[1]});
    }
    companion_set_actions(std::move(actions));
    this->send_(socket_fd, "RESULT|catalogue|ok");
  } else if (parts[0] == "RESULT") {
    // Result messages are intentionally shown only in diagnostics/logging;
    // cards do not optimistically claim a Mac app opened.
    ESP_LOGD(TAG, "Mac result: %s", message.c_str());
  } else if (parts[0] == "HEARTBEAT") {
    this->send_(socket_fd, "HEARTBEAT|ok");
  }
}

void CompanionService::send_(int socket_fd, const std::string &message) {
  if (!this->server_ || socket_fd < 0) return;
  httpd_ws_frame_t frame{};
  frame.type = HTTPD_WS_TYPE_TEXT;
  frame.payload = reinterpret_cast<uint8_t *>(const_cast<char *>(message.data()));
  frame.len = message.size();
  httpd_ws_send_frame_async(this->server_, socket_fd, &frame);
}

void CompanionService::set_connected_(bool connected) {
  if (!connected) this->authenticated_socket_ = -1;
  companion_set_connected(connected);
  if (!connected) companion_set_actions({});
}

void CompanionService::publish_catalogue_() { this->send_(this->authenticated_socket_, "CATALOGUE|requested"); }

bool CompanionService::invoke_(const std::string &action_id, const std::string &request_id) {
  if (this->authenticated_socket_ < 0 || !safe_field(action_id, 96) || !safe_field(request_id, 64)) return false;
  this->send_(this->authenticated_socket_, "INVOKE|" + request_id + "|" + action_id);
  return true;
}

void CompanionService::begin_pairing() {
  this->pairing_code_.clear();
  for (size_t i = 0; i < 8; i++) {
    if (i == 4) this->pairing_code_ += '-';
    this->pairing_code_ += PAIRING_ALPHABET[esp_random() % (sizeof(PAIRING_ALPHABET) - 1)];
  }
  this->pairing_expires_at_ = millis() + PAIRING_WINDOW_MS;
  this->next_attempt_at_ = 0;
}

bool CompanionService::pairing_active() const {
  return !this->pairing_code_.empty() &&
    static_cast<int32_t>(millis() - this->pairing_expires_at_) < 0;
}

void CompanionService::revoke_pairing() {
  this->identity_.paired = 0;
  std::fill(this->identity_.credential, this->identity_.credential + sizeof(this->identity_.credential), 0);
  this->preferences_.save(&this->identity_);
  const int previous_socket = this->authenticated_socket_;
  this->set_connected_(false);
  if (previous_socket >= 0) httpd_sess_trigger_close(this->server_, previous_socket);
}

void begin_companion_pairing() { if (global_companion_service) global_companion_service->begin_pairing(); }
std::string companion_pairing_code() { return global_companion_service ? global_companion_service->pairing_code() : ""; }
std::string companion_pairing_verification_code() {
  return global_companion_service ? global_companion_service->pairing_verification_code() : "";
}
bool companion_pairing_active() { return global_companion_service && global_companion_service->pairing_active(); }
void revoke_companion_pairing() { if (global_companion_service) global_companion_service->revoke_pairing(); }

}  // namespace esphome::companion
