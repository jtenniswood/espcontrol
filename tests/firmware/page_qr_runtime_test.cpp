#include "page_qr_runtime.h"

#include <array>
#include <cassert>
#include <cstdlib>
#include <functional>

#include "device_ui_limits.h"
using namespace espcontrol::pages;
static size_t cpp_calls = 0;
void* operator new(size_t size) {
  ++cpp_calls;
  if (void* memory = std::malloc(size ? size : 1)) return memory;
  std::abort();
}
void* operator new[](size_t size) {
  return ::operator new(size);
}
void operator delete(void* memory) noexcept {
  std::free(memory);
}
void operator delete[](void* memory) noexcept {
  ::operator delete(memory);
}
void operator delete(void* memory, size_t) noexcept {
  ::operator delete(memory);
}
void operator delete[](void* memory, size_t) noexcept {
  ::operator delete(memory);
}
namespace {
struct Arena final : PageArenaAllocator {
  size_t bytes{0}, blocks{0}, calls{0};
  bool fail{false};
  void* allocate(size_t size) override {
    ++calls;
    if (fail) return nullptr;
    auto* result = std::malloc(size);
    assert(result);
    bytes += size;
    ++blocks;
    return result;
  }
  void release(void* memory, size_t size) override {
    assert(memory && bytes >= size && blocks);
    bytes -= size;
    --blocks;
    std::free(memory);
  }
};
struct Origin final : PageQrOrigin {
  uint32_t configuration{1}, binding{1};
  size_t count{20};
  bool online{true};
  bool valid(PageBindingToken token) const override {
    return token.configuration == configuration && token.binding == binding && token.shell < count &&
           !token.back;
  }
  PageBindingToken token(size_t slot = 0) const {
    return {configuration, binding, 0, static_cast<CardId>(slot), static_cast<uint16_t>(slot), false};
  }
};
struct Encoder final : PageQrEncoder {
  bool fail{false}, wrong_side{false};
  size_t calls{0};
  std::function<void()> hook;
  bool encode(std::string_view payload, int side, bool compact, PageQrMatrix& matrix) override {
    ++calls;
    const auto expected = payload.front();
    if (hook) hook();
    // Rebinding must not overwrite the in-flight encoder's input.
    for (char byte : payload) assert(byte == expected);
    matrix.clear();
    if (fail) return false;
    matrix.layout = page_qr_layout(1, side + (wrong_side ? 1 : 0), compact);
    matrix.dark(0, 0, expected == 'A');
    return matrix.layout.valid();
  }
};
struct Frames final : PageQrFrames {
  size_t count{0}, waits{0}, assigns{0}, resets{0};
  bool fail{false}, held{false}, reject{false};
  std::array<const PageQrMatrix*, 20> sources{};
  std::array<PageQrMatrix, 20> previous{};
  std::array<bool, 20> shown{};
  std::function<void()> fence_hook, reset_hook, assign_hook;
  bool initialize(size_t slots) override {
    if (fail) return false;
    assert(!count && slots <= sources.size());
    count = slots;
    return true;
  }
  void hide(size_t slot) override {
    shown[slot] = false;
  }
  bool wait_for_draw() override {
    ++waits;
    for (size_t i = 0; i < count; ++i)
      if (sources[i]) {
        assert(sources[i]->bytes == previous[i].bytes);
        assert(sources[i]->layout.side == previous[i].layout.side);
      }
    if (fence_hook) fence_hook();
    return !held;
  }
  void reset_after_draw(size_t slot) override {
    assert(!held);
    ++resets;
    sources[slot] = nullptr;
    shown[slot] = false;
    if (reset_hook) reset_hook();
  }
  bool assign_after_draw(size_t slot, const PageQrMatrix& matrix) override {
    assert(!sources[slot]);
    ++assigns;
    sources[slot] = &matrix;
    previous[slot] = matrix;
    shown[slot] = true;
    if (assign_hook) assign_hook();
    return !reject;
  }
  bool shutdown_after_draw() override {
    for (size_t i = 0; i < count; ++i) assert(!sources[i]);
    count = 0;
    return true;
  }
};
using Runtime = PageQrRuntime<>;
void lifecycle(size_t count) {
  Arena arena;
  Origin origin;
  origin.count = count;
  Encoder encoder;
  Frames frames;
  Runtime runtime(arena, encoder, frames, origin, count);
  assert(!runtime.available() && runtime.initialize());
  assert(arena.bytes == runtime.reserved_bytes() && arena.blocks == 1);
  const auto bytes = arena.bytes, calls = arena.calls, allocations = cpp_calls;
  uint64_t last_asset = 0;
  std::array<char, PAGE_QR_PAYLOAD_MAX> document{};
  for (unsigned cycle = 0; cycle < 1000; ++cycle) {
    ++origin.binding;
    origin.configuration = cycle == 500 ? 0 : cycle;
    origin.online = cycle % 2;
    document.fill(cycle % 2 ? 'A' : 'B');
    for (size_t i = 0; i < count; ++i) {
      const auto token = origin.token(i);
      assert(runtime.bind(i, token, {document.data(), document.size()}));
      assert(runtime.find(token) == i && runtime.size(i, token, 132, cycle % 3 == 0));
      assert(runtime.snapshot(i, token).pending && !frames.shown[i]);
    }
    document.fill('X');
    runtime.loop();
    const auto encoded = encoder.calls, assigned = frames.assigns, waited = frames.waits;
    for (size_t i = 0; i < count; ++i) {
      const auto token = origin.token(i);
      auto state = runtime.snapshot(i, token);
      assert(state.ready && !state.pending && !state.failed && state.asset > last_asset && frames.shown[i]);
      last_asset = state.asset;
      auto wrong = token;
      ++wrong.card;
      assert(!runtime.snapshot(i, wrong).ready && !runtime.size(i, wrong, 240, false));
      runtime.unbind(i, wrong);
      assert(runtime.snapshot(i, token).ready);
      assert(runtime.size(i, token, 132, cycle % 3 == 0));
    }
    runtime.loop();
    assert(encoded == encoder.calls && assigned == frames.assigns && waited == frames.waits);
    for (size_t i = 0; i < count; ++i) runtime.unbind(i, origin.token(i));
    runtime.loop();
    assert(arena.bytes == bytes && arena.calls == calls && cpp_calls == allocations);
  }
  assert(runtime.shutdown() && !runtime.available() && !arena.bytes && !arena.blocks);
}
void failures() {
  Arena arena;
  Origin origin;
  Encoder encoder;
  Frames frames;
  Runtime runtime(arena, encoder, frames, origin, 2);
  arena.fail = true;
  assert(!runtime.initialize());
  arena.fail = false;
  frames.fail = true;
  assert(!runtime.initialize() && !arena.bytes && !arena.blocks);
  frames.fail = false;
  assert(runtime.initialize());
  auto token = origin.token();
  assert(!runtime.bind(2, token, "AAAA"));
  token.back = true;
  assert(!runtime.bind(0, token, "AAAA"));
  token = origin.token();
  assert(runtime.bind(0, token, "AAAA"));
  assert(!runtime.bind(1, token, "AAAA"));
  assert(!runtime.size(0, token, 0, true));
  assert(runtime.size(0, token, 132, true));
  encoder.fail = true;
  runtime.loop();
  assert(runtime.snapshot(0, token).failed && !frames.shown[0]);
  encoder.fail = false;
  encoder.wrong_side = true;
  assert(runtime.size(0, token, 133, true));
  runtime.loop();
  assert(runtime.snapshot(0, token).failed && !frames.shown[0]);
  encoder.wrong_side = false;
  frames.reject = true;
  assert(runtime.size(0, token, 134, true));
  runtime.loop();
  assert(runtime.snapshot(0, token).failed && !frames.shown[0]);
  frames.reject = false;
  assert(runtime.size(0, token, 135, true));
  runtime.loop();
  assert(runtime.snapshot(0, token).ready);
  // Hidden/stale input cannot replace the visible record.
  auto stale = token;
  ++stale.configuration;
  assert(!runtime.bind(0, stale, "BBBB") && runtime.snapshot(0, token).ready);
  assert(!runtime.bind(0, token, {}) && !runtime.snapshot(0, token).ready);
  runtime.loop();
  std::array<char, 256> oversized{};
  assert(!runtime.bind(0, token, {oversized.data(), oversized.size()}));
  assert(runtime.bind(0, token, "AAAA") && runtime.size(0, token, 132, true));
  runtime.loop();
  frames.held = true;
  assert(runtime.bind(0, token, "BBBB") && runtime.size(0, token, 240, false));
  const auto calls = encoder.calls;
  runtime.loop();
  assert(encoder.calls == calls && runtime.snapshot(0, token).pending);
  assert(!runtime.shutdown() && !runtime.available() && arena.bytes);
  assert(!runtime.bind(0, token, "AAAA"));
  frames.held = false;
  assert(runtime.shutdown() && !arena.bytes && runtime.initialize());
  runtime.retire();
  runtime.loop();
  assert(runtime.shutdown());
  for (size_t capacity : {0u, 256u}) {
    Runtime bad(arena, encoder, frames, origin, capacity);
    assert(!bad.initialize());
  }
}
void reentrant() {
  for (unsigned hook = 0; hook < 4; ++hook) {
    Arena arena;
    Origin origin;
    Encoder encoder;
    Frames frames;
    Runtime runtime(arena, encoder, frames, origin, 2);
    assert(runtime.initialize());
    const auto old = origin.token();
    assert(runtime.bind(0, old, "AAAA") && runtime.size(0, old, 132, true));
    bool fired = false;
    const auto replace = [&] {
      if (fired) return;
      fired = true;
      // Published configuration zero is valid; old document input is owned.
      origin.configuration = 0;
      ++origin.binding;
      const auto token = origin.token();
      assert(runtime.bind(0, token, "BBBB") && runtime.size(0, token, 240, false));
      runtime.loop();
      assert(!runtime.shutdown());
    };
    if (hook == 0) frames.fence_hook = replace;
    if (hook == 1) frames.reset_hook = replace;
    if (hook == 2) encoder.hook = replace;
    if (hook == 3) frames.assign_hook = replace;
    runtime.loop();
    assert(fired && !runtime.snapshot(0, old).ready && !runtime.snapshot(0, origin.token()).ready);
    assert(!frames.shown[0]);
    runtime.loop();
    assert(runtime.snapshot(0, origin.token()).ready && frames.shown[0]);
    assert(!frames.sources[0]->dark(0, 0));
    runtime.retire();
    runtime.loop();
    assert(runtime.shutdown());
  }
}
void exhaustion() {
  Arena arena;
  Origin origin;
  Encoder encoder;
  Frames frames;
  Runtime runtime(arena, encoder, frames, origin, 2, UINT64_MAX - 1);
  assert(runtime.initialize());
  auto token = origin.token();
  assert(runtime.bind(0, token, "AAAA") && runtime.size(0, token, 132, true));
  runtime.loop();
  assert(runtime.snapshot(0, token).asset == UINT64_MAX);
  // No wrapped identity may make an old frame current again.
  assert(!runtime.size(0, token, 240, false) && !runtime.snapshot(0, token).ready);
  assert(!runtime.bind(0, token, "BBBB"));
  runtime.loop();
  assert(!frames.shown[0] && runtime.shutdown());
}
}  // namespace
int main() {
  for (const auto& profile : espcontrol::DEVICE_UI_PROFILES) lifecycle(profile.slot_count);
  failures();
  reentrant();
  exhaustion();
}
