#include <cassert>
#include "companion_pairing_policy.h"

int main() {
  CompanionPairingSnapshot state;
  state.available = true;
  int reads = 0, starts = 0;
  auto provider = [&] { ++reads; return state; };
  auto begin = [&] { ++starts; state.active = true; state.pairing_code = "ABCD-EFGH"; };
  assert(!companion_pairing_status(false, provider, begin));
  assert(reads == 0 && starts == 0);
  auto first = companion_pairing_status(true, provider, begin);
  assert(first && first->pairing_code == "ABCD-EFGH" && starts == 1);
  auto repeated = companion_pairing_status(true, provider, begin);
  assert(repeated && repeated->pairing_code == first->pairing_code && starts == 1);
  state.active = false; // Expired code: a fresh authorized visit starts a new window.
  assert(companion_pairing_status(true, provider, begin)->active && starts == 2);
  state.paired = true;
  assert(companion_pairing_status(true, provider, begin)->pairing_code.empty());
  state.active = false; state.connected = false; // Reconnect does not require pairing again.
  assert(companion_pairing_status(true, provider, begin)->pairing_code.empty() && starts == 2);
  state.available = false; state.paired = false;
  assert(!companion_pairing_status(true, provider, begin)->available && starts == 2);
}
