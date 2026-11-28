#pragma once

#include "display_native_flush.h"

#include "driver/ppa.h"
#include "esp_heap_caps.h"
#include "esphome/components/lvgl/lvgl_esphome.h"
#include "esphome/components/mipi_dsi/mipi_dsi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

namespace espcontrol::display_native_flush {

class FlushTask {
 public:
  size_t attach(esphome::lvgl::LvglComponent &lvgl, esphome::mipi_dsi::MipiDsi &panel) {
    auto *display = lvgl.get_disp();
    if (display_ != nullptr || display == nullptr || panel.is_failed() || lvgl.draw_rounding != 2 ||
        lv_display_get_render_mode(display) != LV_DISPLAY_RENDER_MODE_PARTIAL ||
        lv_display_get_color_format(display) != LV_COLOR_FORMAT_RGB565 || lv_display_is_double_buffered(display)) return 0;

    auto *first = lv_display_get_buf_active(display);
    original_flush_ = lv_display_get_flush_cb(display);
    if (first == nullptr || first->data_size == 0 || original_flush_ == nullptr) return 0;
    const size_t bytes = aligned_(first->data_size);
    second_ = allocate_(bytes);
    rotated_ = allocate_(bytes);
    queue_ = xQueueCreate(1, sizeof(Job));
    done_ = xSemaphoreCreateBinary();

    ppa_client_config_t config{};
    config.oper_type = PPA_OPERATION_SRM;
    config.max_pending_trans_num = 1;
    if (second_ == nullptr || rotated_ == nullptr || queue_ == nullptr || done_ == nullptr ||
        ppa_register_client(&config, &ppa_) != ESP_OK) {
      cleanup_();
      return 0;
    }

    panel_ = &panel;
    if (xTaskCreatePinnedToCore(run_, "v3_display", 4096, this, uxTaskPriorityGet(nullptr), &worker_,
                                1 - xPortGetCoreID()) != pdPASS) {
      cleanup_();
      return 0;
    }

    display_ = display;
    lvgl_ = &lvgl;
    capacity_ = bytes;
    lv_display_set_buffers(display, first->data, second_, first->data_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush_);
    lv_display_set_flush_wait_cb(display, [](lv_display_t *) { instance().finish_(); });
    lv_display_add_event_cb(display, [](lv_event_t *) { instance().finish_(); }, LV_EVENT_REFR_READY, nullptr);
    return 2 * bytes;
  }

  uint32_t processed_regions() const { return processed_regions_; }
  TaskHandle_t worker() const { return worker_; }

  static FlushTask &instance() {
    static FlushTask task;
    return task;
  }

 private:
  struct Job {
    lv_area_t area;
    uint8_t *pixels;
    Region input;
    Region output;
  };

  static constexpr size_t ALIGNMENT = 64;

  static size_t aligned_(size_t bytes) { return (bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1); }

  static uint8_t *allocate_(size_t bytes) {
    return static_cast<uint8_t *>(heap_caps_aligned_alloc(ALIGNMENT, bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  }

  void cleanup_() {
    if (ppa_ != nullptr) ppa_unregister_client(ppa_);
    if (queue_ != nullptr) vQueueDelete(queue_);
    if (done_ != nullptr) vSemaphoreDelete(done_);
    heap_caps_free(second_);
    heap_caps_free(rotated_);
    ppa_ = nullptr;
    queue_ = nullptr;
    done_ = nullptr;
    second_ = nullptr;
    rotated_ = nullptr;
  }

  static void flush_(lv_display_t *display, const lv_area_t *area, uint8_t *pixels) {
    auto &task = instance();
    task.finish_();
    Job job{*area, pixels, {area->x1, area->y1, lv_area_get_width(area), lv_area_get_height(area)}, {}};
    // LVGL waits before submitting its next region. The refresh-end callback
    // also drains the last region before ESPHome can write the panel again.
    if (!task.lvgl_->is_paused() &&
        task.lvgl_->get_rotation() == esphome::display::DISPLAY_ROTATION_90_DEGREES &&
        plan_clockwise_region(job.input, task.panel_->get_width_internal(), task.panel_->get_height_internal(),
                              task.capacity_, job.output) && xQueueSend(task.queue_, &job, 0) == pdTRUE) {
      task.pending_job_ = job;
      task.pending_ = true;
      if (lv_display_flush_is_last(display)) task.finish_();
      return;
    }
    task.original_flush_(display, area, pixels);
  }

  void finish_() {
    if (!pending_) return;
    // Never relinquish LVGL's source buffer before the worker is finished.
    // The ESPHome loop watchdog remains the last resort for a wedged driver.
    xSemaphoreTake(done_, portMAX_DELAY);
    pending_ = false;
    if (rotated_ok_) {
      ++processed_regions_;
      lv_display_flush_ready(display_);
    } else {
      original_flush_(display_, &pending_job_.area, pending_job_.pixels);
    }
  }

  static void run_(void *context) {
    auto &task = *static_cast<FlushTask *>(context);
    Job job;
    for (;;) {
      if (xQueueReceive(task.queue_, &job, portMAX_DELAY) != pdTRUE) continue;
      task.rotated_ok_ = task.rotate_and_transfer_(job);
      xSemaphoreGive(task.done_);
    }
  }

  bool rotate_and_transfer_(const Job &job) {
    ppa_srm_oper_config_t config{};
    config.in.buffer = job.pixels;
    config.in.pic_w = config.in.block_w = job.input.width;
    config.in.pic_h = config.in.block_h = job.input.height;
    config.in.srm_cm = PPA_SRM_COLOR_MODE_RGB565;
    config.out.buffer = rotated_;
    config.out.buffer_size = aligned_(static_cast<size_t>(job.output.width) * job.output.height * sizeof(uint16_t));
    config.out.pic_w = job.output.width;
    config.out.pic_h = job.output.height;
    config.out.srm_cm = PPA_SRM_COLOR_MODE_RGB565;
    config.rotation_angle = PPA_SRM_ROTATION_ANGLE_270;
    config.scale_x = config.scale_y = 1.0f;
    config.mode = PPA_TRANS_MODE_BLOCKING;
    if (ppa_do_scale_rotate_mirror(ppa_, &config) != ESP_OK) return false;

    static_cast<esphome::display::Display &>(*panel_).draw_pixels_at(
        job.output.x, job.output.y, job.output.width, job.output.height, rotated_,
        esphome::display::COLOR_BITNESS_565, false);
    return true;
  }

  esphome::lvgl::LvglComponent *lvgl_{nullptr};
  esphome::mipi_dsi::MipiDsi *panel_{nullptr};
  lv_display_t *display_{nullptr};
  lv_display_flush_cb_t original_flush_{nullptr};
  QueueHandle_t queue_{nullptr};
  SemaphoreHandle_t done_{nullptr};
  TaskHandle_t worker_{nullptr};
  ppa_client_handle_t ppa_{nullptr};
  uint8_t *second_{nullptr};
  uint8_t *rotated_{nullptr};
  size_t capacity_{0};
  Job pending_job_{};
  uint32_t processed_regions_{0};
  bool pending_{false};
  bool rotated_ok_{false};
};

}  // namespace espcontrol::display_native_flush
