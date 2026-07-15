#include "artwork_image.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "esphome/core/version.h"

#ifdef USE_API_HOMEASSISTANT_STATES
#include "esphome/components/api/api_server.h"
#endif

#if defined(USE_LVGL) && ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
#include "src/misc/cache/instance/lv_image_cache.h"
#endif

#ifdef USE_ESP32
#include "esp_heap_caps.h"
#endif

#ifdef USE_ESP_IDF
#include "esp_http_client.h"
#endif

static const char *const TAG = "artwork_image";
static const char *const CONTENT_TYPE_HEADER_NAME = "content-type";
static constexpr uint32_t RETIRED_BUFFER_GRACE_MS = 300;
static constexpr size_t MAX_RETIRED_IMAGE_BUFFERS = 1;
static constexpr size_t ABSOLUTE_MAX_DOWNLOAD_BUFFER_SIZE = 2 * 1024 * 1024;
static constexpr int LOCAL_ARTWORK_HTTP_TIMEOUT_MS = 6500;

#include "image_decoder.h"

#ifdef USE_ARTWORK_IMAGE_JPEG_SUPPORT
#include "jpeg_image.h"
#endif
#ifdef USE_ARTWORK_IMAGE_PNG_SUPPORT
#include "png_image.h"
#endif

namespace esphome {
namespace artwork_image {

using image::ImageType;

static std::string sanitize_artwork_url_for_log(const std::string &url) {
  auto query = url.find('?');
  if (query == std::string::npos) return url;
  return url.substr(0, query) + "?...";
}

static bool is_ha_media_proxy_url(const std::string &url) {
  return url.find("/api/media_player_proxy/") != std::string::npos;
}

static const char *classify_artwork_url_for_log(const std::string &url) {
  if (url.find("mzstatic.com") != std::string::npos) return "apple-cdn";
  if (is_ha_media_proxy_url(url)) return "ha-media-proxy";
  if (url.rfind("http://", 0) == 0) return "http";
  if (url.rfind("https://", 0) == 0) return "https";
  return "unknown";
}

static std::string response_header_for_log(http_request::HttpContainer *container, const std::string &header) {
  if (container == nullptr) return "(none)";
  std::string value = container->get_response_header(header);
  if (value.empty()) return "(none)";
  if (value.size() > 96) return value.substr(0, 96) + "...";
  return value;
}

#ifdef USE_ESP_IDF
class LocalHttpContainer : public http_request::HttpContainer {
 public:
  explicit LocalHttpContainer(esp_http_client_handle_t client) : client_(client) {}

  void add_response_header(const std::string &name, const std::string &value) {
    this->response_headers_.push_back({name, value});
  }
  void set_content_length_known(bool known) { this->content_length_known_ = known; }

  int read(uint8_t *buf, size_t max_len) override {
    int read = esp_http_client_read(this->client_, reinterpret_cast<char *>(buf), max_len);
    if (read > 0) {
      this->bytes_read_ += read;
    }
    return read;
  }

  bool is_read_complete() const override {
    if (this->content_length_known_ && HttpContainer::is_read_complete()) {
      return true;
    }
    if (this->client_ == nullptr) {
      return true;
    }
    if (this->bytes_read_ == 0 && !this->content_length_known_) {
      return false;
    }
    return esp_http_client_is_complete_data_received(this->client_);
  }

  void end() override {
    if (this->client_ != nullptr) {
      esp_http_client_close(this->client_);
      esp_http_client_cleanup(this->client_);
      this->client_ = nullptr;
    }
  }

