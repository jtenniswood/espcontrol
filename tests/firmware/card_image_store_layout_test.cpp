#include "card_image_store_layout.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

using esphome::card_image_store::layout::RecordSpan;
using esphome::card_image_store::layout::cache_region_start;
using esphome::card_image_store::layout::find_empty_offset;
using esphome::card_image_store::layout::span_within_region;

constexpr size_t SECTOR = 4096;

void expect(bool condition, const char *message) {
  if (condition) return;
  std::cerr << message << '\n';
  std::exit(1);
}

}  // namespace

int main() {
  constexpr size_t capacity = 16 * SECTOR;
  const size_t cache_start = cache_region_start(capacity, SECTOR);
  expect(cache_start == 12 * SECTOR, "cache region should occupy the final quarter");

  std::vector<RecordSpan> no_records;
  expect(find_empty_offset(capacity, SECTOR, SECTOR, no_records) == 0,
         "durable records should begin at the start of storage");
  expect(find_empty_offset(capacity, SECTOR, SECTOR, no_records, cache_start, capacity) ==
             static_cast<int>(cache_start),
         "cache records should begin only inside the cache region");

  std::vector<RecordSpan> cache_records = {
      {12 * SECTOR, SECTOR},
      {14 * SECTOR, SECTOR},
  };
  expect(find_empty_offset(capacity, SECTOR, 2 * SECTOR, cache_records,
                           cache_start, capacity) == -1,
         "fragmented cache space should reject a record that cannot fit contiguously");
  expect(find_empty_offset(capacity, SECTOR, 2 * SECTOR, cache_records) == 0,
         "cache fragmentation must not consume the durable reservation");

  std::vector<RecordSpan> durable_records = {{0, 12 * SECTOR}};
  expect(find_empty_offset(capacity, SECTOR, 2 * SECTOR, durable_records) ==
             static_cast<int>(12 * SECTOR),
         "durable records may use spare cache-region capacity");
  expect(find_empty_offset(capacity, SECTOR, 2 * SECTOR, durable_records,
                           cache_start, capacity) == static_cast<int>(12 * SECTOR),
         "cache records may use the cache region when it is not occupied by originals");

  RecordSpan inside{cache_start, SECTOR};
  RecordSpan before{cache_start - SECTOR, SECTOR};
  RecordSpan crossing{cache_start, 5 * SECTOR};
  expect(span_within_region(inside, cache_start, capacity),
         "a cache fully inside its arena should be retained");
  expect(!span_within_region(before, cache_start, capacity),
         "a legacy cache in durable space should be discarded");
  expect(!span_within_region(crossing, cache_start, capacity),
         "a cache crossing the arena boundary should be discarded");

  expect(find_empty_offset(capacity, SECTOR, 0, no_records) == -1,
         "zero-sized records should be rejected");
  expect(find_empty_offset(capacity, SECTOR, 5 * SECTOR, no_records,
                           cache_start, capacity) == -1,
         "records larger than the cache arena should be rejected");

  std::cout << "Card image store layout tests passed.\n";
  return 0;
}
