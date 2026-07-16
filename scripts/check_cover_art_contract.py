#!/usr/bin/env python3
"""Compile and exercise the pure cover-art policy, layout, and state helpers."""
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = r'''
#include <cassert>
#include <limits>
#include "cover_art.h"
using namespace espcontrol::cover_art;
int main() {
  PolicyInput p; assert(!policy_allows_display(p));
  p.enabled = p.media_playing = p.entity_configured = true; assert(policy_allows_display(p));
  p.external_input_active = p.hide_external_input = true;
  assert(!policy_allows_display(p) && !policy_allows_download(p));
  p.hide_external_input = false; p.schedule_blocks = true;
  assert(!policy_allows_display(p) && policy_allows_download(p));
  p.schedule_blocks = false; p.voice_interaction_active = true; assert(!policy_allows_display(p));
  assert(feature_allowed(true, true, true, false, false));
  assert(!feature_allowed(true, true, true, true, true));
  assert(display_allowed(true, true, true, true, false, false, false, false, false));
  assert(!display_allowed(true, true, true, true, false, false, false, true, false));
  auto ten = cover_art_layout("guition-esp32-p4-jc8012p4a1", "0", 1280, 800, 800, 506);
  assert(ten.split && ten.art_size == 800 && ten.panel_x == 840);
  auto ten_v2 = cover_art_layout("guition-esp32-p4-jc8012p4a1-v2", "90", 800, 1280, 800, 506);
  assert(ten_v2.screen_height == 1280 && ten_v2.panel_y == 834);
  auto four = cover_art_layout("guition-esp32-p4-jc4880p443", "90", 800, 480, 480, 220);
  assert(four.screen_width == 800);
  auto square = cover_art_layout("esp32-p4-86", "0", 720, 720, 800, 495);
  assert(!square.split && square.art_size == 720 && square.panel_padding == 36);
  RuntimeState s; assert(!s.needs_download()); s.select_source("track-a"); assert(s.needs_download());
  s.begin_download("track-a?refresh=1"); s.select_source("track-b");
  assert(s.apply_download("track-a?refresh=1") && s.loaded_url == "track-a" && s.needs_download());
  s.begin_download("track-b"); assert(s.download_active());
  assert(s.apply_download("track-b") && s.current_image_loaded() && !s.needs_download());
  s.select_source("broken");
  s.record_failure(); assert(s.retry_count == 0);
  for (int i = 0; i < MAX_DOWNLOAD_RETRIES; ++i) assert(s.begin_retry());
  assert(!s.can_retry() && s.retry_count == MAX_DOWNLOAD_RETRIES);
  s.select_source("recovered"); assert(s.retry_count == 0 && s.can_retry());
  // Playback stop/disable while a retry is pending must cancel all artwork work.
  s.begin_download("recovered"); s.record_failure(); s.clear_image();
  assert(!s.download_active() && !s.needs_download() && s.retry_count == 0);
  // An entity/track change during retry starts a fresh budget and rejects stale completion.
  s.select_source("entity-a"); s.begin_download("entity-a"); s.record_failure();
  s.select_source("entity-b"); assert(s.retry_count == 0 && s.refresh_needed);
  s.begin_download("entity-b"); assert(!s.apply_download("entity-a"));
  assert(s.apply_download("entity-b") && s.current_image_loaded());
  // Rapid play/pause policy changes cannot bypass alarm, voice, schedule, or source filtering.
  assert(display_allowed(true, true, true, true, false, false, false, false, false));
  assert(!display_allowed(true, false, true, true, false, false, false, false, false));
  assert(!display_allowed(true, true, true, false, false, false, false, false, false));
  assert(!display_allowed(true, true, true, true, false, false, true, false, false));
  // Rotations remain deterministic when events repeat or arrive after boot.
  auto portrait_again = cover_art_layout("guition-esp32-p4-jc1060p470", "90", 600, 1024, 600, 260);
  auto portrait_repeat = cover_art_layout("guition-esp32-p4-jc1060p470", "90", 600, 1024, 600, 260);
  assert(portrait_again.panel_y == portrait_repeat.panel_y && portrait_again.art_size == 600);
  const float infinity = std::numeric_limits<float>::infinity();
  const float invalid = std::numeric_limits<float>::quiet_NaN();
  assert(progress_available(120.0f));
  assert(!progress_available(0.0f) && !progress_available(-1.0f));
  assert(!progress_available(infinity) && !progress_available(invalid));
  assert(progress_percent(0, 0) == 0 && progress_percent(30, 120) == 25 && progress_percent(150, 120) == 100);
  assert(progress_percent(30, infinity) == 0 && progress_percent(30, invalid) == 0);
  assert(progress_percent(invalid, 120) == 0);
}
'''
with tempfile.TemporaryDirectory(prefix="cover-art-contract-") as temp_dir:
    temp = Path(temp_dir); source, binary = temp / "test.cpp", temp / "test"
    source.write_text(SOURCE, encoding="utf-8")
    subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    f"-I{ROOT / 'components' / 'espcontrol'}", str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

