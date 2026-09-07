"""Wiring regressions for the recovery work seen in the 7-inch debug log."""

from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile
import textwrap
import unittest

ROOT = Path(__file__).resolve().parents[2]


class ReconnectRecoveryTest(unittest.TestCase):
    def setUp(self):
        self.core = (ROOT / "common/device/core_infra.yaml").read_text()
        self.image = (ROOT / "components/espcontrol/button_grid_image.h").read_text()

    def test_recovery_has_one_restartable_owner_and_yields_first(self):
        script = self.core.split("  - id: ha_refresh_after_connect\n", 1)[1]
        self.assertIn("    mode: restart\n", script)
        self.assertRegex(script, r"(?s)    then:\n(?:\s*#[^\n]*\n)*      - delay: 1s\n")
        self.assertIn("delay: 20s", script)
        self.assertIn("delay: 25s", script)

    def test_only_ha_clients_start_recovery(self):
        connect = self.core.split("  on_client_connected:\n", 1)[1].split("  on_client_disconnected:", 1)[0]
        self.assertRegex(connect, r'(?s)if \(client_info.find\("Home Assistant"\).*?\{\s*id\(ha_refresh_after_connect\).execute')
        self.assertNotIn("delay:", connect)  # no detached old-connection timers

    def test_disconnect_preserves_replacement_recovery(self):
        # Execute the production lambda against the sockets remaining after
        # ESPHome removes the departing one, including a pre-subscription HA.
        disconnect = self.core.split("  on_client_disconnected:\n", 1)[1].split("\nscript:", 1)[0]
        callback = textwrap.dedent(disconnect.split("    - lambda: |-\n", 1)[1])
        harness = r'''
#include <cassert>
#include <string>
#include <vector>
#define ESP_LOGW(...) ((void) 0)
#define id(value) value
struct Client {
  const char *name;
  bool authenticated = true;
  bool removed = false;
  bool is_authenticated() const { return authenticated; }
  bool is_marked_for_removal() const { return removed; }
  const char *get_name() const { return name; }
};
struct Server {
  std::vector<Client *> clients;
  const auto &active_clients() const { return clients; }
} server;
namespace esphome::api { Server *global_api_server = &server; }
bool ha_api_available() { return esphome::api::global_api_server != nullptr; }
bool state_connected = false;
bool ha_api_state_connected() { return state_connected; }
struct Recovery {
  bool running = true;
  void stop() { running = false; }
} ha_refresh_after_connect;
bool retained = true, forecast_pending = true, cover_pending = true;
void ha_invalidate_retained_state() { retained = false; }
void weather_forecast_cancel_pending_requests() { forecast_pending = false; }
void cover_stop_cancel_pending_request() { cover_pending = false; }
void disconnect(const std::string &client_info) {
''' + callback + r'''
}
void reset(std::vector<Client *> clients) {
  server.clients = clients;
  state_connected = false;
  ha_refresh_after_connect.running = true;
  retained = forecast_pending = cover_pending = true;
}
void expect_preserved() {
  assert(ha_refresh_after_connect.running);
  assert(retained && forecast_pending && cover_pending);
}
void expect_cancelled() {
  assert(!ha_refresh_after_connect.running);
  assert(!retained && !forecast_pending && !cover_pending);
}
int main() {
  Client replacement{"Home Assistant 2026.8"};
  Client diagnostic{"ESPHome Logs"};
  Client unauthenticated{"Home Assistant 2026.8", false};
  Client closing{"Home Assistant 2026.8", true, true};
  // Identical names (and addresses) must not conflate overlapping sockets.
  reset({&replacement, &diagnostic});
  disconnect(replacement.name);
  expect_preserved();
  // Diagnostic disconnects must also preserve pre-subscription HA work.
  reset({&replacement});
  disconnect(diagnostic.name);
  expect_preserved();
  // A state-subscribed replacement remains protected too.
  reset({&replacement});
  state_connected = true;
  disconnect(replacement.name);
  expect_preserved();
  // Losing the final HA socket cancels even when log clients remain.
  reset({&diagnostic});
  disconnect(replacement.name);
  expect_cancelled();
  reset({});
  disconnect(replacement.name);
  expect_cancelled();
  // Dead and unauthenticated sockets cannot keep recovery alive.
  reset({&unauthenticated, &closing, &diagnostic});
  disconnect(replacement.name);
  expect_cancelled();
  // Diagnostic disconnects alone never stop the recovery script.
  reset({});
  disconnect(diagnostic.name);
  assert(ha_refresh_after_connect.running);
  reset({});
  esphome::api::global_api_server = nullptr;
  disconnect(replacement.name);
  expect_cancelled();
}
'''
        with tempfile.TemporaryDirectory(prefix="ha-reconnect-test-") as directory:
            executable = str(Path(directory) / "disconnect_test")
            subprocess.run(
                shlex.split(os.environ.get("CXX", "c++"))
                + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-x", "c++", "-", "-o", executable],
                input=harness, text=True, check=True,
            )
            subprocess.run([executable], check=True)

    def test_missing_companion_retry_does_not_force_another_download(self):
        retry = self.image.split("inline void image_card_request_current_picture(", 1)[1].split("inline void image_card_refresh_current_picture(", 1)[0]
        self.assertIn("image_card_request_media_artwork(ctx, false);", retry)
        self.assertNotIn("image_card_request_media_artwork(ctx, true);", retry)

    def test_reconnect_preserves_healthy_artwork_and_pending_metadata(self):
        refresh = self.image.split("inline void image_card_refresh_current_picture(", 1)[1].split("inline void image_card_schedule_picture_retry(", 1)[0]
        self.assertIn("!ctx->image_ready || ctx->media_artwork_refresh.forced", refresh)
        self.assertIn("image_card_schedule_media_artwork_refresh(ctx, force_refresh);", refresh)
        self.assertNotIn("media_artwork_trigger.reset()", refresh)
        self.assertLess(refresh.index("const bool force_refresh"), refresh.index("media_artwork_refresh.reset()"))


if __name__ == "__main__":
    unittest.main()
