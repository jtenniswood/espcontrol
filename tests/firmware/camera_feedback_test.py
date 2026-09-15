#!/usr/bin/env python3
"""Exercise production camera cache/availability callbacks with host UI doubles."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
header = (root / 'components/espcontrol/button_grid_image.h').read_text()

def definition(name):
    matches = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', header, re.M | re.S)
    assert len(matches) == 1, name
    return matches[0]

source = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include "image_pipeline_policy.h"
uint32_t now_ms = 0;
namespace esphome {
uint32_t millis() { return now_ms; }
using StringRef = std::string;
namespace artwork_image {
struct ArtworkImage {
 bool available = true; int released = 0, cancelled = 0;
 bool has_image() { return available; }
 int get_width() { return 320; } int get_height() { return 240; }
 void release() { available = false; ++released; }
 void cancel_update() { ++cancelled; }
};
}
}
namespace espcontrol { enum class DisplayTakeoverKind { INTERACTIVE }; }
using Image = esphome::artwork_image::ArtworkImage;
struct Timer { void *data; uint32_t delay; };
using lv_timer_t = Timer;
struct Widget { bool hidden = false; };
struct ImageCardCtx {
 bool active = true, media_artwork = false, image_ready = true;
 bool camera_entity_unavailable = false, download_active = false, modal_fit = false;
 bool diagnostics_enabled = false; uint32_t last_modal_request_started_ms = 0;
 uint8_t camera_download_errors = 0, startup_download_errors = 0;
 uint32_t camera_retry_after_ms = 0, next_download_retry_ms = 0;
 uint32_t next_picture_retry_ms = 0, last_download_completed_ms = 0;
 Image *image = nullptr, *modal_image = nullptr;
 Widget *widget = nullptr;
 std::string entity_id = "camera.front", source_url = "snapshot", url = "snapshot";
 std::string modal_url = "snapshot", modal_source_url = "snapshot";
 std::function<void(espcontrol::DisplayTakeoverKind)> end_display_takeover;
};
struct ImageCardModalUi {
 ImageCardCtx *active = nullptr; Widget *image_widget = nullptr, *overlay = nullptr;
 Widget *panel = nullptr, *back_btn = nullptr;
};
struct ImageCardModalCache {
 Image *image = nullptr; std::string entity_id, source_url;
 uint32_t cached_at_ms = 0; bool modal_fit = false; Timer *expiry_timer = nullptr; bool ready = false;
};
ImageCardModalUi ui;
ImageCardModalCache cache;
ImageCardCtx contexts[1];
ImageCardCtx *image_card_contexts() { return contexts; }
ImageCardModalUi &image_card_modal_ui() { return ui; }
ImageCardModalCache &image_card_modal_cache() { return cache; }
bool constrained = true;
bool image_card_constrained_memory_profile() { return constrained; }
bool image_card_modal_active_for(ImageCardCtx *ctx) { return ui.active == ctx; }
std::string tile_status, modal_status;
int cleared = 0, pictures = 0, tile_requests = 0, modal_requests = 0, recovered = 0;
void image_card_hide(ImageCardCtx *ctx) { if (ctx->widget) ctx->widget->hidden = true; }
void image_card_set_loading_state(ImageCardCtx *, const char *s, bool) { tile_status = s; }
void image_card_show_modal_loading(ImageCardCtx *, const char *s) { modal_status = s; }
void image_card_clear_widget_source(Widget *) { ++cleared; }
void image_card_hide_loading(ImageCardCtx *) { tile_status.clear(); }
void image_card_release_download_slot(ImageCardCtx *c) { c->download_active = false; }
void image_card_log_diagnostics(ImageCardCtx *, const char *) {}
bool image_card_startup_retry_active(ImageCardCtx *, uint32_t) { return true; }
void image_card_cancel_modal_request_timer() {}
enum class ControlModalKind { IMAGE_CARD };
void control_modal_delete_overlay(ControlModalKind, Widget *) {}
void image_card_schedule_modal_cleanup(ImageCardCtx *) {}
void *lv_timer_get_user_data(Timer *t) { return t->data; }
void lv_timer_del(Timer *t) { delete t; }
Timer *lv_timer_create(void (*)(Timer *), uint32_t delay, void *data) {
 return new Timer{data, delay};
}
constexpr uint32_t IMAGE_CARD_CONSTRAINED_MODAL_CACHE_TTL_MS = 15000;
constexpr uint32_t IMAGE_CARD_RETRY_INTERVAL_MS = 2000;
constexpr uint32_t IMAGE_CARD_MODAL_REFRESH_DELAY_MS = 1000;
constexpr int LV_OBJ_FLAG_HIDDEN = 1;
bool lv_obj_has_flag(Widget *w, int) { return w->hidden; }
void lv_obj_move_background(Widget *) {}
void lv_obj_move_foreground(Widget *) {}
void lv_obj_invalidate(Widget *) {}
void image_card_set_widget_source(Widget *, Image *) {}
void image_card_hide_modal_loading(ImageCardCtx *) { modal_status.clear(); }
bool image_card_has_separate_modal_image(ImageCardCtx *ctx) { return ctx->modal_image != ctx->image; }
bool image_card_apply_modal_geometry(ImageCardCtx *, Image *) { return true; }
void notify_dashboard_content_changed() {}
constexpr uint32_t IMAGE_CARD_STARTUP_DOWNLOAD_RETRIES = 10;
constexpr int IMAGE_CARD_MAX_CONTEXTS = 1;
uint32_t ha_subscription_generation() { return 1; }
bool image_card_context_current(ImageCardCtx *, const std::string &, uint32_t) { return true; }
std::string string_ref_limited(const std::string &v, size_t n) { return v.substr(0,n); }
std::function<void(std::string)> state_callback;
void ha_subscribe_state(const std::string &, std::function<void(std::string)> cb) { state_callback = cb; }
void image_card_request_picture(ImageCardCtx *) { ++pictures; }
void image_card_request_current_picture(ImageCardCtx *) { ++pictures; }
void image_card_request_source_url(ImageCardCtx *) { ++tile_requests; }
void image_card_queue_modal_source_request(ImageCardCtx *) { ++modal_requests; }
void image_card_apply_downloaded(ImageCardCtx *) { ++recovered; }
#define ESP_LOGI(...) do {} while(false)
#define ESP_LOGW(...) do {} while(false)
'''
# Use unchanged production definitions, rather than reproducing their logic.
for name in ('image_card_modal_cache_expired', 'image_card_cancel_modal_cache_expiry',
             'image_card_release_modal_cache', 'image_card_modal_cache_expiry_timer_cb',
             'image_card_schedule_modal_cache_expiry', 'image_card_show_camera_unavailable',
             'image_card_camera_retry_blocked', 'image_card_camera_download_failed',
             'image_card_handle_download_error', 'image_card_modal_has_tile_fallback',
             'image_card_modal_cache_matches', 'image_card_modal_needs_open_refresh',
             'subscribe_image_card_entity_state',
             'image_card_hide_modal', 'image_card_apply_modal_downloaded'):
    source += definition(name) + '\n'
# The polling callback checks get_url as well as availability.
source = source.replace('bool has_image() {', 'std::string get_url() { return "snapshot"; }\n bool has_image() {')
source += definition('image_card_refresh_due')
source += r'''
int main() {
 Image tile, modal; Widget widget;
 auto &ctx = contexts[0]; ctx.image = &tile; ctx.modal_image = &modal; ctx.widget = &widget;
 cache.image = &modal; cache.entity_id = ctx.entity_id; cache.source_url = ctx.source_url;
 cache.ready = true; cache.cached_at_ms = 1000;
 // A long viewing session still leaves a full 15 seconds after closing.
 now_ms = 60000; ui.active = &ctx;
 image_card_hide_modal();
 assert(cache.cached_at_ms == 60000);
 image_card_schedule_modal_cache_expiry(&modal);
 assert(cache.expiry_timer && cache.expiry_timer->delay == 15000);
 now_ms += 12900;
 assert(image_card_modal_cache_matches(&ctx));
 assert(!image_card_modal_needs_open_refresh(&ctx));
 ctx.modal_fit = true; assert(image_card_modal_needs_open_refresh(&ctx));
 ctx.modal_fit = false;
 image_card_schedule_modal_cache_expiry(&modal);
 assert(cache.expiry_timer->delay == 2100);
 now_ms = 75000;
 assert(!image_card_modal_cache_matches(&ctx));
 image_card_modal_cache_expiry_timer_cb(cache.expiry_timer);
 assert(modal.released == 1 && !cache.ready);
 // Close near rollover, then reopen across rollover.
 modal.available = true; cache.image = &modal; cache.ready = true;
 cache.entity_id = ctx.entity_id; cache.source_url = ctx.source_url;
 now_ms = UINT32_MAX - 999; ui.active = &ctx; image_card_hide_modal();
 now_ms = 0; assert(image_card_modal_cache_matches(&ctx));
 ctx.source_url = "new"; assert(!image_card_modal_cache_matches(&ctx));
 ctx.source_url = "snapshot";
 constrained = false; now_ms = 60000; assert(image_card_modal_cache_matches(&ctx));
 assert(image_card_modal_needs_open_refresh(&ctx));
 constrained = true;
 // HTTP errors enter the same UI path regardless of status (including HA 401).
 ui.active = &ctx;
 for (uint32_t delay : {2000u, 4000u, 8000u, 16000u, 30000u, 30000u}) {
   image_card_handle_download_error(&ctx);
   assert(tile_status == "Unavailable" && modal_status == "Unavailable");
   assert(!cache.ready && !image_card_modal_has_tile_fallback(&ctx));
   assert(ctx.camera_retry_after_ms - now_ms == delay);
   ctx.next_download_retry_ms = 0; // A competing refresh must not erase backoff.
   assert(image_card_camera_retry_blocked(&ctx));
   assert(ctx.next_download_retry_ms == ctx.camera_retry_after_ms);
   now_ms += delay;
   assert(!image_card_camera_retry_blocked(&ctx));
 }
 image_card_refresh_due();
 assert(modal_requests == 1 && tile_requests == 0);
 ctx.image_ready = false;
 image_card_refresh_due();
 assert(recovered == 0); // Never resurrect a failed transfer's old decoded frame.
 // A successful modal retry clears the warning and schedules tile recovery.
 image_card_apply_modal_downloaded(&ctx);
 assert(cache.ready && ctx.camera_download_errors == 0 && modal_status.empty());
 assert(ctx.next_download_retry_ms == now_ms + 1000);
 // Unavailable HA state cancels requests, waits, then restarts on recovery.
 subscribe_image_card_entity_state(&ctx, ctx.entity_id);
 state_callback("unavailable");
 assert(ctx.camera_entity_unavailable && tile.cancelled == 1 && modal.cancelled == 1);
 assert(image_card_camera_retry_blocked(&ctx));
 assert(ctx.next_download_retry_ms == 0 && pictures == 0);
 state_callback("idle");
 assert(!ctx.camera_entity_unavailable && ctx.camera_download_errors == 0);
 assert(!image_card_camera_retry_blocked(&ctx) && pictures == 1);
 // Media Cover Art keeps its separate existing retry behavior.
 ctx.media_artwork = true; ctx.image_ready = true; tile_status.clear();
 image_card_handle_download_error(&ctx);
 assert(ctx.camera_download_errors == 0 && tile_status.empty());
}
'''
with tempfile.TemporaryDirectory(prefix='camera-feedback-') as temp:
    cpp = Path(temp) / 'test.cpp'
    binary = Path(temp) / 'test'
    cpp.write_text(source)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else 'c++', '-std=c++17',
                    '-Wall', '-Wextra', '-Werror', '-I', str(root / 'components/artwork_image'),
                    str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Camera cache lifecycle, availability, retry backoff and recovery passed.')
