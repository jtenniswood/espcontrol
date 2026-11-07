#pragma once

#include "native_ha_subscription_index.h"
#include <new>
#include <utility>

// Main-loop owner for the optional native lookup. Allocator must return null
// on exhaustion. Failure leaves the unchanged SDK dispatcher available.
template<typename Allocator>
class PreparedNativeHaSubscriptionIndex {
 public:
  using Lookup = NativeHaSubscriptionIndex<>;
  using Row = typename Lookup::Row;
  using Result = typename Lookup::Result;
  class Delivery {
   public:
    explicit Delivery(PreparedNativeHaSubscriptionIndex &owner) : owner_(owner) { ++owner_.delivery_depth_; }
    Delivery(const Delivery &) = delete;
    Delivery &operator=(const Delivery &) = delete;
    ~Delivery() { --owner_.delivery_depth_; }
   private:
    PreparedNativeHaSubscriptionIndex &owner_;
  };

  PreparedNativeHaSubscriptionIndex(Allocator allocator, size_t minimum_rows, size_t maximum_rows)
      : allocator_(std::move(allocator)), minimum_rows_(minimum_rows), maximum_rows_(maximum_rows) {}
  PreparedNativeHaSubscriptionIndex(const PreparedNativeHaSubscriptionIndex &) = delete;
  PreparedNativeHaSubscriptionIndex &operator=(const PreparedNativeHaSubscriptionIndex &) = delete;
  ~PreparedNativeHaSubscriptionIndex() { release_rows(); }

  Delivery delivery() { return Delivery(*this); }
  bool delivering() const { return delivery_depth_ != 0; }
  void changed() { if (++epoch_ == 0) ++epoch_; }

  template<typename Subscriptions>
  bool current(const void *source, const Subscriptions &subscriptions) const {
    return source != nullptr && source_ == source && lookup_.current(subscriptions, epoch_);
  }

  template<typename Subscriptions>
  bool maintain(const void *source, const Subscriptions &subscriptions, uint32_t now_ms) {
    if (delivering()) return false;
    if (current(source, subscriptions)) return true;
    const bool same_attempt = attempted_ && attempted_source_ == source &&
        attempted_data_ == subscriptions.data() && attempted_size_ == subscriptions.size() &&
        attempted_epoch_ == epoch_;
    // Exhaustion retries are bounded even when native registrations stay put.
    // Unsigned subtraction also handles the millisecond-clock rollover.
    if (same_attempt && uint32_t(now_ms - attempted_at_) < 1000) return false;
    lookup_.reset();
    source_ = nullptr;
    attempted_ = true;
    attempted_source_ = source;
    attempted_data_ = subscriptions.data();
    attempted_size_ = subscriptions.size();
    attempted_epoch_ = epoch_;
    attempted_at_ = now_ms;
    const size_t count = subscriptions.size();
    if (!source || count == 0 || count < minimum_rows_ || count > maximum_rows_ ||
        count > std::numeric_limits<size_t>::max() / sizeof(Row)) {
      release_rows();
      return false;
    }
    if (capacity_ != count) {
      // The old index is already stale. Release it before resizing so this
      // optimization never requires two full tables at once.
      release_rows();
      rows_ = allocator_.allocate(count);
      if (!rows_) { ++failures_; return false; }
      capacity_ = count;
      for (size_t i = 0; i < count; ++i) ::new (static_cast<void *>(rows_ + i)) Row{};
    }
    if (!lookup_.prepare(subscriptions, epoch_, rows_, capacity_)) {
      ++failures_;
      release_rows();
      return false;
    }
    source_ = source;
    ++rebuilds_;
    attempted_ = false;
    return true;
  }

  template<typename Subscriptions, typename Text, typename State>
  Result dispatch(const void *source, const Subscriptions &subscriptions,
                  const Text &entity, const Text &attribute, const State &state) {
    if (!current(source, subscriptions)) return Result::FALLBACK;
    auto guard = delivery();
    return lookup_.dispatch(subscriptions, epoch_, entity, attribute, state);
  }

  bool reset() {
    if (delivering()) return false;
    release_rows();
    source_ = nullptr;
    attempted_ = false;
    return true;
  }
  size_t bytes() const { return capacity_ * sizeof(Row); }
  size_t capacity() const { return capacity_; }
  uint64_t epoch() const { return epoch_; }
  uint64_t rebuilds() const { return rebuilds_; }
  uint64_t failures() const { return failures_; }

 private:
  void release_rows() {
    lookup_.reset();
    if (rows_) allocator_.deallocate(rows_, capacity_);
    rows_ = nullptr;
    capacity_ = 0;
  }
  Allocator allocator_;
  Lookup lookup_;
  Row *rows_{nullptr};
  const void *source_{nullptr};
  const void *attempted_source_{nullptr}, *attempted_data_{nullptr};
  size_t capacity_{0}, delivery_depth_{0};
  size_t minimum_rows_, maximum_rows_, attempted_size_{0};
  uint64_t epoch_{1}, attempted_epoch_{0}, rebuilds_{0}, failures_{0};
  uint32_t attempted_at_{0};
  bool attempted_{false};
};
