#include <iostream>

#include "digest_auth_policy.h"

namespace {

bool expect(bool condition, const char *message) {
  if (condition) return true;
  std::cerr << message << '\n';
  return false;
}

}  // namespace

int main() {
  using esphome::web_server_idf::DigestNonceCountWindow;
  using esphome::web_server_idf::DigestRequestPolicy;
  using esphome::web_server_idf::accept_digest_nonce_count;
  using esphome::web_server_idf::digest_request_policy;

  if (!expect(digest_request_policy(true, false) == DigestRequestPolicy::CHECK_NONCE,
              "A first valid authentication must consume a nonce count"))
    return 1;
  if (!expect(digest_request_policy(true, true) == DigestRequestPolicy::REUSE_REQUEST_NONCE,
              "A second valid check on one request must reuse its accepted nonce"))
    return 1;
  if (!expect(digest_request_policy(false, true) == DigestRequestPolicy::REJECT,
              "Invalid credentials must fail even after request-local authentication"))
    return 1;

  DigestNonceCountWindow browser_window{};
  if (!expect(accept_digest_nonce_count(&browser_window, 1), "A new nonce count must be accepted"))
    return 1;
  if (!expect(!accept_digest_nonce_count(&browser_window, 1),
              "The same nonce count on another request must be rejected as replay"))
    return 1;
  if (!expect(accept_digest_nonce_count(&browser_window, 3), "A later nonce count must be accepted"))
    return 1;
  if (!expect(accept_digest_nonce_count(&browser_window, 2),
              "A parallel request may arrive once within the replay window"))
    return 1;
  if (!expect(!accept_digest_nonce_count(&browser_window, 2),
              "An out-of-order nonce count must not be accepted twice"))
    return 1;

  return 0;
}
