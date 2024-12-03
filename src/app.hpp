#pragma once

#include "graphics/vulkan/renderer.hpp"
#include "platform/window.hpp"

#include <memory>
#include <optional>

namespace craft {
struct SharedState {
  std::shared_ptr<Window> window;
  std::shared_ptr<vk::Renderer> renderer;
};

class App {
  // all shared state goes here, and it is a pointer since we cannot initialize
  // *everything* before `App::Init()` is called.
  SharedState m_state;

public:
  App(SharedState &&state) : m_state(std::move(state)) {}

  ~App();

  // Initializes the whole application/program
  static std::optional<App> Init();
  bool Run();
};
} // namespace craft
