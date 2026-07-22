#ifndef ESPCONTROL_CARD_BACKGROUND_CONTROLLER_H
#define ESPCONTROL_CARD_BACKGROUND_CONTROLLER_H
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace espcontrol::card_background {

enum class RequestAction {
  NONE,
  START,
  QUEUE,
};

struct ResourceState {
  std::string id;
  int width{0};
  int height{0};
  size_t binding_count{0};
  uint32_t source_crc32{0};
  uint32_t retry_deadline_ms{0};
  uint8_t retry_count{0};
  bool active{false};
  bool ready{false};
  bool failed{false};
  bool download_active{false};
  bool download_queued{false};
  bool cache_write_pending{false};

  bool matches(const std::string &candidate_id, int candidate_width,
               int candidate_height) const {
    return active && id == candidate_id && width == candidate_width &&
           height == candidate_height;
  }
};

template<size_t MaxResources>
class Controller {
 public:
  static constexpr int NO_RESOURCE = -1;

  void reset(size_t capacity) {
    capacity_ = capacity > MaxResources ? MaxResources : capacity;
    active_download_ = NO_RESOURCE;
    for (auto &resource : resources_) resource = {};
  }

  size_t capacity() const { return capacity_; }

  ResourceState &resource(size_t index) { return resources_[index]; }
  const ResourceState &resource(size_t index) const { return resources_[index]; }

  int acquire(const std::string &id, int width, int height) {
    if (id.empty() || width <= 0 || height <= 0) return NO_RESOURCE;
    for (size_t index = 0; index < capacity_; index++) {
      if (resources_[index].matches(id, width, height)) return static_cast<int>(index);
    }
    for (size_t index = 0; index < capacity_; index++) {
      if (resources_[index].active) continue;
      resources_[index] = {};
      resources_[index].id = id;
      resources_[index].width = width;
      resources_[index].height = height;
      resources_[index].active = true;
      return static_cast<int>(index);
    }
    return NO_RESOURCE;
  }

  void add_binding(size_t index) {
    if (valid(index) && resources_[index].active) resources_[index].binding_count++;
  }

  void remove_binding(size_t index) {
    if (valid(index) && resources_[index].binding_count > 0) resources_[index].binding_count--;
  }

  bool has_bindings(size_t index) const {
    return valid(index) && resources_[index].binding_count > 0;
  }

  RequestAction request(size_t index) {
    if (!valid(index)) return RequestAction::NONE;
    ResourceState &candidate = resources_[index];
    if (!candidate.active || candidate.width <= 0 || candidate.height <= 0 ||
        candidate.download_active) {
      return RequestAction::NONE;
    }
    if (active_download_ != NO_RESOURCE &&
        active_download_ != static_cast<int>(index)) {
      candidate.download_queued = true;
      return RequestAction::QUEUE;
    }
    candidate.download_active = true;
    candidate.download_queued = false;
    active_download_ = static_cast<int>(index);
    return RequestAction::START;
  }

  int active_download() const { return active_download_; }

  int release_download(size_t index) {
    if (!valid(index)) return NO_RESOURCE;
    ResourceState &finished = resources_[index];
    finished.download_active = false;
    finished.download_queued = false;
    if (active_download_ == static_cast<int>(index)) active_download_ = NO_RESOURCE;
    return next_queued(static_cast<int>(index));
  }

  void mark_ready(size_t index, bool cached) {
    if (!valid(index)) return;
    ResourceState &finished = resources_[index];
    finished.ready = true;
    finished.failed = false;
    finished.retry_count = 0;
    finished.retry_deadline_ms = 0;
    finished.cache_write_pending = !cached;
  }

  bool mark_failed(size_t index, uint32_t now_ms, uint8_t max_retries,
                   uint32_t retry_delay_ms) {
    if (!valid(index) || !resources_[index].active) return false;
    ResourceState &failed = resources_[index];
    failed.ready = false;
    if (failed.retry_count < max_retries) {
      failed.retry_count++;
      failed.retry_deadline_ms = now_ms + retry_delay_ms;
      failed.failed = false;
      return true;
    }
    failed.retry_deadline_ms = 0;
    failed.failed = true;
    return false;
  }

  bool retry_due(size_t index, uint32_t now_ms) const {
    if (!valid(index)) return false;
    const ResourceState &candidate = resources_[index];
    return candidate.active && !candidate.download_active &&
           candidate.retry_deadline_ms != 0 &&
           static_cast<int32_t>(now_ms - candidate.retry_deadline_ms) >= 0;
  }

  void consume_retry(size_t index) {
    if (valid(index)) resources_[index].retry_deadline_ms = 0;
  }

  int next_queued(int excluded = NO_RESOURCE) const {
    for (size_t index = 0; index < capacity_; index++) {
      const ResourceState &candidate = resources_[index];
      if (candidate.active && candidate.download_queued &&
          static_cast<int>(index) != excluded) {
        return static_cast<int>(index);
      }
    }
    return NO_RESOURCE;
  }

  void mark_cache_written(size_t index) {
    if (valid(index)) resources_[index].cache_write_pending = false;
  }

  void deactivate(size_t index) {
    if (!valid(index)) return;
    if (active_download_ == static_cast<int>(index)) active_download_ = NO_RESOURCE;
    resources_[index] = {};
  }

 private:
  bool valid(size_t index) const { return index < capacity_; }

  std::array<ResourceState, MaxResources> resources_{};
  size_t capacity_{0};
  int active_download_{NO_RESOURCE};
};

}  // namespace espcontrol::card_background

#endif
