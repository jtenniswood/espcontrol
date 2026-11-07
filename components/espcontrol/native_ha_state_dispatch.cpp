// Version-pinned SDK seam: authentication and decoding remain in the SDK.
// The generated dispatcher calls this nonvirtual handler across object files.
// Unprepared/nonmember connections retain the unchanged SDK method. The
// generated authentication/decoding gate and all native subscriptions stay intact.
#include "native_ha_state_dispatch.h"
#ifdef USE_API_HOMEASSISTANT_STATES
#include "esphome/components/api/api_connection.h"
#include "esphome/core/version.h"
#include <type_traits>
using NativeConnection = esphome::api::APIConnection;
using NativeStateMessage = esphome::api::HomeAssistantStateResponse;
static_assert(ESPHOME_VERSION_CODE == VERSION_CODE(2026, 10, 1) ||
                  ESPHOME_VERSION_CODE == VERSION_CODE(2026, 10, 2),
              "Audit native HA state dispatch for the new ESPHome version");
static_assert(std::is_same<decltype(&NativeConnection::on_home_assistant_state_response),
                          void (NativeConnection::*)(const NativeStateMessage &)>::value,
              "Native HA handler signature changed");
extern "C" void __real__ZN7esphome3api13APIConnection32on_home_assistant_state_responseERKNS0_26HomeAssistantStateResponseE(NativeConnection *, const NativeStateMessage &);
extern "C" void __wrap__ZN7esphome3api13APIConnection32on_home_assistant_state_responseERKNS0_26HomeAssistantStateResponseE(NativeConnection *self, const NativeStateMessage &message) {
  auto guard = espcontrol::ha::native_lookup::owner().delivery();
  if (espcontrol::ha::native_lookup::dispatch(self, message)) return;
  ++espcontrol::ha::native_lookup::stock_frames;
  __real__ZN7esphome3api13APIConnection32on_home_assistant_state_responseERKNS0_26HomeAssistantStateResponseE(self, message);
}
#endif
