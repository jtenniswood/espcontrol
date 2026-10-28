#include "panel_config_text_bindings.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace espcontrol::configuration {
namespace {

constexpr char BUTTON_ORDER_KEY[] = "button_order";
constexpr char BUTTON_ON_COLOR_KEY[] = "button_on_color";

// Legacy text fields are independently validated as UTF-8. Never split a code
// point between fields, even when the byte budget itself would otherwise fit.
size_t legacy_chunk_size(const uint8_t *value, size_t remaining) {
  size_t size = std::min<size_t>(255, remaining);
  if (size < remaining) {
    while (size > 0 && (value[size] & 0xC0U) == 0x80U) --size;
  }
  return size;
}

}  // namespace

void PanelConfigTextBindings::set_device_profile(const char *device_profile) {
  device_profile_ = device_profile == nullptr ? "" : device_profile;
}

bool PanelConfigTextBindings::set_button(
    uint8_t slot, PanelConfigTextValue *button,
    const std::array<PanelConfigTextValue *, MAX_SUBPAGE_CHUNKS>
        &subpage_chunks) {
  if (slot == 0 || slot > buttons_.size())
    return false;
  if (std::any_of(subpage_chunks.begin() + PANEL_CONFIG_TEXT_CHUNK_COUNT,
                  subpage_chunks.end(),
                  [](auto *chunk) { return chunk != nullptr; }))
    return false;
  auto &sources = buttons_[slot - 1];
  sources.button = button;
  std::copy_n(subpage_chunks.begin(), sources.subpage_chunks.size(),
              sources.subpage_chunks.begin());
  return true;
}

bool PanelConfigTextBindings::configured() const {
  return !device_profile_.empty() && button_order_ != nullptr;
}

bool PanelConfigTextBindings::write_document(uint8_t *output,
                                             size_t output_capacity,
                                             size_t *document_size) const {
  return write_document(nullptr, 0, output, output_capacity, document_size);
}

bool PanelConfigTextBindings::owns_record(
    const PanelConfigRecord &record) const {
  if (record.type == PanelConfigRecordType::DEVICE_PROFILE)
    return true;
  if ((record.type == PanelConfigRecordType::BUTTON ||
       record.type == PanelConfigRecordType::SUBPAGE) &&
      (record.slot == 0 || record.slot > buttons_.size()))
    return false;
  if (record.type == PanelConfigRecordType::BUTTON)
    return buttons_[record.slot - 1].button != nullptr;
  if (record.type == PanelConfigRecordType::SUBPAGE) {
    const auto &chunks = buttons_[record.slot - 1].subpage_chunks;
    return std::any_of(chunks.begin(), chunks.end(),
                       [](auto *chunk) { return chunk != nullptr; });
  }
  if (record.type != PanelConfigRecordType::SETTING)
    return false;
  return (record.key_size == sizeof(BUTTON_ORDER_KEY) - 1 && button_order_ &&
          std::memcmp(record.key, BUTTON_ORDER_KEY, record.key_size) == 0) ||
         (record.key_size == sizeof(BUTTON_ON_COLOR_KEY) - 1 &&
          button_on_color_ &&
          std::memcmp(record.key, BUTTON_ON_COLOR_KEY, record.key_size) == 0);
}

bool PanelConfigTextBindings::merge_document(const uint8_t *native,
                                             size_t native_size,
                                             uint8_t *output,
                                             size_t output_capacity,
                                             size_t *document_size) const {
  PanelConfigReader reader(native, native_size);
  if (reader.validate() != PanelConfigStatus::OK)
    return false;
  return write_document(native, native_size, output, output_capacity,
                        document_size);
}

