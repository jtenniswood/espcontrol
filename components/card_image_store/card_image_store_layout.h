#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace esphome::card_image_store::layout {

constexpr size_t CARD_IMAGE_DURABLE_REGION_PERCENT = 75;

struct RecordSpan {
  size_t offset{0};
  size_t size{0};
};

inline size_t align_up(size_t value, size_t alignment) {
  if (alignment == 0) return value;
  size_t remainder = value % alignment;
  return remainder == 0 ? value : value + alignment - remainder;
}

inline size_t cache_region_start(size_t capacity, size_t sector_size) {
  if (capacity == 0 || sector_size == 0) return capacity;
  size_t durable_bytes = capacity * CARD_IMAGE_DURABLE_REGION_PERCENT / 100;
  return std::min(capacity, align_up(durable_bytes, sector_size));
}

inline bool span_within_region(const RecordSpan &span, size_t region_start,
                               size_t region_end) {
  return span.offset >= region_start && span.offset <= region_end &&
         span.size <= region_end - span.offset;
}

inline int find_empty_offset(size_t capacity, size_t sector_size,
                             size_t requested_bytes,
                             const std::vector<RecordSpan> &records,
                             size_t region_start = 0,
                             size_t region_end = static_cast<size_t>(-1)) {
  if (capacity == 0 || sector_size == 0 || requested_bytes == 0) return -1;
  region_end = std::min(region_end, capacity);
  region_start = align_up(region_start, sector_size);
  region_end -= region_end % sector_size;
  size_t stored_bytes = align_up(requested_bytes, sector_size);
  if (region_start >= region_end || stored_bytes > region_end - region_start) return -1;

  size_t total_sectors = capacity / sector_size;
  std::vector<uint8_t> used(total_sectors, 0);
  for (const auto &record : records) {
    if (record.size == 0 || record.offset >= capacity) continue;
    size_t first = record.offset / sector_size;
    size_t count = align_up(record.size, sector_size) / sector_size;
    for (size_t i = 0; i < count && first + i < used.size(); i++) {
      used[first + i] = 1;
    }
  }

  size_t first_sector = region_start / sector_size;
  size_t end_sector = region_end / sector_size;
  size_t needed = stored_bytes / sector_size;
  size_t run = 0;
  for (size_t sector = first_sector; sector < end_sector; sector++) {
    run = used[sector] ? 0 : run + 1;
    if (run >= needed) {
      size_t offset = (sector + 1 - needed) * sector_size;
      return offset <= static_cast<size_t>(INT32_MAX) ? static_cast<int>(offset) : -1;
    }
  }
  return -1;
}

}  // namespace esphome::card_image_store::layout
