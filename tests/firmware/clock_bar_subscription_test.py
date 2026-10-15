#!/usr/bin/env python3
"""An already connected HA client must learn new clock bar subscriptions without reconnecting."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
header = (root / 'components/espcontrol/editable_clock_bar.h').read_text()
start = header.index('inline void bind()')
end = header.index('inline void rebuild(', start)
program = r'''
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>
#include "clock_bar_layout.h"
using namespace espcontrol::clockbar;
namespace esphome { using StringRef = std::string; }
constexpr unsigned SUBSCRIPTION_SCOPE = 1u << 4;
struct Widget { Element config; float value = NAN; };
struct Runtime { unsigned generation = 0; std::vector<Widget> widgets; };
Runtime &runtime() { static Runtime state; return state; }
struct Subscription { std::string entity; std::function<void(esphome::StringRef)> callback; };
std::vector<Subscription> subscriptions;
int announces = 0, resets = 0;
unsigned ha_subscription_generation() { return 7; }
void ha_reset_subscription_callbacks(unsigned scope) { assert(scope == SUBSCRIPTION_SCOPE); subscriptions.clear(); ++resets; }
bool parse_float_ref(const std::string &text, float &value) { char *end; value = std::strtof(text.c_str(), &end); return *end == '\0'; }
bool ha_subscribe_state(const std::string &entity, std::function<void(esphome::StringRef)> callback, unsigned scope, bool retain) {
 assert(scope == SUBSCRIPTION_SCOPE && retain);
 subscriptions.push_back({entity, callback});
 return true;
}
void ha_reannounce_state_subscriptions() {
 ++announces;
 // Model an existing client delivering states only after it receives the expanded subscription list.
 for (auto &subscription : subscriptions) subscription.callback(subscription.entity == "sensor.new" ? "63" : "42");
}
'''
program += header[start:end]
program += r'''
int main() {
 auto &state = runtime();
 Element local; local.type = "temperature"; local.source = "local"; local.entity = "room_temp";
 Element first; first.type = "temperature"; first.source = "ha"; first.entity = "sensor.room";
 Element second = first; second.type = "humidity"; second.entity = "sensor.humidity";
 Element empty = first; empty.entity.clear();
 Element network = first; network.type = "network";
 state.widgets = {{local, 10}, {network, 0}};
 bind(); assert(announces == 0 && subscriptions.empty());
 state.widgets = {{first}, {second}, {local, 10}, {empty}, {network, 0}};
 bind();
 assert(state.generation == 7 && announces == 1 && subscriptions.size() == 2);
 assert(state.widgets[0].value == 42 && state.widgets[1].value == 42 && state.widgets[2].value == 10);
 state.widgets[0].config.entity = "sensor.new"; state.widgets[0].value = NAN;
 bind();
 assert(announces == 2 && subscriptions.size() == 2 && subscriptions[0].entity == "sensor.new");
 assert(state.widgets[0].value == 63 && state.widgets[1].value == 42);
 subscriptions[0].callback("unavailable"); assert(std::isnan(state.widgets[0].value));
 state.widgets.clear(); bind(); assert(announces == 2 && subscriptions.empty() && resets == 4);
}
'''
with tempfile.TemporaryDirectory(prefix='clock-bar-subscriptions-') as directory:
    source = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    source.write_text(program)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I', str(root/'components/espcontrol'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Added and edited HA sensors reannounce once per binding batch without reconnecting.')