bool PanelConfigTextBindings::write_document(const uint8_t *native,
                                             size_t native_size,
                                             uint8_t *output,
                                             size_t output_capacity,
                                             size_t *document_size) const {
  if (document_size != nullptr) *document_size = 0;
  PanelConfigWriter writer(output, output_capacity);
  if (writer.begin() != PanelConfigStatus::OK ||
      writer.append_device_profile(
          reinterpret_cast<const uint8_t *>(device_profile_.data()),
          device_profile_.size()) != PanelConfigStatus::OK) {
    return false;
  }

  // Publication can trigger a debounced compatibility callback. Preserve the
  // distinction between an absent record and an existing explicitly empty one
  // when its live value has not changed, rather than manufacturing an edit.
  auto keep_empty = [&](PanelConfigRecordType type, uint8_t slot,
                        const char *key, size_t key_size) {
    if (!native) return false;
    PanelConfigReader original(native, native_size);
    if (original.begin() != PanelConfigStatus::OK) return false;
    PanelConfigRecord record;
    while (original.next(&record) == PanelConfigStatus::OK) {
      if (record.type == type && record.slot == slot && record.key_size == key_size &&
          (!key_size || std::memcmp(record.key, key, key_size) == 0))
        return record.value_size == 0;
    }
    return false;
  };

  for (size_t index = 0; index < buttons_.size(); ++index) {
    const ButtonSources &sources = buttons_[index];
    if (sources.button != nullptr && (!sources.button->value().empty() ||
        keep_empty(PanelConfigRecordType::BUTTON, index + 1, nullptr, 0)) &&
        writer.append_button(
            static_cast<uint8_t>(index + 1),
            reinterpret_cast<const uint8_t *>(sources.button->value().data()),
            sources.button->value().size()) != PanelConfigStatus::OK) {
      return false;
    }

    std::string subpage;
    for (PanelConfigTextValue *chunk : sources.subpage_chunks) {
      if (chunk == nullptr) continue;
      if (chunk->value().size() >
          PANEL_CONFIG_MAX_RECORD_BODY_BYTES - 1 - subpage.size()) return false;
      subpage.append(chunk->value());
    }
    if ((!subpage.empty() || keep_empty(PanelConfigRecordType::SUBPAGE, index + 1, nullptr, 0)) &&
        writer.append_subpage(static_cast<uint8_t>(index + 1),
                              reinterpret_cast<const uint8_t *>(subpage.data()),
                              subpage.size()) != PanelConfigStatus::OK) {
      return false;
    }
  }

  if ((!button_order_->value().empty() || keep_empty(PanelConfigRecordType::SETTING,
      0, BUTTON_ORDER_KEY, sizeof(BUTTON_ORDER_KEY) - 1)) &&
      writer.append_setting(
          reinterpret_cast<const uint8_t *>(BUTTON_ORDER_KEY),
          sizeof(BUTTON_ORDER_KEY) - 1,
          reinterpret_cast<const uint8_t *>(button_order_->value().data()),
          button_order_->value().size()) != PanelConfigStatus::OK) {
    return false;
  }
  if (button_on_color_ != nullptr && (!button_on_color_->value().empty() ||
      keep_empty(PanelConfigRecordType::SETTING, 0, BUTTON_ON_COLOR_KEY, sizeof(BUTTON_ON_COLOR_KEY) - 1)) &&
      writer.append_setting(
          reinterpret_cast<const uint8_t *>(BUTTON_ON_COLOR_KEY),
          sizeof(BUTTON_ON_COLOR_KEY) - 1,
          reinterpret_cast<const uint8_t *>(button_on_color_->value().data()),
          button_on_color_->value().size()) != PanelConfigStatus::OK) {
    return false;
  }
  if (native) {
    PanelConfigReader reader(native, native_size);
    if (reader.begin() != PanelConfigStatus::OK)
      return false;
    PanelConfigRecord record;
    PanelConfigStatus status;
    while ((status = reader.next(&record)) == PanelConfigStatus::OK) {
      if (owns_record(record))
        continue;
      PanelConfigStatus appended = PanelConfigStatus::INVALID_DOCUMENT;
      if (record.type == PanelConfigRecordType::BUTTON)
        appended =
            writer.append_button(record.slot, record.value, record.value_size);
      else if (record.type == PanelConfigRecordType::SUBPAGE)
        appended =
            writer.append_subpage(record.slot, record.value, record.value_size);
      else if (record.type == PanelConfigRecordType::SETTING)
        appended = writer.append_setting(record.key, record.key_size,
                                         record.value, record.value_size);
      if (appended != PanelConfigStatus::OK)
        return false;
    }
    if (status != PanelConfigStatus::END)
      return false;
  }
  return writer.finish(document_size) == PanelConfigStatus::OK;
}

