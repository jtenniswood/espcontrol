#include "page_store_runtime.h"

#include <algorithm>
#include <cstring>

#include "panel_config_document.h"
#include "panel_config_document_v2.h"

namespace espcontrol::pages {

PageStoreRuntime::~PageStoreRuntime() {
  discard_prepared();
  if (resources_ != nullptr) resources_->retire_active();
  if (active_arena_ != nullptr)
    allocator_.release(active_arena_, memory_.active_reserved_bytes);
}

bool PageStoreRuntime::prepared_within_limits() const {
  const auto *root = staged_.page(0);
  if (!root || root->runtime_card_count > limits_.slot_count) return false;
  size_t child_pages = 0;
  for (PageId id = 1; id < staged_.page_count(); ++id) {
    const auto *page = staged_.page(id);
    if (!page->navigable) continue;
    if (++child_pages > limits_.max_first_level_subpages || page->parent != 0 ||
        limits_.max_navigation_depth < 1 || page->runtime_card_count > limits_.cards_per_subpage) return false;
    for (size_t index = 0; index < page->runtime_card_count; ++index) {
      const auto *card = staged_.card(page->first_card + index);
      const auto type = staged_.field(*card, CardField::TYPE);
      for (size_t support = 0; support < limits_.card_type_count; ++support)
        if (type == limits_.card_types[support].type && !limits_.card_types[support].allow_in_subpage) return false;
    }
  }
  return true;
}

bool PageStoreRuntime::prepare(uint16_t version, const uint8_t *document, size_t size) {
  return prepare_impl(version, document, size, false);
}
bool PageStoreRuntime::prepare_write(uint16_t version, const uint8_t *document, size_t size) {
  return prepare_impl(version, document, size, true);
}
bool PageStoreRuntime::prepare_impl(uint16_t version, const uint8_t *document, size_t size, bool write) {
  preparation_allocation_failed_ = false;
  discard_prepared();
  const auto allocation_failures = allocator_.allocation_failure_count();
  last_stage_status_ = PageStoreStatus::INVALID_ARGUMENT;
  if ((version != configuration::PANEL_CONFIG_DOCUMENT_VERSION &&
       version != configuration::PANEL_CONFIG_DOCUMENT_VERSION_2) ||
      document == nullptr || arena_capacity_ == 0 || arena_capacity_ > UINT32_MAX) {
    ++memory_.preparation_failures;
    return false;
  }
  struct Scratch {
    PageArenaAllocator &allocator;
    PageRuntimeMemory &memory;
    void *data{nullptr};
    void release() {
      if (data) allocator.release(data, memory.preparation_scratch_bytes);
      data = nullptr;
      memory.preparation_scratch_bytes = 0;
    }
    ~Scratch() { release(); }
  } scratch{allocator_, memory_};
  size_t scratch_bytes = 0;
  if (version == configuration::PANEL_CONFIG_DOCUMENT_VERSION_2) {
    last_stage_status_ = PageStore::v2_workspace_bytes(document, size, &scratch_bytes);
    if (last_stage_status_ != PageStoreStatus::OK || scratch_bytes > arena_capacity_) {
      if (last_stage_status_ == PageStoreStatus::OK)
        last_stage_status_ = write ? PageStoreStatus::CAPACITY_EXCEEDED : PageStoreStatus::BUFFER_TOO_SMALL;
      ++memory_.preparation_failures;
      return false;
    }
    scratch.data = allocator_.allocate(scratch_bytes);
    if (!scratch.data) {
      preparation_allocation_failed_ = true;
      last_stage_status_ = PageStoreStatus::BUFFER_TOO_SMALL;
      ++memory_.preparation_failures;
      return false;
    }
    memory_.preparation_scratch_bytes = scratch_bytes;
    memory_.peak_preparation_scratch_bytes = std::max(memory_.peak_preparation_scratch_bytes, scratch_bytes);
    memory_.peak_reserved_bytes = std::max(memory_.peak_reserved_bytes,
        memory_.active_reserved_bytes + scratch_bytes);
  }
  staged_arena_ = allocator_.allocate(arena_capacity_);
  if (staged_arena_ == nullptr) {
    preparation_allocation_failed_ = true;
    last_stage_status_ = PageStoreStatus::BUFFER_TOO_SMALL;
    ++memory_.preparation_failures;
    return false;
  }
  memory_.staged_reserved_bytes = arena_capacity_;
  memory_.peak_reserved_bytes = std::max(memory_.peak_reserved_bytes,
      memory_.active_reserved_bytes + memory_.staged_reserved_bytes + scratch_bytes);
  auto stage = [&](const uint8_t *source, void *arena, size_t capacity, PageStore *store) {
    return version == configuration::PANEL_CONFIG_DOCUMENT_VERSION
        ? PageStore::stage_v1(source, size, limits_, UINT32_MAX, arena, capacity, store)
        : PageStore::stage_v2(source, size, limits_, UINT32_MAX, arena, capacity,
                              scratch.data, scratch_bytes, store);
  };
  // Staged state is private until the service assigns the committed generation.
  last_stage_status_ = stage(document, staged_arena_, arena_capacity_, &staged_);
  if (write && last_stage_status_ == PageStoreStatus::BUFFER_TOO_SMALL)
    last_stage_status_ = PageStoreStatus::CAPACITY_EXCEEDED;
  if (last_stage_status_ == PageStoreStatus::OK && write && !prepared_within_limits())
    last_stage_status_ = PageStoreStatus::CAPACITY_EXCEEDED;
  if (last_stage_status_ == PageStoreStatus::OK) {
    // The configured capacity is an admission ceiling, not a retained reserve.
    // Build the final view before any resources can borrow its strings. Rebuild
    // from the validated private source so no arena pointers need relocating.
    const size_t used = staged_.memory().arena_bytes;
    if (used < arena_capacity_) {
      void *compact_arena = allocator_.allocate(used);
      if (compact_arena == nullptr) {
        preparation_allocation_failed_ = true;
        last_stage_status_ = PageStoreStatus::BUFFER_TOO_SMALL;
      } else {
        memory_.peak_reserved_bytes =
            std::max(memory_.peak_reserved_bytes,
                     memory_.active_reserved_bytes +
                         memory_.staged_reserved_bytes + used + scratch_bytes);
        PageStore compact;
        last_stage_status_ =
            stage(staged_.source_document(), compact_arena, used, &compact);
        if (last_stage_status_ == PageStoreStatus::OK) {
          allocator_.release(staged_arena_, memory_.staged_reserved_bytes);
          staged_arena_ = compact_arena;
          staged_ = compact;
          memory_.staged_reserved_bytes = used;
        } else {
          allocator_.release(compact_arena, used);
        }
      }
    }
  }
  // No runtime resource can retain the validation/grouping index. Release it
  // before resource preparation and the later allocation-free activation.
  scratch.release();
  if (last_stage_status_ == PageStoreStatus::OK) {
    if (resources_ == nullptr || (write ? resources_->prepare_write(staged_) : resources_->prepare(staged_))) return true;
    preparation_allocation_failed_ =
        allocator_.allocation_failure_count() != allocation_failures;
    last_stage_status_ = resources_->preparation_capacity_exceeded()
        ? PageStoreStatus::CAPACITY_EXCEEDED : PageStoreStatus::RESOURCE_UNAVAILABLE;
    if (last_stage_status_ == PageStoreStatus::CAPACITY_EXCEEDED) preparation_allocation_failed_ = false;
  }
  ++memory_.preparation_failures;
  discard_prepared();
  return false;
}

void PageStoreRuntime::discard_prepared() {
  if (resources_ != nullptr) resources_->discard_prepared();
  staged_ = {};
  if (staged_arena_ != nullptr)
    allocator_.release(staged_arena_, memory_.staged_reserved_bytes);
  staged_arena_ = nullptr;
  memory_.staged_reserved_bytes = 0;
}

bool PageStoreRuntime::activate_prepared(uint32_t generation, uint16_t version,
    const uint8_t *document, size_t size) {
  if (version != staged_.source_version() ||
      staged_arena_ == nullptr || document == nullptr ||
      size != staged_.source_size() ||
      std::memcmp(document, staged_.source_document(), size) != 0) return false;
  void *retired = active_arena_;
  const size_t retired_size = memory_.active_reserved_bytes;
  active_arena_ = staged_arena_;
  staged_arena_ = nullptr;
  active_ = staged_;
  active_.generation_ = generation;
  staged_ = {};
  memory_.active_reserved_bytes = memory_.staged_reserved_bytes;
  memory_.staged_reserved_bytes = 0;
  if (resources_ != nullptr) resources_->activate_prepared(active_);
  // Resource activation unbinds the retired generation before its arena goes.
  if (retired != nullptr)
    allocator_.release(retired, retired_size);
  return true;
}

}  // namespace espcontrol::pages
