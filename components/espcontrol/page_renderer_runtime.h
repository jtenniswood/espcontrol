#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>

#include "page_renderer_layout.h"
#include "page_store_runtime.h"

namespace espcontrol::pages {
struct PageBindingToken {
  uint32_t configuration{0}, binding{0};
  PageId page{NO_PAGE};
  CardId card{NO_CARD};
  uint16_t shell{UINT16_MAX};
  bool back{false};
};
struct PageRendererMemory {
  size_t active_layout_bytes{0}, staged_layout_bytes{0}, peak_layout_bytes{0};
  size_t preparation_failures{0}, view_changes{0}, shell_binds{0};
};
struct PageBackStyleView {
  std::string_view custom_label;
  uint32_t background_color{0};
  bool custom_color{false};
};

// Device ports own exactly the selected profile's shells and bounded widget/
// driver pools. All fallible work belongs in prepare. Bind/unbind/update only
// reuse those resources; they must not allocate widgets, contexts or channels.
// bind must defer state reads until update, after all prepared owners activate.
// Input/timer callbacks must carry the token captured at their origin; reading
// a replacement binding's token when an old callback executes defeats guards.
// Text views are length-delimited and need the port's reserved display buffers.
class PageRendererPort {
 public:
  virtual ~PageRendererPort() = default;
  virtual PageViewGeometry geometry(const PageStore &) const = 0;
  virtual bool prepare(const PageStore &,size_t shell_count) = 0;
  virtual void discard_prepared() = 0;
  virtual void activate_prepared(const PageStore &) = 0;
  virtual void retire_active() = 0;
  virtual void unbind(size_t shell,PageBindingToken retired) = 0;
  virtual void bind(size_t shell,const PageStore &,const PageCellLayout &,
                    PageBackStyleView,PageBindingToken) = 0;
  virtual void update(size_t shell,PageBindingToken) = 0;
  // Complete batched widget layout after every visible shell has updated.
  virtual void finish_update() {}
  virtual void suspend() = 0;
  virtual bool state_changed() const = 0;
};

// Main-loop-only binding/navigation owner. Preparation creates immutable layout
// data; ordinary page switches reuse the fixed shell set and allocate nothing.
// Native/action adapters and each real card driver still require device wiring
// and physical parity before this can replace the production grid.
template<size_t Slots>
class PageRendererRuntime final : public PageRuntimeResources {
 public:
  static_assert(Slots>0 && Slots<=UINT8_MAX);
  PageRendererRuntime(const DeviceUiLimits &limits,PageArenaAllocator &allocator,PageRendererPort &port,
                      uint32_t first_binding=1)
      : limits_(limits),allocator_(allocator),port_(port),next_binding_(first_binding) {}
  ~PageRendererRuntime() override {discard_prepared();retire_active();}
  PageRendererRuntime(const PageRendererRuntime &)=delete;
  PageRendererRuntime &operator=(const PageRendererRuntime &)=delete;

