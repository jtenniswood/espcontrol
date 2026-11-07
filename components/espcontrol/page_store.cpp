#include "page_store.h"

#include <algorithm>
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
    pages[0].navigable = true;
    pages[0].order = intern(order.text());
    set_runtime_cards(pages[0], limits, true);

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
      set_runtime_cards(page, limits);
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
    auto *state = new (workspace) V2StageState{};
    const size_t index_size = (workspace_size - V2_STAGE_FIXED) / 2;
    auto *index = static_cast<uint8_t *>(workspace) + V2_STAGE_FIXED;
    PanelConfigV2Reader reader(document, size);
    if (reader.validate(index, index_size) != PanelConfigStatus::OK)
      return PageStoreStatus::INVALID_DOCUMENT;
    PanelConfigV2Record row;
    for (size_t i = 0; i < reader.record_count(); ++i) {
      reader.record(i, &row);
      if (row.kind == PanelConfigV2Kind::PAGE && row.id &&
          limits.max_navigation_depth < (row.first ? 2 : 1))
        return PageStoreStatus::INVALID_DOCUMENT;
    }
    auto *positions = reinterpret_cast<V2Position *>(index + index_size);
    return build_v2_pages(reader, document, size, limits, *state, positions, output);
  }

 private:
  void set_runtime_cards(PageDefinition &page, const DeviceUiLimits &limits, bool root = false) {
    static_assert(MAX_GRID_SLOTS <= 32, "Page membership must cover the renderer's card IDs");
    page.runtime_card_mask = 0;
    if (page.navigable) {
      const size_t capacity = root ? limits.slot_count : limits.cards_per_subpage;
      if (root || page.card_count <= capacity) {
        // Keep the established state ownership for ordinary saved pages.
        const size_t count = std::min<size_t>(page.card_count, capacity);
        page.runtime_card_mask = count == 32 ? UINT32_MAX : (uint32_t{1} << count) - 1;
      } else {
        // With retained extra rows, Back does not imply that the final row is
        // hidden: the actual order may place that row in any visible cell.
        store_.memory_.arena_bytes = offset_;
        SubpageOrder order;
        parse_subpage_order(std::string(store_.string(page.order)), limits.slot_count, page.card_count, order);
        for (size_t position = 0; position < limits.slot_count; ++position) {
          const int slot = order.positions[position];
          const size_t cell = order.has_back_token ? position : position + 1;
          if (cell < limits.slot_count && slot > 0 && slot <= 32)
            page.runtime_card_mask |= uint32_t{1} << (slot - 1);
        }
      }
    }
    page.runtime_card_count = __builtin_popcount(page.runtime_card_mask);
  }
  static bool normalize_v2(PanelConfigV2Reader &reader,const PanelConfigV2Record &card,
                          bool child,bool compact,V2StageState &state,ParsedCfg *parsed) {
    using namespace panel_config_projection_detail;
    const auto fields=saved_fields(reader,card,child);
    if(card.flags==PANEL_CONFIG_V2_RAW_SOURCE) {
      Fields raw;
      if(!raw_fields(reader.string(card.raw_source),child,compact,state.decoded.data(),
                     state.decoded.size(),&raw) || raw!=fields)return false;
    }
    ParsedCfg saved{std::string(fields[0]),std::string(fields[1]),std::string(fields[2]),
      std::string(fields[3]),std::string(fields[4]),std::string(fields[5]),
      std::string(fields[6]),std::string(fields[7]),std::string(fields[8])};
    if(child) {
      SubpageBtn button{saved.entity,saved.label,saved.icon,saved.icon_on,saved.sensor,
        saved.unit,saved.type,saved.precision,saved.options};
      *parsed=parsed_cfg_from_subpage_btn(normalize_subpage_btn(std::move(button)));
    } else *parsed=normalize_parsed_cfg(std::move(saved));
    return true;
  }

  PageStoreStatus build_v2_pages(PanelConfigV2Reader &reader,const uint8_t *document,size_t size,
      const DeviceUiLimits &limits,V2StageState &state,V2Position *positions,PageStore *output) {
    using Kind=PanelConfigV2Kind;
    using Row=PanelConfigV2Record;
    using namespace panel_config_projection_detail;
    state.roots.fill(0);state.pages.fill(0);state.children.fill(0);
    size_t position_count=0,page_count=0,root_count=limits.slot_count;
    Row row,root;reader.record(Kind::PAGE,0,&root);
    for(size_t i=0;i<reader.record_count();++i) {
      reader.record(i,&row);
      if(row.kind==Kind::DEVICE_PROFILE && row.text!=limits.device_profile)
        return PageStoreStatus::PROFILE_MISMATCH;
      if(row.kind==Kind::CARD) {
        new(positions+position_count++)V2Position{row.first,row.second,row.id,NO_CARD};
        if(!row.first) {
          if(row.second>state.roots.size() || state.roots[row.second-1])return PageStoreStatus::INVALID_DOCUMENT;
          state.roots[row.second-1]=row.id;root_count=std::max(root_count,size_t(row.second));
        }
      } else if(row.kind==Kind::PAGE)++page_count;
      else if(row.kind==Kind::SETTING && reader.string(row.id)=="button_order" &&
              reader.string(row.first)!=reader.string(root.third))return PageStoreStatus::INVALID_DOCUMENT;
    }
    if(page_count>=NO_PAGE)return PageStoreStatus::INVALID_DOCUMENT;
    // Each CARD and PAGE consumes at most one eight-byte entry in the existing
    // second index window. No full identity-space table or retained lookup is added.
    auto *page_positions=positions+position_count;
    size_t next=0;
    for(size_t i=0;i<reader.record_count();++i) {
      reader.record(i,&row);if(row.kind!=Kind::PAGE)continue;
      Row owner;
      if(row.id) {
        if(!reader.record(Kind::CARD,row.second,&owner) || owner.first!=row.first)
          return PageStoreStatus::INVALID_DOCUMENT;
        if(!row.first) {
          if(owner.second>state.pages.size() || state.pages[owner.second-1])return PageStoreStatus::INVALID_DOCUMENT;
          state.pages[owner.second-1]=row.id;
        }
      }
      new(page_positions+next++)V2Position{row.id,row.id?owner.second:uint16_t{0},row.second,NO_PAGE};
    }
    auto key=[&](const V2Position &position) {
      if(!position.page)return std::array<uint16_t,4>{0,0,0,0};
      Row page;reader.record(Kind::PAGE,position.page,&page);
      if(!page.first)return std::array<uint16_t,4>{1,position.slot,0,position.page};
      Row parent,owner;reader.record(Kind::PAGE,page.first,&parent);
      reader.record(Kind::CARD,parent.second,&owner);
      return std::array<uint16_t,4>{2,owner.second,position.slot,position.page};
    };
    std::sort(page_positions,page_positions+page_count,[&](const V2Position &a,const V2Position &b){return key(a)<key(b);});
    if(!page_count || page_positions[0].page)return PageStoreStatus::INVALID_DOCUMENT;
    for(size_t i=0;i<page_count;++i) {
      if(i) {
        const auto previous=key(page_positions[i-1]),current=key(page_positions[i]);
        if(previous[0]==current[0] && previous[1]==current[1] && previous[2]==current[2])
          return PageStoreStatus::INVALID_DOCUMENT;
      }
      page_positions[i].reserved=static_cast<uint16_t>(i);
    }
    std::sort(positions,positions+position_count,[](const V2Position &a,const V2Position &b){
      return a.page!=b.page?a.page<b.page:a.slot<b.slot;
    });
    auto range=[&](uint16_t page) {
      auto *begin=std::lower_bound(positions,positions+position_count,page,
        [](const V2Position &position,uint16_t id){return position.page<id;});
      auto *end=begin;while(end!=positions+position_count && end->page==page)++end;
      return std::make_pair(begin,end);
    };
    size_t card_count=root_count;
    for(size_t i=1;i<page_count;++i) {
      const auto children=range(page_positions[i].page);size_t slot=1;
      for(auto *position=children.first;position!=children.second;++position,++slot)
        if(position->slot!=slot)return PageStoreStatus::INVALID_DOCUMENT;
      card_count+=slot-1;
    }
    if(card_count>=NO_CARD)return PageStoreStatus::INVALID_DOCUMENT;
    auto *pages=take<PageDefinition>(page_count);auto *cards=take<CardDefinition>(card_count);
    auto *source=take<uint8_t>(size);
    if(!pages || !cards || !source)return PageStoreStatus::BUFFER_TOO_SMALL;
    std::memcpy(source,document,size);
    store_.pages_=pages;store_.cards_=cards;store_.source_=source;
    store_.page_count_=page_count;store_.card_count_=card_count;
    store_.source_version_=PANEL_CONFIG_DOCUMENT_VERSION_2;store_.strings_begin_=offset_;
    store_.memory_.record_bytes=page_count*sizeof(PageDefinition)+card_count*sizeof(CardDefinition);
    store_.memory_.source_bytes=size;
    pages[0].card_count=root_count;
    pages[0].navigable=true;pages[0].order=intern(std::string(reader.string(root.third)));
    set_runtime_cards(pages[0],limits,true);
    for(size_t slot=0;slot<root_count;++slot) {
      ParsedCfg parsed=parse_cfg("");
      if(state.roots[slot]) {
        reader.record(Kind::CARD,state.roots[slot],&row);
        if(!normalize_v2(reader,row,false,false,state,&parsed) ||
           (row.flags==PANEL_CONFIG_V2_ABSENT_IMPORT && !state.pages[slot]))return PageStoreStatus::INVALID_DOCUMENT;
      }
      set_card(cards[slot],parsed,slot+1);
    }
    const auto roots=range(0);
    for(auto *position=roots.first;position!=roots.second;++position)position->reserved=position->slot-1;
    CardId next_card=root_count;
    for(size_t index=1;index<page_count;++index) {
      const auto &position=page_positions[index];Row page;
      reader.record(Kind::PAGE,position.page,&page);
      auto &target=pages[index];target.parent=page.first;target.owner=page.second;
      target.first_card=next_card;target.order=intern(std::string(reader.string(page.third)));
      const auto children=range(position.page);target.card_count=children.second-children.first;
      for(auto *child=children.first;child!=children.second;++child) {
        ParsedCfg parsed;reader.record(Kind::CARD,child->card,&row);
        if(row.flags==PANEL_CONFIG_V2_ABSENT_IMPORT ||
           !normalize_v2(reader,row,true,page.flags==PANEL_CONFIG_V2_COMPACT_IMPORT,state,&parsed))
          return PageStoreStatus::INVALID_DOCUMENT;
        child->reserved=next_card;set_card(cards[next_card++],parsed,child->slot);
      }
    }
    for(PageId index=1;index<page_count;++index) {
      auto &target=pages[index];const auto parent_wire=target.parent;const auto owner_wire=target.owner;
      Row parent_record,parent_owner;
      reader.record(Kind::PAGE,parent_wire,&parent_record);
      if(parent_wire)reader.record(Kind::CARD,parent_record.second,&parent_owner);
      // At depth two every parent is root or first-level. The same path key
      // used above keeps lookup bounded without a second identity table.
      const std::array<uint16_t,4> parent_key{parent_wire?uint16_t{1}:uint16_t{0},
        parent_wire?parent_owner.second:uint16_t{0},0,parent_wire};
      const auto *parent=std::lower_bound(page_positions,page_positions+page_count,parent_key,
        [&](const V2Position &position,const std::array<uint16_t,4> &value){return key(position)<value;});
      if(parent==page_positions+page_count || parent->page!=parent_wire || parent->reserved>=index)
        return PageStoreStatus::INVALID_DOCUMENT;
      Row owner;reader.record(Kind::CARD,owner_wire,&owner);
      const auto siblings=range(parent_wire);
      const auto *position=std::lower_bound(siblings.first,siblings.second,owner.second,
        [](const V2Position &candidate,uint16_t slot){return candidate.slot<slot;});
      if(position==siblings.second || position->card!=owner_wire || position->reserved==NO_CARD)
        return PageStoreStatus::INVALID_DOCUMENT;
      Row navigation;const bool navigable=reader.record(Kind::NAVIGATION,owner_wire,&navigation);
      const bool folder=saved_fields(reader,owner)[6]=="subpage";
      Row wire_page;reader.record(Kind::PAGE,page_positions[index].page,&wire_page);
      if(folder!=navigable || (wire_page.flags==PANEL_CONFIG_V2_ABSENT_IMPORT &&
          (wire_page.third || target.card_count || !folder || !navigable)))return PageStoreStatus::INVALID_DOCUMENT;
      target.parent=parent->reserved;target.owner=position->reserved;
      const auto &parent_page=pages[target.parent];
      target.navigable=navigable && parent_page.includes_runtime_card(target.owner);
      set_runtime_cards(target,limits);
      if(target.navigable)cards[target.owner].target=index;
    }
    for(size_t i=0;i<position_count;++i) {
      reader.record(Kind::CARD,positions[i].card,&row);Row navigation;
      if((saved_fields(reader,row)[6]=="subpage")!=reader.record(Kind::NAVIGATION,row.id,&navigation))
        return PageStoreStatus::INVALID_DOCUMENT;
    }
    if(failed_)return PageStoreStatus::BUFFER_TOO_SMALL;
    store_.memory_.arena_bytes=offset_;store_.memory_.string_bytes=offset_-store_.strings_begin_;
    *output=store_;return PageStoreStatus::OK;
  }

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
