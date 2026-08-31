#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"

#include <esp_https_server.h>

#include <array>
#include <string>
#include <vector>

namespace esphome::companion {

/** Persistent device identity and paired-Mac credential.
 *
 * The certificate is made on the panel's first boot and stored in ESPHome's
 * preference store.  The Mac pins its SHA-256 fingerprint after pairing, so a
 * later connection cannot silently move to another panel.
 */
struct CompanionIdentityPreference {
  uint32_t version{1};
  uint16_t certificate_len{0};
  uint16_t private_key_len{0};
  uint8_t certificate[1024]{};
  uint8_t private_key[384]{};
  uint8_t credential[32]{};
  uint8_t paired{0};
};

class CompanionService final : public Component {
 public:
  void set_port(uint16_t port) { this->port_ = port; }
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Called by the panel web setup page. It rotates the eight-letter setup code
  // and invalidates any unfinished attempt. The resulting trust is persistent.
  void begin_pairing();
  const std::string &pairing_code() const { return this->pairing_code_; }
  bool pairing_active() const;
  uint32_t pairing_expires_in_seconds() const;
  bool paired() const { return this->identity_.paired != 0; }
  void revoke_pairing();

 protected:
  static esp_err_t websocket_handler_(httpd_req_t *request);
  static void session_close_(httpd_handle_t server, int socket_fd);
  esp_err_t handle_websocket_(httpd_req_t *request);
  bool start_server_();
  bool ensure_identity_();
  bool authenticate_(const std::vector<std::string> &parts);
  void handle_message_(int socket_fd, const std::string &message);
  void handle_json_(int socket_fd, const std::string &message);
  void handle_binary_(int socket_fd, const uint8_t *data, size_t size);
  void reset_artwork_transfer_(const char *reason = nullptr, bool notify = false);
  void send_artwork_ack_(uint32_t generation, size_t next_offset);
  void expire_now_playing_();
  void send_(int socket_fd, const std::string &message);
  void set_connected_(bool connected);
  void publish_catalogue_();
  bool invoke_(const std::string &action_id, const std::string &request_id);
  bool invoke_url_(const std::string &app_id, const std::string &encoded_url,
                   const std::string &request_id);

  ESPPreferenceObject preferences_;
  CompanionIdentityPreference identity_{};
  httpd_handle_t server_{nullptr};
  uint16_t port_{8443};
  int authenticated_socket_{-1};
  uint32_t last_sequence_{0};
  uint32_t pairing_expires_at_{0};
  uint32_t next_attempt_at_{0};
  uint8_t failed_attempts_{0};
  std::string pairing_code_;
  RAMAllocator<uint8_t> artwork_allocator_{};
  uint8_t *artwork_buffer_{nullptr};
  size_t artwork_length_{0};
  size_t artwork_offset_{0};
  uint32_t artwork_generation_{0};
  std::array<uint8_t, 32> artwork_sha256_{};
  uint32_t now_playing_generation_{0};
  uint32_t disconnect_grace_expires_at_{0};
};

// The display owns the interaction. These narrow helpers avoid exposing the
// WebSocket service to the generic grid code.
void begin_companion_pairing();
std::string companion_pairing_code();
bool companion_pairing_active();
void revoke_companion_pairing();

}  // namespace esphome::companion
