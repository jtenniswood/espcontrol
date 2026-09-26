#include <cassert>
#include <string>

#include "button_grid_string.h"
#include "media_display_text.h"

int main() {
  using espcontrol::media::normalize_media_display_text;
  const std::string title = u8"RAINING IN ＯＳＡＫＡ 🌧️";
  assert(normalize_media_display_text(title) == "RAINING IN OSAKA");
  assert(title == u8"RAINING IN ＯＳＡＫＡ 🌧️");  // Input/identity is untouched.
  assert(normalize_media_display_text(u8"Ａｚ０９！＠＃＊～　") == "Az09!@#*~");
  assert(normalize_media_display_text("").empty());
  assert(normalize_media_display_text("Plain ASCII 123 # *") == "Plain ASCII 123 # *");
  assert(normalize_media_display_text(u8"Beyoncé — Αθήνα Москва 日本語") ==
         u8"Beyoncé — Αθήνα Москва 日本語");
  assert(normalize_display_text(normalize_media_display_text(u8"Ṣade ṣ 🌧️")) == "Sade s");
  assert(normalize_media_display_text(u8"© ® ™ 1 2 # *") == u8"© ® ™ 1 2 # *");
  assert(normalize_media_display_text(u8"©️ ®️ ™️").empty());
  assert(normalize_media_display_text(u8"🌧 ❤ ☀").empty());
  assert(normalize_media_display_text(u8"1️⃣ 2⃣ #️⃣ *️⃣").empty());
  assert(normalize_media_display_text(u8"👩🏽‍💻 👨‍👩‍👧‍👦 🇯🇵 🇺🇸 👍🏿 🏳️‍🌈").empty());
  assert(normalize_media_display_text(u8"🏴\U000E0067\U000E0062\U000E0065\U000E006E\U000E0067\U000E007F").empty());
  assert(normalize_media_display_text(u8"A 🌧️ B") == "A B");
  assert(normalize_media_display_text(u8"🌧️  A\t B  🌧️") == "A B");
  assert(normalize_media_display_text(u8"A\u00A0B　C") == "A B C");
  assert(normalize_media_display_text(u8"A🎵B") == "AB");
  assert(normalize_media_display_text(u8"©︎") == u8"©");
  assert(normalize_media_display_text(u8"A\u200DB") == u8"A\u200DB");
  assert(normalize_media_display_text(u8"\uFE0F\uFE0E\u20E3").empty());
  assert(normalize_media_display_text(u8"👩‍") == "");
  assert(normalize_media_display_text(decode_html_entities(
    "RAINING IN &#xFF2F;&#xFF33;&#xFF21;&#xFF2B;&#xFF21; &#x1F327;&#xFE0F;")) ==
    "RAINING IN OSAKA");
  assert(normalize_media_display_text(decode_html_entities("Beyonc&eacute; &amp; friends")) ==
    u8"Beyoncé & friends");

  // A byte cap must not leave half a UTF-8 character in the artist buffer.
  assert(normalize_media_display_text(u8"ABéC", 3) == "AB");
  assert(normalize_media_display_text(u8"ABéC", 4) == u8"ABé");
  assert(normalize_media_display_text(u8"A é", 3) == "A");
  assert(normalize_media_display_text(title, 0).empty());
  assert(normalize_media_display_text(std::string(1000, 'A'), 255).size() == 255);
  for (const auto &value : {std::string(u8"é"), std::string(u8"界"), std::string(u8"𐐀")}) {
    for (size_t cap = 0; cap < value.size(); ++cap) {
      assert(normalize_media_display_text(value, cap).empty());
      assert(normalize_media_display_text(value.substr(0, cap)).empty());
    }
    assert(normalize_media_display_text(value, value.size()) == value);
  }
  assert(normalize_media_display_text(std::string("A\xFF") + "B") == "AB");
  assert(normalize_media_display_text("\xC0\xAF\xED\xA0\x80\xF4\x90\x80\x80").empty());
  assert(normalize_media_display_text(std::string("A\0B", 3)) == "AB");
  // UI icon codepoints are never classified as emoji.
  assert(normalize_media_display_text(u8"\U000F075A") == u8"\U000F075A");
}
