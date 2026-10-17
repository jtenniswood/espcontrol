#include "page_store.h"

#include <cstring>
#include <limits>
#include <new>
#include <string>

#include "button_grid_subpage_parser.h"
#include "panel_config_document.h"

namespace espcontrol::pages {
namespace {

using namespace espcontrol::configuration;

struct SourceValue {
  const uint8_t *data{nullptr};
  size_t size{0};
  std::string text() const {
    return data == nullptr ? std::string{} :
      std::string(reinterpret_cast<const char *>(data), size);
  }
};

bool overlaps(const void *a, size_t a_size, const void *b, size_t b_size) {
  if (a_size == 0 || b_size == 0) return false;
  const uintptr_t left = reinterpret_cast<uintptr_t>(a);
  const uintptr_t right = reinterpret_cast<uintptr_t>(b);
  return left <= right ? right - left < a_size : left - right < b_size;
}

std::string subpage_order(const std::string &value) {
  const size_t start = !value.empty() && value.front() == '~' ? 1 : 0;
  return value.substr(start, value.find('|', start) - start);
}

}  // namespace

class PageStoreBuilder {
 public:
  PageStoreBuilder(uint8_t *arena, size_t capacity, uint32_t generation)
      : arena_(arena), capacity_(capacity) {
    store_.arena_ = arena;
    store_.arena_capacity_ = capacity;
    store_.generation_ = generation;
  }

  PageStoreStatus build(const uint8_t *document, size_t size,
                       const DeviceUiLimits &limits, PageStore *output) {
    PanelConfigReader reader(document, size);
    if (reader.validate() != PanelConfigStatus::OK)
      return PageStoreStatus::INVALID_DOCUMENT;

    std::array<SourceValue, PANEL_CONFIG_MAX_SLOT_COUNT> buttons{};
    std::array<SourceValue, PANEL_CONFIG_MAX_SLOT_COUNT> subpages{};
    SourceValue order;
    reader.begin();
    PanelConfigRecord record;
    while (reader.next(&record) == PanelConfigStatus::OK) {
      if (record.type == PanelConfigRecordType::DEVICE_PROFILE) {
        if (std::string_view(reinterpret_cast<const char *>(record.value),
                             record.value_size) != limits.device_profile)
          return PageStoreStatus::PROFILE_MISMATCH;
      } else if (record.type == PanelConfigRecordType::BUTTON) {
        buttons[record.slot - 1] = {record.value, record.value_size};
      } else if (record.type == PanelConfigRecordType::SUBPAGE) {
        subpages[record.slot - 1] = {record.value, record.value_size};
      } else if (record.type == PanelConfigRecordType::SETTING &&
                 std::string_view(reinterpret_cast<const char *>(record.key),
                                  record.key_size) == "button_order") {
        order = {record.value, record.value_size};
      }
    }

    // Count stored definitions, including source slots hidden by the profile
    // or orientation. Never size the runtime for a larger device by default.
    size_t root_count = limits.slot_count;
    for (size_t i = root_count; i < buttons.size(); ++i) {
      if (buttons[i].data != nullptr || subpages[i].data != nullptr)
        root_count = i + 1;
    }
    size_t page_count = 1;
    size_t card_count = root_count;
    for (size_t i = 0; i < root_count; ++i) {
      const bool folder = parse_cfg(buttons[i].text()).type == "subpage";
      if (!folder && subpages[i].data == nullptr) continue;
      ++page_count;
      card_count += parse_subpage_config(subpages[i].text()).size();
    }
    if (card_count >= NO_CARD) return PageStoreStatus::INVALID_DOCUMENT;

    auto *pages = take<PageDefinition>(page_count);
    auto *cards = take<CardDefinition>(card_count);
    auto *source = take<uint8_t>(size);
    if (pages == nullptr || cards == nullptr || source == nullptr)
      return PageStoreStatus::BUFFER_TOO_SMALL;
    std::memcpy(source, document, size);
    store_.pages_ = pages;
    store_.cards_ = cards;
    store_.source_ = source;
    store_.page_count_ = page_count;
    store_.card_count_ = card_count;
    store_.strings_begin_ = offset_;
    store_.memory_.record_bytes = page_count * sizeof(PageDefinition) +
                                  card_count * sizeof(CardDefinition);
    store_.memory_.source_bytes = size;
    pages[0].card_count = root_count;
    pages[0].navigable = true;
    pages[0].order = intern(order.text());

    PageId next_page = 1;
    CardId next_card = root_count;
    for (size_t i = 0; i < root_count; ++i) {
      const ParsedCfg parsed = parse_cfg(buttons[i].text());
      set_card(cards[i], parsed, i + 1);
      if (parsed.type != "subpage" && subpages[i].data == nullptr) continue;
      auto &page = pages[next_page];
      page.parent = 0;
      page.owner = i;
      page.first_card = next_card;
      page.navigable = parsed.type == "subpage" && i < limits.slot_count;
      if (page.navigable) cards[i].target = next_page;
      const std::string raw = subpages[i].text();
      page.order = intern(subpage_order(raw));
      const auto children = parse_subpage_config(raw);
      page.card_count = children.size();
      for (size_t child = 0; child < children.size(); ++child) {
        // This is precisely the existing subpage-to-grid interpretation,
        // including generated card normalization and compact field decoding.
        set_card(cards[next_card++], parsed_cfg_from_subpage_btn(children[child]), child + 1);
      }
      ++next_page;
    }
    if (failed_) return PageStoreStatus::BUFFER_TOO_SMALL;
    store_.memory_.arena_bytes = offset_;
    store_.memory_.string_bytes = offset_ - store_.strings_begin_;
    *output = store_;
    return PageStoreStatus::OK;
  }

