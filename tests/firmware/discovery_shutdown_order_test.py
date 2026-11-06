"""Run the production shutdown hooks in ESPHome's two-phase shutdown order."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/espcontrol_app.cpp').read_text()
functions = []
for name in ('on_safe_shutdown', 'on_shutdown'):
    marker = 'void EspControlApp::' + name + '() {'
    if marker not in source:
        continue
    start = source.index(marker)
    end = source.index('\n}', start) + 2
    functions.append(source[start:end])

harness = r'''
#include <cassert>
namespace esphome::artwork_image {
struct TransferObserver {
  static TransferObserver &instance() { static TransferObserver observer; return observer; }
  void set(int, int) {}
};
}
namespace espcontrol {
namespace cards { void set_card_runtime_registry_service(void *) {} }
bool mdns_alive = true;
struct Resolver {
  bool pending_query = true;
  unsigned releases = 0;
  void shutdown() {
    // ESP-IDF frees outstanding asynchronous query handles in mdns_free().
    if (pending_query) { assert(mdns_alive); pending_query = false; ++releases; }
  }
};
struct Core { bool stop() { return true; } };
struct Component {
  virtual void on_safe_shutdown() {}
  virtual void on_shutdown() {}
};
struct EspControlApp : Component {
  Resolver home_assistant_endpoint_;
  Core core_;
'''
for name in ('on_safe_shutdown', 'on_shutdown'):
    if any('EspControlApp::' + name + '(' in f for f in functions):
        harness += '  void ' + name + '() override;\n'
harness += '};\n' + '\n'.join(functions) + r'''
}
int main() {
  using namespace espcontrol;
  EspControlApp app;
  Component &component = app;
  // App.safe_reboot(): every safe hook runs, then shutdown proceeds in
  // reverse setup order (mDNS AFTER_CONNECTION before EspControl AFTER_WIFI).
  component.on_safe_shutdown();
  assert(!app.home_assistant_endpoint_.pending_query);
  assert(app.home_assistant_endpoint_.releases == 1);
  mdns_alive = false;
  component.on_shutdown();
  assert(app.home_assistant_endpoint_.releases == 1);

  // Forced reboot skips safe hooks; don't dereference mDNS-owned handles
  // after the service has already destroyed them. Reset reclaims the memory.
  EspControlApp forced;
  forced.on_shutdown();
  assert(forced.home_assistant_endpoint_.releases == 0);
}
'''
with tempfile.TemporaryDirectory(prefix='discovery-shutdown-') as directory:
    cpp = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    cpp.write_text(harness)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
