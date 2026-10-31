"""Check actual ESPHome generation links the workaround only for P4."""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="espcontrol-p4-restart-codegen-") as temporary:
    root = Path(temporary)
    for variant in ("esp32p4", "esp32s3"):
        hosted = (ROOT / "devices/guition-esp32-p4-jc1060p470/device/network_coprocessor.yaml").read_text() if variant == "esp32p4" else ""
        config = root / f"{variant}.yaml"
        config.write_text(f'''esphome:
  name: restart-{variant}
esp32:
  variant: {variant}
  flash_size: 16MB
  framework:
    type: esp-idf
logger:
api:
ota:
  - platform: esphome
wifi:
  ssid: compiled-test-network
  password: test-password
espcontrol:
external_components:
  - source:
      type: local
      path: {ROOT / "components"}
    components: [espcontrol, web_server_idf]
{hosted}
''')
        result = subprocess.run([sys.executable, "-m", "esphome", "compile", str(config), "--only-generate"], capture_output=True, text=True)
        if result.returncode:
            print(result.stdout)
            print(result.stderr)
            raise SystemExit(result.returncode)
        build = root / f".esphome/build/restart-{variant}"
        definitions = (build / "src/esphome/core/defines.h").read_text()
        cmake = (build / "src/CMakeLists.txt").read_text()
        assert ("#define ESPCONTROL_P4_RESTART_WORKAROUND" in definitions) == (variant == "esp32p4")
        assert ("--wrap=esp_restart_noos" in cmake) == (variant == "esp32p4")
print("ESPHome generation: P4 links the version-bound reset path; S3 does not.")
