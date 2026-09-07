"""Connection wiring and executable artwork recovery regressions."""

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


class ArtworkRecoveryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(prefix="artwork-recovery-test-")
        cls.addClassCleanup(cls.directory.cleanup)
        directory = Path(cls.directory.name)
        source = (ROOT / "components/espcontrol/button_grid_image.h").read_text()
        names = (
            "image_card_request_current_picture",
            "image_card_refresh_current_picture",
            "image_card_process_media_artwork",
            "image_card_media_artwork_timer_cb",
            "image_card_schedule_media_artwork_process",
            "image_card_handle_media_artwork_picture",
            "image_card_request_media_artwork",
            "image_card_media_artwork_trigger_timer_cb",
            "image_card_schedule_media_artwork_refresh",
            "image_card_refresh_media_artwork_on_metadata_change",
        )
        functions = []
        for name in names:
            match = re.search(
                rf"^inline void {name}\([^;{{]*\) \{{\n.*?^\}}",
                source, re.MULTILINE | re.DOTALL,
            )
            if match is None:
                raise AssertionError(f"Missing production function: {name}")
            functions.append(match.group())
        (directory / "artwork_recovery_functions.h").write_text("\n\n".join(functions))
        cls.executable = str(directory / "artwork_recovery_test")
        subprocess.run(
            shlex.split(os.environ.get("CXX", "c++"))
            + ["-std=c++17", "-Wall", "-Wextra", "-Werror",
               "-I", str(directory), "-I", str(ROOT / "components/espcontrol"),
               str(ROOT / "tests/firmware/artwork_recovery_test.cpp"),
               "-o", cls.executable],
            check=True,
        )

    def test_timeout_retry_refreshes_same_url_once(self):
        subprocess.run([self.executable, "timeout_retry"], check=True)

    def test_unchanged_artwork_stays_cached(self):
        subprocess.run([self.executable, "unchanged"], check=True)

    def test_missing_companion_does_not_repeat_download(self):
        subprocess.run([self.executable, "missing_companion"], check=True)

    def test_reconnect_preserves_pending_metadata(self):
        subprocess.run([self.executable, "pending_metadata_reconnect"], check=True)

    def test_reconnect_preserves_active_metadata(self):
        subprocess.run([self.executable, "active_metadata_reconnect"], check=True)

    def test_reconnect_recovers_missing_image(self):
        subprocess.run([self.executable, "missing_image_reconnect"], check=True)

    def test_reconnect_recovers_after_retry_exhaustion(self):
        subprocess.run([self.executable, "exhausted_reconnect"], check=True)


if __name__ == "__main__":
    unittest.main()
