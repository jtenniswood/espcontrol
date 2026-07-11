#!/usr/bin/env python3
"""Compile and exercise the pure cover-art policy, layout, and state helpers."""
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = r'''
#include <cassert>
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
  s.begin_download("track-b"); assert(s.phase == Phase::REPLACING);
  assert(s.apply_download("track-b") && s.current_image_loaded() && !s.needs_download());
  s.select_source("broken");
  for (int i = 0; i < MAX_DOWNLOAD_RETRIES; ++i) s.record_failure();
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
  assert(progress_percent(0, 0) == 0 && progress_percent(30, 120) == 25 && progress_percent(150, 120) == 100);
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
    "Artwork download exceeded device image budget",
    "shrink_to(this->download_buffer_initial_size_)",
):
    if required not in downloader:
        raise SystemExit(f"Artwork downloader memory contract missing: {required}")
print("Cover art policy, layout, and state contract checks passed.")
