"""Check ESPHome emits reset startup independently of merged on_boot actions."""
from pathlib import Path
import subprocess
import sys
import tempfile
import re

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="espcontrol-reset-boot-") as directory:
    root = Path(directory)
    config = root / "reset-boot.yaml"
    config.write_text(f'''esphome:
  name: reset-boot-test
esp32:
  board: esp32-s3-devkitc-1
  framework:
    type: esp-idf
logger:
api:
ota:
  - platform: esphome
wifi:
  ssid: compiled-test-network
  password: test-password
  ap:
web_server:
espcontrol:
external_components:
  - source:
      type: local
      path: {ROOT / "components"}
    components: [espcontrol, web_server_idf]
packages:
  first:
    esphome:
      on_boot:
        priority: 600
        then:
          - lambda: '(void) 123;'
  later:
    esphome:
      on_boot:
        - priority: -100
          then:
            - lambda: '(void) 456;'
''')
    result = subprocess.run([sys.executable, "-m", "esphome", "compile", str(config), "--only-generate"],
                            capture_output=True, text=True)
    if result.returncode:
        print(result.stdout)
        print(result.stderr)
        raise SystemExit(result.returncode)
    generated = (root / ".esphome/build/reset-boot-test/src/main.cpp").read_text()
    assert "(void) 123" not in generated, "Fixture must reproduce the on_boot replacement"
    assert "(void) 456" in generated
    assert "espcontrol::reset::ResetBoot()" in generated, "Reset must have its own registered boot component"
    assert "espcontrol::reset::early_startup(true" in generated
    binding = re.search(r"new\((\w+)\) espcontrol::reset::ResetBoot\(\)", generated)
    assert binding and f"App.register_component_({binding[1]}," in generated
print("ESPHome codegen: reset boot component survives an actual mixed-form package on_boot replacement.")
