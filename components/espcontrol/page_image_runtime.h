#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "page_renderer_runtime.h"

namespace espcontrol::pages {
struct PageImageOptions {
  uint16_t width{0}, height{0};
  uint32_t background{0};
  bool cover{true};
};
struct PageImageSnapshot {
  bool ready{false}, pending{false}, failed{false}, cached{false};
};

class PageImageRequests {
 public:
  using Receive = void (*)(void*, size_t, uint64_t, bool, bool);
  virtual ~PageImageRequests() = default;
  // Main-loop maintenance only. start copies URL bytes before returning; it
  // may allocate transfer storage and may deliver/reenter synchronously.
  virtual bool start(size_t slot, uint64_t request, std::string_view url, PageImageOptions, void* owner,
                     Receive) = 0;
  // Retire the token before calling cancel: cancellation may deliver a result.
  virtual void cancel(size_t slot) = 0;
  // The caller has detached the old LVGL source. Defer surface cleanup until
  // maintenance, outside allocation-free navigation/configuration retirement.
  virtual void release_surface(size_t slot) = 0;
};
class PageImageOrigin {
 public:
  virtual ~PageImageOrigin() = default;
  virtual bool valid(PageBindingToken) const = 0;
  virtual bool connected() const = 0;
  virtual uint32_t connection_generation() const = 0;
};

// One fixed lease per prepared image slot, not per configured card. Binding
// keeps only value tokens; no URL, configuration, entity or widget is borrowed.
// The view detaches its image source BEFORE unbind/suspend/config retirement.
template <size_t Slots>
class PageImageRuntime {
 public:
  static_assert(Slots > 0 && Slots <= UINT16_MAX);
  PageImageRuntime(PageImageRequests& requests, PageImageOrigin& origin, uint64_t first_request = 1)
      : requests_(requests), origin_(origin), next_request_(first_request) {}
  ~PageImageRuntime() {
    stopping_ = true;
    retire();
    cleanup();
  }
  PageImageRuntime(const PageImageRuntime&) = delete;
  PageImageRuntime& operator=(const PageImageRuntime&) = delete;

  bool bind(size_t slot, PageBindingToken binding) {
    if (stopping_ || retiring_ || slot >= Slots || !origin_.valid(binding) || binding.back) return false;
    unbind(slot);
    // Cancellation may have bound a newer card. Preserve that replacement.
    if (slots_[slot].binding.binding != 0) return false;
    slots_[slot].binding = binding;
    slots_[slot].connection = origin_.connection_generation();
    return true;
  }
  void unbind(size_t slot) {
    if (slot >= Slots) return;
    auto& s = slots_[slot];
    const bool owned = s.binding.binding != 0 || s.request != 0;
    ++s.change;
    s.binding = {};
    s.request = 0;
    s.state = {};
    if (!owned) return;
    s.release = true;
    ++revision_;
    requests_.cancel(slot);
  }
  void retire() {
    if (retiring_) return;
    retiring_ = true;
    for (size_t i = 0; i < Slots; ++i) unbind(i);
    retiring_ = false;
  }
  // Called only from loop maintenance after all page owners have activated.
  // A fresh URL comes from the current graph; the transport owns any copy.
  bool request(size_t slot, PageBindingToken binding, std::string_view url, PageImageOptions options) {
    if (stopping_ || retiring_ || !current(slot, binding) || url.empty() || options.width == 0 ||
        options.height == 0 || next_request_ == 0)
      return false;
    auto& s = slots_[slot];
    const uint64_t change = ++s.change;
    const uint64_t previous = s.request;
    s.request = 0;
    if (previous != 0) requests_.cancel(slot);
    // A cancel callback may have rebound the slot or retired configuration.
    if (s.change != change || !current(slot, binding)) return false;
    release(slot);
    if (s.change != change || !current(slot, binding)) return false;
    const uint64_t request = next_request_++;
    s.request = request;
    s.state = {false, true, false, false};
    ++revision_;
    const bool accepted = requests_.start(slot, request, url, options, this, receive);
    if (s.request != request || !current(slot, binding)) return false;
    // Synchronous success/error is authoritative even if a transport returned
    // false after delivering. Never overwrite a completed or replacement lease.
    if (!accepted && s.state.pending) {
      s.state = {false, false, true, false};
      ++revision_;
    }
    return accepted || !s.state.failed;
  }
  PageImageSnapshot snapshot(size_t slot, PageBindingToken binding) const {
    return current(slot, binding) ? slots_[slot].state : PageImageSnapshot{};
  }
  void loop() {
    for (size_t i = 0; i < Slots; ++i) {
      auto& s = slots_[i];
      if (s.binding.binding != 0 && !current(i, s.binding)) {
        const bool had_request = s.request != 0;
        s.request = 0;
        s.state = {};
        s.connection = origin_.connection_generation();
        if (had_request) {
          ++s.change;
          ++revision_;
          requests_.cancel(i);
        }
        // Keep the value binding for reconnect. The visible source may still
        // be attached; only an explicit unbind authorizes surface release.
      }
      release(i);
    }
  }
  uint64_t revision() const {
    return revision_;
  }

 private:
  struct Slot {
    PageBindingToken binding;
    uint64_t request{0}, change{0};
    uint32_t connection{0};
    PageImageSnapshot state;
    bool release{false};
  };
  static bool same(PageBindingToken a, PageBindingToken b) {
    return a.binding != 0 && a.configuration == b.configuration && a.binding == b.binding &&
           a.page == b.page && a.card == b.card && a.shell == b.shell && a.back == b.back;
  }
  bool current(size_t slot, PageBindingToken binding) const {
    return slot < Slots && same(slots_[slot].binding, binding) && origin_.connected() &&
           slots_[slot].connection == origin_.connection_generation() && origin_.valid(binding);
  }
  void release(size_t slot) {
    if (!slots_[slot].release) return;
    slots_[slot].release = false;
    requests_.release_surface(slot);
  }
  void cleanup() {
    for (size_t i = 0; i < Slots; ++i) release(i);
  }
  static void receive(void* owner, size_t slot, uint64_t request, bool success, bool cached) {
    auto& self = *static_cast<PageImageRuntime*>(owner);
    if (slot >= Slots) return;
    auto& s = self.slots_[slot];
    if (request == 0 || request != s.request || !s.state.pending || !self.current(slot, s.binding)) return;
    s.state = {success, false, !success, success && cached};
    ++self.revision_;
  }
  PageImageRequests& requests_;
  PageImageOrigin& origin_;
  std::array<Slot, Slots> slots_{};
  uint64_t next_request_{1}, revision_{0};
  bool stopping_{false}, retiring_{false};
};
}  // namespace espcontrol::pages
