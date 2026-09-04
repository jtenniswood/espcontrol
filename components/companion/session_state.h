#pragma once

#include <atomic>

namespace esphome::companion {

class CompanionSessionState {
 public:
  int authenticated_socket() const { return this->authenticated_socket_.load(); }
  bool connected() const { return this->authenticated_socket() >= 0; }

  int authenticate(int socket_fd) {
    return this->authenticated_socket_.exchange(socket_fd);
  }

  bool disconnect_socket(int socket_fd) {
    int expected = socket_fd;
    return this->authenticated_socket_.compare_exchange_strong(expected, -1);
  }

  int disconnect() { return this->authenticated_socket_.exchange(-1); }

 private:
  std::atomic<int> authenticated_socket_{-1};
};

}  // namespace esphome::companion
