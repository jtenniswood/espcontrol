#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>

#include "configuration_service.h"

namespace espcontrol::configuration {

enum class ConfigurationDispatchStatus : uint8_t { COMPLETED, BUSY, EXPIRED, STOPPED };
struct ConfigurationLoadTransaction {
  ConfigurationDispatchStatus dispatch{ConfigurationDispatchStatus::STOPPED};
  ServiceLoadResult result{};
};
struct ConfigurationSaveTransaction {
  ConfigurationDispatchStatus dispatch{ConfigurationDispatchStatus::STOPPED};
  ServiceSaveResult result{};
};

// One HTTP-task request, borrowing its document until the synchronous caller
// returns. No full-document copy, heap queue, or LVGL work on the HTTP task.
// Only the application main loop calls process(). The owner and service outlive
// callers; stop() must run before shutdown tears down the service/runtime.
class ConfigurationMainLoop {
 public:
  ConfigurationMainLoop() = default;
  ConfigurationMainLoop(const ConfigurationMainLoop &) = delete;
  ConfigurationMainLoop &operator=(const ConfigurationMainLoop &) = delete;

  ConfigurationLoadTransaction load(uint8_t *output, size_t capacity,
      std::chrono::milliseconds queue_timeout = std::chrono::seconds(5));
  ConfigurationSaveTransaction save_if_generation(uint32_t expected_generation,
      uint16_t version, const uint8_t *document, size_t size,
      std::chrono::milliseconds queue_timeout = std::chrono::seconds(5));

  // At most one transaction per iteration. Service preparation, durable commit,
  // runtime activation and compatibility mirroring all execute on this caller.
  bool process(ConfigurationService &service);
  void stop();
  bool pending() const;

 private:
  enum class State : uint8_t { IDLE, QUEUED, RUNNING, DONE };
  enum class Operation : uint8_t { LOAD, SAVE };
  struct Request {
    Operation operation{Operation::LOAD};
    uint8_t *output{nullptr};
    const uint8_t *document{nullptr};
    size_t size{0};
    uint32_t generation{0};
    uint16_t version{0};
    ConfigurationDispatchStatus dispatch{ConfigurationDispatchStatus::COMPLETED};
    ServiceLoadResult loaded{};
    ServiceSaveResult saved{};
  };
  Request submit(Request request, std::chrono::milliseconds queue_timeout);
  mutable std::mutex mutex_;
  std::condition_variable changed_;
  State state_{State::IDLE};
  bool stopped_{false};
  Request request_{};
};

}  // namespace espcontrol::configuration
