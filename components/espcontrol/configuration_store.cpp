#include "configuration_store.h"

#include <algorithm>
#include <array>
#include <climits>

namespace espcontrol::configuration {
namespace {

// Little-endian bytes spell "ECFG" on storage.
constexpr uint32_t ENVELOPE_MAGIC = 0x47464345;
constexpr size_t MAGIC_OFFSET = 0;
constexpr size_t VERSION_OFFSET = 4;
constexpr size_t HEADER_SIZE_OFFSET = 6;
constexpr size_t GENERATION_OFFSET = 8;
constexpr size_t PAYLOAD_SIZE_OFFSET = 12;
constexpr size_t CHECKSUM_OFFSET = 16;
constexpr size_t CHECKSUM_CHUNK_SIZE = 64;

uint16_t read_u16(const uint8_t *data) {
  return static_cast<uint16_t>(data[0]) |
         (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t read_u32(const uint8_t *data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

void write_u16(uint8_t *data, uint16_t value) {
  data[0] = static_cast<uint8_t>(value & 0xFF);
  data[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void write_u32(uint8_t *data, uint32_t value) {
  data[0] = static_cast<uint8_t>(value & 0xFF);
  data[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  data[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  data[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

uint32_t checksum_update(uint32_t crc, const uint8_t *data, size_t size) {
  for (size_t index = 0; index < size; ++index) {
    crc ^= data[index];
    for (uint8_t bit = 0; bit < CHAR_BIT; ++bit) {
      const uint32_t mask = 0U - (crc & 1U);
      crc = (crc >> 1) ^ (0xEDB88320U & mask);
    }
  }
  return crc;
}

}  // namespace

size_t ConfigurationStore::maximum_payload_size() const {
  const size_t capacity = backend_.slot_capacity();
  return capacity > CONFIGURATION_ENVELOPE_HEADER_SIZE
             ? capacity - CONFIGURATION_ENVELOPE_HEADER_SIZE
             : 0;
}

uint32_t ConfigurationStore::checksum(uint32_t generation,
                                      const uint8_t *data, size_t size) {
  std::array<uint8_t, CHECKSUM_OFFSET - VERSION_OFFSET> metadata{};
  write_u16(metadata.data(), CONFIGURATION_ENVELOPE_VERSION);
  write_u16(metadata.data() + HEADER_SIZE_OFFSET - VERSION_OFFSET,
            CONFIGURATION_ENVELOPE_HEADER_SIZE);
  write_u32(metadata.data() + GENERATION_OFFSET - VERSION_OFFSET, generation);
  write_u32(metadata.data() + PAYLOAD_SIZE_OFFSET - VERSION_OFFSET,
            static_cast<uint32_t>(size));
  uint32_t crc =
      checksum_update(0xFFFFFFFFU, metadata.data(), metadata.size());
  return ~checksum_update(crc, data, size);
}

bool ConfigurationStore::generation_is_newer(uint32_t candidate,
                                             uint32_t reference) {
  return static_cast<int32_t>(candidate - reference) > 0;
}

bool ConfigurationStore::checksum_slot_payload(uint8_t slot,
                                               const uint8_t *header,
                                               size_t payload_size,
                                               uint32_t *result) {
  std::array<uint8_t, CHECKSUM_CHUNK_SIZE> buffer{};
  uint32_t crc = checksum_update(
      0xFFFFFFFFU, header + VERSION_OFFSET,
      CHECKSUM_OFFSET - VERSION_OFFSET);
  size_t offset = 0;
  while (offset < payload_size) {
    const size_t chunk = std::min(buffer.size(), payload_size - offset);
    if (!backend_.read(slot, CONFIGURATION_ENVELOPE_HEADER_SIZE + offset,
                       buffer.data(), chunk)) {
      return false;
    }
    crc = checksum_update(crc, buffer.data(), chunk);
    offset += chunk;
  }
  *result = ~crc;
  return true;
}

ConfigurationStore::SlotMetadata ConfigurationStore::inspect_slot(
    uint8_t slot) {
  SlotMetadata metadata;
  metadata.slot = slot;
  if (slot >= CONFIGURATION_SLOT_COUNT ||
      backend_.slot_capacity() < CONFIGURATION_ENVELOPE_HEADER_SIZE) {
    return metadata;
  }

  std::array<uint8_t, CONFIGURATION_ENVELOPE_HEADER_SIZE> header{};
  if (!backend_.read(slot, 0, header.data(), header.size())) {
    metadata.state = SlotState::IO_ERROR;
    return metadata;
  }

  if (read_u32(header.data() + MAGIC_OFFSET) != ENVELOPE_MAGIC ||
      read_u16(header.data() + VERSION_OFFSET) !=
          CONFIGURATION_ENVELOPE_VERSION ||
      read_u16(header.data() + HEADER_SIZE_OFFSET) !=
          CONFIGURATION_ENVELOPE_HEADER_SIZE) {
    return metadata;
  }

  metadata.generation = read_u32(header.data() + GENERATION_OFFSET);
  metadata.payload_size = read_u32(header.data() + PAYLOAD_SIZE_OFFSET);
  metadata.checksum = read_u32(header.data() + CHECKSUM_OFFSET);
  if (metadata.payload_size > maximum_payload_size()) return metadata;

  uint32_t actual_checksum = 0;
  if (!checksum_slot_payload(slot, header.data(), metadata.payload_size,
                             &actual_checksum)) {
    metadata.state = SlotState::IO_ERROR;
    return metadata;
  }
  if (actual_checksum != metadata.checksum) return metadata;

  metadata.state = SlotState::VALID;
  return metadata;
}

LoadResult ConfigurationStore::inspect() {
  const SlotMetadata first = inspect_slot(0);
  const SlotMetadata second = inspect_slot(1);

  const SlotMetadata *selected = nullptr;
  if (first.state == SlotState::VALID && second.state == SlotState::VALID) {
    selected = generation_is_newer(second.generation, first.generation)
                   ? &second
                   : &first;
  } else if (first.state == SlotState::VALID) {
    selected = &first;
  } else if (second.state == SlotState::VALID) {
    selected = &second;
  }

  if (selected == nullptr) {
    const bool read_failed = first.state == SlotState::IO_ERROR ||
                             second.state == SlotState::IO_ERROR;
    return {read_failed ? StoreStatus::READ_FAILED : StoreStatus::EMPTY};
  }

  return {StoreStatus::OK, selected->generation, selected->payload_size,
          selected->slot};
}

LoadResult ConfigurationStore::load(uint8_t *output,
                                    size_t output_capacity) {
  const LoadResult selected = inspect();
  if (!selected.ok()) return selected;

  if (selected.payload_size > output_capacity) {
    return {StoreStatus::BUFFER_TOO_SMALL, selected.generation,
            selected.payload_size, selected.slot};
  }
  if (selected.payload_size > 0 && output == nullptr) {
    return {StoreStatus::INVALID_ARGUMENT, selected.generation,
            selected.payload_size, selected.slot};
  }
  if (selected.payload_size > 0 &&
      !backend_.read(selected.slot, CONFIGURATION_ENVELOPE_HEADER_SIZE,
                     output, selected.payload_size)) {
    return {StoreStatus::READ_FAILED, selected.generation,
            selected.payload_size, selected.slot};
  }

  return selected;
}

CommitResult ConfigurationStore::commit(const uint8_t *payload,
                                        size_t payload_size) {
  if (payload_size > 0 && payload == nullptr) {
    return {StoreStatus::INVALID_ARGUMENT};
  }
  if (payload_size > maximum_payload_size()) {
    return {StoreStatus::PAYLOAD_TOO_LARGE, 0, payload_size};
  }

  const SlotMetadata first = inspect_slot(0);
  const SlotMetadata second = inspect_slot(1);
  const bool first_valid = first.state == SlotState::VALID;
  const bool second_valid = second.state == SlotState::VALID;
  if ((!first_valid && first.state == SlotState::IO_ERROR) ||
      (!second_valid && second.state == SlotState::IO_ERROR)) {
    return {StoreStatus::READ_FAILED};
  }

  uint8_t target_slot = 0;
  uint32_t next_generation = 1;
  if (first_valid && second_valid) {
    const bool second_newer =
        generation_is_newer(second.generation, first.generation);
    const SlotMetadata &newest = second_newer ? second : first;
    target_slot = second_newer ? first.slot : second.slot;
    next_generation = newest.generation + 1;
  } else if (first_valid) {
    target_slot = second.slot;
    next_generation = first.generation + 1;
  } else if (second_valid) {
    target_slot = first.slot;
    next_generation = second.generation + 1;
  }

  // Keep the target unpublished until both its payload and metadata are
  // durable. A torn metadata write cannot accidentally promote stale size or
  // checksum bytes because the magic marker is restored in a final write.
  std::array<uint8_t, sizeof(uint32_t)> invalid_magic{};
  if (!backend_.write(target_slot, MAGIC_OFFSET, invalid_magic.data(),
                      invalid_magic.size())) {
    return {StoreStatus::WRITE_FAILED, next_generation, payload_size,
            target_slot};
  }
  if (!backend_.sync()) {
    return {StoreStatus::SYNC_FAILED, next_generation, payload_size,
            target_slot};
  }

  if (payload_size > 0 &&
      !backend_.write(target_slot, CONFIGURATION_ENVELOPE_HEADER_SIZE,
                      payload, payload_size)) {
    return {StoreStatus::WRITE_FAILED, next_generation, payload_size,
            target_slot};
  }
  if (!backend_.sync()) {
    return {StoreStatus::SYNC_FAILED, next_generation, payload_size,
            target_slot};
  }

  std::array<uint8_t, CONFIGURATION_ENVELOPE_HEADER_SIZE> header{};
  write_u32(header.data() + MAGIC_OFFSET, ENVELOPE_MAGIC);
  write_u16(header.data() + VERSION_OFFSET,
            CONFIGURATION_ENVELOPE_VERSION);
  write_u16(header.data() + HEADER_SIZE_OFFSET,
            CONFIGURATION_ENVELOPE_HEADER_SIZE);
  write_u32(header.data() + GENERATION_OFFSET, next_generation);
  write_u32(header.data() + PAYLOAD_SIZE_OFFSET,
            static_cast<uint32_t>(payload_size));
  write_u32(header.data() + CHECKSUM_OFFSET,
            checksum(next_generation, payload, payload_size));

  if (!backend_.write(target_slot, VERSION_OFFSET,
                      header.data() + VERSION_OFFSET,
                      header.size() - VERSION_OFFSET)) {
    return {StoreStatus::WRITE_FAILED, next_generation, payload_size,
            target_slot};
  }
  if (!backend_.sync()) {
    return {StoreStatus::SYNC_FAILED, next_generation, payload_size,
            target_slot};
  }

  if (!backend_.write(target_slot, MAGIC_OFFSET, header.data() + MAGIC_OFFSET,
                      invalid_magic.size())) {
    return {StoreStatus::WRITE_FAILED, next_generation, payload_size,
            target_slot};
  }
  if (!backend_.sync()) {
    // Some backends expose written bytes before sync confirms durability. If
    // publication cannot be confirmed, withdraw the marker and best-effort
    // persist that rollback so load() continues to select the previous slot.
    if (backend_.write(target_slot, MAGIC_OFFSET, invalid_magic.data(),
                       invalid_magic.size())) {
      backend_.sync();
    }
    return {StoreStatus::SYNC_FAILED, next_generation, payload_size,
            target_slot};
  }

  const SlotMetadata verified = inspect_slot(target_slot);
  if (verified.state != SlotState::VALID ||
      verified.generation != next_generation ||
      verified.payload_size != payload_size ||
      verified.checksum != checksum(next_generation, payload, payload_size)) {
    return {StoreStatus::VERIFY_FAILED, next_generation, payload_size,
            target_slot};
  }

  return {StoreStatus::OK, next_generation, payload_size, target_slot};
}

}  // namespace espcontrol::configuration
