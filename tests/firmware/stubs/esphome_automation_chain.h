// Automation chaining excerpt from ESPHome 2026.8.2 core/automation.h.
// Copyright (c) 2018 Otto Winter and ESPHome contributors.
// SPDX-License-Identifier: MIT
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// Kept verbatim; cover_art_activation_test.py can verify against a generated build.
#pragma once
#include <cstddef>
#include <tuple>
#include <initializer_list>
#include <utility>
#define ESPHOME_ALWAYS_INLINE __attribute__((always_inline))
namespace esphome {
template<typename... Ts> class ActionList;

template<typename... Ts> class Action {
 public:
  virtual void play_complex(const Ts &...x) {
    this->num_running_++;
    this->play(x...);
    this->play_next_(x...);
  }
  virtual void stop_complex() {
    if (num_running_) {
      this->stop();
      this->num_running_ = 0;
    }
    this->stop_next_();
  }
  /// Check if this or any of the following actions are currently running.
  virtual bool is_running() { return this->num_running_ > 0 || this->is_running_next_(); }

  /// The total number of actions that are currently running in this plus any of
  /// the following actions in the chain.
  int num_running_total() {
    int total = this->num_running_;
    if (this->next_ != nullptr)
      total += this->next_->num_running_total();
    return total;
  }

 protected:
  friend ActionList<Ts...>;
  template<typename... Us> friend class ContinuationAction;

  virtual void play(const Ts &...x) = 0;
  void play_next_(const Ts &...x) {
    if (this->num_running_ > 0) {
      this->num_running_--;
      if (this->next_ != nullptr) {
        this->next_->play_complex(x...);
      }
    }
  }
  template<size_t... S> void play_next_tuple_(const std::tuple<Ts...> &tuple, std::index_sequence<S...> /*unused*/) {
    this->play_next_(std::get<S>(tuple)...);
  }
  void play_next_tuple_(const std::tuple<Ts...> &tuple) {
    this->play_next_tuple_(tuple, std::make_index_sequence<sizeof...(Ts)>{});
  }

  virtual void stop() {}
  void stop_next_() {
    if (this->next_ != nullptr) {
      this->next_->stop_complex();
    }
  }

  bool is_running_next_() {
    if (this->next_ == nullptr)
      return false;
    return this->next_->is_running();
  }

  Action<Ts...> *next_{nullptr};

  /// The number of instances of this sequence in the list of actions
  /// that is currently being executed.
  int num_running_{0};
};

template<typename... Ts> class ActionList {
 public:
  void add_action(Action<Ts...> *action) {
    // Walk to end of chain - action lists are short and only built during setup()
    Action<Ts...> **tail = &this->actions_;
    while (*tail != nullptr)
      tail = &(*tail)->next_;
    *tail = action;
  }
  void add_actions(const std::initializer_list<Action<Ts...> *> &actions) {
    // Find tail once, then append all actions in a single pass
    Action<Ts...> **tail = &this->actions_;
    while (*tail != nullptr)
      tail = &(*tail)->next_;
    for (auto *action : actions) {
      *tail = action;
      tail = &action->next_;
    }
  }
  // Force-inline: part of the Trigger→Automation→ActionList forwarding
  // chain collapsed to reduce automation call stack depth.
  inline void play(const Ts &...x) ESPHOME_ALWAYS_INLINE {
    if (this->actions_ != nullptr)
      this->actions_->play_complex(x...);
  }
  void play_tuple(const std::tuple<Ts...> &tuple) {
    this->play_tuple_(tuple, std::make_index_sequence<sizeof...(Ts)>{});
  }
  void stop() {
    if (this->actions_ != nullptr)
      this->actions_->stop_complex();
  }
  bool empty() const { return this->actions_ == nullptr; }

  /// Check if any action in this action list is currently running.
  bool is_running() {
    if (this->actions_ == nullptr)
      return false;
    return this->actions_->is_running();
  }
  /// Return the number of actions in this action list that are currently running.
  int num_running() {
    if (this->actions_ == nullptr)
      return 0;
    return this->actions_->num_running_total();
  }

 protected:
  template<size_t... S> void play_tuple_(const std::tuple<Ts...> &tuple, std::index_sequence<S...> /*unused*/) {
    this->play(std::get<S>(tuple)...);
  }

  Action<Ts...> *actions_{nullptr};
};


}  // namespace esphome
