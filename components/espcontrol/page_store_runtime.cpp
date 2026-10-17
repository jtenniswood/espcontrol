#include "page_store_runtime.h"

#include <algorithm>
#include <cstring>

#include "panel_config_document.h"

namespace espcontrol::pages {

PageStoreRuntime::~PageStoreRuntime() {
  discard_prepared();
  if (active_arena_ != nullptr) allocator_.release(active_arena_, arena_capacity_);
}

bool PageStoreRuntime::prepare(uint16_t version, const uint8_t *document, size_t size) {
  discard_prepared();
  last_stage_status_ = PageStoreStatus::INVALID_ARGUMENT;
  if (version != configuration::PANEL_CONFIG_DOCUMENT_VERSION ||
      document == nullptr || arena_capacity_ == 0 || arena_capacity_ > UINT32_MAX) {
    ++memory_.preparation_failures;
    return false;
  }
  staged_arena_ = allocator_.allocate(arena_capacity_);
  if (staged_arena_ == nullptr) {
    last_stage_status_ = PageStoreStatus::BUFFER_TOO_SMALL;
    ++memory_.preparation_failures;
    return false;
  }
  memory_.staged_reserved_bytes = arena_capacity_;
  memory_.peak_reserved_bytes = std::max(memory_.peak_reserved_bytes,
      memory_.active_reserved_bytes + memory_.staged_reserved_bytes);
  // Staged state is private until the service assigns the committed generation.
  last_stage_status_ = PageStore::stage_v1(document, size, limits_, UINT32_MAX,
      staged_arena_, arena_capacity_, &staged_);
  if (last_stage_status_ == PageStoreStatus::OK) return true;
  ++memory_.preparation_failures;
  discard_prepared();
  return false;
}

void PageStoreRuntime::discard_prepared() {
  staged_ = {};
  if (staged_arena_ != nullptr) allocator_.release(staged_arena_, arena_capacity_);
  staged_arena_ = nullptr;
  memory_.staged_reserved_bytes = 0;
}

bool PageStoreRuntime::activate_prepared(uint32_t generation, uint16_t version,
    const uint8_t *document, size_t size) {
  if (version != configuration::PANEL_CONFIG_DOCUMENT_VERSION ||
      staged_arena_ == nullptr || document == nullptr ||
      size != staged_.source_size() ||
      std::memcmp(document, staged_.source_document(), size) != 0) return false;
  void *retired = active_arena_;
  active_arena_ = staged_arena_;
  staged_arena_ = nullptr;
  active_ = staged_;
  active_.generation_ = generation;
  staged_ = {};
  memory_.active_reserved_bytes = arena_capacity_;
  memory_.staged_reserved_bytes = 0;
  // The renderer cutover adds an explicit unbind barrier before retiring the
  // old generation. No widgets or asynchronous consumers use this owner yet.
  if (retired != nullptr) allocator_.release(retired, arena_capacity_);
  return true;
}

}  // namespace espcontrol::pages
