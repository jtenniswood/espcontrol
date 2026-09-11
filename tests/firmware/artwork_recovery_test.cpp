// Production artwork recovery functions run against simulated HA and LVGL I/O.
#include <cassert>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "artwork_controller.h"

namespace esphome { using StringRef = std::string; }
#define ESP_LOGD(...) ((void) 0)
struct lv_timer_t { void *user_data; };
void *lv_timer_get_user_data(lv_timer_t *timer) { return timer->user_data; }
void lv_timer_del(lv_timer_t *timer) { delete timer; }
lv_timer_t *lv_timer_create(void (*)(lv_timer_t *), uint32_t, void *data) {
  return new lv_timer_t{data};
}
struct ImageCardCtx {
  bool active = true, media_artwork = true, image_ready = true;
  std::string entity_id = "media_player.test";
  std::string source_url = "https://ha/artwork/stable.jpg";
  std::string pending_fallback_picture;
  espcontrol::artwork::RefreshBatch media_artwork_refresh;
  espcontrol::artwork::RefreshTrigger media_artwork_trigger;
  espcontrol::artwork::SourceCandidates media_artwork_sources;
  bool media_artwork_refresh_forced = false;
  bool explicit_picture_refresh = false;
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
struct Read {
  std::string attribute;
  std::function<void(esphome::StringRef)> callback;
};
std::vector<Read> reads;
std::vector<std::string> downloads;
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
void image_card_clear_media_artwork(ImageCardCtx *ctx) {
  ctx->image_ready = false;
  ctx->source_url.clear();
  ctx->media_artwork_sources.clear();
  ctx->media_artwork_refresh.reset();
  ctx->media_artwork_trigger.reset();
  ctx->media_artwork_refresh_forced = false;
}
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
  if (scenario == "timeout_retry") {
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
