#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace espcontrol::backup {
inline constexpr const char* BACKUP_CARD_FIELDS[] = {
    "entity", "label", "icon", "icon_on", "sensor", "unit", "type", "precision", "options"};

template<typename Sink>
bool write_backup_json_string(std::string_view value, Sink& sink) {
  if (!sink("\"")) return false;
  constexpr char HEX[] = "0123456789abcdef";
  std::string chunk;
  chunk.reserve(1024);
  for (const unsigned char byte : value) {
    const size_t count = byte < 0x20 ? 6 : (byte == '"' || byte == '\\') ? 2 : 1;
    if (chunk.size() + count > 1024) {
      if (!sink(chunk)) return false;
      chunk.clear();
    }
    if (byte < 0x20) {
      chunk += "\\u00";
      chunk += HEX[byte >> 4]; chunk += HEX[byte & 15];
    } else {
      if (count == 2) chunk += '\\';
      chunk += static_cast<char>(byte);
    }
  }
  return (chunk.empty() || sink(chunk)) && sink("\"");
}

// Emit the original document without retaining an expanded base64 copy.
template<typename Sink>
bool write_backup_base64(const uint8_t* document, size_t size, Sink& sink) {
  if (!document && size) return false;
  constexpr char BASE64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string encoded;
  encoded.reserve(1024);
  for (size_t i = 0; i < size; i += 3) {
    const uint32_t value = (uint32_t(document[i]) << 16) |
        (i + 1 < size ? uint32_t(document[i + 1]) << 8 : 0) |
        (i + 2 < size ? document[i + 2] : 0);
    encoded += BASE64[(value >> 18) & 63];
    encoded += BASE64[(value >> 12) & 63];
    encoded += i + 1 < size ? BASE64[(value >> 6) & 63] : '=';
    encoded += i + 2 < size ? BASE64[value & 63] : '=';
    if (encoded.size() == 1024) {
      if (!sink(encoded)) return false;
      encoded.clear();
    }
  }
  return encoded.empty() || sink(encoded);
}
}  // namespace espcontrol::backup
