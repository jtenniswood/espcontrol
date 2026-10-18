#include "page_renderer_runtime.h"

// Compile the selected profile's complete binding owner with the real device
// toolchain even before production selects a widget port. No instance or
// second renderer is retained by this explicit instantiation.
#ifdef ESPCONTROL_DEVICE_UI_SLOT_COUNT
template class espcontrol::pages::PageRendererRuntime<ESPCONTROL_DEVICE_UI_SLOT_COUNT>;
#endif
