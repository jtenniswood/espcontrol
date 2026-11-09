#pragma once

#include <cassert>
#include <cstring>
#include <new>

#include "page_qr_matrix.h"
#include "page_renderer_runtime.h"

namespace espcontrol::pages {
// Local QR contents remain available without an HA connection. The scene checks
// the complete read-only binding, including while the renderer binds/paints.
class PageQrOrigin {
 public:
  virtual ~PageQrOrigin() = default;
  virtual bool valid(PageBindingToken) const = 0;
};
struct PageQrBindingIdentity {
  static bool active(PageBindingToken t) {
    return t.binding != 0;
  }
  static bool eligible(PageBindingToken t) {
    return active(t) && !t.back;
  }
  static bool same(PageBindingToken a, PageBindingToken b) {
    return active(a) && a.configuration == b.configuration && a.binding == b.binding && a.page == b.page &&
           a.card == b.card && a.shell == b.shell && a.back == b.back;
  }
};
class PageQrFrames {
 public:
  virtual ~PageQrFrames() = default;
  // Fallible scene preflight only. Failure leaves no live frame resources.
  virtual bool initialize(size_t slots) = 0;
  // Allocation-free revocation only: no style changes, invalidation or dispatch.
  virtual void hide(size_t slot) = 0;
  // Safe application-loop maintenance only, outside HA/LVGL/native dispatch.
  virtual bool wait_for_draw() = 0;
  virtual void reset_after_draw(size_t slot) = 0;
  virtual bool assign_after_draw(size_t slot, const PageQrMatrix&) = 0;
  // Refuse shutdown while a decoder still borrows a matrix; caller retries.
  virtual bool shutdown_after_draw() = 0;
};
struct PageQrSnapshot {
  bool pending{false}, ready{false}, failed{false};
  uint64_t asset{0};
};

// One arena lease per visible slot, independent of the image downloader pool.
// Bind, size, unbind and retire only copy fixed metadata and hide old drawing.
// Pixels and persistent descriptors change in loop(), after a hardware fence.
// Token/Origin/Identity can also represent a demanded local modal opening.
template <typename Token = PageBindingToken, typename Origin = PageQrOrigin,
          typename Identity = PageQrBindingIdentity>
class PageQrRuntime {
 public:
  PageQrRuntime(PageArenaAllocator& allocator, PageQrEncoder& encoder, PageQrFrames& frames, Origin& origin,
                size_t capacity, uint64_t first_asset = 1)
      : allocator_(allocator),
        encoder_(encoder),
        frames_(frames),
        origin_(origin),
        capacity_(capacity),
        next_(first_asset) {}
  ~PageQrRuntime() {
    const bool stopped = shutdown();
    assert(stopped);
    (void)stopped;
  }
  PageQrRuntime(const PageQrRuntime&) = delete;
  PageQrRuntime& operator=(const PageQrRuntime&) = delete;
  bool initialize() {
    if (busy_ || stopping_) return false;
    if (slots_) return true;
    if (!capacity_ || capacity_ > UINT8_MAX) return false;
    Guard guard(busy_);
    auto* memory = allocator_.allocate(reserved_bytes());
    if (!memory) return false;
    auto* slots = static_cast<Slot*>(memory);
    for (size_t i = 0; i < capacity_; ++i) new (slots + i) Slot{};
    if (!frames_.initialize(capacity_)) {
      for (size_t i = 0; i < capacity_; ++i) slots[i].~Slot();
      allocator_.release(memory, reserved_bytes());
      return false;
    }
    slots_ = slots;
    return true;
  }
  bool available() const {
    return slots_ && !stopping_;
  }
  size_t capacity() const {
    return capacity_;
  }
  size_t reserved_bytes() const {
    return capacity_ * sizeof(Slot);
  }
  uint64_t revision() const {
    return revision_;
  }
  size_t find(Token token) const {
    if (available())
      for (size_t i = 0; i < capacity_; ++i)
        if (Identity::same(slots_[i].binding, token)) return i;
    return capacity_;
  }
  bool bind(size_t slot, Token token, std::string_view payload) {
    if (!available() || slot >= capacity_ || !Identity::eligible(token) || !origin_.valid(token))
      return false;
    const auto duplicate = find(token);
    if (duplicate != capacity_ && duplicate != slot) return false;
    auto& s = slots_[slot];
    revoke(slot);
    s.binding = {};
    s.payload.fill(0);
    s.length = s.side = 0;
    if (payload.empty() || payload.size() > PAGE_QR_PAYLOAD_MAX || !next_) return false;
    s.binding = token;
    s.request = next_++;
    s.length = static_cast<uint16_t>(payload.size());
    std::memcpy(s.payload.data(), payload.data(), payload.size());
    return true;
  }
  bool size(size_t slot, Token token, uint16_t side, bool compact) {
    if (!current(slot, token)) return false;
    auto& s = slots_[slot];
    if (s.side == side && s.compact == compact) return side != 0;
    revoke(slot);
    s.side = side;
    s.compact = compact;
    if (!side || !next_) {
      s.state.failed = true;
      return false;
    }
    s.request = next_++;
    s.state.pending = true;
    return true;
  }
  void unbind(size_t slot, Token token) {
    if (!slots_ || slot >= capacity_ || !Identity::same(slots_[slot].binding, token)) return;
    revoke(slot);
    auto& s = slots_[slot];
    s.binding = {};
    s.payload.fill(0);
    s.length = s.side = 0;
  }
  void retire() {
    if (!slots_) return;
    for (size_t i = 0; i < capacity_; ++i) {
      revoke(i);
      slots_[i].binding = {};
      slots_[i].payload.fill(0);
      slots_[i].length = slots_[i].side = 0;
    }
  }
  PageQrSnapshot snapshot(size_t slot, Token token) const {
    return current(slot, token) ? slots_[slot].state : PageQrSnapshot{};
  }
  // Reentrant navigation may replace metadata during the fence/encoder/adapter.
  // Copy the payload before crossing those boundaries. Old output is never
  // published to a replacement binding; no borrowed configuration text survives.
  void loop() {
    if (!available() || busy_) return;
    Guard guard(busy_);
    for (size_t i = 0; i < capacity_; ++i) {
      auto& s = slots_[i];
      if (!s.reset) continue;
      const auto token = s.binding;
      const auto request = s.request;
      const auto side = s.side;
      const auto compact = s.compact;
      const auto length = s.length;
      std::array<char, PAGE_QR_PAYLOAD_MAX> payload{};
      if (length) std::memcpy(payload.data(), s.payload.data(), length);
      if (!frames_.wait_for_draw()) continue;
      if (!unchanged(i, token, request)) continue;
      s.reset = false;
      frames_.reset_after_draw(i);
      if (!unchanged(i, token, request)) continue;
      s.matrix.clear();
      if (!s.state.pending || !current(i, token)) continue;
      s.state.pending = false;
      const bool encoded = encoder_.encode({payload.data(), length}, side, compact, s.matrix);
      if (!unchanged(i, token, request) || !current(i, token)) continue;
      if (!encoded || !s.matrix.layout.valid() || s.matrix.layout.side != side) {
        s.state.failed = true;
        ++revision_;
        continue;
      }
      const bool assigned = frames_.assign_after_draw(i, s.matrix);
      if (!unchanged(i, token, request) || !current(i, token)) {
        frames_.hide(i);
        s.reset = true;
        continue;
      }
      if (!assigned) frames_.hide(i);
      s.state = {false, assigned, !assigned, assigned ? request : 0};
      // Even a rejected adapter may have touched a descriptor. Retain the matrix
      // until a later revocation/reset rather than clearing it in this call.
      ++revision_;
    }
  }
  bool shutdown() {
    if (busy_) return false;
    if (!slots_) return true;
    Guard guard(busy_);
    stopping_ = true;
    retire();
    if (!frames_.wait_for_draw()) return false;
    for (size_t i = 0; i < capacity_; ++i) frames_.reset_after_draw(i);
    if (!frames_.shutdown_after_draw()) return false;
    for (size_t i = 0; i < capacity_; ++i) slots_[i].~Slot();
    allocator_.release(slots_, reserved_bytes());
    slots_ = nullptr;
    stopping_ = false;
    return true;
  }

