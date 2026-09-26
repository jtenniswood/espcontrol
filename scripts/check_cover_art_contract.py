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
  assert(external_media_source("TV"));
  assert(external_media_source(" line-in "));
  assert(external_media_source("Line In"));
  assert(external_media_source("HDMI"));
  assert(external_media_source("hdmi 1"));
  assert(!external_media_source("Spotify"));
  assert(media_entity_state_usable("playing"));
  assert(media_entity_state_usable("paused"));
  assert(media_entity_state_usable(" BUFFERING "));
  assert(!media_entity_state_usable("idle"));
  assert(!media_entity_state_usable("off"));
  assert(!media_entity_state_usable(" unavailable "));
  assert(media_artwork_content_current(true, true, "playing", true));
  assert(media_artwork_content_current(true, true, "paused", true));
  assert(media_artwork_content_current(true, true, "buffering", true));
  assert(!media_artwork_content_current(true, true, "idle", true));
  assert(!media_artwork_content_current(true, true, "off", true));
  assert(!media_artwork_content_current(true, false, "playing", true));
  assert(!media_artwork_content_current(false, true, "playing", true));
  assert(!media_artwork_content_current(true, true, "playing", false));
  assert(media_state_change_invalidates_retained_content(false, "unknown", "idle"));
  assert(media_state_change_invalidates_retained_content(true, "playing", "idle"));
  assert(media_state_change_invalidates_retained_content(true, "paused", "off"));
  assert(media_state_change_invalidates_retained_content(true, "idle", "off"));
  assert(!media_state_change_invalidates_retained_content(true, "idle", "idle"));
  assert(!media_state_change_invalidates_retained_content(true, "idle", "playing"));
  assert(!media_state_change_invalidates_retained_content(true, "paused", "buffering"));
  assert(!media_card_artwork_should_clear(false, true, "unknown", false));
  assert(!media_card_artwork_should_clear(true, true, "playing", false));
  assert(!media_card_artwork_should_clear(true, true, "paused", false));
  assert(!media_card_artwork_should_clear(true, true, "buffering", false));
  assert(media_card_artwork_should_clear(true, true, "idle", false));
  assert(!media_card_artwork_should_clear(true, true, "idle", true));
  assert(media_card_artwork_should_clear(true, true, "off", false));
  assert(media_card_artwork_should_clear(true, false, "playing", true));
  assert(media_entity_content_available(true, true, true));
  assert(!media_entity_content_available(true, true, false));
  assert(!media_entity_content_available(true, false, true));
  assert(!media_entity_content_available(false, true, true));
  assert(!use_secondary_media_entity(false, true, true, true));
  assert(!use_secondary_media_entity(true, false, true, true));
  assert(!use_secondary_media_entity(true, true, false, true));
  assert(!use_secondary_media_entity(true, true, true, false));
  assert(use_secondary_media_entity(true, true, true, true));
  // Only a pause requested from this screensaver owns a retained session.
  PlaybackControl control;
  control.observe("player.a", "paused", 1);
  assert(!control.retains_pause("player.a"));
  assert(control.begin("player.a", "playing", 10) == PlaybackCommand::PAUSE);
  assert(control.begin("player.a", "playing", 11) == PlaybackCommand::NONE);
  control.observe("player.a", "playing", 20); // repeated state before acknowledgement
  control.observe("player.a", "paused", 30);
  assert(control.retains_pause("player.a") && !control.pending());
  assert(control.begin("player.a", "paused", 40) == PlaybackCommand::PLAY);
  control.observe("player.a", "paused", 50);
  assert(control.retains_pause("player.a"));
  control.observe("player.a", "buffering", 60);
  control.observe("player.a", "paused", 70); // next pause was external
  assert(!control.retains_pause("player.a"));
  // Failed/timed-out pause requests must not claim a later external pause.
  control.begin("player.a", "playing", 100);
  control.cancel_pending();
  control.observe("player.a", "paused", 101);
  assert(!control.retains_pause("player.a"));
  control.begin("player.a", "playing", 200);
  control.observe("player.a", "paused", 200 + PlaybackControl::COMMAND_TIMEOUT_MS);
  assert(!control.retains_pause("player.a"));
  control.begin("player.a", "playing", UINT32_MAX - 100);
  control.observe("player.a", "paused", 20); // millis wrap
  assert(control.retains_pause("player.a"));
  control.begin("player.a", "paused", 30);
  control.expire(30 + PlaybackControl::COMMAND_TIMEOUT_MS);
  assert(!control.pending() && control.retains_pause("player.a"));
  // Dismissal, player replacement, and stopped playback release ownership.
  control.reset();
  control.observe("player.a", "paused", 6000);
  assert(!control.retains_pause("player.a"));
  for (const auto &state : {"idle", "off", "unavailable", "unknown"}) {
    control.begin("player.a", "playing", 7000);
    control.observe("player.a", "paused", 7001);
    control.observe("player.a", state, 7002);
    assert(!control.retains_pause("player.a"));
  }
  control.begin("player.a", "playing", 8000);
  control.observe("player.b", "paused", 8001);
  assert(!control.retains_pause("player.a") && !control.retains_pause("player.b"));
  assert(control.begin("player.b", "paused", 8002) == PlaybackCommand::NONE);
  // Connection loss ends the retained session, including a pending resume.
  control.begin("player.a", "playing", 9000);
  control.observe("player.a", "paused", 9001);
  assert(!control.update_connection(true));
  assert(control.retains_pause("player.a"));
  control.begin("player.a", "paused", 9002);
  assert(control.update_connection(false));
  assert(!control.pending() && !control.retains_pause("player.a"));
  assert(!control.update_connection(false));
  assert(!control.update_connection(true));
  control.observe("player.a", "paused", 9003);
  assert(!control.retains_pause("player.a")); // reconnect cannot reclaim the pause
  control.begin("player.a", "playing", 9010);
  assert(!control.update_connection(false)); // pending pause is not a retained screen
  assert(!control.pending());
  control.observe("player.a", "paused", 9011);
  assert(!control.retains_pause("player.a"));
  // A local pause reveals faded details and prevents their timer from hiding them.
  control.reset();
  assert(track_overlay_mode(true, false, true, true, 10) == TrackOverlayMode::TIMED);
  control.begin("player.a", "playing", 10000);
  control.observe("player.a", "paused", 10001);
  for (float seconds : {-1.0f, 0.0f, 10.0f, 30.0f}) {
    assert(track_overlay_mode(false, control.retains_pause("player.a"), true, true, seconds)
           == TrackOverlayMode::PERSISTENT);
  }
  control.begin("player.a", "paused", 10002);
  assert(track_overlay_mode(false, control.retains_pause("player.a"), true, true, 10)
         == TrackOverlayMode::PERSISTENT); // wait for HA before restarting the timer
  control.observe("player.a", "playing", 10003);
  assert(track_overlay_mode(true, control.retains_pause("player.a"), true, true, 10)
         == TrackOverlayMode::TIMED);
  assert(track_overlay_mode(true, false, true, true, 0) == TrackOverlayMode::HIDDEN);
  assert(track_overlay_mode(true, false, true, true, -1) == TrackOverlayMode::PERSISTENT);
  control.observe("player.a", "paused", 10004); // external pause must not retain details
  assert(track_overlay_mode(false, control.retains_pause("player.a"), true, true, 10)
         == TrackOverlayMode::HIDDEN);
  assert(track_overlay_mode(true, false, false, true, 10) == TrackOverlayMode::PERSISTENT);
  assert(track_overlay_mode(true, false, true, false, 10) == TrackOverlayMode::PERSISTENT);
  PolicyInput p; assert(!policy_allows_display(p));
  p.enabled = p.media_playing = p.entity_configured = true; assert(policy_allows_display(p));
  p.external_input_active = p.hide_external_input = true;
  assert(!policy_allows_display(p) && !policy_allows_download(p));
  p.hide_external_input = false; p.schedule_blocks = true;
  assert(!policy_allows_display(p) && policy_allows_download(p));
  p.schedule_blocks = false; p.alarm_takeover_active = true; assert(!policy_allows_display(p));
  assert(feature_allowed(true, true, true, false, false));
  assert(!feature_allowed(true, true, true, true, true));
  assert(display_allowed(true, true, true, true, false, false, false, false));
  assert(!display_allowed(true, true, true, true, false, false, false, true));
  auto ten = cover_art_layout("guition-esp32-p4-jc8012p4a1", "0", 1280, 800, 800, 506);
  assert(ten.split && ten.art_size == 800 && ten.panel_x == 840);
  auto ten_v2 = cover_art_layout("guition-esp32-p4-jc8012p4a1-v2", "90", 800, 1280, 800, 506);
  assert(ten_v2.screen_height == 1280 && ten_v2.panel_y == 834);
  // Five full 103px title lines must leave one artist line and elapsed time
  // above the playback button. Short titles release space for longer artists.
  for (const auto &slug : {"guition-esp32-p4-jc8012p4a1",
                          "guition-esp32-p4-jc8012p4a1-v2",
                          "guition-esp32-p4-jc8012p4a1-v3"}) {
    for (const auto &rotation : {"0", "180"}) {
      const auto layout = cover_art_layout(slug, rotation, 1280, 800, 800, 506);
      const auto button = playback_button_layout(layout, 103);
      assert(layout.title_max_lines == 5 && button.title_max_height == 515);
      const int artist = artist_height_budget(button.panel_height, 515, 47, 4, 35);
      assert(artist == 51);
      assert(515 + artist + 35 <= button.panel_height);
      assert(layout.panel_y + button.panel_height <= 800 - button.margin - button.size);
      assert(artist_height_budget(button.panel_height, 103, 47, 4, 35) > artist);
      assert(artist_height_budget(button.panel_height, 515, 47, 4, 0) >= artist);
    }
    assert(cover_art_layout(slug, "90", 800, 1280, 800, 506).title_max_lines == 0);
  }
  auto seven_v2 = cover_art_layout("guition-esp32-p4-jc1060p470-v2", "0", 1024, 600, 600, 260);
  assert(seven_v2.split && seven_v2.art_size == 600 && seven_v2.panel_x == 615);
  // The 7-inch font has 89px lines with -8px spacing: four lines use 332px.
  for (const auto &slug : {"guition-esp32-p4-jc1060p470", "guition-esp32-p4-jc1060p470-v2"}) {
    for (const auto &rotation : {"0", "180"}) {
      const auto layout = cover_art_layout(slug, rotation, 1024, 600, 600, 260);
      const auto button = playback_button_layout(layout, 89, -8);
      assert(layout.title_max_lines == 4 && button.title_max_height == 332);
      const int artist = artist_height_budget(button.panel_height, 332, 47, 10, 42);
      assert(artist == 57);
      assert(332 + artist + 42 <= button.panel_height);
      assert(layout.panel_y + button.panel_height <= 600 - button.margin - button.size);
      assert(artist_height_budget(button.panel_height, 89, 47, 10, 42) > artist);
    }
    for (const auto &rotation : {"90", "270"}) {
      const auto layout = cover_art_layout(slug, rotation, 600, 1024, 600, 260);
      assert(layout.title_max_lines == 0 && layout.title_max_height == 162);
    }
  }
  auto four = cover_art_layout("guition-esp32-p4-jc4880p443", "90", 800, 480, 480, 220);
  assert(four.screen_width == 800);
  // Three 69px title lines leave room for artist and elapsed time above the
  // playback control in both landscape rotations of the 4.3-inch display.
  for (const auto &rotation : {"90", "270"}) {
    const auto layout = cover_art_layout("guition-esp32-p4-jc4880p443", rotation, 800, 480, 480, 130);
    const auto button = playback_button_layout(layout, 69);
    assert(layout.title_max_lines == 3 && button.title_max_height == 207);
    const int artist = artist_height_budget(button.panel_height, 207, 47, 10, 42);
    assert(artist >= 57);
    assert(207 + artist + 42 <= button.panel_height);
    assert(layout.panel_y + button.panel_height <= 480 - button.margin - button.size);
    assert(artist_height_budget(button.panel_height, 69, 47, 10, 42) > artist);
  }
  for (const auto &rotation : {"0", "180"}) {
    const auto layout = cover_art_layout("guition-esp32-p4-jc4880p443", rotation, 480, 800, 480, 130);
    assert(layout.title_max_lines == 0 && layout.title_max_height == 130);
  }
  auto square = cover_art_layout("esp32-p4-86", "0", 720, 720, 800, 495);
  assert(!square.split && square.art_size == 720 && square.panel_padding == 36);
  for (const auto &slug : {"guition-esp32-s3-4848s040", "esp32-p4-86"}) {
    const bool s3 = std::string(slug) == "guition-esp32-s3-4848s040";
    const int side = s3 ? 480 : 720;
    const int title_line = s3 ? 82 : 123;
    const int artist_line = s3 ? 47 : 70;
    const int artist_padding = s3 ? 6 : 9;
    const int time_height = s3 ? 35 : 53;
    for (const auto &rotation : {"0", "90", "180", "270"}) {
      const auto layout = cover_art_layout(slug, rotation, side, side, side, s3 ? 330 : 495);
      const auto button = playback_button_layout(layout, title_line);
      assert(layout.title_max_lines == 3 && button.title_max_height == 3 * title_line);
      const int content_height = button.panel_height - layout.panel_padding - button.panel_bottom_padding;
      const int artist = artist_height_budget(content_height, button.title_max_height,
                                              artist_line, artist_padding, time_height);
      assert(artist >= artist_line + artist_padding);
      assert(button.title_max_height + artist + time_height <= content_height);
      assert(layout.panel_y + button.panel_height <= side - button.margin - button.size);
      // Turning off the control restores the full panel, still capped at three title lines.
      assert(button.title_max_height + artist_line + artist_padding + time_height <=
             layout.panel_height - 2 * layout.panel_padding);
    }
  }
  for (const auto &layout : {ten, ten_v2, seven_v2, four, square,
       cover_art_layout("guition-esp32-s3-4848s040", "0", 480, 480, 480, 330),
       cover_art_layout("guition-esp32-p4-jc4880p443", "0", 480, 800, 480, 130)}) {
    const auto button = playback_button_layout(layout);
    assert(button.size >= 80 && button.size <= 112);
    assert(button.panel_width > 0 && button.panel_height > 0 && button.title_max_height > 0);
    const int button_x = layout.screen_width - button.margin - button.size;
    const int button_y = layout.screen_height - button.margin - button.size;
    assert(layout.panel_x + button.panel_width <= button_x ||
           layout.panel_y + button.panel_height <= button_y);
  }
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
  // Rapid play/pause policy changes cannot bypass alarm, schedule, or source filtering.
  assert(display_allowed(true, true, true, true, false, false, false, false));
  assert(!display_allowed(true, false, true, true, false, false, false, false));
  assert(!display_allowed(true, true, true, false, false, false, false, false));
  assert(!display_allowed(true, true, true, true, false, false, true, false));
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
  const uint8_t red_le[] = {0x00, 0xF8};
  const uint8_t red_be[] = {0xF8, 0x00};
  auto little_red = extract_accent_color_rgb565(red_le, 1, 1, false, 0, 0, 1, 1);
  auto big_red = extract_accent_color_rgb565(red_be, 1, 1, true, 0, 0, 1, 1);
  assert(little_red.valid && little_red.red == 255 && little_red.green == 0 && little_red.blue == 0);
  assert(big_red.valid && big_red.red == 255 && big_red.green == 0 && big_red.blue == 0);
  const uint8_t red_blue_le[] = {0x00, 0xF8, 0x1F, 0x00};
  auto blue_content = extract_accent_color_rgb565(red_blue_le, 2, 1, false, 1, 0, 1, 1);
  assert(blue_content.valid && blue_content.red == 0 && blue_content.blue == 255);
  auto full_fallback = extract_accent_color_rgb565(red_blue_le, 2, 1, false, 4, 0, 1, 1);
  assert(full_fallback.valid && full_fallback.red == 127 && full_fallback.blue == 127);
  auto dark_red = darken_accent_color(little_red);
  // Keep the icon readable for pale artwork without reducing button brightness.
  assert(playback_icon_color({229,229,229,true}, {234,234,234,true}) == 0x000000);
  assert(playback_icon_color({49,49,49,true}, {88,88,88,true}) == 0xFFFFFF);
  for (int red = 0; red <= 255; red += 17) {
    for (int green = 0; green <= 255; green += 17) {
      for (int blue = 0; blue <= 255; blue += 17) {
        const AccentColor normal{static_cast<uint8_t>(red * 9 / 10),
                                 static_cast<uint8_t>(green * 9 / 10),
                                 static_cast<uint8_t>(blue * 9 / 10), true};
        const AccentColor pressed{static_cast<uint8_t>(normal.red + (255 - normal.red) * 48 / 255),
                                  static_cast<uint8_t>(normal.green + (255 - normal.green) * 48 / 255),
                                  static_cast<uint8_t>(normal.blue + (255 - normal.blue) * 48 / 255), true};
        const bool dark_icon = playback_icon_color(normal, pressed) == 0;
        for (const auto &background : {normal, pressed}) {
          const float luminance = accent_luminance(background);
          const float contrast = dark_icon ? (luminance + 0.05f) / 0.05f : 1.05f / (luminance + 0.05f);
          assert(contrast >= 3.0f);
        }
      }
    }
  }
  assert(dark_red.valid && dark_red.red == 85 && dark_red.green == 0 && dark_red.blue == 0);
  assert(!extract_accent_color_rgb565(nullptr, 1, 1, false, 0, 0, 1, 1).valid);
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
    "shrink_to(0)",
    "new (std::nothrow) P4PipelineJob()",
    "P4_PIPELINE_PENDING_SLOTS",
    "P4_PIPELINE_COMPLETED_SLOTS",
    "can_use_p4_pipeline(this->url_)",
):
    if required not in downloader:
        raise SystemExit(f"Artwork downloader memory contract missing: {required}")
