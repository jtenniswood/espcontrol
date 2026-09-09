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
  using esphome::web_server_idf::digest_session_expired;
  using esphome::web_server_idf::find_http_cookie;

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

  const char cookie_header[] = "theme=dark; ESPControlAuth=session-token; locale=en";
  const char *cookie_value = nullptr;
  size_t cookie_value_length = 0;
  if (!expect(find_http_cookie(cookie_header, sizeof(cookie_header) - 1, "ESPControlAuth", &cookie_value,
                               &cookie_value_length),
              "The Digest browser session cookie must be found among unrelated cookies"))
    return 1;
  if (!expect(cookie_value_length == 13 && std::memcmp(cookie_value, "session-token", 13) == 0,
              "The complete Digest browser session token must be returned"))
    return 1;
  if (!expect(!find_http_cookie(cookie_header, sizeof(cookie_header) - 1, "ESPControl", &cookie_value,
                                &cookie_value_length),
              "A cookie name prefix must not authenticate"))
    return 1;
  if (!expect(!digest_session_expired(100, 400, 300), "A session remains valid at its idle boundary"))
    return 1;
  if (!expect(digest_session_expired(100, 401, 300), "A session expires beyond its idle boundary"))
    return 1;
  if (!expect(!digest_session_expired(0xfffffff0U, 0x10U, 0x20U),
              "Session expiry must tolerate the millisecond timer wrapping"))
    return 1;

  return 0;
}
