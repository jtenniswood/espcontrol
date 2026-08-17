#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

// The native panel document is deliberately independent of firmware releases.
// It stores today's compact card and subpage strings as UTF-8 payloads, while
// providing a stable envelope for gradually moving their meaning into the
// Product Model.
namespace espcontrol::configuration {

constexpr uint16_t PANEL_CONFIG_DOCUMENT_VERSION = 1;
constexpr size_t PANEL_CONFIG_HEADER_SIZE = 16;
constexpr size_t PANEL_CONFIG_MAX_DEVICE_PROFILE_BYTES = 64;
constexpr size_t PANEL_CONFIG_MAX_RECORD_BODY_BYTES = 2048;
constexpr uint16_t PANEL_CONFIG_MAX_RECORD_COUNT = 128;
constexpr uint8_t PANEL_CONFIG_MAX_SLOT_COUNT = 32;
constexpr size_t PANEL_CONFIG_MAX_SETTING_KEY_BYTES = 63;

enum class PanelConfigStatus : uint8_t {
  OK,
  END,
  INVALID_ARGUMENT,
  BUFFER_TOO_SMALL,
  INVALID_DOCUMENT,
};

enum class PanelConfigRecordType : uint8_t {
  DEVICE_PROFILE = 1,
  BUTTON = 2,
  SUBPAGE = 3,
  SETTING = 4,
};

struct PanelConfigRecord {
  PanelConfigRecordType type{PanelConfigRecordType::DEVICE_PROFILE};
  uint8_t slot{0};
  const uint8_t *value{nullptr};
  size_t value_size{0};
  const uint8_t *key{nullptr};
  size_t key_size{0};
};

inline bool panel_config_valid_utf8(const uint8_t *value, size_t value_size) {
  size_t index = 0;
  while (index < value_size) {
    const uint8_t first = value[index++];
    if (first <= 0x7FU)
      continue;
    const size_t remaining = value_size - index;
    const auto continuation = [&](size_t count) {
      if (remaining < count)
        return false;
      for (size_t offset = 0; offset < count; ++offset) {
        if ((value[index + offset] & 0xC0U) != 0x80U)
          return false;
      }
      index += count;
      return true;
    };
    if (first >= 0xC2U && first <= 0xDFU) {
      if (!continuation(1))
        return false;
    } else if (first == 0xE0U) {
      if (remaining < 2 || value[index] < 0xA0U || value[index] > 0xBFU ||
          !continuation(2))
        return false;
    } else if ((first >= 0xE1U && first <= 0xECU) ||
               (first >= 0xEEU && first <= 0xEFU)) {
      if (!continuation(2))
        return false;
    } else if (first == 0xEDU) {
      if (remaining < 2 || value[index] < 0x80U || value[index] > 0x9FU ||
          !continuation(2))
        return false;
    } else if (first == 0xF0U) {
      if (remaining < 3 || value[index] < 0x90U || value[index] > 0xBFU ||
          !continuation(3))
        return false;
    } else if (first >= 0xF1U && first <= 0xF3U) {
      if (!continuation(3))
        return false;
    } else if (first == 0xF4U) {
      if (remaining < 3 || value[index] < 0x80U || value[index] > 0x8FU ||
          !continuation(3))
        return false;
    } else {
      return false;
    }
  }
  return true;
}

class PanelConfigWriter {
public:
  PanelConfigWriter(uint8_t *output, size_t output_capacity)
      : output_(output), output_capacity_(output_capacity) {}

  PanelConfigStatus begin() {
    if (output_ == nullptr || output_capacity_ < PANEL_CONFIG_HEADER_SIZE) {
      return status_ = PanelConfigStatus::BUFFER_TOO_SMALL;
    }
    std::memset(output_, 0, PANEL_CONFIG_HEADER_SIZE);
    output_[0] = 'E';
    output_[1] = 'P';
    output_[2] = 'C';
    output_[3] = 'F';
    write_u16(output_ + 4, PANEL_CONFIG_DOCUMENT_VERSION);
    write_u16(output_ + 6, PANEL_CONFIG_HEADER_SIZE);
    offset_ = PANEL_CONFIG_HEADER_SIZE;
    record_count_ = 0;
    button_slots_ = 0;
    subpage_slots_ = 0;
    has_device_profile_ = false;
    started_ = true;
    return status_ = PanelConfigStatus::OK;
  }

