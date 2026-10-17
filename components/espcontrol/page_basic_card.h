#pragma once

#include "page_entity_state_runtime.h"
#include "page_renderer_runtime.h"

namespace espcontrol::pages {
enum class BasicCardKind:uint8_t { EMPTY, TOGGLE, NUMBER, ICON, FOLDER };
struct BasicCardDefinition {
  BasicCardKind kind{BasicCardKind::EMPTY};
  std::string_view entity,sensor,label,unit,icon,icon_on;
  uint32_t off_color{0},on_color{0};
  uint8_t precision{0};
  bool custom_off{false},custom_on{false},theme_active{false},active_color{false},large_numbers{false};
};
// Preparation only: reject not-yet-ported variants instead of showing a partial
// interpretation. Borrowed strings refer to the immutable PageStore arena.
bool prepare_basic_card(const PageStore &,CardId,BasicCardDefinition &);
struct BasicCardState {
  bool on{false},numeric{false};
  std::string_view label,unit;
  char number[48]{};
};
// Pull only from the configuration-owned cache. It neither registers a callback
// nor owns a copy of HA state. The result's views last until the next delivery.
BasicCardState read_basic_card(const BasicCardDefinition &,ConfigurationHandle,PageEntityStateRuntime &);
}  // namespace espcontrol::pages
