#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

// Prepared lookup over the native SDK's own registration positions. It owns
// no names/callbacks and allocates nothing. Its caller owns the row buffer and
// must keep it alive until reset succeeds outside dispatch.
struct NativeHaSubscriptionHash {
  uint32_t operator()(const char *entity, size_t entity_size,
                      const char *attribute, size_t attribute_size) const {
    uint32_t value = 2166136261u;
    for (size_t i = 0; i < entity_size; ++i)
      value = (value ^ static_cast<unsigned char>(entity[i])) * 16777619u;
    value *= 16777619u;  // Separate entity and attribute, including empty ones.
    for (size_t i = 0; i < attribute_size; ++i)
      value = (value ^ static_cast<unsigned char>(attribute[i])) * 16777619u;
    return value;
  }
};

template<typename Hash = NativeHaSubscriptionHash>
class NativeHaSubscriptionIndex {
 public:
  struct Row { uint32_t hash; uint32_t native_index; };
  static_assert(sizeof(Row) == 8, "Native HA lookup row must stay compact");
  enum class Result { FALLBACK, COMPLETE, INTERRUPTED };

  // Names are borrowed by the SDK. The audited SDK only appends records;
  // project compaction also advances epoch even if size/data stay unchanged.
  template<typename Subscriptions>
  bool prepare(const Subscriptions &subscriptions, uint64_t epoch,
               Row *rows, size_t capacity) {
    if (dispatching()) return false;
    reset();
    if (subscriptions.size() > capacity ||
        subscriptions.size() > std::numeric_limits<uint32_t>::max() ||
        (!subscriptions.empty() && rows == nullptr)) return false;
    for (size_t i = 0; i < subscriptions.size(); ++i) {
      const auto &entry = subscriptions[i];
      if (entry.entity_id == nullptr) return false;
      rows[i] = {hash_(entry.entity_id, std::strlen(entry.entity_id),
                       entry.attribute, entry.attribute ? std::strlen(entry.attribute) : 0),
                 static_cast<uint32_t>(i)};
    }
    if (!subscriptions.empty()) {
      std::sort(rows, rows + subscriptions.size(), [](const Row &a, const Row &b) {
        return a.hash < b.hash || (a.hash == b.hash && a.native_index < b.native_index);
      });
    }
    rows_ = rows;
    native_data_ = subscriptions.data();
    count_ = subscriptions.size();
    epoch_ = epoch;
    ready_ = true;
    return true;
  }

  template<typename Subscriptions>
  bool current(const Subscriptions &subscriptions, uint64_t epoch) const {
    return ready_ && native_data_ == subscriptions.data() &&
           count_ == subscriptions.size() && epoch_ == epoch;
  }

  // Epoch is a reference so a callback that invalidates/replaces the native
  // vector cannot cause subsequent callbacks to use stale positions. An
  // interrupted dispatch must never be retried through the stock path.
  template<typename Subscriptions, typename Text, typename State>
  Result dispatch(const Subscriptions &subscriptions, const uint64_t &epoch,
                  const Text &entity, const Text &attribute, const State &state) {
    if (!current(subscriptions, epoch)) return Result::FALLBACK;
    if (entity.size() == 0 || count_ == 0) return Result::COMPLETE;
    struct Guard {
      size_t &depth;
      explicit Guard(size_t &value) : depth(value) { ++depth; }
      ~Guard() { --depth; }
    } guard(dispatch_depth_);
    const uint32_t key = hash_(entity.c_str(), entity.size(), attribute.c_str(), attribute.size());
    const Row *row = std::lower_bound(rows_, rows_ + count_, key,
        [](const Row &candidate, uint32_t value) { return candidate.hash < value; });
    const Row *end = rows_ + count_;
    while (row != end && row->hash == key) {
      if (!current(subscriptions, epoch)) return Result::INTERRUPTED;
      const auto &entry = subscriptions[(row++)->native_index];
      if (!equal(entity, entry.entity_id)) continue;
      if (entry.attribute ? !equal(attribute, entry.attribute) : attribute.size() != 0) continue;
      entry.callback(state);
    }
    return current(subscriptions, epoch) ? Result::COMPLETE : Result::INTERRUPTED;
  }

  bool dispatching() const { return dispatch_depth_ != 0; }
  bool reset() {
    if (dispatching()) return false;
    ready_ = false;
    rows_ = nullptr;
    native_data_ = nullptr;
    count_ = 0;
    return true;
  }
  size_t bytes() const { return ready_ ? count_ * sizeof(Row) : 0; }

 private:
  template<typename Text>
  static bool equal(const Text &value, const char *expected) {
    return value.size() == std::strlen(expected) &&
           (value.size() == 0 || std::memcmp(value.c_str(), expected, value.size()) == 0);
  }
  Hash hash_;
  Row *rows_{nullptr};
  const void *native_data_{nullptr};
  size_t count_{0}, dispatch_depth_{0};
  uint64_t epoch_{0};
  bool ready_{false};
};
