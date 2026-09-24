// Exercise production image handling and scheduling against simulated network/display I/O.
#include <cassert>
#include <functional>
#include <string>
#include "camera_refresh_policy.h"
#include "artwork_controller.h"
#include "../artwork_image/image_pipeline_policy.h"

#define ESP_LOGI(...) ((void) 0)
#define ESP_LOGD(...) ((void) 0)
#define ESP_LOGW(...) ((void) 0)
namespace esphome {
using StringRef = std::string;
uint32_t now = 100000;
uint32_t millis() { return now; }
}
namespace esphome::artwork_image { enum class ImageResizeMode { COVER, FIT }; }
using lv_coord_t = int;
using lv_obj_t = int;
int tile_requests = 0, modal_requests = 0;
struct FakeImage {
  std::string url;
  bool cancelled = false, active = false;
  bool has_image() const { return true; }
  const std::string &get_url() const { return url; }
  bool request_is_active() const { return active; }
  void cancel_update() { cancelled = true; active = false; }
  void set_target_size(int, int) {}
  void set_resize_mode(esphome::artwork_image::ImageResizeMode) {}
  std::string request_update_url(const std::string &value, int) {
    ++tile_requests;
    url = value;
    active = true;
    return value;
  }
};
struct ImageCardCtx {
  bool active = true, media_artwork = false, image_ready = true;
  bool access_token_request_pending = false, explicit_picture_refresh = false;
  bool download_active = false, visible = true, modal = false;
  bool scheduled_tile_request = false, download_queued = false, requested_once = false;
  int startup_download_errors = 0;
  lv_obj_t *widget = nullptr, *btn = nullptr;
  uint32_t retry_deadline_ms = 0, last_tile_request_started_ms = 0;
  uint32_t access_token_request_started_ms = 0;
  std::string entity_id = "image.test", source_url = "http://ha/image?token=old", url, access_token;
  uint8_t media_artwork_retry_mask = 0;
  uint32_t last_download_completed_ms = 99000, next_picture_retry_ms = 0, next_download_retry_ms = 0;
  uint32_t revision_retry_ms = 0;
  uint8_t camera_download_errors = 0;
  bool camera_entity_unavailable = false;
  FakeImage *image = nullptr, *modal_image = nullptr;
  espcontrol::camera::RefreshSchedule refresh_schedule;
  espcontrol::camera::ActivityTrigger activity_trigger;
  espcontrol::camera::ImageRevision revision;
};
struct ModalUi { void *request_timer = nullptr; };
ModalUi modal_ui;
ModalUi &image_card_modal_ui() { return modal_ui; }
constexpr int IMAGE_CARD_MAX_CONTEXTS = 2;
constexpr uint32_t IMAGE_CARD_MIN_REPEAT_REFRESH_MS = 30000;
constexpr uint32_t IMAGE_CARD_MODAL_REFRESH_DELAY_MS = 1000;
constexpr uint32_t IMAGE_CARD_RETRY_INTERVAL_MS = 2000;
FakeImage tile, modal;
ImageCardCtx contexts[2];
constexpr uint32_t IMAGE_CARD_API_RETRY_INTERVAL_MS = 250;
constexpr int IMAGE_CARD_STARTUP_DOWNLOAD_RETRIES = 3;
bool enough_memory = true;
ImageCardCtx *active_download = nullptr;
ImageCardCtx *&image_card_active_download_context() { return active_download; }
void image_card_release_download_slot(ImageCardCtx *ctx, bool = true) {
  ctx->download_active = ctx->download_queued = false;
  if (active_download == ctx) active_download = nullptr;
}
std::function<bool()> &image_card_page_visible() { static std::function<bool()> visible; return visible; }
bool image_card_position_context_widget(ImageCardCtx *, int *width, int *height) {
  *width = *height = 100; return true;
}
int image_card_media_artwork_target_width(ImageCardCtx *, int width) { return width; }
lv_obj_t *image_card_loading_widget(lv_obj_t *) { return nullptr; }
void image_card_position_widget(lv_obj_t *, lv_obj_t *) {}
void image_card_refresh_loading_layout(lv_obj_t *) {}
void image_card_hide_loading(ImageCardCtx *) {}
bool image_card_memory_available(ImageCardCtx *, const char *, int, int) { return enough_memory; }
void image_card_tile_request_size(int width, int height, int *w, int *h) { *w=width; *h=height; }
std::string image_card_sized_url(const std::string &url, int, int) { return url; }
bool connected = true;
ImageCardCtx *image_card_contexts() { return contexts; }
bool ha_api_connected() { return true; } // A diagnostic client may remain connected.
bool ha_api_state_connected() { return connected; }
bool image_card_pipeline_suspended() { return false; }
bool image_card_camera_retry_blocked(ImageCardCtx *) { return false; }
void image_card_camera_download_failed(ImageCardCtx *) {}
uint32_t ha_subscription_generation() { return 1; }
struct HaCoordinator { uint32_t connection_generation() const { return 1; } };
HaCoordinator &ha_read_coordinator() { static HaCoordinator value; return value; }
bool image_card_context_current(ImageCardCtx *, const std::string &, uint32_t) { return true; }
bool image_card_modal_active_for(ImageCardCtx *ctx) { return ctx->modal; }
bool image_card_context_on_active_screen(ImageCardCtx *ctx) { return ctx->visible && (!image_card_page_visible() || image_card_page_visible()()); }
bool image_card_has_separate_modal_image(ImageCardCtx *) { return true; }
std::string string_ref_limited(const std::string &s, size_t n) { return s.substr(0, n); }
std::string image_card_base_url(ImageCardCtx *) { return "http://ha"; }
std::string image_card_join_url(const std::string &, const std::string &s) { return s; }
std::string image_card_entity_proxy_path(const std::string &) { return ""; }
std::string image_card_proxy_path_with_token(const std::string &s, const std::string &) { return s; }
bool image_card_valid_access_token(const std::string &) { return true; }
bool image_card_home_assistant_proxy_authed(const std::string &) { return true; }
bool ha_read_retained_attribute(const std::string &, const std::string &,
                                std::function<void(std::string)>, void * = nullptr) {
  return false;
}
void image_card_log_diagnostics(ImageCardCtx *, const char *, int = 0, int = 0) {}
void image_card_hide(ImageCardCtx *) {}
void image_card_clear_media_artwork(ImageCardCtx *) {}
void image_card_set_loading_state(ImageCardCtx *, const char *, bool = false) {}
void image_card_show_camera_unavailable(ImageCardCtx *) {}
bool image_card_startup_retry_active(ImageCardCtx *, uint32_t = 0) { return false; }
bool image_card_access_token_request_expired(const ImageCardCtx *, uint32_t = 0) { return false; }
void image_card_wait_for_picture(ImageCardCtx *) {}
void image_card_schedule_picture_retry(ImageCardCtx *ctx, uint32_t delay) { ctx->next_picture_retry_ms = esphome::now + delay; }
void image_card_schedule_source_refresh(ImageCardCtx *ctx, uint32_t delay, const char *) { ctx->next_download_retry_ms = esphome::now + delay; }
void image_card_request_source_url(ImageCardCtx *ctx, bool source_changed = false);
bool image_card_queue_modal_source_request(ImageCardCtx *ctx) {
  if (ctx->refresh_schedule.in_flight) return true;
  ++modal_requests;
  ctx->revision.modal_requested = ctx->revision.latest;
  ctx->refresh_schedule.started();
  return true;
}
void image_card_cancel_modal_request_timer() {}
void image_card_apply_downloaded(ImageCardCtx *) {}
void image_card_request_current_picture(ImageCardCtx *) {}
#include "camera_refresh_runtime_functions.h"

