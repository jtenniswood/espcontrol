#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome::companion::protocol {

static constexpr size_t MAX_ARTWORK_BYTES = 256 * 1024;
static constexpr size_t MAX_ARTWORK_CHUNK_BYTES = 12 * 1024;
static constexpr size_t MAX_TEXT_FIELD_BYTES = 256;

inline bool text_field_valid(size_t utf8_bytes) {
  return utf8_bytes <= MAX_TEXT_FIELD_BYTES;
}

inline bool artwork_begin_valid(bool authenticated, uint32_t generation,
                                uint32_t current_generation, bool artwork_expected,
                                size_t byte_length, bool jpeg_mime, bool sha256_valid) {
  return authenticated && generation != 0 && generation == current_generation &&
         artwork_expected && byte_length >= 12 && byte_length <= MAX_ARTWORK_BYTES &&
         jpeg_mime && sha256_valid;
}

inline bool artwork_chunk_valid(bool authenticated, uint32_t generation,
                                uint32_t transfer_generation, size_t offset,
                                size_t expected_offset, size_t chunk_size,
                                size_t declared_length) {
  return authenticated && generation != 0 && generation == transfer_generation &&
         offset == expected_offset && chunk_size > 0 &&
         chunk_size <= MAX_ARTWORK_CHUNK_BYTES && expected_offset <= declared_length &&
         chunk_size <= declared_length - expected_offset;
}

inline bool artwork_complete_valid(bool authenticated, uint32_t generation,
                                   uint32_t transfer_generation, size_t received,
                                   size_t declared_length, bool jpeg_signature,
                                   bool sha256_matches) {
  return authenticated && generation != 0 && generation == transfer_generation &&
         received == declared_length && jpeg_signature && sha256_matches;
}

inline bool jpeg_signature_valid(const uint8_t *data, size_t size) {
  return data && size >= 4 && data[0] == 0xff && data[1] == 0xd8 &&
         data[size - 2] == 0xff && data[size - 1] == 0xd9;
}

}  // namespace esphome::companion::protocol