if """if (effective_url == this->url_) {
      if (this->update_pending_) {
        this->update_pending_ = false;
        this->pending_url_.clear();
""" not in downloader:
    raise SystemExit("Artwork downloader must cancel a stale pending URL when the source returns to the active URL")
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

if 'if (err == ESP_ERR_NOT_SUPPORTED) {' not in jpeg_decoder:
    raise SystemExit("P4 JPEG unsupported-format fallback must be handled explicitly")
if 'ESP_LOGD(TAG, "ESP32-P4 JPEG format is not hardware-supported; using software decoder");' not in jpeg_decoder:
    raise SystemExit("P4 JPEG unsupported-format fallback must be debug-level")
if 'ESP_LOGW(TAG, "ESP32-P4 JPEG hardware rejected image (error %d); using software decoder", err);' not in jpeg_decoder:
    raise SystemExit("P4 JPEG unexpected hardware failures must remain warnings")

image_cards = (ROOT / "components" / "espcontrol" / "button_grid_image.h").read_text(encoding="utf-8")
for required in (
    "RefreshBatch media_artwork_refresh",
    "RefreshTrigger media_artwork_trigger",
    "IMAGE_CARD_MEDIA_ARTWORK_TRIGGER_DEBOUNCE_MS = 75",
    "IMAGE_CARD_MEDIA_ARTWORK_RESPONSE_DEBOUNCE_MS = 300",
    "if (ctx->media_artwork_refresh.active())",
    "media_artwork_refresh.begin(",
    "media_artwork_refresh.complete()",
    "Ignoring stale media artwork response",
):
    if required not in image_cards:
        raise SystemExit(f"Media artwork refresh-batch contract missing: {required}")