void reset() {
  contexts[0] = {};
  contexts[1] = {};
  contexts[1].active = false;
  active_download = nullptr;
  enough_memory = true;
  tile.cancelled = modal.cancelled = false;
  image_card_page_visible() = nullptr;
  contexts[0].image = &tile;
  contexts[0].modal_image = &modal;
  tile_requests = modal_requests = 0;
  connected = true;
  esphome::now = 100000;
}
void finish_tile() {
  auto &ctx = contexts[0];
  image_card_finish_scheduled_tile_request(&ctx, true);
  image_card_release_download_slot(&ctx);
  tile.active = false;
  ctx.revision.tile_applied = ctx.revision.tile_requested;
}
int main() {
  reset();
  auto &ctx = contexts[0];
  ctx.revision.observe("first");
  image_card_handle_picture(&ctx, ctx.source_url);
  assert(tile_requests == 1); // Real revision bypasses the 30-second guard.
  ctx.revision.observe("second");
  ctx.revision.observe("third");
  image_card_refresh_due();
  assert(tile_requests == 1); // Revisions during a download coalesce.
  finish_tile();
  image_card_refresh_due();
  assert(tile_requests == 2);
  finish_tile();
  image_card_refresh_due();
  assert(tile_requests == 2);
  image_card_handle_picture(&ctx, "http://ha/image?token=new");
  assert(ctx.source_url == "http://ha/image?token=new");
  assert(tile_requests == 2); // Credentials are updated without reloading current bytes.
  ctx.explicit_picture_refresh = true;
  esphome::now += 31000;
  image_card_handle_picture(&ctx, ctx.source_url);
  assert(tile_requests == 3); // Explicit HA refresh still works.

  reset();
  ctx.entity_id = "camera.test";
  ctx.last_download_completed_ms = 0;
  ctx.modal = true;
  ctx.refresh_schedule.mode = espcontrol::camera::RefreshMode::ACTIVITY;
  ctx.refresh_schedule.begin(esphome::now, false);
  image_card_handle_picture(&ctx, "http://ha/camera?token=new");
  assert(modal_requests == 0); // A token rotation does not start an activity window.
  ctx.refresh_schedule.activate(esphome::now);
  image_card_refresh_due();
  assert(modal_requests == 1);
  esphome::now += 10000;
  image_card_refresh_due();
  assert(modal_requests == 1); // Actual maintenance loop respects an in-flight request.
  ctx.refresh_schedule.finished(esphome::now, true);
  esphome::now += 5000;
  connected = false;
  image_card_refresh_due();
  assert(modal_requests == 1);
  connected = true;
  image_card_refresh_due();
  assert(modal_requests == 2);
  ctx.refresh_schedule.finished(esphome::now, true);
  esphome::now += 20000;
  image_card_refresh_due();
  assert(modal_requests == 2); // Window expiry stops starting downloads.

  reset();
  ctx.entity_id = "camera.test";
  ctx.last_download_completed_ms = 0;
  ctx.modal = true;
  ctx.refresh_schedule.mode = espcontrol::camera::RefreshMode::PERIODIC;
  ctx.refresh_schedule.begin(esphome::now, false);
  ctx.visible = false;
  image_card_refresh_due();
  assert(!ctx.refresh_schedule.open && modal.cancelled);
  assert(modal_requests == 0);

  reset();
  ctx.entity_id = "camera.test";
  ctx.last_download_completed_ms = 0;
  ctx.refresh_schedule.mode = espcontrol::camera::RefreshMode::ACTIVITY;
  image_card_refresh_due();
  image_card_handle_activity_state(&ctx, "off", true, 1);
  image_card_handle_activity_state(&ctx, "on", true, 1);
  image_card_refresh_due();
  assert(tile_requests == 1 && modal_requests == 0 && ctx.refresh_schedule.in_flight);
  esphome::now += 9000;
  image_card_refresh_due();
  assert(tile_requests == 1); // Slow transfers do not overlap.
  finish_tile();
  esphome::now += 4999;
  image_card_refresh_due();
  assert(tile_requests == 1);
  ++esphome::now;
  image_card_refresh_due();
  assert(tile_requests == 2); // Five seconds after completion, bypassing tile repeat guard.
  image_card_handle_download_error(&ctx);
  assert(ctx.image_ready && ctx.next_download_retry_ms == 0);
  assert(ctx.refresh_schedule.failures == 1);
  esphome::now += 4999;
  image_card_refresh_due();
  assert(tile_requests == 2);
  ++esphome::now;
  image_card_refresh_due();
  assert(tile_requests == 3);
  image_card_handle_download_error(&ctx);
  assert(ctx.refresh_schedule.next_due == esphome::now + 10000);
  image_card_handle_activity_state(&ctx, "off", true, 1);
  image_card_handle_activity_state(&ctx, "on", true, 1);
  assert(ctx.refresh_schedule.window_end == esphome::now + 30000);
  image_card_refresh_due();
  assert(tile_requests == 3); // Further activations cannot bypass failure backoff.
  esphome::now += 10000;
  connected = false;
  image_card_refresh_due();
  assert(tile_requests == 3);
  connected = true;
  image_card_refresh_due();
  assert(tile_requests == 4);
  ctx.visible = false;
  image_card_refresh_due();
  assert(tile.cancelled && !ctx.download_active && !ctx.refresh_schedule.open);
  assert(!ctx.scheduled_tile_request && !ctx.download_queued && ctx.next_download_retry_ms == 0);
  image_card_handle_activity_state(&ctx, "off", true, 1);
  image_card_handle_activity_state(&ctx, "on", true, 1);
  ctx.visible = true;
  image_card_refresh_due();
  assert(tile_requests == 4 && !ctx.refresh_schedule.window); // No hidden-event replay.

  // A trigger received while the page is hidden must not start a later refresh.
  reset();
  ctx.entity_id = "camera.test";
  ctx.last_download_completed_ms = 0;
  ctx.refresh_schedule.mode = espcontrol::camera::RefreshMode::ACTIVITY;
  image_card_handle_activity_state(&ctx, "off", true, 1);
  image_card_handle_activity_state(&ctx, "on", true, 1);
  image_card_refresh_due();
  assert(tile_requests == 1);

  // Screen-off/saver visibility is independent of the underlying LVGL page.
  image_card_refresh_due([] { return false; });
  assert(!ctx.refresh_schedule.open && tile.cancelled);
  image_card_handle_activity_state(&ctx, "off", true, 1);
  image_card_handle_activity_state(&ctx, "on", true, 1);
  image_card_refresh_due([] { return true; });
  assert(tile_requests == 1 && !ctx.refresh_schedule.window);

  // Hiding one tile must not cancel the shared modal belonging to another card.
  reset();
  ctx.entity_id = "camera.test";
  ctx.last_download_completed_ms = 0;
  ctx.refresh_schedule.mode = espcontrol::camera::RefreshMode::ACTIVITY;
  ctx.refresh_schedule.begin(esphome::now, true);
  ctx.visible = false;
  auto &other = contexts[1];
  other.active = other.modal = true;
  other.image = &tile;
  other.modal_image = &modal;
  other.entity_id = "camera.other";
  other.refresh_schedule.mode = espcontrol::camera::RefreshMode::PERIODIC;
  other.refresh_schedule.begin(esphome::now, false);
  image_card_refresh_due();
  assert(!ctx.refresh_schedule.open && !modal.cancelled && modal_requests == 1);

  reset();
  ctx.entity_id = "camera.test";
  ctx.last_download_completed_ms = 0;
  ctx.refresh_schedule.mode = espcontrol::camera::RefreshMode::PERIODIC;
  ctx.refresh_schedule.interval_ms = 5000;
  image_card_refresh_due();
  assert(tile_requests == 1 && ctx.scheduled_tile_request); // Periodic refresh starts on the visible card.
  finish_tile();
  esphome::now += 4999;
  image_card_refresh_due();
  assert(tile_requests == 1);
  esphome::now += 1;
  image_card_refresh_due();
  assert(tile_requests == 2 && ctx.scheduled_tile_request);

  reset();
  ctx.entity_id = "camera.test";
  ctx.last_download_completed_ms = 0;
  ctx.refresh_schedule.mode = espcontrol::camera::RefreshMode::ACTIVITY;
  image_card_handle_activity_state(&ctx, "off", true, 1);
  image_card_handle_activity_state(&ctx, "on", true, 1);
  image_card_refresh_due();
  assert(tile_requests == 1 && ctx.scheduled_tile_request);
  const auto window_end = ctx.refresh_schedule.window_end;
  image_card_preempt_active_tile_for_modal();
  assert(tile.cancelled && !ctx.scheduled_tile_request && !ctx.download_queued);
  assert(!ctx.refresh_schedule.in_flight &&
         ctx.next_download_retry_ms == esphome::now + IMAGE_CARD_MODAL_REFRESH_DELAY_MS);
  ctx.modal = true;
  ctx.refresh_schedule.enter_expanded(esphome::now, true);
  image_card_refresh_due();
  assert(modal_requests == 1 && ctx.refresh_schedule.window_end == window_end);
  ctx.refresh_schedule.finished(esphome::now, true);
  ctx.modal = false;
  ctx.refresh_schedule.leave_expanded();
  image_card_refresh_due();
  assert(tile_requests == 1);
  esphome::now += 5000;
  image_card_refresh_due();
  assert(tile_requests == 2 && ctx.refresh_schedule.window_end == window_end);

}
