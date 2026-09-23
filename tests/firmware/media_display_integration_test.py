#!/usr/bin/env python3
"""Compile production card/modal and Cover Art text renderers on the host."""
from pathlib import Path
import subprocess
import sys
import tempfile
import textwrap

from generate_media_lifecycle_integration import definition


root = Path(__file__).resolve().parents[2]
media = (root / "components/espcontrol/button_grid_media.h").read_text(encoding="utf-8")
cover = (root / "common/device/screen_cover_art.yaml").read_text(encoding="utf-8")
cover = cover.split("  - id: cover_art_sync_track_text\n", 1)[1]


def label_lambda(label: str) -> str:
    lines = cover.split(f"id: {label}\n", 1)[1].splitlines()
    assert lines[0].strip() == "text: !lambda |-"
    indentation = len(lines[1]) - len(lines[1].lstrip())
    body = []
    for line in lines[1:]:
        if line.strip() and len(line) - len(line.lstrip()) < indentation:
            break
        body.append(line)
    return textwrap.dedent("\n".join(body))


source = r'''
#include <cassert>
#include <string>
#include "button_grid_string.h"
#include "media_display_text.h"
#include "media_metadata_policy.h"
#include "cover_art.h"
constexpr size_t HA_STATE_TEXT_MAX_LEN = 255;
struct MediaControlCtx {
  std::string title, artist, friendly_name, label, state_text = "playing";
  bool cover_art_mode = false;
};
std::string media_status_text(const std::string &state) { return state; }
std::string espcontrol_i18n(const std::string &text) { return text; }
const char *espcontrol_i18n(const char *text) { return text; }
bool cover_art_external_input_active = false;
std::string cover_art_media_source, cover_art_title, cover_art_artist;
#define id(x) x
struct Label { std::string text; bool hidden = false; };
constexpr int LV_OBJ_FLAG_HIDDEN = 1;
struct MediaNowPlayingCtx {
  bool source_known = false, external_source = false, cover_art_mode = false;
  bool external_source_fallback = false, show_track_details = true, play_pause_background = false;
  Label *title_lbl = nullptr, *artist_lbl = nullptr, *btn = nullptr;
  void *cover_art = nullptr;
  char artist[10] = {};
};
struct MediaPlaybackState {
  bool source_known = false, external_source = false, has_state = true;
  bool available = true, playing = true;
  std::string state_text = "playing", title, artist, source;
};
bool media_playback_has_current_content(MediaPlaybackState *s) { return !s->title.empty(); }
void media_cover_art_set_idle_placeholder(MediaNowPlayingCtx*, bool) {}
void image_card_clear_media_artwork(void*) {}
void image_card_set_media_artwork_suppressed(void*, bool) {}
void lv_label_set_display_text(Label *label, const char *text) { label->text = normalize_display_text(text); }
void lv_obj_clear_flag(Label *label, int) { label->hidden = false; }
void lv_obj_add_flag(Label *label, int) { label->hidden = true; }
void media_position_now_playing_artist(MediaNowPlayingCtx*) {}
void set_card_checked_state(Label*, bool) {}
'''
for name in ("media_metadata_text", "media_control_title_text", "media_control_artist_text",
             "media_apply_now_playing_artist_text", "media_set_now_playing_artist",
             "media_playback_apply_state_to_now_playing_snapshot"):
    line, body = definition(media, name)
    source += f'\n#line {line} "button_grid_media.h"\n{body}\n'
source += "\nstd::string cover_title() {\n" + label_lambda("cover_art_title_label") + "\n}\n"
source += "\nstd::string cover_artist() {\n" + label_lambda("cover_art_artist_label") + "\n}\n"
source += r'''
int main() {
  const std::string title = u8"RAINING IN ＯＳＡＫＡ 🌧️";
  const std::string artist = u8"Ｂeyoncé 👩🏽‍💻";
  assert(media_metadata_text(esphome::StringRef(title), "--") == "RAINING IN OSAKA");
  assert(media_metadata_text(esphome::StringRef("&#xFF21; &amp; B &#x1F327;&#xFE0F;"), "--") == "A & B");
  assert(media_metadata_text(esphome::StringRef(u8"🌧️"), "--") == "--");
  assert(media_metadata_text(esphome::StringRef("unavailable"), "--") == "--");
  MediaControlCtx ctx;
  ctx.title = cover_art_title = title;
  ctx.artist = cover_art_artist = artist;
  assert(media_control_title_text(&ctx) == "RAINING IN OSAKA");
  assert(media_control_artist_text(&ctx) == u8"Beyoncé");
  assert(cover_title() == "RAINING IN OSAKA");
  assert(cover_artist() == u8"Beyoncé");
  assert(ctx.title == title && cover_art_title == title);
  assert(ctx.artist == artist && cover_art_artist == artist);
  ctx.title = cover_art_title = u8"🌧️";
  assert(media_control_title_text(&ctx) == "playing");
  assert(cover_title() == u8"—");
  ctx.artist = cover_art_artist = u8"👩🏽‍💻";
  assert(media_control_artist_text(&ctx).empty());
  assert(cover_artist().empty());
  ctx.title = cover_art_title = "Next track";
  assert(media_control_title_text(&ctx) == "Next track");
  assert(cover_title() == "Next track");
  MediaPlaybackState state;
  state.title = title;
  state.artist = artist;
  Label title_label, artist_label;
  MediaNowPlayingCtx card;
  card.title_lbl = &title_label;
  card.artist_lbl = &artist_label;
  media_playback_apply_state_to_now_playing_snapshot(&state, &card);
  assert(title_label.text == "RAINING IN OSAKA");
  assert(artist_label.text == u8"Beyoncé" && !artist_label.hidden);
  assert(state.title == title && state.artist == artist);
  state.artist = u8"AAAAAAAAé";
  media_playback_apply_state_to_now_playing_snapshot(&state, &card);
  assert(artist_label.text == "AAAAAAAA");
  media_set_now_playing_artist(&card, esphome::StringRef(state.artist));
  assert(std::string(card.artist) == "AAAAAAAA");
  state.title = state.artist = u8"🌧️";
  media_playback_apply_state_to_now_playing_snapshot(&state, &card);
  assert(title_label.text == "--" && artist_label.hidden);
}
'''
with tempfile.TemporaryDirectory(prefix="media-display-test-") as tmp:
    path = Path(tmp) / "test.cpp"
    binary = Path(tmp) / "test"
    path.write_text(source, encoding="utf-8")
    subprocess.run([
        sys.argv[1] if len(sys.argv) > 1 else "g++",
        "-std=c++17", "-Wall", "-Wextra", "-Werror",
        "-I", str(root / "components/espcontrol"),
        "-I", str(root / "tests/firmware/stubs"), str(path), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
print("Media display integration tests passed.")
