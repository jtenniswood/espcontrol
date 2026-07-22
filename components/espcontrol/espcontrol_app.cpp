#include "espcontrol_app.h"

namespace espcontrol {

void EspControlApp::setup() {
  core_.start();
  card_assets_.start();
}

void EspControlApp::loop() {
  core_.run_once();
  card_assets_.loop();
}

void EspControlApp::on_shutdown() {
  card_assets_.stop();
  core_.stop();
}

}  // namespace espcontrol
