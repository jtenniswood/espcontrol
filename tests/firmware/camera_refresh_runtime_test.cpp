// Exercise production image handling and scheduling against simulated network/display I/O.
#include <cassert>
#include <functional>
#include <string>
#include "camera_refresh_policy.h"
#include "artwork_controller.h"
#include "../artwork_image/image_pipeline_policy.h"

#define ESP_LOGD(...) ((void) 0)
#define ESP_LOGW(...) ((void) 0)
namespace esphome {
using StringRef = std::string;
uint32_t now = 100000;
uint32_t millis() { return now; }
}
struct FakeImage {
  std::string url;
  bool cancelled = false;
  bool has_image() const { return true; }
  const std::string &get_url() const { return url; }
  void cancel_update() { cancelled = true; }
};
struct ImageCardCtx {
  bool active = true, media_artwork = false, image_ready = true;
  bool access_token_request_pending = false, explicit_picture_refresh = false;
  bool download_active = false, visible = true, modal = false;
  std::string entity_id = "image.test", source_url = "http://ha/image?token=old", url, access_token;
  uint8_t media_artwork_retry_mask = 0;
  uint32_t last_download_completed_ms = 99000, next_picture_retry_ms = 0, next_download_retry_ms = 0;
  uint32_t revision_retry_ms = 0;
  FakeImage *image = nullptr, *modal_image = nullptr;
  espcontrol::camera::RefreshSchedule refresh_schedule;
  espcontrol::camera::ImageRevision revision;
};
constexpr int IMAGE_CARD_MAX_CONTEXTS = 1;
constexpr uint32_t IMAGE_CARD_MIN_REPEAT_REFRESH_MS = 30000;
constexpr uint32_t IMAGE_CARD_MODAL_REFRESH_DELAY_MS = 1000;
constexpr uint32_t IMAGE_CARD_RETRY_INTERVAL_MS = 2000;
FakeImage tile, modal;
ImageCardCtx contexts[1];
int tile_requests = 0, modal_requests = 0;
bool connected = true;
ImageCardCtx *image_card_contexts() { return contexts; }
bool ha_api_connected() { return true; } // A diagnostic client may remain connected.
bool ha_api_state_connected() { return connected; }
uint32_t ha_subscription_generation() { return 1; }
bool image_card_context_current(ImageCardCtx *, const std::string &, uint32_t) { return true; }
bool image_card_modal_active_for(ImageCardCtx *ctx) { return ctx->modal; }
bool image_card_context_on_active_screen(ImageCardCtx *ctx) { return ctx->visible; }
bool image_card_has_separate_modal_image(ImageCardCtx *) { return true; }
std::string string_ref_limited(const std::string &s, size_t n) { return s.substr(0, n); }
std::string image_card_base_url(ImageCardCtx *) { return "http://ha"; }
std::string image_card_join_url(const std::string &, const std::string &s) { return s; }
std::string image_card_entity_proxy_path(const std::string &) { return ""; }
std::string image_card_proxy_path_with_token(const std::string &s, const std::string &) { return s; }
bool image_card_valid_access_token(const std::string &) { return true; }
bool image_card_home_assistant_proxy_authed(const std::string &) { return true; }
bool ha_read_retained_attribute(const std::string &, const std::string &, std::function<void(std::string)>) { return false; }
void image_card_log_diagnostics(ImageCardCtx *, const char *) {}
void image_card_hide(ImageCardCtx *) {}
void image_card_clear_media_artwork(ImageCardCtx *) {}
void image_card_set_loading_state(ImageCardCtx *, const char *, bool = false) {}
bool image_card_startup_retry_active(ImageCardCtx *) { return false; }
void image_card_schedule_picture_retry(ImageCardCtx *ctx, uint32_t delay) { ctx->next_picture_retry_ms = esphome::now + delay; }
void image_card_schedule_source_refresh(ImageCardCtx *ctx, uint32_t delay, const char *) { ctx->next_download_retry_ms = esphome::now + delay; }
void image_card_request_source_url(ImageCardCtx *ctx, bool = false) {
  if (ctx->modal || ctx->download_active) return;
  ++tile_requests;
  ctx->download_active = true;
  ctx->revision.tile_requested = ctx->revision.latest;
}
void image_card_queue_modal_source_request(ImageCardCtx *ctx) {
  if (ctx->refresh_schedule.in_flight) return;
  ++modal_requests;
  ctx->revision.modal_requested = ctx->revision.latest;
  ctx->refresh_schedule.started();
}
void image_card_cancel_modal_request_timer() {}
void image_card_apply_downloaded(ImageCardCtx *) {}
void image_card_request_current_picture(ImageCardCtx *) {}
#include "camera_refresh_runtime_functions.h"

void reset() {
  contexts[0] = {};
  contexts[0].image = &tile;
  contexts[0].modal_image = &modal;
  tile_requests = modal_requests = 0;
  connected = true;
  esphome::now = 100000;
}
void finish_tile() {
  auto &ctx = contexts[0];
  ctx.download_active = false;
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
  ctx.modal = true;
  ctx.refresh_schedule.mode = espcontrol::camera::RefreshMode::PERIODIC;
  ctx.refresh_schedule.begin(esphome::now, false);
  ctx.visible = false;
  image_card_refresh_due();
  assert(!ctx.refresh_schedule.open && modal.cancelled);
  assert(modal_requests == 0);
}
