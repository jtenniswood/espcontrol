"""Render the production setup lambdas before AP startup with default/custom IPs."""
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SCREENS = (
    ("common/device/screen_loading.yaml", "loading_status_label"),
    ("common/device/screen_wifi_setup.yaml", "wifi_setup_instructions"),
    ("common/addon/connectivity.yaml", "wifi_setup_instructions"),
)
functions = []
for index, (filename, label) in enumerate(SCREENS):
    text = (ROOT / filename).read_text()
    start = text.index("auto hotspot_ap =")
    end = text.index(f"lv_label_set_display_text(id({label}), msg.c_str());", start)
    body = text[start:end].replace("${friendly_name}", "Test Display")
    functions.append(f"std::string render_{index}() {{\n{body}\nreturn msg;\n}}")

languages = [path.name.removeprefix("strings.").removesuffix(".txt")
             for path in sorted((ROOT / "product/v2/translations").glob("strings.*.txt"))]
source = r'''
#include <cassert>
#include <optional>
#include <string>
#include "i18n_generated.h"
#define id(value) value
std::string wifi_setup_title;
void lv_label_set_text(std::string &label, const char *text) { label = text; }
namespace wifi {
struct StringRef {
  std::string value;
  std::string str() const { return value; }
};
struct IPAddress {
  std::string value;
  std::string str() const { return value; }
};
struct ManualIP { IPAddress static_ip; };
struct AP {
  StringRef ssid;
  std::optional<ManualIP> manual_ip;
  const StringRef &get_ssid() const { return ssid; }
#ifdef USE_WIFI_MANUAL_IP
  const std::optional<ManualIP> &get_manual_ip() const { return manual_ip; }
#endif
};
struct WiFi {
  AP ap;
  AP get_ap() const { return ap; }
  // No running AP/IP accessor: loading instructions must work before it starts.
} component;
WiFi *global_wifi_component = &component;
}
''' + "\n".join(functions) + r'''
int main() {
  using Render = std::string (*)();
  for (Render render : {render_0, render_1, render_2}) {
    wifi::component.ap = {{"EspControl_F36A"}, std::nullopt};
    set_espcontrol_language("en");
    assert(render() == "Connect to the WiFi hotspot 'EspControl_F36A' to configure your network.\n"
                       "Then visit 192.168.4.1 in your browser");
    for (const char *language : LANGUAGES) {
      set_espcontrol_language(language);
      wifi::component.ap = {{"EspControl_F36A"}, std::nullopt};
      assert(render().find("192.168.4.1") != std::string::npos);
#ifdef USE_WIFI_MANUAL_IP
      for (const char *address : {"10.42.0.1", "172.22.1.99"}) {
        wifi::component.ap = {{"Custom Hotspot"}, wifi::ManualIP{{address}}};
        const auto message = render();
        assert(message.find(address) != std::string::npos);
        assert(message.find("192.168.4.1") == std::string::npos);
        assert(message.find("Custom Hotspot") != std::string::npos);
      }
#endif
    }
  }
}
'''
source = source.replace("LANGUAGES", "{" + ",".join(json.dumps(lang) for lang in languages) + "}")
with tempfile.TemporaryDirectory(prefix="espcontrol-setup-address-") as directory:
    root = Path(directory)
    cpp = root / "setup-address.cpp"
    cpp.write_text(source)
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    for manual_ip in (False, True):
        binary = root / ("setup-address-manual" if manual_ip else "setup-address-default")
        flags = ["-DUSE_WIFI_MANUAL_IP"] if manual_ip else []
        subprocess.run([*compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", *flags, str(cpp),
                        "-I", str(ROOT / "components/espcontrol"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
print(f"WiFi setup: three production screens render default/custom AP addresses before startup in all {len(languages)} languages.")
