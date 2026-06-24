#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include "stb_image_write.h"

#ifdef USE_ESP_IDF

#include "esphome/components/script/script.h"

#endif  // USE_ESP_IDF

#include "lvgl_screenshot.h"

#ifdef USE_ESP_IDF

#include "esphome/core/log.h"
#include "esp_heap_caps.h"
#include "cJSON.h"
#include "freertos/task.h"
#include <algorithm>
#include "managed_components/lvgl__lvgl/src/draw/snapshot/lv_snapshot.h"
#include "managed_components/lvgl__lvgl/src/draw/lv_draw_buf.h"
#include "managed_components/lvgl__lvgl/src/core/lv_obj_draw_private.h"
#include "managed_components/lvgl__lvgl/src/misc/lv_math.h"
#include "managed_components/lvgl__lvgl/src/misc/lv_types.h"
#include "managed_components/lvgl__lvgl/src/widgets/button/lv_button.h"
#include "managed_components/lvgl__lvgl/src/widgets/slider/lv_slider.h"

namespace esphome {
namespace lvgl_screenshot {

static const char *const TAG = "lvgl_screenshot";

LvglScreenshot *LvglScreenshot::instance_ = nullptr;

// Context passed to stb's JPEG write callback
struct JpegWriteCtx {
  uint8_t *buf;
  size_t capacity;
  size_t size;
};

// ---------------------------------------------------------------------------
// jpeg_write_cb_()  –  stb calls this repeatedly as it produces JPEG data
// ---------------------------------------------------------------------------
void LvglScreenshot::jpeg_write_cb_(void *ctx, void *data, int size) {
  auto *c = (JpegWriteCtx *) ctx;
  if (size <= 0 || !data)
    return;
  size_t avail = c->capacity - c->size;
  size_t copy = std::min((size_t) size, avail);
  if (copy < (size_t) size) {
    ESP_LOGW(TAG, "JPEG buffer full — truncating %d → %u bytes", size, (unsigned) copy);
  }
  memcpy(c->buf + c->size, data, copy);
  c->size += copy;
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void LvglScreenshot::setup() {
  instance_ = this;

  // Wire the screensaver_wake script so we can wake the display from
  // HTTP handlers (screenshot / touch) without needing a separate YAML hook.
  this->wake_script_ = screensaver_wake;
  if (this->wake_script_) {
    ESP_LOGI(TAG, "Wired screensaver_wake script for display wake");
  } else {
    ESP_LOGW(TAG, "screensaver_wake script not found — display will not auto-wake");
  }

  // Create semaphores for HTTP handler <-> main-loop synchronisation
  this->capture_requested_ = xSemaphoreCreateBinary();
  this->capture_done_ = xSemaphoreCreateBinary();

  if (!this->capture_requested_ || !this->capture_done_) {
    ESP_LOGE(TAG, "Failed to create semaphores");
    this->mark_failed();
    return;
  }

  // Determine display dimensions from the default LVGL display
  lv_display_t *disp = lv_display_get_default();
  if (!disp) {
    ESP_LOGE(TAG, "No LVGL display found - is lvgl: initialised before this component?");
    this->mark_failed();
    return;
  }

  uint32_t width = (uint32_t) lv_display_get_horizontal_resolution(disp);
  uint32_t height = (uint32_t) lv_display_get_vertical_resolution(disp);

  // RGB888 intermediate buffer: width * height * 3 bytes
  size_t rgb_size = width * height * 3u;
  this->rgb_buf_ = (uint8_t *) heap_caps_malloc(rgb_size, MALLOC_CAP_SPIRAM);
  if (!this->rgb_buf_) {
    ESP_LOGE(TAG, "Failed to allocate %u bytes for RGB buffer in PSRAM", (unsigned) rgb_size);
    this->mark_failed();
    return;
  }

  // Pre-allocate snapshot buffer in PSRAM.  lv_snapshot_take() uses lv_malloc()
  // which targets the small internal heap — a 720x720 RGB888 buffer (~1.5 MiB)
  // will silently fail or produce garbage.  By providing our own PSRAM-backed
  // buffer and using lv_snapshot_take_to_draw_buf() we avoid that problem entirely.
  // Allocate with extra headroom for alignment padding.
  uint32_t rgb_snap_stride = lv_draw_buf_width_to_stride(width, LV_COLOR_FORMAT_RGB888);
  uint32_t argb_snap_stride = lv_draw_buf_width_to_stride(width, LV_COLOR_FORMAT_ARGB8888);
  this->snap_buf_size_ = std::max(rgb_snap_stride * height, argb_snap_stride * height) + LV_DRAW_BUF_ALIGN;
  this->snap_buf_ = (uint8_t *) heap_caps_malloc(this->snap_buf_size_, MALLOC_CAP_SPIRAM);
  if (!this->snap_buf_) {
    ESP_LOGE(TAG, "Failed to allocate %u bytes for snapshot buffer in PSRAM",
             (unsigned) this->snap_buf_size_);
    this->mark_failed();
    return;
  }

  // JPEG output buffer: allocate 60% of the raw RGB size — more than enough for quality 80
  this->jpeg_capacity_ = rgb_size * 6 / 10;
  this->jpeg_buf_ = (uint8_t *) heap_caps_malloc(this->jpeg_capacity_, MALLOC_CAP_SPIRAM);
  if (!this->jpeg_buf_) {
    ESP_LOGE(TAG, "Failed to allocate %u bytes for JPEG buffer in PSRAM", (unsigned) this->jpeg_capacity_);
    this->mark_failed();
    return;
  }

  this->jpeg_size_ = 0;

  this->start_server_();
  ESP_LOGI(TAG, "LVGL screenshot server started — http://<device-ip>:%u/screenshot", this->port_);
}

// ---------------------------------------------------------------------------
// start_server_()  –  spin up esp_http_server on the configured port
// ---------------------------------------------------------------------------
void LvglScreenshot::start_server_() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = this->port_;
  cfg.stack_size = 8192;
  // Use a unique ctrl_port so it doesn't clash with any other httpd instance
  cfg.ctrl_port = (uint16_t) (this->port_ + 1u);

  if (httpd_start(&this->server_, &cfg) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start HTTP server on port %u", this->port_);
    this->server_ = nullptr;
    return;
  }

  httpd_uri_t screenshot_uri = {
      .uri = "/screenshot",
      .method = HTTP_GET,
      .handler = LvglScreenshot::handle_screenshot_,
      .user_ctx = nullptr,
  };
  httpd_register_uri_handler(this->server_, &screenshot_uri);

  httpd_uri_t touch_uri = {
      .uri = "/touch",
      .method = HTTP_POST,
      .handler = LvglScreenshot::handle_touch_,
      .user_ctx = nullptr,
  };
  httpd_register_uri_handler(this->server_, &touch_uri);
}

// ---------------------------------------------------------------------------
// wake_display_()  –  execute the screensaver_wake script to wake the display
// ---------------------------------------------------------------------------
void LvglScreenshot::wake_display_() {
  if (this->wake_script_ && !this->wake_script_->is_running()) {
    ESP_LOGD(TAG, "Waking display via screensaver_wake script");
    this->wake_script_->execute();
    // Wait for the display to stabilize after wake (fade-in, LVGL redraw, etc.)
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// ---------------------------------------------------------------------------
// loop()  –  called from the ESPHome main task; safe to touch LVGL here
// ---------------------------------------------------------------------------
void LvglScreenshot::loop() {
  // Process remote touch events (if any)
  if (this->new_touch_.exchange(false)) {
    uint16_t tx = this->touch_x_.load();
    uint16_t ty = this->touch_y_.load();
    bool pressed = this->touch_pressed_.load();

    if (pressed) {
      // Wake the display — same thing a real touch does
      this->wake_display_();

      // Find the topmost clickable object at the touch point and trigger click.
      // Detail/modals are drawn on LVGL's top/sys layers, so search those
      // before the active screen.  Otherwise remote touches pass through to
      // the main grid even while a detail screen is visible on the device.
      lv_point_t point = {(int16_t) tx, (int16_t) ty};
      lv_obj_t *search_roots[] = {lv_layer_sys(), lv_layer_top(), lv_screen_active()};
      const char *search_names[] = {"sys", "top", "screen"};
      lv_obj_t *obj = nullptr;
      const char *obj_layer = "none";
      for (size_t i = 0; i < sizeof(search_roots) / sizeof(search_roots[0]); i++) {
        lv_obj_t *root = search_roots[i];
        if (!root) continue;
        if (i < 2 && lv_obj_get_child_count(root) == 0) continue;
        lv_obj_t *candidate = lv_indev_search_obj(root, &point);
        // Ignore the layer object itself; it is only a transparent container.
        if (candidate && candidate != root) {
          obj = candidate;
          obj_layer = search_names[i];
          break;
        }
      }
      if (obj) {
        // Sliders need a value update based on the touch coordinate; simply
        // sending click events only changes their pressed style.
        lv_obj_t *slider = obj;
        while (slider && !lv_obj_has_class(slider, &lv_slider_class)) {
          slider = lv_obj_get_parent(slider);
        }
        if (slider) {
          lv_area_t coords;
          lv_obj_get_coords(slider, &coords);
          int32_t min = lv_slider_get_min_value(slider);
          int32_t max = lv_slider_get_max_value(slider);
          int32_t value = min;
          int32_t w = coords.x2 - coords.x1 + 1;
          int32_t h = coords.y2 - coords.y1 + 1;
          if (h > w) {
            int32_t rel = coords.y2 - (int32_t) ty;
            if (rel < 0) rel = 0;
            if (rel > h) rel = h;
            value = min + (max - min) * rel / h;
          } else {
            int32_t rel = (int32_t) tx - coords.x1;
            if (rel < 0) rel = 0;
            if (rel > w) rel = w;
            value = min + (max - min) * rel / w;
          }
          ESP_LOGD(TAG, "Remote touch at (%u,%u) → layer=%s slider=%p value=%d active_before=%p",
                   tx, ty, obj_layer, (void *) slider, (int) value, (void *) lv_scr_act());
          lv_obj_send_event(slider, LV_EVENT_PRESSED, NULL);
          lv_slider_set_value(slider, value, LV_ANIM_OFF);
          lv_obj_send_event(slider, LV_EVENT_VALUE_CHANGED, NULL);
          lv_obj_send_event(slider, LV_EVENT_RELEASED, NULL);
        } else {
          // Find the nearest clickable ancestor.  Prefer real LVGL buttons,
          // but some navigation controls are generic clickable objects with a
          // label child, so falling back to the original target misses them.
          lv_obj_t *btn = obj;
          while (btn && !lv_obj_has_class(btn, &lv_button_class)) {
            btn = lv_obj_get_parent(btn);
          }
          if (!btn) {
            btn = obj;
            while (btn && !lv_obj_has_flag(btn, LV_OBJ_FLAG_CLICKABLE)) {
              btn = lv_obj_get_parent(btn);
            }
          }
          if (!btn) btn = obj;

          ESP_LOGD(TAG, "Remote touch at (%u,%u) → layer=%s target=%p (button=%p, active_before=%p)",
                   tx, ty, obj_layer, (void *) obj, (void *) btn, (void *) lv_scr_act());

          // Main-grid cards use ESPHome's on_short_click, while detail-page
          // controls mostly register LV_EVENT_CLICKED callbacks directly.
          // Send both events to mirror a normal tap closely enough for each path.
          lv_obj_send_event(btn, LV_EVENT_PRESSED, NULL);
          lv_obj_send_event(btn, LV_EVENT_RELEASED, NULL);
          lv_obj_send_event(btn, LV_EVENT_SHORT_CLICKED, NULL);
          lv_obj_send_event(btn, LV_EVENT_CLICKED, NULL);
        }
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(50));
        lv_timer_handler();

        ESP_LOGD(TAG, "Remote touch: active_after=%p", (void *) lv_scr_act());
      } else {
        ESP_LOGD(TAG, "Remote touch at (%u,%u) → no clickable object found (active_screen=%p)",
                 tx, ty, (void *) lv_scr_act());
      }
    }
  }

  // Non-blocking check: did the HTTP handler signal a capture request?
  if (xSemaphoreTake(this->capture_requested_, 0) == pdTRUE) {
    this->do_capture_();
    xSemaphoreGive(this->capture_done_);
  }
}

// ---------------------------------------------------------------------------
// do_capture_()  –  render active screen to RGB888 via lv_snapshot, encode JPEG
// ---------------------------------------------------------------------------
void LvglScreenshot::do_capture_() {
  lv_display_t *disp = lv_display_get_default();
  if (!disp) {
    ESP_LOGE(TAG, "No LVGL display found");
    this->jpeg_size_ = 0;
    return;
  }

  // Render the full active screen into our pre-allocated PSRAM snapshot buffer
  // via lv_snapshot_take_to_draw_buf().  This works regardless of the display
  // buffer_size setting — even with buffer_size < 100% where the display
  // buffers only hold partial frames rendered in tiles.  The snapshot always
  // captures the complete rendered screen.
  //
  // We use lv_snapshot_take_to_draw_buf() with our own PSRAM buffer instead of
  // lv_snapshot_take() because lv_snapshot_take() allocates internally via
  // lv_malloc() which targets the small internal heap.  A 720x720 RGB888
  // buffer (~1.5 MiB) will silently fail or produce garbage on internal heap.
#if LV_USE_SNAPSHOT
  lv_obj_t *screen = lv_scr_act();
  if (!screen) {
    ESP_LOGE(TAG, "No active screen");
    this->jpeg_size_ = 0;
    return;
  }

  // Debug: log the active screen pointer and display dimensions
  uint32_t disp_w = lv_display_get_horizontal_resolution(disp);
  uint32_t disp_h = lv_display_get_vertical_resolution(disp);
  lv_area_t screen_coords;
  lv_obj_get_coords(screen, &screen_coords);
  ESP_LOGD(TAG, "Screenshot: active_screen=%p disp=%ux%u coords=(%d,%d,%d,%d)",
           (void *) screen, disp_w, disp_h,
           (int) screen_coords.x1, (int) screen_coords.y1,
           (int) screen_coords.x2, (int) screen_coords.y2);
  // Also log the default display's active screen for comparison
  lv_obj_t *act_from_disp = lv_display_get_screen_active(disp);
  ESP_LOGD(TAG, "Screenshot: disp->act_scr=%p same=%s", (void *) act_from_disp,
           (screen == act_from_disp) ? "yes" : "NO");

  // Log parent chain to identify if this is a top-level screen or a child widget
  ESP_LOGD(TAG, "Screenshot: screen parent chain:");
  lv_obj_t *par = lv_obj_get_parent(screen);
  int depth = 0;
  while (par && depth < 6) {
    ESP_LOGD(TAG, "  parent[%d] %p", depth, (void *) par);
    par = lv_obj_get_parent(par);
    depth++;
  }
  if (!par) {
    ESP_LOGD(TAG, "  (no parent — this is a top-level screen)");
  }

  // Determine the snapshot dimensions (screen size + external draw area)
  lv_obj_update_layout(screen);
  uint32_t snap_w = (uint32_t) lv_obj_get_width(screen);
  uint32_t snap_h = (uint32_t) lv_obj_get_height(screen);
  int32_t ext_size = lv_obj_get_ext_draw_size(screen);
  snap_w += (uint32_t) (ext_size * 2);
  snap_h += (uint32_t) (ext_size * 2);
  ESP_LOGD(TAG, "Screenshot: snap_dims=%ux%u ext_draw=%d", snap_w, snap_h, ext_size);

  if (snap_w == 0 || snap_h == 0) {
    ESP_LOGE(TAG, "Invalid snapshot dimensions %ux%u", snap_w, snap_h);
    this->jpeg_size_ = 0;
    return;
  }

  // Calculate the stride LVGL will use for RGB888
  uint32_t snap_stride = lv_draw_buf_width_to_stride(snap_w, LV_COLOR_FORMAT_RGB888);
  uint32_t snap_data_size = snap_stride * snap_h;

  // Check our pre-allocated buffer is large enough; if not, fall back to
  // lv_snapshot_take() and hope for the best (should not happen in normal use).
  bool use_prealloc = (snap_data_size <= this->snap_buf_size_);

  lv_draw_buf_t draw_buf;
  lv_draw_buf_t *snap = nullptr;

  if (use_prealloc) {
    // Align the data pointer to LV_DRAW_BUF_ALIGN (64 bytes) to match what
    // lv_draw_buf_create() does internally.
    uint8_t *aligned = (uint8_t *) LV_ROUND_UP((lv_uintptr_t) this->snap_buf_,
                                                LV_DRAW_BUF_ALIGN);
    size_t aligned_size = this->snap_buf_size_ - ((size_t) (aligned - this->snap_buf_));

    lv_result_t res = lv_draw_buf_init(&draw_buf, snap_w, snap_h,
                                       LV_COLOR_FORMAT_RGB888, snap_stride,
                                       aligned, (uint32_t) aligned_size);
    if (res != LV_RESULT_OK) {
      ESP_LOGE(TAG, "lv_draw_buf_init failed");
      this->jpeg_size_ = 0;
      return;
    }

    res = lv_snapshot_take_to_draw_buf(screen, LV_COLOR_FORMAT_RGB888, &draw_buf);
    if (res != LV_RESULT_OK) {
      ESP_LOGE(TAG, "lv_snapshot_take_to_draw_buf failed");
      this->jpeg_size_ = 0;
      return;
    }

    snap = &draw_buf;
  } else {
    ESP_LOGW(TAG, "Snapshot buffer too small for %ux%u — falling back to lv_snapshot_take",
             snap_w, snap_h);
    snap = lv_snapshot_take(screen, LV_COLOR_FORMAT_RGB888);
    if (!snap || !snap->data) {
      ESP_LOGE(TAG, "lv_snapshot_take failed");
      this->jpeg_size_ = 0;
      return;
    }
  }

  // Copy the active screen into our tightly-packed RGB buffer.  We keep the
  // RGB buffer as the final compositing target so LVGL's top/sys layers (where
  // espcontrol detail modals live) can be blended in before JPEG encoding.
  snap_w = snap->header.w;
  snap_h = snap->header.h;
  uint32_t stride = snap->header.stride;

  ESP_LOGD(TAG, "Snapshot %ux%u stride=%u (packed=%u)", snap_w, snap_h, stride,
           (unsigned) (snap_w * 3u));

  if (snap_w > disp_w || snap_h > disp_h) {
    ESP_LOGE(TAG, "Snapshot %ux%u exceeds display buffer %ux%u", snap_w, snap_h, disp_w, disp_h);
    this->jpeg_size_ = 0;
    if (!use_prealloc && snap) lv_draw_buf_destroy(snap);
    return;
  }

  for (uint32_t y = 0; y < snap_h; y++) {
    const uint8_t *src_row = snap->data + y * stride;
    uint8_t *dst_row = this->rgb_buf_ + y * snap_w * 3u;
    for (uint32_t x = 0; x < snap_w; x++) {
      const uint8_t *src = src_row + x * 3u;
      uint8_t *dst = dst_row + x * 3u;
      // LVGL's RGB888 snapshot bytes are stored as B,G,R in memory on this
      // target, while stb expects R,G,B.
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
    }
  }

  // Only destroy if we allocated via lv_snapshot_take() (fallback path).  The
  // preallocated draw buffer can now be reused for ARGB top-layer snapshots.
  if (!use_prealloc && snap) {
    lv_draw_buf_destroy(snap);
  }

  auto blend_layer = [&](lv_obj_t *layer, const char *name) {
    if (!layer || lv_obj_get_child_count(layer) == 0) return;

    lv_obj_update_layout(layer);
    uint32_t layer_w = (uint32_t) lv_obj_get_width(layer);
    uint32_t layer_h = (uint32_t) lv_obj_get_height(layer);
    if (layer_w == 0 || layer_h == 0) {
      layer_w = disp_w;
      layer_h = disp_h;
    }
    layer_w = std::min(layer_w, snap_w);
    layer_h = std::min(layer_h, snap_h);

    uint32_t layer_stride = lv_draw_buf_width_to_stride(layer_w, LV_COLOR_FORMAT_ARGB8888);
    uint32_t layer_data_size = layer_stride * layer_h;
    if (layer_data_size > this->snap_buf_size_) {
      ESP_LOGW(TAG, "Skipping %s layer snapshot: need %u bytes, have %u",
               name, (unsigned) layer_data_size, (unsigned) this->snap_buf_size_);
      return;
    }

    uint8_t *aligned = (uint8_t *) LV_ROUND_UP((lv_uintptr_t) this->snap_buf_, LV_DRAW_BUF_ALIGN);
    size_t aligned_size = this->snap_buf_size_ - ((size_t) (aligned - this->snap_buf_));
    memset(aligned, 0, layer_data_size);

    lv_draw_buf_t layer_buf;
    lv_result_t layer_res = lv_draw_buf_init(&layer_buf, layer_w, layer_h,
                                             LV_COLOR_FORMAT_ARGB8888, layer_stride,
                                             aligned, (uint32_t) aligned_size);
    if (layer_res != LV_RESULT_OK) {
      ESP_LOGW(TAG, "lv_draw_buf_init failed for %s layer", name);
      return;
    }

    layer_res = lv_snapshot_take_to_draw_buf(layer, LV_COLOR_FORMAT_ARGB8888, &layer_buf);
    if (layer_res != LV_RESULT_OK) {
      ESP_LOGW(TAG, "Snapshot failed for %s layer", name);
      return;
    }

    uint32_t out_w = std::min((uint32_t) layer_buf.header.w, snap_w);
    uint32_t out_h = std::min((uint32_t) layer_buf.header.h, snap_h);
    uint32_t out_stride = layer_buf.header.stride;
    uint32_t blended = 0;

    for (uint32_t y = 0; y < out_h; y++) {
      const lv_color32_t *src_row = (const lv_color32_t *) (layer_buf.data + y * out_stride);
      uint8_t *dst_row = this->rgb_buf_ + y * snap_w * 3u;
      for (uint32_t x = 0; x < out_w; x++) {
        const lv_color32_t &src = src_row[x];
        uint8_t a = src.alpha;
        if (a == 0) continue;
        uint8_t *dst = dst_row + x * 3u;
        if (a == 255) {
          dst[0] = src.red;
          dst[1] = src.green;
          dst[2] = src.blue;
        } else {
          uint16_t inv = 255u - a;
          dst[0] = (uint8_t) ((src.red * a + dst[0] * inv + 127u) / 255u);
          dst[1] = (uint8_t) ((src.green * a + dst[1] * inv + 127u) / 255u);
          dst[2] = (uint8_t) ((src.blue * a + dst[2] * inv + 127u) / 255u);
        }
        blended++;
      }
    }

    ESP_LOGD(TAG, "Screenshot: blended %s layer (%u children, %u pixels)",
             name, (unsigned) lv_obj_get_child_count(layer), (unsigned) blended);
  };

  blend_layer(lv_layer_top(), "top");
  blend_layer(lv_layer_sys(), "sys");

  JpegWriteCtx ctx = {this->jpeg_buf_, this->jpeg_capacity_, 0};
  stbi_write_jpg_to_func(LvglScreenshot::jpeg_write_cb_, &ctx,
                         (int) snap_w, (int) snap_h, 3, this->rgb_buf_, 80);
  this->jpeg_size_ = ctx.size;

  ESP_LOGD(TAG, "Captured %ux%u JPEG (%u bytes) via snapshot (prealloc=%s)",
           snap_w, snap_h, (unsigned) this->jpeg_size_,
           use_prealloc ? "yes" : "no");
#else
  ESP_LOGE(TAG, "Snapshot not available — enable CONFIG_LV_USE_SNAPSHOT");
  this->jpeg_size_ = 0;
#endif
}

// ---------------------------------------------------------------------------
// handle_screenshot_()  –  runs in esp_http_server's task, NOT the main loop
// ---------------------------------------------------------------------------
esp_err_t LvglScreenshot::handle_screenshot_(httpd_req_t *req) {
  LvglScreenshot *self = instance_;
  if (!self || !self->jpeg_buf_) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Component not ready");
    return ESP_FAIL;
  }

  // Only one capture at a time (atomic test-and-set)
  if (self->in_progress_.exchange(true)) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "Capture in progress, try again");
    self->in_progress_ = false;
    return ESP_OK;
  }

