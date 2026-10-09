"""Exercise the tile-to-expanded download handoff with production camera methods."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
header = (root / "components/espcontrol/button_grid_image.h").read_text()


def definition(name):
    match = re.search(rf"^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}", header, re.M | re.S)
    assert match, name
    return match.group()


source = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include "camera_refresh_policy.h"
#define ESP_LOGI(...) ((void)0)
namespace esphome {
uint32_t millis() { return 100000; }
namespace artwork_image {
enum class ImageResizeMode { FIT, COVER };
struct ArtworkImage {
 int width = 0, height = 0, requests = 0, cancellations = 0;
 ImageResizeMode mode = ImageResizeMode::COVER;
 uint32_t fit_background_color = 0;
 std::string url;
 void set_target_size(int w, int h) { width = w; height = h; }
 void set_resize_mode(ImageResizeMode m) { mode = m; }
 void set_fit_background_color(uint32_t color) { fit_background_color = color; }
 void cancel_update() { ++cancellations; }
 std::string request_update_url(const std::string &u, int) {
   url = u; ++requests; return u;
 }
};
}}
using Image = esphome::artwork_image::ArtworkImage;
using lv_coord_t = int;
struct Widget { int width = 1280, height = 800; };
struct ImageCardCtx {
 bool active = true, modal_fit = true, scheduled_tile_request = false;
 bool download_active = false, explicit_picture_refresh = false;
 Image *image = nullptr, *modal_image = nullptr;
 std::string entity_id = "camera.front", source_url = "snapshot";
 std::string modal_url, modal_source_url;
 uint32_t next_download_retry_ms = 0, last_modal_request_started_ms = 0;
 uint32_t fit_background_color = 0x304860;
 espcontrol::camera::RefreshSchedule refresh_schedule;
 espcontrol::camera::ImageRevision revision;
};
using lv_timer_t = struct Timer {
 void (*callback)(Timer *);
 ImageCardCtx *ctx;
};
struct ImageCardModalUi {
 ImageCardCtx *active = nullptr;
 Widget *panel = nullptr;
 Timer *request_timer = nullptr;
} ui;
ImageCardModalUi &image_card_modal_ui() { return ui; }
bool image_card_modal_active_for(ImageCardCtx *ctx) { return ui.active == ctx; }
bool image_card_has_separate_modal_image(ImageCardCtx *ctx) {
 return ctx->modal_image && ctx->modal_image != ctx->image;
}
bool image_card_camera_retry_blocked(ImageCardCtx *) { return false; }
bool image_card_pipeline_suspended() { return false; }
bool image_card_modal_refresh_supported() { return true; }
bool image_card_context_on_active_screen(ImageCardCtx *) { return true; }
bool ha_api_state_connected() { return true; }
bool image_card_memory_available(ImageCardCtx *, const char *, int, int) { return true; }
bool image_card_modal_has_preview(ImageCardCtx *) { return true; }
void image_card_show_modal_loading(ImageCardCtx *, const char *) {}
void image_card_log_diagnostics(ImageCardCtx *, const char *, int = 0, int = 0) {}
void lv_obj_update_layout(Widget *) {}
int lv_obj_get_width(Widget *w) { return w->width; }
int lv_obj_get_height(Widget *w) { return w->height; }
void image_card_limit_target_size(int w, int h, int *width, int *height) {
 *width = w; *height = h;
}
std::string image_card_sized_url(const std::string &url, int, int) { return url; }
int released_slots = 0, errors = 0;
void image_card_release_download_slot(ImageCardCtx *ctx) {
 ctx->download_active = false; ++released_slots;
}
void image_card_preempt_active_tile_for_modal() {}
void image_card_handle_modal_download_error(ImageCardCtx *) { ++errors; }
void *lv_timer_get_user_data(Timer *t) { return t->ctx; }
void lv_timer_del(Timer *t) { delete t; }
Timer *lv_timer_create(void (*callback)(Timer *), uint32_t, ImageCardCtx *ctx) {
 return new Timer{callback, ctx};
}
constexpr uint32_t IMAGE_CARD_MODAL_REQUEST_DELAY_MS = 80;
'''
for name in (
    "image_card_cancel_scheduled_tile_request",
    "image_card_finish_scheduled_tile_request",
    "image_card_request_modal_source_url",
    "image_card_modal_request_timer_cb",
    "image_card_cancel_modal_request_timer",
    "image_card_queue_modal_source_request",
):
    source += definition(name) + "\n"
source += r'''
int main() {
 for (auto refresh : {espcontrol::camera::RefreshMode::PERIODIC,
                      espcontrol::camera::RefreshMode::ACTIVITY}) {
  for (bool fit : {false, true}) {
   Image tile, expanded;
   ImageCardCtx ctx;
   Widget panel;
   ctx.image = &tile; ctx.modal_image = &expanded; ctx.modal_fit = fit;
   ctx.refresh_schedule.mode = refresh;
   ctx.refresh_schedule.begin(esphome::millis(), true);
   ctx.refresh_schedule.started();
   ctx.scheduled_tile_request = ctx.download_active = true;
   ui.active = &ctx; ui.panel = &panel;
   released_slots = 0;
   // Opening during a tile refresh must replace the cropped preview with a
   // separately decoded expanded image, rather than treating the tile as it.
   assert(image_card_queue_modal_source_request(&ctx));
   assert(ui.request_timer != nullptr);
   assert(tile.cancellations == 1 && released_slots == 1);
   assert(!ctx.download_active && !ctx.scheduled_tile_request);
   assert(!ctx.refresh_schedule.in_flight);
   ui.request_timer->callback(ui.request_timer);
   assert(ui.request_timer == nullptr && errors == 0);
   assert(expanded.requests == 1 && ctx.refresh_schedule.in_flight);
   assert(expanded.width == 1280 && expanded.height == 800);
   assert(expanded.fit_background_color == ctx.fit_background_color);
   assert(expanded.mode == (fit ? esphome::artwork_image::ImageResizeMode::FIT
                                : esphome::artwork_image::ImageResizeMode::COVER));
   // A late tile completion must not end the expanded request's refresh clock.
   image_card_finish_scheduled_tile_request(&ctx, true);
   assert(ctx.refresh_schedule.in_flight);
   assert(image_card_queue_modal_source_request(&ctx));
   assert(ui.request_timer == nullptr && expanded.requests == 1);
  }
 }
}
'''

with tempfile.TemporaryDirectory(prefix="camera-expanded-") as tmp:
    cpp, binary = Path(tmp) / "test.cpp", Path(tmp) / "test"
    cpp.write_text(source)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "c++", "-std=c++17",
                    "-Wall", "-Wextra", "-Werror", "-I", str(root / "components/espcontrol"),
                    str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
