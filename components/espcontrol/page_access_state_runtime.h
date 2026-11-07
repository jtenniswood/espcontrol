#pragma once

#include "page_entity_state_runtime.h"
#include "page_status_captions.h"

namespace espcontrol::pages {
enum class PageAccessHistory : uint8_t { NONE, TRANSIENT, PERSISTENT, LOCK };
struct PageAccessSnapshot {
  bool on{false}, icon_on{false}, available{false}, locked{false}, showing{false}, reported{false};
  std::string_view status;
};
struct PageAccessMemory {
  size_t fixed_bytes{0}, active_bytes{0}, staged_bytes{0}, peak_bytes{0}, histories{0}, failures{0};
};
// Configuration-owned status history for garage/gate status cards and lock toggles.
// A source/policy pair is shared; persistent and transient consumers retain independent deadlines.
class PageAccessStateRuntime final : public PageRuntimeResources, public PageEntityObserver {
 public:
  using Now = uint32_t (*)();
  PageAccessStateRuntime(PageArenaAllocator&, PageEntitySubscriptions&, Now, size_t max_histories = 64,
                         size_t max_key_bytes = 4096, PageStatusCaptions captions = {});
  ~PageAccessStateRuntime() override;
  PageAccessStateRuntime(const PageAccessStateRuntime&) = delete;
  PageAccessStateRuntime& operator=(const PageAccessStateRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void observed(uint32_t, std::string_view, std::string_view, std::string_view, bool) override;
  void loop();
  PageAccessSnapshot read(ConfigurationHandle) const;
  bool owns(ConfigurationHandle handle) const {
    const auto* card = active_.store.runtime_card(handle);
    if (!active_.store.source_document() || handle.generation != active_.store.generation() || !card)
      return false;
    const auto type = active_.store.field(*card, CardField::TYPE);
    return type == "garage" || type == "gate" || type == "lock";
  }
  uint64_t revision() const {
    return revision_;
  }
  PageAccessMemory memory() const;
  // Preparation only: option decoding may allocate. Frames use the prepared card-to-history index.
  static PageAccessHistory classify(const PageStore&, CardId);

 private:
  struct History {
    std::string_view entity;
    const char* literal{nullptr};
    char caption[97]{};
    uint32_t changed_ms{0};
    PageAccessHistory policy{PageAccessHistory::NONE};
    bool on{false}, icon_on{false}, available{false}, locked{false}, has_status{false}, showing{false},
        revert{false};
  };
  struct Slot {
    void* arena{nullptr};
    size_t bytes{0}, count{0};
    History* histories{nullptr};
    uint16_t* card_history{nullptr};
    PageStore store;
  };
  void clear(Slot&);
  void update(History&, std::string_view);
  static const char* text(const History& source) {
    return source.literal ? source.literal : source.caption;
  }
  PageArenaAllocator& allocator_;
  PageEntitySubscriptions& subscriptions_;
  Now now_;
  size_t max_histories_, max_key_bytes_, peak_{0}, failures_{0};
  PageStatusCaptions captions_;
  uint32_t connection_;
  uint64_t revision_{0};
  Slot active_, staged_;
};
}  // namespace espcontrol::pages
