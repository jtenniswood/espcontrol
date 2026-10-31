"""Validate sparse-preference metadata against real ESPHome text schemas."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="espcontrol-empty-defaults-") as directory:
    root = Path(directory)
    config = root / "empty-defaults.yaml"
    config.write_text(f'''esphome:
  name: empty-defaults-test
esp32:
  board: esp32-s3-devkitc-1
  framework:
    type: esp-idf
logger:
api:
wifi:
  ssid: compiled-test-network
  password: test-password
espcontrol:
  id: target_app
  panel_config:
    device_profile: fixture
    button_order: order
    button_on_color: color
    buttons:
      - config: button
        subpage_chunks: [empty_restore, volatile_text, calculated_text, copied_text, another_empty, nonempty]
external_components:
  - source:
      type: local
      path: {ROOT / "components"}
    components: [espcontrol, web_server_idf]
text:
  - platform: template
    mode: text
    id: order
    optimistic: true
    restore_value: true
    initial_value: ""
  - platform: template
    mode: text
    id: color
    optimistic: true
    restore_value: true
    initial_value: "112233"
  - platform: template
    mode: text
    id: button
    optimistic: true
    restore_value: true
    initial_value: factory
  - platform: template
    mode: text
    id: empty_restore
    optimistic: true
    restore_value: true
    initial_value: ""
  - platform: template
    mode: text
    id: volatile_text
    optimistic: true
    restore_value: false
    initial_value: ""
  - platform: template
    mode: text
    id: calculated_text
    set_action:
      - lambda: (void) x;
    lambda: return {{"live"}};
  - platform: copy
    mode: text
    id: copied_text
    source_id: empty_restore
  - platform: template
    mode: text
    id: another_empty
    optimistic: true
    restore_value: true
  - platform: template
    mode: text
    id: nonempty
    optimistic: true
    restore_value: true
    initial_value: preserve
  - platform: template
    mode: text
    id: unrelated
    optimistic: true
    restore_value: true
    initial_value: ""
''')
    result = subprocess.run(
        [sys.executable, "-m", "esphome", "compile", str(config), "--only-generate"],
        capture_output=True, text=True,
    )
    if result.returncode:
        print(result.stdout)
        print(result.stderr)
        raise SystemExit(result.returncode)
    generated = (root / ".esphome/build/empty-defaults-test/src/main.cpp").read_text()
    defines = (root / ".esphome/build/empty-defaults-test/src/esphome/core/defines.h").read_text()
    assert "#define ESPCONTROL_PANEL_CONFIG_TEXT_SLOT_COUNT 1" in defines
    assert "#define ESPCONTROL_PANEL_CONFIG_TEXT_CHUNK_COUNT 6" in defines
    marked = set(re.findall(r"target_app->set_panel_config_text_empty_default\((\w+)\)", generated))
    assert marked == {"order", "empty_restore", "another_empty"}, marked
    assert "target_app->set_panel_config_button(1," in generated
    assert generated.index("target_app->set_panel_config_button(1,") < generated.index(
        "target_app->set_panel_config_text_empty_default("
    )
print("ESPHome sparse preferences: only referenced, restorable, initial-empty templates are marked.")