  // Wake the display so the screenshot isn't blank
  self->wake_display_();

  // Ask the main loop to do the capture
  xSemaphoreGive(self->capture_requested_);

  // Wait up to 3 s for the main loop to finish (it runs at ~60 Hz so ~16 ms max wait)
  if (xSemaphoreTake(self->capture_done_, pdMS_TO_TICKS(3000)) != pdTRUE) {
    self->in_progress_ = false;
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Capture timed out");
    return ESP_FAIL;
  }

  if (self->jpeg_size_ == 0) {
    self->in_progress_ = false;
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Framebuffer unavailable");
    return ESP_FAIL;
  }

  // Send headers
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=\"screenshot.jpg\"");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store");

  // Stream the JPEG in 4 KB chunks
  const size_t CHUNK = 4096;
  size_t sent = 0;
  esp_err_t ret = ESP_OK;
  while (sent < self->jpeg_size_) {
    size_t chunk_len = std::min(CHUNK, self->jpeg_size_ - sent);
    ret = httpd_resp_send_chunk(req, (const char *) self->jpeg_buf_ + sent, (ssize_t) chunk_len);
    if (ret != ESP_OK) {
      ESP_LOGW(TAG, "Failed to send chunk at offset %u", (unsigned) sent);
      break;
    }
    sent += chunk_len;
  }

  // Terminate chunked transfer
  httpd_resp_send_chunk(req, nullptr, 0);

  self->in_progress_ = false;
  return ret;
}

