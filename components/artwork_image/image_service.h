#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace esphome {
namespace artwork_image {

class ArtworkImage;

// A single queue coordinates every dynamic image consumer. Higher-priority
// work starts first, while equal-priority work remains first-in, first-out.
enum class ImageRequestPriority : uint8_t {
  BACKGROUND = 0,
  TILE = 1,
  COVER_ART = 2,
  MODAL = 3,
};

template<typename Owner> class ImageRequestQueue {
 public:
  struct Request {
    Owner *owner{nullptr};
    uint32_t generation{0};
    ImageRequestPriority priority{ImageRequestPriority::BACKGROUND};
    uint64_t sequence{0};
  };

  void enqueue(Owner *owner, uint32_t generation, ImageRequestPriority priority) {
    auto existing = std::find_if(this->requests_.begin(), this->requests_.end(),
                                 [owner](const Request &request) { return request.owner == owner; });
    if (existing != this->requests_.end()) {
      existing->generation = generation;
      existing->priority = priority;
      return;
    }
    this->requests_.push_back(Request{owner, generation, priority, this->next_sequence_++});
  }

  bool remove(Owner *owner) {
    auto previous_size = this->requests_.size();
    this->requests_.erase(
        std::remove_if(this->requests_.begin(), this->requests_.end(),
                       [owner](const Request &request) { return request.owner == owner; }),
        this->requests_.end());
    return this->requests_.size() != previous_size;
  }

  bool pop_next(Request &request) {
    if (this->requests_.empty()) return false;
    auto selected = std::max_element(
        this->requests_.begin(), this->requests_.end(), [](const Request &left, const Request &right) {
          if (left.priority != right.priority) return left.priority < right.priority;
          return left.sequence > right.sequence;
        });
    request = *selected;
    this->requests_.erase(selected);
    return true;
  }

  bool contains(Owner *owner) const {
    return std::any_of(this->requests_.begin(), this->requests_.end(),
                       [owner](const Request &request) { return request.owner == owner; });
  }

  size_t size() const { return this->requests_.size(); }

 private:
  std::vector<Request> requests_{};
  uint64_t next_sequence_{0};
};

class ImageService {
 public:
  static ImageService &instance();

  void request(ArtworkImage *owner, uint32_t generation, ImageRequestPriority priority);
  void complete(ArtworkImage *owner);
  void complete_and_request(ArtworkImage *owner, uint32_t generation, ImageRequestPriority priority);
  void cancel(ArtworkImage *owner);

  bool is_active(const ArtworkImage *owner) const { return this->active_ == owner; }
  size_t queued_requests() const { return this->queue_.size(); }

 private:
  void dispatch_next_();

  ArtworkImage *active_{nullptr};
  ImageRequestQueue<ArtworkImage> queue_{};
  bool dispatching_{false};
};

}  // namespace artwork_image
}  // namespace esphome
