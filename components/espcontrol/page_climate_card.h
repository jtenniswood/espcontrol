#pragma once

#include "page_entity_state_runtime.h"

namespace espcontrol::pages {
enum class ClimateCardLabel : uint8_t { LABEL, STATUS, ACTUAL, TARGET };
enum class ClimateCardNumber : uint8_t { TARGET, ACTUAL, ICON };
struct ClimateCardOptions {
  ClimateCardLabel label{ClimateCardLabel::LABEL};
  ClimateCardNumber number{ClimateCardNumber::TARGET};
  uint8_t precision{0};
  bool half_step{false};
};
using PageCardTranslate = const char *(*)(const char *);
struct ClimateCardPresentation {
  bool active{false},icon_on{false},number{false};
  std::string_view unit,configured_label;
  char value[48]{},label[257]{};
};
// Translation returns a firmware-lifetime string or the supplied key. Results
// copy labels before returning; no view may refer to these temporary buffers.
// State is pulled from declared dependencies only, without contexts/callbacks.
ClimateCardPresentation read_climate_card(std::string_view entity,std::string_view label,
    ClimateCardOptions,ConfigurationHandle,PageEntityStateRuntime &,std::string_view unit,
    PageCardTranslate translate=nullptr);
}  // namespace espcontrol::pages
