#pragma once

#include "graphics/vulkan/renderer.hpp"
#include "platform/window.hpp"

#include <memory>

namespace craft {

class App {
  std::shared_ptr<Window> m_window;
  std::shared_ptr<vk::Renderer> m_renderer;

public:
  App();
  ~App();

  bool Run();
};
} // namespace craft