image_grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
if "image_card_schedule_media_artwork_refresh(art)" not in image_grid:
    raise SystemExit("Media artwork subscriptions must use the trigger scheduler")
screen_cover_art = (ROOT / "common" / "device" / "screen_cover_art.yaml").read_text(encoding="utf-8")
for required in (
    "artwork_refresh.begin(",
    "artwork_refresh.receive(",
    "artwork_refresh.finish();",
    "cover_art_request_paired_artwork",
    "cover_art_schedule_paired_artwork",
    "ARTWORK_TRIGGER_DEBOUNCE_MS",
    "cover_art_artwork_trigger).schedule(force_refresh)",
    "cover_art_artwork_trigger).consume()",
    "Ignoring stale %s artwork response",
):
    if required not in screen_cover_art:
        raise SystemExit(f"Cover-art screensaver refresh-batch contract missing: {required}")
deferred_retry_start = screen_cover_art.index("  - id: cover_art_deferred_request_artwork")
trigger_scheduler_start = screen_cover_art.index("  - id: cover_art_schedule_paired_artwork")
deferred_retry = screen_cover_art[deferred_retry_start:trigger_scheduler_start]
if "script.execute: cover_art_request_paired_artwork" not in deferred_retry:
    raise SystemExit("Cover-art failed reads must retry the paired request immediately")
