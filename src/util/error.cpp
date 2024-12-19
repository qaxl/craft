#include "error.hpp"

#include <stacktrace>

#include <winsock2.h>

#include <SDL3/SDL.h>

namespace craft {
static const char *GetWSAErrorString(int error_code) {
  switch (error_code) {
  case WSAEINTR:
    return "Interrupted function call.";
  case WSAEACCES:
    return "Permission denied.";
  case WSAEFAULT:
    return "Bad address.";
  case WSAEINVAL:
    return "Invalid argument.";
  case WSAEMFILE:
    return "Too many open files.";
  case WSAEWOULDBLOCK:
    return "Resource temporarily unavailable.";
  case WSAEINPROGRESS:
    return "Operation now in progress.";
  case WSAEALREADY:
    return "Operation already in progress.";
  case WSAENOTSOCK:
    return "Socket operation on non-socket.";
  case WSAEDESTADDRREQ:
    return "Destination address required.";
  case WSAEMSGSIZE:
    return "Message too long.";
  case WSAEPROTOTYPE:
    return "Protocol wrong type for socket.";
  case WSAENOPROTOOPT:
    return "Bad protocol option.";
  case WSAEPROTONOSUPPORT:
    return "Protocol not supported.";
  case WSAESOCKTNOSUPPORT:
    return "Socket type not supported.";
  case WSAEOPNOTSUPP:
    return "Operation not supported.";
  case WSAEPFNOSUPPORT:
    return "Protocol family not supported.";
  case WSAEAFNOSUPPORT:
    return "Address family not supported by protocol family.";
  case WSAEADDRINUSE:
    return "Address already in use.";
  case WSAEADDRNOTAVAIL:
    return "Cannot assign requested address.";
  case WSAENETDOWN:
    return "Network is down.";
  case WSAENETUNREACH:
    return "Network is unreachable.";
  case WSAENETRESET:
    return "Network dropped connection on reset.";
  case WSAECONNABORTED:
    return "Software caused connection abort.";
  case WSAECONNRESET:
    return "Connection reset by peer.";
  case WSAENOBUFS:
    return "No buffer space available.";
  case WSAEISCONN:
    return "Socket is already connected.";
  case WSAENOTCONN:
    return "Socket is not connected.";
  case WSAESHUTDOWN:
    return "Cannot send after socket shutdown.";
  case WSAETOOMANYREFS:
    return "Too many references: cannot splice.";
  case WSAETIMEDOUT:
    return "Connection timed out.";
  case WSAECONNREFUSED:
    return "Connection refused.";
  case WSAELOOP:
    return "Too many levels of symbolic links.";
  case WSAENAMETOOLONG:
    return "File name too long.";
  case WSAEHOSTDOWN:
    return "Host is down.";
  case WSAEHOSTUNREACH:
    return "No route to host.";
  case WSAENOTEMPTY:
    return "Directory not empty.";
  case WSAEPROCLIM:
    return "Too many processes.";
  case WSAEUSERS:
    return "User quota exceeded.";
  case WSAEDQUOT:
    return "Disk quota exceeded.";
  case WSAESTALE:
    return "Stale file handle reference.";
  case WSAEREMOTE:
    return "Object is remote.";
  case WSASYSNOTREADY:
    return "Network subsystem is unavailable.";
  case WSAVERNOTSUPPORTED:
    return "Winsock.dll version out of range.";
  case WSANOTINITIALISED:
    return "Successful WSAStartup not yet performed.";
  case WSAEDISCON:
    return "Graceful shutdown in progress.";
  case WSAENOMORE:
    return "No more results.";
  case WSAECANCELLED:
    return "Call has been canceled.";
  case WSAEINVALIDPROCTABLE:
    return "Procedure call table is invalid.";
  case WSAEINVALIDPROVIDER:
    return "Service provider is invalid.";
  case WSAEPROVIDERFAILEDINIT:
    return "Service provider failed to initialize.";
  case WSASYSCALLFAILURE:
    return "System call failure.";
  default:
    return "Unknown error.";
  }
}

void RuntimeError::SetErrorString(std::string_view description, ErrorFlags flags) {
  std::lock_guard<std::mutex> lock(s_mutex);

  auto trunc = description.substr(0, s_description.size());
  std::copy(trunc.begin(), trunc.end(), s_description.begin());

  if (flags & EF_Passthrough) {
  } else {
    if (flags & EF_AppendSDLErrors) {
      const char *error = SDL_GetError();
      size_t error_len = strlen(error);

      s_description += "\nSDL error:\n\n";
      s_description.append(std::string_view(error, error_len));
    }

    if (flags & EF_AppendWSAErrors) {
      s_description += "\nWSA (WinSock) error: ";
      s_description += GetWSAErrorString(WSAGetLastError());
    }

    if (flags & EF_DumpStacktrace) {
      auto st = std::stacktrace::current();
      s_description += "\nStacktrace dump:\n\n";

      // This'll be slow, but it doesn't matter since error has occurred and the
      // program won't continue anyway.
      for (auto &fn : st) {
        s_description += std::format("{}:{}@{}\n", fn.source_file(), fn.source_line(), fn.description());
      }
    }
  }

  s_has_an_error.store(true);
}

void RuntimeError::Throw(std::string_view description, ErrorFlags flags) {
  SetErrorString(description, static_cast<ErrorFlags>(flags | EF_Passthrough));
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Craft Engine: A Runtime Error Has Occurred", s_description.c_str(),
                           nullptr);

  // Yes this is simply the same as we could throw this error ourselves...
  // But I instead chose this way so that in the future we *could* just terminate the program instantly if we want to
  // disable exceptions.
  throw std::runtime_error(s_description);
}

} // namespace craft
