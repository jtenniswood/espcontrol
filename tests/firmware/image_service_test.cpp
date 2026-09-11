#include <cassert>
#include <vector>

#include "artwork_image.h"
#include "image_service.h"

using esphome::artwork_image::ArtworkImage;
using esphome::artwork_image::ImageService;
using esphome::artwork_image::ImageRequestPriority;
using esphome::artwork_image::ImageRequestQueue;
using esphome::artwork_image::dispatch_next_image_request;

static void test_queue_ordering() {
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

static void test_request_defers_and_wakes_idle_image() {
  ImageService service;
  ArtworkImage image;
  assert(!image.loop_enabled);

  service.request(&image, 1, ImageRequestPriority::COVER_ART);
  assert(image.loop_enabled);
  assert(image.started_generations.empty());
  assert(!service.is_active(&image));
  assert(service.queued_requests() == 1);

  service.request(&image, 2, ImageRequestPriority::COVER_ART);
  assert(image.started_generations.empty());
  assert(service.queued_requests() == 1);
  service.process_pending();
  assert(image.started_generations == std::vector<uint32_t>{2});
  assert(service.is_active(&image));
  service.process_pending();
  assert(image.started_generations.size() == 1);
}

static void test_completion_defers_next_request() {
  ImageService service;
  ArtworkImage first;
  ArtworkImage next;
  service.request(&first, 1, ImageRequestPriority::TILE);
  service.process_pending();
  service.request(&next, 1, ImageRequestPriority::TILE);
  service.complete(&next);
  assert(service.is_active(&first));

  service.complete(&first);
  assert(!service.is_active(&first));
  assert(next.started_generations.empty());
  assert(service.queued_requests() == 1);
  service.process_pending();
  assert(next.started_generations == std::vector<uint32_t>{1});
  assert(service.is_active(&next));
}

static void test_cancellation_defers_next_request() {
  ImageService service;
  ArtworkImage active;
  ArtworkImage cancelled;
  ArtworkImage next;
  service.request(&active, 1, ImageRequestPriority::TILE);
  service.process_pending();
  service.request(&cancelled, 1, ImageRequestPriority::MODAL);
  service.request(&next, 1, ImageRequestPriority::TILE);

  service.cancel(&cancelled);
  assert(service.is_active(&active));
  assert(service.queued_requests() == 1);
  service.cancel(&active);
  assert(!service.is_active(&active));
  assert(next.started_generations.empty());
  service.process_pending();
  assert(cancelled.started_generations.empty());
  assert(next.started_generations == std::vector<uint32_t>{1});
  assert(service.is_active(&next));
}

static void test_followup_defers_and_wakes_image() {
  ImageService service;
  ArtworkImage image;
  ArtworkImage tile;
  service.request(&image, 1, ImageRequestPriority::COVER_ART);
  service.process_pending();
  service.request(&tile, 1, ImageRequestPriority::TILE);
  image.loop_enabled = false;

  service.complete_and_request(&image, 2, ImageRequestPriority::COVER_ART);
  assert(image.loop_enabled);
  assert(image.started_generations == std::vector<uint32_t>{1});
  assert(tile.started_generations.empty());
  assert(!service.is_active(&image));
  assert(service.queued_requests() == 2);

  service.process_pending();
  assert((image.started_generations == std::vector<uint32_t>{1, 2}));
  assert(tile.started_generations.empty());
  assert(service.is_active(&image));
  service.complete(&image);
  assert(tile.started_generations.empty());
  service.process_pending();
  assert(service.is_active(&tile));
}

static void test_start_callback_cannot_dispatch_recursively() {
  ImageService service;
  ArtworkImage image;
  bool inside_start = false;
  image.on_start = [&](uint32_t generation) {
    assert(!inside_start);
    inside_start = true;
    if (generation == 1) {
      service.complete_and_request(&image, 2, ImageRequestPriority::COVER_ART);
      service.process_pending();
      assert(image.started_generations.size() == 1);
    }
    inside_start = false;
  };

  service.request(&image, 1, ImageRequestPriority::COVER_ART);
  assert(image.started_generations.empty());
  service.process_pending();
  assert((image.started_generations == std::vector<uint32_t>{1, 2}));
  assert(service.is_active(&image));
}

static void test_rejected_start_releases_active_slot() {
  ImageService service;
  ArtworkImage stale;
  ArtworkImage next;
  stale.accept_start = false;
  service.request(&stale, 1, ImageRequestPriority::MODAL);
  service.request(&next, 2, ImageRequestPriority::TILE);
  service.process_pending();
  assert(stale.started_generations == std::vector<uint32_t>{1});
  assert(next.started_generations == std::vector<uint32_t>{2});
  assert(service.is_active(&next));
  assert(service.queued_requests() == 0);
}

int main() {
  test_queue_ordering();
  test_request_defers_and_wakes_idle_image();
  test_completion_defers_next_request();
  test_cancellation_defers_next_request();
  test_followup_defers_and_wakes_image();
  test_start_callback_cannot_dispatch_recursively();
  test_rejected_start_releases_active_slot();
}