 protected:
  esp_http_client_handle_t client_{nullptr};
  bool content_length_known_{false};
};

static esp_err_t insecure_local_http_event_handler(esp_http_client_event_t *evt) {
  auto *container = static_cast<LocalHttpContainer *>(evt->user_data);
  if (container == nullptr || evt->event_id != HTTP_EVENT_ON_HEADER) {
    return ESP_OK;
  }
  const std::string header_name = str_lower_case(evt->header_key);
  if (header_name == CONTENT_TYPE_HEADER_NAME) {
    container->add_response_header(header_name, evt->header_value);
  }
  return ESP_OK;
}
#endif

inline bool is_color_on(const Color &color) {
  // This produces the most accurate monochrome conversion, but is slightly slower.
  //  return (0.2125 * color.r + 0.7154 * color.g + 0.0721 * color.b) > 127;

  // Approximation using fast integer computations; produces acceptable results
  // Equivalent to 0.25 * R + 0.5 * G + 0.25 * B
  return ((color.r >> 2) + (color.g >> 1) + (color.b >> 2)) & 0x80;
}

ArtworkImage::ArtworkImage(const std::string &url, int width, int height, ImageFormat format,
                         ImageResizeMode resize_mode, ImageType type, image::Transparency transparency,
                         uint32_t download_buffer_size, bool is_big_endian, bool allow_insecure_local_urls)
    : Image(nullptr, 0, 0, type, transparency),
      buffer_(nullptr),
      download_buffer_(download_buffer_size),
      download_buffer_initial_size_(download_buffer_size),
      // Compressed JPEG/PNG data can be larger than the resized RGB565 output.
      // Keep the transfer limit independent from the decoded image dimensions.
      max_download_buffer_size_(ABSOLUTE_MAX_DOWNLOAD_BUFFER_SIZE),
      format_(format),
      resize_mode_(resize_mode),
      fixed_width_(width),
      fixed_height_(height),
      is_big_endian_(is_big_endian),
      allow_insecure_local_urls_(allow_insecure_local_urls),
      buffer_width_(0),
      buffer_height_(0),
      start_time_(0) {
  this->set_url(url);
}

void ArtworkImage::draw(int x, int y, display::Display *display, Color color_on, Color color_off) {
  if (this->data_start_) {
    Image::draw(x, y, display, color_on, color_off);
  } else if (this->placeholder_) {
    this->placeholder_->draw(x, y, display, color_on, color_off);
  }
}

void ArtworkImage::release() {
  this->update_pending_ = false;
  this->pending_url_.clear();
  this->end_connection_();
  this->retire_active_buffer_();
  this->cleanup_retired_buffers_(true);
}

size_t ArtworkImage::resize_(int width_in, int height_in) {
  int width = this->fixed_width_;
  int height = this->fixed_height_;
  int content_width = width;
  int content_height = height;
  int offset_x = 0;
  int offset_y = 0;
  if (this->is_auto_resize_()) {
    width = width_in;
    height = height_in;
    content_width = width;
    content_height = height;
  } else if (width_in > 0 && height_in > 0) {
    if (width_in != height_in) {
      double width_scale = static_cast<double>(this->fixed_width_) / width_in;
      double height_scale = static_cast<double>(this->fixed_height_) / height_in;
      double scale = this->resize_mode_ == ImageResizeMode::COVER
        ? std::max(width_scale, height_scale)
        : std::min(width_scale, height_scale);
      content_width = std::max(1, (static_cast<int>(width_in * scale) + 3) & ~3);
      content_height = std::max(1, (static_cast<int>(height_in * scale) + 3) & ~3);
      if (this->resize_mode_ != ImageResizeMode::COVER) {
        if (content_width > this->fixed_width_) content_width = this->fixed_width_;
        if (content_height > this->fixed_height_) content_height = this->fixed_height_;
      }
      offset_x = (this->fixed_width_ - content_width) / 2;
      offset_y = (this->fixed_height_ - content_height) / 2;
    }
  }
  size_t new_size = this->get_buffer_size_(width, height);
  if (!this->retired_buffers_.empty()) {
    ESP_LOGI(TAG, "Freeing retired artwork buffers before next decode: need=%zu retired_bytes=%zu",
             new_size, this->retired_buffer_bytes_());
    this->cleanup_retired_buffers_(true);
  }
  if (this->decode_buffer_) {
    if (new_size <= this->get_decode_buffer_size_()) {
      this->decode_buffer_width_ = width;
      this->decode_buffer_height_ = height;
      this->decode_content_width_ = content_width;
      this->decode_content_height_ = content_height;
      this->decode_offset_x_ = offset_x;
      this->decode_offset_y_ = offset_y;
      memset(this->decode_buffer_, 0, new_size);
      ESP_LOGI(TAG, "Artwork fit: source=%dx%d target=%dx%d content=%dx%d offset=%d,%d",
               width_in, height_in, width, height, content_width, content_height, offset_x, offset_y);
      return new_size;
    }
    this->allocator_.deallocate(this->decode_buffer_, this->get_decode_buffer_size_());
    this->decode_buffer_ = nullptr;
    this->decode_buffer_width_ = 0;
    this->decode_buffer_height_ = 0;
    this->decode_content_width_ = 0;
    this->decode_content_height_ = 0;
    this->decode_offset_x_ = 0;
    this->decode_offset_y_ = 0;
  }
  ESP_LOGD(TAG, "Allocating decode buffer of %zu bytes", new_size);
  this->decode_buffer_ = this->allocator_.allocate(new_size);
  if (this->decode_buffer_ == nullptr) {
    ESP_LOGE(TAG, "allocation of %zu bytes failed. Biggest block in heap: %zu Bytes", new_size,
             this->allocator_.get_max_free_block_size());
    this->end_connection_();
    return 0;
  }
  this->decode_buffer_width_ = width;
  this->decode_buffer_height_ = height;
  this->decode_content_width_ = content_width;
  this->decode_content_height_ = content_height;
  this->decode_offset_x_ = offset_x;
  this->decode_offset_y_ = offset_y;
  memset(this->decode_buffer_, 0, new_size);
  ESP_LOGI(TAG, "Artwork fit: source=%dx%d target=%dx%d content=%dx%d offset=%d,%d",
           width_in, height_in, width, height, content_width, content_height, offset_x, offset_y);
  return new_size;
}

std::string ArtworkImage::request_update_url(const std::string &url, int max_source_dim) {
  int max_dim = max_source_dim > 0 ? max_source_dim : (this->fixed_width_ > 0 ? this->fixed_width_ : 600);
  std::string effective_url = cap_artwork_url(url, max_dim);
  if (effective_url != url) {
    ESP_LOGI(TAG, "Rewrote artwork URL to a capped JPEG (%dpx)", max_dim);
  }
  if (!this->validate_url_(effective_url)) {
    return "";
  }
  if (this->is_busy_()) {
    if (effective_url == this->url_) {
      ESP_LOGI(TAG, "Artwork update already in progress for URL; ignoring duplicate request");
      return effective_url;
    }
    this->queue_pending_update_(effective_url);
    return effective_url;
  }
  this->url_ = effective_url;
  this->update();
  return effective_url;
}

void ArtworkImage::cancel_update() {
  this->update_pending_ = false;
  this->pending_url_.clear();
  if (this->is_busy_()) {
    ESP_LOGW(TAG, "Cancelling in-flight artwork update");
    this->end_connection_();
  }
}

void ArtworkImage::update() {
  if (this->is_busy_()) {
    this->queue_pending_update_(this->url_);
    return;
  }
  this->last_http_status_ = 0;
  this->last_error_was_ha_media_proxy_ = false;
  this->peak_download_buffer_size_ = this->download_buffer_.size();
  ESP_LOGI(TAG, "Updating image %s", sanitize_artwork_url_for_log(this->url_).c_str());
  ESP_LOGD(TAG, "Artwork URL source: %s", classify_artwork_url_for_log(this->url_));
  this->log_state_("request-start");

  std::vector<http_request::Header> headers = {};

  http_request::Header accept_header;
  accept_header.name = "Accept";
  std::string accept_mime_type;
  switch (this->format_) {
    case ImageFormat::AUTO:
      accept_mime_type = "image/jpeg, image/png";
      break;
#ifdef USE_ARTWORK_IMAGE_JPEG_SUPPORT
    case ImageFormat::JPEG:
      accept_mime_type = "image/jpeg";
      break;
#endif  // USE_ARTWORK_IMAGE_JPEG_SUPPORT
#ifdef USE_ARTWORK_IMAGE_PNG_SUPPORT
    case ImageFormat::PNG:
      accept_mime_type = "image/png";
      break;
#endif  // USE_ARTWORK_IMAGE_PNG_SUPPORT
    default:
      accept_mime_type = "image/*";
  }
  accept_header.value = accept_mime_type + ",*/*;q=0.8";

  headers.push_back(accept_header);

  for (auto &header : this->request_headers_) {
    headers.push_back(http_request::Header{header.first, header.second.value()});
  }

  if (this->should_use_local_idf_url_(this->url_)) {
    this->downloader_ = this->get_local_idf_(this->url_, headers);
  } else {
    this->downloader_ = this->parent_->get(this->url_, headers, {CONTENT_TYPE_HEADER_NAME});
  }

  if (this->downloader_ == nullptr) {
    this->last_error_was_ha_media_proxy_ = is_ha_media_proxy_url(this->url_);
    ESP_LOGE(TAG, "Download failed before response; source=%s url=%s",
             classify_artwork_url_for_log(this->url_), sanitize_artwork_url_for_log(this->url_).c_str());
    this->fail_download_();
    return;
  }

  int http_code = this->downloader_->status_code;
  this->log_state_("response-ready");
  if (http_code == HTTP_CODE_NOT_MODIFIED) {
    // Image hasn't changed on server. Skip download.
    ESP_LOGI(TAG, "Server returned HTTP 304 (Not Modified). Download skipped.");
    this->end_connection_();
    if (this->has_newer_pending_update_()) {
      this->start_pending_update_();
      return;
    }
    this->download_finished_callback_.call(true);
    this->start_pending_update_();
    return;
  }
  if (http_code != HTTP_CODE_OK) {
    this->last_http_status_ = http_code;
    this->last_error_was_ha_media_proxy_ = is_ha_media_proxy_url(this->url_);
    ESP_LOGE(TAG, "Artwork HTTP result: status=%d content_length=%zu content_type=%s source=%s url=%s",
             http_code, this->downloader_->content_length,
             response_header_for_log(this->downloader_.get(), CONTENT_TYPE_HEADER_NAME).c_str(),
             classify_artwork_url_for_log(this->url_), sanitize_artwork_url_for_log(this->url_).c_str());
    this->fail_download_();
    return;
  }

  ESP_LOGD(TAG, "Starting download");
  size_t total_size = this->get_sane_content_length_();

  if (this->format_ == ImageFormat::AUTO) {
    ESP_LOGD(TAG, "Deferring auto image format detection until magic bytes are available");
    this->log_state_("format-detect-wait");
    this->start_time_ = ::time(nullptr);
    this->last_data_millis_ = millis();
    this->enable_loop();
    return;
  }

  ImageFormat resolved = this->detect_format_();
  if (!this->create_decoder_(resolved, total_size)) {
    this->fail_download_();
    return;
  }
  this->log_state_("decoder-ready");
  ESP_LOGI(TAG, "Downloading image (Size: %zu)", total_size);
  this->start_time_ = ::time(nullptr);
  this->last_data_millis_ = millis();
  this->enable_loop();
}

bool ArtworkImage::should_use_local_idf_url_(const std::string &url) const {
  bool is_http = url.rfind("http://", 0) == 0;
  bool is_https = url.rfind("https://", 0) == 0;
  if (!is_http && !(is_https && this->allow_insecure_local_urls_)) {
    return false;
  }

  size_t host_start = is_https ? 8 : 7;
  size_t host_end = url.find_first_of("/?#", host_start);
  std::string authority = url.substr(host_start, host_end == std::string::npos ? std::string::npos : host_end - host_start);
  size_t at = authority.rfind('@');
  if (at != std::string::npos) {
    authority = authority.substr(at + 1);
  }

  std::string host;
  if (!authority.empty() && authority.front() == '[') {
    size_t end = authority.find(']');
    host = end == std::string::npos ? authority : authority.substr(1, end - 1);
  } else {
    size_t colon = authority.find(':');
    host = colon == std::string::npos ? authority : authority.substr(0, colon);
  }

  std::transform(host.begin(), host.end(), host.begin(), [](unsigned char c) { return std::tolower(c); });
  return this->is_private_or_local_host_(host);
}

bool ArtworkImage::is_private_or_local_host_(const std::string &host) const {
  if (host == "localhost" || host == "homeassistant.local" ||
      (host.size() > 6 && host.compare(host.size() - 6, 6, ".local") == 0)) {
    return true;
  }
  if (host.rfind("fe80:", 0) == 0 || host == "::1") {
    return true;
  }

  int parts[4] = {-1, -1, -1, -1};
  const char *cursor = host.c_str();
  char *end = nullptr;
  for (int i = 0; i < 4; i++) {
    long value = std::strtol(cursor, &end, 10);
    if (end == cursor || value < 0 || value > 255) {
      return false;
    }
    parts[i] = static_cast<int>(value);
    if (i < 3) {
      if (*end != '.') return false;
      cursor = end + 1;
    } else if (*end != '\0') {
      return false;
    }
  }

  return parts[0] == 10 || parts[0] == 127 || (parts[0] == 192 && parts[1] == 168) ||
         (parts[0] == 172 && parts[1] >= 16 && parts[1] <= 31) || (parts[0] == 169 && parts[1] == 254);
}

std::shared_ptr<http_request::HttpContainer> ArtworkImage::get_local_idf_(
    const std::string &url, const std::vector<http_request::Header> &headers) {
#ifdef USE_ESP_IDF
  bool secure = url.rfind("https://", 0) == 0;
  std::string safe_url = sanitize_artwork_url_for_log(url);
  if (secure) {
    ESP_LOGW(TAG, "Using insecure TLS for local artwork URL: %s", safe_url.c_str());
  } else {
    ESP_LOGD(TAG, "Using guarded local artwork request: %s", safe_url.c_str());
  }
  esp_http_client_config_t config = {};
  config.url = url.c_str();
  config.method = HTTP_METHOD_GET;
  config.timeout_ms = std::min<int>(this->parent_->get_timeout(), LOCAL_ARTWORK_HTTP_TIMEOUT_MS);
  config.disable_auto_redirect = false;
  config.max_redirection_count = 3;
  config.auth_type = HTTP_AUTH_TYPE_NONE;
  config.event_handler = insecure_local_http_event_handler;

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == nullptr) {
    ESP_LOGE(TAG, "Local artwork request failed; client could not be initialized");
    return nullptr;
  }