if "id: cover_art_schedule_paired_artwork" in deferred_retry:
    raise SystemExit("Cover-art failed reads must not use the trigger debounce")
image_decoder = (ROOT / "components" / "artwork_image" / "image_decoder.cpp").read_text(encoding="utf-8")
for required in (
    "if (!this->buffer_ || offset > this->size_)",
    "return nullptr;",
):
    if required not in image_decoder:
        raise SystemExit(f"Artwork download-buffer null-safety contract missing: {required}")
overlay_tint_start = image_cards.find("inline void image_card_apply_media_overlay_tint(")
overlay_tint_end = image_cards.find("\ninline void image_card_apply_downloaded", overlay_tint_start)
if overlay_tint_start < 0 or overlay_tint_end < 0:
    raise SystemExit("Media cover art overlay tint contract missing")
overlay_tint = image_cards[overlay_tint_start:overlay_tint_end]
if "lv_obj_set_style_bg_opa(ctx->media_overlay, LV_OPA_60, LV_PART_MAIN);" not in overlay_tint:
    raise SystemExit("Media cover art overlay tint must remain 60% opaque")

web_styles = (ROOT / "src" / "webserver" / "application" / "styles.ts").read_text(encoding="utf-8")
if ".sp-media-cover-tint{position:absolute;inset:-2px;background:rgba(49,49,49,.6)" not in web_styles:
    raise SystemExit("Web preview media cover tint must use the 60% standard card grey")
