#pragma once

#include <volk.h>

namespace craft::vk {
class Renderer {
public:
  Renderer();

private:
  VkInstance m_instance;
  VkPhysicalDevice m_gpu;
  VkDevice m_device;
};
} // namespace craft::vk
