// Production artwork recovery functions run against simulated HA and LVGL I/O.
#include <cassert>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "artwork_controller.h"

uint32_t current_time = 14u * 60 * 60 * 1000;
namespace esphome {
using StringRef = std::string;
uint32_t millis() { return current_time; }
}
#define ESP_LOGD(...) ((void) 0)
#define ESP_LOGI(...) ((void) 0)
#define ESP_LOGW(...) ((void) 0)
struct lv_timer_t { void *user_data; };
void *lv_timer_get_user_data(lv_timer_t *timer) { return timer->user_data; }
void lv_timer_del(lv_timer_t *timer) { delete timer; }
lv_timer_t *lv_timer_create(void (*)(lv_timer_t *), uint32_t, void *data) {
  return new lv_timer_t{data};
}
struct ImageCardCtx {
  bool active = true, media_artwork = true, image_ready = true;
  bool download_active = false, diagnostics_enabled = false, requested_once = false;
  void *widget = nullptr, *btn = nullptr, *media_overlay = nullptr;
  struct Image {
    std::string url;
    const std::string &get_url() const { return url; }
    void release() {}
  } image_storage;
  Image *image = &image_storage;
  std::string url;
  uint32_t retry_deadline_ms = 45000, next_download_retry_ms = 0;
  uint32_t last_download_completed_ms = 0, last_tile_request_started_ms = 0;
  std::string entity_id = "media_player.test";
  std::string source_url = "https://ha/artwork/stable.jpg";
  std::string pending_fallback_picture;
  espcontrol::artwork::RefreshBatch media_artwork_refresh;
  espcontrol::artwork::RefreshTrigger media_artwork_trigger;
  espcontrol::artwork::DownloadRecovery media_artwork_download_recovery;
  espcontrol::artwork::SourceCandidates media_artwork_sources;
  bool media_artwork_refresh_forced = false;
  uint8_t media_artwork_retry_mask = 0, media_artwork_timeout_retries = 0;
  uint8_t startup_download_errors = 0;
  uint32_t next_picture_retry_ms = 0;
  lv_timer_t *media_artwork_timer = nullptr;
  lv_timer_t *media_artwork_trigger_timer = nullptr;
  ~ImageCardCtx() {
    delete media_artwork_timer;
    delete media_artwork_trigger_timer;
  }
};
constexpr uint32_t IMAGE_CARD_MEDIA_ARTWORK_MAX_TIMEOUT_RETRIES = 3;
constexpr uint32_t IMAGE_CARD_API_RETRY_INTERVAL_MS = 5000;
constexpr uint32_t IMAGE_CARD_RETRY_INTERVAL_MS = 5000;
constexpr uint32_t IMAGE_CARD_MEDIA_ARTWORK_TRIGGER_DEBOUNCE_MS = 100;
constexpr uint32_t IMAGE_CARD_MEDIA_ARTWORK_RESPONSE_DEBOUNCE_MS = 100;
constexpr uint8_t IMAGE_CARD_STARTUP_DOWNLOAD_RETRIES = 10;
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_DEFAULT = 1;
constexpr int LV_OBJ_FLAG_HIDDEN = 1;
struct Read {
  std::string attribute;
  std::function<void(esphome::StringRef)> callback;
};
std::vector<Read> reads;
std::vector<std::string> downloads;
std::vector<std::string> fresh_requests;
void ha_schedule_metadata_refresh(const std::string &,
                                  std::initializer_list<const char *> attributes,
                                  uint32_t) {
  for (const auto *attribute : attributes) fresh_requests.emplace_back(attribute);
}
void image_card_release_download_slot(ImageCardCtx *ctx) { ctx->download_active = false; }
void image_card_clear_widget_source(void *) {}
void image_card_hide(ImageCardCtx *) {}
void image_card_hide_loading(ImageCardCtx *) {}
void image_card_sync_media_artwork_visibility(ImageCardCtx *) {}
void image_card_set_widget_source(void *, ImageCardCtx::Image *) {}
void lv_obj_add_flag(void *, int) {}
void lv_obj_clear_flag(void *, int) {}
void lv_obj_move_background(void *) {}
void lv_obj_invalidate(void *) {}
void notify_dashboard_content_changed() {}
bool image_card_startup_retry_active(ImageCardCtx *ctx, uint32_t now) {
  return ctx->retry_deadline_ms != 0 && int32_t(now - ctx->retry_deadline_ms) < 0;
}
bool image_card_modal_active_for(ImageCardCtx *) { return false; }
bool ha_api_connected() { return true; }
bool ha_api_state_connected() { return true; }
uint32_t ha_subscription_generation() { return 1; }
bool ha_read_retained_attribute(const std::string &, const std::string &attribute,
                                std::function<void(esphome::StringRef)> callback,
                                ImageCardCtx *) {
  reads.push_back({attribute, std::move(callback)});
  return true;
}
bool image_card_context_current(ImageCardCtx *, const std::string &, uint32_t) { return true; }
void image_card_log_diagnostics(ImageCardCtx *, const char *) {}
void image_card_set_loading_state(ImageCardCtx *, const char *, bool) {}
std::string string_ref_limited(const std::string &value, size_t limit) { return value.substr(0, limit); }
std::string image_card_base_url(ImageCardCtx *) { return "https://ha"; }
std::string image_card_join_url(const std::string &, const std::string &value) { return value; }
void image_card_handle_picture(ImageCardCtx *ctx, const std::string &url) {
  downloads.push_back(url);
  ctx->source_url = url;
  ctx->image_ready = true;
}
void image_card_request_picture(ImageCardCtx *) {}
void image_card_schedule_picture_retry(ImageCardCtx *ctx, uint32_t delay) {
  ctx->next_picture_retry_ms = delay;
}
void image_card_request_media_artwork(ImageCardCtx *, bool force_refresh = false);
void image_card_schedule_media_artwork_refresh(ImageCardCtx *, bool force_refresh = false);