for required in (
    ".sp-btn-big .sp-media-cover-details-title{font-size:var(--media-cover-artist)}",
    ".sp-btn-big.sp-media-cover-control-fonts .sp-media-cover-details-title{font-size:calc(var(--btn-label)*1.75)}",
    ".sp-btn-extra-large .sp-media-cover-details-title,.sp-btn-portrait-large .sp-media-cover-details-title{font-size:var(--media-cover-title)}",
    ".sp-btn-extra-large .sp-media-cover-details-row .sp-media-now-artist,.sp-btn-portrait-large .sp-media-cover-details-row .sp-media-now-artist{font-size:var(--media-cover-artist)}",
    ".sp-btn-extra-large .sp-media-cover-details-row .sp-media-now-artist{font-weight:300}",
    ".sp-btn-big.sp-media-cover-details-card,.sp-btn-extra-large.sp-media-cover-details-card,.sp-btn-portrait-large.sp-media-cover-details-card{justify-content:flex-start}",
    ".sp-btn-big .sp-media-cover-details-row{margin-top:calc(var(--btn-pad)*.5)}",
    ".sp-btn-big .sp-media-cover-details-title{-webkit-line-clamp:2}",
    ".sp-btn-wide .sp-media-cover-details-title,.sp-btn-extra-wide .sp-media-cover-details-title{-webkit-line-clamp:2}",
    ".sp-btn-extra-large .sp-media-cover-details-title{-webkit-line-clamp:5}",
):
    if required not in web_styles:
        raise SystemExit(f"Large cover art web font-selection contract missing: {required}")
web_media = (ROOT / "src" / "webserver" / "cards" / "media.ts").read_text(encoding="utf-8")
for required in (
    'deviceId === "guition-esp32-p4-jc4880p443"',
    '" sp-media-cover-control-fonts"',
):
    if required not in web_media:
        raise SystemExit(f"4.3-inch P4 cover art preview font contract missing: {required}")
for required in (
    "ctx->image->request_update_url(ctx->url, max_source_dim)",
    "ctx->modal_image->request_update_url(ctx->modal_url, max_source_dim)",
    "image_card_preempt_active_tile_for_modal();",
    "candidate->image->request_is_active()",
):
    if required not in image_cards:
        raise SystemExit(f"Image card centralized scheduling contract missing: {required}")
for forbidden in (
    "image_card_active_download_context",
    "download_queued",
):
    if forbidden in image_cards:
        raise SystemExit(f"Image cards must not keep a second request scheduler: {forbidden}")
media_clear_start = image_cards.find(
    "inline void image_card_clear_media_artwork(ImageCardCtx *ctx) {"
)
media_clear_end = image_cards.find(
    "\ninline void image_card_layout_modal_loading", media_clear_start
)
if media_clear_start < 0 or media_clear_end < 0:
    raise SystemExit("Media artwork clear contract missing")
media_clear = image_cards[media_clear_start:media_clear_end]
for persistent_binding in (
    "ctx->media_overlay_artwork_tint = false;",
    "ctx->media_artwork_applied = nullptr;",
):
    if persistent_binding in media_clear:
        raise SystemExit(
            "Temporary artwork loss must preserve the configured overlay bindings: "
            + persistent_binding
        )
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
if "cover_art_request_paired_artwork" not in cover_art:
    raise SystemExit("Cover art must retain paired artwork refresh requests")
cover_art_subscription_order = []
for handler, attribute in (
    ("handle_media_content_type", "media_content_type"),
    ("handle_media_content_id", "media_content_id"),
    ("handle_media_title", "media_title"),
    ("handle_media_artist", "media_artist"),
    ("handle_media_album", "media_album_name"),
):
    handler_start = resubscribe.find(handler)
    subscription_start = resubscribe.find(
        f'std::string("{attribute}")', handler_start
    )
    if handler_start < 0 or subscription_start < 0:
        raise SystemExit(f"Cover art metadata policy subscription missing: {attribute}")
    cover_art_subscription_order.append(subscription_start)
if cover_art_subscription_order != sorted(cover_art_subscription_order):
    raise SystemExit(
        "Cover art must subscribe to content kind before title, artist, and album"
    )
for required in (
    "media_metadata_clear_decision(",
    "media_content_identity_fingerprint(",
    "should_replace_media_metadata_identity(next)",
    "id(cover_art_content_fingerprint) = 0;",
    "id(cover_art_content_type).clear();",
    "id(cover_art_content_kind) = static_cast<uint8_t>(",
):
    if required not in resubscribe:
        raise SystemExit(f"Cover art metadata reset contract missing: {required}")
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
artist_subscription = metadata.find('std::string("media_artist")')
if artist_subscription < 0:
    raise SystemExit("Media artist subscription contract missing")
if "state->artist.clear()" in metadata[:artist_subscription]:
    raise SystemExit("Media title updates must preserve an unchanged subscribed artist")
for required in (
    "media_title_refresh_pending(",
    "state->metadata_title_refresh_started_ms = 0;",
):
    if required not in metadata:
        raise SystemExit(f"Media title refresh expiry contract missing: {required}")
content_start = media.find(
    "inline void media_playback_subscribe_content(MediaPlaybackState *state) {"
)
content_end = media.find(
    "inline void media_playback_subscribe_friendly_name", content_start
)
if content_start < 0 or content_end < 0:
    raise SystemExit("Media content subscription contract missing")
content = media[content_start:content_end]
content_type_subscription = content.find('std::string("media_content_type")')
content_id_subscription = content.find('std::string("media_content_id")')
if not 0 <= content_type_subscription < content_id_subscription:
    raise SystemExit("Media content type must be subscribed before the content ID")
for required in (
    "media_metadata_clear_decision(",
    "media_content_identity_fingerprint(",
    "should_replace_media_metadata_identity(",
    "if (decision.clear_title) {",
    "state->metadata_title_awaiting_refresh = true;",
    "state->metadata_title_refresh_started_ms = esphome::millis();",
    "if (decision.item_changed) {",
    "media_playback_clear_stale_artist(state);",
    "media_playback_schedule_metadata_refresh(state);",
):
    if required not in content:
        raise SystemExit(f"Media item-change policy contract missing: {required}")
