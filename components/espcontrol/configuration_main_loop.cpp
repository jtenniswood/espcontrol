#include "configuration_main_loop.h"

namespace espcontrol::configuration {

ConfigurationLoadTransaction ConfigurationMainLoop::load(uint8_t *output,
    size_t capacity, std::chrono::milliseconds timeout) {
  Request request;
  request.output = output;
  request.size = capacity;
  const auto completed = submit(request, timeout);
  return {completed.dispatch, completed.loaded};
}

ConfigurationSaveTransaction ConfigurationMainLoop::save_if_generation(
    uint32_t generation, uint16_t version, const uint8_t *document, size_t size,
    std::chrono::milliseconds timeout) {
  Request request;
  request.operation = Operation::SAVE;
  request.document = document;
  request.size = size;
  request.generation = generation;
  request.version = version;
  const auto completed = submit(request, timeout);
  return {completed.dispatch, completed.saved};
}

ConfigurationMainLoop::Request ConfigurationMainLoop::submit(Request request,
    std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (stopped_ || state_ != State::IDLE) {
    request.dispatch = stopped_ ? ConfigurationDispatchStatus::STOPPED
                                : ConfigurationDispatchStatus::BUSY;
    return request;
  }
  request_ = request;
  state_ = State::QUEUED;
  // A request can expire only before main-loop execution starts. Once running,
  // keep its borrowed storage alive and report the actual durable outcome.
  // Returning an ambiguous timeout during commit would allow the web buffer
  // to be reused underneath preparation, persistence or activation.
  if (!changed_.wait_for(lock, timeout, [this] { return state_ != State::QUEUED; })) {
    state_ = State::IDLE;
    request_ = {};
    request.dispatch = ConfigurationDispatchStatus::EXPIRED;
    return request;
  }
  changed_.wait(lock, [this] { return state_ == State::DONE; });
  const auto completed = request_;
  request_ = {};
  state_ = State::IDLE;
  return completed;
}

bool ConfigurationMainLoop::process(ConfigurationService &service) {
  Request request;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stopped_ || state_ != State::QUEUED) return false;
    state_ = State::RUNNING;
    request = request_;
  }
  changed_.notify_one();
  // Never hold the queue mutex while service/driver code runs. Reentrant
  // process(), stop() or an additional worker request must not deadlock it.
  if (request.operation == Operation::LOAD)
    request.loaded = service.load(request.output, request.size);
  else
    request.saved = service.save_if_generation(request.generation, request.version,
                                               request.document, request.size);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    request_ = request;
    state_ = State::DONE;
  }
  changed_.notify_one();
  return true;
}

void ConfigurationMainLoop::stop() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stopped_ = true;
    if (state_ == State::QUEUED) {
      request_.dispatch = ConfigurationDispatchStatus::STOPPED;
      state_ = State::DONE;
    }
    // A running operation still owns the borrowed buffer. Its main-loop stack
    // completes before application teardown and publishes the real result.
  }
  changed_.notify_one();
}

bool ConfigurationMainLoop::pending() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return state_ == State::QUEUED;
}

}  // namespace espcontrol::configuration
