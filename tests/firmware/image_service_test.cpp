#include <cassert>

#include "image_service.h"

using esphome::artwork_image::ImageRequestPriority;
using esphome::artwork_image::ImageRequestQueue;

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
  assert(queue.pop_next(request));
  assert(request.owner == &modal);
  assert(queue.pop_next(request));
  assert(request.owner == &cover_art);
  assert(queue.pop_next(request));
  assert(request.owner == &first_tile);
  assert(queue.pop_next(request));
  assert(request.owner == &second_tile);
  assert(queue.pop_next(request));
  assert(request.owner == &background);
  assert(!queue.pop_next(request));

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
}
