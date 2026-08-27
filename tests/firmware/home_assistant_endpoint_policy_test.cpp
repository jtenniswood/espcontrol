#include <cassert>

#include "home_assistant_endpoint_policy.h"

using espcontrol::home_assistant_endpoint::Mode;
using espcontrol::home_assistant_endpoint::ServiceRecord;
using espcontrol::home_assistant_endpoint::build_origin;
using espcontrol::home_assistant_endpoint::infer_legacy_mode;
using espcontrol::home_assistant_endpoint::normalize_address;
using espcontrol::home_assistant_endpoint::protocol_from_internal_url;
using espcontrol::home_assistant_endpoint::record_matches_client;
using espcontrol::home_assistant_endpoint::select_discovered_origin;

int main() {
  assert(normalize_address(" 192.168.1.10:60532 ") == "192.168.1.10");
  assert(normalize_address("::ffff:192.168.1.10") == "192.168.1.10");
  assert(normalize_address("[FE80::1234%wlan0]") ==
         "fe80:0:0:0:0:0:0:1234");
  assert(normalize_address("fe80:0000:0000:0000:0000:0000:0000:1234") ==
         "fe80:0:0:0:0:0:0:1234");
  assert(build_origin("http", "192.168.1.10", 80) ==
         "http://192.168.1.10:80");
  assert(build_origin("https", "fe80::1234", 443) ==
         "https://[fe80:0:0:0:0:0:0:1234]:443");
  assert(protocol_from_internal_url("https://home.example:9443", "http") ==
         "https");
  assert(protocol_from_internal_url("HTTP://home.example", "https") ==
         "http");
  assert(protocol_from_internal_url("ftp://home.example", "https") ==
         "https");

  ServiceRecord matching{{"192.168.1.10", "fe80::1234"}, 80,
                         "http://homeassistant.local", false};
  ServiceRecord other{{"192.168.1.20"}, 8123,
                      "http://other.local:8123", false};
  ServiceRecord landing{{"192.168.1.10"}, 8123, "", true};
  assert(record_matches_client(matching, "192.168.1.10:60532"));
  assert(!record_matches_client(other, "192.168.1.10"));
  assert(!record_matches_client(landing, "192.168.1.10"));
  assert(select_discovered_origin({other, matching}, "192.168.1.10", "http") ==
         "http://192.168.1.10:80");
  assert(select_discovered_origin({landing, other}, "192.168.1.10", "http").empty());

  ServiceRecord same_host_other_instance{{"192.168.1.10"}, 8123,
                                         "http://other.local:8123", false};
  assert(select_discovered_origin({matching, same_host_other_instance},
                                  "192.168.1.10", "http").empty());

  ServiceRecord missing_url{{"fe80::1234"}, 8123, "", false};
  assert(select_discovered_origin({missing_url}, "[fe80::1234]", "https") ==
         "https://[fe80:0:0:0:0:0:0:1234]:8123");
  assert(infer_legacy_mode("http", 8123) == Mode::AUTOMATIC);
  assert(infer_legacy_mode("http", 80) == Mode::MANUAL);
  assert(infer_legacy_mode("https", 8123) == Mode::MANUAL);
  return 0;
}