// ---------------------------------------------------------------------------
// handle_touch_()  –  accept remote touch coordinates via POST /touch
// ---------------------------------------------------------------------------
esp_err_t LvglScreenshot::handle_touch_(httpd_req_t *req) {
  LvglScreenshot *self = instance_;
  if (!self) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Component not ready");
    return ESP_FAIL;
  }

  // Read the request body
  char body_buf[128];
  int body_len = req->content_len;
  if (body_len <= 0 || body_len >= (int) sizeof(body_buf)) {
    body_len = (int) sizeof(body_buf) - 1;
  }
  int received = httpd_req_recv(req, body_buf, body_len);
  if (received <= 0) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to read body");
    return ESP_FAIL;
  }
  body_buf[received] = '\0';

  // Parse JSON: {"x": 100, "y": 200, "pressed": true}
  cJSON *root = cJSON_Parse(body_buf);
  if (!root) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
    return ESP_FAIL;
  }

  cJSON *x_obj = cJSON_GetObjectItem(root, "x");
  cJSON *y_obj = cJSON_GetObjectItem(root, "y");
  cJSON *p_obj = cJSON_GetObjectItem(root, "pressed");

  if (!cJSON_IsNumber(x_obj) || !cJSON_IsNumber(y_obj)) {
    cJSON_Delete(root);
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing x or y");
    return ESP_FAIL;
  }

  uint16_t tx = (uint16_t) cJSON_GetNumberValue(x_obj);
  uint16_t ty = (uint16_t) cJSON_GetNumberValue(y_obj);
  bool pressed = (p_obj && cJSON_IsTrue(p_obj)) ? true : false;

  self->touch_x_.store(tx);
  self->touch_y_.store(ty);
  self->touch_pressed_.store(pressed);
  self->new_touch_.store(true);

  cJSON_Delete(root);

  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
  return ESP_OK;
}

}  // namespace lvgl_screenshot
}  // namespace esphome

