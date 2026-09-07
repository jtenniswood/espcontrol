#include <cassert>
#include <cstdlib>
#include <iostream>
#include <new>
#include "companion_controls.h"

static size_t allocations = 0;
void *operator new(std::size_t size) {
  ++allocations;
  if (void *p = std::malloc(size ? size : 1)) return p;
  throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }

int main() {
  espcontrol::EspControlAppCore first, second;
  first.companion_runtime().set_actions({{"com.example.First", "First"}});
  first.companion_runtime().set_connected(true);
  second.companion_runtime().set_actions({{"com.example.Second", "Second"}});
  assert(!second.companion_runtime().connected());
  assert(first.companion_runtime().snapshot().actions[0].id == "com.example.First");
  assert(second.companion_runtime().snapshot().actions[0].id == "com.example.Second");
  const auto before = allocations;
  for (int i = 0; i < 1000; ++i) assert(first.companion_runtime().connected());
  assert(allocations == before); // Simple status reads never copy the catalogue.
  assert(first.start());
  bool invoked = false;
  assert(companion_expect_action_result("old-session", [&] { invoked = true; }));
  companion_set_connected(false);
  companion_deliver_action_result("old-session", "activated");
  assert(!invoked);
  auto lifetime = std::make_shared<int>(1);
  std::weak_ptr<int> callback_lifetime = lifetime;
  companion_action_sender() = [lifetime](const std::string &, const std::string &) { return true; };
  first.companion_runtime().begin_pairing = [lifetime] {};
  first.companion_runtime().revoke_pairing = [lifetime] {};
  assert(!second.companion_runtime().begin_pairing);
  assert(!second.companion_runtime().revoke_pairing);
  lifetime.reset();
  assert(!callback_lifetime.expired());
  assert(first.stop());
  assert(callback_lifetime.expired());
  assert(second.companion_runtime().snapshot().actions[0].id == "com.example.Second");
  std::cout << "Companion service: " << sizeof(CompanionRuntimeService)
            << " bytes; connection reads: 0 allocations\n";
}
