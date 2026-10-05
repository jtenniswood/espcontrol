#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <string>

#if defined(USE_ESP_IDF) && (defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32P4))

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

struct BackgroundArtworkTransferResult {
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

  static void *operator new(size_t size, const std::nothrow_t &) noexcept;
  static void operator delete(void *pointer) noexcept;
  static void operator delete(void *pointer, const std::nothrow_t &) noexcept;
  ~BackgroundArtworkTransferResult();
  uint8_t *release_data();
};

// Moves blocking HTTP transactions away from the ESPHome display loop on
// ESP32-S3 and ESP32-P4. ImageService still serializes image consumers; the
// worker returns a complete, generation-tagged transfer for decoding.
class BackgroundArtworkTransferService {
 public:
  static BackgroundArtworkTransferService &instance();

  bool submit(ArtworkImage *owner, uint32_t generation,
              const std::string &url,
              std::vector<http_request::Header> headers,
              bool allow_insecure_local_urls, int timeout_ms,
              size_t reserved_free_bytes,
              size_t reserved_largest_block_bytes);
  void cancel(ArtworkImage *owner);
  BackgroundArtworkTransferResult *take(ArtworkImage *owner, uint32_t generation,
                                bool *allocation_failed);

 private:
  BackgroundArtworkTransferService();
  BackgroundArtworkTransferService(const BackgroundArtworkTransferService &) = delete;
  BackgroundArtworkTransferService &operator=(const BackgroundArtworkTransferService &) = delete;

  struct Job;
  static constexpr size_t QUEUE_CAPACITY = 2;
  struct Transfer;
  struct AllocationFailure {
    ArtworkImage *owner{nullptr};
    uint32_t generation{0};
  };

  static void task_entry_(void *arg);
  static esp_err_t http_event_(esp_http_client_event_t *event);
  void task_loop_();
  Job *next_job_();
  BackgroundArtworkTransferResult *perform_(Job *job);
  void cancel_locked_(ArtworkImage *owner);
  void record_allocation_failure_locked_(const Job *job);
  BackgroundArtworkTransferResult *remove_completed_at_locked_(size_t index);
  void discard_completed_for_owner_locked_(ArtworkImage *owner);
  void lock_();
  void unlock_();

  SemaphoreHandle_t mutex_{nullptr};
  TaskHandle_t task_{nullptr};
  bool ready_{false};
  Job *pending_[QUEUE_CAPACITY]{};
  size_t pending_count_{0};
  Job *active_{nullptr};
  BackgroundArtworkTransferResult *completed_[QUEUE_CAPACITY]{};
  size_t completed_count_{0};
  AllocationFailure allocation_failures_[QUEUE_CAPACITY]{};
};

}  // namespace artwork_image
}  // namespace esphome

#endif
