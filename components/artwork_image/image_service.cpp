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
  this->queue_.enqueue(owner, generation, priority);
  this->dispatch_next_();
}

void ImageService::complete(ArtworkImage *owner) {
  if (this->active_ == owner) this->active_ = nullptr;
  this->dispatch_next_();
}

void ImageService::cancel(ArtworkImage *owner) {
  this->queue_.remove(owner);
  if (this->active_ == owner) this->active_ = nullptr;
  this->dispatch_next_();
}

void ImageService::dispatch_next_() {
  if (this->dispatching_ || this->active_ != nullptr) return;
  this->dispatching_ = true;
  ImageRequestQueue<ArtworkImage>::Request request;
  while (this->active_ == nullptr && this->queue_.pop_next(request)) {
    this->active_ = request.owner;
    if (!request.owner->start_service_update_(request.generation)) this->active_ = nullptr;
  }
  this->dispatching_ = false;
}

}  // namespace artwork_image
}  // namespace esphome
