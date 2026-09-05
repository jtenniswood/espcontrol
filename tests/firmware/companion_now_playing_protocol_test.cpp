#include <cassert>
#include <cstdint>
#include "now_playing_protocol.h"

using namespace esphome::companion::protocol;

int main() {
  assert(text_field_valid(256));
  assert(!text_field_valid(257));

  assert(artwork_begin_valid(true, 7, 7, true, 1024, true, true));
  assert(!artwork_begin_valid(false, 7, 7, true, 1024, true, true));
  assert(!artwork_begin_valid(true, 6, 7, true, 1024, true, true));
  assert(!artwork_begin_valid(true, 7, 7, false, 1024, true, true));
  assert(!artwork_begin_valid(true, 7, 7, true, MAX_ARTWORK_BYTES + 1, true, true));

  assert(artwork_chunk_valid(true, 7, 7, 0, 0, MAX_ARTWORK_CHUNK_BYTES, 24000));
  assert(!artwork_chunk_valid(false, 7, 7, 0, 0, 100, 100));
  assert(!artwork_chunk_valid(true, 7, 7, 10, 0, 100, 100));
  assert(!artwork_chunk_valid(true, 8, 7, 0, 0, 100, 100));
  assert(!artwork_chunk_valid(true, 7, 7, 0, 0, MAX_ARTWORK_CHUNK_BYTES + 1, 20000));
  assert(!artwork_chunk_valid(true, 7, 7, 90, 90, 11, 100));

  const uint8_t jpeg[] = {0xff, 0xd8, 1, 2, 0xff, 0xd9};
  assert(jpeg_signature_valid(jpeg, sizeof(jpeg)));
  assert(!jpeg_signature_valid(jpeg + 1, sizeof(jpeg) - 1));
  assert(artwork_complete_valid(true, 7, 7, 100, 100, true, true));
  assert(!artwork_complete_valid(false, 7, 7, 100, 100, true, true));
  assert(!artwork_complete_valid(true, 7, 7, 99, 100, true, true));
  assert(!artwork_complete_valid(true, 7, 7, 100, 100, true, false));
  return 0;
}
