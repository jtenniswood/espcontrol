// LCD identification sequence adapted from Waveshare's Apache-2.0 board support:
// https://github.com/waveshareteam/waveshare_boards/blob/main/boards/esp32_p4_wifi6_touch_lcd_4b/lcd_panel_probe.c
// SPDX-FileCopyrightText: 2026 Waveshare
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/mipi_dsi_ll.h"

namespace esphome::mipi_dsi::waveshare_p4_86 {

enum class Panel { UNKNOWN, ST7703, JD9365DA };

inline constexpr int64_t COMMAND_TIMEOUT_US = 100000;

inline bool send_header(dsi_host_dev_t *host, mipi_dsi_data_type_t type, uint8_t high, uint8_t low, int64_t deadline) {
  while (mipi_dsi_host_ll_gen_is_cmd_fifo_full(host)) {
    if (esp_timer_get_time() >= deadline)
      return false;
    vTaskDelay(1);
  }
  mipi_dsi_host_ll_gen_set_packet_header(host, 0, type, high, low);
  return true;
}

// Both ID reads fit in one FIFO word. Use bounded waits because the ESP-IDF
// DBI read can block indefinitely when the panel does not respond.
inline bool read_id(dsi_host_dev_t *host, uint8_t command, uint8_t length, uint32_t &id) {
  const int64_t deadline = esp_timer_get_time() + COMMAND_TIMEOUT_US;
  while (!mipi_dsi_host_ll_gen_is_read_fifo_empty(host)) {
    if (esp_timer_get_time() >= deadline)
      return false;
    (void) mipi_dsi_host_ll_gen_read_payload_fifo(host);
  }
  if (!send_header(host, MIPI_DSI_DT_SET_MAXIMUM_RETURN_PKT, 0, length, deadline))
    return false;

  mipi_dsi_host_ll_enable_video_mode(host, false);
  mipi_dsi_host_ll_gen_set_rx_vcid(host, 0);
  mipi_dsi_host_ll_enable_bta(host, true);
  bool received = send_header(host, MIPI_DSI_DT_DCS_READ_0, 0, command, deadline);
  if (received) {
    while (mipi_dsi_host_ll_gen_is_read_cmd_busy(host) || mipi_dsi_host_ll_gen_is_read_fifo_empty(host)) {
      if (esp_timer_get_time() >= deadline) {
        received = false;
        break;
      }
      vTaskDelay(1);
    }
    if (received)
      id = mipi_dsi_host_ll_gen_read_payload_fifo(host);
  }
  mipi_dsi_host_ll_enable_bta(host, false);
  return received;
}

inline Panel probe() {
  auto *host = MIPI_DSI_LL_GET_HOST(0);
  uint32_t display_id = 0;
  if (!read_id(host, 0x04, 3, display_id))
    return Panel::UNKNOWN;
  if ((display_id & 0xFFFFFF) == 0x1F2138)
    return Panel::ST7703;

  // Unlock the JD9365DA manufacturer commands and select page 1 for its ID.
  constexpr uint8_t unlock[][2] = {{0xE0, 0x00}, {0xE1, 0x93}, {0xE2, 0x65}, {0xE3, 0xF8}, {0xE0, 0x01}};
  for (const auto &command : unlock) {
    if (!send_header(host, MIPI_DSI_DT_DCS_SHORT_WRITE_1, command[1], command[0],
                     esp_timer_get_time() + COMMAND_TIMEOUT_US))
      return Panel::UNKNOWN;
  }
  uint32_t high = 0;
  uint32_t low = 0;
  if (read_id(host, 0x8D, 1, high) && read_id(host, 0x8E, 1, low) && (high & 0xFF) == 0x93 && (low & 0xFF) == 0x65)
    return Panel::JD9365DA;
  return Panel::UNKNOWN;
}

}  // namespace esphome::mipi_dsi::waveshare_p4_86
