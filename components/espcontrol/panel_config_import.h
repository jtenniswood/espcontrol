#pragma once

#include "panel_config_projection.h"

namespace espcontrol::configuration {
using PanelConfigImportStatus = PanelConfigProjectionStatus;
struct PanelConfigImportBounds {
  size_t workspace_bytes{0};
  size_t output_bytes{0};
  uint32_t max_records{0};
};
namespace panel_config_import_detail {
using View = panel_config_projection_detail::View;
using Fields = panel_config_projection_detail::Fields;
using Kind = PanelConfigV2Kind;
using Row = PanelConfigV2Record;
using Status = PanelConfigImportStatus;
using panel_config_projection_detail::raw_fields;
using panel_config_projection_detail::writer_status;
constexpr size_t TEXT_BYTES = PANEL_CONFIG_V2_MAX_STRING_BYTES;
struct Setting { View key, value; };
struct State {
  std::array<View, 32> buttons{}, subpages{};
  std::array<Setting, PANEL_CONFIG_MAX_RECORD_COUNT> settings{};
  View profile{}, order{};
  size_t setting_count{0};
  std::array<char, TEXT_BYTES> decoded{};
};
struct Entry {
  uint32_t hash, offset;
  uint16_t length, string_id, entity_id, reserved;
};
static_assert(sizeof(Entry) == 16);
constexpr size_t FIXED = (sizeof(State) + 7) & ~size_t(7);
constexpr size_t MAX_BUCKETS = 131072;
inline View view(const uint8_t *data, size_t size) { return View(reinterpret_cast<const char *>(data), size); }
inline uint32_t hash(View value) {
  uint32_t h = 2166136261U;
  for (unsigned char c : value) h = (h ^ c) * 16777619U;
  return h;
}
inline size_t buckets_for(size_t strings) {
  const size_t wanted = std::max(size_t(2), std::min(strings, size_t(65534)) * 2);
  size_t count = 1; while (count < wanted) count *= 2; return count;
}
class Importer {
 public:
  Importer(uint8_t *output, size_t capacity, State *state, Entry *entries, size_t buckets)
      : output_(output), state_(state), entries_(entries), buckets_(buckets), writer_(output, capacity) {}
  Status begin() { return status_ = writer_status(writer_.begin()); }
  bool append(const Row &row) {
    if (status_ != Status::OK) return false;
    if (records_ == UINT16_MAX) { status_ = Status::NOT_REPRESENTABLE; return false; }
    const auto result = writer_.append(row);
    if (result != PanelConfigStatus::OK) { status_ = writer_status(result); return false; }
    ++records_; return true;
  }
  uint16_t intern(View text, bool as_entity = false) {
    if (text.empty() || status_ != Status::OK) return 0;
    const auto h = hash(text);
    size_t bucket = h & (buckets_ - 1);
    Entry *entry = nullptr;
    for (size_t probes = 0; probes < buckets_; ++probes) {
      auto &candidate = entries_[bucket];
      if (!candidate.string_id || (candidate.hash == h && candidate.length == text.size() &&
          std::memcmp(output_ + candidate.offset, text.data(), text.size()) == 0)) { entry = &candidate; break; }
      bucket = (bucket + 1) & (buckets_ - 1);
    }
    if (!entry) { status_ = Status::BUFFER_TOO_SMALL; return 0; }
    if (!entry->string_id) {
      if (strings_ == 65534) { status_ = Status::NOT_REPRESENTABLE; return 0; }
      if ((size_t(strings_) + 1) * 2 > buckets_) { status_ = Status::BUFFER_TOO_SMALL; return 0; }
      const size_t offset = writer_.buffered_bytes() + 5;
      Row row; row.kind = Kind::STRING; row.id = uint16_t(strings_ + 1); row.text = text;
      if (!append(row)) return 0;
      *entry = {h, uint32_t(offset), uint16_t(text.size()), ++strings_, 0, 0};
    }
    if (!as_entity) return entry->string_id;
    if (!entry->entity_id) {
      if (entities_ == 65534) { status_ = Status::NOT_REPRESENTABLE; return 0; }
      Row row; row.kind = Kind::ENTITY; row.id = uint16_t(entities_ + 1); row.first = entry->string_id;
      if (!append(row)) return 0;
      entry->entity_id = ++entities_;
    }
    return entry->entity_id;
  }
  bool card(uint16_t id, uint16_t page, uint16_t slot, const Fields &fields, View raw, bool present) {
    Row row; row.kind = Kind::CARD; row.id = id; row.first = page; row.second = slot;
    for (size_t i = 0; i < fields.size(); ++i) {
      row.fields[i] = intern(fields[i], i == 0 || i == 4);
      if (row.fields[i]) row.field_mask |= uint16_t(1 << i);
    }
    row.flags = present ? PANEL_CONFIG_V2_RAW_SOURCE : PANEL_CONFIG_V2_ABSENT_IMPORT;
    row.raw_source = present ? intern(raw) : 0;
    return append(row);
  }
  Status run() {
    Row row; row.kind = Kind::DEVICE_PROFILE; row.text = state_->profile;
    if (!append(row)) return status_;
    row = {}; row.kind = Kind::PAGE; row.first = UINT16_MAX; row.second = UINT16_MAX;
    row.third = intern(state_->order);
    if (!append(row)) return status_;
    uint16_t page_id = 1;
    uint32_t child_id = 33;
    for (uint16_t slot = 1; slot <= 32; ++slot) {
      const auto raw = state_->buttons[slot - 1], source_page = state_->subpages[slot - 1];
      const bool present = raw.data() != nullptr, page_present = source_page.data() != nullptr;
      if (!present && !page_present) continue;
      Fields fields;
      if (!raw_fields(raw, false, false, state_->decoded.data(), state_->decoded.size(), &fields)) return Status::NOT_REPRESENTABLE;
      const bool folder = fields[6] == "subpage";
      if (!card(slot, 0, slot, fields, raw, present)) return status_;
      if (!folder && !page_present) continue;
      View tokens = source_page;
      const bool compact = !tokens.empty() && tokens.front() == '~';
      if (compact) tokens.remove_prefix(1);
      size_t pipe = tokens.find('|');
      const View order = tokens.substr(0, pipe == View::npos ? tokens.size() : pipe);
      row = {}; row.kind = Kind::PAGE; row.id = page_id; row.second = slot; row.third = intern(order);
      row.flags = page_present ? uint8_t(compact) : PANEL_CONFIG_V2_ABSENT_IMPORT;
      if (!append(row)) return status_;
      if (folder) { row = {}; row.kind = Kind::NAVIGATION; row.id = slot; row.first = page_id; if (!append(row)) return status_; }
      uint16_t position = 1;
      while (pipe != View::npos) {
        const size_t start = pipe + 1; pipe = tokens.find('|', start);
        const View child = tokens.substr(start, pipe == View::npos ? tokens.size() - start : pipe - start);
        if (child_id == UINT16_MAX || !raw_fields(child, true, compact, state_->decoded.data(), state_->decoded.size(), &fields)) return Status::NOT_REPRESENTABLE;
        if (!card(uint16_t(child_id++), page_id, position++, fields, child, true)) return status_;
      }
      ++page_id;
    }
    std::sort(state_->settings.begin(), state_->settings.begin() + state_->setting_count,
              [](const Setting &a, const Setting &b) { return a.key < b.key; });
    for (size_t i = 0; i < state_->setting_count; ++i) {
      row = {}; row.kind = Kind::SETTING; row.id = intern(state_->settings[i].key); row.first = intern(state_->settings[i].value);
      if (!append(row)) return status_;
    }
    return status_;
  }
  Status finish(void *workspace, size_t capacity, size_t *written) {
    if (status_ != Status::OK) return status_;
    return writer_status(writer_.finish(workspace, capacity, written));
  }
 private:
  uint8_t *output_; State *state_; Entry *entries_; size_t buckets_;
  PanelConfigV2Writer writer_; uint16_t strings_{0}, entities_{0}; uint32_t records_{0}; Status status_{Status::OK};
};
}  // namespace panel_config_import_detail

// Conservative one-pass bounds. Callers may instead supply a smaller fixed
// scratch budget; exhaustion returns BUFFER_TOO_SMALL without publishing output.
inline PanelConfigImportStatus panel_config_v1_to_v2_bounds(
    const uint8_t *document, size_t size, PanelConfigImportBounds *bounds) {
  using namespace panel_config_import_detail;
  if (!bounds || panel_config_v2_detail::overlap(document, size, bounds, sizeof(*bounds))) return Status::INVALID_ARGUMENT;
  *bounds = {};
  PanelConfigReader reader(document, size);
  if (reader.validate() != PanelConfigStatus::OK || reader.begin() != PanelConfigStatus::OK) return Status::INVALID_DOCUMENT;
  uint32_t slots = 0; size_t children = 0, settings = 0;
  PanelConfigRecord row;
  while (reader.next(&row) == PanelConfigStatus::OK) {
    if (row.type == PanelConfigRecordType::BUTTON || row.type == PanelConfigRecordType::SUBPAGE) slots |= uint32_t(1) << (row.slot - 1);
    if (row.type == PanelConfigRecordType::SUBPAGE) children += size_t(std::count(row.value, row.value + row.value_size, uint8_t('|')));
    if (row.type == PanelConfigRecordType::SETTING) ++settings;
  }
  size_t roots = 0; for (; slots; slots &= slots - 1) ++roots;
  const size_t cards = roots + children;
  if (children > 65502 || cards + 2 > UINT16_MAX) return Status::NOT_REPRESENTABLE;
  const size_t strings = cards * 10 + roots + settings * 2 + 1;
  const size_t records = std::min(size_t(UINT16_MAX), 2 + strings + cards * 3 + roots * 2 + settings);
  const size_t table_bytes = buckets_for(strings) * sizeof(Entry);
  bounds->workspace_bytes = FIXED + std::max(table_bytes, records * sizeof(panel_config_v2_detail::Entry));
  bounds->output_bytes = size * 3 + cards * 128 + roots * 32 + settings * 32 + 64;
  bounds->max_records = uint32_t(records);
  return Status::OK;
}

inline PanelConfigImportStatus panel_config_v1_to_v2(
    const uint8_t *document, size_t size, uint8_t *output, size_t capacity,
    void *workspace, size_t workspace_capacity, size_t *written) {
  using namespace panel_config_import_detail;
  using panel_config_v2_detail::overlap;
  if (!document || !output || !workspace || !written ||
      overlap(document, size, output, capacity) || overlap(document, size, workspace, workspace_capacity) ||
      overlap(output, capacity, workspace, workspace_capacity) || overlap(document, size, written, sizeof(*written)) ||
      overlap(output, capacity, written, sizeof(*written)) || overlap(workspace, workspace_capacity, written, sizeof(*written))) return Status::INVALID_ARGUMENT;
  panel_config_projection_detail::Publication publication(output, capacity, written);
  if (reinterpret_cast<uintptr_t>(workspace) % 8) return Status::INVALID_ARGUMENT;
  PanelConfigImportBounds bounds;
  const auto sizing = panel_config_v1_to_v2_bounds(document, size, &bounds);
  if (sizing != Status::OK) return sizing;
  if (workspace_capacity < FIXED + sizeof(Entry) || capacity < PANEL_CONFIG_HEADER_SIZE) return Status::BUFFER_TOO_SMALL;
  auto *state = new (workspace) State{};
  PanelConfigReader reader(document, size); reader.begin(); PanelConfigRecord source;
  while (reader.next(&source) == PanelConfigStatus::OK) {
    const View value = view(source.value, source.value_size);
    if (source.type == PanelConfigRecordType::DEVICE_PROFILE) state->profile = value;
    else if (source.type == PanelConfigRecordType::BUTTON) state->buttons[source.slot - 1] = value;
    else if (source.type == PanelConfigRecordType::SUBPAGE) state->subpages[source.slot - 1] = value;
    else if (source.type == PanelConfigRecordType::SETTING) {
      const View key = view(source.key, source.key_size);
      state->settings[state->setting_count++] = {key, value}; if (key == "button_order") state->order = value;
    }
  }
  auto *storage = static_cast<uint8_t *>(workspace) + FIXED;
  const size_t available = workspace_capacity - FIXED;
  const size_t maximum = std::min(MAX_BUCKETS, available / sizeof(Entry));
  size_t buckets = 1; while (buckets <= maximum / 2) buckets *= 2;
  auto *entries = reinterpret_cast<Entry *>(storage);
  for (size_t i = 0; i < buckets; ++i) new (entries + i) Entry{};
  Importer importer(output, capacity, state, entries, buckets);
  auto status = importer.begin();
  if (status == Status::OK) status = importer.run();
  size_t final_size = 0;
  // The dictionary is no longer needed. Reuse its memory for whole-graph
  // validation rather than retaining both the hash table and reader index.
  if (status == Status::OK) status = importer.finish(storage, available, &final_size);
  if (status != Status::OK) return status;
  publication.commit(final_size); return Status::OK;
}
}  // namespace espcontrol::configuration
