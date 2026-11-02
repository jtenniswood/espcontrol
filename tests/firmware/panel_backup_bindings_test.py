"""Check export allowlisting, renamed entities and optional device features."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("backup_bindings", ROOT / "components/espcontrol/backup.py")
backup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(backup)


class BackupBindingsTest(unittest.TestCase):
    def test_types_and_custom_names(self):
        config = {
            "substitutions": {"entity_screen_language": "Custom language"},
            "select": [{"name": "Custom language", "id": "language"}],
            "switch": [{"name": "Screen: Clock Bar", "id": "clock"}],
            "number": [{"name": "Screen: Daytime Brightness", "id": "brightness"}],
            "text": [{"name": "WiFi Password", "id": "secret"}],
        }
        self.assertEqual(list(backup.configured_backup_fields(config)), [
            ("settings", "clock_bar", "switch", "clock"),
            ("settings", "language", "select", "language"),
            ("screen", "brightness_day", "number", "brightness"),
        ])

    def test_no_configured_features(self):
        self.assertEqual(list(backup.configured_backup_fields({})), [])

    def test_fields_are_unique_and_do_not_expose_credentials(self):
        fields = [(row[0], row[1]) for row in backup.BACKUP_FIELDS]
        self.assertEqual(len(fields), len(set(fields)))
        self.assertTrue(all(row[3] in {"switch", "select", "number", "text", "light"} for row in backup.BACKUP_FIELDS))
        self.assertFalse(any(word in row[1] for row in backup.BACKUP_FIELDS for word in ("password", "pin", "encryption", "api_key")))


if __name__ == "__main__":
    unittest.main()
