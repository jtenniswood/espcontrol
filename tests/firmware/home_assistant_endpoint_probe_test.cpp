#include <cassert>
#include "home_assistant_endpoint_probe.h"
#include "esp_http_client.h"
#include "freertos/idf_additions.h"
#include "esphome/core/hal.h"
using namespace espcontrol;
int main() {
 EndpointProbeService probe;
 EndpointProbeResult result;
 assert(probe.start("https://ha.example:8443", 1, false));
 assert(!probe.take(result));
 assert(!probe.start("http://10.1.2.3:80", 2, false));
 probe_test_run_task();
 assert(probe_test_url == "https://ha.example:8443/manifest.json");
 assert(probe_test_headers.size() == 2 && probe_test_headers["Accept-Encoding"] == "identity");
 assert(probe_test_config.disable_auto_redirect && probe_test_config.timeout_ms == 3000);
 assert(probe_test_config.crt_bundle_attach && !probe_test_config.skip_cert_common_name_check);
 assert(probe.take(result) && result.generation == 1 && result.outcome == EndpointProbeOutcome::READY);
 assert(!probe.take(result) && probe_test_cleanups == 1);

 assert(probe.start("https://10.1.2.3:8123", 2, false));
 probe_test_status = 403; probe_test_run_task();
 assert(probe_test_config.skip_cert_common_name_check && !probe_test_config.crt_bundle_attach);
 assert(probe.take(result) && result.outcome == EndpointProbeOutcome::ACCESS_DENIED);
 assert(probe.start("https://[fd12:0:0:0:0:0:0:1]:8443", 3, false));
 probe_test_status = 302; probe_test_run_task();
 assert(probe_test_config.crt_bundle_attach); // do not broaden existing local TLS exemption to ULA
 assert(probe.take(result) && result.outcome == EndpointProbeOutcome::RESPONSE);

 assert(probe.start("http://ha.local:80", 4, false));
 probe_test_status = 200; probe_test_content_type = "text/html"; probe_test_run_task();
 assert(probe.take(result) && result.outcome == EndpointProbeOutcome::RESPONSE);
 assert(probe.start("http://ha.local:80", 5, false));
 probe_test_content_type = "Application/JSON; charset=utf-8"; probe_test_run_task();
 assert(probe.take(result) && result.outcome == EndpointProbeOutcome::READY);

 assert(probe.start("http://ha.local:80", 6, false));
 esphome::endpoint_test_now += 3001;
 assert(probe.take(result) && result.generation == 6 && result.outcome == EndpointProbeOutcome::TRANSPORT);
 assert(!probe.start("http://ha.local:80", 7, false)); // deadline cannot accumulate unfinished jobs
 probe_test_run_task();
 assert(!probe.take(result)); // late success is not published
 assert(probe.start("http://ha.local:80", 7, false));
 probe.shutdown(); probe_test_run_task(); // worker owns data after service detaches
 assert(!probe.take(result));

 probe_test_allocation_fails = true;
 assert(!probe.start("http://ha.local:80", 8, false));
 probe_test_allocation_fails = false;
 probe_test_client_allocation_fails = true;
 assert(probe.start("http://ha.local:80", 8, false)); probe_test_run_task();
 assert(probe.take(result) && result.outcome == EndpointProbeOutcome::RESOURCE);
 probe_test_client_allocation_fails = false;
 probe_test_error = ESP_ERR_NO_MEM;
 assert(probe.start("https://ha.example:443", 9, false)); probe_test_run_task();
 assert(probe.take(result) && result.outcome == EndpointProbeOutcome::RESOURCE);
 probe_test_error = ESP_FAIL;
 assert(probe.start("https://ha.example:443", 10, false)); probe_test_run_task();
 assert(probe.take(result) && result.outcome == EndpointProbeOutcome::TRANSPORT);

 probe_test_error = ESP_OK;
 probe_test_status = 200;
 probe_test_content_type = "application/json";
 assert(probe.start("http://10.1.2.3:8123", 11, true));
 probe_test_run_task();
 assert(probe.take(result) && result.outcome == EndpointProbeOutcome::READY);
 const int calls_before_public_advertisement = probe_test_client_allocations;
 assert(probe.start("http://198.51.100.8:8123", 12, true));
 probe_test_run_task();
 assert(probe.take(result) && result.outcome == EndpointProbeOutcome::RESPONSE && result.error == -3);
 assert(probe_test_client_allocations == calls_before_public_advertisement);
}
