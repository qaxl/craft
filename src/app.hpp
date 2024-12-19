#pragma once

#include <memory>

#include "graphics/vulkan/renderer.hpp"
#include "graphics/widgets/widget.hpp"
#include "platform/window.hpp"

namespace craft {

class App {
  std::shared_ptr<Window> m_window;
  std::shared_ptr<vk::Renderer> m_renderer;
  std::shared_ptr<WidgetManager> m_widget_manager;

public:
  App();
  ~App();

  bool Run();
};
} // namespace craft
