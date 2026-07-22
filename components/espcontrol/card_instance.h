#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "card_reconciler.h"

namespace espcontrol::cards {

enum class CardResourceKind : uint8_t {
  WIDGET,
  SUBSCRIPTION,
  TIMER,
  MEDIA_CONTROL,
  MEDIA_PLAYBACK_STATE,
  MEDIA_ARTWORK,
  CUSTOM,
};

using CardResourceRelease = void (*)(void *resource);

struct CardResourceHandle {
  void *resource = nullptr;
  CardResourceRelease release = nullptr;
  CardResourceKind kind = CardResourceKind::CUSTOM;
  uint8_t release_on_domains = static_cast<uint8_t>(
      CHANGE_IDENTITY | CHANGE_RESOURCES | CHANGE_BINDINGS);
};

template <std::size_t MaxResources>
class FixedCardInstance;

template <std::size_t MaxResources>
struct CardCallbackToken {
  const FixedCardInstance<MaxResources> *owner = nullptr;
  uint32_t generation = 0;

  bool valid() const;
};

// Owns every runtime resource belonging to one rendered card. It intentionally
// stores plain release functions rather than std::function so ownership stays
// fixed-capacity and visible in firmware memory.
template <std::size_t MaxResources>
class FixedCardInstance {
 public:
  FixedCardInstance() = default;
  FixedCardInstance(const FixedCardInstance &) = delete;
  FixedCardInstance &operator=(const FixedCardInstance &) = delete;

  bool active() const { return active_; }
  const CardAddress &address() const { return address_; }
  uint32_t generation() const { return generation_; }
  std::size_t resource_count() const { return resource_count_; }
  static constexpr std::size_t resource_capacity() { return MaxResources; }

  bool activate(const CardAddress &address) {
    if (active_) return false;
    advance_generation();
    address_ = address;
    active_ = true;
    return true;
  }

  bool own(const CardResourceHandle &handle) {
    if (!active_ || handle.resource == nullptr || handle.release == nullptr ||
        resource_count_ >= MaxResources) {
      return false;
    }
    resources_[resource_count_++] = handle;
    return true;
  }

  CardCallbackToken<MaxResources> callback_token() const {
    return {this, generation_};
  }

  bool accepts(const CardCallbackToken<MaxResources> &token) const {
    return active_ && token.owner == this && token.generation == generation_;
  }

  // Invalidates callbacks before releasing affected resources. Appearance and
  // layout-only updates keep existing callbacks and owned media state alive.
  std::size_t release_for_changes(uint8_t changed_domains) {
    if (!active_ || changed_domains == CHANGE_NONE) return 0;
    if ((changed_domains &
         (CHANGE_IDENTITY | CHANGE_RESOURCES | CHANGE_BINDINGS)) != 0) {
      advance_generation();
    }

    std::size_t released = 0;
    for (std::size_t reverse = resource_count_; reverse > 0; --reverse) {
      CardResourceHandle &handle = resources_[reverse - 1];
      if ((handle.release_on_domains & changed_domains) == 0) continue;
      handle.release(handle.resource);
      handle.resource = nullptr;
      ++released;
    }
    compact_resources();
    return released;
  }

  std::size_t reset() {
    if (!active_) return 0;
    advance_generation();
    const std::size_t released = resource_count_;
    for (std::size_t reverse = resource_count_; reverse > 0; --reverse) {
      CardResourceHandle &handle = resources_[reverse - 1];
      handle.release(handle.resource);
      handle.resource = nullptr;
    }
    resource_count_ = 0;
    active_ = false;
    address_ = {};
    return released;
  }

 private:
  void advance_generation() {
    ++generation_;
    if (generation_ == 0) ++generation_;
  }

  void compact_resources() {
    std::size_t output = 0;
    for (std::size_t input = 0; input < resource_count_; ++input) {
      if (resources_[input].resource == nullptr) continue;
      if (output != input) resources_[output] = resources_[input];
      ++output;
    }
    for (std::size_t index = output; index < resource_count_; ++index) {
      resources_[index] = {};
    }
    resource_count_ = output;
  }

  CardAddress address_{};
  std::array<CardResourceHandle, MaxResources> resources_{};
  std::size_t resource_count_ = 0;
  uint32_t generation_ = 0;
  bool active_ = false;
};

template <std::size_t MaxResources>
bool CardCallbackToken<MaxResources>::valid() const {
  return owner != nullptr && owner->accepts(*this);
}

template <std::size_t MaxCards, std::size_t MaxResourcesPerCard>
class FixedCardInstancePool {
 public:
  using Instance = FixedCardInstance<MaxResourcesPerCard>;

  Instance *find(const CardAddress &address) {
    for (auto &instance : instances_) {
      if (instance.active() && instance.address() == address) return &instance;
    }
    return nullptr;
  }

  const Instance *find(const CardAddress &address) const {
    for (const auto &instance : instances_) {
      if (instance.active() && instance.address() == address) return &instance;
    }
    return nullptr;
  }

  Instance *acquire(const CardAddress &address, bool *created = nullptr) {
    if (Instance *existing = find(address)) {
      if (created != nullptr) *created = false;
      return existing;
    }
    for (auto &instance : instances_) {
      if (instance.active()) continue;
      if (!instance.activate(address)) return nullptr;
      ++size_;
      if (created != nullptr) *created = true;
      return &instance;
    }
    return nullptr;
  }

  bool release(const CardAddress &address) {
    Instance *instance = find(address);
    if (instance == nullptr) return false;
    instance->reset();
    --size_;
    return true;
  }

  std::size_t size() const { return size_; }
  static constexpr std::size_t capacity() { return MaxCards; }

 private:
  std::array<Instance, MaxCards> instances_{};
  std::size_t size_ = 0;
};

}  // namespace espcontrol::cards
