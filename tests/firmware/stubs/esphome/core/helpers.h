#pragma once

#include <cstddef>
#include <new>

namespace fake_esphome_allocator {

inline bool external_available = true;
inline bool internal_available = true;
inline const void *last_external_pointer = nullptr;

}  // namespace fake_esphome_allocator

namespace esphome {

template<typename T>
class RAMAllocator {
 public:
  T *allocate(size_t count) {
    if (count == 0 || (!fake_esphome_allocator::external_available &&
                       !fake_esphome_allocator::internal_available)) {
      return nullptr;
    }
    T *pointer = static_cast<T *>(
        ::operator new(count * sizeof(T), std::nothrow));
    if (pointer != nullptr && fake_esphome_allocator::external_available) {
      fake_esphome_allocator::last_external_pointer = pointer;
    }
    return pointer;
  }

  void deallocate(T *pointer, size_t) { ::operator delete(pointer); }
};

}  // namespace esphome
