#include "s3_artwork_transfer.h"

#if defined(USE_ESP_IDF) && defined(CONFIG_IDF_TARGET_ESP32S3)

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>

#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include "image_pipeline_policy.h"

namespace esphome {
namespace artwork_image {

static const char *const TAG = "artwork_image.s3_transfer";
static constexpr size_t MAX_TRANSFER_SIZE = 2 * 1024 * 1024;
static constexpr size_t INITIAL_TRANSFER_CAPACITY = 16 * 1024;
static constexpr size_t MAX_REDIRECT_URL_LENGTH = 2048;
static constexpr int MAX_REDIRECTS = 3;

static std::string sanitize_url(const std::string &url) {
  const size_t query = url.find('?');
  return query == std::string::npos ? url : url.substr(0, query) + "?...";
}

static bool is_https_url(const std::string &url) {
  return url.rfind("https://", 0) == 0;
}

static bool is_supported_url(const std::string &url) {
  return url.rfind("http://", 0) == 0 || is_https_url(url);
}

static std::string url_host(const std::string &url) {
  const bool secure = is_https_url(url);
  const size_t host_start = secure ? 8 : 7;
  if (url.size() <= host_start) return {};
  const size_t host_end = url.find_first_of("/?#", host_start);
  std::string authority = url.substr(
      host_start, host_end == std::string::npos ? std::string::npos
                                                : host_end - host_start);
  const size_t at = authority.rfind('@');
  if (at != std::string::npos) authority = authority.substr(at + 1);

  std::string host;
  if (!authority.empty() && authority.front() == '[') {
    const size_t end = authority.find(']');
    host = end == std::string::npos ? authority : authority.substr(1, end - 1);
  } else {
    const size_t colon = authority.find(':');
    host = colon == std::string::npos ? authority : authority.substr(0, colon);
  }
  std::transform(host.begin(), host.end(), host.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return host;
}

static bool is_private_or_local_host(const std::string &host) {
  if (host == "localhost" || host == "homeassistant.local" ||
      (host.size() > 6 && host.compare(host.size() - 6, 6, ".local") == 0) ||
      host == "::1" || host.rfind("fe80:", 0) == 0) {
    return true;
  }

  int parts[4] = {-1, -1, -1, -1};
  const char *cursor = host.c_str();
  char *end = nullptr;
  for (int i = 0; i < 4; i++) {
    const long value = std::strtol(cursor, &end, 10);
    if (end == cursor || value < 0 || value > 255) return false;
    parts[i] = static_cast<int>(value);
    if (i < 3) {
      if (*end != '.') return false;
      cursor = end + 1;
    } else if (*end != '\0') {
      return false;
    }
  }
  return parts[0] == 10 || parts[0] == 127 ||
         (parts[0] == 192 && parts[1] == 168) ||
         (parts[0] == 172 && parts[1] >= 16 && parts[1] <= 31) ||
         (parts[0] == 169 && parts[1] == 254);
}

struct S3ArtworkTransferService::Job {
  ArtworkImage *owner{nullptr};
  uint32_t generation{0};
  uint8_t priority{0};
  uint64_t sequence{0};
  char *url{nullptr};
  std::vector<http_request::Header> headers;
  bool allow_insecure_local_urls{false};
  int timeout_ms{10000};
  std::atomic<bool> cancelled{false};
  std::atomic<esp_http_client_handle_t> client{nullptr};

  ~Job() { heap_caps_free(this->url); }
};

struct S3ArtworkTransferService::Transfer {
  Job *job{nullptr};
  uint8_t *data{nullptr};
  size_t size{0};
  size_t capacity{0};
  bool allocation_failed{false};
  bool oversized{false};
  bool redirect_failed{false};
  bool redirect_response{false};
  std::string redirect_location;
  uint32_t response_ready_ms{0};
  uint32_t first_byte_ms{0};
};

S3ArtworkTransferResult::~S3ArtworkTransferResult() {
  heap_caps_free(this->data);
}

uint8_t *S3ArtworkTransferResult::release_data() {
  uint8_t *data = this->data;
  this->data = nullptr;
  return data;
}

S3ArtworkTransferService &S3ArtworkTransferService::instance() {
  static S3ArtworkTransferService service;
  return service;
}

S3ArtworkTransferService::S3ArtworkTransferService() {
  this->mutex_ = xSemaphoreCreateMutex();
  if (!this->mutex_) return;
  const BaseType_t created = xTaskCreate(
      task_entry_, "s3_artwork_http", 8192, this, 2, &this->task_);
  this->ready_ = created == pdPASS && this->task_ != nullptr;
  if (!this->ready_) {
    ESP_LOGE(TAG, "Could not start guarded ESP32-S3 artwork transfer task");
  }
}

bool S3ArtworkTransferService::submit(
    ArtworkImage *owner, uint32_t generation, uint8_t priority,
    const std::string &url, const std::vector<http_request::Header> &headers,
    bool allow_insecure_local_urls, int timeout_ms) {
  if (!this->ready_ || !owner || !is_supported_url(url)) return false;
  auto *job = new (std::nothrow) Job();
  if (!job) return false;
  job->url = static_cast<char *>(heap_caps_malloc(
      url.size() + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!job->url) {
    delete job;
    return false;
  }
  std::memcpy(job->url, url.c_str(), url.size() + 1);
  job->owner = owner;
  job->generation = generation;
  job->priority = priority;
  job->headers = headers;
  job->allow_insecure_local_urls = allow_insecure_local_urls;
  job->timeout_ms = timeout_ms;

  this->lock_();
  this->cancel_locked_(owner);
  if (this->pending_count_ >= 16) {
    this->unlock_();
    delete job;
    return false;
  }
  job->sequence = this->next_sequence_++;
  this->pending_[this->pending_count_++] = job;
  this->unlock_();
  xTaskNotifyGive(this->task_);
  return true;
}

void S3ArtworkTransferService::cancel(ArtworkImage *owner) {
  if (!this->ready_ || !owner) return;
  this->lock_();
  this->cancel_locked_(owner);
  this->unlock_();
}

S3ArtworkTransferResult *S3ArtworkTransferService::take(
    ArtworkImage *owner, uint32_t generation, bool *allocation_failed) {
  if (!this->ready_ || !owner) return nullptr;
  if (allocation_failed) *allocation_failed = false;
  S3ArtworkTransferResult *match = nullptr;
  this->lock_();
  for (auto &failure : this->allocation_failures_) {
    if (failure.owner != owner) continue;
    if (allocation_failed && failure.generation == generation) {
      *allocation_failed = true;
    }
    failure = AllocationFailure{};
  }
  for (size_t i = 0; i < this->completed_count_;) {
    S3ArtworkTransferResult *candidate = this->completed_[i];
    if (candidate->owner != owner) {
      ++i;
      continue;
    }
    this->remove_completed_at_locked_(i);
    if (background_transfer_result_is_current(
            generation, candidate->generation, false) &&
        match == nullptr) {
      match = candidate;
    } else {
      delete candidate;
    }
  }
  this->unlock_();
  return match;
}

void S3ArtworkTransferService::task_entry_(void *arg) {
  static_cast<S3ArtworkTransferService *>(arg)->task_loop_();
}

void S3ArtworkTransferService::task_loop_() {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    while (true) {
      Job *job = this->next_job_();
      if (!job) break;
      S3ArtworkTransferResult *result = this->perform_(job);
      this->lock_();
      this->active_ = nullptr;
      if (result) {
        this->discard_completed_for_owner_locked_(result->owner);
        if (this->completed_count_ < 16) {
          this->completed_[this->completed_count_++] = result;
        } else {
          delete result;
          this->record_allocation_failure_locked_(job);
        }
      } else if (!job->cancelled.load()) {
        this->record_allocation_failure_locked_(job);
      }
      this->unlock_();
      delete job;
    }
  }
}

S3ArtworkTransferService::Job *S3ArtworkTransferService::next_job_() {
  this->lock_();
  size_t best = 16;
  for (size_t i = 0; i < this->pending_count_;) {
    if (this->pending_[i]->cancelled.load()) {
      delete this->pending_[i];
      for (size_t next = i + 1; next < this->pending_count_; ++next) {
        this->pending_[next - 1] = this->pending_[next];
      }
      this->pending_[--this->pending_count_] = nullptr;
      continue;
    }
    if (best == 16 || p4_pipeline_candidate_precedes(
                          this->pending_[i]->priority,
                          this->pending_[i]->sequence,
                          this->pending_[best]->priority,
                          this->pending_[best]->sequence)) {
      best = i;
    }
    ++i;
  }
  if (best == 16) {
    this->unlock_();
    return nullptr;
  }
  Job *job = this->pending_[best];
  for (size_t next = best + 1; next < this->pending_count_; ++next) {
    this->pending_[next - 1] = this->pending_[next];
  }
  this->pending_[--this->pending_count_] = nullptr;
  this->active_ = job;
  this->unlock_();
  return job;
}

esp_err_t S3ArtworkTransferService::http_event_(esp_http_client_event_t *event) {
  auto *transfer = static_cast<Transfer *>(event->user_data);
  if (!transfer || !transfer->job) return ESP_OK;
  if (transfer->job->cancelled.load()) return ESP_FAIL;

  const uint32_t now = millis();
  if (event->event_id == HTTP_EVENT_ON_HEADER) {
    if (transfer->response_ready_ms == 0) transfer->response_ready_ms = now;
    if (event->header_key && event->header_value) {
      std::string key(event->header_key);
      std::transform(key.begin(), key.end(), key.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      if (key == "location") {
        const size_t length = std::strlen(event->header_value);
        if (length == 0 || length > MAX_REDIRECT_URL_LENGTH) {
          transfer->redirect_failed = true;
        } else {
          transfer->redirect_location.assign(event->header_value, length);
        }
      }
    }
    return ESP_OK;
  }
  if (event->event_id == HTTP_EVENT_REDIRECT) {
    // Leave automatic redirect handling disabled. perform_() will resolve the
    // captured Location and create a fresh client with destination-specific
    // TLS settings.
    transfer->redirect_response = true;
    if (transfer->redirect_location.empty()) transfer->redirect_failed = true;
    return ESP_OK;
  }
  if (event->event_id != HTTP_EVENT_ON_DATA || event->data_len <= 0) {
    return ESP_OK;
  }
  if (transfer->redirect_failed || transfer->redirect_response) {
    return ESP_OK;
  }

  if (transfer->first_byte_ms == 0) transfer->first_byte_ms = now;
  const size_t incoming = static_cast<size_t>(event->data_len);
  if (incoming > MAX_TRANSFER_SIZE - transfer->size) {
    transfer->oversized = true;
    return ESP_FAIL;
  }
  const size_t required = transfer->size + incoming;
  if (required > transfer->capacity) {
    size_t reported_content_length = 0;
    if (transfer->capacity == 0 && event->client) {
      const int64_t content_length =
          esp_http_client_get_content_length(event->client);
      if (content_length > 0) {
        reported_content_length =
            static_cast<uint64_t>(content_length) > MAX_TRANSFER_SIZE
                ? MAX_TRANSFER_SIZE + 1
                : static_cast<size_t>(content_length);
      }
    }
    const size_t next_capacity = p4_pipeline_transfer_capacity(
        transfer->capacity, required, reported_content_length,
        INITIAL_TRANSFER_CAPACITY, MAX_TRANSFER_SIZE);
    if (next_capacity == 0) {
      transfer->oversized = true;
      return ESP_FAIL;
    }
    uint8_t *resized = static_cast<uint8_t *>(heap_caps_realloc(
        transfer->data, next_capacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!resized) {
      transfer->allocation_failed = true;
      return ESP_FAIL;
    }
    transfer->data = resized;
    transfer->capacity = next_capacity;
  }
  std::memcpy(transfer->data + transfer->size, event->data, incoming);
  transfer->size += incoming;
  return ESP_OK;
}

S3ArtworkTransferResult *S3ArtworkTransferService::perform_(Job *job) {
  if (!job || job->cancelled.load()) return nullptr;
  auto *result = new (std::nothrow) S3ArtworkTransferResult();
  if (!result) return nullptr;
  result->owner = job->owner;
  result->generation = job->generation;
  result->request_started_ms = millis();

  Transfer transfer;
  transfer.job = job;
  std::string current_url(job->url);
  esp_err_t error = ESP_OK;
  int status = 0;
  int redirect_count = 0;

  while (true) {
    if (job->cancelled.load()) {
      delete result;
      return nullptr;
    }
    if (!is_supported_url(current_url)) {
      error = ESP_ERR_INVALID_ARG;
      break;
    }

    const BackgroundTransferTlsMode tls_mode = background_transfer_tls_mode(
        is_https_url(current_url),
        is_private_or_local_host(url_host(current_url)),
        job->allow_insecure_local_urls);

    esp_http_client_config_t config{};
    config.url = current_url.c_str();
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = job->timeout_ms;
    // Redirects are handled outside this client so each destination receives
    // a newly evaluated TLS policy.
    config.disable_auto_redirect = true;
    config.auth_type = HTTP_AUTH_TYPE_NONE;
    config.event_handler = http_event_;
    config.user_data = &transfer;
    config.buffer_size = 8192;
    if (tls_mode == BackgroundTransferTlsMode::VERIFIED_HTTPS) {
      config.crt_bundle_attach = esp_crt_bundle_attach;
    } else if (tls_mode ==
               BackgroundTransferTlsMode::INSECURE_LOCAL_HTTPS) {
      config.skip_cert_common_name_check = true;
      ESP_LOGW(TAG, "Using explicitly permitted insecure TLS for local artwork: %s",
               sanitize_url(current_url).c_str());
    }

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
      error = ESP_ERR_NO_MEM;
      break;
    }
    for (const auto &header : job->headers) {
      esp_http_client_set_header(client, header.name.c_str(),
                                 header.value.c_str());
    }

    // Publish and retire the active client under the same mutex used by
    // cancel_locked_(). This prevents cancellation from racing cleanup and
    // calling esp_http_client_cancel_request() on a freed handle.
    this->lock_();
    if (job->cancelled.load()) {
      this->unlock_();
      esp_http_client_cleanup(client);
      delete result;
      return nullptr;
    }
    job->client.store(client);
    this->unlock_();

    error = esp_http_client_perform(client);
    status = esp_http_client_get_status_code(client);
    this->lock_();
    job->client.store(nullptr);
    esp_http_client_cleanup(client);
    this->unlock_();

    if (job->cancelled.load()) {
      heap_caps_free(transfer.data);
      transfer.data = nullptr;
      delete result;
      return nullptr;
    }
    if (transfer.allocation_failed) {
      error = ESP_ERR_NO_MEM;
      break;
    }
    if (transfer.oversized) {
      error = ESP_ERR_INVALID_SIZE;
      break;
    }
    if (transfer.redirect_failed) {
      error = ESP_ERR_INVALID_RESPONSE;
      break;
    }
    if (background_transfer_should_follow_redirect(
            transfer.redirect_response, !transfer.redirect_location.empty())) {
      if (redirect_count >= MAX_REDIRECTS) {
        ESP_LOGE(TAG, "Too many artwork redirects from %s",
                 sanitize_url(current_url).c_str());
        error = ESP_ERR_INVALID_RESPONSE;
        break;
      }
      std::string redirected = background_transfer_resolve_redirect_url(
          current_url, transfer.redirect_location);
      if (!is_supported_url(redirected)) {
        ESP_LOGE(TAG, "Artwork redirect has an unsupported destination from %s",
                 sanitize_url(current_url).c_str());
        error = ESP_ERR_INVALID_ARG;
        break;
      }
      current_url = redirected;
      ++redirect_count;
      transfer.redirect_location.clear();
      transfer.redirect_failed = false;
      transfer.redirect_response = false;
      transfer.response_ready_ms = 0;
      transfer.first_byte_ms = 0;
      heap_caps_free(transfer.data);
      transfer.data = nullptr;
      transfer.size = 0;
      transfer.capacity = 0;
      status = 0;
      continue;
    }
    break;
  }

  const uint32_t completed_at = millis();
  result->status = status;
  result->error = error;
  result->data = transfer.data;
  transfer.data = nullptr;
  result->size = transfer.size;
  result->response_ready_ms =
      transfer.response_ready_ms ? transfer.response_ready_ms : completed_at;
  result->first_byte_ms =
      transfer.first_byte_ms ? transfer.first_byte_ms : result->response_ready_ms;
  result->transfer_complete_ms = completed_at;
  return result;
}

void S3ArtworkTransferService::cancel_locked_(ArtworkImage *owner) {
  for (size_t i = 0; i < this->pending_count_;) {
    if (this->pending_[i]->owner != owner) {
      ++i;
      continue;
    }
    delete this->pending_[i];
    for (size_t next = i + 1; next < this->pending_count_; ++next) {
      this->pending_[next - 1] = this->pending_[next];
    }
    this->pending_[--this->pending_count_] = nullptr;
  }
  if (this->active_ && this->active_->owner == owner) {
    this->active_->cancelled.store(true);
    esp_http_client_handle_t client = this->active_->client.load();
    if (client) esp_http_client_cancel_request(client);
  }
  this->discard_completed_for_owner_locked_(owner);
  for (auto &failure : this->allocation_failures_) {
    if (failure.owner == owner) failure = AllocationFailure{};
  }
}

void S3ArtworkTransferService::record_allocation_failure_locked_(
    const Job *job) {
  if (!job) return;
  for (auto &failure : this->allocation_failures_) {
    if (failure.owner && failure.owner != job->owner) continue;
    failure.owner = job->owner;
    failure.generation = job->generation;
    return;
  }
  ESP_LOGE(TAG, "No slot available to report S3 transfer allocation failure");
}

S3ArtworkTransferResult *
S3ArtworkTransferService::remove_completed_at_locked_(size_t index) {
  if (index >= this->completed_count_) return nullptr;
  S3ArtworkTransferResult *result = this->completed_[index];
  for (size_t next = index + 1; next < this->completed_count_; ++next) {
    this->completed_[next - 1] = this->completed_[next];
  }
  this->completed_[--this->completed_count_] = nullptr;
  return result;
}

void S3ArtworkTransferService::discard_completed_for_owner_locked_(
    ArtworkImage *owner) {
  for (size_t i = 0; i < this->completed_count_;) {
    if (this->completed_[i]->owner != owner) {
      ++i;
      continue;
    }
    delete this->remove_completed_at_locked_(i);
  }
}

void S3ArtworkTransferService::lock_() {
  xSemaphoreTake(this->mutex_, portMAX_DELAY);
}

void S3ArtworkTransferService::unlock_() { xSemaphoreGive(this->mutex_); }

}  // namespace artwork_image
}  // namespace esphome

#endif