  PanelConfigStatus append_device_profile(const uint8_t *value,
                                          size_t value_size) {
    if (!ready() || has_device_profile_ || value == nullptr ||
        value_size == 0 || value_size > PANEL_CONFIG_MAX_DEVICE_PROFILE_BYTES ||
        !panel_config_valid_utf8(value, value_size)) {
      return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    }
    const PanelConfigStatus append_status =
        append_record(PanelConfigRecordType::DEVICE_PROFILE, value, value_size);
    if (append_status == PanelConfigStatus::OK)
      has_device_profile_ = true;
    return append_status;
  }

  PanelConfigStatus append_button(uint8_t slot, const uint8_t *value,
                                  size_t value_size) {
    return append_slot_record(PanelConfigRecordType::BUTTON, slot, value,
                              value_size, &button_slots_);
  }

  PanelConfigStatus append_subpage(uint8_t slot, const uint8_t *value,
                                   size_t value_size) {
    return append_slot_record(PanelConfigRecordType::SUBPAGE, slot, value,
                              value_size, &subpage_slots_);
  }

  PanelConfigStatus append_setting(const uint8_t *key, size_t key_size,
                                   const uint8_t *value, size_t value_size) {
    if (!ready() || key == nullptr || value == nullptr || key_size == 0 ||
        key_size > PANEL_CONFIG_MAX_SETTING_KEY_BYTES ||
        key_size > PANEL_CONFIG_MAX_RECORD_BODY_BYTES - 1 ||
        value_size > PANEL_CONFIG_MAX_RECORD_BODY_BYTES - 1 - key_size ||
        !panel_config_valid_utf8(key, key_size) ||
        !panel_config_valid_utf8(value, value_size) ||
        setting_key_seen(key, key_size)) {
      return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    }
    const size_t body_size = 1 + key_size + value_size;
    if (!can_append(body_size))
      return status_ = PanelConfigStatus::BUFFER_TOO_SMALL;
    write_record_header(PanelConfigRecordType::SETTING, body_size);
    output_[offset_++] = static_cast<uint8_t>(key_size);
    std::memcpy(output_ + offset_, key, key_size);
    offset_ += key_size;
    std::memcpy(output_ + offset_, value, value_size);
    offset_ += value_size;
    ++record_count_;
    return status_ = PanelConfigStatus::OK;
  }

  PanelConfigStatus finish(size_t *document_size) {
    if (!ready() || !has_device_profile_)
      return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    write_u32(output_ + 8,
              static_cast<uint32_t>(offset_ - PANEL_CONFIG_HEADER_SIZE));
    write_u16(output_ + 12, record_count_);
    if (document_size != nullptr)
      *document_size = offset_;
    started_ = false;
    return status_ = PanelConfigStatus::OK;
  }

private:
  static void write_u16(uint8_t *output, uint16_t value) {
    output[0] = static_cast<uint8_t>(value & 0xFFU);
    output[1] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
  }
  static void write_u32(uint8_t *output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value & 0xFFU);
    output[1] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
    output[2] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
    output[3] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
  }
  bool ready() const { return started_ && status_ == PanelConfigStatus::OK; }
  bool can_append(size_t body_size) const {
    return record_count_ < PANEL_CONFIG_MAX_RECORD_COUNT &&
           body_size <= PANEL_CONFIG_MAX_RECORD_BODY_BYTES &&
           offset_ <= output_capacity_ && output_capacity_ - offset_ >= 3 &&
           body_size <= output_capacity_ - offset_ - 3;
  }
  void write_record_header(PanelConfigRecordType type, size_t body_size) {
    output_[offset_++] = static_cast<uint8_t>(type);
    write_u16(output_ + offset_, static_cast<uint16_t>(body_size));
    offset_ += 2;
  }
  PanelConfigStatus append_record(PanelConfigRecordType type,
                                  const uint8_t *value, size_t value_size) {
    if (value_size > PANEL_CONFIG_MAX_RECORD_BODY_BYTES ||
        !can_append(value_size)) {
      return status_ = PanelConfigStatus::BUFFER_TOO_SMALL;
    }
    write_record_header(type, value_size);
    std::memcpy(output_ + offset_, value, value_size);
    offset_ += value_size;
    ++record_count_;
    return status_ = PanelConfigStatus::OK;
  }
  PanelConfigStatus append_slot_record(PanelConfigRecordType type, uint8_t slot,
                                       const uint8_t *value, size_t value_size,
                                       uint32_t *seen_slots) {
    const uint32_t slot_mask = slot > 0 && slot <= PANEL_CONFIG_MAX_SLOT_COUNT
                                   ? (uint32_t{1} << (slot - 1))
                                   : 0;
    if (!ready() || value == nullptr || slot_mask == 0 ||
        (*seen_slots & slot_mask) != 0 ||
        value_size > PANEL_CONFIG_MAX_RECORD_BODY_BYTES - 1 ||
        !panel_config_valid_utf8(value, value_size)) {
      return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    }
    if (!can_append(value_size + 1)) {
      return status_ = PanelConfigStatus::BUFFER_TOO_SMALL;
    }
    write_record_header(type, value_size + 1);
    output_[offset_++] = slot;
    std::memcpy(output_ + offset_, value, value_size);
    offset_ += value_size;
    *seen_slots |= slot_mask;
    ++record_count_;
    return status_ = PanelConfigStatus::OK;
  }
  bool setting_key_seen(const uint8_t *key, size_t key_size) const {
    size_t cursor = PANEL_CONFIG_HEADER_SIZE;
    while (cursor < offset_) {
      const auto type = static_cast<PanelConfigRecordType>(output_[cursor]);
      const size_t body_size = static_cast<size_t>(output_[cursor + 1]) |
                               (static_cast<size_t>(output_[cursor + 2]) << 8U);
      if (type == PanelConfigRecordType::SETTING && body_size >= 1 &&
          output_[cursor + 3] == key_size &&
          std::memcmp(output_ + cursor + 4, key, key_size) == 0)
        return true;
      cursor += 3 + body_size;
    }
    return false;
  }

