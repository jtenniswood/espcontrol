#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"

#ifdef USE_ESP_IDF

#include <atomic>

#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lvgl.h"

#elif defined(USE_HOST)

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <lvgl.h>

#endif  // USE_ESP_IDF / USE_HOST

#ifdef USE_ESP_IDF
#include "screen_wake_extern.h"
#endif

namespace esphome {
namespace lvgl_screenshot {

class LvglScreenshot : public Component {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  void set_port(uint16_t port) { this->port_ = port; }

 protected:
  uint16_t port_{8080};

#ifdef USE_ESP_IDF
  httpd_handle_t server_{nullptr};
  SemaphoreHandle_t capture_requested_{nullptr};
  SemaphoreHandle_t capture_done_{nullptr};
  std::atomic<bool> in_progress_{false};

  // Remote touch state — written by HTTP handler, read by main loop
  std::atomic<uint16_t> touch_x_{0};
  std::atomic<uint16_t> touch_y_{0};
  std::atomic<bool> touch_pressed_{false};
  std::atomic<bool> new_touch_{false};

  // Pointer to the screensaver_wake script (set by ESPHome codegen)
  script::RestartScript<> *wake_script_{nullptr};

  void start_server_();
  static esp_err_t handle_screenshot_(httpd_req_t *req);
  static esp_err_t handle_touch_(httpd_req_t *req);
  void wake_display_();
#elif defined(USE_HOST)
  int server_fd_{-1};
  pthread_t server_thread_;
  std::atomic<bool> server_running_{false};
  std::mutex capture_mutex_;
  std::condition_variable capture_cv_;
  std::atomic<bool> capture_requested_{false};
  std::atomic<bool> capture_done_{false};
  std::atomic<bool> in_progress_{false};
  void start_server_();
  static void *server_loop_(void *arg);
  static void handle_connection_(int client_fd, LvglScreenshot *self);
#endif

  // Pre-allocated snapshot buffer — allocated in PSRAM (ESP-IDF) or heap (host).
  // Used with lv_snapshot_take_to_draw_buf() so the snapshot lands in PSRAM
  // instead of LVGL's internal lv_malloc() which targets the small internal heap.
  uint8_t *snap_buf_{nullptr};
  size_t snap_buf_size_{0};

  // Intermediate RGB888 buffer — allocated in PSRAM (ESP-IDF) or heap (host)
  uint8_t *rgb_buf_{nullptr};

  // JPEG output buffer — allocated in PSRAM (ESP-IDF) or heap (host)
  uint8_t *jpeg_buf_{nullptr};
  size_t jpeg_capacity_{0};
  size_t jpeg_size_{0};

  void do_capture_();

  static void jpeg_write_cb_(void *ctx, void *data, int size);

#ifdef USE_ESP_IDF
  static LvglScreenshot *instance_;
#endif
};

}  // namespace lvgl_screenshot
}  // namespace esphome
