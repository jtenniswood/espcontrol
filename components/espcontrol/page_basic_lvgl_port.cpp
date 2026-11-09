// Compile the real LVGL port for every selected device profile. Production
// selection waits for the remaining drivers and hardware parity qualification.
#include "esphome/core/defines.h"
#ifdef USE_LVGL
#include "page_basic_lvgl_port.h"
#ifdef ESPCONTROL_DEVICE_UI_SLOT_COUNT
template class espcontrol::pages::PageBasicLvglPort<ESPCONTROL_DEVICE_UI_SLOT_COUNT>;
#endif
#endif
