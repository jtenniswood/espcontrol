#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include "components/captive_portal/dns_server_esp32_idf.h"

std::vector<uint8_t> dns_query(const std::string &hostname) {
  std::vector<uint8_t> packet{0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0, 0, 0, 0, 0, 0};
  size_t start = 0;
  while (start < hostname.size()) {
    size_t end = hostname.find('.', start);
    if (end == std::string::npos) end = hostname.size();
    packet.push_back(end - start);
    packet.insert(packet.end(), hostname.begin() + start, hostname.begin() + end);
    start = end + 1;
  }
  packet.insert(packet.end(), {0, 0, 1, 0, 1});
  return packet;
}

int main() {
  esphome::captive_portal::DNSServer server;
  server.start(esphome::network::IPAddress("192.168.4.1"));
  sockaddr_storage bound{};
  socklen_t length = sizeof(bound);
  assert(getsockname(esphome::socket::last_server_fd, reinterpret_cast<sockaddr *>(&bound), &length) == 0);
  const uint16_t port = bound.ss_family == AF_INET
      ? reinterpret_cast<sockaddr_in *>(&bound)->sin_port : reinterpret_cast<sockaddr_in6 *>(&bound)->sin6_port;
  sockaddr_in destination{};
  destination.sin_family = AF_INET;
  destination.sin_port = port;
  destination.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  const int client = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  assert(client >= 0);
  for (const std::string host : {"captive.apple.com", "connectivitycheck.gstatic.com", "www.msftconnecttest.com"}) {
    const auto question = dns_query(host);
    for (unsigned sections : {0U, 2U, 1U, 3U}) {
      auto query = question;
      if (sections & 1) {
        query[9] = 1;  // One authority NS record, omitted by our response.
        query.insert(query.end(), {0xc0, 0x0c, 0, 2, 0, 1, 0, 0, 0, 0, 0, 2, 0xc0, 0x0c});
      }
      if (sections & 2) {
        query[11] = 1;  // One EDNS OPT record, advertising a 1232-byte payload.
        query.insert(query.end(), {0, 0, 41, 4, 208, 0, 0, 0, 0, 0, 0});
      }
      assert(::sendto(client, query.data(), query.size(), 0, reinterpret_cast<sockaddr *>(&destination),
                      sizeof(destination)) == static_cast<ssize_t>(query.size()));
      server.process_next_request();
      pollfd pending{client, POLLIN, 0};
      assert(::poll(&pending, 1, 200) > 0 && "Portal DNS must answer the OS probe with station IPv6 enabled");
      uint8_t response[192]{};
      const ssize_t count = ::recv(client, response, sizeof(response), 0);
      assert(count == static_cast<ssize_t>(question.size() + 16));
      assert(response[0] == 0x12 && response[1] == 0x34);
      assert(response[2] == 0x84 && response[4] == 0 && response[5] == 1);
      assert(response[6] == 0 && response[7] == 1);
      assert(response[8] == 0 && response[9] == 0 && "Response must not advertise omitted authority records");
      assert(response[10] == 0 && response[11] == 0 && "Response must not advertise an omitted EDNS OPT record");
      assert(std::equal(question.begin() + 12, question.end(), response + 12));
      assert(response[count - 4] == 192 && response[count - 3] == 168 && response[count - 2] == 4 && response[count - 1] == 1);
    }
  }
  const uint8_t invalid[]{0, 1, 2};
  assert(::sendto(client, invalid, sizeof(invalid), 0, reinterpret_cast<sockaddr *>(&destination), sizeof(destination)) == 3);
  server.process_next_request();
  pollfd pending{client, POLLIN, 0};
  assert(::poll(&pending, 1, 50) == 0);
  server.stop();
  ::close(client);
  std::cout << "Captive DNS answered plain/EDNS Apple/Android/Windows A queries with consistent section counts with station IPv6=" << USE_NETWORK_IPV6 << "\n";
}
