#include "page_local_sensor_runtime.h"

#include <algorithm>
#include <cstring>
#include <new>

namespace espcontrol::pages {
namespace {
constexpr std::string_view NUMBER = "local_number", TEXT = "local_text";
PageEntityBudget local_budget(PageEntityBudget budget, size_t text_bytes) {
  budget.channel_bytes = std::min({budget.channel_bytes, text_bytes, HA_TEXT_SENSOR_STATE_MAX_LEN});
  return budget;
}
}  // namespace
bool PageLocalSensorDependencies::local(const PageStore& store, const CardDefinition& card) {
  const auto type = store.field(card, CardField::TYPE);
  return type == "local_sensor" || (type == "sensor" && store.field(card, CardField::SENSOR) == "local");
}
bool PageLocalSensorDependencies::has_values(const PageStore& store) {
  for (PageId id = 0; id < store.page_count(); ++id) {
    const auto* page = store.page(id);
    if (!page->navigable) continue;
    for (CardId card : page->runtime_cards())
      if (local(store, *store.card(card)) && !store.field(*store.card(card), CardField::ENTITY).empty())
        return true;
  }
  return false;
}
bool PageLocalSensorDependencies::enumerate(const PageStore& store, PageDependencySink& sink) const {
  for (PageId id = 0; id < store.page_count(); ++id) {
    const auto* page = store.page(id);
    if (!page->navigable) continue;
    for (CardId index : page->runtime_cards()) {
      const auto& card = *store.card(index);
      if (!local(store, card)) continue;
      const auto key = store.field(card, CardField::ENTITY);
      if (key.empty()) continue;
      if (!sink.add(index, key, store.field(card, CardField::PRECISION) == "text" ? TEXT : NUMBER))
        return false;
    }
  }
  return true;
}
PageLocalSensorRuntime::Subscriptions::Subscriptions(PageArenaAllocator& allocator,
                                                     PageLocalSensorProvider& provider, size_t channels)
    : allocator(allocator), provider(provider) {
  if (channels <= SIZE_MAX / (2 * sizeof(Binding))) max_channels = channels;
}
PageLocalSensorRuntime::Subscriptions::~Subscriptions() {
  cleanup();
}
bool PageLocalSensorRuntime::Subscriptions::prepare_batch(
    PageEntitySubscriptionBatch&, void* owner, uint32_t, size_t channels, Deliver) {
  if (!owner || channels > max_channels) return false;
  for (const auto& batch : batches)
    if (batch.owner == owner) return false;
  for (auto& batch : batches) {
    if (batch.owner) continue;
    const auto required = channels * sizeof(Binding);
    auto* bindings = required ? static_cast<Binding*>(allocator.allocate(required)) : nullptr;
    if (required && !bindings) return false;
    for (size_t i = 0; i < channels; ++i) new (bindings + i) Binding{};
    batch = {owner, bindings, channels};
    staged_owner = owner;
    bytes += required;
    peak = std::max(peak, bytes);
    return true;
  }
  return false;
}
size_t PageLocalSensorRuntime::Subscriptions::staged_bytes() const {
  for (const auto& batch : batches)
    if (batch.owner && batch.owner == staged_owner) return batch.capacity * sizeof(Binding);
  return 0;
}
void PageLocalSensorRuntime::Subscriptions::deliver(Binding& binding, const PageLocalSensorSample& sample,
                                                    bool initial) {
  binding.seen = sample.revision;
  // The old grid initially keeps "--" for an empty text producer. A later
  // empty report explicitly clears the label, so do not collapse these cases.
  if (binding.text) {
    if (!initial || !sample.text.empty())
      binding.deliver(binding.owner, binding.revision, binding.channel, sample.text);
  } else {
    static_assert(sizeof(float) == 4);
    char bytes[sizeof(float)];
    std::memcpy(bytes, &sample.number, sizeof(float));
    binding.deliver(binding.owner, binding.revision, binding.channel, {bytes, sizeof(bytes)});
  }
}
bool PageLocalSensorRuntime::Subscriptions::subscribe(const char* entity, const char* attribute, void* owner,
                                                      uint32_t revision, uint32_t channel, Deliver callback) {
  if (!owner || !callback || !revision || !entity || !attribute) return false;
  const std::string_view field(attribute);
  if (field != TEXT && field != NUMBER) return false;
  const bool text = field == TEXT;
  const auto source = provider.resolve(entity, text);
  for (auto& batch : batches) {
    if (batch.owner != owner) continue;
    for (size_t i = 0; i < batch.capacity; ++i) {
      auto& binding = batch.bindings[i];
      if (binding.owner) continue;
      if (source != NO_LOCAL_SENSOR && !provider.watch(source)) return false;
      binding = {owner, callback, entity, 0, revision, channel, source, text};
      ++live;
      if (source != NO_LOCAL_SENSOR) deliver(binding, provider.read(source), true);
      return true;
    }
  }
  return false;
}
void PageLocalSensorRuntime::Subscriptions::release(void* owner) {
  for (auto& batch : batches) {
    if (!batch.owner || batch.owner != owner) continue;
    for (size_t i = 0; i < batch.capacity; ++i) {
      if (batch.bindings[i].owner) --live;
      batch.bindings[i].~Binding();
    }
    const auto released = batch.capacity * sizeof(Binding);
    if (batch.bindings) allocator.release(batch.bindings, released);
    bytes -= released;
    if (staged_owner == owner) staged_owner = nullptr;
    batch = {};
  }
}
void PageLocalSensorRuntime::Subscriptions::loop() {
  for (auto& batch : batches)
    for (size_t i = 0; i < batch.capacity; ++i) {
      auto& binding = batch.bindings[i];
      if (!binding.owner || binding.source == NO_LOCAL_SENSOR) continue;
      const auto sample = provider.read(binding.source);
      if (sample.revision != binding.seen) deliver(binding, sample, false);
    }
}
bool PageLocalSensorRuntime::Subscriptions::feed(std::string_view key, bool text, float number,
                                                 std::string_view value) {
  bool applied = false;
  for (auto& batch : batches)
    for (size_t i = 0; i < batch.capacity; ++i) {
      auto& binding = batch.bindings[i];
      if (!binding.owner || binding.text != text || std::string_view(binding.key) != key) continue;
      PageLocalSensorSample sample{binding.seen, number, value};
      deliver(binding, sample, false);
      applied = true;
    }
  return applied;
}
void PageLocalSensorRuntime::Subscriptions::cleanup() {
  if (live) return;
  for (auto& batch : batches)
    if (batch.owner) release(batch.owner);
}
PageLocalSensorRuntime::PageLocalSensorRuntime(PageArenaAllocator& allocator,
                                               PageLocalSensorProvider& provider, PageEntityBudget budget,
                                               size_t text_bytes)
    : provider_(provider),
      subscriptions_(allocator, provider, budget.channels),
      graph_(source_, allocator, subscriptions_, local_budget(budget, text_bytes)),
      text_bytes_(
          std::min({text_bytes, budget.channel_bytes, budget.state_bytes, HA_TEXT_SENSOR_STATE_MAX_LEN})) {}
PageLocalSensorRuntime::~PageLocalSensorRuntime() {
  if (page_local_sensor_feed().owner == this) page_local_sensor_feed() = {};
  discard_prepared();
  retire_active();
}
bool PageLocalSensorRuntime::install_manual_feed(bool legacy_renderer_retired) {
  auto& feed = page_local_sensor_feed();
  if (!legacy_renderer_retired || (feed.owner && feed.owner != this)) return false;
  feed = {this,
          [](void* owner, std::string_view key, float value) {
            return static_cast<PageLocalSensorRuntime*>(owner)->subscriptions_.feed(key, false, value, {});
          },
          [](void* owner, std::string_view key, std::string_view value) {
            return static_cast<PageLocalSensorRuntime*>(owner)->subscriptions_.feed(key, true, {}, value);
          }};
  return true;
}
bool PageLocalSensorRuntime::prepare(const PageStore& store) {
  discard_prepared();
  if(!store.source_document())return false;
  if (!PageLocalSensorDependencies::has_values(store)) {
    prepared_empty_ = store.generation();
    prepared_empty_valid_ = true;
    return true;
  }
  const bool prepared = graph_.prepare(store);
  subscriptions_.cleanup();
  return prepared;
}
void PageLocalSensorRuntime::discard_prepared() {
  prepared_empty_ = 0;
  prepared_empty_valid_ = false;
  graph_.discard_prepared();
  subscriptions_.cleanup();
}
void PageLocalSensorRuntime::activate_prepared(const PageStore& store) {
  if (prepared_empty_valid_ && page_prepared_generation_matches(prepared_empty_, store.generation())) {
    prepared_empty_ = 0;
    prepared_empty_valid_ = false;
    graph_.retire_active();
    subscriptions_.cleanup();
  } else {
    // Refresh existing producer handles before publishing the staged snapshot.
    // Reads and packed-cache updates allocate nothing and dispatch no actions.
    subscriptions_.loop();
    graph_.activate_prepared(store);
    subscriptions_.activate();
  }
  subscriptions_.cleanup();
}
void PageLocalSensorRuntime::retire_active() {
  graph_.retire_active();
  subscriptions_.cleanup();
}
void PageLocalSensorRuntime::loop() {
  subscriptions_.loop();
}
PageEntityValue PageLocalSensorRuntime::read(ConfigurationHandle handle, std::string_view key, bool text) {
  return graph_.read_attribute(handle, key, text ? TEXT : NUMBER);
}
PageLocalSensorMemory PageLocalSensorRuntime::memory() const {
  return {sizeof(*this),           subscriptions_.bytes,  subscriptions_.peak, subscriptions_.live,
          provider_.fixed_bytes(), provider_.callbacks(), graph_.memory(),
          sizeof(PageLocalSensorFeed), subscriptions_.staged_bytes()};
}
}  // namespace espcontrol::pages
