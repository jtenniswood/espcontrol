#!/usr/bin/env python3
"""Exercise production camera cache/availability callbacks with host UI doubles."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import textwrap

root = Path(__file__).resolve().parents[2]
header = (root / 'components/espcontrol/button_grid_image.h').read_text()

def definition(name):
    matches = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', header, re.M | re.S)
    assert len(matches) == 1, name
    return matches[0]

picture_handler = definition('image_card_handle_picture')
assert re.search(
    r'ha_read_retained_attribute\(\s*entity_id,\s*std::string\("access_token"\),'
    r'.*?\n\s*\}\),\s*ctx\s*\);',
    picture_handler,
    re.S,
), 'picture retry access-token reads must be coalesced per card owner'

source = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include "camera_refresh_policy.h"
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
struct Resettable { void reset() {} };
struct ImageCardCtx {
 bool active = true, media_artwork = false, image_ready = true;
 bool camera_entity_unavailable = false, download_active = false, modal_fit = false;
 bool diagnostics_enabled = false; uint32_t last_modal_request_started_ms = 0;
 uint8_t camera_download_errors = 0, startup_download_errors = 0;
 uint32_t camera_retry_after_ms = 0, next_download_retry_ms = 0;
 uint32_t access_token_request_started_ms = 0;
 uint32_t next_picture_retry_ms = 0, last_download_completed_ms = 0;
 Image *image = nullptr, *modal_image = nullptr;
 Widget *widget = nullptr;
 std::string entity_id = "camera.front", source_url = "snapshot", url = "snapshot";
 std::string modal_url = "snapshot", modal_source_url = "snapshot";
 std::function<void(espcontrol::DisplayTakeoverKind)> end_display_takeover;
 bool requested_once = false, access_token_request_pending = false, camera_refresh_pending = false;
 bool scheduled_tile_request = false;
 bool media_artwork_refresh_forced = false;
 uint32_t retry_deadline_ms = 0, media_artwork_retry_mask = 0, media_artwork_timeout_retries = 0;
 uint32_t revision_retry_ms = 0;
 espcontrol::camera::RefreshSchedule refresh_schedule;
 espcontrol::camera::ActivityTrigger activity_trigger;
 espcontrol::camera::ImageRevision revision;
 std::string refresh_trigger_entity;
 std::string access_token, pending_fallback_picture;
 Resettable media_artwork_refresh, media_artwork_trigger;
 Timer *modal_cleanup_timer = nullptr, *media_artwork_trigger_timer = nullptr, *media_artwork_timer = nullptr;
};
struct ImageCardModalUi {
 ImageCardCtx *active = nullptr; Widget *image_widget = nullptr, *overlay = nullptr;
 Widget *panel = nullptr, *back_btn = nullptr;
 Timer *request_timer = nullptr;
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
void image_card_schedule_modal_cache_expiry(Image *);
bool constrained = true;
bool image_card_constrained_memory_profile() { return constrained; }
bool suspended = false;
bool &image_card_pipeline_suspended_state() { return suspended; }
bool image_card_pipeline_suspended() { return suspended; }
bool ha_api_connected() { return true; }
bool ha_api_state_connected() { return true; }
bool image_card_context_visible_on_active_screen(ImageCardCtx *) { return true; }
bool image_card_context_on_active_screen(ImageCardCtx *ctx) {
 return image_card_context_visible_on_active_screen(ctx);
}
std::function<bool()> &image_card_page_visible() { static std::function<bool()> page; return page; }
bool image_card_modal_active_for(ImageCardCtx *ctx) { return ui.active == ctx; }
std::string tile_status, modal_status;
int cleared = 0, pictures = 0, tile_requests = 0, modal_requests = 0, recovered = 0;
int retained_state_reads = 0;
std::string retained_state = "idle";
void image_card_hide(ImageCardCtx *ctx) { if (ctx->widget) ctx->widget->hidden = true; }
void image_card_set_loading_state(ImageCardCtx *, const char *s, bool) { tile_status = s; }
void image_card_show_modal_loading(ImageCardCtx *, const char *s) { modal_status = s; }
void image_card_clear_widget_source(Widget *) { ++cleared; }
void image_card_hide_loading(ImageCardCtx *) { tile_status.clear(); }
void image_card_release_download_slot(ImageCardCtx *c) { c->download_active = false; }
void image_card_log_diagnostics(ImageCardCtx *, const char *) {}
void image_card_cancel_modal_request_timer() {}
enum class ControlModalKind { NONE, IMAGE_CARD };
struct Modal { ControlModalKind kind = ControlModalKind::NONE; } active_modal;
Modal &control_modal_active() { return active_modal; }
void control_modal_delete_overlay(ControlModalKind, Widget *) {}
void image_card_schedule_modal_cleanup(ImageCardCtx *) {}
void *lv_timer_get_user_data(Timer *t) { return t->data; }
void lv_timer_del(Timer *t) { delete t; }
Timer *lv_timer_create(void (*)(Timer *), uint32_t delay, void *data) {
 return new Timer{data, delay};
}
constexpr uint32_t IMAGE_CARD_CONSTRAINED_MODAL_CACHE_TTL_MS = 15000;
constexpr uint32_t IMAGE_CARD_RETRY_INTERVAL_MS = 2000;
constexpr uint32_t IMAGE_CARD_STARTUP_RETRY_MS = 30000;
constexpr uint32_t IMAGE_CARD_ATTRIBUTE_REQUEST_TIMEOUT_MS = 10000;
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
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_DEFAULT = 1;
uint32_t ha_subscription_generation() { return 1; }
bool image_card_context_current(ImageCardCtx *, const std::string &, uint32_t) { return true; }
std::string string_ref_limited(const std::string &v, size_t n) { return v.substr(0,n); }
std::function<void(std::string)> state_callback;
void ha_subscribe_state(const std::string &, std::function<void(std::string)> cb,
                        uint32_t = HA_SUBSCRIPTION_SCOPE_DEFAULT, bool = false) {
 state_callback = cb;
}
bool ha_read_retained_state(const std::string &, std::function<void(std::string)> cb,
                            void * = nullptr) {
 ++retained_state_reads;
 cb(retained_state);
 return true;
}
void image_card_request_picture(ImageCardCtx *) { ++pictures; }
void image_card_request_current_picture(ImageCardCtx *) { ++pictures; }
void image_card_refresh_current_picture(ImageCardCtx *) { ++pictures; }
void image_card_request_source_url(ImageCardCtx *) { ++tile_requests; }
bool image_card_queue_modal_source_request(ImageCardCtx *) { ++modal_requests; return true; }
void image_card_cancel_scheduled_tile_request(ImageCardCtx *ctx) {
 ctx->scheduled_tile_request = false;
 ctx->refresh_schedule.in_flight = false;
}
void image_card_begin_refresh_schedule(ImageCardCtx *ctx) {
 ctx->refresh_schedule.begin(now_ms, false);
}
void image_card_apply_downloaded(ImageCardCtx *) { ++recovered; }
#define ESP_LOGI(...) do {} while(false)
#define ESP_LOGW(...) do {} while(false)
'''
# Use unchanged production definitions, rather than reproducing their logic.
for name in ('image_card_startup_retry_active', 'image_card_schedule_picture_retry',
             'image_card_access_token_request_expired', 'image_card_show_camera_unavailable',
             'image_card_wait_for_picture',
             'image_card_modal_cache_expired', 'image_card_cancel_modal_cache_expiry',
             'image_card_release_modal_cache', 'image_card_modal_cache_expiry_timer_cb',
             'image_card_schedule_modal_cache_expiry',
             'image_card_camera_retry_blocked', 'image_card_camera_download_failed',
             'image_card_finish_scheduled_tile_request', 'image_card_handle_download_error',
             'image_card_modal_has_tile_fallback',
             'image_card_modal_cache_matches', 'image_card_modal_needs_open_refresh',
             'image_card_apply_entity_state', 'subscribe_image_card_entity_state',
             'image_card_refresh_entity_state',
             'image_card_hide_modal', 'image_card_apply_modal_downloaded'):
    source += definition(name) + '\n'
# The polling callback checks get_url as well as availability.
source = source.replace('bool has_image() {', 'std::string get_url() { return "snapshot"; }\n bool has_image() {')
source += definition('image_card_refresh_due')
source += definition('refresh_visible_image_cards')
source += definition('image_card_suspend_pipeline')
source += definition('image_card_resume_pipeline')
camera_yaml = (root / 'common/device/screen_camera_screensaver.yaml').read_text()
callback = re.search(
    r'std::function<void\(esphome::StringRef\)>\(\[entity, subscription_generation\]\(esphome::StringRef state\) \{.*?\}\)',
    camera_yaml, re.S)
assert callback, 'screensaver state callback'
source += r'''
struct Setting { std::string state = "camera.front"; } screensaver_camera_entity;
struct Script { int calls = 0; void execute() { ++calls; } };
Script camera_screensaver_show_unavailable, camera_screensaver_request_image;
Image *camera_screensaver_downloaded_image;
bool camera_screensaver_entity_unavailable = false;
bool camera_screensaver_request_pending = false, camera_screensaver_image_available = true;
#define id(x) x
auto screensaver_callback(std::string entity, uint32_t subscription_generation) {
  return ''' + callback[0] + r''';
}
'''
error_callback = textwrap.dedent(camera_yaml.split('    on_error:\n      - lambda: |-\n', 1)[1]
                                 .split('\n      - if:', 1)[0])
source += r'''
namespace espcontrol { enum class DisplayMode { CAMERA }; }
struct CameraDisplay {
 bool active = true, current = true;
 bool target_mode_is(espcontrol::DisplayMode) { return active; }
 bool transition_is_current(uint32_t, espcontrol::DisplayMode) { return current; }
};
struct CameraApp { CameraDisplay state; CameraDisplay &display() { return state; } } espcontrol_app;
struct Retry { int delay_ms = -1; void execute(int delay) { delay_ms = delay; } } camera_screensaver_retry;
uint32_t camera_screensaver_transition_generation = 1;
bool camera_screensaver_use_sized_request = false;
void screensaver_download_error() {
''' + error_callback + '\n}\n'
source += r'''
int main() {
 // Pending camera cards leave the startup spinner after its grace period,
 // retry missing HA state, and preserve an already displayed image.
 ImageCardCtx waiting;
 Widget waiting_widget;
 waiting.widget = &waiting_widget;
 waiting.image_ready = false;
 waiting.retry_deadline_ms = 10000;
 now_ms = 0;
 image_card_wait_for_picture(&waiting);
 assert(tile_status == "Loading" && waiting.next_picture_retry_ms == 2000);
 assert(!waiting_widget.hidden);
 now_ms = 10000;
 image_card_wait_for_picture(&waiting);
 assert(tile_status == "Unavailable" && waiting.next_picture_retry_ms == 12000);
 assert(waiting_widget.hidden);
 waiting.image_ready = true;
 waiting_widget.hidden = false;
 image_card_wait_for_picture(&waiting);
 assert(tile_status.empty() && !waiting_widget.hidden);
 waiting.image_ready = false;
 waiting.camera_entity_unavailable = true;
 waiting.next_picture_retry_ms = 0;
 image_card_wait_for_picture(&waiting);
 assert(tile_status == "Unavailable" && waiting.next_picture_retry_ms == 0);
 waiting.access_token_request_pending = true;
 waiting.access_token_request_started_ms = now_ms;
 assert(!image_card_access_token_request_expired(&waiting, now_ms + 9999));
 assert(image_card_access_token_request_expired(&waiting, now_ms + 10000));

 // Original-image failures try a bounded snapshot on the next loop; bounded
 // failures retain backoff, and stale/inactive sessions cannot enable fallback.
 screensaver_download_error();
 assert(camera_screensaver_use_sized_request && camera_screensaver_retry.delay_ms == 1);
 screensaver_download_error();
 assert(camera_screensaver_retry.delay_ms == 10000);
 camera_screensaver_use_sized_request = false;
 espcontrol_app.state.current = false;
 screensaver_download_error();
 assert(!camera_screensaver_use_sized_request && camera_screensaver_retry.delay_ms == 10000);
 espcontrol_app.state.active = false;
 camera_screensaver_retry.delay_ms = -1;
 screensaver_download_error();
 assert(!camera_screensaver_use_sized_request && camera_screensaver_retry.delay_ms == -1);
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
 // A shared modal image remains alive while another card is using it when its
 // retention timer expires; the timer must be restarted until that modal closes.
 modal.available = true; modal.released = 0; cache.image = &modal; cache.ready = true;
 cache.entity_id = ctx.entity_id; cache.source_url = ctx.source_url;
 cache.cached_at_ms = 1000; now_ms = 2000; ui.active = &ctx;
 image_card_schedule_modal_cache_expiry(&modal);
 now_ms = 17000;
 image_card_modal_cache_expiry_timer_cb(cache.expiry_timer);
 assert(modal.released == 0 && cache.ready && cache.expiry_timer);
 assert(cache.cached_at_ms == now_ms);
 ui.active = nullptr;
 now_ms += 15000;
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
 assert(ctx.next_download_retry_ms == now_ms); // Keep retry alive until request starts.
 ctx.image_ready = false;
 image_card_refresh_due();
 assert(recovered == 0); // Never resurrect a failed transfer's old decoded frame.
 // A successful modal retry clears the warning and schedules tile recovery.
 image_card_apply_modal_downloaded(&ctx);
 assert(cache.ready && ctx.camera_download_errors == 0 && modal_status.empty());
 assert(ctx.next_download_retry_ms == now_ms + 1000);
 // Unavailable HA state cancels requests, waits, then restarts on recovery.
 subscribe_image_card_entity_state(&ctx, ctx.entity_id);
 const int tile_cancelled_before_unavailable = tile.cancelled;
 const int modal_cancelled_before_unavailable = modal.cancelled;
 state_callback("unavailable");
 assert(ctx.camera_entity_unavailable &&
        tile.cancelled == tile_cancelled_before_unavailable + 1 &&
        modal.cancelled == modal_cancelled_before_unavailable + 1);
 assert(image_card_camera_retry_blocked(&ctx));
 assert(ctx.next_download_retry_ms == 0 && pictures == 0);
 state_callback("idle");
 assert(!ctx.camera_entity_unavailable && ctx.camera_download_errors == 0);
 assert(!image_card_camera_retry_blocked(&ctx) && pictures == 1);
 // Resuming the shared image pipeline must revalidate retained availability.
 ctx.camera_entity_unavailable = true;
 ctx.active = true;
 image_card_refresh_entity_state(&ctx);
 assert(retained_state_reads == 1 && !ctx.camera_entity_unavailable);
 // A previously hidden subpage also recovers from retained HA state on entry.
 ctx.camera_entity_unavailable = true;
 refresh_visible_image_cards();
 assert(retained_state_reads == 2 && !ctx.camera_entity_unavailable);
 // P4 panels retain card images and credentials across camera screensaver entry.
 constrained = false;
 ctx.access_token = "p4-token";
 ctx.image_ready = true;
 const int releases_before_p4 = tile.released + modal.released;
 const int pictures_before_p4 = pictures;
 image_card_suspend_pipeline();
 image_card_resume_pipeline();
 assert(!suspended && ctx.active && ctx.image_ready);
 assert(ctx.access_token == "p4-token");
 assert(tile.released + modal.released == releases_before_p4);
 assert(pictures == pictures_before_p4 && retained_state_reads == 2);
 constrained = true;
 // Tokens may rotate while callbacks are suspended; never reuse the old token.
 ctx.access_token = "old-token";
 image_card_suspend_pipeline();
 assert(suspended && !ctx.active && ctx.access_token.empty());
 assert(!ctx.access_token_request_pending);
 // Visible cameras revalidate availability when the real resume path runs.
 ctx.camera_entity_unavailable = true;
 image_card_resume_pipeline();
 assert(!suspended && ctx.active && !ctx.camera_entity_unavailable);
 assert(retained_state_reads == 3);
 // An unavailable media player must not acquire the camera-only blocker on wake.
 ctx.media_artwork = true; ctx.entity_id = "media_player.lounge";
 image_card_suspend_pipeline();
 retained_state = "unavailable";
 const int pictures_before_media_resume = pictures;
 image_card_resume_pipeline();
 assert(ctx.active && !ctx.camera_entity_unavailable);
 assert(retained_state_reads == 3 && pictures > pictures_before_media_resume);
 retained_state = "playing";
 refresh_visible_image_cards();
 assert(!ctx.camera_entity_unavailable && pictures > pictures_before_media_resume + 1);
 // Media Cover Art keeps its separate existing retry behavior.
 ctx.media_artwork = true; ctx.image_ready = true; tile_status.clear();
 image_card_handle_download_error(&ctx);
 assert(ctx.camera_download_errors == 0 && tile_status.empty());
 // A definitive unavailable state discards the displayed frame, then recovers.
 camera_screensaver_downloaded_image = &tile;
 auto camera_state = screensaver_callback("camera.front", 1);
 for (const auto *status : {"unavailable", "unknown"}) {
   camera_screensaver_image_available = true;
   camera_screensaver_request_pending = true;
   const int cancelled = tile.cancelled;
   camera_state(status);
   assert(camera_screensaver_entity_unavailable && !camera_screensaver_image_available);
   assert(!camera_screensaver_request_pending && tile.cancelled == cancelled + 1);
 }
 assert(camera_screensaver_show_unavailable.calls == 2);
 camera_state("idle");
 assert(!camera_screensaver_entity_unavailable && camera_screensaver_request_image.calls == 1);
 // Old subscriptions cannot hide the new entity's image or restart requests.
 screensaver_callback("camera.old", 1)("unavailable");
 screensaver_callback("camera.front", 0)("unavailable");
 assert(!camera_screensaver_entity_unavailable && camera_screensaver_show_unavailable.calls == 2);
}
'''
with tempfile.TemporaryDirectory(prefix='camera-feedback-') as temp:
    cpp = Path(temp) / 'test.cpp'
    binary = Path(temp) / 'test'
    cpp.write_text(source)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else 'c++', '-std=c++17',
                    '-Wall', '-Wextra', '-Werror', '-I', str(root / 'components/artwork_image'),
                    '-I', str(root / 'components/espcontrol'),
                    str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Camera cache lifecycle, availability, retry backoff and recovery passed.')