 private:
  template<typename T> T *take(size_t count) {
    const size_t padding = (alignof(T) - offset_ % alignof(T)) % alignof(T);
    if (padding > capacity_ - offset_) return nullptr;
    offset_ += padding;
    if (count > (capacity_ - offset_) / sizeof(T)) return nullptr;
    T *result = reinterpret_cast<T *>(arena_ + offset_);
    for (size_t i = 0; i < count; ++i) new (result + i) T{};
    offset_ += sizeof(T) * count;
    return result;
  }

  StringId intern(const std::string &value) {
    if (value.empty() || failed_) return EMPTY_STRING;
    for (size_t cursor = store_.strings_begin_; cursor < offset_;) {
      uint32_t length;
      std::memcpy(&length, arena_ + cursor, sizeof(length));
      cursor += sizeof(length);
      if (length == value.size() &&
          std::memcmp(arena_ + cursor, value.data(), length) == 0)
        return cursor;
      cursor += length;
    }
    if (capacity_ - offset_ < sizeof(uint32_t) ||
        value.size() > capacity_ - offset_ - sizeof(uint32_t)) {
      failed_ = true;
      return EMPTY_STRING;
    }
    const auto length = static_cast<uint32_t>(value.size());
    std::memcpy(arena_ + offset_, &length, sizeof(length));
    offset_ += sizeof(length);
    const StringId id = offset_;
    std::memcpy(arena_ + offset_, value.data(), value.size());
    offset_ += value.size();
    ++store_.memory_.string_count;
    return id;
  }

  void set_card(CardDefinition &card, const ParsedCfg &parsed, uint16_t slot) {
    card.slot = slot;
    card.fields = {intern(parsed.entity), intern(parsed.label), intern(parsed.icon),
      intern(parsed.icon_on), intern(parsed.sensor), intern(parsed.unit),
      intern(parsed.type), intern(parsed.precision), intern(parsed.options)};
  }

  uint8_t *arena_;
  size_t capacity_;
  size_t offset_{0};
  bool failed_{false};
  PageStore store_;
};

PageStoreStatus PageStore::stage_v1(const uint8_t *document, size_t size,
    const DeviceUiLimits &limits, uint32_t generation, void *arena,
    size_t capacity, PageStore *output) {
  if (document == nullptr || arena == nullptr || output == nullptr ||
      limits.device_profile == nullptr || limits.slot_count == 0 ||
      limits.slot_count > PANEL_CONFIG_MAX_SLOT_COUNT || generation == 0 ||
      capacity > UINT32_MAX ||
      reinterpret_cast<uintptr_t>(arena) % alignof(PageDefinition) != 0 ||
      reinterpret_cast<uintptr_t>(arena) % alignof(CardDefinition) != 0 ||
      overlaps(document, size, arena, capacity) ||
      overlaps(output, sizeof(*output), arena, capacity) ||
      overlaps(output->arena_, output->arena_capacity_, arena, capacity))
    return PageStoreStatus::INVALID_ARGUMENT;
  return PageStoreBuilder(static_cast<uint8_t *>(arena), capacity, generation)
      .build(document, size, limits, output);
}

std::string_view PageStore::string(StringId id) const {
  if (id == EMPTY_STRING || id < strings_begin_ + sizeof(uint32_t) ||
      id > memory_.arena_bytes) return {};
  uint32_t length;
  std::memcpy(&length, arena_ + id - sizeof(length), sizeof(length));
  if (length > memory_.arena_bytes - id) return {};
  return {reinterpret_cast<const char *>(arena_ + id), length};
}

}  // namespace espcontrol::pages
