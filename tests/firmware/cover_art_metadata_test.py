#!/usr/bin/env python3
"""Compile actual Cover Art callbacks with P4/S3 doubles and delayed HA replies."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
text = (root / "common/device/screen_cover_art.yaml").read_text()


def callback(name):
    match = re.search(rf"handle_media_{name}\s*=\s*(.*?\n            \}});", text, re.S)
    assert match, name
    return f"auto handle_media_{name} = {match[1]};\n"


invalidator = re.search(
    r"invalidate_stale_media_duration\s*=\s*(.*?\n          \});", text, re.S
)
assert invalidator
script = text.split("  - id: cover_art_refresh_media_metadata\n", 1)[1].split("\n  - id:", 1)[0]
schedule = script.split("- lambda: |-\n", 1)[1]
expiry_script = text.split("  - id: cover_art_expire_title_refresh\n", 1)[1].split("\n  - id:", 1)[0]
assert "- delay: 2s" in expiry_script
assert "id(cover_art_title_awaiting_refresh) = false;" in expiry_script
content_id_callback = callback("content_id")
assert "id(cover_art_expire_title_refresh).execute();" in content_id_callback
source = r'''
#include <cassert>
#include <functional>
#include <string>
#include <vector>
#include "media_metadata_policy.h"
namespace esphome { using StringRef = std::string; }
namespace espcontrol { enum class DisplayMode { COVER_ART }; }
struct Switch { bool state = true; } cover_art_screensaver_enabled;
struct Script { template<typename... Args> void execute(Args...) {} };
Script cover_art_refresh_media_metadata, cover_art_sync_track_text,
       cover_art_expire_title_refresh, cover_art_show_track_overlay,
       cover_art_request_artwork;
struct Display { bool target_mode_is(espcontrol::DisplayMode) { return false; } };
struct App { Display display() { return {}; } } espcontrol_app;
struct Runtime { std::string loaded_url = "image"; bool image_available = true; } cover_art_runtime;
int cover_art_subscription_generation = 1, cover_art_last_progress_percent = 50;
std::string cover_art_active_media_player_entity = "media_player.room";
std::string cover_art_title = "Old title", cover_art_artist = "Artist", cover_art_album = "Album";
std::string cover_art_content_type = "music";
uint8_t cover_art_content_kind = 0;
uint64_t cover_art_content_fingerprint = 1;
bool cover_art_title_awaiting_refresh = false;
uint32_t cover_art_last_duration_callback_ms = 100, test_now = 100;
float cover_art_media_duration = 240;
int cover_art_progress_bar = 0, cover_art_time_label = 0, progress_invalidations = 0;
constexpr int LV_ANIM_OFF = 0, HA_STATE_TEXT_MAX_LEN = 255, HA_SUBSCRIPTION_SCOPE_COVER_ART = 2;
uint32_t millis() { return test_now; }
void lv_bar_set_value(int, int, int) {}
void lv_label_set_text(int, const char*) {}
void media_playback_invalidate_stale_progress(const std::string&) { ++progress_invalidations; }
std::string string_ref_limited(const std::string &s, size_t n) { return s.substr(0, n); }
std::string decode_html_entities(const std::string &s) { return s; }
std::vector<std::string> requested;
void ha_schedule_metadata_refresh(const std::string&, std::initializer_list<const char*> attrs, uint32_t) {
  for (auto attr : attrs) requested.emplace_back(attr);
}
#define id(x) x
#define ESP_LOGI(...) do {} while(false)
int main() {
 const std::string cover_entity = cover_art_active_media_player_entity;
 const int subscription_generation = cover_art_subscription_generation;
 auto mark_artwork_refresh_needed = []() {};
 auto invalidate_stale_media_duration = ''' + invalidator[1] + ";\n"
source += content_id_callback
source += "\n".join(callback(name) for name in ("title", "artist", "album"))
source += r'''
 auto schedule_metadata = []() {
''' + schedule + r'''
 };
 schedule_metadata();
#ifdef ESPCONTROL_LOW_HEAP_COVER_ART
 assert((requested == std::vector<std::string>{"media_title", "media_artist"}));
#else
 assert((requested == std::vector<std::string>{"media_title", "media_artist", "media_album_name"}));
#endif
 // A title transition clears the artist, followed by fresh progress from HA.
 handle_media_title("New title");
 assert(cover_art_artist.empty());
 cover_art_media_duration = 240;
 cover_art_last_duration_callback_ms = 150;
 test_now = 1150;  // send rejection/recovery takes longer than 250 ms
 int invalidations = progress_invalidations;
 handle_media_artist("Artist");
 handle_media_album("New album");
 assert(cover_art_media_duration == 240 && progress_invalidations == invalidations);
 assert(cover_art_artist == "Artist" && cover_art_album == "New album");
 // A content transition invalidates once, then a delayed title restores text.
 handle_media_content_id("track:two");
 assert(cover_art_title.empty() && cover_art_title_awaiting_refresh);
 assert(cover_art_media_duration == 0);
 cover_art_media_duration = 300;
 cover_art_last_duration_callback_ms = 1200;
 test_now = 2200;
 invalidations = progress_invalidations;
 handle_media_artist("Same artist");  // Attributes can precede the title reply.
 handle_media_title("Second title");
 assert(cover_art_artist == "Same artist");
 assert(cover_art_media_duration == 300 && progress_invalidations == invalidations);
 assert(!cover_art_title_awaiting_refresh);
 // A later genuine title-only transition must still invalidate old progress.
 handle_media_title("Third title");
 assert(cover_art_media_duration == 0);
}
'''
with tempfile.TemporaryDirectory(prefix="cover-art-metadata-") as temp:
    cpp = Path(temp) / "metadata.cpp"
    cpp.write_text(source)
    for profile in ("p4", "s3"):
        binary = Path(temp) / profile
        command = [sys.argv[1], "-std=c++17", "-Wall", "-Wextra", "-Werror",
                   "-I", str(root / "components/espcontrol"), str(cpp), "-o", str(binary)]
        if profile == "s3":
            command.append("-DESPCONTROL_LOW_HEAP_COVER_ART=1")
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)
        print(f"{profile}: metadata selection and delayed progress preservation passed")
