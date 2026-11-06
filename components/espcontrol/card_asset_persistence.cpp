#include "card_asset_persistence.h"

#ifdef ESP_PLATFORM
#include "esphome/core/preferences.h"

namespace espcontrol {
namespace {
class PreferenceCardAssetPersistence final : public CardAssetPersistence {
 public:
  bool ready() const override { return esphome::global_preferences != nullptr; }
  bool load(PendingDeleteRecord &record) override { return read(PENDING_DELETE_PREFERENCE_KEY, record); }
  bool load(RestoreSessionRecord &record) override { return read(RESTORE_SESSION_PREFERENCE_KEY, record); }
  bool load(CompletedRestoresRecord &record) override { return read(COMPLETED_RESTORES_PREFERENCE_KEY, record); }
  bool save(const PendingDeleteRecord &record) override { return write(PENDING_DELETE_PREFERENCE_KEY, record); }
  bool save(const RestoreSessionRecord &record) override { return write(RESTORE_SESSION_PREFERENCE_KEY, record); }
  bool save(const CompletedRestoresRecord &record) override { return write(COMPLETED_RESTORES_PREFERENCE_KEY, record); }
  bool sync() override { return ready() && esphome::global_preferences->sync(); }

 private:
  template<typename Record> bool read(uint32_t key, Record &record) {
    if (!ready()) return false;
    auto preference = esphome::global_preferences->make_preference<Record>(key, true);
    return preference.load(&record);
  }
  template<typename Record> bool write(uint32_t key, const Record &record) {
    if (!ready()) return false;
    auto preference = esphome::global_preferences->make_preference<Record>(key, true);
    return preference.save(&record);
  }
};
}
CardAssetPersistence *card_asset_persistence() {
  static PreferenceCardAssetPersistence persistence;
  return &persistence;
}
}  // namespace espcontrol
#else
namespace espcontrol {
CardAssetPersistence *card_asset_persistence() { return nullptr; }
}
#endif
