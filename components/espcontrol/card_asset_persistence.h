#pragma once

#include <cstdint>
#include "../card_image_store/card_image_store.h"

namespace espcontrol {

// Preserve the deployed preference keys, record layouts and checksums.
constexpr uint32_t PENDING_DELETE_MAGIC = 0x43414444;
constexpr uint8_t PENDING_DELETE_VERSION = 1;
constexpr uint32_t PENDING_DELETE_PREFERENCE_KEY = 0x5D19A62C;
constexpr uint32_t RESTORE_SESSION_MAGIC = 0x43415253;
constexpr uint8_t RESTORE_SESSION_VERSION = 1;
constexpr uint32_t RESTORE_SESSION_PREFERENCE_KEY = 0x5D19A62D;
constexpr uint8_t RESTORE_SESSION_COMMITTED_FLAG = 0x80;
constexpr size_t MAX_STAGED_RESTORE_ASSETS =
    esphome::card_image_store::CARD_IMAGE_INDEX_MAX_RECORDS;
static_assert(MAX_STAGED_RESTORE_ASSETS <= 0x7F);

struct PendingDeleteRecord {
  uint32_t magic{PENDING_DELETE_MAGIC};
  uint8_t version{PENDING_DELETE_VERSION};
  char id[41]{};
  uint32_t checksum{0};
};

struct RestoreSessionRecord {
  uint32_t magic{RESTORE_SESSION_MAGIC};
  uint8_t version{RESTORE_SESSION_VERSION};
  uint8_t count{0};
  char session[17]{};
  char ids[MAX_STAGED_RESTORE_ASSETS][41]{};
  uint32_t checksum{0};
};

static_assert(sizeof(PendingDeleteRecord) == 52, "Preserve deployed delete journal layout");
static_assert(offsetof(RestoreSessionRecord, session) == 6 &&
              offsetof(RestoreSessionRecord, ids) == 23,
              "Preserve deployed restore journal layout");

constexpr uint32_t COMPLETED_RESTORES_PREFERENCE_KEY = 0x5D19A62E;
constexpr uint32_t COMPLETED_RESTORES_MAGIC = 0x43415243;
constexpr size_t COMPLETED_RESTORE_CAPACITY = 4;
struct CompletedRestoresRecord {
  uint32_t magic{COMPLETED_RESTORES_MAGIC};
  char sessions[COMPLETED_RESTORE_CAPACITY][17]{};
  uint32_t checksum{0};
};

// Only the platform adapter knows about ESPHome preferences. Protocol decisions,
// encoding and explicit durability boundaries also run in host tests.
class CardAssetPersistence {
 public:
  virtual ~CardAssetPersistence() = default;
  virtual bool ready() const = 0;
  virtual bool load(PendingDeleteRecord &record) = 0;
  virtual bool load(RestoreSessionRecord &record) = 0;
  virtual bool load(CompletedRestoresRecord &record) = 0;
  virtual bool save(const PendingDeleteRecord &record) = 0;
  virtual bool save(const RestoreSessionRecord &record) = 0;
  virtual bool save(const CompletedRestoresRecord &record) = 0;
  virtual bool sync() = 0;
};

CardAssetPersistence *card_asset_persistence();

}  // namespace espcontrol
