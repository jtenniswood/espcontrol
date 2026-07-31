#include "espcontrol_app.h"

namespace espcontrol {

void EspControlApp::setup() { core_.start(); }

void EspControlApp::loop() { core_.run_once(); }

void EspControlApp::on_shutdown() { core_.stop(); }

}  // namespace espcontrol
