#include <cassert>
#include <deque>
#include <vector>
#include "home_assistant_endpoint_resolver.h"
#include "../../components/artwork_image/transfer_observer.h"
#include "esphome/core/hal.h"
using namespace espcontrol;
using namespace espcontrol::home_assistant_endpoint;

struct FakeProbe : EndpointProbe {
  std::vector<std::string> calls;
  std::vector<bool> local_destination_required;
  std::deque<EndpointProbeResult> results;
  uint32_t generation{0};
  bool available{true};
  bool start(const std::string &origin, uint32_t gen, bool require_local_destination) override {
    if (!available) return false;
    calls.push_back(origin); local_destination_required.push_back(require_local_destination);
    generation = gen; return true;
  }
  bool take(EndpointProbeResult &result) override {
    if (results.empty()) return false;
    result = results.front(); results.pop_front(); return true;
  }
  void shutdown() override {}
  void reply(EndpointProbeOutcome outcome, int status = 0) { results.push_back({generation, outcome, status, 0}); }
};
ServiceRecord server{{"10.20.30.40"}, 8123, "https://10.20.30.40:9443", false, "one"};

int main() {
  FakeProbe probe;
  HomeAssistantEndpointResolver resolver(&probe);
  int changes = 0;
  resolver.set_change_callback([&] { ++changes; });
  resolver.setup();
  resolver.configure("Automatic", "http", 8123, "10.20.30.40");
  resolver.accept_discovery({server}); resolver.loop();
  assert(probe.calls.back() == "https://10.20.30.40:9443");
  assert(!probe.local_destination_required.back());
  probe.reply(EndpointProbeOutcome::TRANSPORT); resolver.loop();
  assert(probe.calls.back() == "http://10.20.30.40:8123");
  assert(!probe.local_destination_required.back());
  probe.reply(EndpointProbeOutcome::READY, 200); resolver.loop();
  assert(resolver.origin() == "http://10.20.30.40:8123");
  assert(resolver.health() == "Connection checked" && changes == 1);
  const auto gen = resolver.generation();
  resolver.report_download(resolver.origin(), gen, 200, true, false);
  assert(resolver.health() == "Images received" && changes == 1);
  resolver.accept_discovery({}); resolver.loop();
  assert(probe.calls.size() == 2 && changes == 1);
  resolver.report_download(resolver.origin(), gen - 1, 0, false, true);
  resolver.report_download("https://cdn.example:443", gen, 0, false, true);
  resolver.report_download(resolver.origin(), gen, 200, false, false); // decode error
  assert(resolver.health() == "Images received");
  resolver.report_download(resolver.origin(), gen, 0, false, true);
  resolver.loop(); assert(probe.calls.size() == 2);
  resolver.report_download(resolver.origin(), gen, 0, false, true);
  resolver.loop(); assert(probe.calls.size() == 3);
  resolver.report_download(resolver.origin(), gen, 0, false, true);
  resolver.loop(); assert(probe.calls.size() == 3); // coalesce
  const uint32_t stale = probe.generation;
  resolver.configure("Manual", "https", 8443, "10.20.30.40", "ha.example.test");
  probe.results.push_back({stale, EndpointProbeOutcome::READY, 200, 0}); resolver.loop();
  assert(resolver.origin() == "https://ha.example.test:8443");
  assert(resolver.health() == "Manual connection");
  resolver.report_download(resolver.origin(), resolver.generation(), 0, false, true);
  resolver.report_download(resolver.origin(), resolver.generation(), 0, false, true);
  resolver.loop(); assert(probe.calls.size() == 3);
  resolver.configure("Automatic", "http", 8123, "10.20.30.41");
  assert(resolver.origin().empty());
  resolver.accept_discovery({server}); resolver.loop(); // wrong instance => fallback only
  assert(probe.calls.back() == "http://10.20.30.41:8123");
  probe.reply(EndpointProbeOutcome::RESPONSE, 404); resolver.loop();
  assert(resolver.source() == Source::FALLBACK && resolver.health() == "Connection failed");
  resolver.report_download(resolver.origin(), resolver.generation(), 200, true, false);
  assert(resolver.health() == "Images received"); // restricted manifest doesn't block images

  FakeProbe denied;
  HomeAssistantEndpointResolver access(&denied); access.setup();
  access.configure("Automatic", "http", 8123, "10.20.30.40");
  access.accept_discovery({server}); access.loop();
  denied.reply(EndpointProbeOutcome::ACCESS_DENIED, 403); access.loop();
  assert(access.health() == "Access denied" && denied.calls.size() == 1);
  access.report_download(access.origin(), access.generation(), 403, false, false);
  access.loop(); assert(denied.calls.size() == 1);

  // Backoff survives a successful manifest when actual images still fail.
  FakeProbe retry;
  HomeAssistantEndpointResolver backoff(&retry); backoff.setup();
  backoff.configure("Automatic", "http", 8123, "10.20.30.40");
  backoff.accept_discovery({server}); backoff.loop();
  retry.reply(EndpointProbeOutcome::READY, 200); backoff.loop();
  for (int i = 0; i < 2; ++i)
    backoff.report_download(backoff.origin(), backoff.generation(), 0, false, true);
  backoff.loop(); assert(retry.calls.size() == 2);
  retry.reply(EndpointProbeOutcome::READY, 200); backoff.loop();
  for (int i = 0; i < 2; ++i)
    backoff.report_download(backoff.origin(), backoff.generation(), 0, false, true);
  backoff.loop(); assert(retry.calls.size() == 2);
  esphome::endpoint_test_now += 5000;
  backoff.loop(); assert(retry.calls.size() == 3);

  FakeProbe unavailable; unavailable.available = false;
  HomeAssistantEndpointResolver resource(&unavailable); resource.setup();
  resource.configure("Automatic", "http", 8123, "10.20.30.40");
  resource.accept_discovery({server}); resource.loop();
  assert(resource.health() == "Connection failed");
  assert(resource.origin() == "http://10.20.30.40:8123");
  unavailable.available = true; resource.loop(); assert(unavailable.calls.empty());
  esphome::endpoint_test_now += 5000; resource.loop(); assert(unavailable.calls.size() == 1);
  resource.configure("Manual", "http", 8123, "10.20.30.40");
  unavailable.reply(EndpointProbeOutcome::READY, 200); resource.loop();
  resource.configure("Automatic", "http", 8123, "10.20.30.40");
  resource.accept_discovery({server}); resource.loop();
  unavailable.reply(EndpointProbeOutcome::READY, 200); resource.loop();
  assert(resource.origin() == "https://10.20.30.40:9443");

  // A second installation on the API host invalidates an otherwise healthy route.
  auto other = server; other.identity = "two";
  resource.accept_discovery({server, other}); resource.loop();
  assert(unavailable.calls.back() == "http://10.20.30.40:8123");
  unavailable.reply(EndpointProbeOutcome::READY, 200); resource.loop();
  assert(resource.source() == Source::FALLBACK);

  FakeProbe replaced_probe;
  HomeAssistantEndpointResolver replaced_resolver(&replaced_probe); replaced_resolver.setup();
  replaced_resolver.configure("Automatic", "http", 8123, "10.20.30.40");
  replaced_resolver.accept_discovery({server}); replaced_resolver.loop();
  const auto old_selection = replaced_probe.generation;
  auto updated = server;
  updated.addresses = {"10.20.30.40", "10.20.30.41"};
  updated.internal_url = "https://10.20.30.40:9444";
  replaced_resolver.accept_discovery({updated});
  replaced_probe.results.push_back({old_selection, EndpointProbeOutcome::READY, 200, 0});
  replaced_resolver.loop();
  assert(replaced_resolver.origin().empty());
  assert(replaced_probe.calls.back() == "https://10.20.30.40:9444");
  const auto old_connection = replaced_resolver.generation();
  replaced_probe.reply(EndpointProbeOutcome::READY, 200);
  replaced_resolver.invalidate_connection();
  replaced_resolver.loop();
  assert(replaced_resolver.origin().empty());
  assert(replaced_resolver.generation() != old_connection);

  using namespace esphome::artwork_image;
  auto &observer = TransferObserver::instance();
  int notices = 0;
  observer.set([](const std::string &origin) { return origin == "http://ha:8123" ? 5 : 0; },
      [&](const TransferNotice &n) { ++notices; assert(n.request.origin == "http://ha:8123"); });
  for (const auto *path : {"/api/camera_proxy/camera.test?token=secret", "/api/image_proxy/image.test?token=secret",
                            "/api/media_player_proxy/media_player.test?token=secret"}) {
    auto stamp = observer.begin(std::string("http://ha:8123") + path, 9);
    assert(stamp.endpoint_generation == 5);
    observer.complete(stamp, 9, 200, TransferFailure::NONE);
    observer.complete(stamp, 9, 200, TransferFailure::NONE); // once only
  }
  assert(notices == 3);
  auto external = observer.begin("https://cdn.example/image.jpg", 9);
  observer.complete(external, 9, 0, TransferFailure::TRANSPORT);
  auto cancelled = observer.begin("http://ha:8123/api/camera_proxy/camera.test", 9);
  observer.complete(cancelled, 10, 0, TransferFailure::TRANSPORT);
  auto replaced = observer.begin("http://ha:8123/api/image_proxy/image.test", 9);
  observer.complete(replaced, 9, 0, TransferFailure::TRANSPORT, true);
  assert(notices == 3);
  observer.set({}, {});
}
