#pragma once

#include <cstdint>
#include <limits>

#include "ha_state_limits.h"
#include "page_entity_state_runtime.h"
#include "page_local_sensor_feed.h"

namespace espcontrol::pages {
constexpr uint32_t NO_LOCAL_SENSOR = UINT32_MAX;
// Physical producer handles are stable for the firmware lifetime. A revision
// advances for every report, including a repeated empty text report.
struct PageLocalSensorSample {
  uint64_t revision{0};
  float number{std::numeric_limits<float>::quiet_NaN()};
  std::string_view text;
};
class PageLocalSensorProvider {
 public:
  virtual ~PageLocalSensorProvider() = default;
  virtual size_t capacity() const = 0;
  virtual uint32_t resolve(std::string_view key, bool text) const = 0;
  virtual bool watch(uint32_t source) = 0;
  virtual PageLocalSensorSample read(uint32_t source) const = 0;
  virtual size_t fixed_bytes() const {
    return 0;
  }
  virtual size_t callbacks() const {
    return 0;
  }
};
struct PageLocalSensorMemory {
  size_t fixed_bytes{0}, binding_bytes{0}, peak_binding_bytes{0}, live_bindings{0};
  size_t producer_fixed_bytes{0}, producer_callbacks{0};
  PageEntityMemory state;
  size_t feed_fixed_bytes{sizeof(PageLocalSensorFeed)};
};
class PageLocalSensorDependencies final : public PageDependencySource {
 public:
  static bool local(const PageStore&, const CardDefinition&);
  static bool has_values(const PageStore&);
  bool enumerate(const PageStore&, PageDependencySink&) const override;
};
// Local data uses the same transactional packed cache, with an independent
// connection generation. HA reconnects cannot invalidate device-local values.
class PageLocalSensorRuntime final : public PageRuntimeResources {
 public:
  PageLocalSensorRuntime(PageArenaAllocator&, PageLocalSensorProvider&, PageEntityBudget,
                         size_t text_bytes = HA_TEXT_SENSOR_STATE_MAX_LEN);
  ~PageLocalSensorRuntime() override;
  PageLocalSensorRuntime(const PageLocalSensorRuntime&) = delete;
  PageLocalSensorRuntime& operator=(const PageLocalSensorRuntime&) = delete;
  bool prepare(const PageStore&) override;
  void discard_prepared() override;
  void activate_prepared(const PageStore&) override;
  void retire_active() override;
  void loop();
  bool install_manual_feed(bool legacy_renderer_retired);
  uint64_t revision() {
    return graph_.revision();
  }
  PageEntityValue read(ConfigurationHandle, std::string_view key, bool text);
  PageLocalSensorMemory memory() const;
  size_t text_capacity() const {
    return text_bytes_;
  }

 private:
  class Subscriptions final : public PageEntitySubscriptions {
   public:
    Subscriptions(PageArenaAllocator&, PageLocalSensorProvider&, size_t channels);
    ~Subscriptions() override;
    bool subscribe(const char*, const char*, void*, uint32_t, uint32_t, Deliver) override;
    void release(void*) override;
    uint32_t connection_generation() const override {
      return 1;
    }
    bool copy_shared_values() const override {
      return false;
    }
    void loop();
    void cleanup();
    bool feed(std::string_view, bool, float, std::string_view);
    struct Binding {
      void* owner{nullptr};
      Deliver deliver{nullptr};
      const char* key{nullptr};
      uint64_t seen{0};
      uint32_t revision{0}, channel{0}, source{NO_LOCAL_SENSOR};
      bool text{false};
    };
    bool initialize();
    void deliver(Binding&, const PageLocalSensorSample&, bool initial);
    PageArenaAllocator& allocator;
    PageLocalSensorProvider& provider;
    Binding* bindings{nullptr};
    size_t capacity{0}, bytes{0}, peak{0}, live{0};
  };
  PageLocalSensorProvider& provider_;
  PageLocalSensorDependencies source_;
  Subscriptions subscriptions_;
  PageEntityStateRuntime graph_;
  size_t text_bytes_{0};
  uint32_t prepared_empty_{0};
  bool prepared_empty_valid_{false};
};
}  // namespace espcontrol::pages
