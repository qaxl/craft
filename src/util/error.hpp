#pragma once

#include <mutex>
#include <string>

namespace craft {
// Basically a locked string_view of the error. Guarantees thread-safety.
class ErrorString {
public:
  operator std::string_view() { return m_view; }
  std::string_view operator*() { return m_view; }

private:
  friend class RuntimeError;

  ErrorString(std::mutex &mtx, std::string_view desc)
      : m_lock(std::lock_guard<std::mutex>(mtx)), m_view(desc) {}

  std::lock_guard<std::mutex> m_lock;
  std::string_view m_view;
};

enum ErrorFlags : uint32_t {
  EF_None,
  EF_AppendSDLErrors = 1 << 0,
  EF_DumpStacktrace = 1 << 1,
};

class RuntimeError {
public:
  static bool HasAnError() { return s_has_an_error.load(std::memory_order_relaxed); }

  static void SetErrorString(std::string_view description, ErrorFlags flags = EF_None);

  static ErrorString GetErrorString() { return ErrorString(s_mutex, s_description); }

private:
  RuntimeError() = default;

  // Yes, this RuntimeError class supports multithreaded erroring :3
  static inline std::mutex s_mutex;
  static inline std::string s_description = std::string(1024, 0);
  static inline std::atomic_bool s_has_an_error = false;
};
} // namespace craft
