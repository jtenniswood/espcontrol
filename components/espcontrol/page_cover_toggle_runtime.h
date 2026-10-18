#pragma once
#include "page_entity_state_runtime.h"
#include "page_status_captions.h"
namespace espcontrol::pages {
using PageCoverStatusCaptions = PageStatusCaptions;
struct PageCoverToggleSnapshot {
  bool on{false}, icon_open{false}, showing{false};
  std::string_view status;
};
struct PageCoverToggleMemory {
  size_t fixed_bytes{0}, active_bytes{0}, staged_bytes{0}, peak_bytes{0}, sources{0}, failures{0};
};
// Ordered, localized transient history shared by cover-toggle consumers.
// Terminal states revert after three seconds; movement/unknown states stay visible.
// Prepare before the graph; views borrow snapshots and no hidden timer/widget is retained.
class PageCoverToggleRuntime final : public PageRuntimeResources, public PageEntityObserver {
 public:
  using Now = uint32_t (*)();
  PageCoverToggleRuntime(PageArenaAllocator&, PageEntitySubscriptions&, Now, size_t max_sources = 32,
                         size_t max_key_bytes = 2048, PageCoverStatusCaptions captions = {});
  ~PageCoverToggleRuntime() override;
  PageCoverToggleRuntime(const PageCoverToggleRuntime&) = delete;
  PageCoverToggleRuntime& operator=(const PageCoverToggleRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void observed(uint32_t, std::string_view, std::string_view, std::string_view, bool) override;
  void loop();
  PageCoverToggleSnapshot read(ConfigurationHandle) const;
  uint64_t revision() const {
    return revision_;
  }
  PageCoverToggleMemory memory() const;
  static bool supported(const PageStore&, CardId);

 private:
  struct Source {
    std::string_view entity;
    const char* literal{nullptr};
    char caption[97]{};
    uint32_t changed_ms{0};
    bool on{false}, icon_open{false}, has_status{false}, showing{false}, revert{false};
  };
  struct Slot {
    void* arena{nullptr};
    size_t bytes{0}, count{0};
    Source* sources{nullptr};
    PageStore store;
  };
  void clear(Slot&);
  void update(Source&, std::string_view);
  static const char* text(const Source& source) {
    return source.literal ? source.literal : source.caption;
  }
  PageArenaAllocator& allocator_;
  PageEntitySubscriptions& subscriptions_;
  Now now_;
  size_t max_sources_, max_key_bytes_, peak_{0}, failures_{0};
  PageCoverStatusCaptions captions_;
  uint32_t connection_;
  uint64_t revision_{0};
  Slot active_, staged_;
};
}  // namespace espcontrol::pages