  auto container = std::make_shared<LocalHttpContainer>(client);
  container->set_parent(this->parent_);
  container->set_secure(secure);
  esp_http_client_set_user_data(client, static_cast<void *>(container.get()));

  for (const auto &header : headers) {
    esp_http_client_set_header(client, header.name.c_str(), header.value.c_str());
  }

  const uint32_t start = millis();
  App.feed_wdt();
  esp_err_t err = esp_http_client_open(client, 0);
  App.feed_wdt();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Local artwork request failed: %s source=%s url=%s", esp_err_to_name(err),
             classify_artwork_url_for_log(url), safe_url.c_str());
    container->end();
    return nullptr;
  }

  int64_t content_length = esp_http_client_fetch_headers(client);
  App.feed_wdt();
  container->content_length = content_length > 0 ? static_cast<size_t>(content_length) : 0;
  container->set_content_length_known(content_length > 0);
  container->set_chunked(esp_http_client_is_chunked_response(client));
  container->status_code = esp_http_client_get_status_code(client);
  if (container->status_code <= 0 && is_ha_media_proxy_url(url)) {
    ESP_LOGW(TAG, "Home Assistant media proxy returned an unknown HTTP status; trying artwork bytes anyway");
    container->status_code = HTTP_CODE_OK;
  }
  container->duration_ms = millis() - start;
  return container;
