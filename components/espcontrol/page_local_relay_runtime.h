#pragma once
#include <string_view>

#include "page_store_runtime.h"

namespace espcontrol::pages {
struct PageLocalRelayFeed {
  void* owner{nullptr};
  void (*changed)(void*, std::string_view, bool){nullptr};
  void notify(std::string_view key, bool on) const {
    const auto feed = *this;
    if (feed.owner && feed.changed) feed.changed(feed.owner, key, on);
  }
};
inline PageLocalRelayFeed& page_local_relay_feed() {
  static PageLocalRelayFeed feed;
  return feed;
}
struct PageLocalRelaySnapshot {
  bool on{false};
  std::string_view label;
};
class PageLocalRelayProvider {
 public:
  virtual ~PageLocalRelayProvider() = default;
  // Registry-backed reads run on the application loop and dispatch no actions.
  virtual PageLocalRelaySnapshot read(std::string_view key) const = 0;
};
struct PageLocalRelayMemory {
  size_t fixed_bytes{0}, feed_fixed_bytes{sizeof(PageLocalRelayFeed)}, active_bytes{0}, staged_bytes{0},
      peak_bytes{0};
  size_t active_sources{0}, preparation_failures{0};
};
class PageLocalRelayRuntime final : public PageRuntimeResources {
 public:
  PageLocalRelayRuntime(PageArenaAllocator&, PageLocalRelayProvider&, size_t max_sources = 32,
                        size_t max_key_bytes = 2048);
  ~PageLocalRelayRuntime() override;
  PageLocalRelayRuntime(const PageLocalRelayRuntime&) = delete;
  PageLocalRelayRuntime& operator=(const PageLocalRelayRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  bool install(bool legacy_renderer_retired);
  bool on(uint32_t generation, std::string_view key) const;
  std::string_view registered_label(std::string_view key) const {
    return provider_.read(key).label;
  }
  uint64_t revision() const {
    return revision_;
  }
  PageLocalRelayMemory memory() const;
  static bool toggle(const PageStore&, const CardDefinition&);

 private:
  struct Target {
    uint32_t offset{0};
    uint16_t length{0};
    bool on{false};
  };
  struct Slot {
    void* arena{nullptr};
    size_t bytes{0}, count{0};
    uint32_t generation{0};
    Target* targets{nullptr};
    char* names{nullptr};
  };
  void clear(Slot&);
  void changed(std::string_view, bool);
  PageArenaAllocator& allocator_;
  PageLocalRelayProvider& provider_;
  size_t max_sources_, max_key_bytes_, peak_{0}, failures_{0};
  uint64_t revision_{0};
  Slot active_, staged_;
};
}  // namespace espcontrol::pages
