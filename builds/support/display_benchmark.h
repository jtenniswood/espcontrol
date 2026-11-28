#pragma once

#include <array>
#include <cinttypes>
#include <cstdio>

#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "esphome/components/touchscreen/touchscreen.h"
#include "esphome/core/log.h"

namespace espcontrol::display_benchmark {

// This observer is local to the instrumented build. Keep the callbacks
// allocation-free and batch the output outside rendering and touch handling.
class Probe final : public esphome::touchscreen::TouchListener {
 public:
  void attach(lv_display_t *display, esphome::touchscreen::Touchscreen &touchscreen) {
    if (display == nullptr || attached_) return;
    draw_buffer_bytes_ = lv_display_get_draw_buf_size(display);
    lv_display_add_event_cb(display, display_event_, LV_EVENT_ALL, this);
    touchscreen.register_listener(this);
    attached_ = true;
  }

  void touch(esphome::touchscreen::TouchPoint) override { record_('T'); }
  void release() override { record_('U'); }
  void set_dsi_staging(size_t bytes, uint32_t (*submissions)()) {
    dsi_staging_bytes_ = bytes;
    dsi_staged_submissions_ = submissions;
  }

  void report() {
    if (!attached_) return;

    constexpr auto INTERNAL = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    constexpr auto EXTERNAL = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
    using Runtime = decltype(TaskStatus_t{}.ulRunTimeCounter);
    Runtime total_runtime = 0;
    const auto task_count = uxTaskGetSystemState(tasks_.data(), tasks_.size(), &total_runtime);
    ESP_LOGI(TAG, "sample time_us=%" PRId64 " internal_free=%zu internal_min=%zu internal_largest=%zu "
             "psram_free=%zu psram_min=%zu psram_largest=%zu draw_buffer_bytes=%" PRIu32
             " dsi_staging_bytes=%zu dsi_staged_submissions=%" PRIu32 " task_count=%u total_runtime=%" PRIu64 " lost=%u",
             esp_timer_get_time(),
             heap_caps_get_free_size(INTERNAL), heap_caps_get_minimum_free_size(INTERNAL),
             heap_caps_get_largest_free_block(INTERNAL), heap_caps_get_free_size(EXTERNAL),
             heap_caps_get_minimum_free_size(EXTERNAL), heap_caps_get_largest_free_block(EXTERNAL),
             draw_buffer_bytes_, dsi_staging_bytes_, dsi_staged_submissions_ != nullptr ? dsi_staged_submissions_() : 0,
             static_cast<unsigned>(task_count),
             static_cast<uint64_t>(total_runtime), lost_);
    for (unsigned i = 0; i < task_count; ++i) {
      const auto &task = tasks_[i];
      // The snapshot holds a pointer to the task name. A worker can exit
      // before we log, so use the copied task number and stable idle handles.
      int idle_core = -1;
      for (int core = 0; core < configNUMBER_OF_CORES; ++core) {
        if (task.xHandle == xTaskGetIdleTaskHandleForCore(core)) idle_core = core;
      }
      ESP_LOGI(TAG, "task id=%u idle_core=%d runtime=%" PRIu64, static_cast<unsigned>(task.xTaskNumber),
               idle_core, static_cast<uint64_t>(task.ulRunTimeCounter));
    }

    char line[420];
    size_t pos = 0;
    for (size_t i = 0; i < used_; ++i) {
      const auto &event = events_[i];
      pos += std::snprintf(line + pos, sizeof(line) - pos, " %c:%" PRId64, event.code, event.time_us);
      // A signed 64-bit timestamp with a separator and event code takes at most 24 bytes.
      if (pos >= sizeof(line) - 24 || i + 1 == used_) {
        ESP_LOGI(TAG, "events%s", line);
        pos = 0;
      }
    }
    used_ = 0;
    lost_ = 0;
  }

 private:
  struct Event {
    int64_t time_us;
    char code;
  };

  static void display_event_(lv_event_t *event) {
    auto &probe = *static_cast<Probe *>(lv_event_get_user_data(event));
    switch (lv_event_get_code(event)) {
      case LV_EVENT_RENDER_START: probe.record_('D'); break;
      case LV_EVENT_RENDER_READY: probe.record_('R'); break;
      case LV_EVENT_FLUSH_START: probe.record_('F'); break;
      case LV_EVENT_FLUSH_FINISH: probe.record_('C'); break;
      case LV_EVENT_FLUSH_WAIT_START: probe.record_('W'); break;
      case LV_EVENT_FLUSH_WAIT_FINISH: probe.record_('A'); break;
      case LV_EVENT_REFR_READY: probe.record_('V'); break;
      default: break;
    }
  }

  void record_(char code) {
    if (used_ == events_.size()) {
      ++lost_;
      return;
    }
    events_[used_++] = {esp_timer_get_time(), code};
  }

  static constexpr const char *TAG = "display_benchmark";
  std::array<Event, 512> events_{};
  std::array<TaskStatus_t, 48> tasks_{};
  uint32_t draw_buffer_bytes_{0};
  size_t dsi_staging_bytes_{0};
  uint32_t (*dsi_staged_submissions_)() = nullptr;
  size_t used_{0};
  unsigned lost_{0};
  bool attached_{false};
};

inline Probe &probe() {
  static Probe instance;
  return instance;
}

}  // namespace espcontrol::display_benchmark
