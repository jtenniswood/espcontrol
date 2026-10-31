#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "device_ui_limits.h"

namespace espcontrol::pages {

using PageId = uint16_t;
using CardId = uint16_t;
using StringId = uint32_t;
constexpr PageId NO_PAGE = UINT16_MAX;
constexpr CardId NO_CARD = UINT16_MAX;
constexpr StringId EMPTY_STRING = 0;

enum class CardField : uint8_t {
  ENTITY, LABEL, ICON, ICON_ON, SENSOR, UNIT, TYPE, PRECISION, OPTIONS, COUNT,
};

struct CardDefinition {
  std::array<StringId, static_cast<size_t>(CardField::COUNT)> fields{};
  uint16_t slot{0};
  PageId target{NO_PAGE};
};

struct PageDefinition {
  PageId parent{NO_PAGE};
  CardId owner{NO_CARD};
  CardId first_card{0};
  uint16_t card_count{0};
  StringId order{EMPTY_STRING};
  // Legacy documents can retain a subpage behind a non-folder card. Keep its
  // data without making it a navigable target.
  bool navigable{false};
  // Retained root definitions may extend beyond this device's capacity.
  // Runtime owners use this prefix; serialization keeps card_count unchanged.
  uint16_t runtime_card_count{0};
};

struct ConfigurationHandle {
  uint32_t generation{0};
  uint16_t index{UINT16_MAX};
};

enum class PageStoreStatus : uint8_t {
  OK, INVALID_ARGUMENT, INVALID_DOCUMENT, PROFILE_MISMATCH, BUFFER_TOO_SMALL,
  RESOURCE_UNAVAILABLE, CAPACITY_EXCEEDED,
};

struct PageStoreMemory {
  size_t arena_bytes{0};
  size_t record_bytes{0};
  size_t string_bytes{0};
  size_t source_bytes{0};
  size_t string_count{0};
};

// Immutable view over one caller-owned arena. It contains no widgets, mutable
// HA state or storage-owned pointers. Keep its arena alive until the generation
// is retired. Staging must use a different arena from the active view.
class PageStore {
 public:
  static PageStoreStatus stage_v1(const uint8_t *document, size_t document_size,
      const DeviceUiLimits &limits, uint32_t generation,
      void *arena, size_t arena_capacity, PageStore *output);
  // V2 is read directly, without building an intermediate v1 document. The
  // caller owns disjoint, aligned scratch, released after staging. The current
  // runtime still admits only root -> child navigation, including inactive data.
  static PageStoreStatus v2_workspace_bytes(const uint8_t *document,
      size_t document_size, size_t *bytes);
  static PageStoreStatus stage_v2(const uint8_t *document, size_t document_size,
      const DeviceUiLimits &limits, uint32_t generation,
      void *arena, size_t arena_capacity, void *workspace,
      size_t workspace_capacity, PageStore *output);

  uint32_t generation() const { return generation_; }
  size_t page_count() const { return page_count_; }
  size_t card_count() const { return card_count_; }
  const PageDefinition *page(PageId id) const {
    return id < page_count_ ? pages_ + id : nullptr;
  }
  const CardDefinition *card(CardId id) const {
    return id < card_count_ ? cards_ + id : nullptr;
  }
  const CardDefinition *card(ConfigurationHandle handle) const {
    return handle.generation == generation_ ? card(handle.index) : nullptr;
  }
  const CardDefinition *runtime_card(CardId id) const {
    if (id >= card_count_) return nullptr;
    for (PageId page = 0; page < page_count_; ++page) {
      const auto &definition = pages_[page];
      if (definition.navigable && id >= definition.first_card &&
          id - definition.first_card < definition.runtime_card_count) return cards_ + id;
    }
    return nullptr;
  }
  const CardDefinition *runtime_card(ConfigurationHandle handle) const {
    return handle.generation == generation_ ? runtime_card(handle.index) : nullptr;
  }
  const PageDefinition *page(ConfigurationHandle handle) const {
    return handle.generation == generation_ ? page(handle.index) : nullptr;
  }
  std::string_view string(StringId id) const;
  std::string_view field(const CardDefinition &card, CardField field) const {
    const size_t index = static_cast<size_t>(field);
    return index < card.fields.size() ? string(card.fields[index]) : std::string_view{};
  }
  // Exact source bytes preserve settings, unsupported/inactive configuration
  // and import encodings until persistence and compatibility readers can borrow
  // the durable source directly. They also bind activation to the prepared input.
  const uint8_t *source_document() const { return source_; }
  uint16_t source_version() const { return source_version_; }
  size_t source_size() const { return memory_.source_bytes; }
  const PageStoreMemory &memory() const { return memory_; }

 private:
  friend class PageStoreBuilder;
  friend class PageStoreRuntime;
  const uint8_t *arena_{nullptr};
  size_t arena_capacity_{0};
  const PageDefinition *pages_{nullptr};
  const CardDefinition *cards_{nullptr};
  const uint8_t *source_{nullptr};
  uint32_t generation_{0};
  uint16_t page_count_{0};
  uint16_t card_count_{0};
  uint16_t source_version_{1};
  size_t strings_begin_{0};
  PageStoreMemory memory_{};
};

}  // namespace espcontrol::pages
