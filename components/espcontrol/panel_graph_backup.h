#pragma once

#include <array>
#include <string>
#include <string_view>

#include "panel_backup_stream.h"
#include "panel_config_document_v2.h"
#include "panel_config_representation.h"

namespace espcontrol::backup {
static_assert(sizeof(BACKUP_CARD_FIELDS) / sizeof(*BACKUP_CARD_FIELDS) ==
              configuration::PANEL_CONFIG_V2_FIELD_COUNT);

// The caller owns immutable native bytes and an aligned validation index. Both
// must outlive this view and the complete HTTP response. Prepare before sending
// headers; no PageStore, legacy projection or second document is retained here.
class PanelGraphBackupView {
 public:
  PanelGraphBackupView(const uint8_t* document, size_t size)
      : document_(document), size_(size), reader_(document, size) {}

  configuration::PanelConfigStatus workspace_bytes(size_t* bytes) const {
    if (size_ > configuration::PANEL_CONFIG_V2_HTTP_MAX_BYTES)
      return configuration::PanelConfigStatus::INVALID_DOCUMENT;
    return reader_.workspace_bytes(bytes);
  }

  configuration::PanelConfigStatus prepare(void* workspace, size_t capacity,
                                            size_t slots, std::string_view expected_profile) {
    using namespace configuration;
    ready_ = false;
    if (!slots || slots > PANEL_CONFIG_MAX_SLOT_COUNT || expected_profile.empty())
      return PanelConfigStatus::INVALID_ARGUMENT;
    size_t needed = 0;
    const auto sized = workspace_bytes(&needed);
    if (sized != PanelConfigStatus::OK) return sized;
    const auto valid = reader_.validate(workspace, capacity);
    if (valid != PanelConfigStatus::OK) return valid;
    PanelConfigV2Record profile, root;
    if (!reader_.record(PanelConfigV2Kind::DEVICE_PROFILE, 0, &profile) ||
        profile.text != expected_profile || !reader_.record(PanelConfigV2Kind::PAGE, 0, &root))
      return PanelConfigStatus::INVALID_DOCUMENT;
    profile_ = profile.text;
    order_ = reader_.string(root.third);
    color_ = "0073FF";
    root_cards_.fill(0);
    slots_ = slots;
    for (size_t i = 0; i < reader_.record_count(); ++i) {
      PanelConfigV2Record row;
      if (!reader_.record(i, &row)) return PanelConfigStatus::INVALID_DOCUMENT;
      if (row.kind == PanelConfigV2Kind::CARD && row.second > PANEL_CONFIG_MAX_SLOT_COUNT)
        return PanelConfigStatus::INVALID_DOCUMENT;
      if (row.kind == PanelConfigV2Kind::PAGE && row.id && !page_key(row))
        return PanelConfigStatus::INVALID_DOCUMENT;
      if (row.kind == PanelConfigV2Kind::CARD && row.first == 0 && row.second <= slots)
        root_cards_[row.second - 1] = row.id;
      if (row.kind == PanelConfigV2Kind::SETTING && reader_.string(row.id) == "button_on_color" && row.first)
        color_ = reader_.string(row.first);
    }
    ready_ = true;
    return PanelConfigStatus::OK;
  }

  template<typename Sink>
  bool write(const std::string& settings, const std::string& screen,
             const std::string& identity, const std::string& exported_at, Sink& sink) const {
    if (!ready_) return false;
    // Format three requires the complete native graph. Legacy subpage strings
    // cannot represent these links; never substitute a flattened projection.
    if (!sink("{\"version\":3,\"format\":\"espcontrol.backup\",\"device\":") ||
        !write_backup_json_string(profile_, sink) || !sink(",\"source\":{\"device\":") ||
        !write_backup_json_string(profile_, sink) || !sink(",\"slots\":" + std::to_string(slots_) + "}")) return false;
    if (!exported_at.empty() && (!sink(",\"exported_at\":") || !write_backup_json_string(exported_at, sink))) return false;
    if (!identity.empty() && (!sink(",\"identity\":") || !sink(identity))) return false;
    if (!sink(",\"button_order\":") || !write_backup_json_string(order_, sink) ||
        !sink(",\"button_on_color\":") || !write_backup_json_string(color_, sink) || !sink(",\"buttons\":[")) return false;
    for (size_t i = 0; i < slots_; ++i) {
      if (i && !sink(",")) return false;
      if (!write_card(root_cards_[i], sink)) return false;
    }
    if (!sink("],\"subpage_objects\":{")) return false;
    bool first = true;
    for (size_t i = 0; i < reader_.record_count(); ++i) {
      configuration::PanelConfigV2Record page;
      if (!reader_.record(i, &page)) return false;
      if (page.kind != configuration::PanelConfigV2Kind::PAGE || !page.id) continue;
      if (!sink(std::string(first ? "\"" : ",\"") + std::to_string(page_key(page)) + "\":") ||
          !write_page(page, sink)) return false;
      first = false;
    }
    if (!sink("},\"subpages\":{},\"settings\":") || !sink(settings) || !sink(",\"screen\":") || !sink(screen) ||
        !sink(",\"native_config\":{\"document_version\":2,\"device_profile\":") ||
        !write_backup_json_string(profile_, sink) || !sink(",\"payload\":\"")) return false;
    return write_backup_base64(document_, size_, sink) && sink("\"}}");
  }

