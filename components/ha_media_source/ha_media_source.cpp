#include "ha_media_source.h"

#ifdef USE_ESP_IDF

#include "esphome/core/log.h"
#include "esphome/components/json/json_util.h"
#include "esp_heap_caps.h"

#include <cstring>

namespace esphome {
namespace ha_media_source {

static const char *const TAG = "ha_media_source";

// Home Assistant closes the socket if a message goes unanswered past its auth
// timeout, and re-browsing too fast wastes disk I/O on the HA side, so pace
// reconnection attempts.
static const uint32_t RECONNECT_INTERVAL_MS = 10000;
static const int WS_NETWORK_TIMEOUT_MS = 10000;

void HaMediaSource::setup() {
  this->rx_lock_ = xSemaphoreCreateMutex();
  if (this->rx_lock_ == nullptr) {
    ESP_LOGE(TAG, "Failed to allocate receive lock");
    this->mark_failed();
    return;
  }
}

void HaMediaSource::set_connection(const std::string &url, const std::string &token) {
  const bool changed = url != this->url_ || token != this->token_;
  if (!changed) return;
  this->url_ = url;
  this->token_ = token;

  // Derive the http(s) origin used to join signed media paths. The websocket URL
  // ends in /api/websocket; strip that and swap the ws scheme back to http.
  this->base_url_.clear();
  if (!url.empty()) {
    std::string origin = url;
    const std::string suffix = "/api/websocket";
    if (origin.size() >= suffix.size() &&
        origin.compare(origin.size() - suffix.size(), suffix.size(), suffix) == 0) {
      origin.erase(origin.size() - suffix.size());
    }
    if (origin.rfind("wss://", 0) == 0) {
      origin = "https://" + origin.substr(6);
    } else if (origin.rfind("ws://", 0) == 0) {
      origin = "http://" + origin.substr(5);
    }
    this->base_url_ = origin;
  }

  // Reconnect with the new details on the main task.
  this->pending_disconnect_ = true;
  this->pending_connect_ = !url.empty() && !token.empty();
  this->last_connect_attempt_ = 0;
}

void HaMediaSource::set_folder(const std::string &media_content_id) {
  if (media_content_id == this->folder_) return;
  this->folder_ = media_content_id;
  this->index_.clear();
  if (this->ready()) {
    this->refresh_index();
  } else {
    this->refresh_when_ready_ = true;
  }
}

void HaMediaSource::refresh_index() {
  if (this->folder_.empty()) {
    this->index_.clear();
    this->index_ready_callback_.call();
    return;
  }
  if (!this->ready()) {
    this->refresh_when_ready_ = true;
    return;
  }
  this->send_browse_();
}

void HaMediaSource::resolve(const std::string &media_content_id,
                            std::function<void(const std::string &)> &&callback) {
  if (media_content_id.empty()) {
    callback(std::string());
    return;
  }
  // Thumbnail paths from the browse response are downloadable directly (with
  // the caller's Authorization header); no websocket round-trip needed.
  if (espcontrol::media_item_is_direct_path(media_content_id)) {
    callback(espcontrol::media_source_absolute_url(this->base_url_, media_content_id));
    return;
  }
  if (!this->ready()) {
    callback(std::string());
    return;
  }
  const int id = this->next_request_id_();
  this->pending_resolves_.push_back({id, std::move(callback)});
  const std::string json = json::build_json([&](JsonObject root) {
    root["id"] = id;
    root["type"] = "media_source/resolve_media";
    root["media_content_id"] = media_content_id;
  });
  if (!this->send_json_(json)) {
    // Drop the pending entry and report failure so the caller is never stranded.
    for (size_t i = 0; i < this->pending_resolves_.size(); i++) {
      if (this->pending_resolves_[i].request_id == id) {
        auto cb = std::move(this->pending_resolves_[i].callback);
        this->pending_resolves_.erase(this->pending_resolves_.begin() + i);
        cb(std::string());
        break;
      }
    }
  }
}

void HaMediaSource::loop() {
  // Apply connection changes requested from the entity callbacks / other tasks.
  if (this->pending_disconnect_) {
    this->pending_disconnect_ = false;
    this->disconnect_();
  }
  if (this->pending_connect_) {
    const uint32_t now = millis();
    if (this->last_connect_attempt_ == 0 ||
        now - this->last_connect_attempt_ >= RECONNECT_INTERVAL_MS) {
      this->last_connect_attempt_ = now;
      this->connect_();
    }
  }

  // Drain websocket-task messages on the main task, where JSON decode and
  // callbacks are safe.
  std::vector<PayloadBuffer> messages;
  bool oversize = false;
  if (this->rx_lock_ != nullptr &&
      xSemaphoreTake(this->rx_lock_, 0) == pdTRUE) {
    if (!this->rx_queue_.empty()) messages.swap(this->rx_queue_);
    oversize = this->rx_oversize_notice_;
    this->rx_oversize_notice_ = false;
    xSemaphoreGive(this->rx_lock_);
  }
  if (oversize) {
    ESP_LOGW(TAG, "Dropped a websocket message larger than %u bytes",
             (unsigned) kMaxMessageBytes);
    // The dropped message was almost certainly the pending browse response;
    // fail it so callers are not left waiting forever.
    this->fail_pending_browse_();
  }
  for (const PayloadBuffer &message : messages) {
    this->handle_message_(message.data(), message.size());
  }
}

void HaMediaSource::fail_pending_browse_() {
  if (this->browse_request_id_ == 0) return;
  this->browse_request_id_ = 0;
  this->index_.clear();
  this->index_ready_callback_.call();
}

void HaMediaSource::connect_() {
  if (this->client_ != nullptr) return;
  if (this->url_.empty() || this->token_.empty()) return;

  esp_websocket_client_config_t config = {};
  config.uri = this->url_.c_str();
  config.network_timeout_ms = WS_NETWORK_TIMEOUT_MS;
  config.reconnect_timeout_ms = RECONNECT_INTERVAL_MS;
  config.disable_auto_reconnect = false;
  // Home Assistant's default local media is served over plain ws on the LAN.
  config.buffer_size = 4096;

  this->client_ = esp_websocket_client_init(&config);
  if (this->client_ == nullptr) {
    ESP_LOGW(TAG, "Failed to init websocket client");
    return;
  }
  esp_websocket_register_events(this->client_, WEBSOCKET_EVENT_ANY,
                                &HaMediaSource::websocket_event_handler, this);
  this->state_ = ConnectionState::CONNECTING;
  if (esp_websocket_client_start(this->client_) != ESP_OK) {
    ESP_LOGW(TAG, "Failed to start websocket client");
    this->disconnect_();
  }
}

void HaMediaSource::disconnect_() {
  this->fail_pending_resolves_();
  this->browse_request_id_ = 0;
  this->rx_partial_.clear();
  if (this->client_ != nullptr) {
    esp_websocket_client_stop(this->client_);
    esp_websocket_client_destroy(this->client_);
    this->client_ = nullptr;
  }
  if (this->state_ != ConnectionState::FAILED_AUTH) {
    this->state_ = ConnectionState::DISCONNECTED;
  }
}

void HaMediaSource::fail_pending_resolves_() {
  std::vector<PendingResolve> pending;
  pending.swap(this->pending_resolves_);
  for (auto &entry : pending) {
    entry.callback(std::string());
  }
}

bool HaMediaSource::send_json_(const std::string &json) {
  if (this->client_ == nullptr) return false;
  if (!esp_websocket_client_is_connected(this->client_)) return false;
  // A bounded timeout so a stalled connection can never wedge the main loop;
  // a failed send is reported to the caller and retried by higher layers.
  const int sent = esp_websocket_client_send_text(this->client_, json.c_str(),
                                                   json.size(), pdMS_TO_TICKS(5000));
  return sent >= 0;
}

void HaMediaSource::send_auth_() {
  this->state_ = ConnectionState::AUTHENTICATING;
  const std::string json = json::build_json([&](JsonObject root) {
    root["type"] = "auth";
    root["access_token"] = this->token_;
  });
  this->send_json_(json);
}

void HaMediaSource::send_browse_() {
  this->browse_request_id_ = this->next_request_id_();
  const std::string json = json::build_json([&](JsonObject root) {
    root["id"] = this->browse_request_id_;
    root["type"] = "media_source/browse_media";
    root["media_content_id"] = this->folder_;
  });
  if (!this->send_json_(json)) {
    this->browse_request_id_ = 0;
  }
}

// ── Websocket task boundary ──────────────────────────────────────────────
// Runs on the esp_websocket_client task. Copy payloads under the lock; do no
// JSON work and touch no ESPHome/LVGL state here.
void HaMediaSource::websocket_event_handler(void *handler_args, esp_event_base_t base,
                                            int32_t event_id, void *event_data) {
  (void) base;
  auto *self = static_cast<HaMediaSource *>(handler_args);
  auto *data = static_cast<esp_websocket_event_data_t *>(event_data);
  switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
      // Home Assistant sends auth_required first; wait for it before replying.
      break;
    case WEBSOCKET_EVENT_DISCONNECTED:
    case WEBSOCKET_EVENT_CLOSED:
      if (self->state_ != ConnectionState::FAILED_AUTH) {
        self->state_ = ConnectionState::CONNECTING;
      }
      break;
    case WEBSOCKET_EVENT_DATA: {
      if (data->op_code != 0x01 || data->data_len <= 0) break;  // text frames only
      if (self->rx_lock_ == nullptr) break;
      // Home Assistant delivers a large browse response in chunks of one text
      // frame. Reassemble by payload offset/total length before queueing a
      // message; the total is known up front, so the buffer is reserved once
      // (PSRAM-first) and anything beyond the cap is skipped, not accumulated.
      if (xSemaphoreTake(self->rx_lock_, portMAX_DELAY) == pdTRUE) {
        if (data->payload_offset == 0) {
          self->rx_partial_.clear();
          self->rx_skip_current_ = data->payload_len > kMaxMessageBytes;
          if (self->rx_skip_current_) {
            self->rx_oversize_notice_ = true;
          } else {
            self->rx_partial_.reserve(data->payload_len);
          }
        }
        if (!self->rx_skip_current_) {
          self->rx_partial_.insert(self->rx_partial_.end(), data->data_ptr,
                                   data->data_ptr + data->data_len);
        }
        const bool complete =
            data->payload_offset + data->data_len >= data->payload_len;
        if (complete && !self->rx_skip_current_) {
          // Move-construct into the queue; RAMAllocator has no operator==, so a
          // move-assignment of the drained buffer would not compile. clear()
          // leaves the moved-from vector empty and reusable.
          self->rx_queue_.push_back(std::move(self->rx_partial_));
          self->rx_partial_.clear();
        }
        if (complete) self->rx_skip_current_ = false;
        xSemaphoreGive(self->rx_lock_);
      }
      break;
    }
    default:
      break;
  }
}

// ArduinoJson allocator backed by PSRAM-first RAMAllocator, so large documents
// never lean on the small internal heap regardless of the json component's
// USE_PSRAM configuration.
namespace {
struct PsramJsonAllocator : ArduinoJson::Allocator {
  void *allocate(size_t size) override { return this->ram_.allocate(size, 1); }
  void deallocate(void *ptr) override {
    this->ram_.deallocate(static_cast<char *>(ptr), 0);
  }
  void *reallocate(void *ptr, size_t new_size) override {
    return this->ram_.reallocate(static_cast<char *>(ptr), new_size, 1);
  }
  RAMAllocator<char> ram_;
};
PsramJsonAllocator g_psram_json_allocator;  // NOLINT(cert-err58-cpp)
}  // namespace

// ── Main-task message handling ───────────────────────────────────────────
void HaMediaSource::handle_message_(const char *data, size_t len) {
  const uint32_t parse_start = millis();

  // A browse response for a real album is hundreds of kilobytes, most of it
  // fields this client never reads (titles, thumbnails). Parse through a filter
  // that keeps only the fields the handlers below use — it cuts both the
  // document size and the time spent blocking the main loop by an order of
  // magnitude.
  JsonDocument filter;
  filter["type"] = true;
  filter["id"] = true;
  filter["success"] = true;
  JsonObject filter_result = filter["result"].to<JsonObject>();
  filter_result["url"] = true;
  JsonObject filter_child = filter_result["children"].add<JsonObject>();
  filter_child["media_class"] = true;
  filter_child["media_content_type"] = true;
  filter_child["media_content_id"] = true;
  filter_child["can_expand"] = true;
  filter_child["thumbnail"] = true;

  JsonDocument doc(&g_psram_json_allocator);
  const DeserializationError parse_error =
      deserializeJson(doc, data, len, DeserializationOption::Filter(filter));
  if (parse_error != DeserializationError::Ok) {
    ESP_LOGW(TAG, "Ignoring malformed websocket message (%u bytes): %s",
             (unsigned) len, parse_error.c_str());
    return;
  }
  if (len > 16 * 1024) {
    ESP_LOGI(TAG, "Parsed %u byte message in %u ms (heap: %u internal, %u largest)",
             (unsigned) len, (unsigned) (millis() - parse_start),
             (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
  }
  const JsonObjectConst root = doc.as<JsonObjectConst>();
  const std::string type = root["type"] | "";

  if (type == "auth_required") {
    this->send_auth_();
    return;
  }
  if (type == "auth_ok") {
    ESP_LOGI(TAG, "Authenticated with Home Assistant media source");
    this->state_ = ConnectionState::READY;
    if (this->refresh_when_ready_ || !this->folder_.empty()) {
      this->refresh_when_ready_ = false;
      this->refresh_index();
    }
    return;
  }
  if (type == "auth_invalid") {
    ESP_LOGW(TAG, "Home Assistant rejected the media source token");
    this->state_ = ConnectionState::FAILED_AUTH;
    this->pending_connect_ = false;
    this->disconnect_();
    return;
  }
  if (type == "result") {
    const int id = root["id"] | 0;
    const bool success = root["success"] | false;
    if (id != 0 && id == this->browse_request_id_) {
      this->browse_request_id_ = 0;
      if (success) {
        this->handle_browse_result_(root);
      } else {
        ESP_LOGW(TAG, "Browse failed for %s", this->folder_.c_str());
        this->index_.clear();
      }
      this->index_ready_callback_.call();
      return;
    }
    this->handle_resolve_result_(id, root);
    return;
  }
}

void HaMediaSource::handle_browse_result_(JsonObjectConst root) {
  this->index_.clear();
  JsonObjectConst result = root["result"];
  JsonArrayConst children = result["children"];
  if (children.isNull()) {
    ESP_LOGW(TAG, "Browse result for %s had no children", this->folder_.c_str());
    return;
  }
  size_t images = 0;
  for (JsonObjectConst child : children) {
    const std::string media_class = child["media_class"] | "";
    const std::string content_type = child["media_content_type"] | "";
    const char *content_id = child["media_content_id"] | "";
    const char *thumbnail = child["thumbnail"] | "";
    const bool can_expand = child["can_expand"] | false;
    if (espcontrol::media_child_is_folder(media_class, can_expand)) continue;
    if (!espcontrol::media_child_is_image(media_class, content_type)) continue;
    // Prefer Immich's JPEG preview: the browse thumbnail is WebP (undecodable
    // here) and the fullsize original behind resolve_media can run to well over
    // ten megabytes — far too slow over panel WiFi. Sources without a usable
    // preview keep the resolve_media flow via their media_content_id.
    bool added = false;
    if (thumbnail[0] != '\0' && strncmp(thumbnail, "/immich/", 8) == 0) {
      added = this->index_.add_photo(
          espcontrol::immich_thumbnail_to_preview(thumbnail).c_str());
    } else {
      added = this->index_.add_photo(content_id);
    }
    if (added) images++;
  }
  ESP_LOGI(TAG, "Indexed %u photo(s) from %s%s", (unsigned) images,
           this->folder_.c_str(),
           this->index_.truncated() ? " (list truncated to cap)" : "");
}

void HaMediaSource::handle_resolve_result_(int request_id, JsonObjectConst root) {
  for (size_t i = 0; i < this->pending_resolves_.size(); i++) {
    if (this->pending_resolves_[i].request_id != request_id) continue;
    auto callback = std::move(this->pending_resolves_[i].callback);
    this->pending_resolves_.erase(this->pending_resolves_.begin() + i);

    std::string absolute;
    const bool success = root["success"] | false;
    if (success) {
      JsonObjectConst result = root["result"];
      const std::string resolved = result["url"] | "";
      absolute = espcontrol::media_source_absolute_url(this->base_url_, resolved);
    }
    callback(absolute);
    return;
  }
}

void HaMediaSource::dump_config() {
  ESP_LOGCONFIG(TAG, "Home Assistant media source:");
  ESP_LOGCONFIG(TAG, "  URL: %s", this->url_.empty() ? "(unset)" : this->url_.c_str());
  ESP_LOGCONFIG(TAG, "  Token: %s", this->token_.empty() ? "(unset)" : "(set)");
  ESP_LOGCONFIG(TAG, "  Folder: %s", this->folder_.empty() ? "(unset)" : this->folder_.c_str());
}

}  // namespace ha_media_source
}  // namespace esphome

#endif  // USE_ESP_IDF
