#pragma once

// Device startup includes this after the clock, grid and concrete scene
// helpers.
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <utility>

#include "espcontrol_app.h"
#include "page_native_scene.h"

#ifdef USE_ESP32
#include "esp_heap_caps.h"
#endif

namespace espcontrol::pages {
// Scene/model storage is explicitly external. Exhaustion is reported through
// preparation failure; it must never consume the WiFi/DMA internal reserve.
class PageDeviceArena final : public PageArenaAllocator {
public:
  void *allocate(size_t bytes) override {
#ifdef USE_ESP32
    return bytes ? heap_caps_aligned_alloc(alignof(std::max_align_t), bytes,
                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
                 : nullptr;
#else
    return bytes ? std::malloc(bytes) : nullptr;
#endif
  }
  void release(void *memory, size_t) override {
#ifdef USE_ESP32
    heap_caps_free(memory);
#else
    std::free(memory);
#endif
  }
};

struct PageDeviceDisplayHooks {
  // Configuration/onboarding and rotation readiness belong to device wiring.
  std::function<bool()> ready;
  std::function<bool()> touch_allowed;
  std::function<void(DisplayTakeoverKind)> begin_takeover, end_takeover;
};

class PageNativeDevicePlatform final : public PageNativeScenePlatform {
public:
  PageNativeDevicePlatform(EspControlApp &app, lv_obj_t *grid,
                           DisplayModalProfile modal,
                           PageDeviceDisplayHooks hooks)
      : app_(app), grid_(grid), modal_(modal), hooks_(std::move(hooks)) {}
  bool ready() const {
    return grid_ && hooks_.ready && hooks_.touch_allowed &&
           hooks_.begin_takeover && hooks_.end_takeover;
  }
  const ThemePalette *palette() const override { return &current_theme(); }
  void apply_palette(const ThemePalette &palette) override {
    lv_obj_set_style_bg_color(grid_, lv_color_hex(palette.background),
                              LV_PART_MAIN);
  }
  bool grid_active() const override {
    return ready() && hooks_.ready() &&
           app_.display().target_mode_is(DisplayMode::ACTIVE) &&
           lv_screen_active() == grid_;
  }
  bool input_allowed() const override {
    return grid_active() && hooks_.touch_allowed() &&
           !screensaver_pin().locked();
  }
  std::string_view base_url() const override {
    return app_.home_assistant_endpoint().origin();
  }
  void refresh_camera_metadata(std::string_view entity) override {
    ha_schedule_metadata_refresh(std::string(entity), {"access_token", "entity_picture"},
                                 PageNativeSceneState::SUBSCRIPTION_SCOPE);
  }
  uint32_t endpoint_generation() const override {
    return app_.home_assistant_endpoint().generation();
  }
  bool request_allowed(uint16_t width, uint16_t height, bool) const override {
    if (!grid_active() || !width || !height)
      return false;
#ifdef USE_ESP32
    return esphome::artwork_image::image_pipeline_memory_failure(
               constrained(),
               heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
               heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL |
                                                MALLOC_CAP_8BIT),
               heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
               heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM |
                                                MALLOC_CAP_8BIT),
               image_card_estimated_buffer_bytes(width, height),
               image_card_estimated_pipeline_bytes(width, height),
               IMAGE_CARD_MEMORY_HEADROOM_BYTES,
               IMAGE_CARD_CONSTRAINED_INTERNAL_FREE_BYTES,
               IMAGE_CARD_CONSTRAINED_INTERNAL_LARGEST_BYTES) ==
           esphome::artwork_image::ImagePipelineMemoryFailure::NONE;
#else
    return true;
#endif
  }
  uint16_t maximum_source_side(bool) const override {
    return esphome::artwork_image::image_pipeline_modal_max_target_side(
        constrained());
  }
  bool retain_cache(uint16_t width, uint16_t height) const override {
    if (!width || !height)
      return false;
    if (!constrained())
      return true;
#ifdef USE_ESP32
    return esphome::artwork_image::image_pipeline_should_retain_modal_cache(
        true, true, false,
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
        heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
        image_card_estimated_buffer_bytes(width, height),
        image_card_estimated_pipeline_bytes(width, height),
        IMAGE_CARD_MEMORY_HEADROOM_BYTES);
#else
    return false;
#endif
  }
  uint32_t cache_ttl_ms() const override {
    return constrained() ? IMAGE_CARD_CONSTRAINED_MODAL_CACHE_TTL_MS : 0;
  }
  bool cache_skips_open_refresh() const override { return constrained(); }
  void critical(bool active) override {
    if (!ready() || active == critical_)
      return;
    critical_ = active;
    if (active)
      hooks_.begin_takeover(DisplayTakeoverKind::CRITICAL);
    else
      hooks_.end_takeover(DisplayTakeoverKind::CRITICAL);
  }
  void alarm_failure(PageAlarmFailure failure) override {
    if (!failure.valid)
      return;
    // One shared toast, independent of any current pooled shell/card widget.
    alarm_show_failure(nullptr, failure.reason == PageAlarmFailureReason::CODE
                                    ? "PIN was not accepted"
                                    : "Alarm did not change");
  }
  void chrome(const PageStore &store, PageId page, bool settings) override {
    std::string title;
    if (settings)
      title = espcontrol_i18n("Settings");
    else if (page != 0 && page != NO_PAGE) {
      const auto *definition = store.page(page);
      const auto *owner = definition ? store.card(definition->owner) : nullptr;
      if (owner)
        title = std::string(store.field(*owner, CardField::LABEL));
      if (title.empty())
        title = espcontrol_i18n("Subpage");
    }
    // Native panel families own the modal title. Only change the underlying
    // page/settings caption so an ordinary panel's caption remains intact.
    set_clock_bar_subpage_label(title);
  }
  void profile(DisplayModalProfile modal) { modal_ = modal; }

private:
  bool constrained() const { return display_modal_is_constrained(modal_); }
  EspControlApp &app_;
  lv_obj_t *grid_;
  DisplayModalProfile modal_;
  PageDeviceDisplayHooks hooks_;
  bool critical_{false};
};
} // namespace espcontrol::pages
