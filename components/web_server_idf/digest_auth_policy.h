#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace esphome::web_server_idf {

enum class DigestRequestPolicy : uint8_t { REJECT, CHECK_NONCE, REUSE_REQUEST_NONCE };

inline DigestRequestPolicy digest_request_policy(bool response_valid, bool nonce_accepted_for_request) {
  if (!response_valid)
    return DigestRequestPolicy::REJECT;
  return nonce_accepted_for_request ? DigestRequestPolicy::REUSE_REQUEST_NONCE : DigestRequestPolicy::CHECK_NONCE;
}

struct DigestNonceCountWindow {
  uint32_t last_nonce_count{};
  uint64_t used_nonce_counts{};
};

inline bool accept_digest_nonce_count(DigestNonceCountWindow *state, uint32_t nonce_count) {
  if (nonce_count > state->last_nonce_count) {
    const uint32_t advance = nonce_count - state->last_nonce_count;
    state->used_nonce_counts = advance >= 64 ? 1 : (state->used_nonce_counts << advance) | 1;
    state->last_nonce_count = nonce_count;
    return true;
  }

  const uint32_t distance = state->last_nonce_count - nonce_count;
  if (distance >= 64)
    return false;
  const uint64_t count_mask = uint64_t{1} << distance;
  if ((state->used_nonce_counts & count_mask) != 0)
    return false;
  state->used_nonce_counts |= count_mask;
  return true;
}

inline bool digest_session_expired(uint32_t last_used_at, uint32_t now, uint32_t idle_lifetime) {
  return static_cast<uint32_t>(now - last_used_at) > idle_lifetime;
}

inline bool find_http_cookie(const char *header, size_t header_length, const char *name,
                             const char **value, size_t *value_length) {
  const size_t name_length = std::strlen(name);
  size_t i = 0;
  while (i < header_length) {
    while (i < header_length && (header[i] == ' ' || header[i] == ';')) i++;
    const size_t name_start = i;
    while (i < header_length && header[i] != '=' && header[i] != ';') i++;
    if (i >= header_length) break;
    if (header[i] == ';') continue;
    size_t parsed_name_length = i - name_start;
    while (parsed_name_length > 0 && header[name_start + parsed_name_length - 1] == ' ') parsed_name_length--;
    i++;
    const size_t value_start = i;
    while (i < header_length && header[i] != ';') i++;
    size_t parsed_value_length = i - value_start;
    while (parsed_value_length > 0 && header[value_start + parsed_value_length - 1] == ' ') parsed_value_length--;
    if (parsed_name_length == name_length && std::memcmp(header + name_start, name, name_length) == 0) {
      *value = header + value_start;
      *value_length = parsed_value_length;
      return true;
    }
  }
  return false;
}

}  // namespace esphome::web_server_idf
