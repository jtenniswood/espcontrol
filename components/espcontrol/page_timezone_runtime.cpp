#include "page_timezone_runtime.h"

#include <cstring>

#include "timezone_offset.h"

namespace espcontrol::pages {
PageTimezoneRuntime::~PageTimezoneRuntime() {
  discard_prepared();
  retire_active();
}
bool PageTimezoneRuntime::supported(const PageStore& store, CardId id) {
  const auto* card = store.runtime_card(id);
  if (!card) return false;
  const auto type = store.field(*card, CardField::TYPE);
  return type == "clock" || type == "timezone";
}
bool PageTimezoneRuntime::prepare(const PageStore& store) {
  discard_prepared();
  if (!resolver_.resolve || !store.source_document()) {
    ++failures_;
    return false;
  }
  staged_.store = store;
  for (PageId p = 0; p < store.page_count(); ++p) {
    const auto* page = store.page(p);
    if (!page->navigable) continue;
    for (CardId id = page->first_card; id < page->first_card + page->runtime_card_count; ++id)
      if (supported(store, id)) ++staged_.cards;
  }
  return true;
}
void PageTimezoneRuntime::discard_prepared() {
  staged_ = {};
}
void PageTimezoneRuntime::activate_prepared(const PageStore& store) {
  if (!staged_.store.source_document() ||
      !page_prepared_generation_matches(staged_.store.generation(), store.generation()))
    return;
  active_ = staged_;
  active_.store = store;
  staged_ = {};
  ++revision_;
}
void PageTimezoneRuntime::retire_active() {
  if (active_.store.source_document()) {
    active_ = {};
    ++revision_;
  }
}
bool PageTimezoneRuntime::update(bool valid, time_t epoch, std::string_view option, bool use_12h) {
  // The selector contract is bounded; refuse a non-selector producer without truncation.
  if (option.size() >= option_.size()) {
    ++rejected_;
    return false;
  }
  if (!option.empty()) std::memmove(option_.data(), option.data(), option.size());
  option_size_ = option.size();
  option_[option_size_] = '\0';
  valid_ = valid;
  epoch_ = epoch;
  use_12h_ = use_12h;
  // Even an identical Auto event can reflect a changed global HA timezone.
  ++revision_;
  return true;
}
bool PageTimezoneRuntime::seed(bool valid, time_t epoch, std::string_view option, bool use_12h) {
  return !active_.store.source_document() && !staged_.store.source_document() &&
         update(valid, epoch, option, use_12h);
}
PageTimezoneSnapshot PageTimezoneRuntime::read(ConfigurationHandle handle) const {
  PageTimezoneSnapshot result;
  if (!active_.store.source_document() || handle.generation != active_.store.generation() ||
      !supported(active_.store, handle.index))
    return result;
  auto option = active_.store.field(*active_.store.card(handle.index), CardField::ENTITY);
  if (option.empty()) option = {option_.data(), option_size_};
  const auto selected = resolver_.resolve(resolver_.context, option, epoch_);
  result.zone_id = selected.id;
  result.use_12h = use_12h_;
  if (!valid_) return result;
  int minutes = 0;
  if (!timezone_posix_offset_minutes(selected.posix, epoch_, minutes)) return result;
  const time_t local_epoch = epoch_ + static_cast<time_t>(minutes) * 60;
  struct tm local;
  if (!gmtime_r(&local_epoch, &local)) return result;
  result.valid = true;
  result.hour = local.tm_hour;
  result.minute = local.tm_min;
  return result;
}
PageTimezoneMemory PageTimezoneRuntime::memory() const {
  return {sizeof(*this), active_.cards, staged_.cards, failures_, rejected_};
}
}  // namespace espcontrol::pages
