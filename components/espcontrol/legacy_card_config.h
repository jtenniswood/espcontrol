#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace espcontrol {

// Removes an exact bg_image reference from one persisted main-card config.
// Both the original semicolon format and the compact '~' format are retained.
bool clear_card_background_reference(std::string &config, const std::string &asset_id);

// Removes an exact bg_image reference from every card in one persisted
// subpage document. The order/header segment is never changed.
bool clear_subpage_background_references(std::string &config, const std::string &asset_id);

bool card_config_references_asset(const std::string &config, const std::string &asset_id);
bool subpage_config_references_asset(const std::string &config, const std::string &asset_id);

// Splits a subpage document back into its ESPHome text-entity chunks without
// cutting a UTF-8 code point. Empty trailing chunks are returned explicitly.
bool split_subpage_config(const std::string &config, size_t chunk_count,
                          size_t chunk_bytes, std::vector<std::string> &out);

}  // namespace espcontrol
