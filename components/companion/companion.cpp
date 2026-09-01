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
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

namespace esphome::companion {

static const char *const TAG = "companion";
static CompanionService *global_companion_service = nullptr;
static constexpr uint32_t PAIRING_WINDOW_MS = 15 * 60 * 1000;
static constexpr uint32_t RETRY_DELAY_MS = 30 * 1000;
static constexpr size_t MAX_WEBSOCKET_FRAME_BYTES = 16 * 1024;
static constexpr size_t MAX_CATALOGUE_ACTIONS = 256;
static constexpr size_t MAX_NOW_PLAYING_FIELD_BYTES = protocol::MAX_TEXT_FIELD_BYTES;
static constexpr size_t MAX_ARTWORK_BYTES = protocol::MAX_ARTWORK_BYTES;
static constexpr size_t MAX_ARTWORK_CHUNK_BYTES = protocol::MAX_ARTWORK_CHUNK_BYTES;
static constexpr uint32_t NOW_PLAYING_RECONNECT_GRACE_MS = 5000;
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
  register_companion_action_sender([this](const std::string &action, const std::string &request) {
    return this->invoke_(action, request);
  });
  register_companion_url_sender([this](const std::string &app, const std::string &url,
                                       const std::string &request) {
    return this->invoke_url_(app, url, request);
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
      this->pairing_active() ? this->pairing_code() : "",
      App.get_name() + ".local",
    };
  };
  register_companion_pairing_callbacks(pairing_snapshot, [this, pairing_snapshot]() {
    this->begin_pairing();
    return pairing_snapshot();
  });
  register_companion_actions_endpoint();
  if (!this->start_server_()) this->mark_failed();
}

void CompanionService::loop() {
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
    this->send_(socket_fd, "HELLO|1");
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
    if (socket_fd == this->authenticated_socket_) this->set_connected_(false);
    return ESP_OK;
  }
  if (frame.type == HTTPD_WS_TYPE_BINARY) {
    if (socket_fd != this->authenticated_socket_) {
      this->send_(socket_fd, "ERROR|authenticate_first");
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
    const std::vector<std::string> &parts, uint32_t &last_sequence) {
  // AUTH|sequence|nonce|hmac-sha256(AUTH|sequence|nonce). Sequence is strictly
  // increasing, so a captured authenticated frame cannot be replayed.
  std::lock_guard<std::mutex> lock(this->pairing_mutex_);
  last_sequence = this->last_sequence_;
  if (!this->identity_.paired || parts.size() != 4 || parts[0] != "AUTH")
    return AuthenticationResult::FAILED;
  if (!safe_field(parts[1], 10) || !safe_field(parts[2], 96) || parts[3].size() != 64)
    return AuthenticationResult::FAILED;
  const uint32_t sequence = strtoul(parts[1].c_str(), nullptr, 10);
  std::string signed_message = parts[0] + "|" + parts[1] + "|" + parts[2];
  uint8_t digest[32]{};
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), this->identity_.credential, sizeof(this->identity_.credential),
                  reinterpret_cast<const unsigned char *>(signed_message.data()), signed_message.size(), digest);
  if (!constant_time_equal(hex(digest, sizeof(digest)), parts[3]))
    return AuthenticationResult::FAILED;
  if (sequence <= this->last_sequence_) return AuthenticationResult::STALE_SEQUENCE;
  this->last_sequence_ = sequence;
  last_sequence = sequence;
  return AuthenticationResult::AUTHENTICATED;
}

