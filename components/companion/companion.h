#pragma once

#include "esphome/core/component.h"
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

  // Called by the panel web setup page. It rotates the
  // eight-letter code, invalidates any unfinished attempt and expires quickly.
  void begin_pairing();
  const std::string &pairing_code() const { return this->pairing_code_; }
  std::string pairing_verification_code() const;
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
  void send_(int socket_fd, const std::string &message);
  void set_connected_(bool connected);
  void publish_catalogue_();
  bool invoke_(const std::string &action_id, const std::string &request_id);

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
};

// The display owns the interaction. These narrow helpers avoid exposing the
// WebSocket service to the generic grid code.
void begin_companion_pairing();
std::string companion_pairing_code();
std::string companion_pairing_verification_code();
bool companion_pairing_active();
void revoke_companion_pairing();

}  // namespace esphome::companion
