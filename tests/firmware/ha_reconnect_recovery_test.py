"""Wiring regressions for the recovery work seen in the 7-inch debug log."""

from pathlib import Path
import re
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

    def test_only_ha_clients_start_and_cancel_recovery(self):
        connect = self.core.split("  on_client_connected:\n", 1)[1].split("  on_client_disconnected:", 1)[0]
        self.assertRegex(connect, r'(?s)if \(client_info.find\("Home Assistant"\).*?\{\s*id\(ha_refresh_after_connect\).execute')
        self.assertNotIn("delay:", connect)  # no detached old-connection timers
        disconnect = self.core.split("  on_client_disconnected:\n", 1)[1].split("\nscript:", 1)[0]
        self.assertIn("if (is_home_assistant) id(ha_refresh_after_connect).stop();", disconnect)

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
