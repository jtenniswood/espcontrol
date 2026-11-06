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
source = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
using esp_err_t = int;
constexpr int ESP_OK = 0, portMAX_DELAY = -1;
static int waits = 0, draws = 0, failure_at = 0;
static bool pending = false;
#define ESP_LOGE(...) ((void)0)
int esp_lcd_panel_draw_bitmap(int, int, int, int, int, const uint8_t *) {
  assert(!pending);  // Never reuse the source before the previous copy finishes.
  ++draws;
  if (draws == failure_at) return -1;
  pending = true;
  return ESP_OK;
}
int xSemaphoreTake(int, int timeout) {
  assert(timeout == portMAX_DELAY);
  assert(pending);  // A failed submission will never signal completion.
  pending = false;
  ++waits;
  return 1;
}
struct MipiDsi {
  int handle_ = 0, color_depth_ = 1, io_lock_ = 0;
  void write_to_display_(int, int, int, int, const uint8_t *, int, int, int);
};
''' + write + r'''
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
  std::puts("Display submission failures skip waits; successful copies finish before buffer reuse");
}
'''
with tempfile.TemporaryDirectory(prefix="mipi-dsi-draw-") as directory:
    temp = Path(directory)
    (temp / "test.cpp").write_text(source)
    executable = temp / "test"
    subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + [
        "-std=c++17", "-Wall", "-Wextra", "-Werror", str(temp / "test.cpp"), "-o", str(executable)
    ], check=True)
    subprocess.run([str(executable)], check=True)
