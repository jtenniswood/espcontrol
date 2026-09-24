#include "home_assistant_endpoint_probe.h"

#include <arpa/inet.h>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <new>
#include <strings.h>
#include <sys/socket.h>
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"
#ifdef USE_ESP_IDF
#if __has_include("lwip/opt.h")
#include "lwip/opt.h"
#endif
#ifndef LWIP_IPV6
#define LWIP_IPV6 0
#endif
#endif

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

#ifdef USE_ESP_IDF
bool local_or_private_address(const char *text) {
  in_addr ipv4{};
  if (inet_pton(AF_INET, text, &ipv4) == 1) {
    const uint32_t value = ntohl(ipv4.s_addr);
    const uint8_t first = static_cast<uint8_t>(value >> 24);
    const uint8_t second = static_cast<uint8_t>(value >> 16);
    return first == 10 || first == 127 || (first == 172 && second >= 16 && second <= 31) ||
        (first == 192 && second == 168) || (first == 169 && second == 254);
  }
#if LWIP_IPV6
  in6_addr ipv6{};
  if (inet_pton(AF_INET6, text, &ipv6) != 1) return false;
  if (IN6_IS_ADDR_V4MAPPED(&ipv6)) {
    const uint32_t mapped = (static_cast<uint32_t>(ipv6.s6_addr[12]) << 24) |
        (static_cast<uint32_t>(ipv6.s6_addr[13]) << 16) |
        (static_cast<uint32_t>(ipv6.s6_addr[14]) << 8) | ipv6.s6_addr[15];
    const uint8_t first = static_cast<uint8_t>(mapped >> 24);
    const uint8_t second = static_cast<uint8_t>(mapped >> 16);
    return first == 10 || first == 127 || (first == 172 && second >= 16 && second <= 31) ||
        (first == 192 && second == 168) || (first == 169 && second == 254);
  }
  return IN6_IS_ADDR_LOOPBACK(&ipv6) || IN6_IS_ADDR_LINKLOCAL(&ipv6) ||
      (ipv6.s6_addr[0] & 0xfe) == 0xfc;
#else
  return false;
#endif
}

bool local_tls_host(const char *host) {
  in_addr ipv4{};
  if (inet_pton(AF_INET, host, &ipv4) == 1) {
    const uint32_t value = ntohl(ipv4.s_addr);
    const uint8_t first = static_cast<uint8_t>(value >> 24);
    const uint8_t second = static_cast<uint8_t>(value >> 16);
    return first == 10 || first == 127 || (first == 172 && second >= 16 && second <= 31) ||
        (first == 192 && second == 168) || (first == 169 && second == 254);
  }
#if LWIP_IPV6
  in6_addr ipv6{};
  if (inet_pton(AF_INET6, host, &ipv6) == 1)
    return IN6_IS_ADDR_LOOPBACK(&ipv6) || IN6_IS_ADDR_LINKLOCAL(&ipv6);
#endif
  const size_t length = std::strlen(host);
  return std::strcmp(host, "localhost") == 0 ||
      (length > 6 && strcasecmp(host + length - 6, ".local") == 0);
}
#endif
}
struct EndpointProbeService::Job {
  char url[544]{};
  EndpointProbeResult result;
  std::atomic<bool> done{false};
  std::atomic<uint8_t> references{2};
  uint32_t started{0};
  bool require_local_destination{false};
  char content_type[129]{};
};

EndpointProbeService::EndpointProbeService() = default;
EndpointProbeService::~EndpointProbeService() { shutdown(); }

void EndpointProbeService::release(Job *job) {
  if (job && job->references.fetch_sub(1, std::memory_order_acq_rel) == 1) delete job;
}

