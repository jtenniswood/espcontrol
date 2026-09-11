#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#if defined(USE_ESP_IDF) && defined(CONFIG_IDF_TARGET_ESP32S3)

#include <vector>

#include "esp_err.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esphome/components/http_request/http_request.h"

namespace esphome {
namespace artwork_image {

class ArtworkImage;

struct S3ArtworkTransferResult {
  ArtworkImage *owner{nullptr};
  uint32_t generation{0};
  int status{0};
  esp_err_t error{ESP_OK};
  uint8_t *data{nullptr};
  size_t size{0};
  uint32_t request_started_ms{0};
  uint32_t response_ready_ms{0};
  uint32_t first_byte_ms{0};
  uint32_t transfer_complete_ms{0};

  ~S3ArtworkTransferResult();
  uint8_t *release_data();
};

// Owns all ESP32-S3 artwork networking. ImageService still serializes image
// consumers; this worker only moves the blocking HTTP transaction away from
// the ESPHome loop and returns a complete, generation-tagged transfer.
class S3ArtworkTransferService {
 public:
  static S3ArtworkTransferService &instance();

  bool submit(ArtworkImage *owner, uint32_t generation, uint8_t priority,
              const std::string &url,
              const std::vector<http_request::Header> &headers,
              bool allow_insecure_local_urls, int timeout_ms);
  void cancel(ArtworkImage *owner);
  S3ArtworkTransferResult *take(ArtworkImage *owner, uint32_t generation,
                                bool *allocation_failed);

 private:
  S3ArtworkTransferService();
  S3ArtworkTransferService(const S3ArtworkTransferService &) = delete;
  S3ArtworkTransferService &operator=(const S3ArtworkTransferService &) = delete;

  struct Job;
  struct Transfer;
  struct AllocationFailure {
    ArtworkImage *owner{nullptr};
    uint32_t generation{0};
  };

  static void task_entry_(void *arg);
  static esp_err_t http_event_(esp_http_client_event_t *event);
  void task_loop_();
  Job *next_job_();
  S3ArtworkTransferResult *perform_(Job *job);
  void cancel_locked_(ArtworkImage *owner);
  void record_allocation_failure_locked_(const Job *job);
  S3ArtworkTransferResult *remove_completed_at_locked_(size_t index);
  void discard_completed_for_owner_locked_(ArtworkImage *owner);
  void lock_();
  void unlock_();

  SemaphoreHandle_t mutex_{nullptr};
  TaskHandle_t task_{nullptr};
  bool ready_{false};
  Job *pending_[16]{};
  size_t pending_count_{0};
  Job *active_{nullptr};
  S3ArtworkTransferResult *completed_[16]{};
  size_t completed_count_{0};
  AllocationFailure allocation_failures_[16]{};
  uint64_t next_sequence_{0};
};

}  // namespace artwork_image
}  // namespace esphome

#endif
