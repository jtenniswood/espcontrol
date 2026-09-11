#include <cassert>
#include <cstdint>

#include "card_background_controller.h"

using espcontrol::card_background::Controller;
using espcontrol::card_background::RequestAction;

int main() {
  Controller<4> controller;
  controller.reset(3);
  assert(controller.capacity() == 3);

  int kitchen = controller.acquire("kitchen", 200, 200);
  assert(kitchen == 0);
  assert(controller.acquire("kitchen", 200, 200) == kitchen);
  int kitchen_wide = controller.acquire("kitchen", 400, 200);
  int hall = controller.acquire("hall", 200, 200);
  assert(kitchen_wide == 1);
  assert(hall == 2);
  assert(controller.acquire("full", 200, 200) == Controller<4>::NO_RESOURCE);

  controller.add_binding(kitchen);
  controller.add_binding(kitchen);
  assert(controller.has_bindings(kitchen));
  assert(controller.resource(kitchen).binding_count == 2);
  controller.remove_binding(kitchen);
  assert(controller.resource(kitchen).binding_count == 1);

  assert(controller.request(kitchen) == RequestAction::START);
  assert(controller.active_download() == kitchen);
  assert(controller.request(hall) == RequestAction::QUEUE);
  assert(controller.next_queued() == hall);
  controller.mark_ready(kitchen, false);
  assert(controller.resource(kitchen).ready);
  assert(controller.resource(kitchen).cache_write_pending);
  assert(controller.release_download(kitchen) == hall);
  assert(controller.active_download() == Controller<4>::NO_RESOURCE);
  assert(controller.request(hall) == RequestAction::START);

  controller.release_download(hall);
  assert(controller.mark_failed(hall, 1000, 3, 750));
  assert(!controller.retry_due(hall, 1749));
  assert(controller.retry_due(hall, 1750));
  controller.consume_retry(hall);
  for (uint8_t retry = 1; retry < 3; retry++) {
    assert(controller.mark_failed(hall, 2000 + retry, 3, 750));
  }
  assert(!controller.mark_failed(hall, 3000, 3, 750));
  assert(controller.resource(hall).failed);
  assert(controller.resource(hall).retry_deadline_ms == 0);

  controller.mark_ready(hall, true);
  assert(controller.resource(hall).ready);
  assert(!controller.resource(hall).cache_write_pending);
  assert(controller.resource(hall).retry_count == 0);

  controller.remove_binding(kitchen);
  assert(!controller.has_bindings(kitchen));
  controller.deactivate(kitchen);
  int reused = controller.acquire("new", 100, 100);
  assert(reused == kitchen);
  assert(controller.resource(reused).id == "new");

  controller.reset(99);
  assert(controller.capacity() == 4);
  assert(controller.active_download() == Controller<4>::NO_RESOURCE);
  for (size_t index = 0; index < controller.capacity(); index++) {
    assert(!controller.resource(index).active);
  }
}
