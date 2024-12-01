#pragma once

#include <memory>
#include <optional>

namespace craft {
struct SharedState {};

class App {
  // all shared state goes here, and it is a pointer since we cannot initialize
  // *everything* before `App::Init()` is called.
  std::shared_ptr<SharedState> m_state;

public:
  App() = default;
  ~App();

  // Initializes the whole application/program, on error it returns a nullopt.
  static std::optional<App> Init();
  // Gets the latest error string.
  // WARNING: Not thread-safe. Only call from the main thread.
  static std::string_view GetErrorString();

  void Run();
};
} // namespace craft