  uint8_t *output_{nullptr};
  size_t output_capacity_{0};
  size_t offset_{0};
  uint16_t record_count_{0};
  uint32_t button_slots_{0};
  uint32_t subpage_slots_{0};
  bool started_{false};
  bool has_device_profile_{false};
  PanelConfigStatus status_{PanelConfigStatus::OK};
};

class PanelConfigReader {
public:
  PanelConfigReader(const uint8_t *document, size_t document_size)
      : document_(document), document_size_(document_size) {}

  PanelConfigStatus begin() {
    if (document_ == nullptr || document_size_ < PANEL_CONFIG_HEADER_SIZE ||
        document_[0] != 'E' || document_[1] != 'P' || document_[2] != 'C' ||
        document_[3] != 'F' ||
        read_u16(document_ + 4) != PANEL_CONFIG_DOCUMENT_VERSION ||
        read_u16(document_ + 6) != PANEL_CONFIG_HEADER_SIZE ||
        read_u16(document_ + 14) != 0) {
      return status_ = PanelConfigStatus::INVALID_DOCUMENT;
    }
    const size_t payload_size = read_u32(document_ + 8);
    if (payload_size != document_size_ - PANEL_CONFIG_HEADER_SIZE ||
        read_u16(document_ + 12) > PANEL_CONFIG_MAX_RECORD_COUNT) {
      return status_ = PanelConfigStatus::INVALID_DOCUMENT;
    }
    cursor_ = PANEL_CONFIG_HEADER_SIZE;
    record_count_ = read_u16(document_ + 12);
    records_read_ = 0;
    button_slots_ = 0;
    subpage_slots_ = 0;
    has_device_profile_ = false;
    started_ = true;
    return status_ = PanelConfigStatus::OK;
  }

