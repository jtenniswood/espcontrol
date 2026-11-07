#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "page_label_refresh.h"
using namespace espcontrol::pages;
static size_t allocations = 0, live_bytes = 0;
struct alignas(std::max_align_t) Allocation {size_t bytes;};
extern "C" {
void lv_mem_init() {}
void lv_mem_deinit() {}
lv_mem_pool_t lv_mem_add_pool(void *,size_t) {return nullptr;}
void lv_mem_remove_pool(lv_mem_pool_t) {}
void *lv_malloc_core(size_t size) {++allocations;auto *p=static_cast<Allocation *>(std::malloc(size+sizeof(Allocation)));assert(p);p->bytes=size;live_bytes+=size;return p+1;}
void lv_free_core(void *ptr) {if(ptr){auto *p=static_cast<Allocation *>(ptr)-1;live_bytes-=p->bytes;std::free(p);}}
void *lv_realloc_core(void *ptr,size_t size) {if(!ptr)return lv_malloc_core(size);if(!size){lv_free_core(ptr);return nullptr;}auto *p=static_cast<Allocation *>(ptr)-1;const auto old=p->bytes;auto *next=lv_malloc_core(size);std::memcpy(next,ptr,std::min(size,old));lv_free_core(ptr);return next;}
void lv_mem_monitor_core(lv_mem_monitor_t *m) {std::memset(m,0,sizeof(*m));}
lv_result_t lv_mem_test_core() {return LV_RESULT_OK;}
void __wrap_lv_display_add_event_cb(lv_display_t *, lv_event_cb_t, lv_event_code_t, void *);
uint32_t __wrap_lv_display_remove_event_cb_with_user_data(lv_display_t *, lv_event_cb_t, void *);
}
struct Arena:PageArenaAllocator {
  bool fail{false}; size_t bytes{0};
  void *allocate(size_t size)override {if(fail)return nullptr;bytes+=size;return std::malloc(size);}
  void release(void *p,size_t size)override {bytes-=size;std::free(p);}
};
void cycle(size_t count) {
  auto *display=lv_display_create(480,480);assert(display);
  auto *root=lv_obj_create(lv_display_get_screen_active(display));
  std::array<lv_obj_t *,80>labels{};
  std::array<std::array<char,32>,80>text{};
  for(size_t i=0;i<count;++i) {
    labels[i]=lv_label_create(root);lv_obj_set_size(labels[i],160,28);
    lv_obj_set_style_text_font(labels[i],&lv_font_montserrat_14,LV_PART_MAIN);
  }
  struct Victim {lv_obj_t *label;bool armed{false};} victim{labels[1]};
  lv_display_add_event_cb(display,[](lv_event_t *e){
    auto *v=static_cast<Victim *>(lv_event_get_user_data(e));
    if(v->armed){v->armed=false;lv_obj_delete(v->label);v->label=nullptr;}
  },LV_EVENT_UPDATE_LAYOUT_COMPLETED,&victim);
  Arena arena;
  {
    PageLabelRefresh cache(arena);
    assert(!cache.initialize(nullptr,0));
    auto invalid=labels;invalid[1]=invalid[0];assert(!cache.initialize(invalid.data(),count));
    arena.fail=true;assert(!cache.initialize(labels.data(),count));arena.fail=false;
    assert(cache.initialize(labels.data(),count));
    assert(cache.memory().retained_callbacks==count && cache.memory().live_labels==count);
    PageLabelRefresh other(arena);assert(!other.initialize(labels.data(),count));
    const auto events=lv_display_get_event_count(display);
    const auto before=allocations,steady=live_bytes;
    for(size_t n=0;n<1000;++n) {
      for(size_t i=0;i<count;++i) {
        std::snprintf(text[i].data(),text[i].size(),"%zu / %zu °C",n,i);
        lv_label_set_text_static(labels[i],text[i].data());
      }
      lv_obj_update_layout(root);
      assert(allocations==before && live_bytes==steady && lv_display_get_event_count(display)==events);
    }
    assert(cache.memory().reused_adds>=count*1000);
    // Other callbacks using a bank label as their user data still add/remove
    // normally. Only the discovered private callback/filter is retained.
    const auto cb=+[](lv_event_t *){};
    __wrap_lv_display_add_event_cb(display,cb,LV_EVENT_UPDATE_LAYOUT_COMPLETED,labels[0]);
    assert(lv_display_get_event_count(display)==events+1);
    assert(__wrap_lv_display_remove_event_cb_with_user_data(display,cb,labels[0])==1);
    // An unrelated display's labels keep the original allocation/lifetime path.
    auto *second=lv_display_create(100,100);auto *untracked=lv_label_create(lv_display_get_screen_active(second));
    lv_obj_update_layout(untracked);const auto normal=allocations;
    lv_label_set_text_static(untracked,"Untracked");lv_obj_update_layout(untracked);assert(allocations>normal);
    lv_display_delete(second);lv_display_set_default(display);
    // Cleanup is refused while an event callback is still running.
    bool refused=false;
    void *args[]={&cache,&refused};
    lv_obj_add_event_cb(root,[](lv_event_t *e){auto **v=static_cast<void **>(lv_event_get_user_data(e));*static_cast<bool *>(v[1])=!static_cast<PageLabelRefresh *>(v[0])->shutdown();},LV_EVENT_CLICKED,args);
    lv_obj_send_event(root,LV_EVENT_CLICKED,nullptr);assert(refused);
    victim.armed=true;lv_obj_update_layout(root);assert(victim.label==nullptr);labels[1]=nullptr;
    assert(cache.memory().live_labels==count-1 && cache.memory().retained_callbacks==count-1);
    // A pending update must be completed before callbacks are released.
    lv_label_set_text_static(labels[0],"Pending shutdown");assert(cache.shutdown());assert(arena.bytes==0);
    const auto normal_again=allocations;
    lv_label_set_text_static(labels[0],"Ordinary again");lv_obj_update_layout(root);assert(allocations>normal_again);
    assert(cache.initialize(labels.data()+2,count-2));
    lv_display_delete(display);display=nullptr;
    assert(cache.memory().live_labels==0 && cache.memory().retained_callbacks==0);
    assert(cache.shutdown() && arena.bytes==0);
  }
}
int main() {lv_init();for(size_t n:{size_t(24),size_t(36),size_t(60),size_t(80)})cycle(n);lv_deinit();assert(live_bytes==0);}