 private:
  struct Slot {
    PageQrMatrix matrix;
    std::array<char, PAGE_QR_PAYLOAD_MAX> payload{};
    Token binding{};
    PageQrSnapshot state;
    uint64_t request{0};
    uint16_t length{0}, side{0};
    bool compact{false}, reset{false};
  };
  struct Guard {
    bool& busy;
    explicit Guard(bool& value) : busy(value) {
      busy = true;
    }
    ~Guard() {
      busy = false;
    }
  };
  bool current(size_t slot, Token token) const {
    return available() && slot < capacity_ && Identity::same(slots_[slot].binding, token) &&
           origin_.valid(token);
  }
  bool unchanged(size_t slot, Token token, uint64_t request) const {
    const auto& s = slots_[slot];
    return s.request == request &&
           (Identity::same(s.binding, token) || (!Identity::active(s.binding) && !Identity::active(token)));
  }
  void revoke(size_t slot) {
    auto& s = slots_[slot];
    s.request = 0;
    s.state = {};
    s.reset = true;
    frames_.hide(slot);
    ++revision_;
  }
  PageArenaAllocator& allocator_;
  PageQrEncoder& encoder_;
  PageQrFrames& frames_;
  Origin& origin_;
  size_t capacity_;
  Slot* slots_{nullptr};
  uint64_t next_{1}, revision_{0};
  bool busy_{false}, stopping_{false};
};
}  // namespace espcontrol::pages