#elif defined(USE_HOST)

#include "esphome/core/log.h"
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <algorithm>

namespace esphome {
namespace lvgl_screenshot {

static const char *const TAG = "lvgl_screenshot";

// Context passed to stb's JPEG write callback
struct JpegWriteCtx {
  uint8_t *buf;
  size_t capacity;
  size_t size;
};

// ---------------------------------------------------------------------------
// jpeg_write_cb_()  –  stb calls this repeatedly as it produces JPEG data
// ---------------------------------------------------------------------------
void LvglScreenshot::jpeg_write_cb_(void *ctx, void *data, int size) {
  auto *c = (JpegWriteCtx *) ctx;
  if (size <= 0 || !data)
    return;
  size_t avail = c->capacity - c->size;
  size_t copy = std::min((size_t) size, avail);
  if (copy < (size_t) size) {
    ESP_LOGW(TAG, "JPEG buffer full — truncating %d → %u bytes", size, (unsigned) copy);
  }
  memcpy(c->buf + c->size, data, copy);
  c->size += copy;
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void LvglScreenshot::setup() {
  // Determine display dimensions from the default LVGL display
  lv_display_t *disp = lv_display_get_default();
  if (!disp) {
    ESP_LOGE(TAG, "No LVGL display found - is lvgl: initialised before this component?");
    this->mark_failed();
    return;
  }

  uint32_t width = (uint32_t) lv_display_get_horizontal_resolution(disp);
  uint32_t height = (uint32_t) lv_display_get_vertical_resolution(disp);

  // RGB888 intermediate buffer: width * height * 3 bytes
  size_t rgb_size = width * height * 3u;
  this->rgb_buf_ = (uint8_t *) malloc(rgb_size);
  if (!this->rgb_buf_) {
    ESP_LOGE(TAG, "Failed to allocate %u bytes for RGB buffer", (unsigned) rgb_size);
    this->mark_failed();
    return;
  }

  // JPEG output buffer: allocate 60% of the raw RGB size — more than enough for quality 80
  this->jpeg_capacity_ = rgb_size * 6 / 10;
  this->jpeg_buf_ = (uint8_t *) malloc(this->jpeg_capacity_);
  if (!this->jpeg_buf_) {
    ESP_LOGE(TAG, "Failed to allocate %u bytes for JPEG buffer", (unsigned) this->jpeg_capacity_);
    this->mark_failed();
    return;
  }

  this->jpeg_size_ = 0;

  this->start_server_();
  ESP_LOGI(TAG, "LVGL screenshot server started — http://localhost:%u/screenshot", this->port_);
}

// ---------------------------------------------------------------------------
// start_server_()  –  spin up a simple HTTP server on POSIX sockets
// ---------------------------------------------------------------------------
void LvglScreenshot::start_server_() {
  // Create socket
  this->server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (this->server_fd_ < 0) {
    ESP_LOGE(TAG, "Failed to create socket");
    this->mark_failed();
    return;
  }

  // Allow address reuse
  int opt = 1;
  if (setsockopt(this->server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    ESP_LOGW(TAG, "setsockopt(SO_REUSEADDR) failed");
  }

  // Bind to all interfaces on the configured port
  struct sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(this->port_);

  if (bind(this->server_fd_, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    ESP_LOGE(TAG, "Failed to bind to port %u", this->port_);
    close(this->server_fd_);
    this->server_fd_ = -1;
    this->mark_failed();
    return;
  }

  // Listen with backlog of 5
  if (listen(this->server_fd_, 5) < 0) {
    ESP_LOGE(TAG, "Failed to listen on port %u", this->port_);
    close(this->server_fd_);
    this->server_fd_ = -1;
    this->mark_failed();
    return;
  }

  this->server_running_ = true;
  if (pthread_create(&this->server_thread_, nullptr, LvglScreenshot::server_loop_, this) != 0) {
    ESP_LOGE(TAG, "Failed to create server thread");
    this->server_running_ = false;
    close(this->server_fd_);
    this->server_fd_ = -1;
    this->mark_failed();
    return;
  }
}

// ---------------------------------------------------------------------------
// server_loop_()  –  runs in a background pthread, accepts connections
// ---------------------------------------------------------------------------
void *LvglScreenshot::server_loop_(void *arg) {
  LvglScreenshot *self = (LvglScreenshot *) arg;
  while (self->server_running_) {
    struct sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(self->server_fd_, (struct sockaddr *) &client_addr, &client_len);
    if (client_fd < 0) {
      if (self->server_running_) {
        ESP_LOGW(TAG, "accept() failed: %s", strerror(errno));
      }
      continue;
    }
    // Handle the connection in the same thread (simple approach)
    self->handle_connection_(client_fd, self);
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
// handle_connection_()  –  parse HTTP request, serve screenshot or 404
// ---------------------------------------------------------------------------
void LvglScreenshot::handle_connection_(int client_fd, LvglScreenshot *self) {
  char buf[1024];
  ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
  if (n <= 0) {
    close(client_fd);
    return;
  }
  buf[n] = '\0';

  // Check if this is a GET /screenshot request
  bool is_screenshot_request = (strstr(buf, "GET /screenshot") != nullptr);

  if (!is_screenshot_request) {
    // 404 Not Found
    const char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nNot Found";
    send(client_fd, response, strlen(response), 0);
    close(client_fd);
    return;
  }

  // Only one capture at a time
  if (self->in_progress_.exchange(true)) {
    const char *response = "HTTP/1.1 503 Service Unavailable\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nCapture in progress, try again";
    send(client_fd, response, strlen(response), 0);
    self->in_progress_ = false;
    close(client_fd);
    return;
  }

  // Signal the main loop to capture
  {
    std::lock_guard<std::mutex> lock(self->capture_mutex_);
    self->capture_requested_ = true;
    self->capture_done_ = false;
  }
  self->capture_cv_.notify_one();

  // Wait up to 3 s for capture to complete
  std::unique_lock<std::mutex> lock(self->capture_mutex_);
  auto *local_self = self;
  if (self->capture_cv_.wait_for(lock, std::chrono::seconds(3),
                                  [local_self]() { return local_self->capture_done_.load(); })) {
    // Capture completed
    if (self->jpeg_size_ == 0) {
      const char *response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nFramebuffer unavailable";
      send(client_fd, response, strlen(response), 0);
      self->in_progress_ = false;
      close(client_fd);
      return;
    }

    // Build HTTP response with JPEG data
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: image/jpeg\r\n";
    response << "Content-Disposition: inline; filename=\"screenshot.jpg\"\r\n";
    response << "Cache-Control: no-cache, no-store\r\n";
    response << "Content-Length: " << self->jpeg_size_ << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";

    std::string headers = response.str();
    send(client_fd, headers.c_str(), headers.size(), 0);
    send(client_fd, (const char *) self->jpeg_buf_, self->jpeg_size_, 0);
  } else {
    // Timed out
    const char *response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nCapture timed out";
    send(client_fd, response, strlen(response), 0);
  }

  self->in_progress_ = false;
  close(client_fd);
}

// ---------------------------------------------------------------------------
// loop()  –  called from the ESPHome main loop; safe to touch LVGL here
// ---------------------------------------------------------------------------
void LvglScreenshot::loop() {
  // Check if a capture was requested (non-blocking)
  bool requested = false;
  {
    std::lock_guard<std::mutex> lock(this->capture_mutex_);
    if (this->capture_requested_) {
      requested = true;
      this->capture_requested_ = false;
    }
  }

  if (requested) {
    this->do_capture_();
    {
      std::lock_guard<std::mutex> lock(this->capture_mutex_);
      this->capture_done_ = true;
    }
    this->capture_cv_.notify_one();
  }
}

// ---------------------------------------------------------------------------
// do_capture_()  –  convert LVGL RGB565 → RGB888, then encode to JPEG
// ---------------------------------------------------------------------------
void LvglScreenshot::do_capture_() {
  lv_display_t *disp = lv_display_get_default();
  if (!disp) {
    ESP_LOGE(TAG, "No LVGL display found");
    this->jpeg_size_ = 0;
    return;
  }

  uint32_t width = (uint32_t) lv_display_get_horizontal_resolution(disp);
  uint32_t height = (uint32_t) lv_display_get_vertical_resolution(disp);

  // Get the active draw buffer
  lv_draw_buf_t *draw_buf = lv_display_get_buf_active(disp);
  if (!draw_buf || !draw_buf->data) {
    ESP_LOGE(TAG, "LVGL framebuffer not available");
    this->jpeg_size_ = 0;
    return;
  }

  // Convert RGB565 → RGB888 into rgb_buf_ (row-major, top-down)
  auto *lvgl_buf = (lv_color16_t *) draw_buf->data;
  for (uint32_t y = 0; y < height; y++) {
    uint8_t *row = this->rgb_buf_ + y * width * 3u;
    for (uint32_t x = 0; x < width; x++) {
      lv_color16_t c = lvgl_buf[y * width + x];

      // Scale 5-bit → 8-bit and 6-bit → 8-bit by replicating the MSBs
      row[x * 3 + 0] = (uint8_t) ((c.red << 3) | (c.red >> 2));
      row[x * 3 + 1] = (uint8_t) ((c.green << 2) | (c.green >> 4));
      row[x * 3 + 2] = (uint8_t) ((c.blue << 3) | (c.blue >> 2));
    }
  }

  // ------------------------------------------------------------------
  // Encode RGB888 → JPEG via stb_image_write (quality 80)
  // ------------------------------------------------------------------
  JpegWriteCtx ctx = {this->jpeg_buf_, this->jpeg_capacity_, 0};
  stbi_write_jpg_to_func(LvglScreenshot::jpeg_write_cb_, &ctx,
                         (int) width, (int) height, 3, this->rgb_buf_, 80);

  this->jpeg_size_ = ctx.size;
  ESP_LOGD(TAG, "Captured %ux%u JPEG (%u bytes)", width, height, (unsigned) this->jpeg_size_);
}

}  // namespace lvgl_screenshot
}  // namespace esphome

#endif  // USE_ESP_IDF / USE_HOST
