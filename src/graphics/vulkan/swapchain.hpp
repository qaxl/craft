#pragma once

#include <vector>
#include <volk.h>

namespace craft::vk {
class Swapchain {
public:
  Swapchain(VkDevice device, VkSurfaceKHR surface, VkExtent2D extent, VkSurfaceTransformFlagBitsKHR current_transform,
            VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR);
  ~Swapchain();

  VkSwapchainKHR GetHandle() { return m_swapchain; }
  // This is used for functions that require arrays in Vulkan, but we only have really one item in the array.
  VkSwapchainKHR *GetHandlePtr() { return &m_swapchain; }

  VkImage *GetImagesPtr() { return m_images.data(); }
  VkImageView *GetViewsPtr() { return m_views.data(); }

  std::pair<VkImage, VkImageView> AcquireNextImage(VkSemaphore wait_semaphore = nullptr,
                                                   uint64_t wait_time_ns = 1 * 1000 * 1000);
  uint32_t GetCurrentImageIndex() { return m_current_index; }

  VkImage GetCurrentImage() { return m_images[m_current_index]; }
  VkImageView GetCurrentView() { return m_views[m_current_index]; }

private:
  VkDevice m_device = nullptr;

  VkSurfaceKHR m_surface = nullptr;
  VkSwapchainKHR m_swapchain = nullptr;

  std::vector<VkImage> m_images;
  std::vector<VkImageView> m_views;

  uint32_t m_current_index = 0;
};
} // namespace craft::vk