#else
  return this->parent_->get(url, headers, {CONTENT_TYPE_HEADER_NAME});
#endif
}

size_t ArtworkImage::get_sane_content_length_() const {
  if (!this->downloader_) {
    return 0;
  }
  size_t content_length = this->downloader_->content_length;
  if (content_length > this->max_download_buffer_size_) {
    ESP_LOGW(TAG, "Ignoring artwork content length beyond transfer limit: %zu > %zu",
             content_length, this->max_download_buffer_size_);
    return 0;
  }
  return content_length;
}

void ArtworkImage::loop() {
  this->cleanup_retired_buffers_(false);
  if (!this->decoder_ && !this->downloader_) {
    if (this->retired_buffers_.empty()) {
      this->disable_loop();
    }
    return;
  }

  // Deferred decoder creation for AUTO format: read data for magic-byte detection
  if (!this->decoder_ && this->downloader_) {
    if (!this->ensure_download_buffer_capacity_()) {
      this->fail_download_();
      return;
    }

    size_t available = std::min(this->download_buffer_.free_capacity(), this->download_buffer_initial_size_);
    auto len = this->downloader_->read(this->download_buffer_.append(), available);
    bool transfer_complete = false;
    if (len > 0) {
      this->download_buffer_.write(len);
      this->last_data_millis_ = millis();
    } else if (len < 0) {
      ESP_LOGE(TAG, "Download failed while detecting image format: %d", len);
      this->fail_download_();
      return;
    } else if (this->downloader_->is_read_complete()) {
      transfer_complete = true;
      if (this->download_buffer_.unread() < 12) {
        ESP_LOGE(TAG, "Download finished before enough data was received to detect image format");
        this->fail_download_();
        return;
      }
    }

    if (this->download_buffer_.unread() < 12) {
      if (millis() - this->last_data_millis_ > DOWNLOAD_STALL_TIMEOUT_MS) {
        ESP_LOGE(TAG, "Download stalled waiting for format detection bytes");
        this->fail_download_();
      }
      return;
    }

    ImageFormat resolved = this->detect_format_();
    if (resolved == ImageFormat::AUTO) {
      ESP_LOGE(TAG, "Could not determine image format from headers or file content; content_type=%s bytes=%zu source=%s",
               response_header_for_log(this->downloader_.get(), CONTENT_TYPE_HEADER_NAME).c_str(),
               this->download_buffer_.unread(), classify_artwork_url_for_log(this->url_));
      this->fail_download_();
      return;
    }

    size_t total_size = this->get_sane_content_length_();
    if (total_size == 0 && transfer_complete) {
      total_size = this->downloader_->get_bytes_read();
    }
    if (!this->create_decoder_(resolved, total_size)) {
      this->fail_download_();
      return;
    }
    this->log_state_("decoder-ready");
    ESP_LOGI(TAG, "Downloading image (Size: %zu)", total_size);

    // Feed already-buffered data to the newly created decoder
    if (!this->decode_buffered_data_()) {
      this->fail_download_();
      return;
    }
    if (this->decoder_->is_finished()) {
      this->finish_download_();
    }
    return;
  }

  if (this->decoder_->is_finished()) {
    this->finish_download_();
    return;
  }
  if (this->decoder_->is_decoding()) {
    if (!this->decode_buffered_data_()) {
      this->fail_download_();
      return;
    }
    if (this->decoder_->is_finished()) {
      this->finish_download_();
    }
    return;
  }
  if (this->downloader_ == nullptr) {
    ESP_LOGE(TAG, "Downloader not instantiated; cannot download");
    return;
  }

  if (!this->ensure_download_buffer_capacity_()) {
    this->fail_download_();
    return;
  }

  size_t available = std::min(this->download_buffer_.free_capacity(), this->download_buffer_initial_size_);
  auto len = this->downloader_->read(this->download_buffer_.append(), available);
  if (len > 0) {
    this->download_buffer_.write(len);
    this->last_data_millis_ = millis();
    if (!this->decode_buffered_data_()) {
      this->fail_download_();
      return;
    }
    if (this->decoder_->is_finished()) {
      this->finish_download_();
    }
    return;
  }

  if (len < 0) {
    ESP_LOGE(TAG, "Download failed while reading image data: %d", len);
    this->fail_download_();
    return;
  }

  if (this->downloader_->is_read_complete()) {
    if (this->decoder_->has_unknown_download_size()) {
      this->decoder_->set_download_size(this->downloader_->get_bytes_read());
      ESP_LOGD(TAG, "HTTP transfer complete; inferred image size: %zu bytes", this->downloader_->get_bytes_read());
    }
    if (!this->decode_buffered_data_()) {
      this->fail_download_();
      return;
    }
    if (this->decoder_->is_finished()) {
      this->finish_download_();
      return;
    }
    if (this->decoder_->is_decoding()) {
      return;
    }
    ESP_LOGE(TAG, "HTTP transfer finished before image decoder completed");
    this->fail_download_();
    return;
  }

  if (millis() - this->last_data_millis_ > DOWNLOAD_STALL_TIMEOUT_MS) {
    ESP_LOGE(TAG, "Download stalled: no data received for %" PRIu32 "ms (buffered %zu bytes)",
             DOWNLOAD_STALL_TIMEOUT_MS, this->download_buffer_.unread());
    this->fail_download_();
    return;
  }
}

