#pragma once

#include <volk.h>

#include "primitives.hpp"

namespace craft::vk {
class Instance;

class Renderer {
public:
  Renderer();

private:
  Instance m_instance;
  VkPhysicalDevice m_gpu;
  VkDevice m_device;
};
} // namespace craft::vk
