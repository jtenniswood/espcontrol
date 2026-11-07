#pragma once
#include "api_server.h"
namespace esphome::api {
struct HomeAssistantStateResponse {
  StringRef entity_id, attribute, state;
};
}
