#include "panel_config_legacy_adapter.h"
#include "panel_config_compatibility_view.h"
#include "panel_config_durable_representation.h"

namespace espcontrol::configuration {

LegacyLoadResult PanelConfigLegacyAdapter::load(uint8_t *output,
                                                size_t output_capacity) {
  if (!bindings_.configured())
    return {LegacyStatus::EMPTY, PANEL_CONFIG_DOCUMENT_VERSION, 0};
  size_t document_size = 0;
  if (!bindings_.write_document(output, output_capacity, &document_size)) {
    return {LegacyStatus::BUFFER_TOO_SMALL, PANEL_CONFIG_DOCUMENT_VERSION,
            document_size};
  }
  return {LegacyStatus::OK, PANEL_CONFIG_DOCUMENT_VERSION, document_size};
}

LegacyLoadResult PanelConfigLegacyAdapter::convert_for_write(
    uint16_t target, LegacyLoadResult source, uint8_t *output,
    size_t output_capacity) {
  if (source.status != LegacyStatus::OK || source.document_version == target)
    return source;
  PanelConfigDurableRepresentation converted(allocate_, release_);
  const auto status = converted.select(source.document_version, target,
      output, source.document_size, output_capacity);
  if (status != PanelConfigRepresentationStatus::OK) {
    const auto failure = status == PanelConfigRepresentationStatus::UNAVAILABLE
        ? LegacyStatus::ALLOCATION_FAILED
        : (status == PanelConfigRepresentationStatus::TOO_LARGE
               ? LegacyStatus::BUFFER_TOO_SMALL : LegacyStatus::READ_FAILED);
    return {failure, target, 0};
  }
  if (output != converted.data()) std::memcpy(output, converted.data(), converted.size());
  return {LegacyStatus::OK, converted.version(), converted.size()};
}

LegacyLoadResult PanelConfigLegacyAdapter::load_for_write(
    uint16_t target, uint8_t *output, size_t output_capacity) {
  return convert_for_write(target, load(output, output_capacity), output,
                           output_capacity);
}

LegacyLoadResult PanelConfigLegacyAdapter::merge_for_write(
    uint16_t target, uint16_t version, const uint8_t *native, size_t native_size,
    uint8_t *output, size_t output_capacity, bool *changed) {
  if (changed) *changed = false;
  bool edited = false;
  auto result = merge(version, native, native_size, output, output_capacity, &edited);
  if (edited && result.status == LegacyStatus::OK)
    result = convert_for_write(target, result, output, output_capacity);
  if (changed && result.status == LegacyStatus::OK) *changed = edited;
  return result;
}

LegacyLoadResult
PanelConfigLegacyAdapter::merge(uint16_t version, const uint8_t *native,
                                size_t native_size, uint8_t *output,
                                size_t output_capacity, bool *changed) {
  if (changed)
    *changed = false;
  if (!bindings_.configured())
    return {LegacyStatus::READ_FAILED, version, 0};
  PanelConfigCompatibilityView view(allocate_, release_);
  const auto projected = view.load(version, native, native_size, output_capacity);
  if (projected != PanelConfigRepresentationStatus::OK)
    return {projected == PanelConfigRepresentationStatus::UNAVAILABLE
                ? LegacyStatus::ALLOCATION_FAILED : LegacyStatus::READ_FAILED, version, 0};
  native = view.data(); native_size = view.size();
  size_t size = 0;
  if (!bindings_.merge_document(native, native_size, output, output_capacity,
                                &size))
    return {LegacyStatus::READ_FAILED, version, size};
  // Compare records by identity rather than byte ordering. Browser saves may
  // use a different record order from the legacy projection.
  PanelConfigReader original(native, native_size), merged(output, size);
  size_t originals = 0, matches = 0, replacements = 0;
  PanelConfigRecord record, other;
  original.begin();
  while (original.next(&record) == PanelConfigStatus::OK) {
    ++originals;
    merged.begin();
    while (merged.next(&other) == PanelConfigStatus::OK) {
      if (record.type != other.type || record.slot != other.slot ||
          record.key_size != other.key_size ||
          (record.key_size &&
           std::memcmp(record.key, other.key, record.key_size) != 0))
        continue;
      if (record.value_size == other.value_size &&
          (!record.value_size ||
           std::memcmp(record.value, other.value, record.value_size) == 0))
        ++matches;
      break;
    }
  }
  merged.begin();
  while (merged.next(&other) == PanelConfigStatus::OK)
    ++replacements;
  if (changed)
    *changed = matches != originals || matches != replacements;
  // The legacy projection is v1; merge_for_write selects its durable format.
  // An unchanged v2 view returns changed=false and is never rewritten.
  return {LegacyStatus::OK, PANEL_CONFIG_DOCUMENT_VERSION, size};
}

bool PanelConfigLegacyAdapter::publish(uint16_t version, const uint8_t *document, size_t size) {
  return publish_status(version, document, size) == CompatibilityStatus::OK;
}

bool PanelConfigLegacyAdapter::mirror(uint16_t version, const uint8_t *document, size_t size) {
  return mirror_status(version, document, size) == CompatibilityStatus::OK;
}

namespace {
CompatibilityStatus projection_status(PanelConfigRepresentationStatus status) {
  return status == PanelConfigRepresentationStatus::OK ? CompatibilityStatus::OK
      : (status == PanelConfigRepresentationStatus::UNAVAILABLE ? CompatibilityStatus::RETRYABLE_FAILURE
                                                               : CompatibilityStatus::NOT_REPRESENTABLE);
}
} // namespace

CompatibilityStatus PanelConfigLegacyAdapter::admit_write(uint16_t version, const uint8_t *document, size_t size) {
  if (!bindings_.configured()) return CompatibilityStatus::NOT_REPRESENTABLE;
  PanelConfigCompatibilityView view(allocate_, release_);
  const auto status = projection_status(view.load(version, document, size));
  return status == CompatibilityStatus::OK ? bindings_.persistence_admission(view.data(), view.size()) : status;
}

CompatibilityStatus PanelConfigLegacyAdapter::publish_status(uint16_t version, const uint8_t *document, size_t size) {
  if (!bindings_.configured()) return CompatibilityStatus::NOT_REPRESENTABLE;
  PanelConfigCompatibilityView view(allocate_, release_);
  const auto status = projection_status(view.load(version, document, size));
  if (version == PANEL_CONFIG_DOCUMENT_VERSION_2 &&
      status == CompatibilityStatus::NOT_REPRESENTABLE)
    return publish_v2_globals(document, size);
  return status == CompatibilityStatus::OK ? bindings_.publish_document_status(view.data(), view.size()) : status;
}

CompatibilityStatus PanelConfigLegacyAdapter::publish_v2_globals(
    const uint8_t *document, size_t size) {
  if (size > PANEL_CONFIG_V2_HTTP_MAX_BYTES)
    return CompatibilityStatus::NOT_REPRESENTABLE;
  PanelConfigV2Reader reader(document, size);
  size_t needed = 0;
  if (reader.workspace_bytes(&needed) != PanelConfigStatus::OK ||
      !needed || needed > PANEL_CONFIG_CONVERSION_MAX_WORKSPACE)
    return CompatibilityStatus::NOT_REPRESENTABLE;
  ConfigurationBuffer index(allocate_, release_);
  auto *workspace = index.allocate(needed);
  if (!workspace) return CompatibilityStatus::RETRYABLE_FAILURE;
  if (reader.validate(workspace, needed) != PanelConfigStatus::OK)
    return CompatibilityStatus::NOT_REPRESENTABLE;
  PanelConfigV2Record profile, root, row;
  if (!reader.record(PanelConfigV2Kind::DEVICE_PROFILE, 0, &profile) ||
      !reader.record(PanelConfigV2Kind::PAGE, 0, &root))
    return CompatibilityStatus::NOT_REPRESENTABLE;
  const auto order = reader.string(root.third);
  std::string_view color;
  for (size_t i = 0; i < reader.record_count(); ++i) {
    reader.record(i, &row);
    if (row.kind != PanelConfigV2Kind::SETTING) continue;
    const auto key = reader.string(row.id);
    if (key == "button_on_color") color = reader.string(row.first);
    if (key == "button_order" && reader.string(row.first) != order)
      return CompatibilityStatus::NOT_REPRESENTABLE;
  }
  const auto status = bindings_.publish_global_values(profile.text, order, color);
  return status == CompatibilityStatus::OK ? CompatibilityStatus::NATIVE_ONLY : status;
}

CompatibilityStatus PanelConfigLegacyAdapter::mirror_status(uint16_t version, const uint8_t *document, size_t size) {
  if (!bindings_.configured()) return CompatibilityStatus::NOT_REPRESENTABLE;
  PanelConfigCompatibilityView view(allocate_, release_);
  const auto status = projection_status(view.load(version, document, size));
  return status == CompatibilityStatus::OK ? bindings_.persist_document_status(view.data(), view.size()) : status;
}

} // namespace espcontrol::configuration
