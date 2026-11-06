"""Exercise the production one-time curve migration against queued preferences."""
from pathlib import Path
import json
import os
import subprocess
import tempfile

import yaml

ROOT = Path(__file__).resolve().parents[2]
class Loader(yaml.SafeLoader):
    pass

Loader.add_constructor("!lambda", lambda loader, node: loader.construct_scalar(node))
config = yaml.load((ROOT / "common/config/display.yaml").read_text(), Loader=Loader)
selector = next(item for item in config["select"] if item.get("id") == "card_curve_select")
assert selector["initial_option"] == "Medium" and selector["restore_value"]
marker = next(item for item in config["globals"] if item["id"] == "card_curve_medium_default_migrated")
assert marker["restore_value"] and marker["initial_value"] == "false"
boot = next(item for item in config["esphome"]["on_boot"]
            if "card_curve_medium_default_migrated" in str(item))
body = boot["then"][0]["lambda"]
for slug in json.loads((ROOT / "devices/manifest.json").read_text())["devices"]:
    device = yaml.load((ROOT / "devices" / slug / "device/sensors.yaml").read_text(), Loader=Loader)
    for phase in device.get("esphome", {}).get("on_boot", []):
        if "grid_phase1(" in str(phase):
            assert boot["priority"] > phase["priority"], slug

fixture = r'''
#include <cassert>
#include <string>
struct Preferences {
  std::string saved, pending;
  bool migrated = false, pending_marker = false, has_marker = false;
  int syncs = 0;
  void sync() {
    if (!pending.empty()) { saved = pending; pending.clear(); }
    if (has_marker) { migrated = pending_marker; has_marker = false; }
    ++syncs;
  }
} *global_preferences;
struct Marker {
  bool value;
  void update() {
    global_preferences->pending_marker = value;
    global_preferences->has_marker = true;
  }
} *card_curve_medium_default_migrated;
struct Selector {
  std::string state;
  struct Call {
    Selector *target;
    std::string option;
    void set_option(const char *value) { option = value; }
    void perform() { target->state = option; global_preferences->pending = option; }
  };
  Call make_call() { return {this, {}}; }
} *card_curve_select;
bool &resolve(Marker *value) { return value->value; }
Selector &resolve(Selector *value) { return *value; }
#define id(name) resolve(name)
void migrate() {
__PRODUCTION_BODY__
}
void boot(Preferences &prefs) {
  global_preferences = &prefs;
  Marker marker{prefs.migrated};
  Selector selector{prefs.saved.empty() ? "__INITIAL_OPTION__" : prefs.saved};
  card_curve_medium_default_migrated = &marker;
  card_curve_select = &selector;
  migrate();
  assert(selector.state == prefs.saved);
}
int main() {
  // New panels and upgrades from every previous choice migrate before drawing.
  for (const char *old : {"", "Small", "Medium", "Large"}) {
    Preferences prefs;
    prefs.saved = old;
    boot(prefs);
    assert(prefs.saved == "Medium" && prefs.migrated && prefs.syncs == 1);
    // Simulate a later user's saved choice, then multiple restarts/upgrades.
    for (const char *choice : {"Small", "Large", "Medium"}) {
      prefs.saved = choice;
      boot(prefs); boot(prefs);
      assert(prefs.saved == choice && prefs.syncs == 1);
    }
  }
}
'''.replace("__PRODUCTION_BODY__", body).replace("__INITIAL_OPTION__", selector["initial_option"])
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / "migration.cpp"
    binary = Path(directory) / "migration"
    source.write_text(fixture)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror", str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Curve migration: new/existing panels start at Medium once; later choices survive restarts. All profiles migrate before grid setup.")
