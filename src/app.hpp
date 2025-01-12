#pragma once

#include <memory>

#include "graphics/vulkan/renderer.hpp"
#include "graphics/widgets/widget.hpp"
#include "platform/window.hpp"

#include <FastNoiseLite/FastNoiseLite.h>

namespace craft {

class App {
  std::shared_ptr<Window> m_window;
  std::shared_ptr<vk::Renderer> m_renderer;
  std::shared_ptr<WidgetManager> m_widget_manager;
  std::unique_ptr<Chunk> m_chunk;

  Camera m_camera;
  FastNoiseLite m_noise;
  bool m_regenerate = false;
  bool m_regenerate_with_one_block = false;
  float m_scale_factor = 10.0f;
  float m_max_height = 10.0f;
  BlockType m_current_block_type = BlockType::Air;
  bool m_replace = true;

  float time_taken_to_render = 0;

public:
  App();
  ~App();

  bool Run();
};
} // namespace craft
