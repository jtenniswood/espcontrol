#pragma once

#include <cassert>
#include <new>
#include "lvgl.h"
#include "page_modal_runtime.h"

namespace espcontrol::pages {
// One fallible reservation at modal setup. Entries survive DELETE callbacks
// until the modal owner can destroy the pool outside dispatch. Deleted widget
// slots are reusable when an option list is rebuilt, without growing storage.
class PageModalGestures {
 public:
  using Valid=bool (*)(void *,PageModalToken);
  PageModalGestures(PageArenaAllocator &allocator,size_t capacity,PageModalToken token,
                    void *owner,Valid valid)
      :allocator_(allocator),capacity_(capacity),token_(token),owner_(owner),valid_(valid) {
    if(capacity==0||capacity>SIZE_MAX/sizeof(Entry)||valid==nullptr)return;
    entries_=static_cast<Entry *>(allocator_.allocate(capacity*sizeof(Entry)));
    if(entries_)for(size_t i=0;i<capacity;++i)new(entries_+i)Entry{};
  }
  ~PageModalGestures(){assert(live_==0);if(entries_)allocator_.release(entries_,capacity_*sizeof(Entry));}
  PageModalGestures(const PageModalGestures &)=delete;
  PageModalGestures &operator=(const PageModalGestures &)=delete;
  bool available()const{return entries_!=nullptr;}
  size_t live()const{return live_;}
  size_t bytes()const{return entries_?capacity_*sizeof(Entry):0;}
  void invalidate(){enabled_=false;for(size_t i=0;entries_&&i<capacity_;++i)entries_[i].pressed=false;}
  // Call after creating the main panel or a nested/inline option list, before
  // returning to the event loop. On failure the caller must close that UI.
  bool attach(lv_obj_t *root) {
    if(!enabled_||!entries_||!root)return false;
    if(lv_obj_has_flag(root,LV_OBJ_FLAG_CLICKABLE)&&!track(root))return false;
    const uint32_t children=lv_obj_get_child_count(root);
    for(uint32_t i=0;i<children;++i)if(!attach(lv_obj_get_child(root,i)))return false;
    return true;
  }
 private:
  struct Entry {PageModalGestures *pool{nullptr};lv_obj_t *object{nullptr};bool pressed{false};};
  bool current()const{return enabled_&&valid_(owner_,token_);}
  bool track(lv_obj_t *object) {
    Entry *free=nullptr;
    for(size_t i=0;i<capacity_;++i) {
      if(entries_[i].object==object)return true;
      if(!entries_[i].object&&!free)free=entries_+i;
    }
    if(!free)return false;
    *free={this,object,false};
    const auto filter=static_cast<lv_event_code_t>(static_cast<unsigned>(LV_EVENT_ALL)|static_cast<unsigned>(LV_EVENT_PREPROCESS));
    if(!lv_obj_add_event_cb(object,event,filter,free)){*free={};return false;}
    ++live_;return true;
  }
  static void event(lv_event_t *event) {
    auto *entry=static_cast<Entry *>(lv_event_get_user_data(event));auto *pool=entry->pool;
    const auto code=lv_event_get_code(event);
    if(code==LV_EVENT_DELETE) {
      if(lv_event_get_target(event)!=entry->object)return;
      entry->object=nullptr;entry->pressed=false;assert(pool->live_>0);--pool->live_;return;
    }
    if(code==LV_EVENT_PRESS_LOST){entry->pressed=false;if(!pool->current())lv_event_stop_processing(event);return;}
    if(code==LV_EVENT_PRESSED||code==LV_EVENT_KEY||code==LV_EVENT_ROTARY) {
      entry->pressed=pool->current();if(!entry->pressed)lv_event_stop_processing(event);return;
    }
    if(code==LV_EVENT_CLICKED||code==LV_EVENT_SHORT_CLICKED||code==LV_EVENT_LONG_PRESSED||
        code==LV_EVENT_LONG_PRESSED_REPEAT||code==LV_EVENT_PRESSING||code==LV_EVENT_RELEASED||code==LV_EVENT_VALUE_CHANGED) {
      if(!entry->pressed||!pool->current())lv_event_stop_processing(event);
      if(code==LV_EVENT_CLICKED)entry->pressed=false;
    }
  }
  PageArenaAllocator &allocator_;size_t capacity_{0},live_{0};Entry *entries_{nullptr};
  PageModalToken token_;void *owner_{nullptr};Valid valid_{nullptr};bool enabled_{true};
};
}  // namespace espcontrol::pages
