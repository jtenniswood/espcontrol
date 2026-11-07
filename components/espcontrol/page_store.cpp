#include "page_store.h"

#include <cstring>
#include <limits>
#include <new>
#include <string>

#include "button_grid_subpage_parser.h"
#include "panel_config_document.h"
#include "panel_config_projection.h"

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

// Temporary graph lookup/grouping lives in caller-owned scratch, never in the
// retained store or a table sized for the entire uint16 identity namespace.
struct V2StageState {
  std::array<uint16_t, PANEL_CONFIG_MAX_SLOT_COUNT> roots{};
  std::array<uint16_t, PANEL_CONFIG_MAX_SLOT_COUNT> pages{};
  std::array<uint16_t, PANEL_CONFIG_MAX_SLOT_COUNT> children{};
  std::array<char, PANEL_CONFIG_V2_MAX_STRING_BYTES> decoded{};
};
constexpr size_t V2_STAGE_FIXED = (sizeof(V2StageState) + 7) & ~size_t(7);
using V2Position = panel_config_projection_detail::Position;

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
    pages[0].runtime_card_count = limits.slot_count;
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
      page.runtime_card_count = page.navigable ? page.card_count : 0;
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

  PageStoreStatus build_v2(const uint8_t *document, size_t size,
      const DeviceUiLimits &limits, void *workspace, size_t workspace_size,
      PageStore *output) {
    using Kind = PanelConfigV2Kind;
    using Row = PanelConfigV2Record;
    using namespace panel_config_projection_detail;
    auto *state = new (workspace) V2StageState{};
    const size_t index_size = (workspace_size - V2_STAGE_FIXED) / 2;
    auto *index = static_cast<uint8_t *>(workspace) + V2_STAGE_FIXED;
    PanelConfigV2Reader reader(document, size);
    if (reader.validate(index, index_size) != PanelConfigStatus::OK)
      return PageStoreStatus::INVALID_DOCUMENT;
    auto *positions = reinterpret_cast<V2Position *>(index + index_size);
    size_t position_count = 0, root_count = limits.slot_count, page_count = 1;
    Row row, root;
    reader.record(Kind::PAGE, 0, &root);
    for (size_t i = 0; i < reader.record_count(); ++i) {
      reader.record(i, &row);
      if (row.kind == Kind::DEVICE_PROFILE && row.text != limits.device_profile)
        return PageStoreStatus::PROFILE_MISMATCH;
      if (row.kind == Kind::CARD) {
        new (positions + position_count++) V2Position{row.first, row.second, row.id, 0};
        if (!row.first) {
          if (row.second > state->roots.size()) return PageStoreStatus::INVALID_DOCUMENT;
          state->roots[row.second - 1] = row.id;
          root_count = std::max(root_count, size_t(row.second));
        }
      } else if (row.kind == Kind::PAGE && row.id) {
        // The wire codec can describe future nested pages. This runtime cannot
        // activate them until the separate depth-two feature is qualified.
        if (row.first != 0) return PageStoreStatus::INVALID_DOCUMENT;
        Row owner;
        reader.record(Kind::CARD, row.second, &owner);
        if (owner.first != 0 || owner.second > state->pages.size())
          return PageStoreStatus::INVALID_DOCUMENT;
        state->pages[owner.second - 1] = row.id;
        ++page_count;
      } else if (row.kind == Kind::SETTING && reader.string(row.id) == "button_order" &&
                 reader.string(row.first) != reader.string(root.third)) {
        return PageStoreStatus::INVALID_DOCUMENT;
      }
    }
    std::sort(positions, positions + position_count, [](const V2Position &a, const V2Position &b) {
      return a.page != b.page ? a.page < b.page : a.slot < b.slot;
    });
    auto range = [&](uint16_t page) {
      const auto *begin = std::lower_bound(positions, positions + position_count, page,
          [](const V2Position &position, uint16_t id) { return position.page < id; });
      const auto *end = begin;
      while (end != positions + position_count && end->page == page) ++end;
      return std::make_pair(begin, end);
    };
    size_t card_count = root_count;
    for (size_t slot = 0; slot < root_count; ++slot) {
      if (!state->pages[slot]) continue;
      const auto cards = range(state->pages[slot]);
      size_t child_slot = 1;
      for (auto *position = cards.first; position != cards.second; ++position, ++child_slot)
        if (position->slot != child_slot) return PageStoreStatus::INVALID_DOCUMENT;
      state->children[slot] = child_slot - 1;
      card_count += child_slot - 1;
    }
    if (card_count >= NO_CARD) return PageStoreStatus::INVALID_DOCUMENT;
    auto *pages = take<PageDefinition>(page_count);
    auto *cards = take<CardDefinition>(card_count);
    auto *source = take<uint8_t>(size);
    if (!pages || !cards || !source) return PageStoreStatus::BUFFER_TOO_SMALL;
    std::memcpy(source, document, size);
    store_.pages_ = pages; store_.cards_ = cards; store_.source_ = source;
    store_.page_count_ = page_count; store_.card_count_ = card_count;
    store_.source_version_ = PANEL_CONFIG_DOCUMENT_VERSION_2;
    store_.strings_begin_ = offset_;
    store_.memory_.record_bytes = page_count * sizeof(PageDefinition) + card_count * sizeof(CardDefinition);
    store_.memory_.source_bytes = size;
    pages[0].card_count = root_count; pages[0].runtime_card_count = limits.slot_count;
    pages[0].navigable = true; pages[0].order = intern(std::string(reader.string(root.third)));
    auto normalized = [&](const Row &card, bool child, bool compact, ParsedCfg *parsed) {
      const auto fields = saved_fields(reader, card, child);
      if (card.flags == PANEL_CONFIG_V2_RAW_SOURCE) {
        Fields raw;
        if (!raw_fields(reader.string(card.raw_source), child, compact,
                        state->decoded.data(), state->decoded.size(), &raw) || raw != fields)
          return false;
      }
      ParsedCfg saved{std::string(fields[0]), std::string(fields[1]), std::string(fields[2]),
        std::string(fields[3]), std::string(fields[4]), std::string(fields[5]),
        std::string(fields[6]), std::string(fields[7]), std::string(fields[8])};
      if (child) {
        SubpageBtn button{saved.entity, saved.label, saved.icon, saved.icon_on, saved.sensor,
          saved.unit, saved.type, saved.precision, saved.options};
        *parsed = parsed_cfg_from_subpage_btn(normalize_subpage_btn(std::move(button)));
      } else *parsed = normalize_parsed_cfg(std::move(saved));
      return true;
    };
    PageId next_page = 1;
    CardId next_card = root_count;
    for (size_t slot = 0; slot < root_count; ++slot) {
      ParsedCfg parsed = parse_cfg("");
      Row owner;
      if (state->roots[slot]) {
        reader.record(Kind::CARD, state->roots[slot], &owner);
        if (!normalized(owner, false, false, &parsed)) return PageStoreStatus::INVALID_DOCUMENT;
        if (owner.flags == PANEL_CONFIG_V2_ABSENT_IMPORT && !state->pages[slot])
          return PageStoreStatus::INVALID_DOCUMENT;
      }
      set_card(cards[slot], parsed, slot + 1);
      const auto wire_page = state->pages[slot];
      if (!wire_page) {
        if (parsed.type == "subpage") return PageStoreStatus::INVALID_DOCUMENT;
        continue;
      }
      Row page, navigation;
      reader.record(Kind::PAGE, wire_page, &page);
      const bool folder = saved_fields(reader, owner)[6] == "subpage";
      const bool navigable = reader.record(Kind::NAVIGATION, owner.id, &navigation);
      if (folder != navigable || (page.flags == PANEL_CONFIG_V2_ABSENT_IMPORT &&
          (page.third || state->children[slot] || !folder || !navigable)))
        return PageStoreStatus::INVALID_DOCUMENT;
      auto &target = pages[next_page];
      target.parent = 0; target.owner = slot; target.first_card = next_card;
      target.card_count = state->children[slot];
      target.navigable = navigable && slot < limits.slot_count;
      target.runtime_card_count = target.navigable ? target.card_count : 0;
      target.order = intern(std::string(reader.string(page.third)));
      if (target.navigable) cards[slot].target = next_page;
      const auto children = range(wire_page);
      for (auto *position = children.first; position != children.second; ++position) {
        reader.record(Kind::CARD, position->card, &row);
        if (row.flags == PANEL_CONFIG_V2_ABSENT_IMPORT ||
            !normalized(row, true, page.flags == PANEL_CONFIG_V2_COMPACT_IMPORT, &parsed))
          return PageStoreStatus::INVALID_DOCUMENT;
        set_card(cards[next_card++], parsed, position->slot);
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
      limits.slot_count > PANEL_CONFIG_MAX_SLOT_COUNT ||
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

PageStoreStatus PageStore::v2_workspace_bytes(const uint8_t *document,
    size_t size, size_t *bytes) {
  if (!bytes || overlaps(document, size, bytes, sizeof(*bytes)))
    return PageStoreStatus::INVALID_ARGUMENT;
  *bytes = 0;
  size_t index_bytes = 0;
  if (PanelConfigV2Reader(document, size).workspace_bytes(&index_bytes) != PanelConfigStatus::OK)
    return PageStoreStatus::INVALID_DOCUMENT;
  *bytes = V2_STAGE_FIXED + index_bytes * 2;
  return PageStoreStatus::OK;
}

PageStoreStatus PageStore::stage_v2(const uint8_t *document, size_t size,
    const DeviceUiLimits &limits, uint32_t generation, void *arena, size_t capacity,
    void *workspace, size_t workspace_capacity, PageStore *output) {
  if (!document || !arena || !output || !workspace || !limits.device_profile ||
      !limits.slot_count || limits.slot_count > PANEL_CONFIG_MAX_SLOT_COUNT || capacity > UINT32_MAX ||
      reinterpret_cast<uintptr_t>(arena) % alignof(PageDefinition) ||
      reinterpret_cast<uintptr_t>(arena) % alignof(CardDefinition) ||
      reinterpret_cast<uintptr_t>(workspace) % 8 ||
      overlaps(document, size, arena, capacity) || overlaps(document, size, workspace, workspace_capacity) ||
      overlaps(output, sizeof(*output), document, size) ||
      overlaps(output, sizeof(*output), arena, capacity) ||
      overlaps(output, sizeof(*output), workspace, workspace_capacity) ||
      overlaps(arena, capacity, workspace, workspace_capacity) ||
      overlaps(output->arena_, output->arena_capacity_, arena, capacity) ||
      overlaps(output->arena_, output->arena_capacity_, workspace, workspace_capacity))
    return PageStoreStatus::INVALID_ARGUMENT;
  size_t needed = 0;
  const auto sized = v2_workspace_bytes(document, size, &needed);
  if (sized != PageStoreStatus::OK) return sized;
  if (workspace_capacity < needed) return PageStoreStatus::BUFFER_TOO_SMALL;
  return PageStoreBuilder(static_cast<uint8_t *>(arena), capacity, generation)
      .build_v2(document, size, limits, workspace, needed, output);
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
