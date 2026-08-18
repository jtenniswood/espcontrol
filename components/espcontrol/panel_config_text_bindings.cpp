#include "panel_config_text_bindings.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace espcontrol::configuration {
namespace {

constexpr char BUTTON_ORDER_KEY[] = "button_order";
constexpr char BUTTON_ON_COLOR_KEY[] = "button_on_color";

bool append_text(std::array<uint8_t, PANEL_CONFIG_MAX_RECORD_BODY_BYTES - 1>
                     *output,
                 size_t *output_size, const std::string &value) {
  if (output == nullptr || output_size == nullptr ||
      value.size() > output->size() - *output_size)
    return false;
  if (!value.empty()) {
    std::memcpy(output->data() + *output_size, value.data(), value.size());
    *output_size += value.size();
  }
  return true;
}

}  // namespace

void PanelConfigTextBindings::set_device_profile(const char *device_profile) {
  device_profile_ = device_profile == nullptr ? "" : device_profile;
}

void PanelConfigTextBindings::set_button(
    uint8_t slot, PanelConfigTextValue *button,
    const std::array<PanelConfigTextValue *, MAX_SUBPAGE_CHUNKS>
        &subpage_chunks) {
  if (slot == 0 || slot > buttons_.size()) return;
  buttons_[slot - 1] = {button, subpage_chunks};
}

bool PanelConfigTextBindings::configured() const {
  return !device_profile_.empty() && button_order_ != nullptr;
}

bool PanelConfigTextBindings::write_document(uint8_t *output,
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

  for (size_t index = 0; index < buttons_.size(); ++index) {
    const ButtonSources &sources = buttons_[index];
    if (sources.button != nullptr && !sources.button->value().empty() &&
        writer.append_button(
            static_cast<uint8_t>(index + 1),
            reinterpret_cast<const uint8_t *>(sources.button->value().data()),
            sources.button->value().size()) != PanelConfigStatus::OK) {
      return false;
    }

    std::array<uint8_t, PANEL_CONFIG_MAX_RECORD_BODY_BYTES - 1> subpage{};
    size_t subpage_size = 0;
    for (PanelConfigTextValue *chunk : sources.subpage_chunks) {
      if (chunk != nullptr &&
          !append_text(&subpage, &subpage_size, chunk->value())) {
        return false;
      }
    }
    if (subpage_size > 0 &&
        writer.append_subpage(static_cast<uint8_t>(index + 1), subpage.data(),
                              subpage_size) != PanelConfigStatus::OK) {
      return false;
    }
  }

  if (!button_order_->value().empty() &&
      writer.append_setting(
          reinterpret_cast<const uint8_t *>(BUTTON_ORDER_KEY),
          sizeof(BUTTON_ORDER_KEY) - 1,
          reinterpret_cast<const uint8_t *>(button_order_->value().data()),
          button_order_->value().size()) != PanelConfigStatus::OK) {
    return false;
  }
  if (button_on_color_ != nullptr && !button_on_color_->value().empty() &&
      writer.append_setting(
          reinterpret_cast<const uint8_t *>(BUTTON_ON_COLOR_KEY),
          sizeof(BUTTON_ON_COLOR_KEY) - 1,
          reinterpret_cast<const uint8_t *>(button_on_color_->value().data()),
          button_on_color_->value().size()) != PanelConfigStatus::OK) {
    return false;
  }
  return writer.finish(document_size) == PanelConfigStatus::OK;
}

bool PanelConfigTextBindings::persist_document(const uint8_t *document,
                                               size_t document_size) {
  return apply_document(document, document_size, true);
}

bool PanelConfigTextBindings::publish_document(const uint8_t *document,
                                               size_t document_size) {
  return apply_document(document, document_size, false);
}

bool PanelConfigTextBindings::write_value(PanelConfigTextValue *target,
                                          const char *value, size_t value_size,
                                          bool persist) {
  if (target == nullptr) return false;
  const std::string &current = target->value();
  if (current.size() == value_size &&
      (value_size == 0 || std::memcmp(current.data(), value, value_size) == 0)) {
    return true;
  }
  return persist ? target->set_value(value, value_size)
                 : target->publish_value(value, value_size);
}

bool PanelConfigTextBindings::apply_document(const uint8_t *document,
                                             size_t document_size,
                                             bool persist) {
  PanelConfigReader reader(document, document_size);
  if (reader.begin() != PanelConfigStatus::OK) return false;

  uint32_t button_slots = 0;
  uint32_t subpage_slots = 0;
  bool has_button_order = false;
  bool has_button_on_color = false;
  PanelConfigRecord record;
  PanelConfigStatus status = PanelConfigStatus::OK;
  while ((status = reader.next(&record)) == PanelConfigStatus::OK) {
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
                       record.value_size, persist)) {
        return false;
      }
      button_slots |= uint32_t{1} << (record.slot - 1);
    } else if (record.type == PanelConfigRecordType::SUBPAGE) {
      ButtonSources &sources = buttons_[record.slot - 1];
      size_t offset = 0;
      for (PanelConfigTextValue *chunk : sources.subpage_chunks) {
        if (chunk == nullptr) continue;
        const size_t chunk_size =
            std::min<size_t>(255, record.value_size - offset);
        if (!write_value(chunk,
                         reinterpret_cast<const char *>(record.value + offset),
                         chunk_size, persist)) {
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
                       record.value_size, persist)) {
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
                       record.value_size, persist)) {
        return false;
      }
      has_button_on_color = true;
    }
  }
  if (status != PanelConfigStatus::END) return false;

  if (!has_button_order && !write_value(button_order_, "", 0, persist))
    return false;
  if (!has_button_on_color && button_on_color_ != nullptr &&
      !write_value(button_on_color_, "", 0, persist))
    return false;
  for (size_t index = 0; index < buttons_.size(); ++index) {
    ButtonSources &sources = buttons_[index];
    const uint32_t slot_mask = uint32_t{1} << index;
    if ((button_slots & slot_mask) == 0 && sources.button != nullptr &&
        !write_value(sources.button, "", 0, persist))
      return false;
    if ((subpage_slots & slot_mask) != 0) continue;
    for (PanelConfigTextValue *chunk : sources.subpage_chunks) {
      if (chunk != nullptr && !write_value(chunk, "", 0, persist)) return false;
    }
  }
  return true;
}

}  // namespace espcontrol::configuration
