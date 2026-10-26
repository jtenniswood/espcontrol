#pragma once

#include <cstring>
#include <cstdio>
#include <optional>
#include <utility>

#include "page_entity_state_runtime.h"

namespace espcontrol::pages {
// One temporary, terminated value owned for the complete legacy callback. A
// callback can close its modal or retire the source model/cache while reading.
class PageEntityReplaySnapshot {
 public:
  PageEntityReplaySnapshot(PageArenaAllocator &allocator,PageEntityStateRuntime &state,uint64_t revision,size_t size):allocator_(&allocator),size_(size),state_(&state),revision_(revision) {
    if(size<SIZE_MAX)bytes_=static_cast<char *>(allocator.allocate(size+1));
  }
  ~PageEntityReplaySnapshot(){release();}
  PageEntityReplaySnapshot(const PageEntityReplaySnapshot &)=delete;
  PageEntityReplaySnapshot &operator=(const PageEntityReplaySnapshot &)=delete;
  PageEntityReplaySnapshot(PageEntityReplaySnapshot &&other)noexcept
      :allocator_(other.allocator_),bytes_(other.bytes_),size_(other.size_),state_(other.state_),revision_(other.revision_){other.bytes_=nullptr;}
  PageEntityReplaySnapshot &operator=(PageEntityReplaySnapshot &&other)noexcept {
    if(this!=&other){release();allocator_=other.allocator_;bytes_=other.bytes_;size_=other.size_;state_=other.state_;revision_=other.revision_;other.bytes_=nullptr;}return *this;
  }
  char *data(){return bytes_;}
  const char *data()const{return bytes_;}
  size_t size()const{return size_;}
  bool current()const{return bytes_&&state_&&state_->revision()==revision_;}
 private:
  void release(){if(bytes_)allocator_->release(bytes_,size_+1);bytes_=nullptr;}
  PageArenaAllocator *allocator_;char *bytes_{nullptr};size_t size_;PageEntityStateRuntime *state_;uint64_t revision_;
};

// Synchronous main-loop source for HaReadCoordinator::replay_owner_missing.
// Only dependencies belonging to the requested card/generation can be seeded.
// Missing/stale/overflowed values and failed reservations remain unseeded so a
// later modal setup step can retry. Live announcements supersede initial replay.
class PageEntityModalReplay {
 public:
  PageEntityModalReplay(PageEntityStateRuntime &state,PageArenaAllocator &allocator,ConfigurationHandle card)
      :state_(state),allocator_(allocator),card_(card){}
  std::optional<PageEntityReplaySnapshot> operator()(std::string_view entity,std::string_view attribute)const {
    const auto revision=state_.revision();
    for(size_t ordinal=0;ordinal<state_.dependency_count(card_);++ordinal) {
      const auto value=state_.dependency(card_,ordinal);
      if(value.entity!=entity||value.attribute!=attribute)continue;
      // Cover callbacks ignore malformed position updates. Seed a new panel
      // with the channel's last valid value even when its latest raw text is bad.
      if(const auto percent=state_.last_percent(card_,ordinal)) {
        char text[4];const auto size=static_cast<size_t>(std::snprintf(text,sizeof(text),"%u",static_cast<unsigned>(*percent)));
        PageEntityReplaySnapshot snapshot(allocator_,state_,revision,size);
        if(!snapshot.data()||state_.revision()!=revision)return std::nullopt;
        std::memcpy(snapshot.data(),text,size+1);return snapshot;
      }
      if(value.status!=PageEntityStatus::AVAILABLE)return std::nullopt;
      PageEntityReplaySnapshot snapshot(allocator_,state_,revision,value.value.size());
      if(!snapshot.data()||state_.revision()!=revision)return std::nullopt;
      if(!value.value.empty())std::memcpy(snapshot.data(),value.value.data(),value.value.size());
      snapshot.data()[value.value.size()]='\0';return snapshot;
    }
    return std::nullopt;
  }
 private:
  PageEntityStateRuntime &state_;PageArenaAllocator &allocator_;ConfigurationHandle card_;
};
}  // namespace espcontrol::pages