for subscription_helper in (
    "subscribe_media_control_state",
    "subscribe_media_now_playing_state",
):
    helper_start = media.find(f"inline void {subscription_helper}")
    if helper_start < 0:
        raise SystemExit(f"Media subscription helper missing: {subscription_helper}")
    helper_end = media.find("\n}", helper_start)
    helper = media[helper_start:helper_end]
    content_call = helper.find("media_playback_subscribe_content(state)")
    metadata_call = helper.find("media_playback_subscribe_metadata(state)")
    if not 0 <= content_call < metadata_call:
        raise SystemExit(
            f"{subscription_helper} must subscribe to content before metadata"
        )

media_driver = (
    ROOT / "components" / "espcontrol" / "button_grid_media_driver.h"
).read_text(encoding="utf-8")
route_start = media_driver.find("inline void media_driver_bind_cover_art_route(")
route_end = media_driver.find("\ninline ", route_start + 1)
if route_start < 0 or route_end < 0:
    raise SystemExit("Media cover-art route subscription contract missing")
route = media_driver[route_start:route_end]
for state_name in ("primary", "secondary"):
    content_call = route.find(f"media_playback_subscribe_content({state_name})")
    metadata_call = route.find(f"media_playback_subscribe_metadata({state_name})")
    if not 0 <= content_call < metadata_call:
        raise SystemExit(
            f"Media cover-art {state_name} content must be subscribed before metadata"
        )
for required in (
    "inline bool media_cover_art_uses_screensaver_fonts(int row_span, int col_span)",
    "return row_span >= 2 && col_span >= 2;",
    "inline bool media_cover_art_uses_compact_large_fonts(int row_span, int col_span)",
    "return row_span == 2 && col_span == 2;",
    "inline lv_coord_t media_cover_art_artist_gap(lv_coord_t top_padding,",
    "return top_padding / 2;",
    "ctx->artist_gap = media_cover_art_artist_gap(",
    "inline int media_cover_art_title_line_limit(int row_span, int col_span)",
    "if (row_span == 3 && col_span == 3) return 5;",
    "if (row_span == 1 || (row_span == 2 && col_span == 2)) return 2;",
    "inline void media_position_now_playing_artist(MediaNowPlayingCtx *ctx)",
    "LV_ALIGN_OUT_BOTTOM_LEFT, 0, ctx->artist_gap",
    "ctx->artist_below_title = media_cover_art_uses_screensaver_fonts(",
):
    if required not in media:
        raise SystemExit(f"Large cover art font-selection contract missing: {required}")

cover_details_start = media.find('    if (mode == "cover_art") {')
cover_title_start = media.find(
    "\n      lv_obj_t *title_lbl = lv_label_create(s.btn);", cover_details_start
)
cover_details_end = media.find(
    "\n    lv_obj_t *title_lbl = lv_label_create(s.btn);", cover_title_start + 1
)
if cover_details_start < 0 or cover_details_end < 0:
    raise SystemExit("Cover art track-details layout contract missing")
cover_details = media[cover_details_start:cover_details_end]
for required in (
    "lv_obj_t *artist_lbl = lv_label_create(s.btn);",
    "ctx->artist_lbl = artist_lbl;",
    "lv_obj_add_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);",
    "media_cover_art_title_line_limit(row_span, col_span)",
):
    if required not in cover_details:
        raise SystemExit(f"Cover art track-details layout contract missing: {required}")
if "ctx->artist_lbl = s.text_lbl;" in cover_details:
    raise SystemExit("Cover art artist metadata must not reuse the static card caption label")

for required in (
    "bool highlight_playing = true;",
    "set_card_checked_state(ctx->btn, ctx->available &&",
    "ctx->group_only ? media_control_group_size(ctx) > 1",
    ": ctx->highlight_playing && ctx->playing));",
):
    if required not in media:
        raise SystemExit(f"Media control playing-highlight contract missing: {required}")

media_driver = (ROOT / "components" / "espcontrol" / "button_grid_media_driver.h").read_text(encoding="utf-8")
if "if (control) control->highlight_playing = false;" not in media_driver:
    raise SystemExit("Cover art control modals must not highlight their parent card while playing")
for required in (
    "display.modal.layout_family == DisplayModalLayoutFamily::COMPACT_PORTRAIT",
    "compact_portrait_cover_art\n      ? display_media_control_title_font(display)",
    "compact_portrait_cover_art\n      ? display_media_control_artist_font(display, label_font)",
):
    if required not in media_driver:
        raise SystemExit(f"2x2 cover art must reuse All Controls fonts: {required}")

grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
layout = (ROOT / "components" / "espcontrol" / "button_grid_layout.h").read_text(encoding="utf-8")
subpages = (ROOT / "components" / "espcontrol" / "button_grid_subpages.h").read_text(encoding="utf-8")
subscriptions = (ROOT / "components" / "espcontrol" / "button_grid_subscriptions.h").read_text(encoding="utf-8")
sliders = (ROOT / "components" / "espcontrol" / "button_grid_sliders.h").read_text(encoding="utf-8")
climate = (ROOT / "components" / "espcontrol" / "button_grid_climate.h").read_text(encoding="utf-8")
if "lv_obj_set_width(label, lv_pct(100));" not in layout:
    raise SystemExit("Firmware card labels must retain their full width below top-left icons")
if "lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);" not in grid:
    raise SystemExit("Firmware card setup must place main icons in the top-left corner")
