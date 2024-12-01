#pragma once

#include <volk.h>

#include <memory>

#include "platform/window.hpp"
#include "primitives.hpp"

namespace craft::vk {
class Instance;

class Renderer {
public:
  Renderer(std::shared_ptr<Window> window);

private:
  std::shared_ptr<Window> m_window;

  Instance m_instance;
  VkPhysicalDevice m_gpu;
  VkDevice m_device;
};
} // namespace craft::vk
