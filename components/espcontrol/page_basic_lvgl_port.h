#pragma once

#include <array>
#include <algorithm>
#include <cstring>
#include <limits>
#include <new>

#include "lvgl.h"
#include "bounded_display_text.h"
#include "display_color.h"
#include "button_grid_slider_contrast.h"
#include "page_basic_card.h"
#include "page_forecast_frame.h"
#include "weather_condition_maps_generated.h"

namespace espcontrol::pages {
// The selected firmware supplies exactly one existing bank of grid widgets.
// The bank and fonts outlive this port; no widgets are created by navigation.
struct PageBasicWidgets {
  lv_obj_t *button{nullptr},*icon{nullptr},*label{nullptr},*sensor_container{nullptr},*value{nullptr},*unit{nullptr},*fill{nullptr},*forecast{nullptr};
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
  PageBasicLvglPort(PageArenaAllocator &allocator,PageEntityStateRuntime &state,
                    std::array<PageBasicWidgets,Slots> widgets,PageViewGeometry geometry,
                    const PageBasicTheme &theme,PageBasicActions actions,PageForecastRuntime *forecasts=nullptr,PageMediaStateRuntime *media=nullptr)
      :allocator_(allocator),state_(state),widgets_(widgets),geometry_(geometry),theme_(theme),actions_(actions),forecasts_(forecasts),media_(media) {
    for(size_t i=0;i<Slots;++i){shells_[i].port=this;shells_[i].index=i;}
  }
  ~PageBasicLvglPort()override {
    retire_active();discard_prepared();
    for(size_t i=0;i<callbacks_;++i)
      lv_obj_remove_event_cb_with_user_data(widgets_[i].button,event,&shells_[i]);
    for(size_t slot=0;slot<Slots;++slot) {
      const auto &w=widgets_[slot];
      if(shells_[slot].forecast_callback)lv_obj_remove_event_cb_with_user_data(w.forecast,forecast_event,&shells_[slot]);
      const std::array<lv_obj_t *,4>labels={w.icon,w.label,w.value,w.unit};
      for(size_t i=0;i<shells_[slot].contrast_callbacks;++i)
        lv_obj_remove_event_cb_with_user_data(labels[i],slider_text_contrast_event_cb,w.fill);
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
    for(const auto &shell:shells_)if(shell.contrast_callbacks||shell.forecast_callback)return false;
    if(actions_.icon==nullptr)return false;
    page_basic_push_transition();
    page_basic_no_transition();
    std::array<lv_obj_t *,Slots*8> unique{};size_t used=0;
    for(const auto &w:widgets_) {
      for(auto *object:{w.button,w.icon,w.label,w.sensor_container,w.value,w.unit}) {
        if(!object)return false;
        for(size_t index=0;index<used;++index)if(unique[index]==object)return false;
        unique[used++]=object;
      }
      for(auto *optional:{w.fill,w.forecast})if(optional) {
        for(size_t index=0;index<used;++index)if(unique[index]==optional)return false;
        unique[used++]=optional;if(lv_obj_get_parent(optional)!=w.button)return false;
      }
      if(lv_obj_get_parent(w.icon)!=w.button||lv_obj_get_parent(w.label)!=w.button||
         lv_obj_get_parent(w.sensor_container)!=w.button||lv_obj_get_parent(w.value)!=w.sensor_container||
         lv_obj_get_parent(w.unit)!=w.sensor_container)return false;
    }
    for(size_t i=0;i<Slots;++i) {
      const auto &w=widgets_[i];lv_obj_add_flag(w.button,LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(w.button,LV_OBJ_FLAG_CHECKABLE);lv_obj_clear_state(w.button,LV_STATE_CHECKED);
      for(const auto selector:{static_cast<lv_style_selector_t>(LV_PART_MAIN),
           (static_cast<lv_style_selector_t>(LV_PART_MAIN)|static_cast<lv_style_selector_t>(LV_STATE_CHECKED))}) {
        lv_obj_set_style_bg_color(w.button,lv_color_hex(theme_.surface),selector);
        lv_obj_set_style_text_color(w.button,lv_color_hex(theme_.text),selector);
        lv_obj_set_style_transition(w.button,page_basic_no_transition(),selector);
      }
      lv_obj_set_grid_cell(w.button,LV_GRID_ALIGN_STRETCH,0,1,LV_GRID_ALIGN_STRETCH,0,1);
      lv_obj_set_style_grid_cell_row_pos(w.button,0,LV_PART_MAIN);
      lv_obj_set_style_grid_cell_column_span(w.button,1,LV_PART_MAIN);
      lv_obj_set_style_grid_cell_row_span(w.button,1,LV_PART_MAIN);
      lv_label_set_long_mode(w.label,LV_LABEL_LONG_WRAP);
      for(auto *label:{w.icon,w.label,w.value,w.unit}){
        lv_label_set_text_static(label,"");
        lv_obj_set_style_text_color(label,lv_color_hex(theme_.text),LV_PART_MAIN);
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
          if(!lv_obj_add_event_cb(label,slider_text_contrast_event_cb,LV_EVENT_DRAW_TASK_ADDED,w.fill))return false;
          ++shells_[i].contrast_callbacks;
        }
      }
      if(w.forecast) {
        lv_obj_add_flag(w.forecast,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_HIDDEN|LV_OBJ_FLAG_IGNORE_LAYOUT));
        lv_obj_clear_flag(w.forecast,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE));
        lv_obj_set_style_bg_opa(w.forecast,LV_OPA_TRANSP,LV_PART_MAIN);lv_obj_set_style_border_width(w.forecast,0,LV_PART_MAIN);
        lv_obj_set_style_pad_all(w.forecast,0,LV_PART_MAIN);lv_obj_set_style_shadow_width(w.forecast,0,LV_PART_MAIN);
        lv_obj_set_size(w.forecast,lv_pct(100),lv_pct(100));lv_obj_align(w.forecast,LV_ALIGN_TOP_LEFT,0,0);
        lv_obj_set_style_clip_corner(w.button,true,LV_PART_MAIN);
        if(!lv_obj_add_event_cb(w.forecast,forecast_event,LV_EVENT_DRAW_MAIN,&shells_[i]))return false;
        shells_[i].forecast_callback=true;
      }
      if(theme_.sensor_font)lv_obj_set_style_text_font(w.value,theme_.sensor_font,LV_PART_MAIN);
      if(lv_obj_add_event_cb(w.button,event,LV_EVENT_ALL,&shells_[i])==nullptr)return false;
      ++callbacks_;
    }
    initialized_=true;return true;
  }
  bool prepare(const PageStore &store,size_t count)override {
    discard_prepared();if(!initialized_||count!=Slots)return false;
    TextCounter strings;size_t summary_count=0;bool has_weather=false,has_forecast=false;size_t forecast_frames=0;
    size_t label_capacity=std::max<size_t>(65,std::max({theme_.back_label.size(),theme_.empty_label.size(),theme_.folder_label.size()})+1),unit_capacity=1;
    // Back labels are decoded by the layout compiler; their source order size
    // is a conservative, profile-bounded upper bound on decoded text bytes.
    for(PageId page=0;page<store.page_count();++page) {
      const auto *p=store.page(page);if(!p->navigable)continue;
      label_capacity=std::max(label_capacity,store.string(p->order).size()+1);
      for(CardId card=p->first_card;card<p->first_card+p->card_count;++card) {
        BasicCardDefinition view;if(!prepare_basic_card(store,card,view,&strings))return false;
        if(view.kind==BasicCardKind::WEATHER||view.kind==BasicCardKind::FORECAST)has_weather=true;
        if(view.kind==BasicCardKind::FORECAST) {
          has_forecast=true;if(!forecasts_||!actions_.forecast)return false;
          for(const auto &w:widgets_)if(!w.forecast)return false;
        }
        if(view.kind==BasicCardKind::CLIMATE&&actions_.climate==nullptr)return false;
        if(view.kind==BasicCardKind::ACTION&&actions_.action==nullptr)return false;
        if(view.kind==BasicCardKind::MEDIA_ACTION&&(actions_.media==nullptr||(view.media.kind==MediaActionKind::PLAYLIST&&!media_)))return false;
        if((view.kind==BasicCardKind::LIGHT||view.kind==BasicCardKind::COVER)&&actions_.control==nullptr)return false;
        if(view.kind==BasicCardKind::COVER)for(const auto &w:widgets_)if(!w.fill)return false;
        if((view.kind==BasicCardKind::MEDIA_ACTION&&view.media.show_state)||view.kind==BasicCardKind::TEXT||view.kind==BasicCardKind::CLIMATE||view.text_overlay||view.folder_mode==BasicFolderMode::TEXT||view.action_display==BasicActionDisplay::TEXT)label_capacity=std::max<size_t>(label_capacity,257);
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
    if(label_capacity>SIZE_MAX-unit_capacity-48)return false;
    const size_t stride=label_capacity+unit_capacity+48;
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
    size_t frame_offset=0;
    if(forecast_frames) {
      const auto padding=(alignof(PageForecastFrame)-bytes%alignof(PageForecastFrame))%alignof(PageForecastFrame);
      if(padding>SIZE_MAX-bytes||forecast_frames>(SIZE_MAX-bytes-padding)/sizeof(PageForecastFrame))return false;
      frame_offset=bytes+padding;bytes=frame_offset+forecast_frames*sizeof(PageForecastFrame);
    }
    void *memory=allocator_.allocate(bytes);if(memory==nullptr)return false;
    staged_={memory,bytes,label_capacity,unit_capacity,stride,store.card_count()};
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
        auto &view=prepared(staged_)[card];if(!prepare_basic_card(store,card,view.definition,&writer)){discard_prepared();return false;}
        const auto &d=view.definition;auto domain=d.entity.substr(0,d.entity.find('.'));
        if(d.kind==BasicCardKind::ICON)domain={};
        const auto fallback=d.kind==BasicCardKind::CLIMATE?std::string_view("Thermostat"):d.kind==BasicCardKind::FOLDER&&d.entity.empty()?std::string_view("Folder"):std::string_view("Auto");
        view.off=actions_.icon(actions_.owner,d.icon.empty()||d.icon=="Auto"?fallback:d.icon,domain);
        view.on=d.icon_on.empty()||d.icon_on=="Auto"?view.off:actions_.icon(actions_.owner,d.icon_on,domain);
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
    for(auto &shell:shells_){shell.token={};shell.pressed={};shell.definition=nullptr;shell.back={};shell.cell={};shell.label=shell.unit=shell.number=nullptr;shell.fill_percent=0;shell.frame=nullptr;}
    detach_text();for(const auto &w:widgets_){lv_obj_add_flag(w.button,LV_OBJ_FLAG_HIDDEN);if(w.forecast)lv_obj_add_flag(w.forecast,LV_OBJ_FLAG_HIDDEN);}
    release(active_);store_={};
  }
  void unbind(size_t index,PageBindingToken)override {
    if(index>=Slots)return;auto &s=shells_[index];
    const bool feedback=s.definition&&s.definition->definition.push_feedback;
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
    s.frame=nullptr;if(w.forecast)lv_obj_add_flag(w.forecast,LV_OBJ_FLAG_HIDDEN);
    s.fill_percent=0;if(w.fill){lv_obj_add_flag(w.fill,LV_OBJ_FLAG_HIDDEN);lv_obj_set_size(w.fill,0,0);}
    // Hidden static text only refers to this port's reusable buffers or
    // firmware glyphs; keep it attached until rebind or arena retirement.
  }
  void bind(size_t index,const PageStore &,const PageCellLayout &cell,PageBackStyleView back,PageBindingToken token)override {
    if(index>=Slots||!active_.arena||(!cell.back&&cell.card>=active_.cards))return;auto &s=shells_[index];const auto &w=widgets_[index];
    s.token=token;s.pressed={};s.cell=cell;s.back=back;
    s.definition=cell.back?nullptr:prepared(active_)+cell.card;
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
    const bool clickable=cell.back||(s.definition&&(s.definition->definition.kind==BasicCardKind::TOGGLE||s.definition->definition.kind==BasicCardKind::FOLDER||s.definition->definition.kind==BasicCardKind::CLIMATE||s.definition->definition.kind==BasicCardKind::ACTION||s.definition->definition.kind==BasicCardKind::LIGHT||s.definition->definition.kind==BasicCardKind::COVER||s.definition->definition.kind==BasicCardKind::FORECAST||s.definition->definition.kind==BasicCardKind::MEDIA_ACTION));
    if(clickable)lv_obj_add_flag(w.button,LV_OBJ_FLAG_CLICKABLE);else lv_obj_clear_flag(w.button,LV_OBJ_FLAG_CLICKABLE);
    // Do not read the new state graph here: its activation may follow ours.
  }
  void update(size_t index,PageBindingToken token)override {
    if(index>=Slots||!same(shells_[index].token,token)||!active_.arena)return;
    auto &s=shells_[index];const auto &w=widgets_[index];
    BasicCardState value;BasicCardDefinition definition;
    const char *glyph="\xF3\xB0\x85\x81"; // MDI arrow-left, U+F0141
    bool number=false,checked=false;
    if(s.cell.back){value.label=s.back.custom_label.empty()?theme_.back_label:s.back.custom_label;definition.custom_off=s.back.custom_color;definition.off_color=s.back.background_color;}
    else {
      definition=s.definition->definition;value=read_basic_card(definition,{token.configuration,token.card},state_,s.cell.column_span>1?2:1,theme_.temperature_unit,theme_.translate,media_);
      if(definition.folder_mode==BasicFolderMode::CHILDREN) {
        for(size_t child=0;child<s.definition->summary_count&&!value.on;++child) {
          const auto id=s.definition->summary[child];
          value.on=read_basic_indicator({token.configuration,id},basic_folder_child_indicator(store_,id),state_);
        }
      }
      if(definition.kind==BasicCardKind::FOLDER&&definition.folder_mode==BasicFolderMode::NUMBER&&value.label.empty())value.label=theme_.folder_label;
      if(definition.kind==BasicCardKind::EMPTY&&value.label.empty())value.label=theme_.empty_label;
      checked=value.on&&(definition.kind==BasicCardKind::TOGGLE||definition.active_color);
      number=(definition.kind==BasicCardKind::CLIMATE&&definition.climate.number!=ClimateCardNumber::ICON)||definition.kind==BasicCardKind::NUMBER||definition.kind==BasicCardKind::DURATION||definition.folder_mode==BasicFolderMode::NUMBER||definition.action_display==BasicActionDisplay::NUMBER||(definition.kind==BasicCardKind::TOGGLE&&!definition.text_overlay&&!definition.sensor.empty()&&value.on);
      const bool state_icon=definition.kind==BasicCardKind::TOGGLE||definition.kind==BasicCardKind::ICON||
        definition.kind==BasicCardKind::STATUS||definition.kind==BasicCardKind::LIGHT||definition.kind==BasicCardKind::COVER||definition.action_display==BasicActionDisplay::ICON||definition.folder_mode==BasicFolderMode::ENTITY||definition.folder_mode==BasicFolderMode::CLIMATE||definition.folder_mode==BasicFolderMode::MOWER||definition.folder_mode==BasicFolderMode::CHILDREN;
      if(definition.kind==BasicCardKind::WEATHER)glyph=active_.weather->glyphs[value.weather_condition];
      else glyph=definition.kind==BasicCardKind::CLIMATE?(value.climate_icon_on?s.definition->on:s.definition->off):state_icon&&value.on?s.definition->on:s.definition->off;
    }
    write(w.label,s.label,active_.label_capacity,value.formatted_label?std::string_view(value.text):value.label);
    for(const auto selector:{static_cast<lv_style_selector_t>(LV_PART_MAIN),
         static_cast<lv_style_selector_t>(LV_PART_MAIN)|static_cast<lv_style_selector_t>(LV_STATE_CHECKED)})
      lv_obj_set_style_transition(w.button,definition.push_feedback?page_basic_push_transition():page_basic_no_transition(),selector);
    write(w.unit,s.unit,active_.unit_capacity,value.unit);
    write(w.value,s.number,48,value.numeric?std::string_view(value.number):std::string_view{});
    if(std::strcmp(lv_label_get_text(w.icon),glyph)!=0)lv_label_set_text_static(w.icon,glyph);
    if(number){lv_obj_add_flag(w.icon,LV_OBJ_FLAG_HIDDEN);lv_obj_clear_flag(w.sensor_container,LV_OBJ_FLAG_HIDDEN);}
    else {lv_obj_clear_flag(w.icon,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(w.sensor_container,LV_OBJ_FLAG_HIDDEN);}
    const auto font=definition.large_numbers&&s.cell.column_span>1&&s.cell.row_span>1&&theme_.large_sensor_font?theme_.large_sensor_font:theme_.sensor_font;
    if(font&&s.painted_font!=font){lv_obj_set_style_text_font(w.value,font,LV_PART_MAIN);s.painted_font=font;}
    if(checked!=lv_obj_has_state(w.button,LV_STATE_CHECKED)){if(checked)lv_obj_add_state(w.button,LV_STATE_CHECKED);else lv_obj_clear_state(w.button,LV_STATE_CHECKED);}
    const auto correct=[this](uint32_t raw){return correct_display_color(raw,theme_.red_percent,theme_.green_percent,theme_.blue_percent);};
    uint32_t background=definition.custom_off?correct(definition.off_color):(definition.kind==BasicCardKind::NUMBER||definition.kind==BasicCardKind::TEXT||definition.kind==BasicCardKind::DURATION||definition.kind==BasicCardKind::STATUS||definition.kind==BasicCardKind::WEATHER||definition.kind==BasicCardKind::FORECAST)?theme_.sensor_surface:theme_.surface;
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
      if(definition.kind==BasicCardKind::COVER&&value.closed_percent) {
        s.fill_percent=value.closed_percent;update_fill_geometry(s);
        const auto fill_color=definition.custom_on?correct(definition.on_color):definition.custom_off&&!definition.theme_active?correct(active_card_color(definition.off_color)):theme_.active;
        lv_obj_set_style_bg_color(w.fill,lv_color_hex(fill_color),LV_PART_MAIN);lv_obj_clear_flag(w.fill,LV_OBJ_FLAG_HIDDEN);
      } else {s.fill_percent=0;lv_obj_add_flag(w.fill,LV_OBJ_FLAG_HIDDEN);lv_obj_set_size(w.fill,0,0);}
    }
    if(definition.kind==BasicCardKind::FORECAST)update_forecast(s);
    else if(w.forecast)lv_obj_add_flag(w.forecast,LV_OBJ_FLAG_HIDDEN);
    seen_state_=state_.revision();seen_theme_=theme_.revision;seen_forecast_=forecasts_?forecasts_->revision():0;seen_media_=media_?media_->revision():0;
  }
  void suspend()override{for(auto &shell:shells_)shell.pressed={};}
  bool state_changed()const override{return state_.revision()!=seen_state_||theme_.revision!=seen_theme_||(forecasts_&&forecasts_->revision()!=seen_forecast_)||(media_&&media_->revision()!=seen_media_);}
  PageBasicMemory memory()const{auto m=memory_;m.fixed_bytes=sizeof(*this);m.borrowed_widgets=Slots*6;for(const auto &w:widgets_)m.borrowed_widgets+=(w.fill?1:0)+(w.forecast?1:0);m.forecast_frames=active_.frame_count;m.active_bytes=active_.bytes;m.staged_bytes=staged_.bytes;return m;}
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
  struct Prepared {BasicCardDefinition definition;const char *off{nullptr},*on{nullptr};const CardId *summary{nullptr};uint16_t summary_count{0};};
  struct WeatherGlyphs {std::array<const char *,espcontrol::weather::CONDITION_COUNT>glyphs{};const char *loading{nullptr};};
  struct Arena {void *arena{nullptr};size_t bytes{0},label_capacity{0},unit_capacity{0},stride{0},cards{0};WeatherGlyphs *weather{nullptr};PageForecastFrame *frames{nullptr};size_t frame_count{0};};
  struct Shell {PageBasicLvglPort *port{nullptr};size_t index{0};PageBindingToken token,pressed;PageCellLayout cell;PageBackStyleView back;Prepared *definition{nullptr};char *label{nullptr},*unit{nullptr},*number{nullptr};bool painted{false};uint8_t fill_percent{0},contrast_callbacks{0};bool forecast_callback{false};PageForecastFrame *frame{nullptr};uint32_t background{0},foreground{0};const lv_font_t *painted_font{nullptr};};
  static bool same(PageBindingToken a,PageBindingToken b){return a.binding!=0&&a.configuration==b.configuration&&a.binding==b.binding&&a.page==b.page&&a.card==b.card&&a.shell==b.shell&&a.back==b.back;}
  static Prepared *prepared(const Arena &arena){return static_cast<Prepared *>(arena.arena);}
  void release(Arena &arena){if(arena.arena)allocator_.release(arena.arena,arena.bytes);arena={};}
  void detach_text(){for(const auto &w:widgets_)if(w.button)for(auto *label:{w.icon,w.label,w.value,w.unit})if(label)lv_label_set_text_static(label,"");}
  void write(lv_obj_t *label,char *buffer,size_t capacity,std::string_view text) {
    bool changed=false;const auto result=copy_display_text(buffer,capacity,text,SIZE_MAX,&changed);memory_.text_truncations+=result.truncated;memory_.invalid_text+=result.invalid;
    if(changed||lv_label_get_text(label)!=buffer)lv_label_set_text_static(label,buffer);
  }
  void update_fill_geometry(Shell &shell) {
    const auto &w=widgets_[shell.index];if(!w.fill||!shell.fill_percent)return;
    const auto width=lv_obj_get_width(w.button),height=lv_obj_get_height(w.button);
    const auto fill_height=static_cast<lv_coord_t>(static_cast<int32_t>(height)*shell.fill_percent/100);
    lv_obj_set_size(w.fill,width,fill_height);
    lv_obj_set_pos(w.fill,-lv_obj_get_style_pad_left(w.button,LV_PART_MAIN),-lv_obj_get_style_pad_top(w.button,LV_PART_MAIN));
    lv_obj_set_style_radius(w.fill,lv_obj_get_style_radius(w.button,LV_PART_MAIN),LV_PART_MAIN);
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
    if (!port->active_.arena || !shell->frame || !shell->token.binding) return;
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
    if(code==LV_EVENT_SIZE_CHANGED){port->update_fill_geometry(*shell);port->update_forecast(*shell);return;}
    if(code==LV_EVENT_PRESSED) {
      shell->pressed=shell->token;
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
    if(definition.kind==BasicCardKind::FOLDER) {
      const auto target=port->store_.card(origin.card)->target;
      if(port->actions_.open&&target!=NO_PAGE)port->actions_.open(port->actions_.owner,origin,target);
    } else if(definition.kind==BasicCardKind::CLIMATE&&port->actions_.climate)
      port->actions_.climate(port->actions_.owner,origin);
    else if(definition.kind==BasicCardKind::TOGGLE&&port->actions_.toggle)
      port->actions_.toggle(port->actions_.owner,origin,definition.entity);
    else if((definition.kind==BasicCardKind::LIGHT||definition.kind==BasicCardKind::COVER)&&port->actions_.control)
      port->actions_.control(port->actions_.owner,origin);
    else if(definition.kind==BasicCardKind::FORECAST&&port->actions_.forecast)
      port->actions_.forecast(port->actions_.owner,origin);
    else if(definition.kind==BasicCardKind::MEDIA_ACTION&&port->actions_.media)
      port->actions_.media(port->actions_.owner,origin);
    else if(definition.kind==BasicCardKind::ACTION&&port->actions_.action)
      port->actions_.action(port->actions_.owner,origin);
  }
  PageArenaAllocator &allocator_;PageEntityStateRuntime &state_;std::array<PageBasicWidgets,Slots>widgets_;
  PageViewGeometry geometry_;const PageBasicTheme &theme_;PageBasicActions actions_;PageForecastRuntime *forecasts_{nullptr};PageMediaStateRuntime *media_{nullptr};std::array<Shell,Slots>shells_{};
  Arena active_,staged_;PageStore store_;bool initialized_{false};size_t callbacks_{0};uint64_t seen_state_{0},seen_theme_{0},seen_forecast_{0},seen_media_{0};PageBasicMemory memory_;
};
}  // namespace espcontrol::pages
