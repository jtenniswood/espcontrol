#include "page_image_runtime.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>

using namespace espcontrol::pages;
static size_t allocations = 0;
void* operator new(size_t size) {
  ++allocations;
  if (void* p = std::malloc(size ? size : 1)) return p;
  std::abort();
}
void* operator new[](size_t size) {
  return ::operator new(size);
}
void operator delete(void* p) noexcept {
  std::free(p);
}
void operator delete[](void* p) noexcept {
  std::free(p);
}
void operator delete(void* p, std::size_t) noexcept {
  std::free(p);
}
void operator delete[](void* p, std::size_t) noexcept {
  std::free(p);
}
namespace {
struct Origin final : PageImageOrigin {
  bool online{true};
  uint32_t configuration{1}, binding{1}, connection{1};
  bool valid(PageBindingToken t) const override {
    return t.configuration == configuration && t.binding == binding;
  }
  bool connected() const override {
    return online;
  }
  uint32_t connection_generation() const override {
    return connection;
  }
  PageBindingToken token(size_t slot = 0) const {
    return {configuration, binding, 0, static_cast<CardId>(slot), static_cast<uint16_t>(slot), false};
  }
};
struct Requests final : PageImageRequests {
  struct Delivery {
    void* owner{nullptr};
    Receive receive{nullptr};
    size_t slot{0};
    uint64_t request{0};
    void reply(bool success = true, bool cached = false) const {
      if (receive) receive(owner, slot, request, success, cached);
    }
  };
  std::array<Delivery, 20> deliveries{};
  std::array<char, 4097> copied_url{};
  size_t starts{0}, cancels{0}, releases{0};
  bool accepted{true}, synchronous{false}, synchronous_success{true}, cancel_delivers{true};
  std::function<void()> on_start, on_cancel, on_release;
  bool start(size_t slot, uint64_t request, std::string_view url, PageImageOptions options, void* owner,
             Receive receive) override {
    assert(options.width == 200 && options.height == 100);
    assert(url.size() < copied_url.size());
    std::memcpy(copied_url.data(), url.data(), url.size());
    copied_url[url.size()] = '\0';
    ++starts;
    const Delivery delivery{owner, receive, slot, request};
    deliveries[slot] = delivery;
    if (on_start) on_start();
    if (synchronous) delivery.reply(synchronous_success, true);
    return accepted;
  }
  void cancel(size_t slot) override {
    ++cancels;
    const auto old = deliveries[slot];
    if (on_cancel) on_cancel();
    if (cancel_delivers) old.reply();
  }
  void release_surface(size_t) override {
    ++releases;
    if (on_release) on_release();
  }
};
constexpr PageImageOptions options{200, 100, 0xabcdef, true};
template <size_t Slots>
void lifecycle() {
  Requests requests;
  Origin origin;
  PageImageRuntime<Slots> runtime(requests, origin);
  auto token = origin.token();
  assert(!runtime.request(0, token, "url", options));
  assert(!runtime.bind(Slots, token));
  assert(runtime.bind(0, token));
  auto wrong = token;
  ++wrong.card;
  assert(!runtime.request(0, wrong, "wrong card", options));
  wrong = token;
  ++wrong.page;
  assert(!runtime.request(0, wrong, "wrong page", options));
  wrong = token;
  ++wrong.shell;
  assert(!runtime.request(0, wrong, "wrong shell", options));
  wrong = token;
  wrong.back = true;
  assert(!runtime.bind(0, wrong));
  assert(!runtime.request(0, token, "", options));
  assert(!runtime.request(0, token, "url", {}));
  assert(requests.starts == 0);
  {
    std::string url(4096, 'x');
    assert(runtime.request(0, token, url, options));
  }  // No configuration/URL bytes survive by borrowing this destroyed string.
  assert(std::strlen(requests.copied_url.data()) == 4096);
  auto previous = requests.deliveries[0];
  assert(runtime.snapshot(0, token).pending);
  previous.reply(true, true);
  auto state = runtime.snapshot(0, token);
  assert(state.ready && state.cached && !state.pending && !state.failed);
  const auto revision = runtime.revision();
  previous.reply(false);
  assert(runtime.revision() == revision);  // Completed requests are consumed.
  assert(runtime.request(0, token, "replacement", options));
  previous.reply(false);
  assert(runtime.snapshot(0, token).pending);
  auto replacement = requests.deliveries[0];
  replacement.reply(false);
  assert(runtime.snapshot(0, token).failed);

  // Navigation retires tokens synchronously, but no completed surface is freed
  // until maintenance after the view has detached its LVGL descriptor.
  const size_t released = requests.releases;
  runtime.unbind(0);
  replacement.reply();
  assert(requests.releases == released && !runtime.snapshot(0, token).ready);
  ++origin.binding;
  token = origin.token();
  assert(runtime.bind(0, token));
  runtime.loop();
  assert(requests.releases == released + 1);
  assert(runtime.request(0, token, "new page", options));
  replacement.reply();
  assert(runtime.snapshot(0, token).pending);

  // Reconnect invalidates the delivery without freeing a still-attached image
  // and keeps the value binding so the current view can request a new image.
  previous = requests.deliveries[0];
  origin.online = false;
  runtime.loop();
  previous.reply();
  assert(!runtime.snapshot(0, token).ready);
  assert(requests.releases == released + 1);
  ++origin.connection;
  origin.online = true;
  runtime.loop();
  assert(runtime.request(0, token, "reconnected", options));
  previous.reply();
  assert(runtime.snapshot(0, token).pending);
  requests.deliveries[0].reply();
  assert(runtime.snapshot(0, token).ready);

  // Replacing configuration retires active work before the old store is freed.
  previous = requests.deliveries[0];
  runtime.retire();
  ++origin.configuration;
  ++origin.binding;
  token = origin.token();
  assert(runtime.bind(0, token));
  assert(runtime.request(0, token, "new config", options));
  previous.reply();
  assert(runtime.snapshot(0, token).pending);
  requests.deliveries[0].reply();
  assert(runtime.snapshot(0, token).ready);

  // Bounded 1,000-cycle hot path, including cancellation delivering old results.
  const auto count = allocations;
  for (int i = 0; i < 1000; ++i) {
    for (size_t slot = 0; slot < Slots; ++slot) {
      const auto binding = origin.token(slot);
      assert(runtime.bind(slot, binding));
      assert(runtime.request(slot, binding, "image", options));
      requests.deliveries[slot].reply();
      assert(runtime.snapshot(slot, binding).ready);
      runtime.unbind(slot);
    }
    runtime.loop();
    ++origin.binding;
  }
  assert(allocations == count);
}
void reentrancy_and_failure() {
  Requests requests;
  Origin origin;
  PageImageRuntime<1> runtime(requests, origin);
  auto token = origin.token();
  assert(runtime.bind(0, token));
  requests.accepted = false;
  assert(!runtime.request(0, token, "rejected", options));
  assert(runtime.snapshot(0, token).failed);
  requests.synchronous = true;
  assert(runtime.request(0, token, "synchronous success", options));
  assert(runtime.snapshot(0, token).ready);
  requests.synchronous_success = false;
  assert(!runtime.request(0, token, "synchronous error", options));
  assert(runtime.snapshot(0, token).failed);
  requests.synchronous = false;
  requests.accepted = true;
  assert(runtime.request(0, token, "before rebind", options));
  const auto old = requests.deliveries[0];
  bool once = true;
  requests.on_cancel = [&] {
    if (!once) return;
    once = false;
    ++origin.binding;
    token = origin.token();
    assert(runtime.bind(0, token));
    assert(runtime.request(0, token, "cancel replacement", options));
  };
  assert(!runtime.request(0, token, "interrupted", options));
  old.reply();
  assert(runtime.snapshot(0, token).pending);
  requests.deliveries[0].reply();
  assert(runtime.snapshot(0, token).ready);
  requests.on_cancel = {};
  requests.on_start = [&] { runtime.retire(); };
  assert(!runtime.request(0, token, "retired during start", options));
  requests.deliveries[0].reply();
  assert(!runtime.snapshot(0, token).ready);
  requests.on_start = {};
  assert(runtime.bind(0, token));
  assert(runtime.request(0, token, "before cleanup", options));
  runtime.unbind(0);
  assert(runtime.bind(0, token));
  requests.on_release = [&] { runtime.retire(); };
  const auto starts = requests.starts;
  assert(!runtime.request(0, token, "retired during cleanup", options));
  assert(requests.starts == starts);
  requests.on_release = {};

  PageImageRuntime<1> exhausted(requests, origin, UINT64_MAX);
  assert(exhausted.bind(0, token));
  assert(exhausted.request(0, token, "last", options));
  assert(!exhausted.request(0, token, "cannot wrap", options));
  requests.deliveries[0].reply();
  assert(exhausted.snapshot(0, token).ready);
}
void acquisition_and_retained_refresh() {
  Requests requests;
  Origin origin;
  PageImageRuntime<2> runtime(requests, origin);
  const auto first = origin.token(4), second = origin.token(7), third = origin.token(9);
  const auto before = allocations;
  assert(runtime.acquire(first) == 0 && runtime.acquire(first) == 0);
  assert(runtime.acquire(second) == 1 && runtime.acquire(third) == 2);
  assert(allocations == before);
  assert(runtime.request(0, first, "original", options));
  const auto initial = requests.deliveries[0];
  initial.reply(true, true);
  const auto asset = runtime.snapshot(0, first).asset;
  assert(asset == initial.request);
  auto refresh = options;
  refresh.retain_completed = true;
  assert(runtime.request(0, first, "replacement", refresh));
  auto state = runtime.snapshot(0, first);
  assert(state.ready && state.pending && state.cached && state.asset == asset);
  requests.deliveries[0].reply(false);
  state = runtime.snapshot(0, first);
  assert(state.ready && state.failed && !state.pending && state.asset == asset);
  requests.accepted = false;
  assert(!runtime.request(0, first, "rejected refresh", refresh));
  state = runtime.snapshot(0, first);
  assert(state.ready && state.failed && !state.pending && state.asset == asset);
  requests.accepted = true;
  assert(runtime.request(0, first, "successful refresh", refresh));
  requests.deliveries[0].reply();
  state = runtime.snapshot(0, first);
  assert(state.ready && !state.failed && state.asset != asset);
  assert(runtime.request(0, first, "different source", options));
  state = runtime.snapshot(0, first);
  assert(!state.ready && state.pending && state.asset == 0);
  runtime.unbind(0);  // A retired binding cannot retain the former card's pixels.
  assert(runtime.acquire(third) == 0);
  assert(runtime.request(0, third, "new card", refresh));
  state = runtime.snapshot(0, third);
  assert(!state.ready && state.pending && state.asset == 0);
  initial.reply();
  assert(!runtime.snapshot(0, third).ready);
}
void cancellation_preserves_completed_surface() {
  Requests requests;
  Origin origin;
  PageImageRuntime<2> runtime(requests, origin);
  const auto token = origin.token();
  assert(runtime.bind(0, token) && runtime.request(0, token, "first", options));
  requests.deliveries[0].reply();
  const auto asset = runtime.snapshot(0, token).asset;
  auto refresh = options;
  refresh.retain_completed = true;
  assert(runtime.request(0, token, "refresh", refresh));
  const auto old = requests.deliveries[0];
  const auto released = requests.releases;
  const auto before = allocations;
  assert(runtime.cancel_request(0, token) && allocations == before);
  auto snapshot = runtime.snapshot(0, token);
  assert(snapshot.ready && !snapshot.pending && !snapshot.failed && snapshot.asset == asset &&
         requests.releases == released);
  old.reply(false);
  assert(runtime.snapshot(0, token).asset == asset);
  assert(runtime.cancel_request(0, token, false) && !runtime.snapshot(0, token).ready &&
         requests.releases == released);
  bool rebound = false;
  requests.on_cancel = [&]() {
    if (rebound) return;
    rebound = true;
    ++origin.binding;
    assert(runtime.bind(0, origin.token()));
  };
  assert(runtime.cancel_request(0, token));
  requests.on_cancel = {};
  assert(runtime.bound_to(0, origin.token()) && !runtime.cancel_request(0, token));
  runtime.loop();
  assert(requests.releases == released + 1);
}
}  // namespace
int main() {
  cancellation_preserves_completed_surface();
  acquisition_and_retained_refresh();
  lifecycle<4>();   // S3.
  lifecycle<20>();  // Largest P4 profile.
  reentrancy_and_failure();
}
