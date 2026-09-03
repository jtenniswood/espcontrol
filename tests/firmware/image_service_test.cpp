#include <cassert>
#include <vector>

#include "image_service.h"

using esphome::artwork_image::ImageRequestPriority;
using esphome::artwork_image::ImageRequestQueue;
using esphome::artwork_image::dispatch_next_image_request;

int main() {
  int background = 1;
  int first_tile = 2;
  int second_tile = 3;
  int cover_art = 4;
  int modal = 5;

  ImageRequestQueue<int> queue;
  queue.enqueue(&background, 1, ImageRequestPriority::BACKGROUND);
  queue.enqueue(&first_tile, 1, ImageRequestPriority::TILE);
  queue.enqueue(&second_tile, 1, ImageRequestPriority::TILE);
  queue.enqueue(&cover_art, 1, ImageRequestPriority::COVER_ART);
  queue.enqueue(&modal, 1, ImageRequestPriority::MODAL);

  ImageRequestQueue<int>::Request request;
  std::vector<int *> started;
  int *active = nullptr;
  bool dispatching = false;
  auto start = [&started](const ImageRequestQueue<int>::Request &next) {
    started.push_back(next.owner);
    return true;
  };

  // Queuing a request must not invoke its start callback synchronously.
  assert(started.empty());
  dispatch_next_image_request(queue, active, dispatching, start);
  assert(started.size() == 1);
  assert(started.back() == &modal);
  assert(active == &modal);

  // An active request blocks dispatch until a later processing pass.
  dispatch_next_image_request(queue, active, dispatching, start);
  assert(started.size() == 1);
  active = nullptr;
  dispatch_next_image_request(queue, active, dispatching, start);
  assert(started.size() == 2);
  assert(started.back() == &cover_art);
  assert(active == &cover_art);

  active = nullptr;
  dispatch_next_image_request(queue, active, dispatching, start);
  assert(started.size() == 3);
  assert(started.back() == &first_tile);
  active = nullptr;
  dispatch_next_image_request(queue, active, dispatching, start);
  assert(started.size() == 4);
  assert(started.back() == &second_tile);
  active = nullptr;
  dispatch_next_image_request(queue, active, dispatching, start);
  assert(started.size() == 5);
  assert(started.back() == &background);
  assert(active == &background);

  active = nullptr;
  dispatch_next_image_request(queue, active, dispatching, start);
  assert(started.size() == 5);

  // Repeated requests from one consumer are coalesced to the latest generation.
  queue.enqueue(&first_tile, 10, ImageRequestPriority::TILE);
  queue.enqueue(&first_tile, 11, ImageRequestPriority::MODAL);
  assert(queue.size() == 1);
  assert(queue.pop_next(request));
  assert(request.owner == &first_tile);
  assert(request.generation == 11);
  assert(request.priority == ImageRequestPriority::MODAL);

  queue.enqueue(&cover_art, 2, ImageRequestPriority::COVER_ART);
  assert(queue.contains(&cover_art));
  assert(queue.remove(&cover_art));
  assert(!queue.contains(&cover_art));

  // A failed start releases the active slot and does not leave queued work behind.
  queue.enqueue(&background, 20, ImageRequestPriority::BACKGROUND);
  auto fail_start = [](const ImageRequestQueue<int>::Request &) { return false; };
  dispatch_next_image_request(queue, active, dispatching, fail_start);
  assert(active == nullptr);
  assert(!queue.pop_next(request));
}
