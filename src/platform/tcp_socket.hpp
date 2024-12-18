#pragma once

#include <cstdint>

namespace craft {
class TcpSocket {
public:
  TcpSocket();

  bool Connect(const char *ip, int port);
  void Close();

  size_t Receive(char *buf, size_t size);
  size_t Send(const char *buf, size_t size);

private:
  int64_t m_fd;
};
} // namespace craft
