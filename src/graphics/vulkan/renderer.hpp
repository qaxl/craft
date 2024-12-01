#pragma once

#include <volk.h>

#include <memory>
#include <vector>

#include "platform/window.hpp"

namespace craft::vk {
struct InstanceExtension {
  // the extension name to enable
  const char *name;
  // whether an extension must be loaded, or only loaded if existing
  bool required = true;
};

class Renderer {
public:
  Renderer(std::shared_ptr<Window> window);
  ~Renderer();

private:
  std::shared_ptr<Window> m_window;

  VkInstance m_instance;
  VkDebugUtilsMessengerEXT m_messenger;

  VkPhysicalDevice m_physical_device;
  VkDevice m_device;
  VkQueue m_queue;

  VkSurfaceKHR m_surface;
  VkSwapchainKHR m_swapchain;

  std::vector<VkImage> m_swapchain_images;
  std::vector<VkImageView> m_swapchain_image_views;
};
} // namespace craft::vk
