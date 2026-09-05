#include "companion.h"
#include "now_playing_protocol.h"

#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/components/json/json_util.h"

#include "../espcontrol/companion_controls.h"

#include <esp_random.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>
#include <mbedtls/x509_crt.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

namespace esphome::companion {

static const char *const TAG = "companion";
static CompanionService *global_companion_service = nullptr;
static constexpr uint32_t PAIRING_WINDOW_MS = COMPANION_PAIRING_WINDOW_SECONDS * 1000;
static constexpr uint32_t RETRY_DELAY_MS = 30 * 1000;
static constexpr size_t MAX_WEBSOCKET_FRAME_BYTES = COMPANION_MAXIMUM_TEXT_FRAME_BYTES;
static constexpr size_t MAX_CATALOGUE_ACTIONS = 256;
static constexpr size_t MAX_NOW_PLAYING_FIELD_BYTES = protocol::MAX_TEXT_FIELD_BYTES;
static constexpr size_t MAX_ARTWORK_BYTES = COMPANION_MAXIMUM_ARTWORK_BYTES;
static constexpr size_t MAX_ARTWORK_CHUNK_BYTES = COMPANION_ARTWORK_CHUNK_BYTES;
static constexpr uint32_t NOW_PLAYING_RECONNECT_GRACE_MS = 5000;
static constexpr uint32_t AUTHENTICATION_TIMEOUT_MS = 15 * 1000;
static constexpr char PAIRING_ALPHABET[] = "ABCDEFGHJKLMNPQRSTUVWXYZ";

static bool safe_field(const std::string &value, size_t limit) {
  if (value.empty() || value.size() > limit) return false;
  return std::all_of(value.begin(), value.end(), [](unsigned char byte) {
    return byte >= 0x20 && byte <= 0x7e && byte != '|' && byte != ',';
  });
}

static bool safe_utf8_field(const std::string &value, size_t limit) {
  if (value.empty() || value.size() > limit) return false;
  return std::all_of(value.begin(), value.end(), [](unsigned char byte) {
    return byte >= 0x20 && byte != '|' && byte != ',';
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

static bool parse_hex_sha256(const std::string &value, std::array<uint8_t, 32> &result) {
  if (value.size() != 64) return false;
  auto nibble = [](char byte) -> int {
    if (byte >= '0' && byte <= '9') return byte - '0';
    if (byte >= 'a' && byte <= 'f') return byte - 'a' + 10;
    if (byte >= 'A' && byte <= 'F') return byte - 'A' + 10;
    return -1;
  };
  for (size_t i = 0; i < result.size(); i++) {
    const int high = nibble(value[i * 2]);
    const int low = nibble(value[i * 2 + 1]);
    if (high < 0 || low < 0) return false;
    result[i] = static_cast<uint8_t>((high << 4) | low);
  }
  return true;
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
  this->sequence_preferences_ =
      global_preferences->make_preference<uint32_t>(fnv1a_hash("companion_auth_sequence"));
  if (!this->identity_.paired || !this->sequence_preferences_.load(&this->last_sequence_))
    this->last_sequence_ = 0;
  register_companion_action_sender([this](const std::string &action, const std::string &request) {
    return this->invoke_(action, request);
  });
  register_companion_url_sender([this](const std::string &app, const std::string &url,
                                       const std::string &request) {
    return this->invoke_url_(app, url, request);
  });
  register_companion_value_sender([this](const std::string &control, int value,
                                         const std::string &request) {
    return this->invoke_value_(control, value, request);
  });
  auto pairing_snapshot = [this]() {
    const auto runtime = companion_runtime_snapshot();
    return CompanionPairingSnapshot{
      true,
      this->pairing_active(),
      this->paired(),
      runtime.connected,
      this->pairing_expires_in_seconds(),
      this->port_,
      runtime.system_metrics.generation,
      this->pairing_active() ? this->pairing_code() : "",
      App.get_name() + ".local",
    };
  };
  register_companion_pairing_provider(pairing_snapshot);
  register_companion_actions_endpoint();
  if (!this->start_server_()) this->mark_failed();
}

void CompanionService::loop() {
  companion_expire_action_results(millis());
  {
    std::lock_guard<std::mutex> lock(this->pairing_mutex_);
    if (!this->pairing_code_.empty() && static_cast<int32_t>(millis() - this->pairing_expires_at_) >= 0) {
      this->pairing_code_.clear();
      this->pairing_expires_at_ = 0;
      this->next_attempt_at_ = 0;
    }
  }
  const uint32_t disconnect_deadline = this->disconnect_grace_expires_at_.load();
  if (disconnect_deadline != 0 &&
      static_cast<int32_t>(millis() - disconnect_deadline) >= 0 &&
      !this->disconnect_expiry_queued_.exchange(true)) {
    if (!this->server_ || httpd_queue_work(this->server_, &CompanionService::disconnect_expiry_work_, this) != ESP_OK)
      this->disconnect_expiry_queued_.store(false);
  }
  const uint32_t authentication_deadline = this->authentication_expires_at_.load();
  if (authentication_deadline != 0 &&
      static_cast<int32_t>(millis() - authentication_deadline) >= 0 &&
      !this->authentication_expiry_queued_.exchange(true)) {
    if (!this->server_ || httpd_queue_work(this->server_, &CompanionService::authentication_expiry_work_, this) != ESP_OK)
      this->authentication_expiry_queued_.store(false);
  }
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
  // Never evict an authenticated Companion session just to admit an
  // unauthenticated socket. Excess connections are rejected instead.
  config.httpd.lru_purge_enable = false;
  // The main web UI owns ESP-IDF's default HTTPD control port. Each HTTPD
  // instance needs a distinct internal control socket even when its public
  // listener uses a different port.
  config.httpd.ctrl_port += 1;
  config.httpd.close_fn = &CompanionService::session_close_;
  config.port_secure = this->port_;
  config.httpd.global_user_ctx = this;
  config.servercert = this->identity_.certificate;
  config.servercert_len = this->identity_.certificate_len;
  config.prvtkey_pem = this->identity_.private_key;
  config.prvtkey_len = this->identity_.private_key_len;
  if (httpd_ssl_start(&this->server_, &config) != ESP_OK) return false;
  const httpd_uri_t websocket = {
      .uri = COMPANION_PROTOCOL_PATH, .method = HTTP_GET, .handler = &CompanionService::websocket_handler_, .user_ctx = this,
      .is_websocket = true, .handle_ws_control_frames = false};
  return httpd_register_uri_handler(this->server_, &websocket) == ESP_OK;
}

void CompanionService::session_close_(httpd_handle_t server, int socket_fd) {
  (void) server;
  if (global_companion_service) {
    global_companion_service->forget_unauthenticated_socket_(socket_fd);
    global_companion_service->set_connected_(false, socket_fd);
  }
  shutdown(socket_fd, SHUT_RD);
  close(socket_fd);
}

void CompanionService::authentication_expiry_work_(void *context) {
  auto *service = static_cast<CompanionService *>(context);
  if (!service) return;
  service->authentication_expiry_queued_.store(false);
  const uint32_t now = millis();
  for (auto &session : service->unauthenticated_sessions_) {
    if (session.socket < 0 || static_cast<int32_t>(now - session.expires_at) < 0) continue;
    const int socket_fd = session.socket;
    session = {};
    session.socket = -1;
    httpd_sess_trigger_close(service->server_, socket_fd);
  }
  service->update_authentication_deadline_();
}

void CompanionService::disconnect_expiry_work_(void *context) {
  auto *service = static_cast<CompanionService *>(context);
  if (!service) return;
  service->disconnect_expiry_queued_.store(false);
  uint32_t deadline = service->disconnect_grace_expires_at_.load();
  if (deadline == 0 || static_cast<int32_t>(millis() - deadline) < 0) return;
  if (service->disconnect_grace_expires_at_.compare_exchange_strong(deadline, 0))
    service->expire_now_playing_();
}

esp_err_t CompanionService::websocket_handler_(httpd_req_t *request) {
  auto *service = static_cast<CompanionService *>(request->user_ctx);
  return service ? service->handle_websocket_(request) : ESP_FAIL;
}

esp_err_t CompanionService::handle_websocket_(httpd_req_t *request) {
  const int socket_fd = httpd_req_to_sockfd(request);
  if (request->method == HTTP_GET) {
    this->track_unauthenticated_socket_(socket_fd);
    this->send_(socket_fd, "{\"type\":\"hello\",\"protocol\":" +
        std::to_string(COMPANION_PROTOCOL_VERSION) + "}");
    return ESP_OK;
  }
  httpd_ws_frame_t frame{};
  if (httpd_ws_recv_frame(request, &frame, 0) != ESP_OK) return ESP_FAIL;
  if (frame.len > MAX_WEBSOCKET_FRAME_BYTES) {
    ESP_LOGW(TAG, "Rejected oversized Companion frame (%u bytes)", static_cast<unsigned>(frame.len));
    return ESP_FAIL;
  }
  std::vector<uint8_t> payload(frame.len + 1, 0);
  frame.payload = payload.data();
  if (httpd_ws_recv_frame(request, &frame, frame.len) != ESP_OK) return ESP_FAIL;
  if (frame.type == HTTPD_WS_TYPE_CLOSE) {
    this->set_connected_(false, socket_fd);
    return ESP_OK;
  }
  if (frame.type == HTTPD_WS_TYPE_BINARY) {
    if (socket_fd != this->session_.authenticated_socket()) {
      this->send_(socket_fd, "{\"type\":\"error\",\"protocol\":" +
          std::to_string(COMPANION_PROTOCOL_VERSION) + ",\"code\":\"authenticate_first\"}");
      this->expire_unauthenticated_socket_(socket_fd);
      return ESP_OK;
    }
    this->handle_binary_(socket_fd, payload.data(), frame.len);
    return ESP_OK;
  }
  if (frame.type != HTTPD_WS_TYPE_TEXT) return ESP_OK;
  this->handle_message_(socket_fd, reinterpret_cast<const char *>(payload.data()));
  return ESP_OK;
}

CompanionService::AuthenticationResult CompanionService::authenticate_(
    uint32_t sequence, const std::string &nonce, const std::string &signature,
    uint32_t &last_sequence) {
  // Sign a stable protocol-v3 authentication payload. The sequence is strictly
  // increasing, so a captured authenticated frame cannot be replayed.
  std::lock_guard<std::mutex> lock(this->pairing_mutex_);
  last_sequence = this->last_sequence_;
  if (!this->identity_.paired || sequence == 0 || !safe_field(nonce, 96) || signature.size() != 64)
    return AuthenticationResult::FAILED;
  const std::string signed_message = "auth.request|" + std::to_string(sequence) + "|" + nonce;
  uint8_t digest[32]{};
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), this->identity_.credential, sizeof(this->identity_.credential),
                  reinterpret_cast<const unsigned char *>(signed_message.data()), signed_message.size(), digest);
  if (!constant_time_equal(hex(digest, sizeof(digest)), signature))
    return AuthenticationResult::FAILED;
  if (sequence <= this->last_sequence_) return AuthenticationResult::STALE_SEQUENCE;
  if (!this->sequence_preferences_.save(&sequence)) {
    ESP_LOGE(TAG, "Could not persist the Companion authentication sequence");
    return AuthenticationResult::FAILED;
  }
  this->last_sequence_ = sequence;
  last_sequence = sequence;
  return AuthenticationResult::AUTHENTICATED;
}

void CompanionService::handle_message_(int socket_fd, const std::string &message) {
  if (message.empty() || message.front() != '{') {
    this->send_(socket_fd, "{\"type\":\"error\",\"protocol\":3,\"code\":\"json_required\"}");
    this->expire_unauthenticated_socket_(socket_fd);
    return;
  }
  this->handle_json_(socket_fd, message);
}

void CompanionService::handle_json_(int socket_fd, const std::string &message) {
  bool parsed = json::parse_json(message, [this, socket_fd](JsonObject root) -> bool {
    const std::string type = root["type"] | "";
    const uint32_t version = root["protocol"] | 0;
    if (version != COMPANION_PROTOCOL_VERSION) return false;
    auto send_error = [this, socket_fd](const char *code, uint32_t last_sequence = 0) {
      std::string response = "{\"type\":\"error\",\"protocol\":" +
          std::to_string(COMPANION_PROTOCOL_VERSION) + ",\"code\":\"" + code + "\"";
      if (last_sequence != 0) response += ",\"lastSequence\":" + std::to_string(last_sequence);
      this->send_(socket_fd, response + "}");
    };

    if (type == "auth.request") {
      const uint32_t sequence = root["sequence"] | 0;
      const std::string nonce = root["nonce"] | "";
      const std::string signature = root["signature"] | "";
      uint32_t last_sequence = 0;
      const auto authentication = this->authenticate_(sequence, nonce, signature, last_sequence);
      if (authentication == AuthenticationResult::STALE_SEQUENCE) {
        send_error("authentication_sequence", last_sequence);
        this->expire_unauthenticated_socket_(socket_fd);
        return true;
      }
      if (authentication != AuthenticationResult::AUTHENTICATED) {
        send_error("authentication_failed");
        this->expire_unauthenticated_socket_(socket_fd);
        return true;
      }
      const int previous_socket = this->session_.authenticate(socket_fd);
      if (previous_socket != -1 && previous_socket != socket_fd)
        httpd_sess_trigger_close(this->server_, previous_socket);
      companion_set_media_actions_supported(false);
      this->forget_unauthenticated_socket_(socket_fd);
      this->set_connected_(true);
      {
        std::lock_guard<std::mutex> lock(this->pairing_mutex_);
        this->pairing_code_.clear();
        this->pairing_expires_at_ = 0;
        this->next_attempt_at_ = 0;
      }
      this->send_(socket_fd, "{\"type\":\"auth.accepted\",\"protocol\":" +
          std::to_string(COMPANION_PROTOCOL_VERSION) + ",\"capabilityVersion\":" +
          std::to_string(COMPANION_CAPABILITY_VERSION) + "}");
      this->publish_catalogue_();
      return true;
    }

    if (type == "pair.request") {
      const std::string code = root["code"] | "";
      const uint32_t now = millis();
      std::unique_lock<std::mutex> pairing_lock(this->pairing_mutex_);
      if (!safe_field(code, 16) || !this->pairing_active_locked_(now)) {
        send_error("pairing_failed");
        this->expire_unauthenticated_socket_(socket_fd);
        return true;
      }
      if (this->next_attempt_at_ != 0 && static_cast<int32_t>(now - this->next_attempt_at_) < 0) {
        send_error("pairing_throttled");
        this->expire_unauthenticated_socket_(socket_fd);
        return true;
      }
      if (!constant_time_equal(code, this->pairing_code_)) {
        this->failed_attempts_++;
        this->next_attempt_at_ = now + RETRY_DELAY_MS;
        send_error("pairing_failed");
        this->expire_unauthenticated_socket_(socket_fd);
        return true;
      }
      const int previous_socket = this->session_.authenticated_socket();
      this->set_connected_(false);
      if (previous_socket >= 0 && previous_socket != socket_fd)
        httpd_sess_trigger_close(this->server_, previous_socket);
      const auto previous_identity = this->identity_;
      const uint32_t previous_sequence = this->last_sequence_;
      for (auto &byte : this->identity_.credential) byte = static_cast<uint8_t>(esp_random());
      this->identity_.paired = 1;
      this->last_sequence_ = 0;
      if (!this->preferences_.save(&this->identity_) ||
          !this->sequence_preferences_.save(&this->last_sequence_)) {
        this->identity_ = previous_identity;
        this->last_sequence_ = previous_sequence;
        this->preferences_.save(&this->identity_);
        this->sequence_preferences_.save(&this->last_sequence_);
        send_error("pairing_storage_failed");
        this->expire_unauthenticated_socket_(socket_fd);
        return true;
      }
      this->pairing_code_.clear();
      this->pairing_expires_at_ = 0;
      this->next_attempt_at_ = 0;
      this->failed_attempts_ = 0;
      const std::string credential = hex(this->identity_.credential, sizeof(this->identity_.credential));
      pairing_lock.unlock();
      this->send_(socket_fd, "{\"type\":\"pair.accepted\",\"protocol\":" +
          std::to_string(COMPANION_PROTOCOL_VERSION) + ",\"credential\":\"" + credential + "\"}");
      return true;
    }

    if (socket_fd != this->session_.authenticated_socket()) {
      send_error("authenticate_first");
      this->expire_unauthenticated_socket_(socket_fd);
      return true;
    }

    if (type == "capabilities") {
      bool media_actions = false;
      bool keyboard_actions = false;
      bool keyboard_actions_capability_received = false;
      std::vector<std::string> window_actions;
      const JsonArray values = root["values"].as<JsonArray>();
      for (JsonVariant value : values) {
        const std::string capability = value.as<const char *>();
        if (capability == "media_actions") {
          media_actions = true;
        } else if (capability == "keyboard_shortcuts") {
          keyboard_actions = true;
          keyboard_actions_capability_received = true;
        } else if (capability == "keyboard_shortcuts_unavailable") {
          keyboard_actions = false;
          keyboard_actions_capability_received = true;
        } else if (companion_window_action_valid(capability)) {
          window_actions.push_back(capability);
        }
      }
      companion_set_media_actions_supported(media_actions);
      if (keyboard_actions_capability_received) {
        companion_set_keyboard_actions_supported(keyboard_actions);
      }
      companion_set_window_actions(std::move(window_actions));
      return true;
    }

    if (type == "catalogue.page") {
      const uint32_t catalogue_generation = root["generation"] | 0;
      const uint16_t page = root["page"] | 0;
      const bool complete = root["complete"] | false;
      if (catalogue_generation == 0 ||
          (page == 0 ? false : catalogue_generation != this->catalogue_generation_) ||
          page != (page == 0 ? 0 : this->catalogue_next_page_)) return false;
      if (page == 0) {
        this->catalogue_actions_.clear();
        this->catalogue_generation_ = catalogue_generation;
        this->catalogue_next_page_ = 0;
      }
      const JsonArray items = root["items"].as<JsonArray>();
      for (JsonObject item : items) {
        if (this->catalogue_actions_.size() >= MAX_CATALOGUE_ACTIONS) break;
        const std::string id = item["id"] | "";
        const std::string label = item["label"] | "";
        if (safe_field(id, 96) && safe_utf8_field(label, 96))
          this->catalogue_actions_.emplace_back(id, label);
      }
      this->catalogue_next_page_ = page + 1;
      if (complete) {
        std::vector<CompanionAction> actions;
        actions.reserve(this->catalogue_actions_.size());
        for (const auto &item : this->catalogue_actions_) actions.push_back({item.first, item.second});
        companion_set_actions(std::move(actions));
      }
      return true;
    }

    if (type == "action.result") {
      const std::string request_id = root["requestId"] | "";
      const std::string status = root["status"] | "";
      if (!safe_field(request_id, 64) || !safe_field(status, 32)) return false;
      this->defer([request_id, status]() { companion_deliver_action_result(request_id, status); });
      return true;
    }

    if (type == "value.state") {
      const std::string control_id = root["controlId"] | "";
      if (!companion_volume_control_valid(control_id)) return false;
      if (!(root["available"] | true)) {
        companion_remove_value(control_id);
        return true;
      }
      const int value = root["value"] | -1;
      if (value < 0 || value > 100) return false;
      companion_set_value(control_id, value);
      return true;
    }

    if (type == "focus.changed") {
      const std::string action_id = root["actionId"] | "";
      if (!action_id.empty() && !safe_field(action_id, 96)) return false;
      companion_set_focused_action(action_id);
      return true;
    }

    if (type == "timezone.changed") {
      const std::string timezone = root["identifier"] | "";
      if (!safe_field(timezone, 96)) return false;
      this->defer([timezone]() { companion_set_timezone_id(timezone); });
      return true;
    }

    const uint32_t generation = root["generation"] | 0;
    if (generation == 0) return false;

    if (type == "now_playing") {
      if (generation < this->now_playing_generation_) return true;
      auto text = [&root](const char *key) { return std::string(root[key] | ""); };
      CompanionNowPlayingSnapshot snapshot;
      snapshot.generation = generation;
      snapshot.source_application_id = text("applicationIdentifier");
      snapshot.source_application_name = text("applicationName");
      snapshot.content_id = text("contentIdentifier");
      snapshot.title = text("title");
      snapshot.artist = text("artist");
      snapshot.album = text("album");
      const std::array<const std::string *, 6> fields{{
          &snapshot.source_application_id, &snapshot.source_application_name,
          &snapshot.content_id, &snapshot.title, &snapshot.artist, &snapshot.album}};
      if (std::any_of(fields.begin(), fields.end(), [](const std::string *field) {
            return field->size() > MAX_NOW_PLAYING_FIELD_BYTES;
          })) return false;
      const std::string state = text("state");
      if (state == "playing") snapshot.playback_state = CompanionPlaybackState::PLAYING;
      else if (state == "paused") snapshot.playback_state = CompanionPlaybackState::PAUSED;
      else if (state == "stopped") snapshot.playback_state = CompanionPlaybackState::STOPPED;
      else if (state == "unavailable") snapshot.playback_state = CompanionPlaybackState::UNAVAILABLE;
      else return false;
      const double duration_ms = root["durationMs"] | 0.0;
      const double position_ms = root["positionMs"] | 0.0;
      const double playback_rate = root["playbackRate"] | 0.0;
      if (!std::isfinite(duration_ms) || !std::isfinite(position_ms) ||
          !std::isfinite(playback_rate) || duration_ms < 0 || position_ms < 0 ||
          duration_ms > 86400000.0 || position_ms > 86400000.0 ||
          playback_rate < -16.0 || playback_rate > 16.0) return false;
      snapshot.duration = static_cast<float>(duration_ms / 1000.0);
      snapshot.position = static_cast<float>(position_ms / 1000.0);
      snapshot.playback_rate = static_cast<float>(playback_rate);
      snapshot.artwork_follows = root["hasArtwork"] | false;
      if (generation != this->now_playing_generation_)
        this->reset_artwork_transfer_("new now-playing generation");
      else if (!snapshot.artwork_follows)
        this->reset_artwork_transfer_("artwork removed from current snapshot", true);
      this->now_playing_generation_ = generation;
      this->now_playing_artwork_follows_ = snapshot.artwork_follows;
      this->defer([snapshot = std::move(snapshot)]() mutable {
        companion_set_now_playing(std::move(snapshot));
      });
      return true;
    }

    if (type == "system_metrics") {
      if (!(root["available"] | true)) {
        this->defer([] { companion_set_system_metrics({}); });
        return true;
      }
      CompanionSystemMetricsSnapshot snapshot;
      snapshot.generation = generation;
      snapshot.cpu_usage_percent = root["cpuUsagePercent"] | NAN;
      snapshot.memory_usage_percent = root["memoryUsagePercent"] | NAN;
      snapshot.storage_usage_percent = root["storageUsagePercent"] | NAN;
      snapshot.battery_percent = root["batteryPercent"] | NAN;
      snapshot.network_throughput_kbps = root["networkThroughputKBps"] | NAN;
      const std::array<float, 3> required{{
          snapshot.cpu_usage_percent,
          snapshot.memory_usage_percent,
          snapshot.storage_usage_percent,
      }};
      if (std::any_of(required.begin(), required.end(), [](float value) {
            return !std::isfinite(value) || value < 0.0f || value > 100.0f;
          })) return false;
      if (std::isfinite(snapshot.battery_percent) &&
          (snapshot.battery_percent < 0.0f || snapshot.battery_percent > 100.0f)) return false;
      if (std::isfinite(snapshot.network_throughput_kbps) &&
          (snapshot.network_throughput_kbps < 0.0f || snapshot.network_throughput_kbps > 1.0e9f)) return false;
      this->defer([snapshot]() mutable {
        companion_set_system_metrics(std::move(snapshot));
      });
      return true;
    }

    if (type == "artwork.begin") {
      const size_t length = root["byteLength"] | 0;
      const std::string sha256 = root["sha256"] | "";
      const std::string mime_type = root["mimeType"] | "";
      std::array<uint8_t, 32> expected{};
      const bool hash_valid = parse_hex_sha256(sha256, expected);
      if (!protocol::artwork_begin_valid(true, generation, this->now_playing_generation_,
                                         this->now_playing_artwork_follows_, length,
                                         mime_type == "image/jpeg", hash_valid) ||
          generation != this->now_playing_generation_) return false;
      this->reset_artwork_transfer_("replaced artwork transfer");
      this->artwork_buffer_ = this->artwork_allocator_.allocate(length);
      if (!this->artwork_buffer_) return false;
      this->artwork_length_ = length;
      this->artwork_generation_ = generation;
      this->artwork_sha256_ = expected;
      this->send_artwork_ack_(generation, 0);
      return true;
    }

    if (type == "artwork.end") {
      if (!this->artwork_buffer_ || generation != this->artwork_generation_ ||
          this->artwork_offset_ != this->artwork_length_) return false;
      if (!protocol::jpeg_signature_valid(this->artwork_buffer_, this->artwork_length_)) {
        this->reset_artwork_transfer_("invalid JPEG signature", true);
        return true;
      }
      std::array<uint8_t, 32> actual{};
      mbedtls_sha256(this->artwork_buffer_, this->artwork_length_, actual.data(), 0);
      unsigned char different = 0;
      for (size_t i = 0; i < actual.size(); i++) different |= actual[i] ^ this->artwork_sha256_[i];
      if (different != 0) {
        this->reset_artwork_transfer_("SHA-256 mismatch", true);
        return true;
      }
      uint8_t *owned = this->artwork_buffer_;
      const size_t owned_size = this->artwork_length_;
      this->artwork_buffer_ = nullptr;
      this->artwork_length_ = 0;
      this->artwork_offset_ = 0;
      this->artwork_generation_ = 0;
      this->defer([this, generation, owned, owned_size]() {
        if (!companion_deliver_artwork(generation, owned, owned_size)) {
          this->artwork_allocator_.deallocate(owned, owned_size);
        }
      });
      return true;
    }

    if (type == "artwork.abort") {
      if (generation == this->artwork_generation_) this->reset_artwork_transfer_("Mac aborted transfer");
      return true;
    }
    return false;
  });
  if (!parsed) {
    ESP_LOGW(TAG, "Rejected invalid Companion JSON message");
    this->send_(socket_fd, "{\"type\":\"error\",\"protocol\":" +
        std::to_string(COMPANION_PROTOCOL_VERSION) + ",\"code\":\"invalid_message\"}");
  }
}

void CompanionService::handle_binary_(int socket_fd, const uint8_t *data, size_t size) {
  if (!this->artwork_buffer_ || size < 9 || size > MAX_ARTWORK_CHUNK_BYTES + 8) {
    this->reset_artwork_transfer_("unexpected binary frame", true);
    return;
  }
  const uint32_t generation = (static_cast<uint32_t>(data[0]) << 24) |
                              (static_cast<uint32_t>(data[1]) << 16) |
                              (static_cast<uint32_t>(data[2]) << 8) | data[3];
  const uint32_t offset = (static_cast<uint32_t>(data[4]) << 24) |
                          (static_cast<uint32_t>(data[5]) << 16) |
                          (static_cast<uint32_t>(data[6]) << 8) | data[7];
  const size_t chunk_size = size - 8;
  if (!protocol::artwork_chunk_valid(true, generation, this->artwork_generation_, offset,
                                      this->artwork_offset_, chunk_size,
                                      this->artwork_length_)) {
    this->reset_artwork_transfer_("invalid generation or byte offset", true);
    return;
  }
  std::memcpy(this->artwork_buffer_ + this->artwork_offset_, data + 8, chunk_size);
  this->artwork_offset_ += chunk_size;
  this->send_artwork_ack_(generation, this->artwork_offset_);
  (void) socket_fd;
}

void CompanionService::send_artwork_ack_(uint32_t generation, size_t next_offset) {
  const int socket_fd = this->session_.authenticated_socket();
  this->send_(socket_fd, "{\"type\":\"artwork.ack\",\"protocol\":" +
      std::to_string(COMPANION_PROTOCOL_VERSION) + ",\"generation\":" +
      std::to_string(generation) + ",\"nextOffset\":" + std::to_string(next_offset) + "}");
}

void CompanionService::reset_artwork_transfer_(const char *reason, bool notify) {
  const uint32_t generation = this->artwork_generation_;
  if (this->artwork_buffer_) this->artwork_allocator_.deallocate(this->artwork_buffer_, this->artwork_length_);
  this->artwork_buffer_ = nullptr;
  this->artwork_length_ = 0;
  this->artwork_offset_ = 0;
  this->artwork_generation_ = 0;
  if (reason) ESP_LOGD(TAG, "Artwork transfer reset: %s", reason);
  const int socket_fd = this->session_.authenticated_socket();
  if (notify && generation != 0 && socket_fd >= 0) {
    this->send_(socket_fd, "{\"type\":\"artwork.abort\",\"protocol\":" +
        std::to_string(COMPANION_PROTOCOL_VERSION) + ",\"generation\":" +
        std::to_string(generation) + "}");
  }
}

void CompanionService::expire_now_playing_() {
  this->reset_artwork_transfer_("connection grace period expired");
  auto snapshot = companion_runtime_snapshot().now_playing;
  snapshot.playback_state = CompanionPlaybackState::UNAVAILABLE;
  snapshot.artwork_follows = false;
  this->defer([snapshot = std::move(snapshot)]() mutable {
    companion_set_now_playing(std::move(snapshot));
  });
}

bool CompanionService::send_(int socket_fd, const std::string &message) {
  if (!this->server_ || socket_fd < 0) return false;
  httpd_ws_frame_t frame{};
  frame.type = HTTPD_WS_TYPE_TEXT;
  frame.payload = reinterpret_cast<uint8_t *>(const_cast<char *>(message.data()));
  frame.len = message.size();
  const esp_err_t result = httpd_ws_send_frame_async(this->server_, socket_fd, &frame);
  if (result != ESP_OK) {
    ESP_LOGW(TAG, "Failed to queue Companion message: %s", esp_err_to_name(result));
    return false;
  }
  return true;
}

void CompanionService::track_unauthenticated_socket_(int socket_fd) {
  const uint32_t deadline = millis() + AUTHENTICATION_TIMEOUT_MS;
  for (auto &session : this->unauthenticated_sessions_) {
    if (session.socket == socket_fd) {
      session.expires_at = deadline;
      this->update_authentication_deadline_();
      return;
    }
  }
  for (auto &session : this->unauthenticated_sessions_) {
    if (session.socket < 0) {
      session.socket = socket_fd;
      session.expires_at = deadline;
      this->update_authentication_deadline_();
      return;
    }
  }
  // LRU purging can replace a socket before its close callback is delivered.
  this->unauthenticated_sessions_[0] = {socket_fd, deadline};
  this->update_authentication_deadline_();
}

void CompanionService::forget_unauthenticated_socket_(int socket_fd) {
  for (auto &session : this->unauthenticated_sessions_) {
    if (session.socket == socket_fd) {
      session.socket = -1;
      session.expires_at = 0;
    }
  }
  this->update_authentication_deadline_();
}

void CompanionService::expire_unauthenticated_socket_(int socket_fd) {
  for (auto &session : this->unauthenticated_sessions_) {
    if (session.socket == socket_fd) session.expires_at = millis();
  }
  this->update_authentication_deadline_();
}

void CompanionService::update_authentication_deadline_() {
  uint32_t earliest = 0;
  const uint32_t now = millis();
  for (const auto &session : this->unauthenticated_sessions_) {
    if (session.socket < 0) continue;
    if (earliest == 0 || static_cast<int32_t>(session.expires_at - earliest) < 0)
      earliest = session.expires_at;
  }
  // Preserve an already-expired deadline so loop() queues cleanup immediately.
  this->authentication_expires_at_.store(earliest == 0 ? 0 :
      (static_cast<int32_t>(now - earliest) >= 0 ? now : earliest));
}

void CompanionService::set_connected_(bool connected, int closing_socket) {
  if (!connected) {
    if (closing_socket >= 0) {
      if (!this->session_.disconnect_socket(closing_socket)) return;
    } else {
      this->session_.disconnect();
    }
  }
  if (connected) {
    this->now_playing_generation_ = 0;
    this->disconnect_grace_expires_at_.store(0);
  } else {
    this->reset_artwork_transfer_("connection closed");
    this->disconnect_grace_expires_at_.store(millis() + NOW_PLAYING_RECONNECT_GRACE_MS);
  }
  this->defer([connected]() {
    companion_set_connected(connected);
    if (!connected) {
      companion_set_actions({});
      companion_set_timezone_id("");
    }
  });
}

void CompanionService::publish_catalogue_() {
  this->send_(this->session_.authenticated_socket(),
              "{\"type\":\"catalogue.request\",\"protocol\":" +
              std::to_string(COMPANION_PROTOCOL_VERSION) + "}");
}

bool CompanionService::invoke_(const std::string &action_id, const std::string &request_id) {
  const int socket_fd = this->session_.authenticated_socket();
  if (socket_fd < 0 || !safe_field(action_id, 96) || !safe_field(request_id, 64)) return false;
  return this->send_(socket_fd, "{\"type\":\"action.invoke\",\"protocol\":" +
                     std::to_string(COMPANION_PROTOCOL_VERSION) + ",\"kind\":\"action\",\"requestId\":\"" +
                     request_id + "\",\"actionId\":\"" + action_id + "\"}");
}

bool CompanionService::invoke_url_(const std::string &app_id, const std::string &encoded_url,
                                   const std::string &request_id) {
  const int socket_fd = this->session_.authenticated_socket();
  if (socket_fd < 0 || !safe_field(app_id, 96) ||
      !safe_field(encoded_url, 128) || !safe_field(request_id, 64)) return false;
  return this->send_(socket_fd, "{\"type\":\"action.invoke\",\"protocol\":" +
                     std::to_string(COMPANION_PROTOCOL_VERSION) + ",\"kind\":\"url\",\"requestId\":\"" +
                     request_id + "\",\"appId\":\"" + app_id + "\",\"encodedUrl\":\"" + encoded_url + "\"}");
}

bool CompanionService::invoke_value_(const std::string &control_id, int value,
                                     const std::string &request_id) {
  const int socket_fd = this->session_.authenticated_socket();
  if (socket_fd < 0 ||
      !companion_volume_control_valid(control_id) ||
      value < 0 || value > 100 || !safe_field(request_id, 64)) return false;
  return this->send_(socket_fd, "{\"type\":\"value.set\",\"protocol\":" +
                     std::to_string(COMPANION_PROTOCOL_VERSION) + ",\"requestId\":\"" + request_id +
                     "\",\"controlId\":\"" + control_id + "\",\"value\":" + std::to_string(value) + "}");
}

void CompanionService::begin_pairing() {
  std::lock_guard<std::mutex> lock(this->pairing_mutex_);
  this->pairing_code_.clear();
  for (size_t i = 0; i < 8; i++) {
    if (i == 4) this->pairing_code_ += '-';
    this->pairing_code_ += PAIRING_ALPHABET[esp_random() % (sizeof(PAIRING_ALPHABET) - 1)];
  }
  this->pairing_expires_at_ = millis() + PAIRING_WINDOW_MS;
  this->next_attempt_at_ = 0;
}

bool CompanionService::pairing_active() const {
  std::lock_guard<std::mutex> lock(this->pairing_mutex_);
  return this->pairing_active_locked_(millis());
}

uint32_t CompanionService::pairing_expires_in_seconds() const {
  std::lock_guard<std::mutex> lock(this->pairing_mutex_);
  const uint32_t now = millis();
  if (!this->pairing_active_locked_(now)) return 0;
  return (this->pairing_expires_at_ - now + 999) / 1000;
}

bool CompanionService::pairing_active_locked_(uint32_t now) const {
  return !this->pairing_code_.empty() &&
    static_cast<int32_t>(now - this->pairing_expires_at_) < 0;
}

std::string CompanionService::pairing_code() const {
  std::lock_guard<std::mutex> lock(this->pairing_mutex_);
  return this->pairing_code_;
}

bool CompanionService::paired() const {
  std::lock_guard<std::mutex> lock(this->pairing_mutex_);
  return this->identity_.paired != 0;
}

void CompanionService::request_now_playing_artwork() {
  const int socket_fd = this->session_.authenticated_socket();
  if (socket_fd < 0 || this->now_playing_generation_ == 0) return;
  this->send_(socket_fd,
              "{\"type\":\"artwork.request\",\"protocol\":" +
              std::to_string(COMPANION_PROTOCOL_VERSION) + ",\"generation\":" +
              std::to_string(this->now_playing_generation_) + "}");
}

void CompanionService::revoke_pairing() {
  std::lock_guard<std::mutex> lock(this->pairing_mutex_);
  const int previous_socket = this->session_.authenticated_socket();
  this->identity_.paired = 0;
  this->last_sequence_ = 0;
  std::fill(this->identity_.credential, this->identity_.credential + sizeof(this->identity_.credential), 0);
  this->preferences_.save(&this->identity_);
  this->sequence_preferences_.save(&this->last_sequence_);
  this->set_connected_(false);
  if (previous_socket >= 0) httpd_sess_trigger_close(this->server_, previous_socket);
}

void begin_companion_pairing() { if (global_companion_service) global_companion_service->begin_pairing(); }
std::string companion_pairing_code() { return global_companion_service ? global_companion_service->pairing_code() : ""; }
bool companion_pairing_active() { return global_companion_service && global_companion_service->pairing_active(); }
void revoke_companion_pairing() { if (global_companion_service) global_companion_service->revoke_pairing(); }
void request_companion_now_playing_artwork() {
  if (global_companion_service) global_companion_service->request_now_playing_artwork();
}

}  // namespace esphome::companion
