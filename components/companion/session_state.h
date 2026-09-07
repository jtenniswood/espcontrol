#pragma once

#include <atomic>
#include <cstdint>

namespace esphome::companion {

class CompanionSessionState {
 public:
  int authenticated_socket() const { return this->authenticated_socket_.load(); }
  bool connected() const { return this->authenticated_socket() >= 0; }

  uint32_t generation() const { return generation_.load(); }
  bool current(uint32_t generation) const { return generation_.load() == generation; }

  int authenticate(int socket_fd) {
    generation_.fetch_add(1);
    return this->authenticated_socket_.exchange(socket_fd);
  }

  bool disconnect_socket(int socket_fd) {
    int expected = socket_fd;
    if (!this->authenticated_socket_.compare_exchange_strong(expected, -1)) return false;
    generation_.fetch_add(1);
    return true;
  }

  int disconnect() { generation_.fetch_add(1); return this->authenticated_socket_.exchange(-1); }

 private:
  std::atomic<uint32_t> generation_{0};
  std::atomic<int> authenticated_socket_{-1};
};

}  // namespace esphome::companion
