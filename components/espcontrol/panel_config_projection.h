#pragma once

#include "panel_config_document_v2.h"
#include "button_grid_contract_generated.h"

namespace espcontrol::configuration {

enum class PanelConfigProjectionStatus : uint8_t {
  OK, INVALID_ARGUMENT, INVALID_DOCUMENT, NOT_REPRESENTABLE, BUFFER_TOO_SMALL,
};

namespace panel_config_projection_detail {
using View = std::string_view;
using Fields = std::array<View, 9>;
using Kind = PanelConfigV2Kind;
using Row = PanelConfigV2Record;
using Status = PanelConfigProjectionStatus;
constexpr size_t TEXT_BYTES = PANEL_CONFIG_V2_MAX_STRING_BYTES;
struct Position { uint16_t page, slot, card, reserved; };
static_assert(sizeof(Position) == 8);
struct Page { uint16_t id, owner, slot; };
struct State {
  std::array<uint16_t, 32> roots{};
  std::array<Page, 32> pages{};
  size_t page_count{0};
  std::array<char, TEXT_BYTES> decoded{};
  std::array<char, TEXT_BYTES> body{};
};
constexpr size_t FIXED = (sizeof(State) + 7) & ~size_t(7);
inline const uint8_t *bytes(View value) {
  return reinterpret_cast<const uint8_t *>(value.empty() ? "" : value.data());
}
inline int hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
// Preserve an invalid contiguous escape run exactly, matching the deployed
// parser and decodeURIComponent-based browser adapter, including BOM and NUL.
inline bool decode(View input, char *output, size_t capacity, size_t *used, View *value) {
  const size_t start = *used;
  auto append = [&](View text) {
    if (text.size() > capacity - *used) return false;
    if (!text.empty()) std::memcpy(output + *used, text.data(), text.size());
    *used += text.size(); return true;
  };
  for (size_t i = 0; i < input.size();) {
    const size_t before = *used, run = i;
    while (i + 2 < input.size() && input[i] == '%' && hex(input[i + 1]) >= 0 && hex(input[i + 2]) >= 0) {
      if (*used == capacity) return false;
      output[(*used)++] = char((hex(input[i + 1]) << 4) | hex(input[i + 2])); i += 3;
    }
    if (i != run) {
      if (!panel_config_valid_utf8(reinterpret_cast<const uint8_t *>(output + before), *used - before)) {
        *used = before;
        if (!append(input.substr(run, i - run))) return false;
      }
    } else {
      if (!append(input.substr(i++, 1))) return false;
    }
  }
  *value = View(output + start, *used - start); return true;
}
inline void child_defaults(Fields *fields) {
  if ((*fields)[2].empty()) (*fields)[2] = "Auto";
  if ((*fields)[3].empty()) (*fields)[3] = "Auto";
}
inline bool raw_fields(View raw, bool child, bool compact, State *state, Fields *fields) {
  *fields = {};
  if (!child) { compact = !raw.empty() && raw.front() == '~'; if (compact) raw.remove_prefix(1); }
  constexpr size_t CHILD_ORDER[9] = {6, 0, 1, 2, 3, 4, 5, 7, 8};
  const char separator = compact ? ',' : child ? ':' : ';';
  size_t offset = 0, used = 0;
  for (size_t i = 0; i < 9; ++i) {
    View token;
    if (offset <= raw.size()) {
      const size_t end = raw.find(separator, offset);
      token = raw.substr(offset, end == View::npos ? raw.size() - offset : end - offset);
      offset = end == View::npos ? raw.size() + 1 : end + 1;
    }
    const size_t field = child && compact ? CHILD_ORDER[i] : i;
    if (child && compact && i == 0) (*fields)[field] = card_contract_subpage_type_view(token);
    else if (compact) {
      if (!decode(token, state->decoded.data(), state->decoded.size(), &used, &(*fields)[field])) return false;
    } else (*fields)[field] = token;
  }
  if (child) child_defaults(fields);
  return true;
}
inline Fields saved_fields(const PanelConfigV2Reader &reader, const Row &card, bool child = false) {
  Fields fields;
  for (size_t i = 0; i < fields.size(); ++i)
    fields[i] = i == 0 || i == 4 ? reader.entity(card.fields[i]) : reader.string(card.fields[i]);
  if (child) child_defaults(&fields);
  return fields;
}
class Buffer {
 public:
  Buffer(char *data, size_t capacity) : data_(data), capacity_(capacity) {}
  bool append(View value) {
    if (!valid_ || value.size() > capacity_ - used_) return valid_ = false;
    if (!value.empty()) std::memcpy(data_ + used_, value.data(), value.size());
    used_ += value.size(); return true;
  }
  bool encoded(View value) {
    constexpr char HEX[] = "0123456789ABCDEF";
    for (unsigned char c : value) {
      if (c == '%' || c == ',' || c == ';' || c == '|' || c == ':') {
        const char escaped[] = {'%', HEX[c >> 4], HEX[c & 15]};
        if (!append(View(escaped, 3))) return false;
      } else { const char literal = char(c); if (!append(View(&literal, 1))) return false; }
    }
    return true;
  }
  bool joined(const Fields &fields, char separator, bool compact, bool literal_type = false) {
    size_t count = fields.size();
    while (count > 1 && fields[count - 1].empty()) --count;
    for (size_t i = 0; i < count; ++i) {
      if (i && !append(View(&separator, 1))) return false;
      if (compact && !(literal_type && i == 0)) { if (!encoded(fields[i])) return false; }
      else if (!append(fields[i])) return false;
    }
    return true;
  }
  View view() const { return View(data_, used_); }
 private:
  char *data_; size_t capacity_, used_{0}; bool valid_{true};
};
inline bool plain_safe(const Fields &fields, View separators) {
  return std::none_of(fields.begin(), fields.end(), [&](View field) { return field.find_first_of(separators) != View::npos; });
}
inline bool root_text(const Fields &fields, State *state, View *text) {
  Buffer compact(state->body.data(), state->body.size());
  if (compact.append("~") && compact.joined(fields, ',', true)) { *text = compact.view(); return true; }
  if (!plain_safe(fields, ";") || (!fields[0].empty() && fields[0].front() == '~')) return false;
  Buffer plain(state->body.data(), state->body.size());
  if (!plain.joined(fields, ';', false)) return false;
  *text = plain.view(); return true;
}
inline bool page_text(const PanelConfigV2Reader &reader, const Row &page,
                      const Position *begin, const Position *end, bool compact,
                      State *state, View *text) {
  const View order = reader.string(page.third);
  if (!compact && !order.empty() && order.front() == '~') return false;
  Buffer buffer(state->body.data(), state->body.size());
  if ((compact && !buffer.append("~")) || !buffer.append(order)) return false;
  for (auto *position = begin; position != end; ++position) {
    Row card; if (!reader.record(Kind::CARD, position->card, &card) || !buffer.append("|")) return false;
    if (card.flags == PANEL_CONFIG_V2_RAW_SOURCE) {
      if (compact != (page.flags == PANEL_CONFIG_V2_COMPACT_IMPORT) || !buffer.append(reader.string(card.raw_source))) return false;
      continue;
    }
    Fields fields = saved_fields(reader, card, true);
    if (fields[2] == "Auto") fields[2] = {};
    if (fields[3] == "Auto") fields[3] = {};
    if (compact) {
      const auto code = card_contract_subpage_code_view(fields[6]);
      if (code.find_first_of(",|") != View::npos || card_contract_subpage_type_view(code) != fields[6]) return false;
      const Fields reordered{code, fields[0], fields[1], fields[2], fields[3], fields[4], fields[5], fields[7], fields[8]};
      if (!buffer.joined(reordered, ',', true, true)) return false;
    } else if (!plain_safe(fields, ":|") || !buffer.joined(fields, ':', false)) return false;
  }
  *text = buffer.view(); return true;
}
class Publication {
 public:
  Publication(uint8_t *out, size_t capacity, size_t *size) : out_(out), capacity_(capacity), size_(size) {
    *size_ = 0; clear();
  }
  ~Publication() { if (!committed_) clear(); }
  void commit(size_t size) { out_[0] = 'E'; *size_ = size; committed_ = true; }
 private:
  void clear() { if (capacity_ >= 4) std::memset(out_, 0, 4); }
  uint8_t *out_; size_t capacity_; size_t *size_; bool committed_{false};
};
inline Status writer_status(PanelConfigStatus status) {
  return status == PanelConfigStatus::OK ? Status::OK :
      status == PanelConfigStatus::BUFFER_TOO_SMALL ? Status::BUFFER_TOO_SMALL : Status::NOT_REPRESENTABLE;
}
}  // namespace panel_config_projection_detail

// Includes the validated-reader index, an eight-byte/card grouping upper bound,
// and two record-sized text windows. No full decoded model or heap allocation.
inline PanelConfigProjectionStatus panel_config_v2_to_v1_workspace_bytes(
    const uint8_t *document, size_t size, size_t *bytes) {
  using namespace panel_config_projection_detail;
  if (!bytes || panel_config_v2_detail::overlap(document, size, bytes, sizeof(*bytes))) return Status::INVALID_ARGUMENT;
  *bytes = 0;
  PanelConfigV2Reader reader(document, size);
  size_t index_bytes = 0;
  if (reader.workspace_bytes(&index_bytes) != PanelConfigStatus::OK) return Status::INVALID_DOCUMENT;
  *bytes = FIXED + index_bytes * 2; return Status::OK;
}

// Caller owns disjoint immutable input, private output and aligned scratch.
// The result is published only on success; failed conversion clears its marker.
// Invalid/aliased arguments leave buffers untouched rather than corrupt input.
// Profile/channel and legacy preference byte/chunk admission are separate gates.
inline PanelConfigProjectionStatus panel_config_v2_to_v1(
    const uint8_t *document, size_t size, uint8_t *output, size_t capacity,
    void *workspace, size_t workspace_capacity, size_t *written) {
  using namespace panel_config_projection_detail;
  using panel_config_v2_detail::overlap;
  if (!document || !output || !workspace || !written ||
      overlap(document, size, output, capacity) || overlap(document, size, workspace, workspace_capacity) ||
      overlap(output, capacity, workspace, workspace_capacity) ||
      overlap(document, size, written, sizeof(*written)) || overlap(output, capacity, written, sizeof(*written)) ||
      overlap(workspace, workspace_capacity, written, sizeof(*written))) return Status::INVALID_ARGUMENT;
  Publication publication(output, capacity, written);
  if (reinterpret_cast<uintptr_t>(workspace) % 8) return Status::INVALID_ARGUMENT;
  size_t needed = 0;
  const auto sizing = panel_config_v2_to_v1_workspace_bytes(document, size, &needed);
  if (sizing != Status::OK) return sizing;
  if (workspace_capacity < needed || capacity < PANEL_CONFIG_HEADER_SIZE) return Status::BUFFER_TOO_SMALL;
  auto *state = new (workspace) State{};
  const size_t index_bytes = (needed - FIXED) / 2;
  auto *index = static_cast<uint8_t *>(workspace) + FIXED;
  PanelConfigV2Reader reader(document, size);
  if (reader.validate(index, index_bytes) != PanelConfigStatus::OK) return Status::INVALID_DOCUMENT;
  auto *positions = reinterpret_cast<Position *>(index + index_bytes);
  size_t card_count = 0, setting_count = 0;
  Row row, root;
  reader.record(Kind::PAGE, 0, &root);
  const View root_order = reader.string(root.third);
  bool has_order = false;
  size_t v1_records = 1;
  for (size_t i = 0; i < reader.record_count(); ++i) {
    reader.record(i, &row);
    if (row.kind == Kind::CARD) {
      new (positions + card_count++) Position{row.first, row.second, row.id, 0};
      if (!row.first) {
        if (row.second > 32) return Status::NOT_REPRESENTABLE;
        state->roots[row.second - 1] = row.id;
        if (row.flags != PANEL_CONFIG_V2_ABSENT_IMPORT) ++v1_records;
      }
    } else if (row.kind == Kind::PAGE && row.id) {
      Row owner; reader.record(Kind::CARD, row.second, &owner);
      if (row.first || owner.second > 32 || state->page_count == state->pages.size()) return Status::NOT_REPRESENTABLE;
      state->pages[state->page_count++] = {row.id, row.second, owner.second};
      if (row.flags != PANEL_CONFIG_V2_ABSENT_IMPORT) ++v1_records;
    } else if (row.kind == Kind::SETTING) {
      ++setting_count;
      const View key = reader.string(row.id), value = reader.string(row.first);
      if (key.size() + value.size() > PANEL_CONFIG_MAX_RECORD_BODY_BYTES - 1) return Status::NOT_REPRESENTABLE;
      if (key == "button_order") {
        has_order = true; if (value != root_order) return Status::NOT_REPRESENTABLE;
      }
    }
  }
  const bool add_order = !has_order && !root_order.empty();
  if (v1_records + setting_count + size_t(add_order) > PANEL_CONFIG_MAX_RECORD_COUNT) return Status::NOT_REPRESENTABLE;
  std::sort(positions, positions + card_count, [](const Position &a, const Position &b) {
    return a.page != b.page ? a.page < b.page : a.slot < b.slot;
  });
  std::sort(state->pages.begin(), state->pages.begin() + state->page_count,
            [](const Page &a, const Page &b) { return a.slot < b.slot; });
  PanelConfigWriter writer(output, capacity);
  auto result = writer.begin(); output[0] = 0;
  if (result != PanelConfigStatus::OK) return writer_status(result);
  reader.record(Kind::DEVICE_PROFILE, 0, &row);
  result = writer.append_device_profile(bytes(row.text), row.text.size());
  if (result != PanelConfigStatus::OK) return writer_status(result);
  for (size_t slot = 0; slot < state->roots.size(); ++slot) {
    const auto id = state->roots[slot]; if (!id) continue;
    reader.record(Kind::CARD, id, &row);
    const Fields saved = saved_fields(reader, row);
    const bool owned = std::any_of(state->pages.begin(), state->pages.begin() + state->page_count,
                                  [&](const Page &page) { return page.owner == id; });
    if ((saved[6] == "subpage" && !owned) || (row.flags == PANEL_CONFIG_V2_ABSENT_IMPORT && !owned)) return Status::NOT_REPRESENTABLE;
    if (row.flags == PANEL_CONFIG_V2_ABSENT_IMPORT) continue;
    View text;
    if (row.flags == PANEL_CONFIG_V2_RAW_SOURCE) {
      Fields parsed; text = reader.string(row.raw_source);
      if (!raw_fields(text, false, false, state, &parsed) || parsed != saved) return Status::NOT_REPRESENTABLE;
    } else if (!root_text(saved, state, &text)) return Status::NOT_REPRESENTABLE;
    result = writer.append_button(uint8_t(slot + 1), bytes(text), text.size());
    if (result != PanelConfigStatus::OK) return writer_status(result);
  }
  for (size_t i = 0; i < state->page_count; ++i) {
    const auto info = state->pages[i]; Row page, owner, navigation;
    reader.record(Kind::PAGE, info.id, &page); reader.record(Kind::CARD, info.owner, &owner);
    const bool folder = saved_fields(reader, owner)[6] == "subpage";
    const bool navigable = reader.record(Kind::NAVIGATION, owner.id, &navigation);
    if (folder != navigable) return Status::NOT_REPRESENTABLE;
    const auto *begin = std::lower_bound(positions, positions + card_count, info.id,
        [](const Position &position, uint16_t id) { return position.page < id; });
    const auto *end = begin;
    while (end != positions + card_count && end->page == info.id) ++end;
    if (page.flags == PANEL_CONFIG_V2_ABSENT_IMPORT) {
      if (page.third || begin != end || !folder || !navigable) return Status::NOT_REPRESENTABLE;
      continue;
    }
    const View order = reader.string(page.third);
    if (order.find('|') != View::npos) return Status::NOT_REPRESENTABLE;
    size_t slot = 1;
    for (auto *position = begin; position != end; ++position, ++slot) {
      reader.record(Kind::CARD, position->card, &row);
      if (row.second != slot || row.flags == PANEL_CONFIG_V2_ABSENT_IMPORT) return Status::NOT_REPRESENTABLE;
      if (row.flags == PANEL_CONFIG_V2_RAW_SOURCE) {
        const View raw = reader.string(row.raw_source); Fields parsed;
        if (raw.find('|') != View::npos || !raw_fields(raw, true, page.flags == PANEL_CONFIG_V2_COMPACT_IMPORT, state, &parsed) ||
            parsed != saved_fields(reader, row, true)) return Status::NOT_REPRESENTABLE;
      }
    }
    View text;
    const bool compact = page.flags == PANEL_CONFIG_V2_COMPACT_IMPORT;
    if (!page_text(reader, page, begin, end, compact, state, &text) &&
        !page_text(reader, page, begin, end, !compact, state, &text)) return Status::NOT_REPRESENTABLE;
    result = writer.append_subpage(uint8_t(info.slot), bytes(text), text.size());
    if (result != PanelConfigStatus::OK) return writer_status(result);
  }
  for (size_t i = 0; i < reader.record_count(); ++i) {
    reader.record(i, &row); if (row.kind != Kind::SETTING) continue;
    const View key = reader.string(row.id), value = reader.string(row.first);
    result = writer.append_setting(bytes(key), key.size(), bytes(value), value.size());
    if (result != PanelConfigStatus::OK) return writer_status(result);
  }
  if (add_order) {
    result = writer.append_setting(bytes("button_order"), 12, bytes(root_order), root_order.size());
    if (result != PanelConfigStatus::OK) return writer_status(result);
  }
  size_t final_size = 0;
  result = writer.finish(&final_size);
  if (result != PanelConfigStatus::OK) return writer_status(result);
  publication.commit(final_size); return Status::OK;
}
}  // namespace espcontrol::configuration
