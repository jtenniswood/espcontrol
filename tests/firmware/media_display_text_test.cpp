#include <cassert>
#include <string>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <new>

#include "button_grid_string.h"
#include "media_display_text.h"

namespace {
bool forbid_allocation = false;
}
void* operator new(size_t size) {
  assert(!forbid_allocation);
  if (void* memory = std::malloc(size ? size : 1)) return memory;
  std::abort();
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

std::string checked_display_text(const std::string& text, size_t cap = std::string::npos) {
  const auto expected = espcontrol::media::normalize_media_display_text(text, cap);
  std::array<char, 1026> buffer{};
  const auto capacity = std::min<size_t>(cap, buffer.size() - 2) + 1;
  buffer[capacity] = '!';
  forbid_allocation = true;
  const auto actual = espcontrol::media::normalize_media_display_text_bounded(buffer.data(), capacity, text);
  forbid_allocation = false;
  assert(actual == expected && buffer[actual.size()] == '\0' && buffer[capacity] == '!');
  // The same fixed buffer may hold bounded HTML-decoded input first.
  assert(text.size() < buffer.size());
  std::memcpy(buffer.data(), text.data(), text.size());
  forbid_allocation = true;
  const auto in_place = espcontrol::media::normalize_media_display_text_bounded(
      buffer.data(), capacity, {buffer.data(), text.size()});
  forbid_allocation = false;
  assert(in_place == expected);
  return expected;
}

int main() {
  const std::string title = u8"RAINING IN ＯＳＡＫＡ 🌧️";
  assert(checked_display_text(title) == "RAINING IN OSAKA");
  assert(title == u8"RAINING IN ＯＳＡＫＡ 🌧️");  // Input/identity is untouched.
  assert(checked_display_text(u8"Ａｚ０９！＠＃＊～　") == "Az09!@#*~");
  assert(checked_display_text("").empty());
  assert(checked_display_text("Plain ASCII 123 # *") == "Plain ASCII 123 # *");
  assert(checked_display_text(u8"Beyoncé — Αθήνα Москва 日本語") ==
         u8"Beyoncé — Αθήνα Москва 日本語");
  assert(normalize_display_text(checked_display_text(u8"Ṣade ṣ 🌧️")) == "Sade s");
  assert(checked_display_text(u8"© ® ™ 1 2 # *") == u8"© ® ™ 1 2 # *");
  assert(checked_display_text(u8"©️ ®️ ™️").empty());
  assert(checked_display_text(u8"🌧 ❤ ☀").empty());
  assert(checked_display_text(u8"1️⃣ 2⃣ #️⃣ *️⃣").empty());
  assert(checked_display_text(u8"👩🏽‍💻 👨‍👩‍👧‍👦 🇯🇵 🇺🇸 👍🏿 🏳️‍🌈").empty());
  assert(checked_display_text(u8"🏴\U000E0067\U000E0062\U000E0065\U000E006E\U000E0067\U000E007F").empty());
  assert(checked_display_text(u8"A 🌧️ B") == "A B");
  assert(checked_display_text(u8"🌧️  A\t B  🌧️") == "A B");
  assert(checked_display_text(u8"A\u00A0B　C") == "A B C");
  assert(checked_display_text(u8"A🎵B") == "AB");
  assert(checked_display_text(u8"©︎") == u8"©");
  assert(checked_display_text(u8"A\u200DB") == u8"A\u200DB");
  assert(checked_display_text(u8"\uFE0F\uFE0E\u20E3").empty());
  assert(checked_display_text(u8"👩‍") == "");
  assert(checked_display_text(decode_html_entities(
    "RAINING IN &#xFF2F;&#xFF33;&#xFF21;&#xFF2B;&#xFF21; &#x1F327;&#xFE0F;")) ==
    "RAINING IN OSAKA");
  assert(checked_display_text(decode_html_entities("Beyonc&eacute; &amp; friends")) ==
    u8"Beyoncé & friends");

  // A byte cap must not leave half a UTF-8 character in the artist buffer.
  assert(checked_display_text(u8"ABéC", 3) == "AB");
  assert(checked_display_text(u8"ABéC", 4) == u8"ABé");
  assert(checked_display_text(u8"A é", 3) == "A");
  assert(checked_display_text(title, 0).empty());
  assert(checked_display_text(std::string(1000, 'A'), 255).size() == 255);
  for (const auto &value : {std::string(u8"é"), std::string(u8"界"), std::string(u8"𐐀")}) {
    for (size_t cap = 0; cap < value.size(); ++cap) {
      assert(checked_display_text(value, cap).empty());
      assert(checked_display_text(value.substr(0, cap)).empty());
    }
    assert(checked_display_text(value, value.size()) == value);
  }
  assert(checked_display_text(std::string("A\xFF") + "B") == "AB");
  assert(checked_display_text("\xC0\xAF\xED\xA0\x80\xF4\x90\x80\x80").empty());
  assert(checked_display_text(std::string("A\0B", 3)) == "AB");
  // UI icon codepoints are never classified as emoji.
  assert(checked_display_text(u8"\U000F075A") == u8"\U000F075A");
  char guard = '!';
  assert(espcontrol::media::normalize_media_display_text_bounded(nullptr, 10, "text").empty());
  assert(espcontrol::media::normalize_media_display_text_bounded(&guard, 0, "text").empty() && guard == '!');
  const std::array<std::string, 11> fragments = {"ab", u8"é", u8"界", u8"𐐀", u8" Ａ", u8" 👩🏽‍💻 ",
      u8"1️⃣", "\t", "\xFF", std::string("x\0y", 3), u8" ©︎ "};
  uint32_t random = 1729;
  for (size_t sample = 0; sample < 10000; ++sample) {
    std::string input;
    for (size_t part = 0; part < sample % 25; ++part) {
      random = random * 1664525u + 1013904223u;
      input += fragments[random % fragments.size()];
    }
    checked_display_text(input, sample % 257);
  }

}
