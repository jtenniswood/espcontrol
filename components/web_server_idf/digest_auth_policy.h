#pragma once

#include <cstdint>

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

}  // namespace esphome::web_server_idf
