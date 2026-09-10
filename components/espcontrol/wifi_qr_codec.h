#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

// The codec is deliberately independent of LVGL so payload rules can be host-tested.
inline std::vector<std::string> wifi_qr_tabs(const std::string &value) {
  const std::string configured = value.empty() ? "qr|credentials" : value;
  std::vector<std::string> tabs;
  size_t start = 0;
  while (start <= configured.size()) {
    const size_t end = configured.find('|', start);
    const std::string tab = configured.substr(start,
      end == std::string::npos ? std::string::npos : end - start);
    if ((tab == "qr" || tab == "credentials") &&
        std::find(tabs.begin(), tabs.end(), tab) == tabs.end()) {
      tabs.push_back(tab);
    }
    if (end == std::string::npos) break;
    start = end + 1;
  }
  if (tabs.empty()) tabs.push_back("qr");
  return tabs;
}

inline bool wifi_qr_valid_utf8_bytes(const std::string &value) {
  int remaining = 0;
  for (unsigned char byte : value) {
    if (remaining == 0) {
      if ((byte & 0x80) == 0) continue;
      if ((byte & 0xE0) == 0xC0) remaining = 1;
      else if ((byte & 0xF0) == 0xE0) remaining = 2;
      else if ((byte & 0xF8) == 0xF0) remaining = 3;
      else return false;
    } else {
      if ((byte & 0xC0) != 0x80) return false;
      --remaining;
    }
  }
  return remaining == 0;
}
inline int wifi_qr_base64_value(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '-' || c == '+') return 62;
  if (c == '_' || c == '/') return 63;
  return -1;
}

inline bool wifi_qr_decode_base64url(const std::string &encoded, std::string *decoded) {
  if (!decoded || encoded.empty() || encoded.size() > 96) return false;
  decoded->clear(); int bits = 0; uint32_t value = 0;
  for (char ch : encoded) {
    int digit = wifi_qr_base64_value(ch); if (digit < 0) return false;
    value = (value << 6) | static_cast<uint32_t>(digit); bits += 6;
    if (bits >= 8) { bits -= 8; decoded->push_back(static_cast<char>((value >> bits) & 0xFF)); }
  }
  return wifi_qr_valid_utf8_bytes(*decoded);
}

inline bool wifi_qr_hex_key(const std::string &password) {
  if (password.size() != 64) return false;
  for (unsigned char ch : password) if (!std::isxdigit(ch)) return false;
  return true;
}
inline bool wifi_qr_password_valid(const std::string &password) {
  return (password.size() >= 8 && password.size() <= 63) || wifi_qr_hex_key(password);
}
inline std::string wifi_qr_escape(const std::string &value) {
  std::string out; out.reserve(value.size() * 2);
  for (char ch : value) { if (ch == '\\' || ch == ';' || ch == ',' || ch == '\"' || ch == ':') out.push_back('\\'); out.push_back(ch); }
  return out;
}
inline std::string wifi_qr_compact_payload(const std::string &payload) {
  static constexpr const char *visible_suffix = "H:false;;";
  static constexpr size_t visible_suffix_size = 9;
  if (payload.size() >= visible_suffix_size &&
      payload.compare(payload.size() - visible_suffix_size,
                      visible_suffix_size, visible_suffix) == 0) {
    return payload.substr(0, payload.size() - visible_suffix_size) + ";";
  }
  return payload;
}
inline bool wifi_qr_build_payload(const std::string &ssid64, const std::string &security_value,
                                  const std::string &pass64, bool hidden, std::string *payload,
                                  std::string *ssid, std::string *decoded_password = nullptr) {
  if (!payload || !ssid) return false;
  const bool open = security_value == "open";
  std::string network_name;
  if (!wifi_qr_decode_base64url(ssid64, &network_name) || network_name.empty() || network_name.size() > 32) return false;
  std::string password;
  if (!open && (!wifi_qr_decode_base64url(pass64, &password) || !wifi_qr_password_valid(password))) return false;
  *ssid = network_name;
  if (decoded_password) *decoded_password = password;
  *payload = "WIFI:T:" + std::string(open ? "nopass" : "WPA") + ";S:" + wifi_qr_escape(network_name) + ";";
  if (!open) *payload += "P:" + wifi_qr_escape(password) + ";";
  *payload += "H:" + std::string(hidden ? "true" : "false") + ";;";
  return payload->size() <= 255;
}
