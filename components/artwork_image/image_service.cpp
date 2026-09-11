#include "image_service.h"

#include "artwork_image.h"

namespace esphome {
namespace artwork_image {

ImageService &ImageService::instance() {
  static ImageService service;
  return service;
}

void ImageService::request(ArtworkImage *owner, uint32_t generation, ImageRequestPriority priority) {
  if (owner == nullptr || this->active_ == owner) return;
  owner->enable_loop();
  this->queue_.enqueue(owner, generation, priority);
}

void ImageService::complete(ArtworkImage *owner) {
  if (this->active_ == owner) this->active_ = nullptr;
}

void ImageService::complete_and_request(ArtworkImage *owner, uint32_t generation,
                                        ImageRequestPriority priority) {
  if (owner == nullptr || this->active_ != owner) return;
  owner->enable_loop();
  this->queue_.enqueue(owner, generation, priority);
  this->active_ = nullptr;
}

void ImageService::cancel(ArtworkImage *owner) {
  this->queue_.remove(owner);
  if (this->active_ == owner) this->active_ = nullptr;
}

void ImageService::process_pending() {
  dispatch_next_image_request(
      this->queue_, this->active_, this->dispatching_,
      [](const ImageRequestQueue<ArtworkImage>::Request &request) {
        return request.owner->start_service_update_(request.generation);
      });
}

}  // namespace artwork_image
}  // namespace esphome
