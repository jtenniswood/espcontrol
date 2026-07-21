#pragma once

#include <initializer_list>
#include <vector>

#include "card_asset_service.h"
#include "esphome/components/text/text.h"

namespace espcontrol {

// Compatibility adapter for the template-text configuration entities used by
// existing installations. It lets CardAssetService perform one device-owned
// reference transaction without changing the public configuration format.
class LegacyCardConfigAdapter final : public CardAssetReferenceAdapter {
 public:
  void configure(esphome::text::Text *const *main_configs, size_t main_count,
                 std::initializer_list<esphome::text::Text *const *> subpage_chunks,
                 size_t subpage_count);

  bool ready() const override;
  bool clear_asset_references(const std::string &asset_id) override;
  bool references_asset(const std::string &asset_id) const override;

 private:
  std::vector<esphome::text::Text *> main_configs_{};
  std::vector<std::vector<esphome::text::Text *>> subpage_chunks_{};
  size_t subpage_count_{0};
};

}  // namespace espcontrol
