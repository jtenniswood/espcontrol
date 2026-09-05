#include <cassert>

#include "session_state.h"

using esphome::companion::CompanionSessionState;

int main() {
  CompanionSessionState session;
  assert(!session.connected());
  assert(session.authenticated_socket() == -1);

  assert(session.authenticate(7) == -1);
  assert(session.connected());
  assert(session.authenticated_socket() == 7);

  assert(session.authenticate(9) == 7);
  assert(!session.disconnect_socket(7));
  assert(session.authenticated_socket() == 9);
  assert(session.disconnect_socket(9));
  assert(!session.connected());

  assert(session.authenticate(11) == -1);
  assert(session.disconnect() == 11);
  assert(session.disconnect() == -1);
  return 0;
}
