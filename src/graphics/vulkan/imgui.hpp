#pragma once

#include <volk.h>

#include <memory>

#include "device.hpp"
#include "platform/window.hpp"
#include "swapchain.hpp"

namespace craft::vk {
class ImGui {
public:
  ImGui() {}
  ImGui(std::shared_ptr<Window> window, Device *device, Swapchain *swapchain);
  ~ImGui();

  ImGui(const ImGui &) = delete;
  ImGui(ImGui &&other) { *this = std::move(other); }

  ImGui &operator=(const ImGui &) = delete;
  ImGui &operator=(ImGui &&other) {
    this->m_instance = other.m_instance;
    this->m_device = other.m_device;
    this->m_pool = other.m_pool;

    other.m_instance = nullptr;
    other.m_device = nullptr;
    other.m_pool = nullptr;

    return *this;
  }

  void Draw(VkCommandBuffer cmd, VkImageView view, VkExtent2D extent) const;

private:
  VkInstance m_instance = nullptr;
  Device *m_device = nullptr;

  VkDescriptorPool m_pool = nullptr;
};
} // namespace craft::vk
