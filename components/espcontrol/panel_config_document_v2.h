#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <string_view>
#include <utility>

#include "panel_config_document.h"

namespace espcontrol::configuration {

// Wire identities are scoped by record kind. They are never arena offsets or
// PageStore indices. Version 1 remains the durable/default representation.
constexpr uint16_t PANEL_CONFIG_DOCUMENT_VERSION_2 = 2;
constexpr uint16_t PANEL_CONFIG_V2_NO_ID = UINT16_MAX;
constexpr uint8_t PANEL_CONFIG_V2_MAX_DEPTH = 2;
constexpr size_t PANEL_CONFIG_V2_FIELD_COUNT = 9;
constexpr size_t PANEL_CONFIG_V2_MAX_STRING_BYTES =
    PANEL_CONFIG_MAX_RECORD_BODY_BYTES - 1;
// A string identity adds two bytes; retain every deployed 4,095-byte value.
constexpr size_t PANEL_CONFIG_V2_MAX_RECORD_BODY_BYTES =
    2 + PANEL_CONFIG_V2_MAX_STRING_BYTES;

enum class PanelConfigV2Kind : uint8_t {
  DEVICE_PROFILE = 1, STRING = 2, ENTITY = 3, PAGE = 4,
  CARD = 5, NAVIGATION = 6, SETTING = 7,
};

enum class PanelConfigV2Field : uint8_t {
  ENTITY, LABEL, ICON, ICON_ON, SENSOR, UNIT, TYPE, PRECISION, OPTIONS,
};

struct PanelConfigV2Record {
  PanelConfigV2Kind kind{PanelConfigV2Kind::DEVICE_PROFILE};
  uint16_t id{0};
  // ENTITY: first=string. PAGE: first=parent, second=owner, third=order.
  // CARD: first=page, second=slot. NAVIGATION: first=target page.
  // SETTING: id=key string, first=value string.
  uint16_t first{0}, second{0}, third{0};
  uint16_t field_mask{0};
  uint8_t flags{0};
  std::array<uint16_t, PANEL_CONFIG_V2_FIELD_COUNT> fields{};
  uint16_t raw_source{0};
  std::string_view text{};
};

// PAGE flag: imported child cards use the compact legacy spelling. CARD flag:
// raw_source retains a noncanonical import for a lossless compatibility bridge.
// Neither flag is a navigation permission or a runtime-layout description.
constexpr uint8_t PANEL_CONFIG_V2_COMPACT_IMPORT = 1;
constexpr uint8_t PANEL_CONFIG_V2_RAW_SOURCE = 1;
// A synthesized v1 owner card or empty folder target had no stored record.
// Conversion validates this provenance before omitting its v1 projection.
constexpr uint8_t PANEL_CONFIG_V2_ABSENT_IMPORT = 2;

namespace panel_config_v2_detail {
inline uint16_t u16(const uint8_t *p) {
  return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}
inline uint32_t u32(const uint8_t *p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
         (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline void put16(uint8_t *p, uint16_t n) {
  p[0] = uint8_t(n); p[1] = uint8_t(n >> 8);
}
inline void put32(uint8_t *p, uint32_t n) {
  for (size_t i = 0; i < 4; ++i) p[i] = uint8_t(n >> (8 * i));
}
inline bool overlap(const void *a, size_t an, const void *b, size_t bn) {
  if (!an || !bn) return false;
  const uintptr_t x = reinterpret_cast<uintptr_t>(a);
  const uintptr_t y = reinterpret_cast<uintptr_t>(b);
  return x <= y ? y - x < an : x - y < bn;
}
inline bool identity(uint16_t id) { return id && id != PANEL_CONFIG_V2_NO_ID; }
inline size_t bits(uint16_t mask) {
  size_t n = 0;
  for (; mask; mask &= uint16_t(mask - 1)) ++n;
  return n;
}
struct Entry {
  uint32_t offset;
  uint16_t id;
  PanelConfigV2Kind kind;
  uint8_t reserved;
};
static_assert(sizeof(Entry) == 8);
inline bool ordered(const Entry &a, const Entry &b) {
  return a.kind < b.kind || (a.kind == b.kind && a.id < b.id);
}
}  // namespace panel_config_v2_detail

// This reader allocates nothing. Its caller owns both the immutable document
// and the temporary eight-byte-per-record index, including their lifetimes.
// Resource/profile admission and legacy projection are separate operations.
class PanelConfigV2Reader {
 public:
  PanelConfigV2Reader(const uint8_t *data, size_t size) : data_(data), size_(size) {}

  PanelConfigStatus workspace_bytes(size_t *bytes) const {
    if (!bytes || panel_config_v2_detail::overlap(data_, size_, bytes, sizeof(*bytes)) ||
        panel_config_v2_detail::overlap(this, sizeof(*this), bytes, sizeof(*bytes)))
      return PanelConfigStatus::INVALID_ARGUMENT;
    *bytes = 0;
    if (!header()) return PanelConfigStatus::INVALID_DOCUMENT;
    size_t offset = PANEL_CONFIG_HEADER_SIZE;
    for (size_t n = 0; n < count(); ++n) {
      PanelConfigV2Record record;
      size_t next = 0;
      if (!decode(offset, &record, &next)) return PanelConfigStatus::INVALID_DOCUMENT;
      offset = next;
    }
    if (offset != size_) return PanelConfigStatus::INVALID_DOCUMENT;
    *bytes = size_t(count()) * sizeof(Entry);
    return PanelConfigStatus::OK;
  }

  PanelConfigStatus validate(void *workspace, size_t capacity) {
    valid_ = false;
    index_ = nullptr;
    if (!header()) return PanelConfigStatus::INVALID_DOCUMENT;
    const size_t required = size_t(count()) * sizeof(Entry);
    if (!workspace || capacity < required) return PanelConfigStatus::BUFFER_TOO_SMALL;
    if (reinterpret_cast<uintptr_t>(workspace) % alignof(Entry) ||
        panel_config_v2_detail::overlap(data_, size_, workspace, required) ||
        panel_config_v2_detail::overlap(this, sizeof(*this), workspace, required))
      return PanelConfigStatus::INVALID_ARGUMENT;
    auto *index = static_cast<Entry *>(workspace);
    size_t offset = PANEL_CONFIG_HEADER_SIZE;
    for (size_t n = 0; n < count(); ++n) {
      PanelConfigV2Record r;
      size_t next = 0;
      if (!decode(offset, &r, &next)) return PanelConfigStatus::INVALID_DOCUMENT;
      new (index + n) Entry{uint32_t(offset), r.id, r.kind, 0};
      offset = next;
    }
    if (offset != size_) return PanelConfigStatus::INVALID_DOCUMENT;
    std::sort(index, index + count(), panel_config_v2_detail::ordered);
    for (size_t i = 1; i < count(); ++i)
      if (index[i].kind == index[i - 1].kind && index[i].id == index[i - 1].id)
        return PanelConfigStatus::INVALID_DOCUMENT;
    index_ = index;
    // Lookup is available privately while structural relationships are checked.
    if (!relationships()) { index_ = nullptr; return PanelConfigStatus::INVALID_DOCUMENT; }
    valid_ = true;
    return PanelConfigStatus::OK;
  }

  size_t record_count() const { return valid_ ? count() : 0; }
  bool record(size_t ordinal, PanelConfigV2Record *out) const {
    return valid_ && ordinal < count() && out && decode(index_[ordinal].offset, out, nullptr, false);
  }
  bool record(PanelConfigV2Kind kind, uint16_t id, PanelConfigV2Record *out) const {
    return valid_ && out && find(kind, id, out);
  }
  std::string_view string(uint16_t id) const { return valid_ ? text(id) : std::string_view{}; }
  std::string_view entity(uint16_t id) const {
    PanelConfigV2Record r;
    return valid_ && id && find(PanelConfigV2Kind::ENTITY, id, &r)
               ? text(r.first) : std::string_view{};
  }

 private:
  friend class PanelConfigV2Writer;
  using Entry = panel_config_v2_detail::Entry;
  using Kind = PanelConfigV2Kind;
  static constexpr uint16_t FIELD_MASK = (uint16_t{1} << PANEL_CONFIG_V2_FIELD_COUNT) - 1;
  uint16_t count() const { return panel_config_v2_detail::u16(data_ + 12); }
  bool header() const {
    using namespace panel_config_v2_detail;
    return data_ && size_ >= PANEL_CONFIG_HEADER_SIZE &&
        std::memcmp(data_, "EPCF", 4) == 0 && u16(data_ + 4) == PANEL_CONFIG_DOCUMENT_VERSION_2 &&
        u16(data_ + 6) == PANEL_CONFIG_HEADER_SIZE && !u16(data_ + 14) &&
        u32(data_ + 8) == size_ - PANEL_CONFIG_HEADER_SIZE && count() >= 2 &&
        size_ - PANEL_CONFIG_HEADER_SIZE >= 16 &&
        count() <= (size_ - PANEL_CONFIG_HEADER_SIZE - 4) / 6;
  }
  bool decode(size_t offset, PanelConfigV2Record *out, size_t *next, bool check_text = true) const {
    using namespace panel_config_v2_detail;
    if (offset > size_ || size_ - offset < 3) return false;
    const auto kind = static_cast<Kind>(data_[offset]);
    const size_t length = u16(data_ + offset + 1);
    offset += 3;
    if (length > PANEL_CONFIG_V2_MAX_RECORD_BODY_BYTES || length > size_ - offset) return false;
    const auto *p = data_ + offset;
    *out = {};
    out->kind = kind;
    if (next) *next = offset + length;
    if (kind == Kind::DEVICE_PROFILE) {
      if (!length || length > PANEL_CONFIG_MAX_DEVICE_PROFILE_BYTES ||
          (check_text && !panel_config_valid_utf8(p, length))) return false;
      out->text = {reinterpret_cast<const char *>(p), length};
      return true;
    }
    if (length < 2) return false;
    out->id = u16(p);
    if (kind == Kind::STRING) {
      if (!identity(out->id) || length == 2 ||
          (check_text && !panel_config_valid_utf8(p + 2, length - 2))) return false;
      out->text = {reinterpret_cast<const char *>(p + 2), length - 2};
      return true;
    }
    if (kind == Kind::ENTITY || kind == Kind::NAVIGATION || kind == Kind::SETTING) {
      if (length != 4 || !identity(out->id)) return false;
      out->first = u16(p + 2);
      return out->first != PANEL_CONFIG_V2_NO_ID;
    }
    if (kind == Kind::PAGE) {
      if (length != 9 || out->id == PANEL_CONFIG_V2_NO_ID) return false;
      out->first = u16(p + 2); out->second = u16(p + 4); out->third = u16(p + 6);
      out->flags = p[8];
      return out->flags <= PANEL_CONFIG_V2_ABSENT_IMPORT &&
          out->third != PANEL_CONFIG_V2_NO_ID &&
          (out->flags != PANEL_CONFIG_V2_ABSENT_IMPORT ||
           (out->id && !out->first && !out->third));
    }
    if (kind != Kind::CARD || length < 9 || !identity(out->id)) return false;
    out->first = u16(p + 2); out->second = u16(p + 4); out->field_mask = u16(p + 6);
    out->flags = p[8];
    if (out->first == PANEL_CONFIG_V2_NO_ID || !out->second ||
        (out->field_mask & ~FIELD_MASK) || out->flags > PANEL_CONFIG_V2_ABSENT_IMPORT ||
        length != 9 + bits(out->field_mask) * 2 + ((out->flags & PANEL_CONFIG_V2_RAW_SOURCE) ? 2 : 0))
      return false;
    if (out->flags == PANEL_CONFIG_V2_ABSENT_IMPORT &&
        (out->first || out->field_mask)) return false;
    size_t cursor = 9;
    for (size_t i = 0; i < out->fields.size(); ++i) {
      if (!(out->field_mask & (uint16_t{1} << i))) continue;
      out->fields[i] = u16(p + cursor); cursor += 2;
      if (!identity(out->fields[i])) return false;
    }
    if (out->flags & PANEL_CONFIG_V2_RAW_SOURCE) {
      out->raw_source = u16(p + cursor);
      if (out->raw_source == PANEL_CONFIG_V2_NO_ID) return false;
    }
    return true;
  }
  bool find(Kind kind, uint16_t id, PanelConfigV2Record *out) const {
    if (!index_ || !out) return false;
    const Entry key{0, id, kind, 0};
    const auto *entry = std::lower_bound(index_, index_ + count(), key, panel_config_v2_detail::ordered);
    return entry != index_ + count() && entry->kind == kind && entry->id == id &&
        decode(entry->offset, out, nullptr, false);
  }
  std::string_view text(uint16_t id) const {
    PanelConfigV2Record r;
    return id && find(Kind::STRING, id, &r) ? r.text : std::string_view{};
  }
  bool has(Kind kind, uint16_t id) const {
    PanelConfigV2Record r;
    return find(kind, id, &r);
  }
  bool optional_string(uint16_t id) const { return !id || has(Kind::STRING, id); }
  std::pair<Entry *, Entry *> range(Kind kind) {
    Entry key{0, 0, kind, 0};
    auto *first = std::lower_bound(index_, index_ + count(), key, panel_config_v2_detail::ordered);
    auto *last = first;
    while (last != index_ + count() && last->kind == kind) ++last;
    return {first, last};
  }
  bool relationships() {
    PanelConfigV2Record root, profile;
    if (!find(Kind::DEVICE_PROFILE, 0, &profile) || !find(Kind::PAGE, 0, &root) ||
        root.first != PANEL_CONFIG_V2_NO_ID || root.second != PANEL_CONFIG_V2_NO_ID || root.flags)
      return false;
    // Validate every reference before comparators dereference dictionary IDs.
    for (size_t n = 0; n < count(); ++n) {
      PanelConfigV2Record r, owner;
      if (!decode(index_[n].offset, &r, nullptr, false)) return false;
      if (r.kind == Kind::ENTITY) {
        if (!r.first || !has(Kind::STRING, r.first)) return false;
      } else if (r.kind == Kind::PAGE) {
        if (!optional_string(r.third)) return false;
        if (r.id && (r.first == PANEL_CONFIG_V2_NO_ID || r.first == r.id ||
            !has(Kind::PAGE, r.first) || !find(Kind::CARD, r.second, &owner) || owner.first != r.first))
          return false;
      } else if (r.kind == Kind::CARD) {
        if (!has(Kind::PAGE, r.first) || !optional_string(r.raw_source)) return false;
        for (size_t i = 0; i < r.fields.size(); ++i) {
          if (!r.fields[i]) continue;
          const auto type = i == size_t(PanelConfigV2Field::ENTITY) || i == size_t(PanelConfigV2Field::SENSOR)
                                ? Kind::ENTITY : Kind::STRING;
          if (!has(type, r.fields[i])) return false;
        }
      } else if (r.kind == Kind::NAVIGATION) {
        PanelConfigV2Record page;
        if (!r.first || !find(Kind::CARD, r.id, &owner) || !find(Kind::PAGE, r.first, &page) ||
            page.second != owner.id || page.first != owner.first ||
            text(owner.fields[size_t(PanelConfigV2Field::TYPE)]) != "subpage") return false;
      } else if (r.kind == Kind::SETTING) {
        if (!has(Kind::STRING, r.id) || !optional_string(r.first) ||
            text(r.id).size() > PANEL_CONFIG_MAX_SETTING_KEY_BYTES) return false;
      }
    }
    // Every parent chain terminates at root within the format's depth ceiling.
    const auto pages = range(Kind::PAGE);
    for (auto *entry = pages.first; entry != pages.second; ++entry) {
      uint16_t id = entry->id;
      unsigned depth = 0;
      while (id) {
        PanelConfigV2Record page;
        if (++depth > PANEL_CONFIG_V2_MAX_DEPTH || !find(Kind::PAGE, id, &page)) return false;
        id = page.first;
      }
    }
    // A card owns at most one page, including retained inactive pages.
    std::sort(pages.first, pages.second, [this](const Entry &a, const Entry &b) {
      return panel_config_v2_detail::u16(data_ + a.offset + 7) <
             panel_config_v2_detail::u16(data_ + b.offset + 7);
    });
    bool unique = true;
    for (auto *p = pages.first + 1; p < pages.second; ++p)
      if (panel_config_v2_detail::u16(data_ + p->offset + 7) ==
          panel_config_v2_detail::u16(data_ + (p - 1)->offset + 7)) unique = false;
    std::sort(pages.first, pages.second, panel_config_v2_detail::ordered);
    if (!unique) return false;
    const auto cards = range(Kind::CARD);
    std::sort(cards.first, cards.second, [this](const Entry &a, const Entry &b) {
      const auto *x = data_ + a.offset + 5, *y = data_ + b.offset + 5;
      const auto xp = panel_config_v2_detail::u16(x), yp = panel_config_v2_detail::u16(y);
      return xp != yp ? xp < yp : panel_config_v2_detail::u16(x + 2) < panel_config_v2_detail::u16(y + 2);
    });
    for (auto *p = cards.first; p != cards.second; ++p) {
      if (p != cards.first && std::memcmp(data_ + p->offset + 5, data_ + (p - 1)->offset + 5, 4) == 0)
        unique = false;
    }
    std::sort(cards.first, cards.second, panel_config_v2_detail::ordered);
    if (!unique) return false;
    // Setting identities are key text, not dictionary IDs: aliases must not
    // smuggle a second value for the same key through a different string ID.
    const auto settings = range(Kind::SETTING);
    std::sort(settings.first, settings.second, [this](const Entry &a, const Entry &b) {
      return text(a.id) < text(b.id);
    });
    for (auto *p = settings.first; p != settings.second; ++p)
      if (p != settings.first && text(p->id) == text((p - 1)->id)) unique = false;
    std::sort(settings.first, settings.second, panel_config_v2_detail::ordered);
    return unique;
  }

  const uint8_t *data_;
  size_t size_;
  Entry *index_{nullptr};
  bool valid_{false};
};


// A bounded writer publishes a usable header only after the same whole-graph
// validation as the reader. It owns neither its output nor its scratch index.
class PanelConfigV2Writer {
 public:
  PanelConfigV2Writer(uint8_t *output, size_t capacity) : output_(output), capacity_(capacity) {}
  PanelConfigStatus begin() {
    count_ = 0; offset_ = 0; started_ = false;
    if (!output_ || capacity_ < PANEL_CONFIG_HEADER_SIZE)
      return status_ = PanelConfigStatus::BUFFER_TOO_SMALL;
    if (panel_config_v2_detail::overlap(output_, capacity_, this, sizeof(*this)))
      return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    std::memset(output_, 0, PANEL_CONFIG_HEADER_SIZE);
    panel_config_v2_detail::put16(output_ + 4, PANEL_CONFIG_DOCUMENT_VERSION_2);
    panel_config_v2_detail::put16(output_ + 6, PANEL_CONFIG_HEADER_SIZE);
    offset_ = PANEL_CONFIG_HEADER_SIZE; started_ = true;
    return status_ = PanelConfigStatus::OK;
  }
  size_t workspace_bytes() const { return size_t(count_) * sizeof(panel_config_v2_detail::Entry); }
  // Private-buffer progress only; these bytes are not a published document
  // until finish succeeds. Borrowed dictionary indexes may refer to them.
  size_t buffered_bytes() const { return offset_; }
  PanelConfigStatus append(const PanelConfigV2Record &input) {
    using namespace panel_config_v2_detail;
    if (!ready()) return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    const PanelConfigV2Record r = input;
    if (r.kind != PanelConfigV2Kind::CARD &&
        (r.field_mask || r.raw_source || std::any_of(r.fields.begin(), r.fields.end(), [](uint16_t id) { return id != 0; })))
      return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    if (r.kind != PanelConfigV2Kind::CARD && r.kind != PanelConfigV2Kind::PAGE && r.flags)
      return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    if ((r.kind == PanelConfigV2Kind::DEVICE_PROFILE || r.kind == PanelConfigV2Kind::STRING) &&
        (r.first || r.second || r.third)) return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    if ((r.kind == PanelConfigV2Kind::ENTITY || r.kind == PanelConfigV2Kind::NAVIGATION || r.kind == PanelConfigV2Kind::SETTING) &&
        (r.second || r.third)) return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    if (r.kind == PanelConfigV2Kind::CARD &&
        (r.third || (!(r.flags & PANEL_CONFIG_V2_RAW_SOURCE) && r.raw_source)))
      return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    std::array<uint8_t, 29> prefix{};
    size_t used = 0;
    const auto word = [&](uint16_t value) { put16(prefix.data() + used, value); used += 2; };
    auto text = r.text;
    if (r.kind == PanelConfigV2Kind::STRING) {
      if (text.size() > PANEL_CONFIG_V2_MAX_STRING_BYTES)
        return status_ = PanelConfigStatus::INVALID_ARGUMENT;
      word(r.id);
    } else if (r.kind == PanelConfigV2Kind::DEVICE_PROFILE) {
      if (r.id || text.size() > PANEL_CONFIG_MAX_DEVICE_PROFILE_BYTES)
        return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    } else {
      if (!text.empty()) return status_ = PanelConfigStatus::INVALID_ARGUMENT;
      word(r.id);
      switch (r.kind) {
        case PanelConfigV2Kind::ENTITY:
        case PanelConfigV2Kind::NAVIGATION:
        case PanelConfigV2Kind::SETTING: word(r.first); break;
        case PanelConfigV2Kind::PAGE:
          word(r.first); word(r.second); word(r.third); prefix[used++] = r.flags;
          break;
        case PanelConfigV2Kind::CARD:
          word(r.first); word(r.second); word(r.field_mask); prefix[used++] = r.flags;
          for (size_t i = 0; i < r.fields.size(); ++i) {
            const bool present = (r.field_mask & (uint16_t{1} << i)) != 0;
            if (present != (r.fields[i] != 0)) return status_ = PanelConfigStatus::INVALID_ARGUMENT;
            if (present) word(r.fields[i]);
          }
          if (r.flags & PANEL_CONFIG_V2_RAW_SOURCE) word(r.raw_source);
          break;
        default: return status_ = PanelConfigStatus::INVALID_ARGUMENT;
      }
    }
    const size_t body = used + text.size();
    const size_t size = 3 + body;
    if (body > PANEL_CONFIG_V2_MAX_RECORD_BODY_BYTES || count_ == UINT16_MAX ||
        offset_ > capacity_ || size > capacity_ - offset_ ||
        offset_ - PANEL_CONFIG_HEADER_SIZE > UINT32_MAX - size)
      return status_ = PanelConfigStatus::BUFFER_TOO_SMALL;
    if (overlap(text.data(), text.size(), output_ + offset_, size))
      return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    output_[offset_] = uint8_t(r.kind);
    put16(output_ + offset_ + 1, uint16_t(body));
    std::memcpy(output_ + offset_ + 3, prefix.data(), used);
    if (!text.empty()) std::memcpy(output_ + offset_ + 3 + used, text.data(), text.size());
    PanelConfigV2Reader probe(output_, offset_ + size);
    PanelConfigV2Record decoded;
    if (!probe.decode(offset_, &decoded, nullptr)) return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    offset_ += size; ++count_;
    return status_ = PanelConfigStatus::OK;
  }
  PanelConfigStatus finish(void *workspace, size_t workspace_capacity, size_t *size) {
    using namespace panel_config_v2_detail;
    if (!ready() || !size || overlap(output_, offset_, size, sizeof(*size)) ||
        overlap(this, sizeof(*this), size, sizeof(*size)) ||
        overlap(workspace, workspace_bytes(), this, sizeof(*this)) ||
        overlap(workspace, workspace_bytes(), size, sizeof(*size)))
      return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    *size = 0;
    put32(output_ + 8, uint32_t(offset_ - PANEL_CONFIG_HEADER_SIZE));
    put16(output_ + 12, count_);
    std::memcpy(output_, "EPCF", 4);
    PanelConfigV2Reader reader(output_, offset_);
    status_ = reader.validate(workspace, workspace_capacity);
    started_ = false;
    if (status_ != PanelConfigStatus::OK) {
      std::memset(output_, 0, 4);
      return status_;
    }
    *size = offset_;
    return status_;
  }
 private:
  bool ready() const { return started_ && status_ == PanelConfigStatus::OK; }
  uint8_t *output_;
  size_t capacity_;
  size_t offset_{0};
  uint16_t count_{0};
  bool started_{false};
  PanelConfigStatus status_{PanelConfigStatus::INVALID_ARGUMENT};
};

}  // namespace espcontrol::configuration