void ArtworkImage::map_chroma_key(Color &color) {
  if (this->transparency_ == image::TRANSPARENCY_CHROMA_KEY) {
    if (color.g == 1 && color.r == 0 && color.b == 0) {
      color.g = 0;
    }
    if (color.w < 0x80) {
      color.r = 0;
      color.g = this->type_ == ImageType::IMAGE_TYPE_RGB565 ? 4 : 1;
      color.b = 0;
    }
  }
}

void ArtworkImage::draw_pixel_(int x, int y, Color color) {
  if (!this->decode_buffer_) {
    ESP_LOGE(TAG, "Decode buffer not allocated!");
    return;
  }
  if (x < 0 || y < 0 || x >= this->decode_buffer_width_ || y >= this->decode_buffer_height_) {
    ESP_LOGE(TAG, "Tried to paint a pixel (%d,%d) outside the image!", x, y);
    return;
  }
  uint32_t pos = this->get_position_(x, y);
  switch (this->type_) {
    case ImageType::IMAGE_TYPE_BINARY: {
      const uint32_t width_8 = ((this->decode_buffer_width_ + 7u) / 8u) * 8u;
      pos = x + y * width_8;
      auto bitno = 0x80 >> (pos % 8u);
      pos /= 8u;
      auto on = is_color_on(color);
      if (this->has_transparency() && color.w < 0x80)
        on = false;
      if (on) {
        this->decode_buffer_[pos] |= bitno;
      } else {
        this->decode_buffer_[pos] &= ~bitno;
      }
      break;
    }
    case ImageType::IMAGE_TYPE_GRAYSCALE: {
      auto gray = static_cast<uint8_t>(0.2125 * color.r + 0.7154 * color.g + 0.0721 * color.b);
      if (this->transparency_ == image::TRANSPARENCY_CHROMA_KEY) {
        if (gray == 1) {
          gray = 0;
        }
        if (color.w < 0x80) {
          gray = 1;
        }
      } else if (this->transparency_ == image::TRANSPARENCY_ALPHA_CHANNEL) {
        if (color.w != 0xFF)
          gray = color.w;
      }
      this->decode_buffer_[pos] = gray;
      break;
    }
    case ImageType::IMAGE_TYPE_RGB565: {
      this->map_chroma_key(color);
      uint16_t col565 = display::ColorUtil::color_to_565(color);
      if (this->is_big_endian_) {
        this->decode_buffer_[pos + 0] = static_cast<uint8_t>((col565 >> 8) & 0xFF);
        this->decode_buffer_[pos + 1] = static_cast<uint8_t>(col565 & 0xFF);
      } else {
        this->decode_buffer_[pos + 0] = static_cast<uint8_t>(col565 & 0xFF);
        this->decode_buffer_[pos + 1] = static_cast<uint8_t>((col565 >> 8) & 0xFF);
      }
      if (this->transparency_ == image::TRANSPARENCY_ALPHA_CHANNEL) {
        this->decode_buffer_[pos + 2] = color.w;
      }
      break;
    }
    case ImageType::IMAGE_TYPE_RGB: {
      this->map_chroma_key(color);
      this->decode_buffer_[pos + 0] = color.r;
      this->decode_buffer_[pos + 1] = color.g;
      this->decode_buffer_[pos + 2] = color.b;
      if (this->transparency_ == image::TRANSPARENCY_ALPHA_CHANNEL) {
        this->decode_buffer_[pos + 3] = color.w;
      }
      break;
    }
  }
}

