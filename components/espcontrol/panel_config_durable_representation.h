#pragma once

#include "panel_config_representation.h"

namespace espcontrol::configuration {

// Select the preferred format within the actual backend limit. A flat v1
// document may remain v1 when its v2 representation expands beyond that limit.
// A v2 request may use the same fallback only when a round trip preserves its
// graph identities, values and import provenance. Dictionary numbers are
// representation details; page/card identities are not.
class PanelConfigDurableRepresentation {
 public:
  PanelConfigDurableRepresentation(ConfigurationBuffer::Allocate allocate,
                                   ConfigurationBuffer::Release release)
      : converted_(allocate, release), allocate_(allocate), release_(release) {}

  const uint8_t *data() const { return data_; }
  size_t size() const { return size_; }
  uint16_t version() const { return version_; }

  PanelConfigRepresentationStatus select(uint16_t source, uint16_t preferred,
      const uint8_t *document, size_t size, size_t durable_limit) {
    using S = PanelConfigRepresentationStatus;
    // Like PanelConfigRepresentation, a reused owner cannot consume its own
    // previous output: a conversion would invalidate that borrowed input.
    if (data_ && panel_config_v2_detail::overlap(data_, size_, document, size))
      return S::INVALID_DOCUMENT;
    data_ = nullptr; size_ = 0; version_ = 0;
    if (!document || !size || !durable_limit || (source != 1 && source != 2) ||
        (preferred != 1 && preferred != 2)) return S::INVALID_DOCUMENT;
    if (source == preferred && size <= durable_limit) {
      use(source, document, size); return S::OK;
    }
    if (source != preferred) {
      const auto status = converted_.convert(source, preferred, document, size, durable_limit);
      if (status == S::OK) {
        use(preferred, converted_.data(), converted_.size()); return S::OK;
      }
      // Allocation failure or invalid input must never silently select a
      // different format. This fallback addresses storage expansion only.
      if (status != S::TOO_LARGE || preferred != 2) return status;
      if (source == 1 && size <= durable_limit &&
          PanelConfigReader(document, size).validate() == PanelConfigStatus::OK) {
        use(1, document, size); return S::OK;
      }
      return S::TOO_LARGE;
    }
    if (preferred != 2) return S::TOO_LARGE;
    const auto projection = converted_.convert(2, 1, document, size, durable_limit);
    if (projection != S::OK)
      return projection == S::NOT_REPRESENTABLE ? S::TOO_LARGE : projection;
    PanelConfigRepresentation roundtrip(allocate_, release_);
    const auto imported = roundtrip.convert(1, 2, converted_.data(), converted_.size(),
                                            PANEL_CONFIG_V2_HTTP_MAX_BYTES);
    if (imported != S::OK) return imported;
    const auto same = equivalent(document, size, roundtrip.data(), roundtrip.size());
    if (same != S::OK) return same;
    use(1, converted_.data(), converted_.size()); return S::OK;
  }

 private:
  void use(uint16_t version, const uint8_t *data, size_t size) {
    version_ = version; data_ = data; size_ = size;
  }
  PanelConfigRepresentationStatus equivalent(const uint8_t *a, size_t an,
                                              const uint8_t *b, size_t bn) {
    using S = PanelConfigRepresentationStatus;
    using K = PanelConfigV2Kind;
    PanelConfigV2Reader left(a, an), right(b, bn);
    size_t ln = 0, rn = 0;
    if (left.workspace_bytes(&ln) != PanelConfigStatus::OK ||
        right.workspace_bytes(&rn) != PanelConfigStatus::OK)
      return S::INVALID_DOCUMENT;
    if (ln > PANEL_CONFIG_CONVERSION_MAX_WORKSPACE ||
        rn > PANEL_CONFIG_CONVERSION_MAX_WORKSPACE - ln) return S::TOO_LARGE;
    ConfigurationBuffer index(allocate_, release_);
    auto *workspace = index.allocate(ln + rn);
    if (!workspace) return S::UNAVAILABLE;
    if (left.validate(workspace, ln) != PanelConfigStatus::OK ||
        right.validate(workspace + ln, rn) != PanelConfigStatus::OK)
      return S::INVALID_DOCUMENT;
    std::array<size_t, 8> left_count{}, right_count{};
    PanelConfigV2Record row, other;
    for (size_t i = 0; i < right.record_count(); ++i) {
      right.record(i, &row);
      ++right_count[static_cast<size_t>(row.kind)];
    }
    for (size_t i = 0; i < left.record_count(); ++i) {
      left.record(i, &row);
      ++left_count[static_cast<size_t>(row.kind)];
      if (row.kind == K::STRING || row.kind == K::ENTITY) continue;
      if (row.kind == K::SETTING) {
        bool found = false;
        for (size_t j = 0; j < right.record_count(); ++j) {
          right.record(j, &other);
          if (other.kind == K::SETTING && left.string(row.id) == right.string(other.id)) {
            found = left.string(row.first) == right.string(other.first); break;
          }
        }
        if (!found) return S::TOO_LARGE;
        continue;
      }
      if (!right.record(row.kind, row.id, &other) || row.flags != other.flags)
        return S::TOO_LARGE;
      if (row.kind == K::DEVICE_PROFILE) {
        if (row.text != other.text) return S::TOO_LARGE;
      } else if (row.kind == K::PAGE) {
        if (row.first != other.first || row.second != other.second ||
            left.string(row.third) != right.string(other.third)) return S::TOO_LARGE;
      } else if (row.kind == K::NAVIGATION) {
        if (row.first != other.first) return S::TOO_LARGE;
      } else if (row.kind == K::CARD) {
        if (row.first != other.first || row.second != other.second ||
            row.field_mask != other.field_mask ||
            left.string(row.raw_source) != right.string(other.raw_source)) return S::TOO_LARGE;
        for (size_t f = 0; f < PANEL_CONFIG_V2_FIELD_COUNT; ++f) {
          const bool entity = f == 0 || f == 4;
          if ((entity ? left.entity(row.fields[f]) : left.string(row.fields[f])) !=
              (entity ? right.entity(other.fields[f]) : right.string(other.fields[f]))) return S::TOO_LARGE;
        }
      }
    }
    return left_count == right_count ? S::OK : S::TOO_LARGE;
  }
  PanelConfigRepresentation converted_;
  ConfigurationBuffer::Allocate allocate_;
  ConfigurationBuffer::Release release_;
  const uint8_t *data_{nullptr};
  size_t size_{0};
  uint16_t version_{0};
};

}  // namespace espcontrol::configuration