 private:
  uint16_t page_key(const configuration::PanelConfigV2Record& page) const {
    using namespace configuration;
    PanelConfigV2Record owner, parent, root_owner;
    if (!reader_.record(PanelConfigV2Kind::CARD, page.second, &owner) ||
        !owner.second || owner.second > PANEL_CONFIG_MAX_SLOT_COUNT) return 0;
    if (!page.first) return owner.second;
    if (!reader_.record(PanelConfigV2Kind::PAGE, page.first, &parent) || parent.first ||
        !reader_.record(PanelConfigV2Kind::CARD, parent.second, &root_owner) ||
        !root_owner.second || root_owner.second > PANEL_CONFIG_MAX_SLOT_COUNT) return 0;
    return PANEL_CONFIG_MAX_SLOT_COUNT + (root_owner.second - 1) * PANEL_CONFIG_MAX_SLOT_COUNT + owner.second;
  }

  template<typename Sink>
  bool write_page(const configuration::PanelConfigV2Record& page, Sink& sink) const {
    using namespace configuration;
    if (!sink("{\"order\":[")) return false;
    const auto order = reader_.string(page.third);
    for (size_t start = 0; start < order.size();) {
      size_t end = order.find(',', start);
      if (end == std::string_view::npos) end = order.size();
      if ((start && !sink(",")) || !write_backup_json_string(order.substr(start, end - start), sink)) return false;
      start = end + 1;
    }
    if (!order.empty() && order.back() == ',' && (!sink(",") || !write_backup_json_string("", sink))) return false;
    if (!sink("],\"buttons\":[")) return false;
    std::array<uint16_t, PANEL_CONFIG_MAX_SLOT_COUNT> cards{};
    size_t count = 0;
    for (size_t i = 0; i < reader_.record_count(); ++i) {
      PanelConfigV2Record card;
      if (!reader_.record(i, &card)) return false;
      if (card.kind != PanelConfigV2Kind::CARD || card.first != page.id) continue;
      cards[card.second - 1] = card.id;
      count = std::max(count, size_t(card.second));
    }
    for (size_t i = 0; i < count; ++i)
      if ((i && !sink(",")) || !write_card(cards[i], sink)) return false;
    return sink("]}");
  }

  template<typename Sink>
  bool write_card(uint16_t id, Sink& sink) const {
    using namespace configuration;
    PanelConfigV2Record card;
    if (id && !reader_.record(PanelConfigV2Kind::CARD, id, &card)) return false;
    // Preview semantic fields; original import spellings remain in the exact
    // native payload. Neither can require a full escaped-card buffer.
    if (!sink("{")) return false;
    for (size_t i = 0; i < PANEL_CONFIG_V2_FIELD_COUNT; ++i) {
      const auto reference = card.fields[i];
      auto value = i == size_t(PanelConfigV2Field::ENTITY) || i == size_t(PanelConfigV2Field::SENSOR)
          ? reader_.entity(reference) : reader_.string(reference);
      if (value.empty() && (i == 2 || i == 3)) value = "Auto";
      if (!sink(std::string(i ? ",\"" : "\"") + BACKUP_CARD_FIELDS[i] + "\":") ||
          !write_backup_json_string(value, sink)) return false;
    }
    return sink("}");
  }

  const uint8_t* document_;
  size_t size_;
  configuration::PanelConfigV2Reader reader_;
  std::array<uint16_t, configuration::PANEL_CONFIG_MAX_SLOT_COUNT> root_cards_{};
  std::string_view profile_, order_, color_;
  size_t slots_{0};
  bool ready_{false};
};
}  // namespace espcontrol::backup
