#pragma once

#include <cctype>
#include <string_view>

namespace espcontrol::pages {
namespace group_card_detail {
inline bool space(char value) {
  return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}
inline std::string_view trim(std::string_view value) {
  while (!value.empty() && space(value.front())) value.remove_prefix(1);
  while (!value.empty() && space(value.back())) value.remove_suffix(1);
  return value;
}
// A borrowed normalized entity, including the existing quote/backslash policy.
// Ordinary HA IDs use a direct view. Unusual quoted forms use a scalar cursor;
// neither path builds a string or bounds a valid ID by temporary-buffer size.
struct Entity {
  std::string_view raw;
  size_t begin{0}, end{0};
  bool decoded{false};
  struct Cursor {
    const Entity& entity;
    size_t offset{0}, output{0};
    char quote{0};
    bool escaped{false};
    bool next(char& result) {
      if (!entity.decoded) {
        if (offset >= entity.raw.size()) return false;
        result = entity.raw[offset++];
        return true;
      }
      while (offset < entity.raw.size() && output < entity.end) {
        const char value = entity.raw[offset++];
        if (escaped)
          escaped = false;
        else if (quote && value == '\\') {
          escaped = true;
          continue;
        } else if (value == '\'' || value == '"') {
          if (!quote) {
            quote = value;
            continue;
          }
          if (quote == value) {
            quote = 0;
            continue;
          }
        }
        const auto index = output++;
        if (index < entity.begin) continue;
        result = value;
        return true;
      }
      return false;
    }
  };
  static Entity literal(std::string_view value) {
    value = trim(value);
    if (value.size() >= 2 &&
        ((value.front() == '\'' && value.back() == '\'') || (value.front() == '"' && value.back() == '"'))) {
      value.remove_prefix(1);
      value.remove_suffix(1);
    }
    return {value, 0, value.size(), false};
  }
  static Entity token(std::string_view value) {
    auto direct = literal(value);
    direct.raw = trim(direct.raw);
    direct.end = direct.raw.size();
    if (direct.raw.find_first_of("'\"\\") == std::string_view::npos) return direct;
    // Locate trimmed decoded bounds, then the optional one literal quote pair
    // removed by media_group_trim after the legacy lexer has removed quotes.
    Entity all{value, 0, value.size(), true};
    Cursor cursor{all};
    size_t index = 0, first = 0, last = 0;
    char first_value = 0, last_value = 0, current;
    bool found = false;
    while (cursor.next(current)) {
      if (!space(current)) {
        if (!found) {
          first = index;
          first_value = current;
          found = true;
        }
        last = index + 1;
        last_value = current;
      }
      ++index;
    }
    if (!found) return {};
    if (last - first >= 2 && (first_value == '\'' || first_value == '"') && first_value == last_value) {
      ++first;
      --last;
    }
    return {value, first, last, true};
  }
  bool valid() const {
    constexpr std::string_view prefix = "media_player.";
    Cursor cursor{*this};
    size_t index = 0;
    char value;
    while (cursor.next(value)) {
      if (index < prefix.size()) {
        if (value != prefix[index]) return false;
      } else if (!std::isalnum(static_cast<unsigned char>(value)) && value != '_')
        return false;
      ++index;
    }
    return index > prefix.size();
  }
  bool same(const Entity& other) const {
    if (!decoded && !other.decoded) return raw == other.raw;
    if (end - begin != other.end - other.begin) return false;
    Cursor left{*this}, right{other};
    char a, b;
    while (left.next(a))
      if (!right.next(b) || a != b) return false;
    return !right.next(b);
  }
};
}  // namespace group_card_detail
// Checked state needs only the second distinct valid player. Keep legacy list
// parsing/primary trimming semantics without materializing all group members.
inline bool media_group_has_multiple(std::string_view primary, std::string_view members) {
  using group_card_detail::Entity;
  Entity first = Entity::literal(primary);
  bool found = first.valid();
  const auto other = [&](std::string_view raw) {
    const auto value = Entity::token(raw);
    if (!value.valid()) return false;
    if (!found) {
      first = value;
      found = true;
      return false;
    }
    return !first.same(value);
  };
  size_t start = 0;
  char quote = 0;
  bool escaped = false;
  for (size_t i = 0; i < members.size(); ++i) {
    const char value = members[i];
    if (escaped) {
      escaped = false;
      continue;
    }
    if (quote && value == '\\') {
      escaped = true;
      continue;
    }
    if (value == '\'' || value == '"') {
      if (!quote)
        quote = value;
      else if (quote == value)
        quote = 0;
      continue;
    }
    if (!quote && (value == ',' || value == '[' || value == ']')) {
      if (other(members.substr(start, i - start))) return true;
      start = i + 1;
    }
  }
  return other(members.substr(start));
}
}  // namespace espcontrol::pages
