#ifndef ESPHOME_HA_MEDIA_SOURCE_H
#define ESPHOME_HA_MEDIA_SOURCE_H

#pragma once

// Home Assistant media-source client.
//
// Browsing a media-source folder is a websocket-only Home Assistant API, so this
// component holds an authenticated websocket connection alongside the ESPHome
// native API connection and issues two commands:
//
//   media_source/browse_media   folder -> children (one level, no pagination)
//   media_source/resolve_media  item   -> signed relative URL, valid 24h
//
// The signed URL needs no Authorization header, so the resolved URL can be handed
// straight to artwork_image for download. Ordering and rotation live in the
// host-tested espcontrol::PhotoIndex; this component only fills it and resolves
// on demand.
//
// Threading: esp_websocket_client dispatches events on its own task. Event data
// is copied under a lock and parsed in loop() on the main task, because the JSON
// decode and every callback touch ESPHome and LVGL state.

#ifdef USE_ESP_IDF

#include <functional>
#include <string>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/json/json_util.h"
#include "esp_websocket_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esphome/components/espcontrol/photo_screensaver.h"

namespace esphome {
namespace ha_media_source {

// The photo index holds hundreds of long media_content_ids for a real album;
// keep both the strings and their vector in PSRAM-first storage so the index
// never leans on the small internal heap.
using PsramString = std::basic_string<char, std::char_traits<char>, RAMAllocator<char>>;
using PsramPhotoIndex = espcontrol::BasicPhotoIndex<PsramString, RAMAllocator<PsramString>>;

enum class ConnectionState : uint8_t {
  DISCONNECTED,
  CONNECTING,
  AUTHENTICATING,
  READY,
  FAILED_AUTH,
};

class HaMediaSource : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  // Connection details come from the home_assistant_url / home_assistant_token
  // text entities, so they arrive after boot and can change at runtime.
  void set_connection(const std::string &url, const std::string &token);

  // media_content_id of the folder to rotate through.
  void set_folder(const std::string &media_content_id);
  const std::string &folder() const { return this->folder_; }

  ConnectionState state() const { return this->state_; }
  bool ready() const { return this->state_ == ConnectionState::READY; }
  bool auth_rejected() const { return this->state_ == ConnectionState::FAILED_AUTH; }

  const PsramPhotoIndex &index() const { return this->index_; }
  // Non-const access for the rotation adapter, which advances the cursor.
  PsramPhotoIndex &mutable_index() { return this->index_; }

  // Re-browse the configured folder. Results land in index(); on_index_ready
  // fires when the browse completes (with an empty index if the folder held no
  // images, so callers can distinguish "done, empty" from "still loading").
  void refresh_index();
  bool index_loading() const { return this->browse_request_id_ != 0; }

  void add_on_index_ready_callback(std::function<void()> &&callback) {
    this->index_ready_callback_.add(std::move(callback));
  }

  // Resolve a media_content_id to an absolute, signed, directly-downloadable URL.
  // The callback receives an empty string when the resolve failed.
  void resolve(const std::string &media_content_id,
               std::function<void(const std::string &)> &&callback);

 protected:
  static void websocket_event_handler(void *handler_args, esp_event_base_t base,
                                      int32_t event_id, void *event_data);

  void connect_();
  void disconnect_();
  void send_auth_();
  void send_browse_();
  void handle_message_(const char *data, size_t len);
  void handle_browse_result_(JsonObjectConst root);
  void handle_resolve_result_(int request_id, JsonObjectConst root);
  bool send_json_(const std::string &json);
  void fail_pending_browse_();
  int next_request_id_() { return ++this->request_id_; }
  void fail_pending_resolves_();

  esp_websocket_client_handle_t client_{nullptr};
  ConnectionState state_{ConnectionState::DISCONNECTED};

  std::string url_;
  std::string token_;
  std::string base_url_;  // http(s):// origin, for joining signed paths
  std::string folder_;

  PsramPhotoIndex index_;
  CallbackManager<void()> index_ready_callback_;

  int request_id_{0};
  int browse_request_id_{0};

  struct PendingResolve {
    int request_id;
    std::function<void(const std::string &)> callback;
  };
  std::vector<PendingResolve> pending_resolves_;

  // Filled on the websocket task, drained on the main task. A browse of a large
  // album can be hundreds of kilobytes, so payloads live in PSRAM-first buffers
  // (internal heap on these panels has only tens of kilobytes free) and any
  // message beyond the cap is skipped and reported instead of exhausting RAM.
  using PayloadBuffer = std::vector<char, RAMAllocator<char>>;
  static constexpr size_t kMaxMessageBytes = 2 * 1024 * 1024;

  SemaphoreHandle_t rx_lock_{nullptr};
  std::vector<PayloadBuffer> rx_queue_;
  PayloadBuffer rx_partial_;
  bool rx_skip_current_{false};
  bool rx_oversize_notice_{false};
  bool pending_connect_{false};
  bool pending_disconnect_{false};

  uint32_t last_connect_attempt_{0};
  bool refresh_when_ready_{false};
};

}  // namespace ha_media_source
}  // namespace esphome

#endif  // USE_ESP_IDF

#endif  // ESPHOME_HA_MEDIA_SOURCE_H
