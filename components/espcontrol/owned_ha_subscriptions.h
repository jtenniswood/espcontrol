#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// The native API has no subscription-removal operation. Keep callbacks and
// borrowed names alive here until maintenance can remove their native records
// outside API dispatch. Native callbacks contain only a never-reused token.
template<typename Backend, typename Allocator = std::allocator<std::byte>>
class OwnedHaSubscriptions {
 public:
  using State = typename Backend::State;
  using Callback = typename Backend::Callback;
  template<typename T>
  using Rebound = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
  template<typename T>
  using Vector = std::vector<T, Rebound<T>>;

  // Firmware-lifetime dispatch boundary: an old native callback can safely
  // arrive after its coordinator was destroyed. Empty registries retain no
  // records or name buffers; only the monotonically increasing token remains.
  static OwnedHaSubscriptions &instance() {
    static auto *registry = new OwnedHaSubscriptions;
    return *registry;
  }

  uint32_t subscribe(const std::string &entity, const std::string &attribute,
                     Callback callback) {
    if (!Backend::available() || entity.empty() || !callback || next_token_ == 0) return 0;
    Record record;
    record.token = next_token_++;
    record.attribute_offset = entity.size() + 1;
    record.names.reserve(entity.size() + attribute.size() + 2);
    record.names.insert(record.names.end(), entity.begin(), entity.end());
    record.names.push_back('\0');
    record.names.insert(record.names.end(), attribute.begin(), attribute.end());
    record.names.push_back('\0');
    record.callback = std::move(callback);
    const uint32_t token = record.token;
    records_.push_back(std::move(record));
    pending_ = true;
    return token;
  }

  void unsubscribe(uint32_t token) {
    if (Record *record = find(token)) {
      record->callback = nullptr;
      pending_ = true;
    }
  }

  // Return true when the native list changed. This must run from the main
  // component loop, never from a native API callback or a widget destructor.
  bool flush() {
    if (!pending_ || dispatch_depth_ != 0 || !Backend::available()) return false;
    // Identify ownership by the exact borrowed buffer addresses, not by entity
    // text or RTTI. Other components can subscribe to the same pair safely.
    const bool removed = Backend::remove_if([this](const char *entity, const char *attribute, bool once) {
      if (once) return false;
      for (const auto &record : records_) {
        if (record.callback || !record.installed || entity != record.names.data()) continue;
        const char *expected = record.names.data() + record.attribute_offset;
        if (attribute == (*expected ? expected : nullptr)) return true;
      }
      return false;
    });
    // Free names only after the corresponding borrowed native pointers vanish.
    records_.erase(std::remove_if(records_.begin(), records_.end(),
        [](const Record &record) { return !record.callback; }), records_.end());
    bool added = false;
    for (auto &record : records_) {
      if (record.installed) continue;
      const char *attribute = record.names.data() + record.attribute_offset;
      Backend::subscribe(record.names.data(), *attribute ? attribute : nullptr,
                         Callback(NativeCallback{record.token}));
      record.installed = true;
      added = true;
    }
    trim(records_);
    pending_ = false;
    if (removed || added) Backend::reannounce();
    return removed || added;
  }

  size_t size() const {
    return static_cast<size_t>(std::count_if(records_.begin(), records_.end(),
        [](const Record &record) { return !!record.callback; }));
  }
  size_t capacity() const { return records_.capacity(); }
  size_t pending_count() const {
    return static_cast<size_t>(std::count_if(records_.begin(), records_.end(),
        [](const Record &record) { return !record.callback || !record.installed; }));
  }
  size_t storage_bytes() const {
    size_t bytes = records_.capacity() * sizeof(Record);
    for (const auto &record : records_) bytes += record.names.capacity();
    return bytes;
  }

 private:
  struct Record {
    Record() = default;
    Record(const Record &) = delete;
    Record &operator=(const Record &) = delete;
    Record(Record &&) noexcept = default;
    Record &operator=(Record &&) noexcept = default;
    uint32_t token{0};
    Vector<char> names;
    size_t attribute_offset{0};
    Callback callback;
    bool installed{false};
  };
  struct NativeCallback {
    uint32_t token;
    void operator()(State state) const { instance().dispatch(token, state); }
  };

  Record *find(uint32_t token) {
    for (auto &record : records_) if (record.token == token) return &record;
    return nullptr;
  }
  void dispatch(uint32_t token, State state) {
    const auto *record = find(token);
    if (record == nullptr || !record->callback) return;
    // The callback can register or retire channels, moving registry records.
    // Invoke a snapshot instead of an std::function living in that vector.
    Callback callback = record->callback;
    struct Guard {
      uint32_t &depth;
      explicit Guard(uint32_t &value) : depth(value) { ++depth; }
      ~Guard() { --depth; }
    } guard(dispatch_depth_);
    callback(state);
  }
  template<typename T>
  static void trim(Vector<T> &values) {
    if (values.empty()) { Vector<T>().swap(values); return; }
    if (values.capacity() <= std::max(size_t{8}, values.size() * 2)) return;
    Vector<T> replacement;
    replacement.reserve(values.size());
    for (auto &value : values) replacement.push_back(std::move(value));
    replacement.swap(values);
  }

  Vector<Record> records_;
  uint32_t next_token_{1};
  uint32_t dispatch_depth_{0};
  bool pending_{false};
};
