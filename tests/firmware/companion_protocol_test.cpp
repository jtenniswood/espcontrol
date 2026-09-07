#include <fstream>
#include <iostream>
#include "companion_protocol_generated.h"

int main(int argc, char **argv) {
  if (argc != 2) return 2;
  std::ifstream input(argv[1]);
  ArduinoJson::JsonDocument fixtures;
  if (!input || deserializeJson(fixtures, input)) return 2;
  for (ArduinoJson::JsonObjectConst fixture : fixtures.as<ArduinoJson::JsonArrayConst>()) {
    const std::string state = fixture["state"];
    const auto session = state == "connected" ? companion_protocol::SessionState::CONNECTED :
      state == "pairing" ? companion_protocol::SessionState::PAIRING : companion_protocol::SessionState::AUTHENTICATING;
    const auto direction = std::string(fixture["direction"]) == "mac_to_panel" ?
      companion_protocol::Direction::MAC_TO_PANEL : companion_protocol::Direction::PANEL_TO_MAC;
    const auto decoded = companion_protocol::decode(fixture["message"].as<ArduinoJson::JsonObjectConst>(), direction, session);
    if (decoded.has_value() != fixture["valid"].as<bool>()) {
      std::cerr << fixture["name"].as<const char *>() << " failed\n";
      return 1;
    }
  }
  companion_protocol::ActionInvoke command;
  command.requestId = "quoted-request";
  command.kind = "action";
  command.actionId = "com.example.\\quoted\"app";
  ArduinoJson::JsonDocument encoded;
  companion_protocol::encode(encoded.to<ArduinoJson::JsonObject>(), command);
  std::string wire;
  serializeJson(encoded, wire);
  ArduinoJson::JsonDocument received;
  if (deserializeJson(received, wire)) return 1;
  const auto roundtrip = companion_protocol::decode(received.as<ArduinoJson::JsonObjectConst>(),
    companion_protocol::Direction::PANEL_TO_MAC, companion_protocol::SessionState::CONNECTED);
  if (!roundtrip || std::get<companion_protocol::ActionInvoke>(*roundtrip).actionId != command.actionId) return 1;
  std::cout << fixtures.size() << " shared Companion protocol fixtures passed\n";
}