void CompanionService::handle_message_(int socket_fd, const std::string &message) {
  if (!message.empty() && message.front() == '{') {
    if (socket_fd != this->authenticated_socket_) {
      this->send_(socket_fd, "ERROR|authenticate_first");
      return;
    }
    this->handle_json_(socket_fd, message);
    return;
  }
  const auto parts = split(message, '|');
  if (parts.empty()) return;
  if (parts[0] == "AUTH") {
    uint32_t last_sequence = 0;
    const auto authentication = this->authenticate_(parts, last_sequence);
    if (authentication == AuthenticationResult::STALE_SEQUENCE) {
      this->send_(socket_fd, "ERROR|authentication_sequence|" + std::to_string(last_sequence));
      return;
    }
    if (authentication != AuthenticationResult::AUTHENTICATED) {
      this->send_(socket_fd, "ERROR|authentication_failed");
      return;
    }
    if (this->authenticated_socket_ != -1 && this->authenticated_socket_ != socket_fd) httpd_sess_trigger_close(this->server_, this->authenticated_socket_);
    this->authenticated_socket_ = socket_fd;
    this->set_connected_(true);
    {
      std::lock_guard<std::mutex> lock(this->pairing_mutex_);
      this->pairing_code_.clear();
      this->pairing_expires_at_ = 0;
      this->next_attempt_at_ = 0;
    }
    this->send_(socket_fd, "AUTHENTICATED|1");
    this->publish_catalogue_();
    return;
  }
  if (parts[0] == "PAIR") {
    // Pairing is permitted only during the setup window opened from the panel
    // webserver. The trusted credential itself does not expire.
    // Mac receives a fresh credential only inside this TLS connection, then
    // stores it in Keychain and pins this certificate's fingerprint.
    if (parts.size() != 2) {
      this->send_(socket_fd, "ERROR|pairing_failed");
      return;
    }
    const uint32_t now = millis();
    std::unique_lock<std::mutex> pairing_lock(this->pairing_mutex_);
    if (!this->pairing_active_locked_(now)) {
      this->send_(socket_fd, "ERROR|pairing_failed");
      return;
    }
    if (this->next_attempt_at_ != 0 && static_cast<int32_t>(now - this->next_attempt_at_) < 0) {
      this->send_(socket_fd, "ERROR|pairing_throttled");
      return;
    }
    if (!constant_time_equal(parts[1], this->pairing_code_)) {
      this->failed_attempts_++;
      this->next_attempt_at_ = now + RETRY_DELAY_MS;
      this->send_(socket_fd, "ERROR|pairing_failed");
      return;
    }
    const int previous_socket = this->authenticated_socket_;
    this->set_connected_(false);
    if (previous_socket >= 0 && previous_socket != socket_fd)
      httpd_sess_trigger_close(this->server_, previous_socket);
    const auto previous_identity = this->identity_;
    for (auto &byte : this->identity_.credential) byte = static_cast<uint8_t>(esp_random());
    this->identity_.paired = 1;
    this->last_sequence_ = 0;
    if (!this->preferences_.save(&this->identity_)) {
      this->identity_ = previous_identity;
      ESP_LOGE(TAG, "Could not persist the paired Mac credential");
      this->send_(socket_fd, "ERROR|pairing_storage_failed");
      return;
    }
    this->pairing_code_.clear();
    this->pairing_expires_at_ = 0;
    this->next_attempt_at_ = 0;
    this->failed_attempts_ = 0;
    pairing_lock.unlock();
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
      if (actions.size() >= MAX_CATALOGUE_ACTIONS) break;
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

void CompanionService::handle_json_(int socket_fd, const std::string &message) {
  bool parsed = json::parse_json(message, [this, socket_fd](JsonObject root) -> bool {
    const std::string type = root["type"] | "";
    const uint32_t version = root["version"] | 0;
    const uint32_t generation = root["generation"] | 0;
    if (version != 2 || generation == 0) return false;

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
      this->now_playing_generation_ = generation;
      this->now_playing_artwork_follows_ = snapshot.artwork_follows;
      this->defer([snapshot = std::move(snapshot)]() mutable {
        companion_set_now_playing(std::move(snapshot));
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
    this->send_(socket_fd, "ERROR|invalid_json_message");
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
  this->send_(this->authenticated_socket_, "{\"type\":\"artwork.ack\",\"version\":2,\"generation\":" +
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
  if (notify && generation != 0 && this->authenticated_socket_ >= 0) {
    this->send_(this->authenticated_socket_, "{\"type\":\"artwork.abort\",\"version\":2,\"generation\":" +
        std::to_string(generation) + "}");
  }
}

void CompanionService::expire_now_playing_() {
  this->reset_artwork_transfer_("connection grace period expired");
  auto snapshot = companion_runtime_snapshot().now_playing;
  snapshot.playback_state = CompanionPlaybackState::UNAVAILABLE;
  snapshot.artwork_follows = false;
  companion_set_now_playing(std::move(snapshot));
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
  if (connected) {
    this->disconnect_grace_expires_at_.store(0);
  } else {
    this->reset_artwork_transfer_("connection closed");
    this->disconnect_grace_expires_at_.store(millis() + NOW_PLAYING_RECONNECT_GRACE_MS);
  }
  companion_set_connected(connected);
  if (!connected) companion_set_actions({});
}

void CompanionService::publish_catalogue_() { this->send_(this->authenticated_socket_, "CATALOGUE|requested"); }

bool CompanionService::invoke_(const std::string &action_id, const std::string &request_id) {
  if (this->authenticated_socket_ < 0 || !safe_field(action_id, 96) || !safe_field(request_id, 64)) return false;
  this->send_(this->authenticated_socket_, "INVOKE|" + request_id + "|" + action_id);
  return true;
}

bool CompanionService::invoke_url_(const std::string &app_id, const std::string &encoded_url,
                                   const std::string &request_id) {
  if (this->authenticated_socket_ < 0 || !safe_field(app_id, 96) ||
      !safe_field(encoded_url, 128) || !safe_field(request_id, 64)) return false;
  this->send_(this->authenticated_socket_,
              "OPEN_URL|" + request_id + "|" + app_id + "|" + encoded_url);
  return true;
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
  if (this->authenticated_socket_ < 0 || this->now_playing_generation_ == 0) return;
  this->send_(this->authenticated_socket_,
              "{\"type\":\"artwork.request\",\"version\":2,\"generation\":" +
              std::to_string(this->now_playing_generation_) + "}");
}

void CompanionService::revoke_pairing() {
  std::lock_guard<std::mutex> lock(this->pairing_mutex_);
  const int previous_socket = this->authenticated_socket_;
  this->identity_.paired = 0;
  std::fill(this->identity_.credential, this->identity_.credential + sizeof(this->identity_.credential), 0);
  this->preferences_.save(&this->identity_);
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
