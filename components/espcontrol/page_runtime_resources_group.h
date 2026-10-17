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
  bool prepare(const PageStore &store) override {
    discard_prepared();
    for(auto *resource:resources_) {
      if(resource==nullptr || !resource->prepare(store)) {discard_prepared();return false;}
    }
    prepared_=true;return true;
  }
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
  std::array<PageRuntimeResources *,Count> resources_;
  bool prepared_{false};
};
}  // namespace espcontrol::pages