  bool prepare(const PageStore &store) override { return prepare_impl(store, false); }
  bool prepare_write(const PageStore &store) override { return prepare_impl(store, true); }
  bool preparation_capacity_exceeded() const override { return capacity_exceeded_; }
  void discard_prepared() override {
    port_.discard_prepared();
    if(staged_arena_!=nullptr)allocator_.release(staged_arena_,staged_bytes_);
    staged_arena_=nullptr;staged_bytes_=0;staged_binding_=0;
  }
  void activate_prepared(const PageStore &active) override {
    if(staged_arena_==nullptr || staged_binding_==0)return;
    Guard guard(changing_);
    const PageId target=restored_page(active);
    unbind_view();
    port_.activate_prepared(active);
    if(active_arena_!=nullptr)allocator_.release(active_arena_,active_bytes_);
    active_arena_=staged_arena_;active_bytes_=staged_bytes_;active_store_=active;
    staged_arena_=nullptr;staged_bytes_=0;
    const uint32_t binding=staged_binding_;staged_binding_=0;
    // A replacement owns prepared resources while the display is covered, but
    // must not attach new view/image bindings until the existing suspension
    // ends. resume() will allocate the next binding generation.
    if(suspended_) {active_page_=target;refresh_=true;}
    else bind_view(target,binding);
  }
  void retire_active() override {
    if(active_arena_==nullptr)return;
    Guard guard(changing_);unbind_view();port_.retire_active();
    allocator_.release(active_arena_,active_bytes_);active_arena_=nullptr;active_bytes_=0;
    active_store_={};active_page_=NO_PAGE;suspended_=false;
  }
  bool open_page(PageId page) {
    if(changing_ || suspended_ || active_arena_==nullptr || page>=active_store_.page_count() ||
       !layouts()[page].navigable)return false;
    if(page==active_page_)return true;
    if(next_binding_==0)return false;
    Guard guard(changing_);unbind_view();bind_view(page,next_binding_++);return true;
  }
  bool suspended() const { return suspended_; }
  bool return_home(){return open_page(0);}
  bool back() {
    if(active_page_==NO_PAGE || active_page_==0)return false;
    const auto *page=active_store_.page(active_page_);
    return page!=nullptr && open_page(page->parent);
  }
  bool suspend() {
    if(changing_ || suspended_ || active_arena_==nullptr)return false;
    Guard guard(changing_);suspended_=true;view_binding_=0;port_.suspend();return true;
  }
  bool resume() {
    if(changing_ || !suspended_ || active_arena_==nullptr || next_binding_==0)return false;
    Guard guard(changing_);const auto page=active_page_;unbind_view();suspended_=false;
    bind_view(page,next_binding_++);return true;
  }
  void refresh(){refresh_=true;}
  void update_visible() {
    if (changing_ || suspended_ || active_arena_ == nullptr) return;
    // Poll state owners before every frame, including a forced navigation or
    // reconnect refresh; the query can expire semantic deadlines and samples.
    const bool changed = port_.state_changed();
    if (!refresh_ && !changed) return;
    Guard guard(changing_);refresh_=false;
    for(size_t shell=0;shell<Slots;++shell)if(bindings_[shell].binding!=0)port_.update(shell,bindings_[shell]);
    port_.finish_update();
  }
  PageBindingToken binding(size_t shell)const{return shell<Slots?bindings_[shell]:PageBindingToken{};}
  // Copy a compiled cell without exposing an arena pointer or requiring the
  // page to be visible. Remote resolution uses the same orientation/layout.
  PageCellLayout cell(PageId page, size_t shell) const {
    return active_arena_ && page < active_store_.page_count() &&
                   shell < Slots && layouts()[page].navigable
               ? layouts()[page].cells[shell]
               : PageCellLayout{};
  }
  // Read-only ownership remains usable while the bank binds/paints. Input and
  // navigation callbacks must use valid(), which also rejects reentrant work.
  bool bound(PageBindingToken token) const {
    if (suspended_ || token.binding == 0 || token.binding != view_binding_ || token.shell >= Slots ||
        token.configuration != active_store_.generation() || token.page != active_page_)
      return false;
    const auto &current=bindings_[token.shell];
    return current.binding==token.binding && current.card==token.card && current.back==token.back;
  }
  bool valid(PageBindingToken token) const {
    return !changing_ && bound(token);
  }
  PageId active_page()const{return active_page_;}
  constexpr size_t shell_count()const{return Slots;}
  PageRendererMemory memory()const {
    auto result=memory_;result.active_layout_bytes=active_bytes_;result.staged_layout_bytes=staged_bytes_;return result;
  }
 private:
  bool prepare_impl(const PageStore &store, bool write) {
    capacity_exceeded_=false;
    if(changing_)return false;
    Guard guard(changing_);
    discard_prepared();
    auto fail=[this](){++memory_.preparation_failures;discard_prepared();return false;};
    if(limits_.slot_count!=Slots || store.page_count()==0 || !store.page(0)->navigable || next_binding_==0)return fail();
    staged_geometry_=port_.geometry(store);
    if(staged_geometry_.visible_slots==0 || staged_geometry_.visible_slots>Slots ||
       staged_geometry_.columns==0 || staged_geometry_.columns>staged_geometry_.visible_slots)return fail();
    size_t label_bytes=0;
    for(PageId page=0;page<store.page_count();++page) {
      const auto *definition=store.page(page);
      if(!definition->navigable)continue;
      if(page!=0 && (definition->parent!=0 || limits_.max_navigation_depth<1))return fail();
      if(!add(label_bytes,store.string(definition->order).size()))return fail();
    }
    size_t bytes=0;
    if(!add(bytes,store.page_count(),sizeof(Layout)) || !align(bytes,alignof(PageCellLayout)))return fail();
    const size_t cells_offset=bytes;
    if(!add(bytes,store.page_count(),Slots*sizeof(PageCellLayout)))return fail();
    const size_t labels_offset=bytes;
    if(!add(bytes,label_bytes) || bytes>UINT32_MAX)return fail();
    staged_arena_=allocator_.allocate(bytes);
    if(staged_arena_==nullptr)return fail();
    staged_bytes_=bytes;
    memory_.peak_layout_bytes=std::max(memory_.peak_layout_bytes,active_bytes_+staged_bytes_);
    auto *raw=static_cast<char *>(staged_arena_);
    auto *layouts=reinterpret_cast<Layout *>(raw);
    auto *cells=reinterpret_cast<PageCellLayout *>(raw+cells_offset);
    size_t labels_used=0;
    for(PageId page=0;page<store.page_count();++page) {
      auto &layout=*new(layouts+page) Layout{};
      layout.cells=cells+page*Slots;
      for(size_t cell=0;cell<Slots;++cell)new(layout.cells+cell) PageCellLayout{};
      if(!store.page(page)->navigable)continue;
      PageBackAppearance back;
      if(!compile_page_layout(store,page,staged_geometry_,layout.cells,Slots,&back) ||
         back.custom_label.size()>label_bytes-labels_used)return fail();
      layout.navigable=true;layout.back.background_color=back.background_color;
      layout.back.custom_color=back.custom_color;
      char *label=raw+labels_offset+labels_used;
      if(!back.custom_label.empty())std::memcpy(label,back.custom_label.data(),back.custom_label.size());
      layout.back.custom_label={label,back.custom_label.size()};labels_used+=back.custom_label.size();
    }
    if(write) {
      // Match the editor's shared Image/Media Cover Art budget across all
      // executable page layouts. Hidden and retained definitions do not bind.
      size_t images=0;
      for(PageId page=0;page<store.page_count();++page) {
        if(!layouts[page].navigable)continue;
        const auto* layout=layouts[page].cells;
        for(size_t cell=0;cell<Slots;++cell) {
          if(!layout[cell].visible||layout[cell].back||layout[cell].card==NO_CARD)continue;
          const auto* card=store.card(layout[cell].card);
          const auto type=store.field(*card,CardField::TYPE);
          if(type!="image" && !(type=="media"&&store.field(*card,CardField::SENSOR)=="cover_art"))continue;
          bool repeated=false;
          for(size_t previous=0;previous<cell;++previous)
            if(layout[previous].visible&&layout[previous].card==layout[cell].card)repeated=true;
          if(!repeated&&++images>limits_.image_slot_count){capacity_exceeded_=true;return fail();}
        }
      }
    }
    if(!port_.prepare(store,Slots))return fail();
    staged_binding_=next_binding_++;
    return true;
  }
  struct Guard {bool &flag;explicit Guard(bool &f):flag(f){flag=true;}~Guard(){flag=false;}};
  struct Layout {PageCellLayout *cells{nullptr};PageBackStyleView back;bool navigable{false};};
  static bool add(size_t &total,size_t count,size_t unit=1) {
    if(unit && count>(std::numeric_limits<size_t>::max()-total)/unit)return false;
    total+=count*unit;return true;
  }
  static bool align(size_t &total,size_t alignment){return add(total,(alignment-total%alignment)%alignment);}
  Layout *layouts()const{return static_cast<Layout *>(active_arena_);}
  PageId restored_page(const PageStore &next)const {
    if(active_page_==NO_PAGE || active_page_==0)return 0;
    const auto *old_page=active_store_.page(active_page_);
    const auto *old_owner=old_page==nullptr?nullptr:active_store_.card(old_page->owner);
    if(old_owner==nullptr || old_owner->slot==0 || old_owner->slot>next.page(0)->card_count)return 0;
    const auto *owner=next.card(next.page(0)->first_card+old_owner->slot-1);
    if(owner==nullptr || owner->target==NO_PAGE || owner->target>=next.page_count() ||
       !next.page(owner->target)->navigable)return 0;
    return owner->target;
  }
  void unbind_view() {
    view_binding_=0;
    for(size_t shell=0;shell<Slots;++shell) {
      const auto retired=bindings_[shell];bindings_[shell]={};
      if(retired.binding!=0)port_.unbind(shell,retired);
    }
  }
  void bind_view(PageId page,uint32_t generation) {
    active_page_=page;view_binding_=generation;refresh_=true;++memory_.view_changes;
    const auto &layout=layouts()[page];
    for(size_t shell=0;shell<Slots;++shell) {
      const auto &cell=layout.cells[shell];if(!cell.visible)continue;
      bindings_[shell]={active_store_.generation(),generation,page,cell.card,static_cast<uint16_t>(shell),cell.back};
      port_.bind(shell,active_store_,cell,layout.back,bindings_[shell]);++memory_.shell_binds;
    }
  }
  const DeviceUiLimits &limits_;PageArenaAllocator &allocator_;PageRendererPort &port_;
  void *active_arena_{nullptr},*staged_arena_{nullptr};size_t active_bytes_{0},staged_bytes_{0};
  PageStore active_store_;
  PageViewGeometry staged_geometry_;
  std::array<PageBindingToken,Slots> bindings_{};
  uint32_t next_binding_{1},staged_binding_{0},view_binding_{0};PageId active_page_{NO_PAGE};
  bool capacity_exceeded_{false};
  bool changing_{false},suspended_{false},refresh_{false};PageRendererMemory memory_;
};
}  // namespace espcontrol::pages
