#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "button_grid_contract_generated.h"

namespace espcontrol::cards {

// A card's address remains stable across configuration revisions. Subpage
// cards include the owning main-grid slot so equal child positions on two
// subpages cannot collide.
enum class CardSurface : uint8_t {
  MAIN_GRID,
  SUBPAGE,
};

struct CardAddress {
  CardSurface surface = CardSurface::MAIN_GRID;
  uint16_t parent_slot = 0;
  uint16_t slot = 0;

  constexpr bool operator==(const CardAddress &other) const {
    return surface == other.surface && parent_slot == other.parent_slot &&
           slot == other.slot;
  }
};

enum CardChangeDomain : uint8_t {
  CHANGE_NONE = 0,
  CHANGE_IDENTITY = 1u << 0,
  CHANGE_RESOURCES = 1u << 1,
  CHANGE_BINDINGS = 1u << 2,
  CHANGE_VISUAL = 1u << 3,
  CHANGE_LAYOUT = 1u << 4,
};

enum class CardMutation : uint8_t {
  NONE,
  ADD,
  REMOVE,
  REPLACE,
  REBIND,
  UPDATE_VISUAL,
  UPDATE_LAYOUT,
};

struct CardNode {
  CardAddress address;
  card_runtime::CardTypeId type = card_runtime::CardTypeId::UNKNOWN;
  card_runtime::CardDriverId driver = card_runtime::CardDriverId::UNKNOWN;
  uint64_t resource_signature = 0;
  uint64_t binding_signature = 0;
  uint64_t visual_signature = 0;
  uint64_t layout_signature = 0;
};

struct CardOperation {
  CardAddress address;
  CardMutation mutation = CardMutation::NONE;
  uint8_t changed_domains = CHANGE_NONE;
  const CardNode *before = nullptr;
  const CardNode *after = nullptr;

  constexpr bool changed(CardChangeDomain domain) const {
    return (changed_domains & static_cast<uint8_t>(domain)) != 0;
  }
};

// FNV-1a gives adapters a small deterministic way to fingerprint parsed card
// fields without retaining their source strings in the graph.
constexpr uint64_t card_signature(std::string_view value) {
  uint64_t hash = 14695981039346656037ULL;
  for (const char character : value) {
    hash ^= static_cast<uint8_t>(character);
    hash *= 1099511628211ULL;
  }
  return hash;
}

constexpr uint64_t combine_card_signatures(uint64_t current, uint64_t next) {
  current ^= next + 0x9e3779b97f4a7c15ULL + (current << 6) + (current >> 2);
  return current;
}

template <std::size_t MaxCards>
class FixedCardGraph {
 public:
  static constexpr std::size_t capacity() { return MaxCards; }

  constexpr std::size_t size() const { return size_; }
  constexpr bool empty() const { return size_ == 0; }

  const CardNode *find(const CardAddress &address) const {
    for (std::size_t index = 0; index < size_; ++index) {
      if (nodes_[index].address == address) return &nodes_[index];
    }
    return nullptr;
  }

  const CardNode &operator[](std::size_t index) const { return nodes_[index]; }

  // Graph construction is deliberately strict: duplicate addresses and an
  // over-capacity configuration are rejected before live UI state is touched.
  bool add(const CardNode &node) {
    if (size_ >= MaxCards || find(node.address) != nullptr) return false;
    nodes_[size_++] = node;
    return true;
  }

  void clear() { size_ = 0; }

 private:
  std::array<CardNode, MaxCards> nodes_{};
  std::size_t size_ = 0;
};

template <std::size_t MaxOperations>
class FixedCardPlan {
 public:
  static constexpr std::size_t capacity() { return MaxOperations; }

  constexpr std::size_t size() const { return size_; }
  constexpr bool empty() const { return size_ == 0; }
  constexpr bool valid() const { return valid_; }

  const CardOperation &operator[](std::size_t index) const {
    return operations_[index];
  }

  bool add(const CardOperation &operation) {
    if (size_ >= MaxOperations) {
      valid_ = false;
      return false;
    }
    operations_[size_++] = operation;
    return true;
  }

 private:
  std::array<CardOperation, MaxOperations> operations_{};
  std::size_t size_ = 0;
  bool valid_ = true;
};

inline uint8_t changed_domains(const CardNode &before, const CardNode &after) {
  uint8_t domains = CHANGE_NONE;
  if (before.type != after.type || before.driver != after.driver) {
    domains |= CHANGE_IDENTITY;
  }
  if (before.resource_signature != after.resource_signature) {
    domains |= CHANGE_RESOURCES;
  }
  if (before.binding_signature != after.binding_signature) {
    domains |= CHANGE_BINDINGS;
  }
  if (before.visual_signature != after.visual_signature) {
    domains |= CHANGE_VISUAL;
  }
  if (before.layout_signature != after.layout_signature) {
    domains |= CHANGE_LAYOUT;
  }
  return domains;
}

inline CardMutation mutation_for(uint8_t domains) {
  if ((domains & (CHANGE_IDENTITY | CHANGE_RESOURCES)) != 0) {
    return CardMutation::REPLACE;
  }
  if ((domains & CHANGE_BINDINGS) != 0) return CardMutation::REBIND;
  if ((domains & CHANGE_VISUAL) != 0) return CardMutation::UPDATE_VISUAL;
  if ((domains & CHANGE_LAYOUT) != 0) return CardMutation::UPDATE_LAYOUT;
  return CardMutation::NONE;
}

// Planning is pure and allocation-free. The returned pointers refer to the two
// input graphs, which must remain alive while the plan is being applied.
template <std::size_t MaxCards>
FixedCardPlan<MaxCards> reconcile_cards(const FixedCardGraph<MaxCards> &before,
                                        const FixedCardGraph<MaxCards> &after) {
  FixedCardPlan<MaxCards> plan;

  // Tear down removed cards before acquiring resources for additions.
  for (std::size_t index = 0; index < before.size(); ++index) {
    const CardNode &old_node = before[index];
    if (after.find(old_node.address) == nullptr) {
      plan.add({old_node.address, CardMutation::REMOVE,
                static_cast<uint8_t>(CHANGE_IDENTITY | CHANGE_RESOURCES |
                                     CHANGE_BINDINGS | CHANGE_VISUAL |
                                     CHANGE_LAYOUT),
                &old_node, nullptr});
    }
  }

  for (std::size_t index = 0; index < after.size(); ++index) {
    const CardNode &new_node = after[index];
    const CardNode *old_node = before.find(new_node.address);
    if (old_node == nullptr) continue;
    const uint8_t domains = changed_domains(*old_node, new_node);
    const CardMutation mutation = mutation_for(domains);
    if (mutation != CardMutation::NONE) {
      plan.add({new_node.address, mutation, domains, old_node, &new_node});
    }
  }

  for (std::size_t index = 0; index < after.size(); ++index) {
    const CardNode &new_node = after[index];
    if (before.find(new_node.address) == nullptr) {
      plan.add({new_node.address, CardMutation::ADD,
                static_cast<uint8_t>(CHANGE_IDENTITY | CHANGE_RESOURCES |
                                     CHANGE_BINDINGS | CHANGE_VISUAL |
                                     CHANGE_LAYOUT),
                nullptr, &new_node});
    }
  }

  return plan;
}

}  // namespace espcontrol::cards
