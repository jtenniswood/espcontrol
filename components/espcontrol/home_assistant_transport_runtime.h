#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "esphome/core/string_ref.h"

namespace espcontrol::ha {
using StateCallback = std::function<void(esphome::StringRef)>;
uint32_t subscribe_owned(const std::string &entity, const std::string &attribute,
                         StateCallback callback);
void unsubscribe_owned(uint32_t token);
void set_subscription_maintenance(void (*maintenance)());
void maintain_subscriptions();
}  // namespace espcontrol::ha