// Extracted from button_grid_image.h by the Python runner, without changes.
#include "artwork_recovery_functions.h"

void run_trigger(ImageCardCtx &ctx) {
  assert(ctx.media_artwork_trigger_timer);
  image_card_media_artwork_trigger_timer_cb(ctx.media_artwork_trigger_timer);
}
void timeout(ImageCardCtx &ctx) {
  assert(ctx.media_artwork_timer);
  image_card_media_artwork_timer_cb(ctx.media_artwork_timer);
}
void respond(const std::string &attribute, const std::string &value) {
  for (auto it = reads.begin(); it != reads.end(); ++it) {
    if (it->attribute != attribute) continue;
    auto callback = std::move(it->callback);
    reads.erase(it);
    callback(value);
    return;
  }
  assert(false && "No pending attribute read");
}
void respond_pair(ImageCardCtx &ctx) {
  respond("entity_picture", ctx.source_url);
  respond("entity_picture_local", "");
}
void retry(ImageCardCtx &ctx) {
  assert(ctx.next_picture_retry_ms != 0);
  ctx.next_picture_retry_ms = 0;
  reads.clear();  // old transport replies were lost; retry creates new callbacks
  image_card_request_current_picture(&ctx);
}
void reconnect(ImageCardCtx &ctx) {
  reads.clear();
  image_card_refresh_current_picture(&ctx);
  run_trigger(ctx);
}
int main(int argc, char **argv) {
  assert(argc == 2);
  const std::string scenario = argv[1];
  ImageCardCtx ctx;
  ctx.media_artwork_sources.update(false, ctx.source_url);
  ctx.media_artwork_sources.finish_refresh();
  if (scenario == "late_download_failure") {
    ctx.image_ready = false;
    image_card_handle_download_error(&ctx);
    assert(ctx.media_artwork_download_recovery.active());
    assert(!ctx.media_artwork_download_recovery.due(current_time + 1999));
    image_card_recover_media_artwork(&ctx, current_time + 2000);
    assert((fresh_requests == std::vector<std::string>{"entity_picture", "entity_picture_local"}));
    assert(downloads.empty()); // Must obtain fresh HA attributes before retrying.
  } else if (scenario == "failed_same_url") {
    ctx.image_ready = false;
    image_card_schedule_media_artwork_refresh(&ctx);
    run_trigger(ctx);
    respond_pair(ctx);
    assert(downloads.size() == 1); // Missing image is never a successful cache hit.
  } else if (scenario == "retry_backoff") {
    ctx.image_ready = false;
    image_card_handle_download_error(&ctx);
    for (uint32_t delay : {2000u, 4000u, 8000u, 16000u, 32000u, 60000u, 60000u}) {
      assert(ctx.media_artwork_download_recovery.retry_at == current_time + delay);
      current_time += delay;
      image_card_recover_media_artwork(&ctx, current_time);
      // No HA reply: recovery must retain its next deadline, with bounded backoff.
    }
    assert(fresh_requests.size() == 14);
  } else if (scenario == "fresh_url_recovery") {
    ctx.image_ready = false;
    image_card_handle_download_error(&ctx);
    // Repeated ordinary notifications must not bypass the backoff.
    image_card_schedule_media_artwork_refresh(&ctx);
    run_trigger(ctx);
    respond_pair(ctx);
    assert(downloads.empty());
    image_card_recover_media_artwork(&ctx, current_time + 2000);
    image_card_schedule_media_artwork_refresh(&ctx); // Fresh subscription notification.
    run_trigger(ctx);
    respond("entity_picture", "https://ha/cover?token=renewed");
    respond("entity_picture_local", "");
    assert(downloads.size() == 1 && downloads.back() == "https://ha/cover?token=renewed");
  } else if (scenario == "no_artwork_cancels_recovery") {
    image_card_handle_download_error(&ctx);
    image_card_schedule_media_artwork_refresh(&ctx);
    run_trigger(ctx);
    respond("entity_picture", "");
    image_card_recover_media_artwork(&ctx, current_time + 60000);
    assert(!ctx.media_artwork_download_recovery.active());
    assert(fresh_requests.empty());
  } else if (scenario == "track_change_resets_recovery") {
    image_card_handle_download_error(&ctx);
    ctx.media_artwork_download_recovery.step = 6;
    image_card_refresh_media_artwork_on_metadata_change(&ctx);
    assert(!ctx.media_artwork_download_recovery.active());
    run_trigger(ctx);
    respond_pair(ctx);
    assert(downloads.size() == 1);
    image_card_handle_download_error(&ctx);
    assert(ctx.media_artwork_download_recovery.delay_ms() == 2000);
  } else if (scenario == "success_cancels_recovery") {
    image_card_handle_download_error(&ctx);
    ctx.widget = &ctx;
    ctx.url = ctx.image->url = ctx.source_url;
    image_card_apply_downloaded(&ctx);
    assert(ctx.image_ready && !ctx.media_artwork_download_recovery.active());
    image_card_recover_media_artwork(&ctx, current_time + 60000);
    assert(fresh_requests.empty());
  } else if (scenario == "retry_waits_for_download") {
    image_card_handle_download_error(&ctx);
    ctx.download_active = true;
    image_card_recover_media_artwork(&ctx, current_time + 60000);
    assert(fresh_requests.empty());
    ctx.download_active = false;
    image_card_recover_media_artwork(&ctx, current_time + 60000);
    assert(fresh_requests.size() == 2);
  } else if (scenario == "retry_clock_wrap") {
    current_time = UINT32_MAX - 999;
    image_card_handle_download_error(&ctx);
    image_card_recover_media_artwork(&ctx, current_time + 1999);
    assert(fresh_requests.empty());
    image_card_recover_media_artwork(&ctx, current_time + 2000);
    assert(fresh_requests.size() == 2);
  } else if (scenario == "failed_same_url_recovery") {
    ctx.image_ready = false;
    image_card_handle_download_error(&ctx);
    image_card_recover_media_artwork(&ctx, current_time + 2000);
    image_card_schedule_media_artwork_refresh(&ctx);
    run_trigger(ctx);
    respond_pair(ctx);
    assert(downloads.size() == 1);
  } else if (scenario == "previous_image_preserved") {
    image_card_handle_download_error(&ctx);
    assert(ctx.image_ready && ctx.media_artwork_download_recovery.active());
    image_card_recover_media_artwork(&ctx, current_time + 2000);
    image_card_schedule_media_artwork_refresh(&ctx);
    run_trigger(ctx);
    respond_pair(ctx);
    assert(downloads.size() == 1); // Old pixels must not suppress recovery of the failed replacement.
  } else if (scenario == "timeout_retry") {
    image_card_refresh_media_artwork_on_metadata_change(&ctx);
    run_trigger(ctx);
    timeout(ctx);
    assert(downloads.empty());
    retry(ctx);
    respond_pair(ctx);
    assert(downloads.size() == 1);
    reconnect(ctx);
    respond_pair(ctx);
    assert(downloads.size() == 1);
  } else if (scenario == "unchanged") {
    reconnect(ctx);
    respond_pair(ctx);
    reconnect(ctx);
    respond_pair(ctx);
    assert(downloads.empty());
  } else if (scenario == "missing_companion") {
    image_card_refresh_media_artwork_on_metadata_change(&ctx);
    run_trigger(ctx);
    respond("entity_picture", ctx.source_url);
    timeout(ctx);
    assert(downloads.size() == 1);
    retry(ctx);
    // Several absent optional responses must not redownload handled artwork.
    timeout(ctx);
    assert(downloads.size() == 1);
    retry(ctx);
    respond("entity_picture_local", "");
    assert(downloads.size() == 1);
  } else if (scenario == "pending_metadata_reconnect") {
    image_card_refresh_media_artwork_on_metadata_change(&ctx);
    reconnect(ctx);
    respond_pair(ctx);
    assert(downloads.size() == 1);
  } else if (scenario == "active_metadata_reconnect") {
    image_card_refresh_media_artwork_on_metadata_change(&ctx);
    run_trigger(ctx);
    reconnect(ctx);
    respond_pair(ctx);
    assert(downloads.size() == 1);
  } else if (scenario == "missing_image_reconnect") {
    ctx.image_ready = false;
    reconnect(ctx);
    respond_pair(ctx);
    assert(downloads.size() == 1);
  } else if (scenario == "exhausted_reconnect") {
    image_card_refresh_media_artwork_on_metadata_change(&ctx);
    run_trigger(ctx);
    for (uint32_t i = 0; i < IMAGE_CARD_MEDIA_ARTWORK_MAX_TIMEOUT_RETRIES; ++i) {
      timeout(ctx);
      retry(ctx);
    }
    timeout(ctx);
    assert(downloads.empty());
    assert(ctx.next_picture_retry_ms == 0);
    reconnect(ctx);
    respond_pair(ctx);
    assert(downloads.size() == 1);
  } else {
    assert(false && "Unknown test scenario");
  }
}