bool PanelConfigTextBindings::compatible_document(const uint8_t *document, size_t size) const {
  if (!configured()) return false;
  PanelConfigReader reader(document, size);
  if (reader.validate() != PanelConfigStatus::OK || reader.begin() != PanelConfigStatus::OK) return false;
  PanelConfigRecord record;
  while (reader.next(&record) == PanelConfigStatus::OK) {
    if (record.type == PanelConfigRecordType::DEVICE_PROFILE)
      return record.value_size == device_profile_.size() &&
          std::memcmp(record.value, device_profile_.data(), record.value_size) == 0;
  }
  return false;
}

bool PanelConfigTextBindings::persist_document(const uint8_t *document, size_t size) {
  return persist_document_status(document, size) == CompatibilityStatus::OK;
}

CompatibilityStatus PanelConfigTextBindings::persist_document_status(
    const uint8_t *document, size_t document_size) {
  if (!compatible_document(document, document_size)) return CompatibilityStatus::NOT_REPRESENTABLE;
  // Check the entire compatibility projection before touching preferences.
  // Native-only settings are not text entities and keep their native limits.
  PanelConfigReader reader(document, document_size);
  if (reader.begin() != PanelConfigStatus::OK) return CompatibilityStatus::NOT_REPRESENTABLE;
  PanelConfigRecord record;
  while (reader.next(&record) == PanelConfigStatus::OK) {
    if (!owns_record(record)) continue;
    if ((record.type == PanelConfigRecordType::BUTTON ||
         (record.type == PanelConfigRecordType::SETTING && owns_record(record))) &&
        record.value_size > 255)
      return CompatibilityStatus::NOT_REPRESENTABLE;
    if (record.type != PanelConfigRecordType::SUBPAGE) continue;
    if (record.slot == 0 || record.slot > buttons_.size())
      return CompatibilityStatus::NOT_REPRESENTABLE;
    size_t offset = 0;
    for (PanelConfigTextValue *chunk : buttons_[record.slot - 1].subpage_chunks) {
      if (chunk != nullptr)
        offset += legacy_chunk_size(record.value + offset, record.value_size - offset);
    }
    if (offset != record.value_size) return CompatibilityStatus::NOT_REPRESENTABLE;
  }
  if (!apply_document(document, document_size, ApplyMode::CHECK_PERSIST))
    return CompatibilityStatus::NOT_REPRESENTABLE;
  // Free obsolete empty defaults before updating nonempty fields. A nearly
  // full NVS partition may otherwise fail on the first replacement blob even
  // though the desired compatibility projection is much smaller.
  if (!apply_document(document, document_size, ApplyMode::PREPARE_EMPTY)) {
    legacy_mirror_current_ = false;
    return CompatibilityStatus::RETRYABLE_FAILURE;
  }
  const bool mirrored =
      apply_document(document, document_size, ApplyMode::PERSIST);
  legacy_mirror_current_ = mirrored;
  return mirrored ? CompatibilityStatus::OK : CompatibilityStatus::RETRYABLE_FAILURE;
}

bool PanelConfigTextBindings::publish_document(const uint8_t *document,
                                               size_t document_size) {
  return publish_document_status(document, document_size) == CompatibilityStatus::OK;
}

CompatibilityStatus PanelConfigTextBindings::publish_document_status(const uint8_t *document, size_t size) {
  if (!compatible_document(document, size)) return CompatibilityStatus::NOT_REPRESENTABLE;
  return apply_document(document, size, ApplyMode::PUBLISH) ? CompatibilityStatus::OK
                                                          : CompatibilityStatus::RETRYABLE_FAILURE;
}

bool PanelConfigTextBindings::write_value(PanelConfigTextValue *target,
                                          const char *value, size_t value_size,
                                          ApplyMode mode) {
  if (target == nullptr) return false;
  if (mode == ApplyMode::CHECK_PERSIST)
    return target->can_persist_value(value_size);
  if (mode == ApplyMode::PREPARE_EMPTY)
    return value_size != 0 || target->prepare_empty_value();
  const bool persist = mode == ApplyMode::PERSIST;
  const std::string &current = target->value();
  if ((!persist || legacy_mirror_current_) && current.size() == value_size &&
      (value_size == 0 || std::memcmp(current.data(), value, value_size) == 0)) {
    return true;
  }
  if (!persist) legacy_mirror_current_ = false;
  return persist ? target->set_value(value, value_size)
                 : target->publish_value(value, value_size);
}