if "lv_obj_align(slot.icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);" not in subpages:
    raise SystemExit("Dynamic subpage cards must place main icons in the top-left corner")
if "lv_obj_align(slot.subpage_lbl, LV_ALIGN_BOTTOM_RIGHT, 0, 2);" not in subpages:
    raise SystemExit("Dynamic subpage cards must retain the bottom-right navigation chevron")
if "lv_obj_align(icon_lbl, LV_ALIGN_TOP_LEFT, padding.left, padding.top);" not in media:
    raise SystemExit("Media cards must refresh main icons in the top-left corner")
if sliders.count("lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, padding.left, padding.top);") < 2:
    raise SystemExit("Slider and light-temperature cards must keep main icons in the top-left corner")
if "lv_obj_align(icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);" not in climate:
    raise SystemExit("Climate cards must keep main icons in the top-left corner")
if "configure_button_label_wrap(text_lbl);" not in subscriptions:
    raise SystemExit("Toggle state updates must restore full-width label wrapping when icons reappear")
for required in (
    "configure_button_label_wrap(entry->back_slot.text_lbl);",
    "configure_button_label_wrap(back_slot.text_lbl);",
):
    if required not in grid:
        raise SystemExit(f"Subpage label clamps must restore full-width wrapping: {required}")
if "refresh_card_layout(sub_slot, sb_cfg, cfg, rs, cs);" not in grid:
    raise SystemExit(
        "Subpage cards must refresh their layout after label clamping so slider label insets are restored"
    )
if "image_card_align_label_stack(label, btn, icon);" not in image_cards:
    raise SystemExit("Image loading-state relayouts must retain the label stack")
if "lv_obj_align(icon, LV_ALIGN_TOP_LEFT, -parent_x, -parent_y);" not in image_cards:
    raise SystemExit("Image card icons must stay in the top-left corner")
if "media_cover_art_title_line_limit(row_span, col_span)" not in grid:
    raise SystemExit("Cover art layout refresh must preserve the per-size track-title limit")
if "ctx->artist_gap = media_cover_art_artist_gap(" not in grid:
    raise SystemExit("Cover art layout refresh must preserve the size-aware artist gap")
if "media_cover_art_artist_gap(\n        ctx->content_padding.top, row_span, col_span)" not in grid:
    raise SystemExit("Cover art layout refresh must use the captured content padding for the artist gap")
for required in (
    "display.modal.layout_family == DisplayModalLayoutFamily::COMPACT_PORTRAIT",
    "compact_portrait\n        ? display_media_control_title_font(display)",
    "compact_portrait\n        ? display_media_control_artist_font(display, label_font)",
):
    if required not in grid:
        raise SystemExit(f"2x2 cover art refresh must reuse All Controls fonts: {required}")
if "if (ha_api_state_connected()) {" not in grid or "refresh_image_cards();" not in grid:
    raise SystemExit("Grid startup must refresh bound cover artwork after Home Assistant state is ready")
for required in (
    "lv_obj_set_height(title_lbl, LV_SIZE_CONTENT);",
    "lv_obj_set_style_max_height(",
    "font->line_height * title_line_limit +",
    "TITLE_LINE_SPACE * (title_line_limit - 1)",
):
    if required not in media:
        raise SystemExit(f"Limited cover art titles must flex to their configured line count: {required}")
media_art_start = grid.find("inline void subscribe_media_cover_art(")
media_art_end = grid.find("\ninline void setup_card_visual(", media_art_start)
if media_art_start < 0 or media_art_end < 0:
    raise SystemExit("Media card cover art subscription contract missing")
media_art = grid[media_art_start:media_art_end]
for required in (
    'std::string("entity_picture")',
    'std::string("entity_picture_local")',
    "image_card_schedule_media_artwork_refresh(art)",
    "media_artwork_content_current(",
    "media_card_artwork_should_clear(",
    "image_card_clear_media_artwork(art)",
):
    if required not in media_art:
        raise SystemExit(f"Media card cover art subscription contract missing: {required}")
if "subscribe_image_card_entity_state" in media_art:
    raise SystemExit(
        "Media card cover art must not add a general entity-state subscription "
        "on top of its picture subscriptions"
    )
now_playing_refresh_start = media.find(
    "inline void media_playback_refresh_stable_artwork("
)
now_playing_refresh_end = media.find(
    "inline void media_playback_apply_state_to_now_playing_snapshot(",
    now_playing_refresh_start,
)
if now_playing_refresh_start < 0 or now_playing_refresh_end < 0:
    raise SystemExit("Media cover-art playback-state contract missing")
now_playing_refresh = media[now_playing_refresh_start:now_playing_refresh_end]
for required in (
    "media_card_artwork_should_clear(",
    "ctx->artwork_refresh_signature.clear();",
    "image_card_clear_media_artwork(ctx->cover_art);",
):
    if required not in now_playing_refresh:
        raise SystemExit(
            f"Stopped media must clear stale card artwork: {required}"
        )
current_content_start = media.find(
    "inline bool media_playback_has_current_content("
)
current_content_end = media.find("\n}", current_content_start)
if current_content_start < 0 or current_content_end < 0:
    raise SystemExit("Secondary media content routing contract missing")
current_content = media[current_content_start:current_content_end]
for required in (
    "media_entity_content_available(",
    "!state->title.empty()",
    "!state->artist.empty()",
    "state->has_current_content_id",
    "state->artwork_content_mask != 0",
):
    if required not in current_content:
        raise SystemExit(
            f"Secondary media content routing contract missing: {required}"
        )
for stale_gate in (
    "media_entity_state_usable(",
    "state->has_current_content_type",
):
    if stale_gate in current_content:
        raise SystemExit(
            f"Secondary routing must follow actual metadata, not {stale_gate}"
        )
