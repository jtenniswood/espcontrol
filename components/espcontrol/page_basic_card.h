#pragma once

#include "page_entity_state_runtime.h"
#include "page_climate_card.h"
#include "page_renderer_runtime.h"

namespace espcontrol::pages {
enum class BasicCardKind:uint8_t { EMPTY, TOGGLE, NUMBER, ICON, FOLDER, TEXT, DURATION, CLIMATE, STATUS, ACTION, LIGHT, COVER };
enum class BasicActionDisplay:uint8_t { NONE, ICON, TEXT, NUMBER };
enum class BasicFolderMode:uint8_t { NONE, ENTITY, CLIMATE, MOWER, CHILDREN, NUMBER, TEXT };
struct BasicCardDefinition {
  BasicCardKind kind{BasicCardKind::EMPTY};
  BasicFolderMode folder_mode{BasicFolderMode::NONE};
  ClimateCardOptions climate;
  BasicActionDisplay action_display{BasicActionDisplay::NONE};
  std::string_view entity,sensor,label,unit,icon,icon_on;
  const char *default_label{nullptr};
  std::string_view text_input,text_output,text_input_2,text_output_2,time_unit;
  bool text_overlay{false};
  uint32_t off_color{0},on_color{0};
  uint8_t precision{0};
  bool custom_off{false},custom_on{false},theme_active{false},active_color{false},large_numbers{false};
  bool presence{false},push_feedback{false};
};
// Decoded mapping labels/time units are copied once into preparation storage.
class BasicCardTextSink {
 public:
  virtual ~BasicCardTextSink()=default;
  virtual bool copy(std::string_view value,std::string_view &stored)=0;
  virtual bool copy_unit(std::string_view value,std::string_view &stored){return copy(value,stored);}
};
// Preparation only: reject not-yet-ported variants instead of showing a partial
// interpretation. Fields borrow PageStore; decoded mapping/unit strings borrow
// the supplied sink and must remain alive until this generation retires.
bool prepare_basic_card(const PageStore &,CardId,BasicCardDefinition &,BasicCardTextSink *text=nullptr);
struct BasicCardState {
  bool on{false},numeric{false},formatted_label{false},climate_icon_on{false};
  std::string_view label,unit;
  uint8_t closed_percent{0};
  char number[48]{},text[257]{};
};
// Pull only from the configuration-owned cache. It neither registers a callback
// nor owns a copy of HA state. The result's views last until the next delivery.
std::string_view basic_folder_child_indicator(const PageStore &,CardId);
bool read_basic_indicator(ConfigurationHandle,std::string_view,PageEntityStateRuntime &,BasicFolderMode=BasicFolderMode::ENTITY);
BasicCardState read_basic_card(const BasicCardDefinition &,ConfigurationHandle,PageEntityStateRuntime &,int duration_components=2,
    std::string_view temperature_unit="°C",PageCardTranslate translate=nullptr);
}  // namespace espcontrol::pages
