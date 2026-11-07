// SPDX-FileCopyrightText: 2022-2025 Espressif Systems (Shanghai) CO LTD
// SPDX-FileCopyrightText: 2026 EspControl contributors
// SPDX-License-Identifier: Apache-2.0
//
// ESP-IDF v5.5.6 components/esp_system/port/soc/esp32p4/system_internal.c,
// adapted for https://github.com/espressif/esp-idf/issues/19171.
// Preserve the SDK reset path; stall the other core without resetting it and
// wait for an in-flight cache sync before starting writeback. Review/remove
// this version-bound backport when the pinned SDK includes the upstream fix.
#include "esphome/core/defines.h"

#ifdef USE_ESP_IDF
#include "sdkconfig.h"

#if defined(CONFIG_IDF_TARGET_ESP32P4) && defined(ESPCONTROL_P4_RESTART_WORKAROUND)
#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_idf_version.h"
#include "esp_macros.h"
#include "esp_memory_utils.h"
#include "esp_private/rtc_clk.h"
#include "esp_private/system_internal.h"
#include "esp_rom_sys.h"
#include "esp32p4/rom/cache.h"
#include "esp32p4/rom/rtc.h"
#include "hal/wdt_hal.h"
#include "riscv/rv_utils.h"
#include "soc/cache_reg.h"
#include "soc/hp_sys_clkrst_reg.h"
#include "soc/hp_system_reg.h"
#include "soc/rtc.h"
#include "soc/rtc_periph.h"
#include "soc/soc.h"

#if ESP_IDF_VERSION != ESP_IDF_VERSION_VAL(5, 5, 6)
#error "Review the P4 restart workaround against the newly pinned ESP-IDF reset path"
#endif

extern "C" int _bss_end;

static void IRAM_ATTR __attribute__((noinline, noreturn)) espcontrol_restart_noos_inner() {
  const uint32_t core_id = esp_cpu_get_core_id();

  // The other core is stalled. Let its last sync finish before touching the
  // shared command registers. The SDK's one-second RTC watchdog remains armed.
  constexpr uint32_t sync_operations = CACHE_INVALIDATE_ENA | CACHE_CLEAN_ENA |
                                       CACHE_WRITEBACK_ENA | CACHE_WRITEBACK_INVALIDATE_ENA;
  while (REG_READ(CACHE_SYNC_CTRL_REG) & sync_operations) {}

#if CONFIG_SPIRAM
  Cache_WriteBack_All(CACHE_MAP_L1_DCACHE);
#endif
  Cache_Disable_L2_Cache();
  esp_system_reset_modules_on_exit();

#if !CONFIG_IDF_ENV_FPGA
  rtc_clk_cpu_set_to_default_config();
#endif
#if !CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE
  ets_set_appcpu_boot_addr(0);
#endif
#if CONFIG_SPIRAM_INSTRUCTIONS_RODATA
  REG_CLR_BIT(HP_SYS_HP_PSRAM_FLASH_ADDR_INTERCHANGE_REG,
              HP_SYS_HP_PSRAM_FLASH_ADDR_INTERCHANGE_DMA |
                  HP_SYS_HP_PSRAM_FLASH_ADDR_INTERCHANGE_CPU);
#endif

  if (core_id == 0) {
#if !CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE
    esp_cpu_reset(1);
#endif
    esp_cpu_reset(0);
  }
#if !CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE
  else {
    esp_cpu_reset(0);
    esp_cpu_unstall(0);
    esp_cpu_reset(1);
  }
#endif
  ESP_INFINITE_LOOP();
}

extern "C" void IRAM_ATTR __attribute__((noreturn)) __wrap_esp_restart_noos() {
  rv_utils_intr_global_disable();
#if SOC_RTC_WDT_SUPPORTED
  wdt_hal_context_t rtc_wdt_ctx;
  wdt_hal_init(&rtc_wdt_ctx, WDT_RWDT, 0, false);
  const uint32_t stage_timeout_ticks =
      static_cast<uint32_t>(1000ULL * rtc_clk_slow_freq_get_hz() / 1000ULL);
  wdt_hal_write_protect_disable(&rtc_wdt_ctx);
  wdt_hal_config_stage(&rtc_wdt_ctx, WDT_STAGE0, stage_timeout_ticks, WDT_STAGE_ACTION_RESET_SYSTEM);
  wdt_hal_config_stage(&rtc_wdt_ctx, WDT_STAGE1, stage_timeout_ticks, WDT_STAGE_ACTION_RESET_RTC);
  wdt_hal_set_flashboot_en(&rtc_wdt_ctx, true);
  wdt_hal_write_protect_enable(&rtc_wdt_ctx);
#endif

#if !CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE
  const uint32_t core_id = esp_cpu_get_core_id();
  esp_cpu_stall(core_id == 0 ? 1 : 0);
#endif

#if SOC_WDT_SUPPORTED
  wdt_hal_context_t wdt0_context = {.inst = WDT_MWDT0, .mwdt_dev = &TIMERG0};
  wdt_hal_write_protect_disable(&wdt0_context);
  wdt_hal_disable(&wdt0_context);
  wdt_hal_write_protect_enable(&wdt0_context);
  wdt_hal_context_t wdt1_context = {.inst = WDT_MWDT1, .mwdt_dev = &TIMERG1};
  wdt_hal_write_protect_disable(&wdt1_context);
  wdt_hal_disable(&wdt1_context);
  wdt_hal_write_protect_enable(&wdt1_context);
#endif

#ifdef CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM
  if (esp_ptr_external_ram(esp_cpu_get_sp())) {
    const uintptr_t new_sp = reinterpret_cast<uintptr_t>(&_bss_end) & ~uintptr_t{15};
    rv_utils_set_sp(reinterpret_cast<void *>(new_sp));
  }
#endif
  espcontrol_restart_noos_inner();
}
#endif  // ESP32-P4 workaround
#endif  // USE_ESP_IDF
