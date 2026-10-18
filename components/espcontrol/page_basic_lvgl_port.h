#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <limits>
#include <new>

#include "../artwork_image/image_pipeline_policy.h"
#include "bounded_display_text.h"
#include "button_grid_slider_contrast.h"
#include "button_grid_slider_geometry.h"
#include "button_grid_string.h"
#include "cover_art.h"
#include "display_color.h"
#include "light_temperature_color.h"
#include "lvgl.h"
#include "page_basic_card.h"
#include "page_cover_art_frame.h"
#include "page_cover_art_view.h"
#include "page_forecast_frame.h"
#include "page_media_group.h"
#include "page_media_label.h"
#include "weather_condition_maps_generated.h"

namespace espcontrol::pages {
// The selected firmware supplies exactly one existing bank of grid widgets.
// The bank and fonts outlive this port; no widgets are created by navigation.
struct PageBasicWidgets {
  lv_obj_t *button{nullptr},*icon{nullptr},*label{nullptr},*sensor_container{nullptr},*value{nullptr},*unit{nullptr},*fill{nullptr},*forecast{nullptr},*slider{nullptr};
};
struct PageBasicTheme {
  uint64_t revision{0};
  uint32_t surface{0x212121},sensor_surface{0x212121},active{0xFF8C00},text{0xFFFFFF};
  int red_percent{100},green_percent{100},blue_percent{100};
  const lv_font_t *sensor_font{nullptr},*large_sensor_font{nullptr};
  std::string_view back_label{"Back"},empty_label{"Configure"},folder_label{"Subpage"};
  std::string_view temperature_unit{"°C"};
  PageCardTranslate translate{nullptr};
  const lv_font_t *forecast_small_font{nullptr},*forecast_icon_font{nullptr};
  int32_t forecast_scale_x{256},forecast_scale_y{256};
  float (*forecast_temperature)(float,const char *){nullptr};
  uint32_t media_surface{0x212121};
  const lv_font_t *media_title_font{nullptr},*icon_font{nullptr},*label_font{nullptr};
  int32_t media_text_scale_x{256},media_text_scale_y{256};
  const lv_font_t *cover_title_font{nullptr}, *cover_artist_font{nullptr};
  const lv_font_t *media_control_title_font{nullptr}, *media_control_artist_font{nullptr};
  const lv_font_t* option_select_font{nullptr};
  bool compact_portrait{false};
  int media_artwork_width_percent{100};
  uint32_t disabled_text{0x707070};
};
struct PageBasicActions {
  void *owner{nullptr};
  // Resolver runs during prepare, and must return a firmware-lifetime glyph.
  const char *(*icon)(void *,std::string_view name,std::string_view domain){nullptr};
  bool (*valid)(void *,PageBindingToken){nullptr};
  // These run only after validating the token captured on PRESSED. The caller
  // handles native service dispatch and navigation with its existing policies.
  void (*toggle)(void *,PageBindingToken,std::string_view entity){nullptr};
  void (*open)(void *,PageBindingToken,PageId){nullptr};
  void (*back)(void *,PageBindingToken){nullptr};
  void (*climate)(void *,PageBindingToken){nullptr};
  void (*action)(void *,PageBindingToken){nullptr};
  void (*control)(void *,PageBindingToken){nullptr};
  void (*forecast)(void *,PageBindingToken){nullptr};
  void (*media)(void *,PageBindingToken){nullptr};
  void (*seek)(void *,PageBindingToken,uint8_t percent,float duration){nullptr};
  void (*media_control)(void*, PageBindingToken){nullptr};
  void (*media_volume)(void*, PageBindingToken){nullptr};
  void (*percent_slider)(void*, PageBindingToken, int percent){nullptr};
  void (*number_slider)(void*, PageBindingToken, double, const number_slider::Metadata&){nullptr};
  void (*light_temperature)(void*, PageBindingToken, int kelvin){nullptr};
  bool (*timer)(void*, PageBindingToken, PageTimerAction){nullptr};
  void (*cover_command)(void*, PageBindingToken, PageCoverCommandSnapshot){nullptr};
  void (*access)(void*, PageBindingToken, PageAccessActionSnapshot){nullptr};
  void (*cleaning)(void*, PageBindingToken, const char* service){nullptr};
};
inline const lv_style_transition_dsc_t *page_basic_push_transition() {
  static const lv_style_prop_t properties[]={LV_STYLE_BG_COLOR,LV_STYLE_PROP_INV};
  static lv_style_transition_dsc_t transition{};static bool initialized=false;
  if(!initialized){lv_style_transition_dsc_init(&transition,properties,lv_anim_path_ease_out,400,0,nullptr);initialized=true;}
  return &transition;
}
inline const lv_style_transition_dsc_t *page_basic_no_transition() {
  // LVGL 9.5 dereferences an explicitly stored null transition pointer after
  // the widget has been drawn. Keep a valid descriptor with no properties.
  static const lv_style_prop_t properties[]={LV_STYLE_PROP_INV};
  static lv_style_transition_dsc_t transition{};static bool initialized=false;
  if(!initialized){lv_style_transition_dsc_init(&transition,properties,lv_anim_path_linear,0,0,nullptr);initialized=true;}
  return &transition;
}
struct PageBasicMemory {size_t fixed_bytes{0},borrowed_widgets{0},active_bytes{0},staged_bytes{0},peak_bytes{0},text_truncations{0},invalid_text{0},forecast_frames{0};};

template<size_t Slots>
class PageBasicLvglPort final:public PageRendererPort {
 public:
  PageBasicLvglPort(PageArenaAllocator& allocator, PageEntityStateRuntime& state,
                    std::array<PageBasicWidgets, Slots> widgets, PageViewGeometry geometry,
                    const PageBasicTheme& theme, PageBasicActions actions,
                    PageForecastRuntime* forecasts = nullptr, PageMediaStateRuntime* media = nullptr,
                    PageCoverArtView* artwork = nullptr, PageLocalSensorRuntime* local = nullptr,
                    PageLocalRelayRuntime* relay = nullptr, PageFanStatusRuntime* fan_status = nullptr,
                    PagePercentSliderRuntime* percent_slider = nullptr,
                    PageNumberSliderRuntime* number_slider = nullptr,
                    PageLightTemperatureRuntime* temperature = nullptr, PageTimerRuntime* timer = nullptr,
                    PageCoverToggleRuntime* cover_toggle = nullptr, PageAccessStateRuntime* access = nullptr)
      : allocator_(allocator),
        state_(state),
        widgets_(widgets),
        geometry_(geometry),
        theme_(theme),
        actions_(actions),
        forecasts_(forecasts),
        media_(media),
        artwork_(artwork),
        local_(local),
        relay_(relay),
        fan_status_(fan_status),
        percent_slider_(percent_slider),
        number_slider_(number_slider),
        temperature_(temperature),
        timer_(timer),
        cover_toggle_(cover_toggle),
        access_(access) {
    for(size_t i=0;i<Slots;++i){shells_[i].port=this;shells_[i].index=i;}
  }
  ~PageBasicLvglPort()override {
    retire_active();discard_prepared();
    for(size_t i=0;i<callbacks_;++i)
      lv_obj_remove_event_cb_with_user_data(widgets_[i].button,event,&shells_[i]);
    for(size_t slot=0;slot<Slots;++slot) {
      const auto &w=widgets_[slot];
      if(shells_[slot].forecast_callback)lv_obj_remove_event_cb_with_user_data(w.forecast,forecast_event,&shells_[slot]);
      if(shells_[slot].slider_guard_callback)lv_obj_remove_event_cb_with_user_data(w.slider,slider_guard,&shells_[slot]);
      if(shells_[slot].slider_callback)lv_obj_remove_event_cb_with_user_data(w.slider,slider_event,&shells_[slot]);
      const std::array<lv_obj_t *,4>labels={w.icon,w.label,w.value,w.unit};
      for(size_t i=0;i<shells_[slot].contrast_callbacks;++i)
        lv_obj_remove_event_cb_with_user_data(labels[i],contrast_event,&shells_[slot]);
    }
  }
  PageBasicLvglPort(const PageBasicLvglPort &)=delete;
  PageBasicLvglPort &operator=(const PageBasicLvglPort &)=delete;
  PageViewGeometry geometry(const PageStore &)const override{return geometry_;}
  // Call once on the LVGL/main loop before the first configuration transaction.
  // Preinstall every style/event property used by this port, including both
  // state selectors; subsequent page switches update existing properties.
  bool initialize() {
    if(initialized_)return true;
    if(callbacks_!=0)return false;
    for(const auto &shell:shells_)if(shell.contrast_callbacks||shell.forecast_callback||shell.slider_guard_callback||shell.slider_callback)return false;
    if(actions_.icon==nullptr)return false;
    page_basic_push_transition();
    page_basic_no_transition();
    std::array<lv_obj_t *,Slots*9> unique{};size_t used=0;
    for(const auto &w:widgets_) {
      for(auto *object:{w.button,w.icon,w.label,w.sensor_container,w.value,w.unit}) {
        if(!object)return false;
        for(size_t index=0;index<used;++index)if(unique[index]==object)return false;
        unique[used++]=object;
      }
      for(auto *optional:{w.fill,w.forecast,w.slider})if(optional) {
        for(size_t index=0;index<used;++index)if(unique[index]==optional)return false;
        unique[used++]=optional;if(lv_obj_get_parent(optional)!=w.button)return false;
      }
      if(lv_obj_get_parent(w.icon)!=w.button||lv_obj_get_parent(w.label)!=w.button||
         lv_obj_get_parent(w.sensor_container)!=w.button||lv_obj_get_parent(w.value)!=w.sensor_container||
         lv_obj_get_parent(w.unit)!=w.sensor_container)return false;
    }
    for(size_t i=0;i<Slots;++i) {
      const auto &w=widgets_[i];
      shells_[i].sensor_align = lv_obj_get_style_align(w.sensor_container, LV_PART_MAIN);
      shells_[i].sensor_x = lv_obj_get_style_x(w.sensor_container, LV_PART_MAIN);
      shells_[i].sensor_y = lv_obj_get_style_y(w.sensor_container, LV_PART_MAIN);
      shells_[i].sensor_width = lv_obj_get_style_width(w.sensor_container, LV_PART_MAIN);
      shells_[i].value_width = lv_obj_get_style_width(w.value, LV_PART_MAIN);
      shells_[i].value_mode = lv_label_get_long_mode(w.value);
      shells_[i].value_font =
          theme_.sensor_font ? theme_.sensor_font : lv_obj_get_style_text_font(w.value, LV_PART_MAIN);
      // Reserve these style properties before any visible-page binding.
      lv_obj_set_width(w.sensor_container, shells_[i].sensor_width);
      lv_obj_set_width(w.value, shells_[i].value_width);
      lv_obj_set_align(w.sensor_container, shells_[i].sensor_align);
      lv_obj_set_pos(w.sensor_container, shells_[i].sensor_x, shells_[i].sensor_y);
      shells_[i].icon_layout=label_layout(w.icon);shells_[i].label_layout=label_layout(w.label);
      restore_label(w.icon,shells_[i].icon_layout);restore_label(w.label,shells_[i].label_layout);
      lv_obj_add_flag(w.button,LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(w.sensor_container,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE));
      lv_obj_clear_flag(w.button,LV_OBJ_FLAG_CHECKABLE);lv_obj_clear_state(w.button,LV_STATE_CHECKED);
      for(const auto selector:{static_cast<lv_style_selector_t>(LV_PART_MAIN),
           (static_cast<lv_style_selector_t>(LV_PART_MAIN)|static_cast<lv_style_selector_t>(LV_STATE_CHECKED))}) {
        lv_obj_set_style_bg_color(w.button,lv_color_hex(theme_.surface),selector);
        lv_obj_set_style_text_color(w.button,lv_color_hex(theme_.text),selector);
        lv_obj_set_style_transition(w.button,page_basic_no_transition(),selector);
      }
      lv_obj_set_style_recolor_opa(w.button, LV_OPA_TRANSP,
                                   static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DISABLED);
      lv_obj_set_style_opa(w.button, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_set_grid_cell(w.button,LV_GRID_ALIGN_STRETCH,0,1,LV_GRID_ALIGN_STRETCH,0,1);
      lv_obj_set_style_grid_cell_row_pos(w.button,0,LV_PART_MAIN);
      lv_obj_set_style_grid_cell_column_span(w.button,1,LV_PART_MAIN);
      lv_obj_set_style_grid_cell_row_span(w.button,1,LV_PART_MAIN);
      lv_label_set_long_mode(w.label,LV_LABEL_LONG_WRAP);
      for(auto *label:{w.icon,w.label,w.value,w.unit}){
        lv_label_set_text_static(label,"");
        lv_obj_set_style_text_color(label,lv_color_hex(theme_.text),LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(theme_.disabled_text),
                                    static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DISABLED);
      }
      if(w.fill) {
        lv_obj_add_flag(w.fill,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_HIDDEN|LV_OBJ_FLAG_IGNORE_LAYOUT|LV_OBJ_FLAG_FLOATING));
        lv_obj_clear_flag(w.fill,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE));
        lv_obj_set_style_bg_opa(w.fill,LV_OPA_COVER,LV_PART_MAIN);
        lv_obj_set_style_border_width(w.fill,0,LV_PART_MAIN);lv_obj_set_style_shadow_width(w.fill,0,LV_PART_MAIN);
        lv_obj_set_style_pad_all(w.fill,0,LV_PART_MAIN);
        lv_obj_set_style_radius(w.fill,0,LV_PART_MAIN);lv_obj_set_style_bg_color(w.fill,lv_color_hex(theme_.active),LV_PART_MAIN);
        lv_obj_set_pos(w.fill,0,0);lv_obj_set_size(w.fill,0,0);lv_obj_move_background(w.fill);
        lv_obj_set_style_clip_corner(w.button,true,LV_PART_MAIN);
        for(auto *label:{w.icon,w.label,w.value,w.unit}) {
          lv_obj_add_flag(label,LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
          if(!lv_obj_add_event_cb(label,contrast_event,LV_EVENT_DRAW_TASK_ADDED,&shells_[i]))return false;
          ++shells_[i].contrast_callbacks;
        }
      }
      if(w.slider) {
        lv_slider_set_range(w.slider,0,100);lv_slider_set_value(w.slider,0,LV_ANIM_OFF);
        lv_obj_add_flag(w.slider,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_HIDDEN|LV_OBJ_FLAG_IGNORE_LAYOUT|LV_OBJ_FLAG_FLOATING));
        lv_obj_clear_flag(w.slider,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_EVENT_BUBBLE));
        for(const auto part:{LV_PART_MAIN,LV_PART_INDICATOR,LV_PART_KNOB}) {
          lv_obj_set_style_bg_opa(w.slider,LV_OPA_TRANSP,part);lv_obj_set_style_border_width(w.slider,0,part);
          lv_obj_set_style_shadow_width(w.slider,0,part);lv_obj_set_style_pad_all(w.slider,0,part);
          lv_obj_set_style_transition(w.slider,page_basic_no_transition(),part);
        }
        lv_obj_set_style_width(w.slider,0,LV_PART_KNOB);lv_obj_set_style_height(w.slider,0,LV_PART_KNOB);
        lv_obj_set_size(w.slider,0,0);lv_obj_move_to_index(w.slider,w.fill?1:0);
        if(!lv_obj_add_event_cb(w.slider,slider_guard,static_cast<lv_event_code_t>(LV_EVENT_ALL|LV_EVENT_PREPROCESS),&shells_[i]))return false;
        shells_[i].slider_guard_callback=true;
        if(!lv_obj_add_event_cb(w.slider,slider_event,LV_EVENT_ALL,&shells_[i]))return false;
        shells_[i].slider_callback=true;
      }
      if(w.forecast) {
        lv_obj_add_flag(w.forecast,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_HIDDEN|LV_OBJ_FLAG_IGNORE_LAYOUT));
        lv_obj_clear_flag(w.forecast,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE));
        lv_obj_set_style_bg_opa(w.forecast,LV_OPA_TRANSP,LV_PART_MAIN);lv_obj_set_style_border_width(w.forecast,0,LV_PART_MAIN);
        lv_obj_set_style_pad_all(w.forecast,0,LV_PART_MAIN);lv_obj_set_style_shadow_width(w.forecast,0,LV_PART_MAIN);
        lv_obj_set_size(w.forecast,lv_pct(100),lv_pct(100));lv_obj_align(w.forecast,LV_ALIGN_TOP_LEFT,0,0);
        lv_obj_set_style_clip_corner(w.button,true,LV_PART_MAIN);
        lv_obj_set_style_radius(w.forecast, 0, LV_PART_MAIN);
        lv_obj_set_style_clip_corner(w.forecast, false, LV_PART_MAIN);
        if (!lv_obj_add_event_cb(w.forecast, forecast_event, LV_EVENT_ALL, &shells_[i])) return false;
        shells_[i].forecast_callback=true;
      }
      if(theme_.sensor_font)lv_obj_set_style_text_font(w.value,theme_.sensor_font,LV_PART_MAIN);
      if(lv_obj_add_event_cb(w.button,event,LV_EVENT_ALL,&shells_[i])==nullptr)return false;
      ++callbacks_;
    }
    initialized_=true;return true;
  }
  // Preview an accepted panel action in its originating card. This neither
  // mutates HA state nor dispatches a service call; pending display lasts 1.5s.
  bool preview_volume(PageBindingToken token, uint8_t percent) {
    if (token.shell >= Slots || !same(shells_[token.shell].token, token) || !actions_.valid ||
        !actions_.valid(actions_.owner, token) || !media_)
      return false;
    auto& shell = shells_[token.shell];
    if (!shell.definition || shell.definition->definition.kind != BasicCardKind::MEDIA_CONTROL ||
        !shell.definition->definition.media.volume_number)
      return false;
    const auto& definition = shell.definition->definition;
    const ConfigurationHandle card{token.configuration, token.card};
    PageMediaSourceSnapshot source;
    const auto volume = read_media_volume(state_, card, definition.entity, definition.media.volume_max);
    if (!media_->source_snapshot(card, definition.entity, source) || !source.available ||
        volume.mode != media::VolumeControlMode::ABSOLUTE)
      return false;
    shell.pending_target = std::min(percent, definition.media.volume_max);
    shell.pending_ms = media_->now_ms();
    shell.gesture_connection = media_->connection_generation();
    media_layout_dirty_ = true;
    return true;
  }
  bool preview_group(PageBindingToken token, bool grouped) {
    if (token.shell >= Slots || !same(shells_[token.shell].token, token) || !actions_.valid ||
        !actions_.valid(actions_.owner, token) || !media_)
      return false;
    auto& shell = shells_[token.shell];
    if (!group_card(shell)) return false;
    const auto& definition = shell.definition->definition;
    const ConfigurationHandle card{token.configuration, token.card};
    PageMediaSourceSnapshot source;
    const auto primary = state_.read_attribute(card, definition.entity, "");
    const auto members = state_.read_attribute(card, definition.entity, "group_members");
    if (!media_->source_snapshot(card, definition.entity, source) || !source.available ||
        primary.status != PageEntityStatus::AVAILABLE ||
        !media_group_source_available(primary.value.substr(0, HA_SHORT_STATE_MAX_LEN)) ||
        members.entity != definition.entity || members.status == PageEntityStatus::EXHAUSTED)
      return false;
    shell.pending_target = grouped ? 1 : 0;
    shell.membership_revision = media_->group_revision(card, definition.entity);
    shell.gesture_connection = media_->connection_generation();
    media_layout_dirty_ = true;
    return true;
  }
  bool prepare(const PageStore& store, size_t count) override {
    discard_prepared();
    if (!initialized_ || count != Slots) return false;
    TextCounter strings;
    size_t summary_count = 0;
    bool has_weather = false, has_forecast = false, has_cleaning = false;
    size_t forecast_frames = 0;
    size_t number_capacity=48,label_capacity=std::max<size_t>(65,std::max({theme_.back_label.size(),theme_.empty_label.size(),theme_.folder_label.size()})+1),unit_capacity=1;
    // Back labels are decoded by the layout compiler; their source order size
    // is a conservative, profile-bounded upper bound on decoded text bytes.
    for(PageId page=0;page<store.page_count();++page) {
      const auto *p=store.page(page);if(!p->navigable)continue;
      label_capacity=std::max(label_capacity,store.string(p->order).size()+1);
      for(CardId card=p->first_card;card<p->first_card+p->card_count;++card) {
        BasicCardDefinition view;
        if (!prepare_basic_card(store, card, view, &strings, relay_)) return false;
        if (view.kind == BasicCardKind::COVER_COMMAND && !actions_.cover_command) return false;
        if (view.kind == BasicCardKind::CLEANING) {
          if (view.cleaning.mode != cleaning::Mode::STATUS && !actions_.cleaning) return false;
          has_cleaning |=
              view.cleaning.automatic_icon && cleaning::needs_state(view.cleaning) && !view.entity.empty();
          if (view.cleaning.mode == cleaning::Mode::STATUS)
            for (const auto* key : {"Cleaning", "Mowing", "Docked", "Error", "Idle", "Paused", "Returning",
                                    "Unavailable", "Unknown"}) {
              const auto* caption = theme_.translate ? theme_.translate(key) : nullptr;
              label_capacity = std::max(label_capacity, std::strlen(caption ? caption : key) + 1);
            }
        }
        if (view.kind == BasicCardKind::ACCESS) {
          if (!access_ || !actions_.access) return false;
          label_capacity = std::max<size_t>(label_capacity, 97);
        }
        if (view.kind == BasicCardKind::COVER_TOGGLE) {
          if (!cover_toggle_ || !actions_.toggle) return false;
          label_capacity = std::max<size_t>(label_capacity, 97);
        }
        if (view.kind == BasicCardKind::TIMER) {
          if (!timer_ || !actions_.timer) return false;
          const auto* confirm = theme_.translate ? theme_.translate("Confirm") : nullptr;
          label_capacity = std::max(label_capacity, std::strlen(confirm ? confirm : "Confirm") + 1);
        }
        if (view.kind == BasicCardKind::LIGHT_TEMPERATURE) {
          if (!temperature_ || !actions_.light_temperature) return false;
          for (const auto& w : widgets_)
            if (!w.fill || !w.slider) return false;
          label_capacity = std::max<size_t>(label_capacity, HA_FRIENDLY_NAME_MAX_LEN + 1);
        }
        if (view.kind == BasicCardKind::NUMBER_SLIDER) {
          if (!number_slider_ || !actions_.number_slider) return false;
          for (const auto& w : widgets_)
            if (!w.fill || !w.slider) return false;
          label_capacity = std::max<size_t>(label_capacity, HA_FRIENDLY_NAME_MAX_LEN + 1);
          unit_capacity = std::max<size_t>(unit_capacity, HA_SHORT_STATE_MAX_LEN + 1);
        }
        if (view.kind == BasicCardKind::PERCENT_SLIDER) {
          if (!percent_slider_ || !actions_.percent_slider) return false;
          for (const auto& w : widgets_)
            if (!w.fill || !w.slider) return false;
          label_capacity = std::max<size_t>(label_capacity, HA_FRIENDLY_NAME_MAX_LEN + 1);
        }
        if (view.kind == BasicCardKind::LOCAL_RELAY && (!relay_ || !actions_.toggle)) return false;
        if (view.kind == BasicCardKind::LOCAL_NUMBER || view.kind == BasicCardKind::LOCAL_TEXT) {
          if (!local_) return false;
          if (view.kind == BasicCardKind::LOCAL_TEXT) {
            if (local_->text_capacity() == SIZE_MAX) return false;
            label_capacity = std::max(label_capacity, local_->text_capacity() + 1);
          }
        }
        if(view.kind==BasicCardKind::WEATHER||view.kind==BasicCardKind::FORECAST)has_weather=true;
        if(view.kind==BasicCardKind::FORECAST) {
          has_forecast=true;if(!forecasts_||!actions_.forecast)return false;
          for(const auto &w:widgets_)if(!w.forecast)return false;
        }
        if(view.kind==BasicCardKind::CLIMATE&&actions_.climate==nullptr)return false;
        if(view.kind==BasicCardKind::ACTION&&actions_.action==nullptr)return false;
        if ((view.kind == BasicCardKind::FAN_SWITCH || view.kind == BasicCardKind::FAN_OSCILLATE)) {
          if (!fan_status_ || !actions_.action) return false;
          label_capacity = std::max<size_t>(label_capacity, HA_FRIENDLY_NAME_MAX_LEN + 1);
          for (const auto status :
               {PageFanStatus::UNAVAILABLE, PageFanStatus::ON, PageFanStatus::OFF, PageFanStatus::UNSUPPORTED,
                PageFanStatus::OSCILLATING, PageFanStatus::STILL}) {
            const auto* key = PageFanStatusRuntime::text(status);
            const auto* translated = theme_.translate ? theme_.translate(key) : nullptr;
            label_capacity = std::max(label_capacity, std::strlen(translated ? translated : key) + 1);
          }
        }
        if (view.kind == BasicCardKind::FAN_DIRECTION && !actions_.action) return false;
        if (view.kind == BasicCardKind::FAN_PRESET && !actions_.control) return false;
        if (view.kind == BasicCardKind::OPTION_SELECT) {
          if (!actions_.control) return false;
          number_capacity = std::max<size_t>(HA_STATE_TEXT_MAX_LEN + 1, number_capacity);
          label_capacity = std::max(label_capacity, view.entity.size() + 1);
        }
        if (view.kind == BasicCardKind::MEDIA_CONTROL &&
            (!media_ ||
             (view.media.kind == MediaActionKind::VOLUME ? !actions_.media_volume : !actions_.media_control)))
          return false;
        if (view.kind == BasicCardKind::MEDIA_COVER_ART) {
          if (!media_ || !artwork_ || !artwork_->ready() || !actions_.media_control ||
              !theme_.media_title_font || !theme_.media_title_font->line_height)
            return false;
          for (const auto& w : widgets_)
            if (!w.forecast) return false;
          const auto* idle = theme_.translate ? theme_.translate("Idle") : nullptr;
          label_capacity =
              std::max<size_t>(257, std::max(label_capacity, std::strlen(idle ? idle : "Idle") + 1));
          number_capacity = std::max<size_t>(number_capacity, 257);
        }
        if(view.kind==BasicCardKind::MEDIA_NOW_PLAYING) {
          if(!media_||!theme_.media_title_font||!theme_.media_title_font->line_height)return false;
          if(view.media.play_pause_background&&!actions_.media)return false;
          number_capacity=std::max<size_t>(number_capacity,257);label_capacity=std::max<size_t>(label_capacity,257);
        }
        if(view.kind==BasicCardKind::MEDIA_POSITION||(view.kind==BasicCardKind::MEDIA_NOW_PLAYING&&view.media.progress)) {
          if(!media_||!actions_.seek)return false;
          for(const auto &w:widgets_)if(!w.fill||!w.slider)return false;
        }
        if(view.kind==BasicCardKind::MEDIA_ACTION&&(actions_.media==nullptr||(view.media.kind==MediaActionKind::PLAYLIST&&!media_)))return false;
        if ((view.kind == BasicCardKind::LIGHT || view.kind == BasicCardKind::COVER ||
             view.kind == BasicCardKind::FAN_CONTROL) &&
            actions_.control == nullptr)
          return false;
        if(view.kind==BasicCardKind::COVER)for(const auto &w:widgets_)if(!w.fill)return false;
        if (((view.kind == BasicCardKind::MEDIA_ACTION || view.kind == BasicCardKind::MEDIA_POSITION ||
              view.kind == BasicCardKind::MEDIA_CONTROL) &&
             view.media.show_state) ||
            view.kind == BasicCardKind::TEXT || view.kind == BasicCardKind::CLIMATE ||
            view.kind == BasicCardKind::FAN_DIRECTION || view.kind == BasicCardKind::FAN_PRESET ||
            view.text_overlay || view.folder_mode == BasicFolderMode::TEXT ||
            view.action_display == BasicActionDisplay::TEXT)
          label_capacity = std::max<size_t>(label_capacity, 257);
        if(view.default_label) {
          const auto *translated=theme_.translate?theme_.translate(view.default_label):nullptr;
          label_capacity=std::max(label_capacity,std::strlen(translated?translated:view.default_label)+1);
        }
        label_capacity=std::max(label_capacity,view.label.size()+1);unit_capacity=std::max(unit_capacity,view.unit.size()+1);
      }
    }
    if(has_forecast)for(PageId page=0;page<store.page_count();++page) {
      if(!store.page(page)->navigable)continue;
      std::array<PageCellLayout,Slots>cells{};PageBackAppearance back;
      if(!compile_page_layout(store,page,geometry_,cells.data(),Slots,&back))return false;
      size_t count=0;for(const auto &cell:cells)if(cell.visible&&!cell.back&&basic_card_is_forecast(store,cell.card))++count;
      forecast_frames=std::max(forecast_frames,count);
    }
    for(PageId page=1;page<store.page_count();++page) {
      std::array<CardId,Slots> cards{};size_t used=0;
      if(!collect_summary(store,page,cards,used)||used>SIZE_MAX-summary_count)return false;
      summary_count+=used;
    }
    if(has_weather) {
      for(const auto &row:espcontrol::weather::CONDITION_ROWS) {
        const auto *translated=theme_.translate?theme_.translate(row.label):nullptr;
        label_capacity=std::max(label_capacity,std::strlen(translated?translated:row.label)+1);
      }
      for(const auto *key:{"Unknown","Unavailable"}) {
        const auto *translated=theme_.translate?theme_.translate(key):nullptr;
        label_capacity=std::max(label_capacity,std::strlen(translated?translated:key)+1);
      }
    }
    label_capacity=std::max(label_capacity,strings.largest+1);
    unit_capacity=std::max(unit_capacity,strings.unit_largest+1);
    unit_capacity=std::max(unit_capacity,theme_.temperature_unit.size()+1);
    const size_t records=store.card_count()*sizeof(Prepared);
    if(label_capacity>SIZE_MAX-unit_capacity-number_capacity)return false;
    const size_t stride=label_capacity+unit_capacity+number_capacity;
    if(stride>(SIZE_MAX-records)/Slots)return false;
    const size_t text_end=records+Slots*stride;if(strings.bytes>SIZE_MAX-text_end)return false;
    const size_t strings_end=text_end+strings.bytes;
    const size_t padding=(alignof(CardId)-strings_end%alignof(CardId))%alignof(CardId);
    if(padding>SIZE_MAX-strings_end)return false;
    const size_t summary_offset=strings_end+padding;
    if(summary_count>(SIZE_MAX-summary_offset)/sizeof(CardId))return false;
    size_t bytes=summary_offset+summary_count*sizeof(CardId),weather_offset=0;
    if(has_weather) {
      const auto padding=(alignof(WeatherGlyphs)-bytes%alignof(WeatherGlyphs))%alignof(WeatherGlyphs);
      if(padding>SIZE_MAX-bytes||sizeof(WeatherGlyphs)>SIZE_MAX-bytes-padding)return false;
      weather_offset=bytes+padding;bytes=weather_offset+sizeof(WeatherGlyphs);
    }
    size_t cleaning_offset = 0;
    if (has_cleaning) {
      const auto padding =
          (alignof(CleaningGlyphs) - bytes % alignof(CleaningGlyphs)) % alignof(CleaningGlyphs);
      if (padding > SIZE_MAX - bytes || sizeof(CleaningGlyphs) > SIZE_MAX - bytes - padding) return false;
      cleaning_offset = bytes + padding;
      bytes = cleaning_offset + sizeof(CleaningGlyphs);
    }
    size_t frame_offset=0;
    if(forecast_frames) {
      const auto padding=(alignof(PageForecastFrame)-bytes%alignof(PageForecastFrame))%alignof(PageForecastFrame);
      if(padding>SIZE_MAX-bytes||forecast_frames>(SIZE_MAX-bytes-padding)/sizeof(PageForecastFrame))return false;
      frame_offset=bytes+padding;bytes=frame_offset+forecast_frames*sizeof(PageForecastFrame);
    }
    void *memory=allocator_.allocate(bytes);if(memory==nullptr)return false;
    staged_={memory,bytes,label_capacity,unit_capacity,stride,store.card_count()};staged_.number_capacity=number_capacity;
    if (has_cleaning) {
      staged_.cleaning = new (static_cast<char*>(memory) + cleaning_offset) CleaningGlyphs{};
      for (size_t index = 0; index < static_cast<size_t>(cleaning::Glyph::COUNT); ++index) {
        const auto* glyph =
            actions_.icon(actions_.owner, cleaning::glyph_name(static_cast<cleaning::Glyph>(index)), {});
        if (!glyph) {
          discard_prepared();
          return false;
        }
        staged_.cleaning->glyphs[index] = glyph;
      }
    }
    if(forecast_frames) {
      staged_.frames=reinterpret_cast<PageForecastFrame *>(static_cast<char *>(memory)+frame_offset);staged_.frame_count=forecast_frames;
      for(size_t frame=0;frame<forecast_frames;++frame)new(staged_.frames+frame)PageForecastFrame{};
    }
    if(has_weather) {
      staged_.weather=new(static_cast<char *>(memory)+weather_offset)WeatherGlyphs{};
      for(size_t index=0;index<espcontrol::weather::CONDITION_COUNT;++index) {
        const auto *glyph=actions_.icon(actions_.owner,espcontrol::weather::CONDITION_ROWS[index].icon,{});
        if(!glyph){discard_prepared();return false;}staged_.weather->glyphs[index]=glyph;
      }
      if(has_forecast){staged_.weather->loading=actions_.icon(actions_.owner,"Weather Cloudy Clock",{});if(!staged_.weather->loading){discard_prepared();return false;}}

    }
    std::memset(static_cast<char *>(memory)+records,0,Slots*stride);
    memory_.peak_bytes=std::max(memory_.peak_bytes,active_.bytes+bytes);
    TextWriter writer(static_cast<char *>(memory)+text_end,strings.bytes);
    for(CardId card=0;card<store.card_count();++card)new(prepared(staged_)+card) Prepared{};
    for(PageId page=0;page<store.page_count();++page) {
      const auto *p=store.page(page);if(!p->navigable)continue;
      for(CardId card=p->first_card;card<p->first_card+p->card_count;++card) {
        auto& view = prepared(staged_)[card];
        if (!prepare_basic_card(store, card, view.definition, &writer, relay_)) {
          discard_prepared();
          return false;
        }
        const auto &d=view.definition;
        // Metadata titles reuse the icon label but never display a glyph.
        if (d.kind == BasicCardKind::TIMER || d.kind == BasicCardKind::MEDIA_NOW_PLAYING ||
            (d.kind == BasicCardKind::MEDIA_CONTROL && d.media.volume_number)) {
          view.off = view.on = "";
          continue;
        }
        if (d.kind == BasicCardKind::MEDIA_COVER_ART) {
          view.off = view.on = actions_.icon(actions_.owner, "Music", {});
          if (!view.off) {
            discard_prepared();
            return false;
          }
          continue;
        }
        auto domain=d.entity.substr(0,d.entity.find('.'));
        if(d.kind==BasicCardKind::ICON)domain={};
        const auto fallback=d.kind==BasicCardKind::CLIMATE?std::string_view("Thermostat"):d.kind==BasicCardKind::FOLDER&&d.entity.empty()?std::string_view("Folder"):std::string_view("Auto");
        view.off=actions_.icon(actions_.owner,d.icon.empty()||d.icon=="Auto"?fallback:d.icon,domain);
        view.on=d.icon_on.empty()||d.icon_on=="Auto"?view.off:actions_.icon(actions_.owner,d.icon_on,domain);
        if (d.kind == BasicCardKind::ACCESS && d.access.command && d.access.status) {
          const auto configured = store.field(*store.card(card), CardField::ICON);
          const auto initial = !configured.empty() && configured != "Auto" ? configured
                               : d.sensor == "open" ? (d.access.kind == BasicAccessKind::GATE
                                                           ? std::string_view("Gate Open")
                                                           : std::string_view("Garage Open"))
                               : d.sensor == "stop" ? std::string_view("Stop")
                                                    : d.icon;
          view.access_initial = actions_.icon(actions_.owner, initial, domain);
          if (!view.access_initial) {
            discard_prepared();
            return false;
          }
        }
        if(!view.off||!view.on){discard_prepared();return false;}
      }
    }
    if(writer.used!=strings.bytes){discard_prepared();return false;}
    size_t summary_used=0;auto *summary=reinterpret_cast<CardId *>(static_cast<char *>(memory)+summary_offset);
    for(PageId page=1;page<store.page_count();++page) {
      std::array<CardId,Slots> cards{};size_t used=0;
      if(!collect_summary(store,page,cards,used)||used>summary_count-summary_used){discard_prepared();return false;}
      if(used==0)continue;
      const auto owner=store.page(page)->owner;
      auto &parent=prepared(staged_)[owner];parent.summary=summary+summary_used;parent.summary_count=static_cast<uint16_t>(used);
      for(size_t index=0;index<used;++index)new(summary+summary_used+index) CardId(cards[index]);
      summary_used+=used;
    }
    if(summary_used!=summary_count){discard_prepared();return false;}
    return true;
  }
  void discard_prepared()override{release(staged_);}
  void activate_prepared(const PageStore &store)override {
    if(!staged_.arena)return;
    // PageRendererRuntime has invalidated/unbound old shells before this call.
    detach_text();release(active_);active_=staged_;staged_={};store_=store;
  }
  void retire_active()override {
    if(!active_.arena)return;
    for (auto& shell : shells_) {
      clear_cover(shell);
      cancel_position_gesture(shell);
      shell.token = {};
      shell.pressed = {};
      shell.definition = nullptr;
      restore_label(widgets_[shell.index].icon, shell.icon_layout);
      restore_label(widgets_[shell.index].label, shell.label_layout);
      shell.back = {};
      shell.cell = {};
      shell.label = shell.unit = shell.number = nullptr;
      shell.fill_percent = 0;
      shell.frame = nullptr;
      shell.dragging = false;
      shell.pending_target = -1;
    }
    detach_text();for(const auto &w:widgets_){lv_obj_add_flag(w.button,LV_OBJ_FLAG_HIDDEN);if(w.forecast)lv_obj_add_flag(w.forecast,LV_OBJ_FLAG_HIDDEN);}
    release(active_);store_={};media_layout_dirty_=false;
  }
  void unbind(size_t index,PageBindingToken)override {
    if(index>=Slots)return;auto &s=shells_[index];
    clear_cover(s);
    const bool feedback=s.definition&&s.definition->definition.push_feedback;
    cancel_position_gesture(s);
    s.token={};s.pressed={};s.definition=nullptr;s.back={};s.cell={};s.label=s.unit=s.number=nullptr;
    const auto &w=widgets_[index];lv_obj_add_flag(w.button,LV_OBJ_FLAG_HIDDEN);
    for(const auto selector:{static_cast<lv_style_selector_t>(LV_PART_MAIN),
         static_cast<lv_style_selector_t>(LV_PART_MAIN)|static_cast<lv_style_selector_t>(LV_STATE_CHECKED)})
      lv_obj_set_style_transition(w.button,page_basic_no_transition(),selector);
    // Writing the existing background property cancels LVGL's running colour
    // transition. A press animation must not repaint this shell after rebind.
    if(feedback&&s.painted)for(const auto selector:{static_cast<lv_style_selector_t>(LV_PART_MAIN),
         static_cast<lv_style_selector_t>(LV_PART_MAIN)|static_cast<lv_style_selector_t>(LV_STATE_CHECKED)})
      lv_obj_set_style_bg_color(w.button,lv_color_hex(s.background),selector);
    lv_obj_clear_state(w.button,static_cast<lv_state_t>(LV_STATE_CHECKED|LV_STATE_PRESSED));
    lv_obj_clear_flag(w.button,LV_OBJ_FLAG_CLICKABLE);
    s.dragging=false;s.pending_target=-1;if(w.slider){lv_obj_add_flag(w.slider,LV_OBJ_FLAG_HIDDEN);lv_obj_clear_flag(w.slider,LV_OBJ_FLAG_CLICKABLE);lv_slider_set_value(w.slider,0,LV_ANIM_OFF);}
    s.frame=nullptr;if(w.forecast)lv_obj_add_flag(w.forecast,LV_OBJ_FLAG_HIDDEN);
    s.fill_percent=0;if(w.fill){lv_obj_add_flag(w.fill,LV_OBJ_FLAG_HIDDEN);lv_obj_set_size(w.fill,0,0);}
    // Hidden static text only refers to this port's reusable buffers or
    // firmware glyphs; keep it attached until rebind or arena retirement.
  }
  void bind(size_t index,const PageStore &,const PageCellLayout &cell,PageBackStyleView back,PageBindingToken token)override {
    if(index>=Slots||!active_.arena||(!cell.back&&cell.card>=active_.cards))return;auto &s=shells_[index];const auto &w=widgets_[index];
    restore_label(w.icon,s.icon_layout);restore_label(w.label,s.label_layout);
    lv_obj_set_align(w.sensor_container, s.sensor_align);
    lv_obj_set_pos(w.sensor_container, s.sensor_x, s.sensor_y);
    lv_obj_set_width(w.sensor_container, s.sensor_width);
    lv_obj_set_width(w.value, s.value_width);
    if (lv_label_get_long_mode(w.value) != s.value_mode) lv_label_set_long_mode(w.value, s.value_mode);
    if (w.slider) lv_slider_set_range(w.slider, 0, 100);
    s.token=token;s.pressed={};s.cell=cell;s.back=back;
    s.definition=cell.back?nullptr:prepared(active_)+cell.card;
    if (timer_card(s)) s.pending_target = -1;
    if (percent_card(s) || number_card(s) || temperature_card(s)) {
      lv_obj_align(w.icon, LV_ALIGN_TOP_LEFT, 0, 0);
      lv_obj_align(w.label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
      if (temperature_card(s)) {
        lv_label_set_long_mode(w.icon, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(w.icon, lv_pct(100));
      }
    }
    if (s.definition && s.definition->definition.kind == BasicCardKind::CLEANING) {
      lv_label_set_long_mode(w.label, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(w.label, lv_pct(100));
      lv_obj_align(w.label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    }
    if (w.forecast) {
      lv_obj_set_style_radius(w.forecast, 0, LV_PART_MAIN);
      lv_obj_set_style_clip_corner(w.forecast, false, LV_PART_MAIN);
      lv_obj_set_size(w.forecast, lv_pct(100), lv_pct(100));
      lv_obj_align(w.forecast, LV_ALIGN_TOP_LEFT, 0, 0);
    }
    if (s.definition && s.definition->definition.kind == BasicCardKind::MEDIA_COVER_ART)
      artwork_->bind(token);
    s.frame=nullptr;
    if(s.definition&&s.definition->definition.kind==BasicCardKind::FORECAST) {
      for(size_t frame=0;frame<active_.frame_count&&!s.frame;++frame) {
        bool used=false;for(const auto &other:shells_)used|=other.frame==active_.frames+frame;
        if(!used)s.frame=active_.frames+frame;
      }
      assert(s.frame);s.frame->count=0;
    }
    auto *text=static_cast<char *>(active_.arena)+active_.cards*sizeof(Prepared)+index*active_.stride;
    s.label=text;s.unit=text+active_.label_capacity;s.number=s.unit+active_.unit_capacity;
    lv_obj_set_grid_cell(w.button,LV_GRID_ALIGN_STRETCH,cell.column,cell.column_span,LV_GRID_ALIGN_STRETCH,cell.row,cell.row_span);
    lv_obj_clear_flag(w.button,LV_OBJ_FLAG_HIDDEN);
    const bool clickable =
        cell.back || (s.definition && (s.definition->definition.kind == BasicCardKind::TOGGLE ||
                                       s.definition->definition.kind == BasicCardKind::LOCAL_RELAY ||
                                       s.definition->definition.kind == BasicCardKind::TIMER ||
                                       s.definition->definition.kind == BasicCardKind::COVER_COMMAND ||
                                       s.definition->definition.kind == BasicCardKind::COVER_TOGGLE ||
                                       s.definition->definition.kind == BasicCardKind::ACCESS ||
                                       (s.definition->definition.kind == BasicCardKind::CLEANING &&
                                        s.definition->definition.cleaning.mode != cleaning::Mode::STATUS) ||
                                       s.definition->definition.kind == BasicCardKind::FOLDER ||
                                       s.definition->definition.kind == BasicCardKind::CLIMATE ||
                                       s.definition->definition.kind == BasicCardKind::ACTION ||
                                       s.definition->definition.kind == BasicCardKind::LIGHT ||
                                       s.definition->definition.kind == BasicCardKind::FAN_CONTROL ||
                                       s.definition->definition.kind == BasicCardKind::FAN_SWITCH ||
                                       s.definition->definition.kind == BasicCardKind::FAN_OSCILLATE ||
                                       s.definition->definition.kind == BasicCardKind::FAN_DIRECTION ||
                                       s.definition->definition.kind == BasicCardKind::FAN_PRESET ||
                                       s.definition->definition.kind == BasicCardKind::OPTION_SELECT ||
                                       s.definition->definition.kind == BasicCardKind::COVER ||
                                       s.definition->definition.kind == BasicCardKind::FORECAST ||
                                       s.definition->definition.kind == BasicCardKind::MEDIA_ACTION ||
                                       s.definition->definition.kind == BasicCardKind::MEDIA_COVER_ART ||
                                       s.definition->definition.kind == BasicCardKind::MEDIA_CONTROL));
    if(clickable)lv_obj_add_flag(w.button,LV_OBJ_FLAG_CLICKABLE);else lv_obj_clear_flag(w.button,LV_OBJ_FLAG_CLICKABLE);
    // Do not read the new state graph here: its activation may follow ours.
  }
  void update(size_t index, PageBindingToken token) override {
    paint(index, token, true);
  }

 private:
  void paint(size_t index, PageBindingToken token, bool commit_seen) {
    if(index>=Slots||!same(shells_[index].token,token)||!active_.arena)return;
    auto &s=shells_[index];const auto &w=widgets_[index];
    BasicCardState value;BasicCardDefinition definition;
    const char *glyph="\xF3\xB0\x85\x81"; // MDI arrow-left, U+F0141
    bool number=false,checked=false;
    if(s.cell.back){value.label=s.back.custom_label.empty()?theme_.back_label:s.back.custom_label;definition.custom_off=s.back.custom_color;definition.off_color=s.back.background_color;}
    else {
      definition = s.definition->definition;
      value = read_basic_card(definition, {token.configuration, token.card}, state_,
                              s.cell.column_span > 1 ? 2 : 1, theme_.temperature_unit, theme_.translate,
                              media_, local_, relay_, fan_status_, percent_slider_, number_slider_,
                              temperature_, timer_, cover_toggle_, access_);
      if(group_card(s))group_feedback(s,value);
      if (timer_card(s)) timer_feedback(s, value);
      if(definition.folder_mode==BasicFolderMode::CHILDREN) {
        for(size_t child=0;child<s.definition->summary_count&&!value.on;++child) {
          const auto id=s.definition->summary[child];
          const auto& child_definition = prepared(active_)[id].definition;
          value.on =
              child_definition.kind == BasicCardKind::LOCAL_RELAY && relay_
                  ? relay_->on(token.configuration, child_definition.entity)
                  : read_basic_indicator(
                        {token.configuration, id},
                        child_definition.kind == BasicCardKind::ACCESS
                            ? child_definition.entity
                            : basic_folder_child_indicator(store_, id),
                        state_,
                        child_definition.kind == BasicCardKind::TIMER ? BasicFolderMode::TIMER
                        : child_definition.kind == BasicCardKind::CLEANING && child_definition.cleaning.mower
                            ? BasicFolderMode::MOWER
                            : BasicFolderMode::ENTITY);
        }
      }
      if(definition.kind==BasicCardKind::FOLDER&&definition.folder_mode==BasicFolderMode::NUMBER&&value.label.empty())value.label=theme_.folder_label;
      if(definition.kind==BasicCardKind::EMPTY&&value.label.empty())value.label=theme_.empty_label;
      checked=value.on&&(definition.kind==BasicCardKind::TOGGLE||definition.active_color);
      number = definition.kind == BasicCardKind::TIMER ||
               (definition.kind == BasicCardKind::NUMBER_SLIDER && value.numeric) ||
               (definition.kind == BasicCardKind::CLIMATE &&
                definition.climate.number != ClimateCardNumber::ICON) ||
               definition.kind == BasicCardKind::LOCAL_NUMBER ||
               definition.kind == BasicCardKind::OPTION_SELECT || definition.kind == BasicCardKind::NUMBER ||
               definition.kind == BasicCardKind::DURATION ||
               definition.kind == BasicCardKind::MEDIA_POSITION ||
               (definition.kind == BasicCardKind::MEDIA_CONTROL && definition.media.volume_number) ||
               definition.folder_mode == BasicFolderMode::NUMBER ||
               definition.action_display == BasicActionDisplay::NUMBER ||
               (definition.kind == BasicCardKind::TOGGLE && !definition.text_overlay &&
                !definition.sensor.empty() && value.on);
      const bool state_icon =
          definition.kind == BasicCardKind::TOGGLE || definition.kind == BasicCardKind::LOCAL_RELAY ||
          definition.kind == BasicCardKind::FAN_SWITCH || definition.kind == BasicCardKind::ICON ||
          definition.kind == BasicCardKind::STATUS || definition.kind == BasicCardKind::FAN_CONTROL ||
          definition.kind == BasicCardKind::LIGHT || definition.kind == BasicCardKind::COVER ||
          definition.action_display == BasicActionDisplay::ICON ||
          definition.folder_mode == BasicFolderMode::ENTITY ||
          definition.folder_mode == BasicFolderMode::CLIMATE ||
          definition.folder_mode == BasicFolderMode::MOWER ||
          definition.folder_mode == BasicFolderMode::CHILDREN;
      if (definition.kind == BasicCardKind::CLEANING && definition.cleaning.automatic_icon &&
          value.cleaning.state_seen && cleaning::needs_state(definition.cleaning))
        glyph = active_.cleaning->glyphs[static_cast<size_t>(value.cleaning.glyph)];
      else if (definition.kind == BasicCardKind::WEATHER)
        glyph = active_.weather->glyphs[value.weather_condition];
      else
        glyph = definition.kind == BasicCardKind::ACCESS && definition.access.command &&
                        definition.access.status && !value.access.history.reported
                    ? s.definition->access_initial
                : definition.kind == BasicCardKind::ACCESS
                    ? (value.cover_icon_open ? s.definition->on : s.definition->off)
                : definition.kind == BasicCardKind::COVER_TOGGLE
                    ? (value.cover_icon_open ? s.definition->on : s.definition->off)
                : definition.kind == BasicCardKind::PERCENT_SLIDER
                    ? (value.percent_slider.icon_on ? s.definition->on : s.definition->off)
                : definition.kind == BasicCardKind::CLIMATE
                    ? (value.climate_icon_on ? s.definition->on : s.definition->off)
                : state_icon && value.on ? s.definition->on
                                         : s.definition->off;
    }
    const bool disabled = definition.kind == BasicCardKind::ACCESS &&
                          definition.access.kind == BasicAccessKind::GATE && !value.access.enabled;
    for (auto* object : {w.button, w.icon, w.label, w.value, w.unit}) {
      if (disabled)
        lv_obj_add_state(object, LV_STATE_DISABLED);
      else
        lv_obj_clear_state(object, LV_STATE_DISABLED);
    }
    for (auto* label : {w.icon, w.label, w.value, w.unit})
      lv_obj_set_style_text_color(label, lv_color_hex(theme_.disabled_text),
                                  static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DISABLED);
    if (definition.kind == BasicCardKind::ACCESS) {
      if (disabled)
        lv_obj_clear_flag(w.button, LV_OBJ_FLAG_CLICKABLE);
      else
        lv_obj_add_flag(w.button, LV_OBJ_FLAG_CLICKABLE);
    }
    const bool now_playing = media_text(definition.kind);
    if (definition.kind == BasicCardKind::MEDIA_CONTROL && definition.media.volume_number) {
      volume_feedback(s, value);
      lv_obj_align(w.sensor_container, LV_ALIGN_TOP_LEFT, 0, 0);
    }
    if(definition.kind==BasicCardKind::MEDIA_POSITION||(now_playing&&definition.media.progress))position_feedback(s,value);
    if (definition.kind == BasicCardKind::PERCENT_SLIDER) percent_feedback(s, value);
    if (definition.kind == BasicCardKind::NUMBER_SLIDER) number_feedback(s, value);
    if (definition.kind == BasicCardKind::LIGHT_TEMPERATURE) temperature_feedback(s, value);
    if(!now_playing){lv_obj_clear_flag(w.label,LV_OBJ_FLAG_HIDDEN);write(w.label,s.label,active_.label_capacity,value.formatted_label?std::string_view(value.text):value.label);}
    for(const auto selector:{static_cast<lv_style_selector_t>(LV_PART_MAIN),
         static_cast<lv_style_selector_t>(LV_PART_MAIN)|static_cast<lv_style_selector_t>(LV_STATE_CHECKED)})
      lv_obj_set_style_transition(w.button,definition.push_feedback?page_basic_push_transition():page_basic_no_transition(),selector);
    write(w.unit,s.unit,active_.unit_capacity,value.unit);
    if(now_playing){const char* empty="";if(lv_label_get_text(w.value)!=empty)lv_label_set_text_static(w.value,empty);}
    else {
      write(w.value, s.number, active_.number_capacity,
            value.numeric ? std::string_view(definition.kind == BasicCardKind::OPTION_SELECT ? value.text
                                                                                             : value.number)
                          : std::string_view{});
      if(std::strcmp(lv_label_get_text(w.icon),glyph)!=0)lv_label_set_text_static(w.icon,glyph);
      if(theme_.icon_font)lv_obj_set_style_text_font(w.icon,theme_.icon_font,LV_PART_MAIN);
      if(theme_.label_font)lv_obj_set_style_text_font(w.label,theme_.label_font,LV_PART_MAIN);
    }
    if(number){lv_obj_add_flag(w.icon,LV_OBJ_FLAG_HIDDEN);lv_obj_clear_flag(w.sensor_container,LV_OBJ_FLAG_HIDDEN);}
    else {lv_obj_clear_flag(w.icon,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(w.sensor_container,LV_OBJ_FLAG_HIDDEN);}
    const bool option = definition.kind == BasicCardKind::OPTION_SELECT;
    if (option) {
      lv_obj_set_width(w.sensor_container, lv_pct(100));
      lv_obj_set_width(w.value, lv_pct(100));
      if (lv_label_get_long_mode(w.value) != LV_LABEL_LONG_DOT)
        lv_label_set_long_mode(w.value, LV_LABEL_LONG_DOT);
    }
    const auto font = option ? (theme_.option_select_font ? theme_.option_select_font
                                : theme_.media_title_font ? theme_.media_title_font
                                                          : s.label_layout.font)
                      : definition.large_numbers && s.cell.column_span > 1 && s.cell.row_span > 1 &&
                              theme_.large_sensor_font
                          ? theme_.large_sensor_font
                      : theme_.sensor_font ? theme_.sensor_font
                                           : s.value_font;
    if(font&&s.painted_font!=font){lv_obj_set_style_text_font(w.value,font,LV_PART_MAIN);s.painted_font=font;}
    if(checked!=lv_obj_has_state(w.button,LV_STATE_CHECKED)){if(checked)lv_obj_add_state(w.button,LV_STATE_CHECKED);else lv_obj_clear_state(w.button,LV_STATE_CHECKED);}
    const auto correct=[this](uint32_t raw){return correct_display_color(raw,theme_.red_percent,theme_.green_percent,theme_.blue_percent);};
    uint32_t background =
        definition.custom_off ? correct(definition.off_color)
        : (definition.kind == BasicCardKind::MEDIA_POSITION || (now_playing && definition.media.progress))
            ? theme_.media_surface
        : (definition.kind == BasicCardKind::LOCAL_NUMBER || definition.kind == BasicCardKind::LOCAL_TEXT ||
           definition.kind == BasicCardKind::OPTION_SELECT || definition.kind == BasicCardKind::NUMBER ||
           definition.kind == BasicCardKind::TEXT || definition.kind == BasicCardKind::DURATION ||
           definition.kind == BasicCardKind::STATUS || definition.kind == BasicCardKind::WEATHER ||
           definition.kind == BasicCardKind::FORECAST)
            ? theme_.sensor_surface
            : theme_.surface;
    if(checked)background=definition.custom_on?correct(definition.on_color):definition.custom_off&&!definition.theme_active?correct(active_card_color(definition.off_color)):theme_.active;
    const auto foreground=checked||definition.custom_off?display_text_color_for_bg(background):theme_.text;
    if(!s.painted||s.background!=background||s.foreground!=foreground) {
      for(const auto selector:{static_cast<lv_style_selector_t>(LV_PART_MAIN),(static_cast<lv_style_selector_t>(LV_PART_MAIN)|static_cast<lv_style_selector_t>(LV_STATE_CHECKED))}) {
        lv_obj_set_style_bg_color(w.button,lv_color_hex(background),selector);
        lv_obj_set_style_text_color(w.button,lv_color_hex(foreground),selector);
      }
      for(auto *label:{w.icon,w.label,w.value,w.unit})lv_obj_set_style_text_color(label,lv_color_hex(foreground),LV_PART_MAIN);
      s.painted=true;s.background=background;s.foreground=foreground;
    }
    if(w.fill) {
      if (definition.kind == BasicCardKind::PERCENT_SLIDER ||
          definition.kind == BasicCardKind::NUMBER_SLIDER ||
          definition.kind == BasicCardKind::LIGHT_TEMPERATURE ||
          definition.kind == BasicCardKind::MEDIA_POSITION || (now_playing && definition.media.progress) ||
          (definition.kind == BasicCardKind::COVER && value.fill_percent)) {
        s.fill_percent=value.fill_percent;update_fill_geometry(s);
        const auto fill_color =
            definition.kind == BasicCardKind::LIGHT_TEMPERATURE && value.light_temperature.color
                ? light_temperature::fill_color(value.light_temperature.fill_kelvin)
            : (definition.kind == BasicCardKind::MEDIA_POSITION || (now_playing && definition.media.progress))
                ? (definition.custom_off ? correct(card_progress_color(definition.off_color))
                                         : theme_.surface)
            : definition.custom_on ? correct(definition.on_color)
            : definition.custom_off && !definition.theme_active
                ? ((definition.kind == BasicCardKind::PERCENT_SLIDER ||
                    definition.kind == BasicCardKind::NUMBER_SLIDER ||
                    definition.kind == BasicCardKind::LIGHT_TEMPERATURE)
                       ? card_slider_fill_color(correct(definition.off_color))
                       : correct(active_card_color(definition.off_color)))
                : theme_.active;
        lv_obj_set_style_bg_color(w.fill,lv_color_hex(fill_color),LV_PART_MAIN);lv_obj_clear_flag(w.fill,LV_OBJ_FLAG_HIDDEN);
      } else {
        s.fill_percent = 0;
        lv_obj_add_flag(w.fill, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(w.fill, 0, 0);
      }
    }
    if(now_playing)media_layout_dirty_=true;
    if(definition.kind==BasicCardKind::FORECAST)update_forecast(s);
    else if (w.forecast && definition.kind != BasicCardKind::MEDIA_COVER_ART)
      lv_obj_add_flag(w.forecast, LV_OBJ_FLAG_HIDDEN);
    if (!commit_seen) return;
    if (number_slider_) seen_number_slider_ = number_slider_->revision();
    if (temperature_) seen_temperature_ = temperature_->revision();
    if (timer_) seen_timer_ = timer_->revision();
    if (cover_toggle_) seen_cover_toggle_ = cover_toggle_->revision();
    if (access_) seen_access_ = access_->revision();
    seen_state_ = state_.revision();
    if (local_) seen_local_ = local_->revision();
    if (relay_) seen_relay_ = relay_->revision();
    if (fan_status_) seen_fan_status_ = fan_status_->revision();
    if (percent_slider_) seen_percent_slider_ = percent_slider_->revision();
    seen_theme_ = theme_.revision;
    seen_forecast_ = forecasts_ ? forecasts_->revision() : 0;
    seen_media_ = media_ ? media_->revision() : 0;
    seen_artwork_ = artwork_ ? artwork_->revision() : 0;
  }

 public:
  void finish_update() override {
    bool any = false;
    for (const auto& shell : shells_)
      any |= shell.definition && media_text(shell.definition->definition.kind);
    if (!any) {
      media_layout_dirty_ = false;
      return;
    }
    if (media_finishing_) return;
    media_finishing_ = true;
    media_layout_dirty_ = false;
    // Resolve the grid once, then arrange all media labels and resolve their
    // measured text boxes together. No shell forces a full-tree layout pass.
    lv_obj_update_layout(widgets_[0].button);
    for (auto& shell : shells_)
      if (shell.definition && media_text(shell.definition->definition.kind)) {
        const auto value =
            read_basic_card(shell.definition->definition, {shell.token.configuration, shell.token.card},
                            state_, 2, theme_.temperature_unit, theme_.translate, media_);
        prepare_now_playing_text(shell, value);
      }
    lv_obj_update_layout(widgets_[0].button);
    for (auto& shell : shells_)
      if (shell.definition && media_text(shell.definition->definition.kind)) {
        const auto& w = widgets_[shell.index];
        if (shell.media_idle) continue;
        // Native DOTS retains the full text's measured height after truncation.
        lv_obj_set_height(w.icon, lv_obj_get_height(w.icon));
        write_media_label(w.icon, shell.number, active_.number_capacity, shell.number,
                          media_title_lines(shell), true);
        if (shell.definition->definition.kind == BasicCardKind::MEDIA_COVER_ART && shell.cell.row_span >= 2 &&
            shell.cell.column_span >= 2)
          lv_obj_align_to(w.label, w.icon, LV_ALIGN_OUT_BOTTOM_LEFT, 0,
                          std::max<int32_t>(0, lv_obj_get_style_pad_top(w.button, LV_PART_MAIN) / 2));
        if (!lv_obj_has_flag(w.label, LV_OBJ_FLAG_HIDDEN))
          write_media_label(w.label, shell.label, active_.label_capacity, shell.label, 1, true);
      }
    media_finishing_ = false;
  }
  void suspend() override {
    for (auto& shell : shells_) {
      clear_cover(shell);
      cancel_position_gesture(shell);
      shell.pending_target = -1;
    }
  }
  bool state_changed() const override {
    if (local_) local_->loop();
    if (fan_status_) fan_status_->loop();
    if (percent_slider_) percent_slider_->loop();
    if (number_slider_) number_slider_->loop();
    if (temperature_) temperature_->loop();
    if (timer_) timer_->loop();
    if (cover_toggle_) cover_toggle_->loop();
    if (access_) access_->loop();
    if (media_layout_dirty_) return true;
    if (media_)
      for (const auto& shell : shells_)
        if (shell.pending_target >= 0 && !timer_card(shell) && !group_card(shell) &&
            static_cast<uint32_t>(media_->now_ms() - shell.pending_ms) >= pending_timeout(shell))
          return true;
    return (access_ && access_->revision() != seen_access_) ||
           (cover_toggle_ && cover_toggle_->revision() != seen_cover_toggle_) ||
           (timer_ && timer_->revision() != seen_timer_) ||
           (temperature_ && temperature_->revision() != seen_temperature_) ||
           (number_slider_ && number_slider_->revision() != seen_number_slider_) ||
           (percent_slider_ && percent_slider_->revision() != seen_percent_slider_) ||
           (fan_status_ && fan_status_->revision() != seen_fan_status_) ||
           (relay_ && relay_->revision() != seen_relay_) || (local_ && local_->revision() != seen_local_) ||
           state_.revision() != seen_state_ || theme_.revision != seen_theme_ ||
           (forecasts_ && forecasts_->revision() != seen_forecast_) ||
           (media_ && media_->revision() != seen_media_) ||
           (artwork_ && artwork_->revision() != seen_artwork_);
  }
  PageBasicMemory memory()const{auto m=memory_;m.fixed_bytes=sizeof(*this);m.borrowed_widgets=Slots*6;for(const auto &w:widgets_)m.borrowed_widgets+=(w.fill?1:0)+(w.forecast?1:0)+(w.slider?1:0);m.forecast_frames=active_.frame_count;m.active_bytes=active_.bytes;m.staged_bytes=staged_.bytes;return m;}
 private:
  bool collect_summary(const PageStore &store,PageId page_id,std::array<CardId,Slots> &cards,size_t &used)const {
    used=0;const auto *page=store.page(page_id);if(!page||!page->navigable)return true;
    const auto *owner=store.card(page->owner);if(!owner)return false;
    if(!store.field(*owner,CardField::ENTITY).empty()||store.field(*owner,CardField::SENSOR)!="indicator")return true;
    std::array<PageCellLayout,Slots>cells{};PageBackAppearance back;
    if(!compile_page_layout(store,page_id,geometry_,cells.data(),Slots,&back))return false;
    for(const auto &cell:cells) {
      if(!cell.visible||cell.back||basic_folder_child_indicator(store,cell.card).empty())continue;
      bool duplicate=false;for(size_t previous=0;previous<used;++previous)duplicate|=cards[previous]==cell.card;
      if(!duplicate)cards[used++]=cell.card;
    }
    return true;
  }
  struct TextCounter final:BasicCardTextSink {
    bool copy(std::string_view value,std::string_view &out)override {
      largest=std::max(largest,value.size());return count(value,out);
    }
    bool copy_unit(std::string_view value,std::string_view &out)override{unit_largest=std::max(unit_largest,value.size());return count(value,out);}
    bool count(std::string_view value,std::string_view &out){if(value.size()>SIZE_MAX-bytes)return false;bytes+=value.size();out={};return true;}
    size_t bytes{0},largest{0},unit_largest{0};
  };
  struct TextWriter final:BasicCardTextSink {
    TextWriter(char *memory,size_t capacity):memory(memory),capacity(capacity){}
    bool copy(std::string_view value,std::string_view &out)override {
      if(value.size()>capacity-used)return false;
      if(!value.empty())std::memcpy(memory+used,value.data(),value.size());out={memory+used,value.size()};used+=value.size();return true;
    }
    char *memory;size_t capacity,used{0};
  };
  struct Prepared {
    BasicCardDefinition definition;
    const char *off{nullptr}, *on{nullptr};
    // Folder summaries and access-command initial glyphs are mutually exclusive.
    union {
      const CardId* summary{nullptr};
      const char* access_initial;
    };
    uint16_t summary_count{0};
  };
  struct CleaningGlyphs {
    std::array<const char*, static_cast<size_t>(cleaning::Glyph::COUNT)> glyphs{};
  };
  struct WeatherGlyphs {std::array<const char *,espcontrol::weather::CONDITION_COUNT>glyphs{};const char *loading{nullptr};};
  struct Arena {
    void* arena{nullptr};
    size_t bytes{0}, label_capacity{0}, unit_capacity{0}, stride{0}, cards{0};
    WeatherGlyphs* weather{nullptr};
    PageForecastFrame* frames{nullptr};
    size_t frame_count{0}, number_capacity{48};
    CleaningGlyphs* cleaning{nullptr};
  };
  struct LabelLayout {
    const lv_font_t* font{nullptr};
    lv_coord_t x{0}, y{0}, width{0}, height{0}, maximum{0}, line_space{0}, letter_space{0}, scale_x{256},
        scale_y{256};
    lv_coord_t left{0}, top{0}, right{0}, bottom{0};
    lv_align_t align{LV_ALIGN_DEFAULT};
    lv_text_align_t text_align{LV_TEXT_ALIGN_AUTO};
    lv_label_long_mode_t mode{LV_LABEL_LONG_WRAP};
  };
  static LabelLayout label_layout(lv_obj_t* label) {
    return {lv_obj_get_style_text_font(label, LV_PART_MAIN),
            lv_obj_get_style_x(label, LV_PART_MAIN),
            lv_obj_get_style_y(label, LV_PART_MAIN),
            lv_obj_get_style_width(label, LV_PART_MAIN),
            lv_obj_get_style_height(label, LV_PART_MAIN),
            lv_obj_get_style_max_height(label, LV_PART_MAIN),
            lv_obj_get_style_text_line_space(label, LV_PART_MAIN),
            lv_obj_get_style_text_letter_space(label, LV_PART_MAIN),
            lv_obj_get_style_transform_scale_x(label, LV_PART_MAIN),
            lv_obj_get_style_transform_scale_y(label, LV_PART_MAIN),
            lv_obj_get_style_pad_left(label, LV_PART_MAIN),
            lv_obj_get_style_pad_top(label, LV_PART_MAIN),
            lv_obj_get_style_pad_right(label, LV_PART_MAIN),
            lv_obj_get_style_pad_bottom(label, LV_PART_MAIN),
            lv_obj_get_style_align(label, LV_PART_MAIN),
            lv_obj_get_style_text_align(label, LV_PART_MAIN),
            lv_label_get_long_mode(label)};
  }
  static void restore_label(lv_obj_t* label, const LabelLayout& layout) {
    lv_obj_set_style_text_font(label, layout.font, LV_PART_MAIN);
    lv_obj_set_align(label, layout.align);
    lv_obj_set_pos(label, layout.x, layout.y);
    lv_obj_set_size(label, layout.width, layout.height);
    lv_obj_set_style_max_height(label, layout.maximum, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(label, layout.line_space, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(label, layout.letter_space, LV_PART_MAIN);
    lv_obj_set_style_text_align(label, layout.text_align, LV_PART_MAIN);
    lv_obj_set_style_transform_scale_x(label, layout.scale_x, LV_PART_MAIN);
    lv_obj_set_style_transform_scale_y(label, layout.scale_y, LV_PART_MAIN);
    lv_obj_set_style_pad_left(label, layout.left, LV_PART_MAIN);
    lv_obj_set_style_pad_top(label, layout.top, LV_PART_MAIN);
    lv_obj_set_style_pad_right(label, layout.right, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(label, layout.bottom, LV_PART_MAIN);
    if (lv_label_get_long_mode(label) != layout.mode) lv_label_set_long_mode(label, layout.mode);
  }
  struct Shell {
    PageBasicLvglPort* port{nullptr};
    LabelLayout icon_layout, label_layout;
    lv_align_t sensor_align{LV_ALIGN_DEFAULT};
    lv_coord_t sensor_x{0}, sensor_y{0}, sensor_width{0}, value_width{0};
    lv_label_long_mode_t value_mode{LV_LABEL_LONG_WRAP};
    const lv_font_t* value_font{nullptr};
    size_t index{0};
    PageBindingToken token, pressed;
    PageCellLayout cell;
    PageBackStyleView back;
    Prepared* definition{nullptr};
    char *label{nullptr}, *unit{nullptr}, *number{nullptr};
    bool painted{false};
    uint8_t fill_percent{0}, contrast_callbacks{0};
    bool forecast_callback{false}, slider_callback{false}, slider_guard_callback{false}, dragging{false};
    // Only the active card mode uses this storage: timed numeric feedback or
    // a confirmed group state waiting for its next membership report.
    union {
      uint32_t pending_ms{0};
      uint32_t membership_revision;
    };
    uint32_t gesture_connection{0}, timer_state_revision{0};
    float pending_target{-1};
    lv_indev_t* input{nullptr};
    PageForecastFrame* frame{nullptr};
    PageCoverArtFrame cover;
    uint64_t cover_asset{0};
    bool media_idle{false};
    uint32_t background{0}, foreground{0};
    const lv_font_t* painted_font{nullptr};
  };
  static bool same(PageBindingToken a,PageBindingToken b){return a.binding!=0&&a.configuration==b.configuration&&a.binding==b.binding&&a.page==b.page&&a.card==b.card&&a.shell==b.shell&&a.back==b.back;}
  static Prepared *prepared(const Arena &arena){return static_cast<Prepared *>(arena.arena);}
  void release(Arena &arena){if(arena.arena)allocator_.release(arena.arena,arena.bytes);arena={};}
  void detach_text(){for(const auto &w:widgets_)if(w.button)for(auto *label:{w.icon,w.label,w.value,w.unit})if(label)lv_label_set_text_static(label,"");}
  void write(lv_obj_t *label,char *buffer,size_t capacity,std::string_view text) {
    bool changed=false;const auto result=copy_display_text(buffer,capacity,text,SIZE_MAX,&changed);memory_.text_truncations+=result.truncated;memory_.invalid_text+=result.invalid;
    if(changed||lv_label_get_text(label)!=buffer)lv_label_set_text_static(label,buffer);
  }
  static void contrast_event(lv_event_t* event) {
    auto* shell = static_cast<Shell*>(lv_event_get_user_data(event));
    if (!shell || !shell->definition) return;
    const auto kind=shell->definition->definition.kind;
    if (kind != BasicCardKind::COVER && kind != BasicCardKind::PERCENT_SLIDER &&
        kind != BasicCardKind::NUMBER_SLIDER && kind != BasicCardKind::LIGHT_TEMPERATURE)
      return;
    slider_text_contrast_event(event, shell->port->widgets_[shell->index].fill);
  }
  void cancel_position_gesture(Shell& shell) {
    auto* input = shell.input;
    const bool dragging = shell.dragging;
    if (dragging && number_card(shell) && number_slider_)
      number_slider_->cancel({shell.token.configuration, shell.token.card});
    if (dragging && temperature_card(shell) && temperature_)
      temperature_->cancel({shell.token.configuration, shell.token.card});
    shell.input = nullptr;
    shell.dragging = false;
    shell.pressed = {};
    if (dragging && input) {
      lv_indev_wait_release(input);
      lv_indev_reset(input, widgets_[shell.index].slider);
    }
    if (widgets_[shell.index].slider) lv_obj_clear_state(widgets_[shell.index].slider, LV_STATE_PRESSED);
  }
  static bool seek_card(const Shell& shell) {
    return shell.definition&&(shell.definition->definition.kind==BasicCardKind::MEDIA_POSITION||
      (shell.definition->definition.kind==BasicCardKind::MEDIA_NOW_PLAYING&&shell.definition->definition.media.progress));
  }
  static bool percent_card(const Shell& shell) {
    return shell.definition && shell.definition->definition.kind == BasicCardKind::PERCENT_SLIDER;
  }
  static bool number_card(const Shell& shell) {
    return shell.definition && shell.definition->definition.kind == BasicCardKind::NUMBER_SLIDER;
  }
  static bool timer_card(const Shell& shell) {
    return shell.definition && shell.definition->definition.kind == BasicCardKind::TIMER;
  }
  PageTimerSnapshot timer_snapshot(const Shell& shell) const {
    timer_->loop();
    return read_basic_timer(shell.definition->definition, {shell.token.configuration, shell.token.card},
                            state_, timer_);
  }
  bool timer_confirmation_valid(const Shell& shell, const PageTimerSnapshot& snapshot) const {
    return shell.pending_target >= 0 && snapshot.mode == PageTimerMode::ACTIVE &&
           shell.gesture_connection == timer_->connection_generation() &&
           shell.timer_state_revision == snapshot.confirmation_revision &&
           static_cast<uint32_t>(timer_->now_ms() - shell.pending_ms) <
               static_cast<uint32_t>(snapshot.confirmation_seconds) * 1000;
  }
  void timer_feedback(Shell& shell, BasicCardState& value) {
    if (!timer_confirmation_valid(shell, value.timer)) shell.pending_target = -1;
    if (shell.pending_target >= 0) {
      const auto* confirm = theme_.translate ? theme_.translate("Confirm") : nullptr;
      value.label = confirm ? confirm : "Confirm";
    }
    if (value.timer.mode == PageTimerMode::UNAVAILABLE)
      lv_obj_clear_flag(widgets_[shell.index].button, LV_OBJ_FLAG_CLICKABLE);
    else
      lv_obj_add_flag(widgets_[shell.index].button, LV_OBJ_FLAG_CLICKABLE);
  }
  void timer_click(Shell& shell, PageBindingToken origin) {
    const auto snapshot = timer_snapshot(shell);
    if (shell.gesture_connection != timer_->connection_generation() ||
        snapshot.mode == PageTimerMode::UNAVAILABLE) {
      shell.pending_target = -1;
      paint(shell.index, shell.token, false);
      return;
    }
    const auto action =
        snapshot.mode == PageTimerMode::ACTIVE ? PageTimerAction::CANCEL : PageTimerAction::START;
    if (action == PageTimerAction::CANCEL && snapshot.confirmation &&
        !timer_confirmation_valid(shell, snapshot)) {
      shell.pending_target = 0;
      shell.pending_ms = timer_->now_ms();
      shell.timer_state_revision = snapshot.confirmation_revision;
      paint(shell.index, shell.token, false);
      return;
    }
    const auto connection = shell.gesture_connection;
    shell.pending_target = -1;
    paint(shell.index, shell.token, false);
    const bool accepted = actions_.timer && actions_.timer(actions_.owner, origin, action);
    if (action == PageTimerAction::CANCEL && connection == timer_->connection_generation())
      timer_->cancel_sent({origin.configuration, origin.card}, accepted);
  }
  static bool temperature_card(const Shell& shell) {
    return shell.definition && shell.definition->definition.kind == BasicCardKind::LIGHT_TEMPERATURE;
  }
  PageLightTemperatureSnapshot temperature_snapshot(const Shell& shell) const {
    temperature_->loop();
    return read_basic_light_temperature(shell.definition->definition,
                                        {shell.token.configuration, shell.token.card}, state_, temperature_);
  }
  void temperature_feedback(Shell& shell, BasicCardState& value) {
    const auto& w = widgets_[shell.index];
    const auto& snapshot = value.light_temperature;
    if (shell.dragging && (!snapshot.gesture || !snapshot.available ||
                           shell.gesture_connection != temperature_->connection_generation()))
      cancel_position_gesture(shell);
    lv_obj_clear_flag(w.slider, LV_OBJ_FLAG_HIDDEN);
    if (snapshot.available)
      lv_obj_add_flag(w.slider, LV_OBJ_FLAG_CLICKABLE);
    else
      lv_obj_clear_flag(w.slider, LV_OBJ_FLAG_CLICKABLE);
    lv_slider_set_value(w.slider, snapshot.percent, LV_ANIM_OFF);
  }
  void temperature_pointer(Shell& shell) {
    if (!shell.dragging) return;
    const auto snapshot = temperature_snapshot(shell);
    if (!snapshot.gesture || !snapshot.available ||
        shell.gesture_connection != temperature_->connection_generation()) {
      cancel_position_gesture(shell);
      return;
    }
    percent_pointer(shell);
    temperature_->drag({shell.token.configuration, shell.token.card},
                       lv_slider_get_value(widgets_[shell.index].slider));
  }
  static void temperature_slider_guard(lv_event_t* event) {
    auto* shell = static_cast<Shell*>(lv_event_get_user_data(event));
    auto* port = shell->port;
    const auto code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED) {
      if (!port->temperature_snapshot(*shell).available || !port->actions_.valid ||
          !port->actions_.valid(port->actions_.owner, shell->token) ||
          !port->temperature_->begin({shell->token.configuration, shell->token.card})) {
        lv_event_stop_processing(event);
        return;
      }
      shell->pressed = shell->token;
      shell->dragging = true;
      shell->input = lv_indev_active();
      shell->gesture_connection = port->temperature_->connection_generation();
    } else if (code == LV_EVENT_PRESS_LOST || code == LV_EVENT_INDEV_RESET) {
      const bool was = shell->dragging;
      port->cancel_position_gesture(*shell);
      if (was) port->paint(shell->index, shell->token, false);
    }
  }
  static void temperature_slider_event(lv_event_t* event) {
    auto* shell = static_cast<Shell*>(lv_event_get_user_data(event));
    auto* port = shell->port;
    const auto code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESS_LOST || code == LV_EVENT_INDEV_RESET) {
      temperature_slider_guard(event);
      return;
    }
    if (!shell->dragging) return;
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING || code == LV_EVENT_VALUE_CHANGED) {
      port->temperature_pointer(*shell);
      port->paint(shell->index, shell->token, false);
      return;
    }
    if (code != LV_EVENT_RELEASED) return;
    port->temperature_pointer(*shell);
    if (!shell->dragging) {
      port->paint(shell->index, shell->token, false);
      return;
    }
    const auto origin = shell->pressed;
    int target;
    if (!same(origin, shell->token) || !port->actions_.valid ||
        !port->actions_.valid(port->actions_.owner, origin) ||
        !port->temperature_snapshot(*shell).available ||
        shell->gesture_connection != port->temperature_->connection_generation() ||
        !port->temperature_->finish({origin.configuration, origin.card},
                                    lv_slider_get_value(port->widgets_[shell->index].slider), target)) {
      port->cancel_position_gesture(*shell);
      port->paint(shell->index, shell->token, false);
      return;
    }
    shell->pressed = {};
    shell->dragging = false;
    shell->input = nullptr;
    port->paint(shell->index, shell->token, false);
    if (port->actions_.light_temperature)
      port->actions_.light_temperature(port->actions_.owner, origin, target);
  }
  PageNumberSliderSnapshot number_snapshot(const Shell& shell) const {
    number_slider_->loop();
    return read_basic_number_slider(shell.definition->definition,
                                    {shell.token.configuration, shell.token.card}, state_, number_slider_);
  }
  void number_feedback(Shell& shell, BasicCardState& value) {
    const auto& w = widgets_[shell.index];
    const auto& snapshot = value.number_slider;
    if (shell.dragging && (!snapshot.gesture || !snapshot.available || !snapshot.ready ||
                           shell.gesture_connection != number_slider_->connection_generation()))
      cancel_position_gesture(shell);
    lv_obj_clear_flag(w.slider, LV_OBJ_FLAG_HIDDEN);
    if (snapshot.available && snapshot.ready)
      lv_obj_add_flag(w.slider, LV_OBJ_FLAG_CLICKABLE);
    else
      lv_obj_clear_flag(w.slider, LV_OBJ_FLAG_CLICKABLE);
    lv_slider_set_range(w.slider, 0, snapshot.positions);
    lv_slider_set_value(w.slider, snapshot.position, LV_ANIM_OFF);
  }
  void paint_number_feedback(Shell& shell) {
    paint(shell.index, shell.token, false);
  }
  void number_pointer(Shell& shell) {
    if (!shell.dragging) return;
    const auto snapshot = number_snapshot(shell);
    if (!snapshot.gesture || !snapshot.ready || !snapshot.available ||
        shell.gesture_connection != number_slider_->connection_generation()) {
      cancel_position_gesture(shell);
      return;
    }
    auto* slider = widgets_[shell.index].slider;
    if (shell.input && lv_indev_get_type(shell.input) == LV_INDEV_TYPE_POINTER) {
      lv_point_t point;
      lv_indev_get_point(shell.input, &point);
      lv_area_t area;
      lv_obj_get_coords(slider, &area);
      int position;
      if (slider_geometry::vertical_pointer_position(point.y, area.y1, area.y2, snapshot.positions, position))
        lv_slider_set_value(slider, position, LV_ANIM_OFF);
    }
    number_slider_->drag({shell.token.configuration, shell.token.card}, lv_slider_get_value(slider));
  }
  static void number_slider_guard(lv_event_t* event) {
    auto* shell = static_cast<Shell*>(lv_event_get_user_data(event));
    auto* port = shell->port;
    const auto code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED) {
      const auto snapshot = port->number_snapshot(*shell);
      if (!snapshot.available || !snapshot.ready || !port->actions_.valid ||
          !port->actions_.valid(port->actions_.owner, shell->token) ||
          !port->number_slider_->begin({shell->token.configuration, shell->token.card})) {
        lv_event_stop_processing(event);
        return;
      }
      shell->pressed = shell->token;
      shell->dragging = true;
      shell->input = lv_indev_active();
      shell->gesture_connection = port->number_slider_->connection_generation();
    } else if (code == LV_EVENT_PRESS_LOST || code == LV_EVENT_INDEV_RESET) {
      const bool was = shell->dragging;
      port->cancel_position_gesture(*shell);
      if (was) port->paint_number_feedback(*shell);
    }
  }
  static void number_slider_event(lv_event_t* event) {
    auto* shell = static_cast<Shell*>(lv_event_get_user_data(event));
    auto* port = shell->port;
    const auto code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESS_LOST || code == LV_EVENT_INDEV_RESET) {
      number_slider_guard(event);
      return;
    }
    if (!shell->dragging) return;
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING || code == LV_EVENT_VALUE_CHANGED) {
      port->number_pointer(*shell);
      port->paint_number_feedback(*shell);
      return;
    }
    if (code != LV_EVENT_RELEASED) return;
    port->number_pointer(*shell);
    if (!shell->dragging) {
      port->paint_number_feedback(*shell);
      return;
    }
    const auto origin = shell->pressed;
    const auto snapshot = port->number_snapshot(*shell);
    double target;
    if (!same(origin, shell->token) || !port->actions_.valid ||
        !port->actions_.valid(port->actions_.owner, origin) || !snapshot.available || !snapshot.ready ||
        shell->gesture_connection != port->number_slider_->connection_generation() ||
        !port->number_slider_->finish({origin.configuration, origin.card},
                                      lv_slider_get_value(port->widgets_[shell->index].slider), target)) {
      port->cancel_position_gesture(*shell);
      port->paint_number_feedback(*shell);
      return;
    }
    shell->pressed = {};
    shell->dragging = false;
    shell->input = nullptr;
    port->paint_number_feedback(*shell);
    if (port->actions_.number_slider)
      port->actions_.number_slider(port->actions_.owner, origin, target, snapshot.metadata);
  }
  PagePercentSliderSnapshot percent_snapshot(const Shell& shell) const {
    return read_basic_percent_slider(shell.definition->definition,
                                     {shell.token.configuration, shell.token.card}, state_, percent_slider_);
  }
  void percent_feedback(Shell& shell, BasicCardState& value) {
    const auto& w = widgets_[shell.index];
    if (shell.dragging && shell.gesture_connection != percent_slider_->connection_generation())
      cancel_position_gesture(shell);
    lv_obj_clear_flag(w.slider, LV_OBJ_FLAG_HIDDEN);
    if (value.percent_slider.available)
      lv_obj_add_flag(w.slider, LV_OBJ_FLAG_CLICKABLE);
    else {
      lv_obj_clear_flag(w.slider, LV_OBJ_FLAG_CLICKABLE);
      cancel_position_gesture(shell);
    }
    if (!shell.dragging || shell.membership_revision != value.percent_slider.write_revision)
      lv_slider_set_value(w.slider, value.percent_slider.percent, LV_ANIM_OFF);
    shell.membership_revision = value.percent_slider.write_revision;
    const auto kind = value.percent_slider.kind;
    const bool inverted =
        kind == PagePercentSliderKind::COVER_POSITION || kind == PagePercentSliderKind::COVER_TILT;
    if (shell.dragging) {
      const auto percent = lv_slider_get_value(w.slider);
      value.fill_percent = static_cast<uint8_t>(inverted ? 100 - percent : percent);
    }
  }
  void paint_percent_feedback(Shell& shell) {
    if (!percent_card(shell)) return;
    BasicCardState value;
    value.percent_slider = percent_snapshot(shell);
    const auto kind = value.percent_slider.kind;
    const bool inverted =
        kind == PagePercentSliderKind::COVER_POSITION || kind == PagePercentSliderKind::COVER_TILT;
    value.fill_percent = !value.percent_slider.fill_known ? 0
                         : inverted                       ? 100 - value.percent_slider.percent
                                                          : value.percent_slider.percent;
    percent_feedback(shell, value);
    shell.fill_percent = value.fill_percent;
    update_fill_geometry(shell);
  }
  void percent_report(Shell& shell) {
    const auto value = percent_snapshot(shell);
    if (shell.membership_revision != value.write_revision)
      lv_slider_set_value(widgets_[shell.index].slider, value.percent, LV_ANIM_OFF);
    shell.membership_revision = value.write_revision;
  }
  static void percent_pointer(Shell& shell) {
    auto* port = shell.port;
    if (!shell.dragging || !shell.input || lv_indev_get_type(shell.input) != LV_INDEV_TYPE_POINTER) return;
    lv_point_t point;
    lv_indev_get_point(shell.input, &point);
    auto* slider = port->widgets_[shell.index].slider;
    lv_area_t area;
    lv_obj_get_coords(slider, &area);
    int percent;
    if (slider_geometry::vertical_pointer_percent(point.y, area.y1, area.y2, percent))
      lv_slider_set_value(slider, percent, LV_ANIM_OFF);
  }
  static void percent_slider_guard(lv_event_t* event) {
    auto* shell = static_cast<Shell*>(lv_event_get_user_data(event));
    auto* port = shell->port;
    const auto code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED) {
      const auto snapshot = port->percent_snapshot(*shell);
      if (!snapshot.available || !port->actions_.valid ||
          !port->actions_.valid(port->actions_.owner, shell->token)) {
        lv_event_stop_processing(event);
        return;
      }
      shell->pressed = shell->token;
      shell->dragging = true;
      shell->input = lv_indev_active();
      shell->gesture_connection = port->percent_slider_->connection_generation();
    } else if (code == LV_EVENT_PRESS_LOST || code == LV_EVENT_INDEV_RESET) {
      const bool was = shell->dragging;
      shell->dragging = false;
      shell->pressed = {};
      shell->input = nullptr;
      if (was) port->paint_percent_feedback(*shell);
    }
  }
  static void percent_slider_event(lv_event_t* event) {
    auto* shell = static_cast<Shell*>(lv_event_get_user_data(event));
    auto* port = shell->port;
    const auto code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED) {
      if (!shell->dragging) return;
      port->percent_report(*shell);
      percent_pointer(*shell);
      port->paint_percent_feedback(*shell);
      return;
    }
    if (code == LV_EVENT_PRESSING) {
      port->percent_report(*shell);
      percent_pointer(*shell);
      port->paint_percent_feedback(*shell);
      return;
    }
    if (code == LV_EVENT_PRESS_LOST || code == LV_EVENT_INDEV_RESET) {
      const bool was = shell->dragging;
      shell->dragging = false;
      shell->pressed = {};
      shell->input = nullptr;
      if (was) port->paint_percent_feedback(*shell);
      return;
    }
    if (code == LV_EVENT_VALUE_CHANGED) {
      if (shell->dragging) port->paint_percent_feedback(*shell);
      return;
    }
    if (code != LV_EVENT_RELEASED || !shell->dragging) return;
    port->percent_report(*shell);
    percent_pointer(*shell);
    const auto origin = shell->pressed;
    const auto connection = shell->gesture_connection;
    shell->pressed = {};
    shell->dragging = false;
    shell->input = nullptr;
    if (!same(origin, shell->token) || !port->actions_.valid ||
        !port->actions_.valid(port->actions_.owner, origin) ||
        connection != port->percent_slider_->connection_generation() ||
        !port->percent_snapshot(*shell).available) {
      port->paint_percent_feedback(*shell);
      return;
    }
    const auto percent = lv_slider_get_value(port->widgets_[shell->index].slider);
    if (percent < 0 || percent > 100 ||
        !port->percent_slider_->preview({origin.configuration, origin.card}, static_cast<uint8_t>(percent)))
      return;
    port->paint_percent_feedback(*shell);
    if (port->actions_.percent_slider) port->actions_.percent_slider(port->actions_.owner, origin, percent);
  }
  bool position_snapshot(Shell& shell, PageMediaPlaybackSnapshot& snapshot) const {
    if (!media_ || !seek_card(shell))
      return false;
    const auto& definition = shell.definition->definition;
    return media_->playback({shell.token.configuration, shell.token.card}, definition.entity, snapshot);
  }
  static bool seek_available(const PageMediaPlaybackSnapshot& snapshot) {
    int seconds;
    return snapshot.available && snapshot.has_duration &&
           media::seek_seconds(snapshot.duration, 100, seconds);
  }
  void update_fill_geometry(Shell& shell) {
    const auto& w = widgets_[shell.index];
    if (percent_card(shell) || number_card(shell) || temperature_card(shell)) {
      const auto width = lv_obj_get_width(w.button), height = lv_obj_get_height(w.button);
      if (width <= 0 || height <= 0 || !w.slider || !w.fill) return;
      const auto left = lv_obj_get_style_pad_left(w.button, LV_PART_MAIN);
      const auto right = lv_obj_get_style_pad_right(w.button, LV_PART_MAIN);
      const auto top = lv_obj_get_style_pad_top(w.button, LV_PART_MAIN);
      const auto bottom = lv_obj_get_style_pad_bottom(w.button, LV_PART_MAIN);
      const auto content_width = lv_obj_get_content_width(w.button),
                 content_height = lv_obj_get_content_height(w.button);
      const auto shift_x = (content_width + left + right) / 2 - content_width / 2 - left;
      const auto shift_y = (content_height + top + bottom) / 2 - content_height / 2 - top;
      const auto slider_width = std::max<int32_t>(1, width >= height ? height - 1 : width);
      lv_obj_set_size(w.slider, slider_width, height);
      lv_obj_align(w.slider, LV_ALIGN_CENTER, shift_x, shift_y);
      const auto fill_height = static_cast<int32_t>(height) * shell.fill_percent / 100;
      const bool inverted =
          percent_card(shell) &&
          (shell.definition->definition.percent_slider == PagePercentSliderKind::COVER_POSITION ||
           shell.definition->definition.percent_slider == PagePercentSliderKind::COVER_TILT);
      lv_obj_set_size(w.fill, width, fill_height);
      lv_obj_align(w.fill, inverted ? LV_ALIGN_TOP_MID : LV_ALIGN_BOTTOM_MID, shift_x,
                   inverted ? -top : bottom);
      lv_obj_set_style_radius(w.fill, lv_obj_get_style_radius(w.button, LV_PART_MAIN), LV_PART_MAIN);
      return;
    }
    const bool horizontal =
        seek_card(shell);
    const auto width = lv_obj_get_width(w.button), height = lv_obj_get_height(w.button);
    const auto x = -lv_obj_get_style_pad_left(w.button, LV_PART_MAIN),
               y = -lv_obj_get_style_pad_top(w.button, LV_PART_MAIN);
    if (horizontal && w.slider) {
      const auto slider_height = std::max<lv_coord_t>(1, height >= width ? width - 1 : height);
      lv_obj_set_size(w.slider, width, slider_height);
      lv_obj_align(w.slider, LV_ALIGN_CENTER, 0, 0);
    }
    if (!w.fill || (!horizontal && !shell.fill_percent)) return;
    const auto radius = lv_obj_get_style_radius(w.button, LV_PART_MAIN);
    const auto horizontal_width =
        std::max<lv_coord_t>(std::min<lv_coord_t>(width, std::max<lv_coord_t>(0, radius * 2)),
                             static_cast<lv_coord_t>(static_cast<int32_t>(width) * shell.fill_percent / 100));
    lv_obj_set_size(w.fill, horizontal ? horizontal_width : width,
                    horizontal
                        ? height
                        : static_cast<lv_coord_t>(static_cast<int32_t>(height) * shell.fill_percent / 100));
    if (horizontal)
      lv_obj_align(w.fill, LV_ALIGN_LEFT_MID, x, 0);
    else
      lv_obj_set_pos(w.fill, x, y);
    lv_obj_set_style_radius(w.fill, lv_obj_get_style_radius(w.button, LV_PART_MAIN), LV_PART_MAIN);
  }
  static uint32_t pending_timeout(const Shell& shell) {
    return shell.definition && shell.definition->definition.kind == BasicCardKind::MEDIA_CONTROL ? 1500
                                                                                                 : 3000;
  }
  static bool group_card(const Shell& shell) {
    return shell.definition && shell.definition->definition.kind == BasicCardKind::MEDIA_CONTROL &&
           shell.definition->definition.media.kind == MediaActionKind::SPEAKER_GROUP;
  }
  void group_feedback(Shell& shell, BasicCardState& value) {
    if (shell.pending_target < 0) return;
    const auto& definition = shell.definition->definition;
    const ConfigurationHandle card{shell.token.configuration, shell.token.card};
    const auto primary = state_.read_attribute(card, definition.entity, "");
    const auto members = state_.read_attribute(card, definition.entity, "group_members");
    PageMediaSourceSnapshot source;
    if (!media_->source_snapshot(card, definition.entity, source) || !source.available ||
        primary.status != PageEntityStatus::AVAILABLE ||
        !media_group_source_available(primary.value.substr(0, HA_SHORT_STATE_MAX_LEN)) ||
        members.status == PageEntityStatus::EXHAUSTED ||
        media_->group_revision(card, definition.entity) != shell.membership_revision ||
        shell.gesture_connection != media_->connection_generation()) {
      shell.pending_target = -1;
      return;
    }
    value.on = shell.pending_target > 0;
  }
  void volume_feedback(Shell& shell, BasicCardState& value) {
    if (shell.pending_target < 0) return;
    PageMediaSourceSnapshot source;
    const auto& definition = shell.definition->definition;
    const bool current =
        media_->source_snapshot({shell.token.configuration, shell.token.card}, definition.entity, source);
    if (!current || !source.available || shell.gesture_connection != media_->connection_generation() ||
        value.media_volume.mode != media::VolumeControlMode::ABSOLUTE ||
        static_cast<uint32_t>(media_->now_ms() - shell.pending_ms) >= 1500 ||
        (value.media_volume.known && value.media_volume.display == shell.pending_target)) {
      shell.pending_target = -1;
      return;
    }
    std::snprintf(value.number, sizeof(value.number), "%d", static_cast<int>(shell.pending_target));
    value.numeric = true;
  }
  void position_feedback(Shell& shell, BasicCardState& value) {
    const auto& w = widgets_[shell.index];
    PageMediaPlaybackSnapshot snapshot;
    if (shell.dragging && shell.gesture_connection != media_->connection_generation())
      cancel_position_gesture(shell);
    const bool interactive = position_snapshot(shell, snapshot) && seek_available(snapshot);
    lv_obj_clear_flag(w.slider, LV_OBJ_FLAG_HIDDEN);
    if (interactive)
      lv_obj_add_flag(w.slider, LV_OBJ_FLAG_CLICKABLE);
    else {
      lv_obj_clear_flag(w.slider, LV_OBJ_FLAG_CLICKABLE);
      cancel_position_gesture(shell);
      shell.pending_target = -1;
    }
    const auto now = media_->now_ms();
    if (shell.pending_target >= 0 &&
        (static_cast<uint32_t>(now - shell.pending_ms) >= 3000 ||
         (snapshot.has_position && std::fabs(snapshot.seconds - shell.pending_target) <= 2.0f)))
      shell.pending_target = -1;
    if (shell.dragging) {
      value.fill_percent = static_cast<uint8_t>(lv_slider_get_value(w.slider));
      media::format_playback_time(
          static_cast<float>(static_cast<double>(snapshot.duration) * value.fill_percent / 100), value.number,
          sizeof(value.number));
    } else if (shell.pending_target >= 0) {
      media::PlaybackClock pending;
      pending.duration = snapshot.duration;
      pending.has_duration = snapshot.has_duration;
      pending.position = shell.pending_target;
      pending.has_position = true;
      pending.updated_ms = shell.pending_ms;
      pending.anchored = true;
      pending.playing = snapshot.playing;
      value.fill_percent = pending.percent(now);
      media::format_playback_time(pending.seconds(now), value.number, sizeof(value.number));
    }
    if (!shell.dragging) lv_slider_set_value(w.slider, value.fill_percent, LV_ANIM_OFF);
    update_fill_geometry(shell);
  }
  void paint_position_feedback(Shell& shell) {
    if (!shell.definition) return;
    auto value = read_basic_card(shell.definition->definition, {shell.token.configuration, shell.token.card},
                                 state_, 2, theme_.temperature_unit, theme_.translate, media_);
    position_feedback(shell, value);
    const auto& w = widgets_[shell.index];
    if(shell.definition->definition.kind==BasicCardKind::MEDIA_POSITION)write(w.value, shell.number, active_.number_capacity, value.number);
    shell.fill_percent = value.fill_percent;
    if (seek_card(shell)) {
      lv_obj_clear_flag(w.fill, LV_OBJ_FLAG_HIDDEN);
      update_fill_geometry(shell);
    } else {
      lv_obj_add_flag(w.fill, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_size(w.fill, 0, 0);
    }
  }
  // Capture/cancel the binding before LVGL can update the slider. Commands read
  // the final value in the ordinary callback, after the class has handled release.
  static void slider_guard(lv_event_t* event) {
    auto* shell = static_cast<Shell*>(lv_event_get_user_data(event));
    auto* port = shell->port;
    if (temperature_card(*shell)) {
      temperature_slider_guard(event);
      return;
    }
    if (number_card(*shell)) {
      number_slider_guard(event);
      return;
    }
    if (percent_card(*shell)) {
      percent_slider_guard(event);
      return;
    }
    if (!seek_card(*shell)) return;
    const auto code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED) {
      PageMediaPlaybackSnapshot snapshot;
      if (port->position_snapshot(*shell, snapshot) && seek_available(snapshot) && port->actions_.valid &&
          port->actions_.valid(port->actions_.owner, shell->token)) {
        shell->pressed = shell->token;
        shell->dragging = true;
        shell->input = lv_indev_active();
        shell->gesture_connection = port->media_->connection_generation();
      } else
        lv_event_stop_processing(event);
      return;
    }
    if (code == LV_EVENT_PRESS_LOST || code == LV_EVENT_INDEV_RESET) {
      const bool was_dragging = shell->dragging;
      shell->pressed = {};
      shell->dragging = false;
      shell->input = nullptr;
      if (was_dragging) port->paint_position_feedback(*shell);
      return;
    }
  }
  static void slider_event(lv_event_t* event) {
    auto* shell = static_cast<Shell*>(lv_event_get_user_data(event));
    auto* port = shell->port;
    if (temperature_card(*shell)) {
      temperature_slider_event(event);
      return;
    }
    if (number_card(*shell)) {
      number_slider_event(event);
      return;
    }
    if (percent_card(*shell)) {
      percent_slider_event(event);
      return;
    }
    if (!seek_card(*shell)) return;
    const auto code = lv_event_get_code(event);
    if (code == LV_EVENT_VALUE_CHANGED) {
      if (shell->dragging && same(shell->pressed, shell->token)) port->paint_position_feedback(*shell);
      return;
    }
    if (code != LV_EVENT_RELEASED) return;
    const auto origin = shell->pressed;
    shell->pressed = {};
    const bool dragging = shell->dragging;
    shell->dragging = false;
    shell->input = nullptr;
    if (!dragging || shell->gesture_connection != port->media_->connection_generation() ||
        !same(origin, shell->token) || !port->actions_.valid ||
        !port->actions_.valid(port->actions_.owner, origin))
      return;
    PageMediaPlaybackSnapshot snapshot;
    if (!port->position_snapshot(*shell, snapshot) || !seek_available(snapshot)) {
      port->paint_position_feedback(*shell);
      return;
    }
    const auto percent = static_cast<uint8_t>(lv_slider_get_value(port->widgets_[shell->index].slider));
    shell->pending_target = static_cast<float>(static_cast<double>(snapshot.duration) * percent / 100);
    shell->pending_ms = port->media_->now_ms();
    port->paint_position_feedback(*shell);
    if (port->actions_.seek) port->actions_.seek(port->actions_.owner, origin, percent, snapshot.duration);
  }
  static bool media_text(BasicCardKind kind) {
    return kind == BasicCardKind::MEDIA_NOW_PLAYING || kind == BasicCardKind::MEDIA_COVER_ART;
  }
  static uint8_t media_title_lines(const Shell& shell) {
    if (shell.definition->definition.kind == BasicCardKind::MEDIA_COVER_ART) {
      if (shell.cell.row_span == 3 && shell.cell.column_span == 3) return 5;
      if (shell.cell.row_span == 1 || (shell.cell.row_span == 2 && shell.cell.column_span == 2)) return 2;
      return 0;
    }
    return shell.cell.row_span == 1 ? 2 : 0;
  }
  void clear_cover(Shell& shell) {
    const auto token = shell.token;
    const bool owned =
        shell.definition && shell.definition->definition.kind == BasicCardKind::MEDIA_COVER_ART;
    shell.cover = {};
    shell.cover_asset = 0;
    shell.media_idle = false;
    if (owned && artwork_) artwork_->unbind(token);
  }
  void update_cover(Shell& shell) {
    const auto& w = widgets_[shell.index];
    const auto width = lv_obj_get_width(w.button), height = lv_obj_get_height(w.button);
    if (width <= 0 || height <= 0 || width > UINT16_MAX || height > UINT16_MAX) return;
    int percent = theme_.media_artwork_width_percent;
    percent = percent <= 0 ? 100 : std::clamp(percent, 50, 150);
    const auto decode_width = std::max<int32_t>(1, (static_cast<int64_t>(width) * percent + 50) / 100);
    if (decode_width > UINT16_MAX) return;
    artwork_->size(shell.token, decode_width, height);
    const auto surface = artwork_->surface(shell.token);
    shell.cover = {};
    shell.cover_asset = 0;
    if (!surface.source || !surface.asset) {
      lv_obj_add_flag(w.forecast, LV_OBJ_FLAG_HIDDEN);
      return;
    }
    const auto overscan = esphome::artwork_image::cover_alignment_edge_overscan(decode_width, height, width,
                                                                                height, LV_SCALE_NONE);
    lv_obj_set_pos(w.forecast, -lv_obj_get_style_pad_left(w.button, LV_PART_MAIN),
                   -lv_obj_get_style_pad_top(w.button, LV_PART_MAIN));
    lv_obj_set_size(w.forecast, width + overscan, height);
    lv_obj_set_style_radius(w.forecast, lv_obj_get_style_radius(w.button, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_clip_corner(w.forecast, lv_obj_get_style_clip_corner(w.button, LV_PART_MAIN),
                                 LV_PART_MAIN);
    shell.cover = {surface.source, surface.tint, width, height,
                   shell.definition->definition.media.track_details};
    shell.cover_asset = surface.asset;
    lv_obj_move_background(w.forecast);
    lv_obj_clear_flag(w.forecast, LV_OBJ_FLAG_HIDDEN);
    lv_obj_refresh_ext_draw_size(w.forecast);
    lv_obj_invalidate(w.forecast);
    for (auto* label : {w.icon, w.label, w.value, w.unit})
      lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_color(w.button, lv_color_white(), LV_PART_MAIN);
    shell.foreground = 0xFFFFFF;
  }
  void cover_event(Shell& shell, lv_event_t* event) {
    const auto code = lv_event_get_code(event);
    if (code != LV_EVENT_DRAW_MAIN && code != LV_EVENT_REFR_EXT_DRAW_SIZE) return;
    const auto token = shell.token;
    const auto asset = shell.cover_asset;
    const auto valid = [&]() {
      return same(token, shell.token) && asset && artwork_ && actions_.valid &&
             actions_.valid(actions_.owner, token) && artwork_->surface(token).asset == asset;
    };
    if (!valid()) return;
    auto frame = shell.cover;
    frame.source = artwork_->surface(token).source;
    auto* surface = widgets_[shell.index].forecast;
    if (code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
      auto* size = static_cast<int32_t*>(lv_event_get_param(event));
      if (size) frame.extend_draw_size(surface, *size);
      return;
    }
    if (frame.draw_image(surface, lv_event_get_layer(event)) && valid())
      frame.draw_tint(surface, lv_event_get_layer(event));
  }
  void prepare_now_playing_text(Shell& shell, const BasicCardState& value) {
    if (!shell.definition || !media_text(shell.definition->definition.kind)) return;
    const auto& w = widgets_[shell.index];
    const auto& definition = shell.definition->definition;
    const bool cover = definition.kind == BasicCardKind::MEDIA_COVER_ART;
    shell.media_idle = cover && value.media_idle;
    if (cover) update_cover(shell);
    if (shell.media_idle) {
      restore_label(w.icon, shell.icon_layout);
      restore_label(w.label, shell.label_layout);
      if (theme_.icon_font) lv_obj_set_style_text_font(w.icon, theme_.icon_font, LV_PART_MAIN);
      if (theme_.label_font) lv_obj_set_style_text_font(w.label, theme_.label_font, LV_PART_MAIN);
      lv_label_set_text_static(w.icon, shell.definition->off);
      const auto* idle = theme_.translate ? theme_.translate("Idle") : nullptr;
      write(w.label, shell.label, active_.label_capacity, idle ? idle : "Idle");
      lv_obj_align(w.icon, LV_ALIGN_TOP_LEFT, 0, 0);
      lv_obj_align(w.label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
      lv_obj_set_width(w.label, lv_pct(100));
      lv_label_set_long_mode(w.label, LV_LABEL_LONG_WRAP);
      lv_obj_clear_flag(w.icon, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(w.label, LV_OBJ_FLAG_HIDDEN);
      return;
    }
    const bool large = cover && shell.cell.row_span >= 2 && shell.cell.column_span >= 2;
    const bool compact = large && shell.cell.row_span == 2 && shell.cell.column_span == 2;
    const auto* title_font = compact && theme_.compact_portrait && theme_.media_control_title_font
                                 ? theme_.media_control_title_font
                             : compact && theme_.cover_artist_font          ? theme_.cover_artist_font
                             : large && !compact && theme_.cover_title_font ? theme_.cover_title_font
                                                                            : theme_.media_title_font;
    const auto* baseline_artist = theme_.label_font ? theme_.label_font : shell.label_layout.font;
    const auto* artist_font = compact && theme_.compact_portrait && theme_.media_control_artist_font
                                  ? theme_.media_control_artist_font
                              : large && !compact && theme_.cover_artist_font ? theme_.cover_artist_font
                                                                              : baseline_artist;
    lv_obj_set_style_text_font(w.icon, title_font, LV_PART_MAIN);
    lv_obj_set_style_text_font(w.label, artist_font, LV_PART_MAIN);
    lv_obj_set_style_pad_all(w.icon, 0, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(w.icon, -1, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(w.icon, lv_obj_get_style_text_letter_space(w.button, LV_PART_MAIN),
                                       LV_PART_MAIN);
    lv_obj_set_style_text_align(w.icon, lv_obj_get_style_text_align(w.button, LV_PART_MAIN), LV_PART_MAIN);
    for (auto* label : {w.icon, w.label}) {
      lv_obj_set_style_transform_scale_x(label, theme_.media_text_scale_x, LV_PART_MAIN);
      lv_obj_set_style_transform_scale_y(label, theme_.media_text_scale_y, LV_PART_MAIN);
    }
    const uint8_t lines = media_title_lines(shell);
    lv_obj_set_style_max_height(w.icon, lines ? title_font->line_height * lines - (lines - 1) : LV_COORD_MAX,
                                LV_PART_MAIN);
    lv_obj_set_style_max_height(w.label, shell.label_layout.maximum, LV_PART_MAIN);
    lv_obj_set_height(w.label, artist_font->line_height);
    lv_obj_align(w.icon, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_align(w.label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_height(w.icon, LV_SIZE_CONTENT);
    lv_coord_t width = lv_pct(100);
    if (definition.media.progress) {
      const auto left = lv_obj_get_style_pad_left(w.button, LV_PART_MAIN),
                 right = lv_obj_get_style_pad_right(w.button, LV_PART_MAIN);
      const auto available = lv_obj_get_width(w.button) - left - right;
      width = left > 0 && available > 1 ? available : lv_obj_get_content_width(w.button) + left + right;
    }
    lv_obj_set_width(w.icon, width);
    lv_obj_set_width(w.label, width);
    char decoded[257];
    auto title = value.media_source_title
                     ? std::string_view(value.text)
                     : decode_html_entities_bounded(decoded, sizeof(decoded), value.media_title);
    if (title == "unknown" || title == "unavailable") title = {};
    title = media::normalize_media_display_text_bounded(decoded, sizeof(decoded), title);
    write(w.icon, shell.number, active_.number_capacity, title.empty() ? std::string_view("--") : title);
    if (lv_label_get_long_mode(w.icon) != LV_LABEL_LONG_WRAP)
      lv_label_set_long_mode(w.icon, LV_LABEL_LONG_WRAP);
    std::string_view artist;
    if (value.media_external_input) {
      const auto* translated = theme_.translate ? theme_.translate("Source") : nullptr;
      artist = translated ? translated : "Source";
    } else {
      artist = decode_html_entities_bounded(decoded, sizeof(decoded), value.media_artist);
      if (artist == "unknown" || artist == "unavailable") artist = {};
      artist = media::normalize_media_display_text_bounded(decoded, sizeof(decoded), artist);
    }
    write(w.label, shell.label, active_.label_capacity, artist);
    if (lv_label_get_long_mode(w.label) != LV_LABEL_LONG_WRAP)
      lv_label_set_long_mode(w.label, LV_LABEL_LONG_WRAP);
    if (cover && !definition.media.track_details && !value.media_source_title)
      lv_obj_add_flag(w.icon, LV_OBJ_FLAG_HIDDEN);
    else
      lv_obj_clear_flag(w.icon, LV_OBJ_FLAG_HIDDEN);
    if (artist.empty() || (cover && !definition.media.track_details && !value.media_source_title))
      lv_obj_add_flag(w.label, LV_OBJ_FLAG_HIDDEN);
    else
      lv_obj_clear_flag(w.label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(w.sensor_container, LV_OBJ_FLAG_HIDDEN);
    if (cover || definition.media.play_pause_background)
      lv_obj_add_flag(w.button, LV_OBJ_FLAG_CLICKABLE);
    else
      lv_obj_clear_flag(w.button, LV_OBJ_FLAG_CLICKABLE);
  }
  void update_forecast(Shell& shell) {
    if (!shell.frame || !shell.definition || shell.definition->definition.kind != BasicCardKind::FORECAST ||
        !forecasts_)
      return;
    const auto& w = widgets_[shell.index];
    const auto& definition = shell.definition->definition;
    PageForecastSnapshot snapshot;
    snapshot.loading = false;
    const bool mapped =
        forecasts_->snapshot({shell.token.configuration, shell.token.card}, definition.entity, snapshot);
    if (mapped)
      lv_obj_add_flag(w.button, LV_OBJ_FLAG_CLICKABLE);
    else
      lv_obj_clear_flag(w.button, LV_OBJ_FLAG_CLICKABLE);
    PageForecastTextStyle style;
    style.font = lv_obj_get_style_text_font(w.label, LV_PART_MAIN);
    style.temperature_font = theme_.forecast_small_font ? theme_.forecast_small_font : style.font;
    style.icon_font = theme_.forecast_icon_font ? theme_.forecast_icon_font
                                                : lv_obj_get_style_text_font(w.icon, LV_PART_MAIN);
    style.translate = theme_.translate;
    style.temperature = theme_.forecast_temperature;
    style.scale_x = theme_.forecast_scale_x;
    style.scale_y = theme_.forecast_scale_y;
    style.letter_space = lv_obj_get_style_text_letter_space(w.label, LV_PART_MAIN);
    style.line_space = lv_obj_get_style_text_line_space(w.label, LV_PART_MAIN);
    style.direction = lv_obj_get_style_base_dir(w.label, LV_PART_MAIN);
    shell.frame->build(snapshot, definition.forecast_view, lv_obj_get_content_width(w.button),
                       lv_obj_get_content_height(w.button), style, active_.weather->glyphs.data(),
                       active_.weather->loading);
    write(w.label, shell.label, active_.label_capacity, shell.frame->caption);
    if (shell.frame->placeholder) {
      if (std::strcmp(lv_label_get_text(w.icon), shell.frame->placeholder) != 0)
        lv_label_set_text_static(w.icon, shell.frame->placeholder);
      lv_obj_clear_flag(w.icon, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(w.forecast, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(w.icon, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(w.forecast, LV_OBJ_FLAG_HIDDEN);
      lv_obj_invalidate(w.forecast);
    }
  }
  static void forecast_event(lv_event_t* event) {
    auto* shell = static_cast<Shell*>(lv_event_get_user_data(event));
    auto* port = shell->port;
    if (!port->active_.arena || !shell->token.binding) return;
    if (shell->definition && shell->definition->definition.kind == BasicCardKind::MEDIA_COVER_ART) {
      port->cover_event(*shell, event);
      return;
    }
    if (lv_event_get_code(event) != LV_EVENT_DRAW_MAIN || !shell->frame) return;
    const auto token = shell->token;
    const auto count = shell->frame->count;
    const auto style = shell->frame->style;
    auto* canvas = port->widgets_[shell->index].forecast;
    lv_area_t area;
    lv_obj_get_coords(canvas, &area);
    const auto color = lv_obj_get_style_text_color(port->widgets_[shell->index].button, LV_PART_MAIN);
    const auto opacity = lv_obj_get_style_text_opa(port->widgets_[shell->index].button, LV_PART_MAIN);
    const bool antialias = lv_display_get_antialiasing(lv_obj_get_display(canvas));
    for (size_t i = 0; i < count; ++i) {
      if (!same(token, shell->token) || !shell->frame) break;
      draw_forecast_text(lv_event_get_layer(event), shell->frame->texts[i], {area.x1, area.y1}, style, color,
                         opacity, antialias);
    }
  }
  static void event(lv_event_t *event) {
    auto *shell=static_cast<Shell *>(lv_event_get_user_data(event));auto *port=shell->port;
    const auto code=lv_event_get_code(event);
    if(code==LV_EVENT_SIZE_CHANGED){port->update_fill_geometry(*shell);port->update_forecast(*shell);
      if (!port->media_finishing_ && shell->definition && media_text(shell->definition->definition.kind))
        port->media_layout_dirty_ = true;
      return;}
    if(code==LV_EVENT_PRESSED) {
      shell->pressed=shell->token;
      if (shell->definition && (shell->definition->definition.kind == BasicCardKind::COVER_COMMAND ||
                                shell->definition->definition.kind == BasicCardKind::COVER_TOGGLE ||
                                shell->definition->definition.kind == BasicCardKind::ACCESS ||
                                shell->definition->definition.kind == BasicCardKind::CLEANING))
        shell->gesture_connection = port->state_.connection_generation();
      if (timer_card(*shell)) {
        port->timer_->loop();
        if (shell->gesture_connection != port->timer_->connection_generation()) shell->pending_target = -1;
        shell->gesture_connection = port->timer_->connection_generation();
      }
      if(shell->definition&&shell->definition->definition.kind==BasicCardKind::MEDIA_ACTION) {
        const auto kind=shell->definition->definition.media.kind;
        if(kind==MediaActionKind::PREVIOUS||kind==MediaActionKind::NEXT) {
          const auto origin=shell->pressed;shell->pressed={};
          if(port->actions_.valid&&port->actions_.valid(port->actions_.owner,origin)&&port->actions_.media)
            port->actions_.media(port->actions_.owner,origin);
        }
      }
      return;
    }
    if(code==LV_EVENT_PRESS_LOST){shell->pressed={};return;}
    if(code!=LV_EVENT_CLICKED)return;
    const auto origin=shell->pressed;shell->pressed={};
    if(!same(origin,shell->token)||!port->actions_.valid||!port->actions_.valid(port->actions_.owner,origin))return;
    if(origin.back){if(port->actions_.back)port->actions_.back(port->actions_.owner,origin);return;}
    if(!shell->definition)return;
    const auto &definition=shell->definition->definition;
    if (definition.kind == BasicCardKind::CLEANING) {
      const auto snapshot =
          read_basic_cleaning(definition, {origin.configuration, origin.card}, port->state_);
      if (snapshot.service && shell->gesture_connection == port->state_.connection_generation() &&
          port->actions_.cleaning)
        port->actions_.cleaning(port->actions_.owner, origin, snapshot.service);
      return;
    }
    if (definition.kind == BasicCardKind::ACCESS) {
      port->access_->loop();
      const auto snapshot =
          read_basic_access(definition, {origin.configuration, origin.card}, port->state_, port->access_);
      if (!snapshot.enabled || shell->gesture_connection != port->state_.connection_generation()) return;
      if (!definition.access.command && definition.access.kind != BasicAccessKind::LOCK)
        lv_obj_add_state(port->widgets_[shell->index].button, LV_STATE_CHECKED);
      if (port->actions_.access)
        port->actions_.access(port->actions_.owner, origin, {snapshot.history.on, snapshot.history.locked});
      return;
    }
    if (definition.kind == BasicCardKind::COVER_TOGGLE) {
      const ConfigurationHandle card{origin.configuration, origin.card};
      if (definition.entity.empty() || shell->gesture_connection != port->state_.connection_generation() ||
          port->state_.read_attribute(card, definition.entity, {}).status == PageEntityStatus::EXHAUSTED)
        return;
      // Existing cover toggles dispatch without an availability gate and optimistically check the card.
      lv_obj_add_state(port->widgets_[shell->index].button, LV_STATE_CHECKED);
      if (port->actions_.toggle) port->actions_.toggle(port->actions_.owner, origin, definition.entity);
      return;
    }
    if (definition.kind == BasicCardKind::COVER_COMMAND) {
      const auto command =
          read_basic_cover_command(definition, {origin.configuration, origin.card}, port->state_);
      if (shell->gesture_connection == port->state_.connection_generation() && command.supported &&
          port->actions_.cover_command)
        port->actions_.cover_command(port->actions_.owner, origin, command);
      return;
    }
    if (definition.kind == BasicCardKind::FAN_SWITCH || definition.kind == BasicCardKind::FAN_OSCILLATE) {
      if (!port->fan_status_) return;
      port->fan_status_->loop();
      const ConfigurationHandle card{origin.configuration, origin.card};
      const auto status = port->fan_status_->read(card).status;
      if (status == PageFanStatus::UNAVAILABLE || status == PageFanStatus::UNSUPPORTED ||
          port->state_.read_attribute(card, definition.entity, {}).status != PageEntityStatus::AVAILABLE ||
          (definition.kind == BasicCardKind::FAN_OSCILLATE &&
           port->state_.read_attribute(card, definition.entity, "oscillating").status !=
               PageEntityStatus::AVAILABLE))
        return;
    }
    if (timer_card(*shell)) {
      port->timer_click(*shell, origin);
      return;
    }
    if(definition.kind==BasicCardKind::FOLDER) {
      const auto target=port->store_.card(origin.card)->target;
      if(port->actions_.open&&target!=NO_PAGE)port->actions_.open(port->actions_.owner,origin,target);
    } else if ((definition.kind == BasicCardKind::FAN_DIRECTION ||
                definition.kind == BasicCardKind::FAN_SWITCH ||
                definition.kind == BasicCardKind::FAN_OSCILLATE) &&
               port->actions_.action)
      port->actions_.action(port->actions_.owner, origin);
    else if ((definition.kind == BasicCardKind::FAN_PRESET ||
              definition.kind == BasicCardKind::OPTION_SELECT) &&
             port->actions_.control)
      port->actions_.control(port->actions_.owner, origin);
    else if (definition.kind == BasicCardKind::CLIMATE && port->actions_.climate)
      port->actions_.climate(port->actions_.owner,origin);
    else if ((definition.kind == BasicCardKind::TOGGLE || definition.kind == BasicCardKind::LOCAL_RELAY) &&
             port->actions_.toggle)
      port->actions_.toggle(port->actions_.owner,origin,definition.entity);
    else if ((definition.kind == BasicCardKind::LIGHT || definition.kind == BasicCardKind::COVER ||
              definition.kind == BasicCardKind::FAN_CONTROL) &&
             port->actions_.control)
      port->actions_.control(port->actions_.owner, origin);
    else if(definition.kind==BasicCardKind::FORECAST&&port->actions_.forecast)
      port->actions_.forecast(port->actions_.owner,origin);
    else if((definition.kind==BasicCardKind::MEDIA_ACTION||(definition.kind==BasicCardKind::MEDIA_NOW_PLAYING&&definition.media.play_pause_background))&&port->actions_.media)
      port->actions_.media(port->actions_.owner,origin);
    else if (definition.kind == BasicCardKind::MEDIA_COVER_ART && port->actions_.media_control)
      port->actions_.media_control(port->actions_.owner, origin);
    else if (definition.kind == BasicCardKind::MEDIA_CONTROL) {
      if (definition.media.kind == MediaActionKind::VOLUME) {
        if (port->actions_.media_volume) port->actions_.media_volume(port->actions_.owner, origin);
      } else if (port->actions_.media_control)
        port->actions_.media_control(port->actions_.owner, origin);
    } else if (definition.kind == BasicCardKind::ACTION && port->actions_.action)
      port->actions_.action(port->actions_.owner,origin);
  }
  PageArenaAllocator &allocator_;PageEntityStateRuntime &state_;std::array<PageBasicWidgets,Slots>widgets_;
  PageViewGeometry geometry_;
  const PageBasicTheme& theme_;
  PageBasicActions actions_;
  PageForecastRuntime* forecasts_{nullptr};
  PageMediaStateRuntime* media_{nullptr};
  PageCoverArtView* artwork_{nullptr};
  PageLocalSensorRuntime* local_{nullptr};
  PageLocalRelayRuntime* relay_{nullptr};
  PageFanStatusRuntime* fan_status_{nullptr};
  PagePercentSliderRuntime* percent_slider_{nullptr};
  PageNumberSliderRuntime* number_slider_{nullptr};
  PageLightTemperatureRuntime* temperature_{nullptr};
  PageTimerRuntime* timer_{nullptr};
  PageCoverToggleRuntime* cover_toggle_{nullptr};
  PageAccessStateRuntime* access_{nullptr};
  std::array<Shell, Slots> shells_{};
  Arena active_, staged_;
  PageStore store_;
  bool initialized_{false}, media_layout_dirty_{false}, media_finishing_{false};
  size_t callbacks_{0};
  uint64_t seen_access_{0}, seen_cover_toggle_{0}, seen_timer_{0}, seen_temperature_{0},
      seen_number_slider_{0}, seen_percent_slider_{0}, seen_fan_status_{0}, seen_relay_{0}, seen_local_{0},
      seen_state_{0}, seen_theme_{0}, seen_forecast_{0}, seen_media_{0}, seen_artwork_{0};
  PageBasicMemory memory_;
};
}  // namespace espcontrol::pages