state_subscription_start = media.find(
    "inline void media_playback_subscribe_playback_state(MediaPlaybackState *state) {"
)
state_subscription_end = media.find(
    "inline void media_playback_subscribe_metadata(", state_subscription_start
)
if state_subscription_start < 0 or state_subscription_end < 0:
    raise SystemExit("Playback-state retained metadata contract missing")
state_subscription = media[state_subscription_start:state_subscription_end]
for required in (
    "media_state_change_invalidates_retained_content(",
    "media_playback_invalidate_retained_content(state);",
):
    if required not in state_subscription:
        raise SystemExit(
            f"Stopped playback must invalidate retained metadata: {required}"
        )
for required in (
    "espcontrol::cover_art::media_entity_state_usable(next)",
    'subscribe_secondary_content_probe(std::string("media_artist"), 16u)',
):
    if required not in resubscribe:
        raise SystemExit(
            f"Full-screen secondary media routing contract missing: {required}"
        )

# A visible screensaver must reach LVGL so the control consumes its own press.
from check_firmware_ha_bindings import yaml_script_body
screen = (ROOT / "common/device/screen_cover_art.yaml").read_text()
touch = yaml_script_body(screen, "cover_art_handle_touch") or ""
for required in (
    "!id(espcontrol_app).display().target_mode_is(espcontrol::DisplayMode::COVER_ART)",
    "lv_obj_has_flag(id(cover_art_screensaver), LV_OBJ_FLAG_HIDDEN)",
    "script.execute: cover_art_pause_after_touch",
    "script.wait: cover_art_pause_after_touch",
    "script.execute: screensaver_wake",
):
    assert required in touch, f"Missing cover-art touch routing: {required}"
control = yaml_script_body(screen, "cover_art_toggle_playback") or ""
assert "screensaver_wake" not in control.replace("screensaver_wake_touch_guard_active", "")
assert "cover_art_active_media_player_entity" in control
assert "LV_OBJ_FLAG_EVENT_BUBBLE" in screen

connection = yaml_script_body(screen, "cover_art_update_playback_control") or ""
assert "update_connection(ha_api_state_connected())" in connection
assert "script.execute: cover_art_return_home_after_playback" in connection

progress = yaml_script_body(screen, "cover_art_refresh_progress") or ""
assert "const bool time_was_hidden = lv_obj_has_flag(id(cover_art_time_label), LV_OBJ_FLAG_HIDDEN);" in progress
assert "if (time_was_hidden != lv_obj_has_flag(id(cover_art_time_label), LV_OBJ_FLAG_HIDDEN))" in progress
assert "id(cover_art_fit_artist_text).execute();" in progress
# Both early exits (external input and missing duration) and normal playback
# must refit after visibility changes, while steady per-second updates skip it.
assert progress.count("refit_artist_if_needed();") == 3
assert progress.count("refit_artist_if_needed();\n            return;") == 2
assert progress.rfind("refit_artist_if_needed();") > progress.index("lv_label_set_text(id(cover_art_time_label), label);")

overlay = yaml_script_body(screen, "cover_art_show_track_overlay") or ""
assert "mode: restart" in overlay
assert "track_overlay_mode(" in overlay
assert "TrackOverlayMode::TIMED" in overlay and "TrackOverlayMode::HIDDEN" in overlay
assert "retains_pause(id(cover_art_active_media_player_entity))" in overlay
assert "id(cover_art_track_overlay_duration).state * 1000" in overlay
delayed_hide = overlay[overlay.index("- delay:"):]
assert "id(cover_art_media_playing) &&" in delayed_hide
playback = screen[screen.index("std::function<void(esphome::StringRef)> handle_playback_state ="):
                  screen.index("std::function<void(esphome::StringRef)> handle_media_title =")]
assert "if (!was_playing) id(cover_art_show_track_overlay).execute();" in playback
assert '''if (id(cover_art_playback_control).retains_pause(cover_entity)) {
                  id(cover_art_show_track_overlay).execute();''' in playback
fallback = yaml_script_body(screen, "cover_art_show_black_screen") or ""
assert "retains_pause(id(cover_art_active_media_player_entity))" in fallback
assert "lv_obj_set_style_text_color(id(cover_art_playback_icon), lv_color_hex(0xFFFFFF), LV_PART_MAIN);" in fallback
accent_script = yaml_script_body(screen, "cover_art_extract_accent_color") or ""
assert "espcontrol::cover_art::playback_icon_color(" in accent_script
assert "rgb(button_color), rgb(pressed_color)" in accent_script
assert "lv_obj_set_style_text_color(id(cover_art_playback_icon), lv_color_hex(icon_color), LV_PART_MAIN);" in accent_script
setting = yaml_script_body(screen, "cover_art_refresh_playback_setting") or ""
assert "${cover_art_square_overlay} && !id(cover_art_playback_control_enabled).state" in setting
assert "script.execute: cover_art_return_home_after_playback" in setting
assert "id(cover_art_playback_control).reset()" in setting
assert "id(cover_art_layout_signature).clear()" in setting
assert "script.execute: cover_art_apply_responsive_layout" in setting
assert "!id(cover_art_playback_control_enabled).state) return;" in control
layout_script = yaml_script_body(screen, "cover_art_apply_responsive_layout") or ""
compensation_call = "apply_width_compensation(id(cover_art_playback_button), icon_width_compensation_percent());"
assert compensation_call in layout_script
assert layout_script.index(compensation_call) < layout_script.index("if (signature ==")
assert "!${cover_art_square_overlay} || id(cover_art_playback_control_enabled).state" in layout_script
assert "lv_obj_add_flag(id(cover_art_playback_button), LV_OBJ_FLAG_HIDDEN)" in layout_script
print("Cover art policy, layout, and state contract checks passed.")
