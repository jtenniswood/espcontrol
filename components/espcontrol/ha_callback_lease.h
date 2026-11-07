#pragma once

#include <cstddef>
#include <memory>
#include <utility>

// Shared callback lifetime for the single main-loop HA coordinator. Copies
// keep a callback alive through reentrant cancellation; they are not safe to
// pass between threads. std::shared_ptr uses a mutex per control block on the
// P4 toolchain, consuming internal RAM even with an external-RAM allocator.
template <typename Callback, typename Allocator> class HaCallbackLease {
private:
  struct Block;
  using BlockAllocator =
      typename std::allocator_traits<Allocator>::template rebind_alloc<Block>;
  using Traits = std::allocator_traits<BlockAllocator>;

  struct Block {
    Block(BlockAllocator allocator, Callback callback)
        : allocator(std::move(allocator)), callback(std::move(callback)) {}
    size_t references = 1;
    BlockAllocator allocator;
    Callback callback;
  };

  struct AllocationDeleter {
    BlockAllocator &allocator;
    void operator()(Block *block) const {
      Traits::deallocate(allocator, block, 1);
    }
  };

public:
  HaCallbackLease() noexcept = default;
  HaCallbackLease(const HaCallbackLease &other) noexcept
      : block_(other.block_) {
    if (block_)
      ++block_->references;
  }
  HaCallbackLease(HaCallbackLease &&other) noexcept
      : block_(std::exchange(other.block_, nullptr)) {}
  HaCallbackLease &operator=(HaCallbackLease other) noexcept {
    std::swap(block_, other.block_);
    return *this;
  }
  ~HaCallbackLease() {
    if (!block_ || --block_->references != 0)
      return;
    // Keep the allocator state after destroying its containing block.
    BlockAllocator allocator(block_->allocator);
    Traits::destroy(allocator, block_);
    Traits::deallocate(allocator, block_, 1);
  }

  static HaCallbackLease create(Callback callback,
                                Allocator allocator = Allocator()) {
    BlockAllocator rebound(allocator);
    std::unique_ptr<Block, AllocationDeleter> allocation(
        Traits::allocate(rebound, 1), AllocationDeleter{rebound});
    Traits::construct(rebound, allocation.get(), rebound, std::move(callback));
    HaCallbackLease lease;
    lease.block_ = allocation.release();
    return lease;
  }

  explicit operator bool() const noexcept { return block_ != nullptr; }
  Callback &operator*() const noexcept { return block_->callback; }
  bool operator==(const HaCallbackLease &other) const noexcept {
    return block_ == other.block_;
  }
  bool operator!=(const HaCallbackLease &other) const noexcept {
    return !(*this == other);
  }

private:
  Block *block_ = nullptr;
};