ImageFormat ArtworkImage::detect_format_() {
  if (this->format_ != ImageFormat::AUTO) {
    return this->format_;
  }

  // Prefer magic bytes because Home Assistant proxy headers can be stale for
  // identical media_player_proxy paths whose cache parameter points at new art.
  if (this->download_buffer_.unread() >= 4) {
    const uint8_t *data = this->download_buffer_.data();
    if (data[0] == 0xFF && data[1] == 0xD8) {
      if (this->detect_progressive_jpeg_()) {
        ESP_LOGW(TAG, "Detected progressive JPEG from magic bytes; attempting native JPEG decoder");
      } else {
        ESP_LOGD(TAG, "Detected JPEG from magic bytes; decoder will report baseline/progressive from the header");
      }
      return ImageFormat::JPEG;
    }
    if (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47) {
      ESP_LOGD(TAG, "Detected PNG from magic bytes");
      return ImageFormat::PNG;
    }
    if (this->detect_heic_()) {
      ESP_LOGW(TAG, "Detected HEIC/HEIF from file signature");
      return ImageFormat::HEIC;
    }
  }

  // Fallback: Content-Type header
  if (this->downloader_) {
    std::string ct = str_lower_case(this->downloader_->get_response_header(CONTENT_TYPE_HEADER_NAME));
    if (ct.find("image/jpeg") != std::string::npos || ct.find("image/jpg") != std::string::npos) {
      ESP_LOGD(TAG, "Detected JPEG from Content-Type: %s", ct.c_str());
      return ImageFormat::JPEG;
    }
    if (ct.find("image/png") != std::string::npos) {
      ESP_LOGD(TAG, "Detected PNG from Content-Type: %s", ct.c_str());
      return ImageFormat::PNG;
    }
    if (ct.find("image/heic") != std::string::npos || ct.find("image/heif") != std::string::npos) {
      ESP_LOGW(TAG, "Detected HEIC/HEIF from Content-Type: %s", ct.c_str());
      return ImageFormat::HEIC;
    }
  }

  return ImageFormat::AUTO;
}

bool ArtworkImage::detect_progressive_jpeg_() {
  size_t len = this->download_buffer_.unread();
  const uint8_t *data = this->download_buffer_.data();
  if (len < 4 || data[0] != 0xFF || data[1] != 0xD8) {
    return false;
  }

  size_t pos = 2;
  while (pos + 3 < len) {
    while (pos < len && data[pos] != 0xFF) pos++;
    while (pos < len && data[pos] == 0xFF) pos++;
    if (pos >= len) break;

    uint8_t marker = data[pos++];
    if (marker == 0xDA || marker == 0xD9) {
      break;
    }
    if (marker >= 0xD0 && marker <= 0xD7) {
      continue;
    }
    if (pos + 1 >= len) break;
    uint16_t segment_len = (static_cast<uint16_t>(data[pos]) << 8) | data[pos + 1];
    if (segment_len < 2) break;

    if (marker == 0xC2) {
      return true;
    }
    if (marker == 0xC0) {
      return false;
    }
    pos += segment_len;
  }
  return false;
}

bool ArtworkImage::detect_heic_() {
  size_t len = this->download_buffer_.unread();
  const uint8_t *data = this->download_buffer_.data();
  if (len < 12) {
    return false;
  }
  if (data[4] != 'f' || data[5] != 't' || data[6] != 'y' || data[7] != 'p') {
    return false;
  }

  for (size_t pos = 8; pos + 3 < len && pos < 64; pos += 4) {
    if ((data[pos] == 'h' && data[pos + 1] == 'e' && data[pos + 2] == 'i' &&
         (data[pos + 3] == 'c' || data[pos + 3] == 'x')) ||
        (data[pos] == 'h' && data[pos + 1] == 'e' && data[pos + 2] == 'v' &&
         (data[pos + 3] == 'c' || data[pos + 3] == 'x')) ||
        (data[pos] == 'm' && data[pos + 1] == 'i' && data[pos + 2] == 'f' && data[pos + 3] == '1') ||
        (data[pos] == 'm' && data[pos + 1] == 's' && data[pos + 2] == 'f' && data[pos + 3] == '1')) {
      return true;
    }
  }
  return false;
}

bool ArtworkImage::create_decoder_(ImageFormat format, size_t total_size) {
  if (format == ImageFormat::HEIC) {
    ESP_LOGE(TAG, "HEIC/HEIF artwork detected, but no native HEIC decoder is bundled for this firmware; source=%s",
             classify_artwork_url_for_log(this->url_));
    return false;
  }
#ifdef USE_ARTWORK_IMAGE_JPEG_SUPPORT
  if (format == ImageFormat::JPEG) {
    ESP_LOGD(TAG, "Allocating JPEG decoder");
    this->decoder_ = esphome::make_unique<JpegDecoder>(this);
  }
#endif
#ifdef USE_ARTWORK_IMAGE_PNG_SUPPORT
  if (format == ImageFormat::PNG) {
    ESP_LOGD(TAG, "Allocating PNG decoder");
    this->decoder_ = make_unique<PngDecoder>(this);
  }
#endif
  if (!this->decoder_) {
    ESP_LOGE(TAG, "Could not instantiate decoder. Image format unsupported: %d", format);
    return false;
  }
  if (this->decoder_->prepare(total_size) < 0) {
    this->decoder_.reset();
    return false;
  }
  return true;
}

void ArtworkImage::discard_decode_buffer_() {
  if (this->decode_buffer_) {
    this->allocator_.deallocate(this->decode_buffer_, this->get_decode_buffer_size_());
    this->decode_buffer_ = nullptr;
  }
  this->decode_buffer_width_ = 0;
  this->decode_buffer_height_ = 0;
  this->decode_content_width_ = 0;
  this->decode_content_height_ = 0;
  this->decode_offset_x_ = 0;
  this->decode_offset_y_ = 0;
}

