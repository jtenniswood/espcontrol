#pragma once

#include "page_modal_router.h"

// Route each native family fixture through the application boundary too. The
// independent router test exercises switching between different family ports.
template <ControlModalKind... Kinds>
auto modal_test_router(espcontrol::pages::PageModalPort& port) {
  using namespace espcontrol::pages;
  return PageModalRouter<sizeof...(Kinds)>({{{Kinds, &port}...}}, &port,
                                           [](void* owner, PageBindingToken token) {
                                             return static_cast<PageModalPort*>(owner)->valid_origin(token);
                                           });
}
