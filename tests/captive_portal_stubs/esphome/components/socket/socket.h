#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <memory>
#include <cassert>
#include <cstring>

namespace esphome::socket {
inline int last_server_fd = -1;
class Socket {
 public:
  explicit Socket(int domain, int type, int protocol) : fd_(::socket(domain, type, protocol)) {
    assert(this->fd_ >= 0);
    last_server_fd = this->fd_;
  }
  ~Socket() { ::close(this->fd_); }
  int get_fd() const { return this->fd_; }
  int setsockopt(int level, int option, const void *value, socklen_t size) {
    return ::setsockopt(this->fd_, level, option, value, size);
  }
  int bind(const sockaddr *address, socklen_t size) {
    // Keep production binding and packet handling; substitute an ephemeral test
    // port so the real UDP exchange requires no privileged port or device.
    sockaddr_storage local{};
    std::memcpy(&local, address, size);
    if (address->sa_family == AF_INET) {
      auto *v4 = reinterpret_cast<sockaddr_in *>(&local);
      assert(ntohs(v4->sin_port) == 53);
      v4->sin_port = 0;
    } else {
      auto *v6 = reinterpret_cast<sockaddr_in6 *>(&local);
      assert(ntohs(v6->sin6_port) == 53);
      v6->sin6_port = 0;
    }
    return ::bind(this->fd_, reinterpret_cast<sockaddr *>(&local), size);
  }
  bool ready() const { pollfd fd{this->fd_, POLLIN, 0}; return ::poll(&fd, 1, 100) > 0; }
  ssize_t sendto(const void *data, size_t size, int flags, const sockaddr *address, socklen_t length) {
    return ::sendto(this->fd_, data, size, flags, address, length);
  }
 private:
  int fd_;
};
using ListenSocket = Socket;
inline std::unique_ptr<ListenSocket> socket_listen_loop_monitored(int domain, int type, int protocol) {
  return std::make_unique<Socket>(domain, type, protocol);
}
// Retain the upstream generic factory's IPv6 behavior so running the imported
// unpatched DNS source against these stubs reproduces the address-family fault.
inline std::unique_ptr<ListenSocket> socket_ip_loop_monitored(int type, int protocol) {
  return socket_listen_loop_monitored(USE_NETWORK_IPV6 ? AF_INET6 : AF_INET, type, protocol);
}
inline socklen_t set_sockaddr_any(sockaddr *address, socklen_t, uint16_t port) {
  if (USE_NETWORK_IPV6) {
    auto *v6 = reinterpret_cast<sockaddr_in6 *>(address);
    v6->sin6_family = AF_INET6;
    v6->sin6_port = htons(port);
    return sizeof(*v6);
  }
  auto *v4 = reinterpret_cast<sockaddr_in *>(address);
  v4->sin_family = AF_INET;
  v4->sin_port = htons(port);
  return sizeof(*v4);
}
}
