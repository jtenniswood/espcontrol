#pragma once

#include <array>
#include <algorithm>
#include <cstring>
#include <limits>
#include <new>

#include "lvgl.h"
#include "bounded_display_text.h"
#include "display_color.h"
#include "page_basic_card.h"

namespace espcontrol::pages {
// The selected firmware supplies exactly one existing bank of grid widgets.
// The bank and fonts outlive this port; no widgets are created by navigation.
struct PageBasicWidgets {
  lv_obj_t *button{nullptr},*icon{nullptr},*label{nullptr},*sensor_container{nullptr},*value{nullptr},*unit{nullptr};
};
struct PageBasicTheme {
  uint64_t revision{0};
  uint32_t surface{0x212121},sensor_surface{0x212121},active{0xFF8C00},text{0xFFFFFF};
  int red_percent{100},green_percent{100},blue_percent{100};
  const lv_font_t *sensor_font{nullptr},*large_sensor_font{nullptr};
  std::string_view back_label{"Back"},empty_label{"Configure"},folder_label{"Subpage"};
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
};
struct PageBasicMemory {size_t fixed_bytes{0},borrowed_widgets{0},active_bytes{0},staged_bytes{0},peak_bytes{0},text_truncations{0},invalid_text{0};};

template<size_t Slots>
class PageBasicLvglPort final:public PageRendererPort {
 public:
  PageBasicLvglPort(PageArenaAllocator &allocator,PageEntityStateRuntime &state,
                    std::array<PageBasicWidgets,Slots> widgets,PageViewGeometry geometry,
                    const PageBasicTheme &theme,PageBasicActions actions)
      :allocator_(allocator),state_(state),widgets_(widgets),geometry_(geometry),theme_(theme),actions_(actions) {
    for(size_t i=0;i<Slots;++i){shells_[i].port=this;shells_[i].index=i;}
  }
  ~PageBasicLvglPort()override {
    retire_active();discard_prepared();
    for(size_t i=0;i<callbacks_;++i)
      lv_obj_remove_event_cb_with_user_data(widgets_[i].button,event,&shells_[i]);
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
    if(actions_.icon==nullptr)return false;
    std::array<lv_obj_t *,Slots*6> unique{};size_t used=0;
    for(const auto &w:widgets_) {
      for(auto *object:{w.button,w.icon,w.label,w.sensor_container,w.value,w.unit}) {
        if(!object)return false;
        for(size_t index=0;index<used;++index)if(unique[index]==object)return false;
        unique[used++]=object;
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
        lv_obj_set_style_transition(w.button,nullptr,selector);
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
      if(theme_.sensor_font)lv_obj_set_style_text_font(w.value,theme_.sensor_font,LV_PART_MAIN);
      if(lv_obj_add_event_cb(w.button,event,LV_EVENT_ALL,&shells_[i])==nullptr)return false;
      ++callbacks_;
    }
    initialized_=true;return true;
  }
  bool prepare(const PageStore &store,size_t count)override {
    discard_prepared();if(!initialized_||count!=Slots)return false;
    TextCounter strings;size_t summary_count=0;
    size_t label_capacity=std::max<size_t>(65,std::max({theme_.back_label.size(),theme_.empty_label.size(),theme_.folder_label.size()})+1),unit_capacity=1;
    // Back labels are decoded by the layout compiler; their source order size
    // is a conservative, profile-bounded upper bound on decoded text bytes.
    for(PageId page=0;page<store.page_count();++page) {
      const auto *p=store.page(page);if(!p->navigable)continue;
      label_capacity=std::max(label_capacity,store.string(p->order).size()+1);
      for(CardId card=p->first_card;card<p->first_card+p->card_count;++card) {
        BasicCardDefinition view;if(!prepare_basic_card(store,card,view,&strings))return false;
        if(view.kind==BasicCardKind::TEXT||view.text_overlay||view.folder_mode==BasicFolderMode::TEXT)label_capacity=std::max<size_t>(label_capacity,257);
        label_capacity=std::max(label_capacity,view.label.size()+1);unit_capacity=std::max(unit_capacity,view.unit.size()+1);
      }
    }
    for(PageId page=1;page<store.page_count();++page) {
      std::array<CardId,Slots> cards{};size_t used=0;
      if(!collect_summary(store,page,cards,used)||used>SIZE_MAX-summary_count)return false;
      summary_count+=used;
    }
    label_capacity=std::max(label_capacity,strings.largest+1);
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
    const size_t bytes=summary_offset+summary_count*sizeof(CardId);void *memory=allocator_.allocate(bytes);if(memory==nullptr)return false;
    staged_={memory,bytes,label_capacity,unit_capacity,stride,store.card_count()};
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
        const auto fallback=d.kind==BasicCardKind::FOLDER&&d.entity.empty()?std::string_view("Folder"):std::string_view("Auto");
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
    for(auto &shell:shells_){shell.token={};shell.pressed={};shell.definition=nullptr;shell.back={};shell.cell={};shell.label=shell.unit=shell.number=nullptr;}
    detach_text();for(const auto &w:widgets_)lv_obj_add_flag(w.button,LV_OBJ_FLAG_HIDDEN);
    release(active_);store_={};
  }
  void unbind(size_t index,PageBindingToken)override {
    if(index>=Slots)return;auto &s=shells_[index];s.token={};s.pressed={};s.definition=nullptr;s.back={};s.cell={};s.label=s.unit=s.number=nullptr;
    const auto &w=widgets_[index];lv_obj_add_flag(w.button,LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_state(w.button,static_cast<lv_state_t>(LV_STATE_CHECKED|LV_STATE_PRESSED));
    lv_obj_clear_flag(w.button,LV_OBJ_FLAG_CLICKABLE);
    // Hidden static text only refers to this port's reusable buffers or
    // firmware glyphs; keep it attached until rebind or arena retirement.
  }
  void bind(size_t index,const PageStore &,const PageCellLayout &cell,PageBackStyleView back,PageBindingToken token)override {
    if(index>=Slots||!active_.arena||(!cell.back&&cell.card>=active_.cards))return;auto &s=shells_[index];const auto &w=widgets_[index];
    s.token=token;s.pressed={};s.cell=cell;s.back=back;
    s.definition=cell.back?nullptr:prepared(active_)+cell.card;
    auto *text=static_cast<char *>(active_.arena)+active_.cards*sizeof(Prepared)+index*active_.stride;
    s.label=text;s.unit=text+active_.label_capacity;s.number=s.unit+active_.unit_capacity;
    lv_obj_set_grid_cell(w.button,LV_GRID_ALIGN_STRETCH,cell.column,cell.column_span,LV_GRID_ALIGN_STRETCH,cell.row,cell.row_span);
    lv_obj_clear_flag(w.button,LV_OBJ_FLAG_HIDDEN);
    const bool clickable=cell.back||(s.definition&&(s.definition->definition.kind==BasicCardKind::TOGGLE||s.definition->definition.kind==BasicCardKind::FOLDER));
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
      definition=s.definition->definition;value=read_basic_card(definition,{token.configuration,token.card},state_,s.cell.column_span>1?2:1);
      if(definition.folder_mode==BasicFolderMode::CHILDREN) {
        for(size_t child=0;child<s.definition->summary_count&&!value.on;++child) {
          const auto id=s.definition->summary[child];
          value.on=read_basic_indicator({token.configuration,id},basic_folder_child_indicator(store_,id),state_);
        }
      }
      if(definition.kind==BasicCardKind::FOLDER&&definition.folder_mode==BasicFolderMode::NUMBER&&value.label.empty())value.label=theme_.folder_label;
      if(definition.kind==BasicCardKind::EMPTY&&value.label.empty())value.label=theme_.empty_label;
      checked=value.on&&(definition.kind==BasicCardKind::TOGGLE||definition.active_color);
      number=definition.kind==BasicCardKind::NUMBER||definition.kind==BasicCardKind::DURATION||definition.folder_mode==BasicFolderMode::NUMBER||(definition.kind==BasicCardKind::TOGGLE&&!definition.text_overlay&&!definition.sensor.empty()&&value.on);
      const bool state_icon=definition.kind==BasicCardKind::TOGGLE||definition.kind==BasicCardKind::ICON||
        definition.folder_mode==BasicFolderMode::ENTITY||definition.folder_mode==BasicFolderMode::CLIMATE||definition.folder_mode==BasicFolderMode::MOWER||definition.folder_mode==BasicFolderMode::CHILDREN;
      glyph=state_icon&&value.on?s.definition->on:s.definition->off;
    }
    write(w.label,s.label,active_.label_capacity,value.formatted_label?std::string_view(value.text):value.label);
    write(w.unit,s.unit,active_.unit_capacity,value.unit);
    write(w.value,s.number,48,value.numeric?std::string_view(value.number):std::string_view{});
    if(std::strcmp(lv_label_get_text(w.icon),glyph)!=0)lv_label_set_text_static(w.icon,glyph);
    if(number){lv_obj_add_flag(w.icon,LV_OBJ_FLAG_HIDDEN);lv_obj_clear_flag(w.sensor_container,LV_OBJ_FLAG_HIDDEN);}
    else {lv_obj_clear_flag(w.icon,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(w.sensor_container,LV_OBJ_FLAG_HIDDEN);}
    const auto font=definition.large_numbers&&s.cell.column_span>1&&s.cell.row_span>1&&theme_.large_sensor_font?theme_.large_sensor_font:theme_.sensor_font;
    if(font&&s.painted_font!=font){lv_obj_set_style_text_font(w.value,font,LV_PART_MAIN);s.painted_font=font;}
    if(checked!=lv_obj_has_state(w.button,LV_STATE_CHECKED)){if(checked)lv_obj_add_state(w.button,LV_STATE_CHECKED);else lv_obj_clear_state(w.button,LV_STATE_CHECKED);}
    const auto correct=[this](uint32_t raw){return correct_display_color(raw,theme_.red_percent,theme_.green_percent,theme_.blue_percent);};
    uint32_t background=definition.custom_off?correct(definition.off_color):(definition.kind==BasicCardKind::NUMBER||definition.kind==BasicCardKind::TEXT||definition.kind==BasicCardKind::DURATION)?theme_.sensor_surface:theme_.surface;
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
    seen_state_=state_.revision();seen_theme_=theme_.revision;
  }
  void suspend()override{for(auto &shell:shells_)shell.pressed={};}
  bool state_changed()const override{return state_.revision()!=seen_state_||theme_.revision!=seen_theme_;}
  PageBasicMemory memory()const{auto m=memory_;m.fixed_bytes=sizeof(*this);m.borrowed_widgets=Slots*6;m.active_bytes=active_.bytes;m.staged_bytes=staged_.bytes;return m;}
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
      if(value.size()>SIZE_MAX-bytes)return false;
      bytes+=value.size();largest=std::max(largest,value.size());out={};return true;
    }
    size_t bytes{0},largest{0};
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
  struct Arena {void *arena{nullptr};size_t bytes{0},label_capacity{0},unit_capacity{0},stride{0},cards{0};};
  struct Shell {PageBasicLvglPort *port{nullptr};size_t index{0};PageBindingToken token,pressed;PageCellLayout cell;PageBackStyleView back;Prepared *definition{nullptr};char *label{nullptr},*unit{nullptr},*number{nullptr};bool painted{false};uint32_t background{0},foreground{0};const lv_font_t *painted_font{nullptr};};
  static bool same(PageBindingToken a,PageBindingToken b){return a.binding!=0&&a.configuration==b.configuration&&a.binding==b.binding&&a.page==b.page&&a.card==b.card&&a.shell==b.shell&&a.back==b.back;}
  static Prepared *prepared(const Arena &arena){return static_cast<Prepared *>(arena.arena);}
  void release(Arena &arena){if(arena.arena)allocator_.release(arena.arena,arena.bytes);arena={};}
  void detach_text(){for(const auto &w:widgets_)if(w.button)for(auto *label:{w.icon,w.label,w.value,w.unit})if(label)lv_label_set_text_static(label,"");}
  void write(lv_obj_t *label,char *buffer,size_t capacity,std::string_view text) {
    bool changed=false;const auto result=copy_display_text(buffer,capacity,text,SIZE_MAX,&changed);memory_.text_truncations+=result.truncated;memory_.invalid_text+=result.invalid;
    if(changed||lv_label_get_text(label)!=buffer)lv_label_set_text_static(label,buffer);
  }
  static void event(lv_event_t *event) {
    auto *shell=static_cast<Shell *>(lv_event_get_user_data(event));auto *port=shell->port;
    const auto code=lv_event_get_code(event);
    if(code==LV_EVENT_PRESSED){shell->pressed=shell->token;return;}
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
    } else if(definition.kind==BasicCardKind::TOGGLE&&port->actions_.toggle)
      port->actions_.toggle(port->actions_.owner,origin,definition.entity);
  }
  PageArenaAllocator &allocator_;PageEntityStateRuntime &state_;std::array<PageBasicWidgets,Slots>widgets_;
  PageViewGeometry geometry_;const PageBasicTheme &theme_;PageBasicActions actions_;std::array<Shell,Slots>shells_{};
  Arena active_,staged_;PageStore store_;bool initialized_{false};size_t callbacks_{0};uint64_t seen_state_{0},seen_theme_{0};PageBasicMemory memory_;
};
}  // namespace espcontrol::pages