void ArtworkImage::invalidate_lvgl_cache_() {
#ifdef USE_LVGL
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  lv_image_cache_drop(&this->dsc_);
#else
  lv_img_cache_invalidate_src(&this->dsc_);
#endif
#endif
}

bool ArtworkImage::promote_decode_buffer_() {
  if (!this->decode_buffer_) {
    ESP_LOGE(TAG, "Decode finished without a decoded image buffer");
    return false;
  }
  if (this->decode_buffer_width_ <= 0 || this->decode_buffer_height_ <= 0) {
    ESP_LOGE(TAG, "Decode finished with invalid dimensions: %dx%d", this->decode_buffer_width_,
             this->decode_buffer_height_);
    return false;
  }

  this->retire_active_buffer_();
  this->buffer_ = this->decode_buffer_;
  this->buffer_width_ = this->decode_buffer_width_;
  this->buffer_height_ = this->decode_buffer_height_;
  this->buffer_content_width_ = this->decode_content_width_;
  this->buffer_content_height_ = this->decode_content_height_;
  this->buffer_offset_x_ = this->decode_offset_x_;
  this->buffer_offset_y_ = this->decode_offset_y_;
  ESP_LOGI(TAG, "Artwork buffer ready: image=%dx%d content=%dx%d offset=%d,%d",
           this->buffer_width_, this->buffer_height_, this->buffer_content_width_, this->buffer_content_height_,
           this->buffer_offset_x_, this->buffer_offset_y_);
  this->decode_buffer_ = nullptr;
  this->decode_buffer_width_ = 0;
  this->decode_buffer_height_ = 0;
  this->decode_content_width_ = 0;
  this->decode_content_height_ = 0;
  this->decode_offset_x_ = 0;
  this->decode_offset_y_ = 0;

  this->data_start_ = this->buffer_;
  this->width_ = this->buffer_width_;
  this->height_ = this->buffer_height_;
  this->invalidate_lvgl_cache_();
#ifdef USE_LVGL
  memset(&this->dsc_, 0, sizeof(this->dsc_));
#endif
  return true;
}

void ArtworkImage::retire_active_buffer_() {
  if (!this->buffer_) {
    return;
  }
  this->retired_buffers_.push_back(RetiredBuffer{this->buffer_, this->get_buffer_size_(), millis()});
  this->buffer_ = nullptr;
  this->data_start_ = nullptr;
  this->buffer_width_ = 0;
  this->buffer_height_ = 0;
  this->buffer_content_width_ = 0;
  this->buffer_content_height_ = 0;
  this->buffer_offset_x_ = 0;
  this->buffer_offset_y_ = 0;
  this->width_ = 0;
  this->height_ = 0;
  this->invalidate_lvgl_cache_();
#ifdef USE_LVGL
  memset(&this->dsc_, 0, sizeof(this->dsc_));
#endif
  this->limit_retired_buffers_();
  this->cleanup_retired_buffers_(false);
  if (!this->retired_buffers_.empty()) {
    this->enable_loop();
  }
}

void ArtworkImage::cleanup_retired_buffers_(bool force) {
  uint32_t now = millis();
  auto it = this->retired_buffers_.begin();
  size_t freed_bytes = 0;
  while (it != this->retired_buffers_.end()) {
    if (force || now - it->retired_at >= RETIRED_BUFFER_GRACE_MS) {
      freed_bytes += it->size;
      this->allocator_.deallocate(it->data, it->size);
      it = this->retired_buffers_.erase(it);
    } else {
      ++it;
    }
  }
  if (freed_bytes > 0) {
    size_t remaining_bytes = 0;
    for (const auto &buffer : this->retired_buffers_) {
      remaining_bytes += buffer.size;
    }
    ESP_LOGI(TAG, "Freed retired artwork buffers: freed=%zu remaining=%zu remaining_bytes=%zu",
             freed_bytes, this->retired_buffers_.size(), remaining_bytes);
  }
}

size_t ArtworkImage::retired_buffer_bytes_() const {
  size_t retired_bytes = 0;
  for (const auto &buffer : this->retired_buffers_) {
    retired_bytes += buffer.size;
  }
  return retired_bytes;
}

void ArtworkImage::limit_retired_buffers_() {
  while (this->retired_buffers_.size() > MAX_RETIRED_IMAGE_BUFFERS) {
    auto it = this->retired_buffers_.begin();
    ESP_LOGW(TAG, "Forcing retired artwork buffer cleanup: size=%zu remaining=%zu",
             it->size, this->retired_buffers_.size() - 1);
    this->allocator_.deallocate(it->data, it->size);
    this->retired_buffers_.erase(it);
  }
}

bool ArtworkImage::ensure_download_buffer_capacity_() {
  if (this->download_buffer_.free_capacity() > 0) {
    return true;
  }

  size_t current_size = this->download_buffer_.size();
  size_t target_size = current_size == 0 ? this->download_buffer_initial_size_ : current_size * 2;
  if (target_size > this->max_download_buffer_size_) {
    target_size = this->max_download_buffer_size_;
  }
  if (target_size <= current_size) {
    ESP_LOGE(TAG, "Artwork download exceeded transfer limit of %zu bytes",
             this->max_download_buffer_size_);
    return false;
  }

  ESP_LOGD(TAG, "Growing download buffer from %zu to %zu bytes", current_size, target_size);
  bool resized = this->download_buffer_.resize(target_size) == target_size;
  if (resized) this->peak_download_buffer_size_ = std::max(this->peak_download_buffer_size_, target_size);
  return resized;
}

