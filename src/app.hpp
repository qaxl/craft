#pragma once

#include <memory>

#include "graphics/vulkan/renderer.hpp"
#include "graphics/widgets/widget.hpp"
#include "platform/window.hpp"
#include "world/player/player.hpp"

#include <FastNoiseLite/FastNoiseLite.h>

namespace craft {
class App {
  std::shared_ptr<Window> m_window;
  std::shared_ptr<vk::Renderer> m_renderer;
  std::shared_ptr<WidgetManager> m_widget_manager;

  World m_world;
  Player m_player;

  float time_taken_to_render = 0;

public:
  App(int argc, char **argv);
  ~App();

  bool Run();

private:
  void ParseParameters(int argc, char **argv);
};
} // namespace craft
