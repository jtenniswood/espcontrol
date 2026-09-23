#include "home_assistant_endpoint_probe.h"
#include "home_assistant_endpoint_policy.h"

#include <atomic>
#include <new>
#include <cstring>
#include <strings.h>
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"

#ifdef USE_ESP_IDF
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#endif

namespace espcontrol {
namespace {
constexpr uint32_t DEADLINE_MS = 3000;
}
struct EndpointProbeService::Job {
  std::string url;
  EndpointProbeResult result;
  std::atomic<bool> done{false};
  uint32_t started{0};
  std::string content_type;
};
EndpointProbeService::EndpointProbeService() = default;
EndpointProbeService::~EndpointProbeService() { shutdown(); }

bool EndpointProbeService::start(const std::string &origin, uint32_t generation) {
#ifdef USE_ESP_IDF
  if (job_ && !job_->done.load(std::memory_order_acquire)) return false;
  auto *raw = new (std::nothrow) Job;
  if (!raw) return false;
  std::shared_ptr<Job> job(raw);
  job->url = origin + "/manifest.json";
  job->result.generation = generation;
  job->started = esphome::millis();
  auto *argument = new (std::nothrow) std::shared_ptr<Job>(job);
  if (!argument) return false;
  TaskHandle_t task = nullptr;
  BaseType_t created = xTaskCreateWithCaps(run, "ha_endpoint", 8192, argument, 1,
                                         &task, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (created != pdPASS)
    created = xTaskCreateWithCaps(run, "ha_endpoint", 8192, argument, 1,
                                 &task, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (created != pdPASS) { delete argument; return false; }
  job_ = std::move(job);
  delivered_ = false;
  return true;
#else
  (void) origin;
  (void) generation;
  return false;
#endif
}

bool EndpointProbeService::take(EndpointProbeResult &result) {
  if (!job_ || delivered_) return false;
  const bool done = job_->done.load(std::memory_order_acquire);
  if (!done && esphome::millis() - job_->started < DEADLINE_MS) return false;
  // Do not read fields the worker could still be writing at the deadline.
  if (done) result = job_->result;
  else result = {job_->result.generation, EndpointProbeOutcome::TRANSPORT, 0, -1};
  delivered_ = true;
  return true;
}
void EndpointProbeService::shutdown() {
  // Worker owns its job until socket cleanup completes. No callback refers to
  // the service; shutdown need not wait on DNS or delete a live network task.
  job_.reset();
  delivered_ = true;
}

void EndpointProbeService::run(void *argument) {
#ifdef USE_ESP_IDF
  {
    auto *holder = static_cast<std::shared_ptr<Job> *>(argument);
    std::shared_ptr<Job> job = std::move(*holder);
    delete holder;
    esp_http_client_config_t config{};
    config.url = job->url.c_str();
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = DEADLINE_MS;
    config.buffer_size = 1024;
    config.disable_auto_redirect = true;
    config.auth_type = HTTP_AUTH_TYPE_NONE;
    config.user_data = job.get();
    config.event_handler = [](esp_http_client_event_t *event) -> esp_err_t {
      auto *job = static_cast<Job *>(event->user_data);
      if (esphome::millis() - job->started >= DEADLINE_MS) return ESP_ERR_TIMEOUT;
      if (event->event_id == HTTP_EVENT_ON_HEADER && event->header_key && event->header_value &&
          strcasecmp(event->header_key, "Content-Type") == 0)
        job->content_type.assign(event->header_value, std::min<size_t>(strlen(event->header_value), 128));
      return ESP_OK;
    };
    const size_t begin = job->url.find("://") + 3;
    const size_t end = job->url.find('/', begin);
    const std::string authority = job->url.substr(begin, end - begin);
    const std::string host = authority.front() == '[' ? authority.substr(0, authority.find(']') + 1)
                                                     : authority.substr(0, authority.find(':'));
    const bool https = job->url.rfind("https://", 0) == 0;
    const std::string address = home_assistant_endpoint::normalize_address(host);
    const bool ipv4_local = address.find(':') == std::string::npos && home_assistant_endpoint::local_address(address);
    const bool local = ipv4_local || host == "[::1]" || address.rfind("fe80:", 0) == 0 ||
        (host.size() > 6 && host.compare(host.size() - 6, 6, ".local") == 0) || host == "localhost";
    // Same opt-in local TLS policy used by the artwork downloaders.
    if (https && local) config.skip_cert_common_name_check = true;
    else if (https) config.crt_bundle_attach = esp_crt_bundle_attach;
    auto client = esp_http_client_init(&config);
    if (!client) {
      job->result.outcome = EndpointProbeOutcome::RESOURCE;
      job->result.error = ESP_ERR_NO_MEM;
    } else {
      esp_http_client_set_header(client, "Accept", "application/manifest+json, application/json");
      esp_http_client_set_header(client, "Accept-Encoding", "identity");
      esp_err_t error = esp_http_client_open(client, 0);
      if (error == ESP_OK) {
        const uint32_t elapsed = esphome::millis() - job->started;
        if (elapsed >= DEADLINE_MS) error = ESP_ERR_TIMEOUT;
        else {
          esp_http_client_set_timeout_ms(client, DEADLINE_MS - elapsed);
          if (esp_http_client_fetch_headers(client) < 0) error = ESP_FAIL;
        }
      }
      job->result.status = esp_http_client_get_status_code(client);
      if (esphome::millis() - job->started >= DEADLINE_MS) error = ESP_ERR_TIMEOUT;
      job->result.error = error;
      const int status = job->result.status;
      auto type = job->content_type.substr(0, job->content_type.find(';'));
      type = home_assistant_endpoint::trim_copy(type);
      std::transform(type.begin(), type.end(), type.begin(), ::tolower);
      if (status == 401 || status == 403) job->result.outcome = EndpointProbeOutcome::ACCESS_DENIED;
      else if (error != ESP_OK) job->result.outcome = EndpointProbeOutcome::TRANSPORT;
      else if (status == 200 && (type == "application/json" || type == "application/manifest+json"))
        job->result.outcome = EndpointProbeOutcome::READY;
      else job->result.outcome = EndpointProbeOutcome::RESPONSE;
      esp_http_client_close(client);
      esp_http_client_cleanup(client);
    }
    job->done.store(true, std::memory_order_release);
  }
  vTaskDeleteWithCaps(nullptr);
#else
  (void) argument;
#endif
}
}  // namespace espcontrol