bool ArtworkImage::decode_buffered_data_() {
  if (!this->decoder_ || this->download_buffer_.unread() == 0) {
    return true;
  }

  size_t unread = this->download_buffer_.unread();
  auto fed = this->decoder_->decode(this->download_buffer_.data(), unread);
  if (fed < 0) {
    ESP_LOGE(TAG, "Error when decoding image.");
    return false;
  }
  if (static_cast<size_t>(fed) > unread) {
    ESP_LOGE(TAG, "Decoder consumed %d bytes, but only %zu were buffered", fed, unread);
    return false;
  }
  this->download_buffer_.read(fed);
  return true;
}

void ArtworkImage::finish_download_() {
  if (this->has_newer_pending_update_()) {
    ESP_LOGI(TAG, "Discarding completed artwork because a newer URL is queued");
    this->end_connection_();
    this->start_pending_update_();
    return;
  }
  if (!this->promote_decode_buffer_()) {
    this->fail_download_();
    return;
  }
  const size_t bytes_read = this->downloader_ ? this->downloader_->get_bytes_read() : 0;
  this->log_state_("download-complete");
  ESP_LOGD(TAG, "Image fully downloaded: bytes=%zu image=%dx%d peak_download_buffer=%zu budget=%zu",
           bytes_read, this->width_, this->height_, this->peak_download_buffer_size_,
           this->max_download_buffer_size_);
  ESP_LOGD(TAG, "Total time: %" PRIu32 "s", (uint32_t) (::time(nullptr) - this->start_time_));
  this->end_connection_();
  this->log_state_("download-resources-released");
  App.feed_wdt();
#ifdef USE_LVGL
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  this->get_lv_image_dsc();
#else
  this->get_lv_img_dsc();
#endif
#endif
  this->log_state_("lvgl-descriptor-ready");
  App.feed_wdt();
  this->download_finished_callback_.call(false);
  App.feed_wdt();
  this->log_state_("download-callback-finished");
  this->start_pending_update_();
}

void ArtworkImage::fail_download_() {
  if (this->has_newer_pending_update_()) {
    ESP_LOGW(TAG, "Skipping stale artwork failure because a newer URL is queued");
    this->end_connection_();
    this->start_pending_update_();
    return;
  }
  this->end_connection_();
  this->download_error_callback_.call();
  this->start_pending_update_();
}

void ArtworkImage::queue_pending_update_(const std::string &url) {
  if (!this->validate_url_(url)) {
    return;
  }
  bool replaced = this->update_pending_ && this->pending_url_ != url;
  this->pending_url_ = url;
  this->update_pending_ = true;
  ESP_LOGW(TAG, "Artwork update %s while busy; latest URL will run after current work finishes",
           replaced ? "re-queued" : "queued");
  this->log_state_("update-queued");
}

void ArtworkImage::start_pending_update_() {
  if (!this->update_pending_ || this->is_busy_()) {
    return;
  }
  std::string url = this->pending_url_;
  this->pending_url_.clear();
  this->update_pending_ = false;
  ESP_LOGI(TAG, "Starting queued artwork update");
  this->url_ = url;
  this->update();
}

void ArtworkImage::log_state_(const char *stage) {
  size_t heap_free = 0;
  size_t heap_largest = this->allocator_.get_max_free_block_size();
  size_t internal_free = 0;
  size_t internal_largest = 0;
#ifdef USE_ESP32
  heap_free = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  heap_largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
#endif
  size_t api_state_subscriptions = 0;
#ifdef USE_API_HOMEASSISTANT_STATES
  if (esphome::api::global_api_server != nullptr) {
    api_state_subscriptions = esphome::api::global_api_server->get_state_subs().size();
  }
#endif
  size_t bytes_read = this->downloader_ ? this->downloader_->get_bytes_read() : 0;
  size_t content_length = this->downloader_ ? this->downloader_->content_length : 0;
  size_t retired_bytes = this->retired_buffer_bytes_();
  ESP_LOGD(TAG,
           "State %-24s url_len=%zu http=%zu/%zu dl_buf=%zu/%zu image=%dx%d content=%dx%d@%d,%d decode=%dx%d content=%dx%d@%d,%d retired=%zu retired_bytes=%zu heap_free=%zu heap_largest=%zu internal_free=%zu internal_largest=%zu api_state_subs=%zu pending=%s",
           stage, this->url_.size(), bytes_read, content_length, this->download_buffer_.unread(),
           this->download_buffer_.size(), this->buffer_width_, this->buffer_height_, this->buffer_content_width_,
           this->buffer_content_height_, this->buffer_offset_x_, this->buffer_offset_y_, this->decode_buffer_width_,
           this->decode_buffer_height_, this->decode_content_width_, this->decode_content_height_,
           this->decode_offset_x_, this->decode_offset_y_, this->retired_buffers_.size(), retired_bytes, heap_free,
           heap_largest, internal_free, internal_largest, api_state_subscriptions,
           this->update_pending_ ? "yes" : "no");
}

void ArtworkImage::end_connection_() {
  if (this->downloader_) {
    this->downloader_->end();
    this->downloader_ = nullptr;
  }
  this->decoder_.reset();
  this->discard_decode_buffer_();
  this->download_buffer_.reset();
  this->download_buffer_.shrink_to(this->download_buffer_initial_size_);
}

bool ArtworkImage::validate_url_(const std::string &url) {
  if ((url.length() < 8) || !url.starts_with("http") || (url.find("://") == std::string::npos)) {
    ESP_LOGE(TAG, "URL is invalid and/or must be prefixed with 'http://' or 'https://'");
    return false;
  }
  return true;
}

}  // namespace artwork_image
}  // namespace esphome
