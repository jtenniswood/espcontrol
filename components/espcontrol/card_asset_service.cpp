#include "card_asset_service.h"

namespace espcontrol {
namespace {
CardAssetService *active_card_asset_service = nullptr;
}

bool CardAssetService::start() {
  if (running_ || (active_card_asset_service != nullptr && active_card_asset_service != this)) {
    return false;
  }
  active_card_asset_service = this;
  running_ = true;
  return true;
}

bool CardAssetService::stop() {
  if (!running_) return false;
  clear_card_background_runtime();
  if (active_card_asset_service == this) active_card_asset_service = nullptr;
  running_ = false;
  return true;
}

CardAssetService *card_asset_service() { return active_card_asset_service; }

}  // namespace espcontrol
