#pragma once

#include <cstddef>
#include <cstdint>
#include <new>

namespace fake_esphome_allocator {

inline bool external_available = true;
inline bool internal_available = true;
inline const void *last_external_pointer = nullptr;
inline uint8_t last_allocation_flags = 0;

}  // namespace fake_esphome_allocator

namespace esphome {

template<typename T>
class RAMAllocator {
 public:
  enum Flags : uint8_t {
    NONE = 0,
    ALLOC_EXTERNAL = 1 << 0,
    ALLOC_INTERNAL = 1 << 1,
    ALLOW_FAILURE = 1 << 2,
    PREFER_INTERNAL = 1 << 3,
  };

  constexpr RAMAllocator() = default;
  constexpr explicit RAMAllocator(uint8_t flags) : flags_(flags) {}

  T *allocate(size_t count) {
    fake_esphome_allocator::last_allocation_flags = flags_;
    if (count == 0) return nullptr;
    const bool use_external = (flags_ & ALLOC_EXTERNAL) != 0 &&
                              fake_esphome_allocator::external_available;
    const bool use_internal = (flags_ & ALLOC_INTERNAL) != 0 &&
                              fake_esphome_allocator::internal_available;
    if (!use_external && !use_internal) return nullptr;
    T *pointer = static_cast<T *>(
        ::operator new(count * sizeof(T), std::nothrow));
    if (pointer != nullptr && use_external) {
      fake_esphome_allocator::last_external_pointer = pointer;
    }
    return pointer;
  }

  void deallocate(T *pointer, size_t) { ::operator delete(pointer); }

 private:
  // Deliberately internal-only so tests fail if production relies on the
  // stub's default instead of requesting external-first allocation explicitly.
  uint8_t flags_{ALLOC_INTERNAL};
};

}  // namespace esphome
