#pragma once

#include <array>
#include "page_runtime_resources.h"

namespace espcontrol::pages {
// Put view/modal unbind owners before shared state owners. Their activation
// then cancels old view work before its configuration/state storage is retired.
// Every participant, and the group itself, is owned by the application lifetime.
template<size_t Count>
class PageRuntimeResourcesGroup final : public PageRuntimeResources {
 public:
  explicit PageRuntimeResourcesGroup(std::array<PageRuntimeResources *,Count> resources) : resources_(resources) {}
  PageRuntimeResourcesGroup(const PageRuntimeResourcesGroup &)=delete;
  PageRuntimeResourcesGroup &operator=(const PageRuntimeResourcesGroup &)=delete;
  bool prepare(const PageStore &store) override { return prepare_impl(store, false); }
  bool prepare_write(const PageStore &store) override { return prepare_impl(store, true); }
  bool preparation_capacity_exceeded() const override { return capacity_exceeded_; }
  void discard_prepared() override {
    for(auto *resource:resources_) if(resource!=nullptr)resource->discard_prepared();
    prepared_=false;
  }
  void activate_prepared(const PageStore &active) override {
    if(!prepared_)return;
    for(auto *resource:resources_)resource->activate_prepared(active);
    prepared_=false;
  }
  void retire_active() override {
    for(auto *resource:resources_)if(resource!=nullptr)resource->retire_active();
  }
 private:
  bool prepare_impl(const PageStore &store, bool write) {
    discard_prepared();capacity_exceeded_=false;
    for(auto *resource:resources_) {
      if(resource==nullptr || !(write?resource->prepare_write(store):resource->prepare(store))) {
        capacity_exceeded_=resource&&resource->preparation_capacity_exceeded();
        discard_prepared();return false;
      }
    }
    prepared_=true;return true;
  }
  std::array<PageRuntimeResources *,Count> resources_;
  bool prepared_{false},capacity_exceeded_{false};
};
}  // namespace espcontrol::pages
