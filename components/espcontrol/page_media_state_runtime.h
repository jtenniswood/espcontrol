#pragma once

#include "ha_state_limits.h"
#include "media_input_state.h"
#include "media_metadata_policy.h"
#include "media_playback_clock.h"
#include "page_entity_state_runtime.h"

namespace espcontrol::pages {
class PageMediaTransport {
 public:
  virtual ~PageMediaTransport() = default;
  virtual bool connected() const = 0;
  virtual uint32_t connection_generation() const = 0;
  virtual uint32_t now_ms() const = 0;
  virtual int64_t epoch_seconds() const = 0;
  virtual void reannounce() = 0;
};
struct PageMediaPlaybackSnapshot {
  float seconds{0}, duration{0};
  uint8_t percent{0};
  bool available{false}, playing{false}, has_duration{false}, has_position{false};
};
struct PageMediaMemory {
  size_t active_bytes{0}, staged_bytes{0}, peak_bytes{0}, sources{0}, reannouncements{0}, ignored{0};
};
// Configuration-owned metadata freshness, independent of visible widgets. Raw
// values remain in PageEntityStateRuntime rather than being copied here.
// Prepare this owner before the state graph so staged live deliveries are seen.
class PageMediaStateRuntime final : public PageRuntimeResources, public PageEntityObserver {
 public:
  PageMediaStateRuntime(PageArenaAllocator& allocator, PageMediaTransport& transport)
      : allocator_(allocator),
        transport_(transport),
        connection_(transport.connection_generation()),
        tick_ms_(transport.now_ms()) {}
  ~PageMediaStateRuntime() override;
  PageMediaStateRuntime(const PageMediaStateRuntime&) = delete;
  PageMediaStateRuntime& operator=(const PageMediaStateRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void observed(uint32_t, std::string_view, std::string_view, std::string_view, bool) override;
  void observed_update(uint32_t, std::string_view, std::string_view, std::string_view, bool,
                       std::string_view) override;
  bool current(ConfigurationHandle, std::string_view entity, std::string_view attribute) const;
  bool playback(ConfigurationHandle, std::string_view entity, PageMediaPlaybackSnapshot&) const;
  bool external_input(ConfigurationHandle, std::string_view entity) const;
  uint32_t now_ms() const {
    return transport_.now_ms();
  }
  uint32_t connection_generation() const {
    return transport_.connection_generation();
  }
  void loop();
  uint64_t revision() const {
    return revision_;
  }
  PageMediaMemory memory() const;

 private:
  struct Source {
    std::string_view entity;
    uint16_t fresh{0}, present{0};
    bool state_known{false}, resync{false};
    uint8_t requirements{0};
    char state[HA_SHORT_STATE_MAX_LEN + 1]{};
    media::PlaybackClock clock;
    media::MediaInputState input;
    uint64_t content_fingerprint{0};
    uint32_t title_refresh_started_ms{0};
    media::MediaItemKind content_kind{media::MediaItemKind::UNKNOWN},
        type_kind{media::MediaItemKind::UNKNOWN};
    bool title_awaiting_refresh{false}, metadata_refresh{false};
  };
  struct Slot {
    void* arena{nullptr};
    Source* sources{nullptr};
    size_t count{0}, bytes{0};
    uint32_t generation{0};
    size_t card_count{0};
  };
  const Source* source(ConfigurationHandle, std::string_view entity) const;
  static uint16_t attribute_bit(std::string_view);
  void synchronize();
  void release(int);
  PageArenaAllocator& allocator_;
  PageMediaTransport& transport_;
  Slot slots_[2];
  int active_{-1}, staged_{-1};
  uint32_t connection_, tick_ms_;
  uint64_t revision_{0};
  PageMediaMemory memory_;
};
}  // namespace espcontrol::pages
