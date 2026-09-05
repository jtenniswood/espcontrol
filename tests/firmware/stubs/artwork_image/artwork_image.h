#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace esphome {
namespace artwork_image {

// Only the hardware boundary is replaced; image_service.cpp is compiled as-is.
class ArtworkImage {
 public:
  void enable_loop() { this->loop_enabled = true; }

  bool loop_enabled{false};
  bool accept_start{true};
  std::vector<uint32_t> started_generations;
  std::function<void(uint32_t)> on_start;

 private:
  bool start_service_update_(uint32_t generation) {
    this->started_generations.push_back(generation);
    if (this->on_start) this->on_start(generation);
    return this->accept_start;
  }

  friend class ImageService;
};

}  // namespace artwork_image
}  // namespace esphome
