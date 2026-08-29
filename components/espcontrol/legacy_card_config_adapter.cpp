#include "legacy_card_config_adapter.h"

#include "legacy_card_config.h"
#include "esphome/core/preferences.h"

namespace espcontrol {
namespace {
constexpr size_t CONFIG_CHUNK_BYTES = 255;

void publish_config(esphome::text::Text *config, const std::string &value) {
  auto call = config->make_call();
  call.set_value(value);
  call.perform();
}
}  // namespace

void LegacyCardConfigAdapter::configure(
    esphome::text::Text *const *main_configs, size_t main_count,
    std::initializer_list<esphome::text::Text *const *> subpage_chunks,
    size_t subpage_count) {
  main_configs_.assign(main_configs, main_configs + main_count);
  subpage_chunks_.clear();
  subpage_chunks_.reserve(subpage_chunks.size());
  for (auto *chunk : subpage_chunks) {
    subpage_chunks_.emplace_back(chunk, chunk + subpage_count);
  }
  subpage_count_ = subpage_count;
}

bool LegacyCardConfigAdapter::ready() const {
  if (main_configs_.empty() || subpage_chunks_.empty() || subpage_count_ == 0) return false;
  for (auto *config : main_configs_) {
    if (config == nullptr) return false;
  }
  for (const auto &chunk : subpage_chunks_) {
    if (chunk.size() != subpage_count_) return false;
    for (auto *config : chunk) {
      if (config == nullptr) return false;
    }
  }
  return true;
}

bool LegacyCardConfigAdapter::clear_asset_references(const std::string &asset_id) {
  if (!ready()) return false;

  for (auto *config : main_configs_) {
    std::string updated = config->state;
    if (clear_card_background_reference(updated, asset_id)) publish_config(config, updated);
  }

  for (size_t page = 0; page < subpage_count_; ++page) {
    std::string combined;
    for (const auto &chunk : subpage_chunks_) combined += chunk[page]->state;
    if (!clear_subpage_background_references(combined, asset_id)) continue;

    std::vector<std::string> split_chunks;
    if (!split_subpage_config(combined, subpage_chunks_.size(), CONFIG_CHUNK_BYTES,
                              split_chunks)) {
      return false;
    }
    for (size_t chunk = 0; chunk < split_chunks.size(); ++chunk) {
      publish_config(subpage_chunks_[chunk][page], split_chunks[chunk]);
    }
  }

  if (esphome::global_preferences == nullptr || !esphome::global_preferences->sync()) return false;
  return !references_asset(asset_id);
}

bool LegacyCardConfigAdapter::references_asset(const std::string &asset_id) const {
  if (!ready()) return false;
  for (auto *config : main_configs_) {
    if (card_config_references_asset(config->state, asset_id)) return true;
  }
  for (size_t page = 0; page < subpage_count_; ++page) {
    std::string combined;
    for (const auto &chunk : subpage_chunks_) combined += chunk[page]->state;
    if (subpage_config_references_asset(combined, asset_id)) return true;
  }
  return false;
}

}  // namespace espcontrol
