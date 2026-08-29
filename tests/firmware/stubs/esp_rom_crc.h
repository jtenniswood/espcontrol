#pragma once

#include <cstddef>
#include <cstdint>

uint32_t esp_rom_crc32_le(uint32_t seed, const uint8_t *data, uint32_t size);
