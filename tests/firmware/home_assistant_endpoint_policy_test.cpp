#include <cassert>
#include "home_assistant_endpoint_policy.h"
using namespace espcontrol::home_assistant_endpoint;

int main() {
  assert(normalize_address(" 192.168.1.10:60532 ") == "192.168.1.10");
  assert(normalize_address("::ffff:192.168.1.10") == "192.168.1.10");
  assert(normalize_address("::ffff:c0a8:10a") == "192.168.1.10");
  assert(normalize_address("0:0:0:0:0:ffff:c0a8:010a") == "192.168.1.10");
  assert(normalize_address("[FE80::1234%wlan0]") == "fe80:0:0:0:0:0:0:1234");
  assert(parse_origin("https://ha.example/") == "https://ha.example:443");
  assert(parse_origin("HTTP://HA.LOCAL:8124") == "http://ha.local:8124");
  assert(parse_origin("https://[fd12::1]:8443/") == "https://[fd12:0:0:0:0:0:0:1]:8443");
  for (const auto *bad : {"https://user:pass@ha", "ftp://ha", "http://ha/path", "http://ha?token=x",
       "http://ha#x", "http://ha:0", "http://ha:65536", "http://ha:", "http://ha:abc",
       "https:///", "http://[xyz]:80", "http://[1:2:3:4:5:6:7:8:]:80", "http://ha\\other", "http://ha host"})
    assert(parse_origin(bad).empty());
  assert(local_address("10.3.2.1") && local_address("172.31.255.1") && local_address("192.168.4.8"));
  assert(local_address("fd12::1") && local_address("fe80::1"));
  assert(!local_address("198.51.100.8") && !local_address("172.32.0.1") && !local_address("999.1.1.1"));

  ServiceRecord local{{"10.20.30.40"}, 8123, "http://10.20.30.40:8123", false, "ha-one"};
  auto found = discover({local}, "10.20.30.40", "http", 8123);
  assert(found.candidates.size() == 2);
  assert(found.candidates[0].origin == "http://10.20.30.40:8123");
  // External HTTPS is not an input. Automatic advertised origins must bind
  // directly to the connected API peer IP.
  local.internal_url = "https://10.20.30.40:9443/";
  found = discover({local}, "10.20.30.40", "http", 80);
  assert(found.candidates.size() == 4);
  assert(found.candidates[0].origin == "https://10.20.30.40:9443");
  assert(!found.candidates[0].require_local_destination);
  assert(found.candidates[1].origin == "http://10.20.30.40:8123");
  assert(!found.candidates[1].require_local_destination);
  assert(found.candidates[2].origin == "https://10.20.30.40:8123");
  assert(found.candidates[3].origin == "http://10.20.30.40:80");
  assert(found.candidates[3].source == Source::FALLBACK);
  local.internal_url = "https://10.20.30.41:9443/";
  found = discover({local}, "10.20.30.40", "http", 80);
  assert(found.invalid_internal_url);
  assert(found.candidates[0].origin == "http://10.20.30.40:8123");
  local.internal_url = "https://split.example:9443/";
  found = discover({local}, "10.20.30.40", "http", 80);
  assert(found.invalid_internal_url);
  assert(found.candidates[0].origin == "http://10.20.30.40:8123");
  local.internal_url = "https://10.20.30.40:9443/";
  assert(discover({local, local}, "10.20.30.40", "http", 80).candidates.size() == 4);
  auto partial = local; partial.internal_url.clear();
  assert(discover({partial, local}, "10.20.30.40", "http", 80).candidates.size() == 4);
  assert(discover({local, partial}, "10.20.30.40", "http", 80).candidates.size() == 4);
  auto second = local; second.identity = "ha-two";
  found = discover({local, second}, "10.20.30.40", "http", 80);
  assert(found.ambiguous && found.candidates.size() == 1 && found.candidates[0].source == Source::FALLBACK);
  second = local; second.addresses = {"10.20.30.41"};
  assert(!discover({local, second}, "10.20.30.40", "http", 80).ambiguous);
  local.internal_url = "http://ha/bad";
  found = discover({local}, "10.20.30.40", "https", 8443);
  assert(found.invalid_internal_url && found.candidates[0].origin == "https://10.20.30.40:8123");
  local.landing_page = true;
  assert(discover({local}, "10.20.30.40", "http", 8123).candidates.size() == 1);
  assert(discover({}, "10.20.30.40", "http", 8123).candidates[0].source == Source::FALLBACK);
  assert(discover({}, "", "http", 8123).candidates.empty());
  assert(infer_legacy_mode("http", 8123) == Mode::AUTOMATIC);
  assert(infer_legacy_mode("https", 8123) == Mode::MANUAL);
}
