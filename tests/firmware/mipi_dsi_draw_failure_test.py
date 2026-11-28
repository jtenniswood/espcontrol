#!/usr/bin/env python3
"""Run production display writes with failed submissions and real wait ordering."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
production = (root / "components/mipi_dsi/mipi_dsi.cpp").read_text()
write = re.search(r"^void MipiDsi::write_to_display_\(.*?^\}", production, re.M | re.S)[0]
staged = re.search(r"^size_t MipiDsi::enable_staged_draw\(.*?^#endif", production, re.M | re.S)[0]
source = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
using esp_err_t = int;
constexpr int ESP_OK = 0, portMAX_DELAY = -1;
static int waits = 0, draws = 0, failure_at = 0;
static bool pending = false;
#define ESP_LOGE(...) ((void)0)
#ifdef USE_ESPCONTROL_STAGED_DSI
constexpr int pdTRUE = 1, MALLOC_CAP_SPIRAM = 1, MALLOC_CAP_8BIT = 2;
#define pdMS_TO_TICKS(ms) (ms)
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
static bool force_timeout = false;
static const uint8_t *submitted_pixels = nullptr;
alignas(64) static uint8_t staging[64];
void *heap_caps_aligned_alloc(int alignment, size_t bytes, int caps) {
  assert(alignment == 64 && caps == (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  return bytes <= sizeof(staging) ? staging : nullptr;
}
#endif
int esp_lcd_panel_draw_bitmap(int, int, int, int, int, [[maybe_unused]] const uint8_t *pixels) {
  assert(!pending);  // Never reuse the source before the previous copy finishes.
  ++draws;
  if (draws == failure_at) return -1;
#ifdef USE_ESPCONTROL_STAGED_DSI
  submitted_pixels = pixels;
#endif
  pending = true;
  return ESP_OK;
}
int xSemaphoreTake(void *, int timeout) {
#ifdef USE_ESPCONTROL_STAGED_DSI
  assert(timeout == portMAX_DELAY || timeout == 100);
  if (force_timeout) {
    assert(timeout == 100 && pending);
    return 0;
  }
#else
  assert(timeout == portMAX_DELAY);
#endif
  assert(pending);  // A failed submission will never signal completion.
  pending = false;
  ++waits;
  return 1;
}
struct MipiDsi {
  int handle_ = 0, color_depth_ = 1;
  void *io_lock_ = this;
  void write_to_display_(int, int, int, int, const uint8_t *, int, int, int);
#ifdef USE_ESPCONTROL_STAGED_DSI
  size_t width_ = 8, height_ = 8, staged_capacity_ = 0;
  uint32_t staged_submissions_ = 0;
  uint8_t *staged_pixels_ = nullptr;
  bool staged_pending_ = false, staged_refresh_ = false;
  bool is_failed() const { return false; }
  size_t enable_staged_draw(size_t);
  void set_staged_refresh(bool);
  bool wait_for_staged_draw_();
  bool try_staged_draw_(int, int, int, int, const uint8_t *);
#endif
};
''' + "\n#ifdef USE_ESPCONTROL_STAGED_DSI\n" + staged + "\n" + write + r'''
int main() {
  MipiDsi display;
  uint8_t data[64]{};
  failure_at = 1;
  display.write_to_display_(0, 0, 2, 2, data, 0, 0, 0);
  assert(draws == 1 && waits == 0 && !pending);
  draws = waits = 0;
  failure_at = 2;
  display.write_to_display_(0, 0, 2, 3, data, 0, 0, 1);
  assert(draws == 2 && waits == 1 && !pending);
  draws = waits = failure_at = 0;
  display.write_to_display_(0, 0, 2, 2, data, 0, 0, 0);
  assert(draws == 1 && waits == 1 && !pending);
  draws = waits = 0;
  display.write_to_display_(0, 0, 2, 3, data, 0, 0, 1);
  assert(draws == 3 && waits == 3 && !pending);
#ifdef USE_ESPCONTROL_STAGED_DSI
  assert(display.enable_staged_draw(sizeof(staging) + 1) == 0);
  assert(display.enable_staged_draw(sizeof(staging)) == sizeof(staging));
  data[0] = 12;
  assert(display.try_staged_draw_(0, 0, 2, 2, data));
  assert(pending && submitted_pixels == staging && staging[0] == 12);
  data[0] = 24;
  assert(staging[0] == 12);

  force_timeout = true;
  assert(display.try_staged_draw_(0, 0, 2, 2, data));
  display.write_to_display_(0, 0, 2, 2, data, 0, 0, 0);
  assert(pending && draws == 4 && staging[0] == 12);
  force_timeout = false;
  assert(display.try_staged_draw_(0, 0, 2, 2, data));
  assert(pending && draws == 5 && waits == 4 && staging[0] == 24);
  display.set_staged_refresh(false);
  assert(!pending && waits == 5);

  failure_at = 6;
  assert(display.try_staged_draw_(0, 0, 2, 2, data));
  display.set_staged_refresh(false);
  assert(draws == 6 && waits == 5 && !pending);
  assert(display.staged_submissions_ == 2);
#endif
  std::puts("Display submission failures skip waits; successful copies finish before buffer reuse");
}
'''
with tempfile.TemporaryDirectory(prefix="mipi-dsi-draw-") as directory:
    temp = Path(directory)
    (temp / "test.cpp").write_text(source)
    executable = temp / "test"
    for flags in ([], ["-DUSE_ESPCONTROL_STAGED_DSI"]):
        subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + flags + [
            "-std=c++17", "-Wall", "-Wextra", "-Werror", str(temp / "test.cpp"), "-o", str(executable)
        ], check=True)
        subprocess.run([str(executable)], check=True)