downloader = (ROOT / "components" / "artwork_image" / "artwork_image.cpp").read_text(encoding="utf-8")
for required in (
    "max_download_buffer_size_",
    "peak_download_buffer_size_",
    "Artwork download exceeded transfer limit",
    "shrink_to(this->download_buffer_initial_size_)",
    "new (std::nothrow) P4PipelineJob()",
    "P4_PIPELINE_PENDING_SLOTS",
    "P4_PIPELINE_COMPLETED_SLOTS",
    "can_use_p4_pipeline(this->url_)",
):
    if required not in downloader:
        raise SystemExit(f"Artwork downloader memory contract missing: {required}")
if "std::make_shared<P4PipelineJob>" in downloader:
    raise SystemExit("P4 artwork jobs must fail cleanly instead of throwing during allocation")
if "std::vector<P4PipelineResult *> completed_" in downloader:
    raise SystemExit("P4 artwork result publication must not allocate while holding the pipeline lock")
if "header_names_" in downloader:
    raise SystemExit("P4 artwork requests must remove moved headers without retaining allocating copies")
for forbidden in (
    "job->url = url",
    "job->headers = headers",
):
    if forbidden in downloader:
        raise SystemExit(f"P4 artwork job metadata must use checked or moved storage: {forbidden}")
for required in (
    "job->url = static_cast<char *>(heap_caps_malloc(",
    "job->headers = std::move(headers)",
    "if (job->cancelled.load()) {\n      delete result;\n      this->reset_client_();",
):
    if required not in downloader:
        raise SystemExit(f"P4 artwork job safety contract missing: {required}")

jpeg_decoder = (ROOT / "components" / "artwork_image" / "jpeg_image.cpp").read_text(encoding="utf-8")
for required in (
    """if (!this->set_size(target_width, target_height)) {
      p4_release_jpeg_workspace();
      return DECODE_ERROR_OUT_OF_MEMORY;
    }""",
    """if (!this->set_size(info.width, info.height)) {
      p4_release_jpeg_workspace();
      return DECODE_ERROR_OUT_OF_MEMORY;
    }""",
):
    if required not in jpeg_decoder:
        raise SystemExit("P4 JPEG workspace must be released after image buffer allocation failure")

image_cards = (ROOT / "components" / "espcontrol" / "button_grid_image.h").read_text(encoding="utf-8")
for required in (
    "image_card_uses_background_pipeline(next->image, next->source_url)",
):
    if required not in image_cards:
        raise SystemExit(f"Image card background-pipeline contract missing: {required}")
modal_request_start = image_cards.find(
    "inline bool image_card_queue_modal_source_request(ImageCardCtx *ctx) {"
)
modal_request_end = image_cards.find(
    "\ninline void image_card_schedule_source_refresh", modal_request_start
)
if modal_request_start < 0 or modal_request_end < 0:
    raise SystemExit("Image card modal-request contract missing")
