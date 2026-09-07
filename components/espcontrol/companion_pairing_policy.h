#pragma once

#include <optional>
#include "companion_runtime.h"
#include "companion_capabilities_generated.h"

// This is the endpoint's operation, shared with the host tests. Authorization
// is evaluated before calling providers or activating a pairing window.
template<typename Provider, typename BeginPairing>
std::optional<CompanionPairingSnapshot> companion_pairing_status(
    bool authorized, Provider provider, BeginPairing begin) {
  if (!authorized) return std::nullopt;
  auto snapshot = provider();
  if (COMPANION_BROWSER_STARTS_PAIRING && snapshot.available &&
      !snapshot.paired && !snapshot.active) {
    begin();
    snapshot = provider();
  }
  if (!COMPANION_BROWSER_EXPOSES_PAIRING_CODE || snapshot.paired || !snapshot.active)
    snapshot.pairing_code.clear();
  return snapshot;
}
