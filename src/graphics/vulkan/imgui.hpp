#pragma once

#include "platform/window.hpp"

#include <volk.h>

#include <memory>

namespace craft::vk {
class ImGui {
public:
  ImGui() {}
  ImGui(VkInstance instance, VkPhysicalDevice gpu, VkDevice device, std::shared_ptr<Window> window, VkQueue queue,
        uint32_t queue_family);
  ~ImGui();

  ImGui(ImGui &) = delete;

  void Draw(VkCommandBuffer cmd, VkImageView view) const;

private:
  VkInstance m_instance = nullptr;
  VkDevice m_device = nullptr;

  VkDescriptorPool m_pool = nullptr;
};
} // namespace craft::vk
