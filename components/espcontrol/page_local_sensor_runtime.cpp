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
    for (CardId card = page->first_card; card < page->first_card + page->card_count; ++card)
      if (local(store, *store.card(card)) && !store.field(*store.card(card), CardField::ENTITY).empty())
        return true;
  }
  return false;
}
bool PageLocalSensorDependencies::enumerate(const PageStore& store, PageDependencySink& sink) const {
  for (PageId id = 0; id < store.page_count(); ++id) {
    const auto* page = store.page(id);
    if (!page->navigable) continue;
    for (CardId index = page->first_card; index < page->first_card + page->card_count; ++index) {
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
  const auto count = channels;
  if (count <= SIZE_MAX / (2 * sizeof(Binding))) capacity = count * 2;
}
PageLocalSensorRuntime::Subscriptions::~Subscriptions() {
  cleanup();
}
bool PageLocalSensorRuntime::Subscriptions::initialize() {
  if (bindings) return true;
  if (!capacity) return false;
  bytes = capacity * sizeof(Binding);
  auto* memory = allocator.allocate(bytes);
  if (!memory) {
    bytes = 0;
    return false;
  }
  bindings = static_cast<Binding*>(memory);
  for (size_t i = 0; i < capacity; ++i) new (bindings + i) Binding{};
  peak = std::max(peak, bytes);
  return true;
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
  if (!initialize()) return false;
  for (size_t i = 0; i < capacity; ++i)
    if (!bindings[i].owner) {
      if (source != NO_LOCAL_SENSOR && !provider.watch(source)) return false;
      bindings[i] = {owner, callback, entity, 0, revision, channel, source, text};
      ++live;
      if (source != NO_LOCAL_SENSOR) deliver(bindings[i], provider.read(source), true);
      return true;
    }
  return false;
}
void PageLocalSensorRuntime::Subscriptions::release(void* owner) {
  for (size_t i = 0; i < capacity && bindings; ++i)
    if (bindings[i].owner == owner) {
      bindings[i] = {};
      --live;
    }
}
void PageLocalSensorRuntime::Subscriptions::loop() {
  for (size_t i = 0; i < capacity && bindings; ++i)
    if (bindings[i].owner && bindings[i].source != NO_LOCAL_SENSOR) {
      auto& binding = bindings[i];
      const auto sample = provider.read(binding.source);
      if (sample.revision != binding.seen) deliver(binding, sample, false);
    }
}
bool PageLocalSensorRuntime::Subscriptions::feed(std::string_view key, bool text, float number,
                                                 std::string_view value) {
  bool applied = false;
  for (size_t i = 0; i < capacity && bindings; ++i) {
    auto& binding = bindings[i];
    if (!binding.owner || binding.text != text || std::string_view(binding.key) != key) continue;
    PageLocalSensorSample sample{binding.seen, number, value};
    deliver(binding, sample, false);
    applied = true;
  }
  return applied;
}
void PageLocalSensorRuntime::Subscriptions::cleanup() {
  if (live || !bindings) return;
  for (size_t i = 0; i < capacity; ++i) bindings[i].~Binding();
  allocator.release(bindings, bytes);
  bindings = nullptr;
  bytes = 0;
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
  if (!PageLocalSensorDependencies::has_values(store)) {
    prepared_empty_ = store.generation();
    return true;
  }
  const bool prepared = graph_.prepare(store);
  subscriptions_.cleanup();
  return prepared;
}
void PageLocalSensorRuntime::discard_prepared() {
  prepared_empty_ = 0;
  graph_.discard_prepared();
  subscriptions_.cleanup();
}
void PageLocalSensorRuntime::activate_prepared(const PageStore& store) {
  if (prepared_empty_ && prepared_empty_ == store.generation()) {
    prepared_empty_ = 0;
    graph_.retire_active();
    subscriptions_.cleanup();
  } else {
    // Refresh existing producer handles before publishing the staged snapshot.
    // Reads and packed-cache updates allocate nothing and dispatch no actions.
    subscriptions_.loop();
    graph_.activate_prepared(store);
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
          provider_.fixed_bytes(), provider_.callbacks(), graph_.memory()};
}
}  // namespace espcontrol::pages