bool EndpointProbeService::start(const std::string &origin, uint32_t generation,
                                 bool require_local_destination) {
#ifdef USE_ESP_IDF
  if (job_ && !job_->done.load(std::memory_order_acquire)) return false;
  if (job_) { release(job_); job_ = nullptr; }
  // The single Job allocation carries both worker and service ownership; do
  // not wrap it in shared_ptr, whose separately allocated control block may
  // throw even after a nothrow object allocation succeeded.
  auto *job = new (std::nothrow) Job;
  if (!job) return false;
  const int url_length = std::snprintf(job->url, sizeof(job->url), "%s/manifest.json", origin.c_str());
  if (url_length < 0 || static_cast<size_t>(url_length) >= sizeof(job->url)) {
    job->references.store(1, std::memory_order_relaxed);
    release(job);
    return false;
  }
  job->result.generation = generation;
  job->started = esphome::millis();
  job->require_local_destination = require_local_destination;
  TaskHandle_t task = nullptr;
  BaseType_t created = xTaskCreateWithCaps(run, "ha_endpoint", 8192, job, 1,
                                         &task, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (created != pdPASS)
    created = xTaskCreateWithCaps(run, "ha_endpoint", 8192, job, 1,
                                 &task, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (created != pdPASS) {
    job->references.store(1, std::memory_order_relaxed);
    release(job);
    return false;
  }
  job_ = job;
  delivered_ = false;
  return true;
#else
  (void) origin;
  (void) generation;
  (void) require_local_destination;
  return false;
#endif
}

bool EndpointProbeService::take(EndpointProbeResult &result) {
  if (!job_ || delivered_) return false;
  const bool done = job_->done.load(std::memory_order_acquire);
  if (!done && esphome::millis() - job_->started < DEADLINE_MS) return false;
  // Do not read fields the worker could still be writing at the deadline.
  if (done) {
    result = job_->result;
    release(job_);
    job_ = nullptr;
  } else result = {job_->result.generation, EndpointProbeOutcome::TRANSPORT, 0, -1};
  delivered_ = true;
  return true;
}

void EndpointProbeService::shutdown() {
  // The worker releases its own reference after socket cleanup; this remains
  // safe when the DNS or socket task outlives the resolver.
  if (job_) release(job_);
  job_ = nullptr;
  delivered_ = true;
}

void EndpointProbeService::run(void *argument) {
#ifdef USE_ESP_IDF
  auto *job = static_cast<Job *>(argument);
  const char *scheme = std::strstr(job->url, "://");
  const char *authority = scheme ? scheme + 3 : nullptr;
  const char *authority_end = authority ? std::strchr(authority, '/') : nullptr;
  size_t host_length = authority && authority_end
      ? static_cast<size_t>(authority_end - authority) : 0;
  const char *host = authority;
  if (host_length && host[0] == '[') {
    const char *closing = static_cast<const char *>(std::memchr(host, ']', host_length));
    if (closing) { ++host; host_length = static_cast<size_t>(closing - host); }
    else host_length = 0;
  } else if (host_length) {
    const char *colon = static_cast<const char *>(std::memchr(host, ':', host_length));
    if (colon) host_length = static_cast<size_t>(colon - host);
  }
  char host_buffer[256]{};
  if (host_length == 0 || host_length >= sizeof(host_buffer)) {
    job->result.outcome = EndpointProbeOutcome::RESPONSE;
    job->result.error = -2;
    job->done.store(true, std::memory_order_release);
    release(job);
    vTaskDeleteWithCaps(nullptr);
    return;
  }
  std::memcpy(host_buffer, host, host_length);
  if (job->require_local_destination) {
    bool safe = local_or_private_address(host_buffer);
    if (!safe) {
      addrinfo hints{};
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;
      addrinfo *addresses = nullptr;
      if (getaddrinfo(host_buffer, nullptr, &hints, &addresses) == 0 && addresses) {
        safe = true;
        for (addrinfo *address = addresses; address; address = address->ai_next) {
#if LWIP_IPV6
          char resolved[INET6_ADDRSTRLEN]{};
#else
          char resolved[INET_ADDRSTRLEN]{};
#endif
          const void *source = nullptr;
          if (address->ai_family == AF_INET)
            source = &reinterpret_cast<sockaddr_in *>(address->ai_addr)->sin_addr;
#if LWIP_IPV6
            else if (address->ai_family == AF_INET6)
              source = &reinterpret_cast<sockaddr_in6 *>(address->ai_addr)->sin6_addr;
#endif
            if (!source || !inet_ntop(address->ai_family, source, resolved, sizeof(resolved)) ||
                !local_or_private_address(resolved)) {
            safe = false;
            break;
          }
        }
        freeaddrinfo(addresses);
      }
    }
    if (!safe) {
      // An unauthenticated mDNS TXT value must not route later token-bearing
      // artwork requests to a public or otherwise unverified destination.
      job->result.outcome = EndpointProbeOutcome::RESPONSE;
      job->result.error = -3;
      job->done.store(true, std::memory_order_release);
      release(job);
      vTaskDeleteWithCaps(nullptr);
      return;
    }
  }

  esp_http_client_config_t config{};
  config.url = job->url;
  config.method = HTTP_METHOD_GET;
  config.timeout_ms = DEADLINE_MS;
  config.buffer_size = 1024;
  config.disable_auto_redirect = true;
  config.auth_type = HTTP_AUTH_TYPE_NONE;
  config.user_data = job;
  config.event_handler = [](esp_http_client_event_t *event) -> esp_err_t {
    auto *active = static_cast<Job *>(event->user_data);
    if (esphome::millis() - active->started >= DEADLINE_MS) return ESP_ERR_TIMEOUT;
    if (event->event_id == HTTP_EVENT_ON_HEADER && event->header_key && event->header_value &&
        strcasecmp(event->header_key, "Content-Type") == 0)
      std::snprintf(active->content_type, sizeof(active->content_type), "%.*s", 128,
                    event->header_value);
    return ESP_OK;
  };
  const bool https = std::strncmp(job->url, "https://", 8) == 0;
  const bool local = local_tls_host(host_buffer);
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
    char *type_end = std::strchr(job->content_type, ';');
    if (type_end) *type_end = '\0';
    char *type = job->content_type;
    while (*type == ' ' || *type == '\t') ++type;
    size_t type_length = std::strlen(type);
    while (type_length && (type[type_length - 1] == ' ' || type[type_length - 1] == '\t'))
      type[--type_length] = '\0';
    for (char *character = type; *character; ++character)
      *character = static_cast<char>(std::tolower(static_cast<unsigned char>(*character)));
    if (status == 401 || status == 403) job->result.outcome = EndpointProbeOutcome::ACCESS_DENIED;
    else if (error == ESP_ERR_NO_MEM) job->result.outcome = EndpointProbeOutcome::RESOURCE;
    else if (error != ESP_OK) job->result.outcome = EndpointProbeOutcome::TRANSPORT;
    else if (status == 200 && (std::strcmp(type, "application/json") == 0 ||
                               std::strcmp(type, "application/manifest+json") == 0))
      job->result.outcome = EndpointProbeOutcome::READY;
    else job->result.outcome = EndpointProbeOutcome::RESPONSE;
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
  }
  job->done.store(true, std::memory_order_release);
  release(job);
  vTaskDeleteWithCaps(nullptr);
#else
  (void) argument;
#endif
}
}  // namespace espcontrol