  PanelConfigStatus next(PanelConfigRecord *record) {
    if (!started_ || record == nullptr)
      return status_ = PanelConfigStatus::INVALID_ARGUMENT;
    if (records_read_ == record_count_) {
      if (cursor_ != document_size_ || !has_device_profile_)
        return status_ = PanelConfigStatus::INVALID_DOCUMENT;
      return status_ = PanelConfigStatus::END;
    }
    if (cursor_ > document_size_ || document_size_ - cursor_ < 3)
      return status_ = PanelConfigStatus::INVALID_DOCUMENT;
    const size_t record_offset = cursor_;
    const auto type = static_cast<PanelConfigRecordType>(document_[cursor_++]);
    const size_t body_size = read_u16(document_ + cursor_);
    cursor_ += 2;
    if (body_size > PANEL_CONFIG_MAX_RECORD_BODY_BYTES ||
        body_size > document_size_ - cursor_) {
      return status_ = PanelConfigStatus::INVALID_DOCUMENT;
    }
    const uint8_t *body = document_ + cursor_;
    cursor_ += body_size;
    ++records_read_;
    *record = {};
    record->type = type;
    if (type == PanelConfigRecordType::DEVICE_PROFILE) {
      if (has_device_profile_ || body_size == 0 ||
          body_size > PANEL_CONFIG_MAX_DEVICE_PROFILE_BYTES ||
          !panel_config_valid_utf8(body, body_size)) {
        return status_ = PanelConfigStatus::INVALID_DOCUMENT;
      }
      has_device_profile_ = true;
      record->value = body;
      record->value_size = body_size;
    } else if (type == PanelConfigRecordType::BUTTON ||
               type == PanelConfigRecordType::SUBPAGE) {
      if (body_size < 1 || body[0] == 0 ||
          body[0] > PANEL_CONFIG_MAX_SLOT_COUNT)
        return status_ = PanelConfigStatus::INVALID_DOCUMENT;
      const uint32_t slot_mask = uint32_t{1} << (body[0] - 1);
      uint32_t *seen_slots = type == PanelConfigRecordType::BUTTON
                                 ? &button_slots_
                                 : &subpage_slots_;
      if ((*seen_slots & slot_mask) != 0)
        return status_ = PanelConfigStatus::INVALID_DOCUMENT;
      if (!panel_config_valid_utf8(body + 1, body_size - 1)) {
        return status_ = PanelConfigStatus::INVALID_DOCUMENT;
      }
      *seen_slots |= slot_mask;
      record->slot = body[0];
      record->value = body + 1;
      record->value_size = body_size - 1;
    } else if (type == PanelConfigRecordType::SETTING) {
      if (body_size < 2 || body[0] == 0 ||
          body[0] > PANEL_CONFIG_MAX_SETTING_KEY_BYTES ||
          static_cast<size_t>(body[0]) >= body_size ||
          !panel_config_valid_utf8(body + 1, body[0]) ||
          !panel_config_valid_utf8(body + 1 + body[0],
                                   body_size - 1 - body[0]) ||
          setting_key_seen(body + 1, body[0], record_offset)) {
        return status_ = PanelConfigStatus::INVALID_DOCUMENT;
      }
      record->key = body + 1;
      record->key_size = body[0];
      record->value = body + 1 + body[0];
      record->value_size = body_size - 1 - body[0];
    } else {
      return status_ = PanelConfigStatus::INVALID_DOCUMENT;
    }
    return status_ = PanelConfigStatus::OK;
  }

  PanelConfigStatus validate() {
    PanelConfigStatus current = begin();
    if (current != PanelConfigStatus::OK)
      return current;
    PanelConfigRecord record;
    while ((current = next(&record)) == PanelConfigStatus::OK) {
    }
    return current == PanelConfigStatus::END ? PanelConfigStatus::OK : current;
  }

private:
  static uint16_t read_u16(const uint8_t *input) {
    return static_cast<uint16_t>(input[0]) |
           (static_cast<uint16_t>(input[1]) << 8U);
  }
  static uint32_t read_u32(const uint8_t *input) {
    return static_cast<uint32_t>(input[0]) |
           (static_cast<uint32_t>(input[1]) << 8U) |
           (static_cast<uint32_t>(input[2]) << 16U) |
           (static_cast<uint32_t>(input[3]) << 24U);
  }
  bool setting_key_seen(const uint8_t *key, size_t key_size, size_t end) const {
    size_t cursor = PANEL_CONFIG_HEADER_SIZE;
    while (cursor < end) {
      const auto type = static_cast<PanelConfigRecordType>(document_[cursor]);
      const size_t body_size = read_u16(document_ + cursor + 1);
      if (type == PanelConfigRecordType::SETTING && body_size >= 1 &&
          document_[cursor + 3] == key_size &&
          std::memcmp(document_ + cursor + 4, key, key_size) == 0)
        return true;
      cursor += 3 + body_size;
    }
    return false;
  }
  const uint8_t *document_{nullptr};
  size_t document_size_{0};
  size_t cursor_{0};
  uint16_t record_count_{0};
  uint16_t records_read_{0};
  uint32_t button_slots_{0};
  uint32_t subpage_slots_{0};
  bool started_{false};
  bool has_device_profile_{false};
  PanelConfigStatus status_{PanelConfigStatus::OK};
};

} // namespace espcontrol::configuration
