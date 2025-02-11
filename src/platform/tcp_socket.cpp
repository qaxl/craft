#include "tcp_socket.hpp"

#include <cstdlib>
#include <string>

#include <WS2tcpip.h>
#include <winsock2.h>
#include <ws2ipdef.h>

#include "util/error.hpp"

namespace craft {
TcpSocket::TcpSocket() {
  // Initialize WSA, required to use sockets on Windows
  static bool run_once = false;
  if (!run_once) {
    WSADATA d;
    WSAStartup(MAKEWORD(2, 2), &d);
    run_once = true;
  }

  m_fd = socket(AF_INET, SOCK_STREAM, 0);
}

bool TcpSocket::Connect(const char *ip, int port) {
  // This is indeed slow way of doing this, but we absolutely do not want this code path breaking due to a C function
  // mis-use.
  std::string port_str = std::to_string(port);

  addrinfo hints{
      .ai_family = AF_INET,
      .ai_socktype = SOCK_STREAM,
      .ai_protocol = IPPROTO_TCP,
  };

  addrinfo *results;
  if (getaddrinfo(ip, port_str.c_str(), &hints, &results) != 0) {
    RuntimeError::Throw("getaddrinfo() failed", EF_AppendWSAErrors);
  }

  if (results == nullptr) {
    return false;
  }

  if (connect(m_fd, results->ai_addr, results->ai_addrlen) != 0) {
    return false;
  }

  return true;
}

void TcpSocket::Close() { closesocket(m_fd); }

size_t TcpSocket::Receive(char *buf, size_t size) { return recv(m_fd, buf, size, 0); }

size_t TcpSocket::Send(const char *buf, size_t size) { return send(m_fd, buf, size, 0); }

} // namespace craft