modal_request = image_cards[modal_request_start:modal_request_end]
if "ui.request_timer = lv_timer_create(" not in modal_request:
    raise SystemExit("Image card modal requests must let the preview paint before refreshing")
if "image_pipeline_can_start_followup_inline" in modal_request:
    raise SystemExit("Image card modal requests must not bypass their preview-paint delay")
modal_open_start = image_cards.find("inline void image_card_open_modal(ImageCardCtx *ctx) {")
modal_open_end = image_cards.find("\ninline void image_card_handle_picture", modal_open_start)
if modal_open_start < 0 or modal_open_end < 0:
    raise SystemExit("Image card modal-open contract missing")
modal_open = image_cards[modal_open_start:modal_open_end]
if "ctx->next_download_retry_ms = 0;" in modal_open:
    raise SystemExit("Opening an image modal must preserve an already scheduled tile retry")

cover_art = (ROOT / "common" / "device" / "screen_cover_art.yaml").read_text(encoding="utf-8")
resubscribe_start = cover_art.find("  - id: cover_art_resubscribe")
resubscribe_end = cover_art.find("\n  - id:", resubscribe_start + 1)
if resubscribe_start < 0 or resubscribe_end < 0:
    raise SystemExit("Cover art subscription lifecycle contract missing")
resubscribe = cover_art[resubscribe_start:resubscribe_end]
if "ha_get_" in resubscribe:
    raise SystemExit("Cover art must use live subscriptions instead of retained one-shot reads")
proxy_404_start = cover_art.find("last_error_was_ha_media_proxy_not_found()")
proxy_404_end = cover_art.find("- script.execute: cover_art_retry_download", proxy_404_start)
if proxy_404_start < 0 or proxy_404_end < 0:
    raise SystemExit("Cover art proxy 404 recovery contract missing")
proxy_404_recovery = cover_art[proxy_404_start:proxy_404_end]
if "script.execute: cover_art_resubscribe" in proxy_404_recovery:
    raise SystemExit("Cover art proxy 404 recovery must reuse subscriptions instead of retaining one-shot reads")
if "id(cover_art_runtime).sources.clear()" in proxy_404_recovery:
    raise SystemExit("Cover art proxy 404 recovery must preserve newly queued subscription candidates")
for required in (
    "id(cover_art_runtime).refresh_needed = true",
    "preserving subscribed artwork candidates while waiting for an update",
):
    if required not in proxy_404_recovery:
        raise SystemExit(f"Cover art proxy 404 recovery contract missing: {required}")

media = (ROOT / "components" / "espcontrol" / "button_grid_media.h").read_text(encoding="utf-8")
metadata_start = media.find("inline void media_playback_subscribe_metadata(MediaPlaybackState *state) {")
metadata_end = media.find("inline void media_playback_subscribe_progress", metadata_start)
if metadata_start < 0 or metadata_end < 0:
    raise SystemExit("Media metadata subscription contract missing")
metadata = media[metadata_start:metadata_end]
if 'state->entity_id, std::string("media_artist")' in metadata:
    raise SystemExit("Media track changes must not retain duplicate one-shot metadata reads")
if "state->artist.clear()" in metadata:
    raise SystemExit("Media title updates must preserve an unchanged subscribed artist")

grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
media_art_start = grid.find("inline void subscribe_media_cover_art(")
media_art_end = grid.find("\ninline void setup_card_visual(", media_art_start)
if media_art_start < 0 or media_art_end < 0:
    raise SystemExit("Media card cover art subscription contract missing")
media_art = grid[media_art_start:media_art_end]
for required in (
    'std::string("entity_picture")',
    'std::string("entity_picture_local")',
    "image_card_request_media_artwork(art)",
):
    if required not in media_art:
        raise SystemExit(f"Media card cover art subscription contract missing: {required}")
if "subscribe_image_card_entity_state" in media_art:
    raise SystemExit(
        "Media card cover art must not add a general entity-state subscription "
        "on top of its picture subscriptions"
    )
print("Cover art policy, layout, and state contract checks passed.")
