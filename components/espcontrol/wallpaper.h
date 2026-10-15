#pragma once
#include <cstddef>
#include <cstdint>
#include <mutex>
#include "wallpaper_format.h"

namespace espcontrol {
// Flash-only service. HTTP and display callers own their respective buffers;
// LVGL is updated exclusively by the application's main loop.
class WallpaperStore {
 public:
  bool available();
  bool save(const uint8_t *data, size_t size);  // size zero removes the wallpaper
  uint8_t *load(size_t &size, uint32_t &revision);
  uint32_t revision();
  WallpaperOpacity opacities();
  bool save_opacities(int off, int active);
  WallpaperBackgroundSetting background();
  bool save_background(bool colour, uint32_t rgb);
  bool save_dimming(int percent);
  uint8_t opacity_percent();
  bool save_opacity_percent(int percent);
 private:
  bool begin_locked();
  bool preferences_locked();
  bool write_primary_locked(const uint8_t *data, size_t size);
  std::mutex mutex_;
  const void *partition_{nullptr};
  int slot_{-1};
  uint32_t revision_{0};
  size_t size_{0};
  WallpaperOpacity opacities_;
  WallpaperBackgroundSetting background_;
  bool preferences_initialized_{false};
  bool initialized_{false};
};
WallpaperStore &wallpaper_store();
}  // namespace espcontrol