bool PanelConfigTextBindings::apply_document(const uint8_t *document,
                                             size_t document_size,
                                             ApplyMode mode) {
  PanelConfigReader reader(document, document_size);
  if (reader.begin() != PanelConfigStatus::OK) return false;

  uint32_t button_slots = 0;
  uint32_t subpage_slots = 0;
  bool has_button_order = false;
  bool has_button_on_color = false;
  PanelConfigRecord record;
  PanelConfigStatus status = PanelConfigStatus::OK;
  while ((status = reader.next(&record)) == PanelConfigStatus::OK) {
    // Stored slots outside this profile are retained by the native document;
    // they are not compatibility text fields and consume no preference budget.
    if (!owns_record(record)) continue;
    if ((record.type == PanelConfigRecordType::BUTTON ||
         record.type == PanelConfigRecordType::SUBPAGE) &&
        (record.slot == 0 || record.slot > buttons_.size()))
      return false;
    if (record.type == PanelConfigRecordType::DEVICE_PROFILE) {
      if (record.value_size != device_profile_.size() ||
          std::memcmp(record.value, device_profile_.data(), record.value_size) !=
              0) {
        return false;
      }
    } else if (record.type == PanelConfigRecordType::BUTTON) {
      ButtonSources &sources = buttons_[record.slot - 1];
      if (sources.button == nullptr ||
          !write_value(sources.button,
                       reinterpret_cast<const char *>(record.value),
                       record.value_size, mode)) {
        return false;
      }
      button_slots |= uint32_t{1} << (record.slot - 1);
    } else if (record.type == PanelConfigRecordType::SUBPAGE) {
      ButtonSources &sources = buttons_[record.slot - 1];
      size_t offset = 0;
      size_t remaining_chunks = static_cast<size_t>(std::count_if(
          sources.subpage_chunks.begin(), sources.subpage_chunks.end(),
          [](PanelConfigTextValue *chunk) { return chunk != nullptr; }));
      for (PanelConfigTextValue *chunk : sources.subpage_chunks) {
        if (chunk == nullptr) continue;
        --remaining_chunks;
        // Runtime publication bypasses the preference-backed 255-byte text
        // calls. Keep existing chunks, with native overflow in the final field.
        const size_t chunk_size =
            mode == ApplyMode::PUBLISH && remaining_chunks == 0
                ? record.value_size - offset
                : legacy_chunk_size(record.value + offset,
                                    record.value_size - offset);
        if (!write_value(chunk,
                         reinterpret_cast<const char *>(record.value + offset),
                         chunk_size, mode)) {
          return false;
        }
        offset += chunk_size;
      }
      if (offset != record.value_size) return false;
      subpage_slots |= uint32_t{1} << (record.slot - 1);
    } else if (record.type == PanelConfigRecordType::SETTING &&
               record.key_size == sizeof(BUTTON_ORDER_KEY) - 1 &&
               std::memcmp(record.key, BUTTON_ORDER_KEY, record.key_size) == 0) {
      if (!write_value(button_order_,
                       reinterpret_cast<const char *>(record.value),
                       record.value_size, mode)) {
        return false;
      }
      has_button_order = true;
    } else if (record.type == PanelConfigRecordType::SETTING &&
               record.key_size == sizeof(BUTTON_ON_COLOR_KEY) - 1 &&
               std::memcmp(record.key, BUTTON_ON_COLOR_KEY, record.key_size) ==
                   0) {
      if (button_on_color_ != nullptr &&
          !write_value(button_on_color_,
                       reinterpret_cast<const char *>(record.value),
                       record.value_size, mode)) {
        return false;
      }
      has_button_on_color = true;
    }
  }
  if (status != PanelConfigStatus::END) return false;

  if (!has_button_order && !write_value(button_order_, "", 0, mode))
    return false;
  if (!has_button_on_color && button_on_color_ != nullptr &&
      !write_value(button_on_color_, "", 0, mode))
    return false;
  for (size_t index = 0; index < buttons_.size(); ++index) {
    ButtonSources &sources = buttons_[index];
    const uint32_t slot_mask = uint32_t{1} << index;
    if ((button_slots & slot_mask) == 0 && sources.button != nullptr &&
        !write_value(sources.button, "", 0, mode))
      return false;
    if ((subpage_slots & slot_mask) != 0) continue;
    for (PanelConfigTextValue *chunk : sources.subpage_chunks) {
      if (chunk != nullptr && !write_value(chunk, "", 0, mode))
        return false;
    }
  }
  return true;
}

}  // namespace espcontrol::configuration
