#pragma once

#include <vector>

#include <volk.h>

#include "util/optimization.hpp"

namespace craft::vk {
class Swapchain {
public:
  Swapchain() {}
  Swapchain(VkDevice device, VkSurfaceKHR surface, VkExtent2D extent, VkSurfaceTransformFlagBitsKHR current_transform,
            VkPresentModeKHR present_mode = VK_PRESENT_MODE_MAILBOX_KHR, Swapchain *old_swapchain = nullptr);
  ~Swapchain();

  Swapchain(const Swapchain &) = delete;
  Swapchain(Swapchain &&other) { *this = std::move(other); }

  Swapchain &operator=(const Swapchain &) = delete;
  Swapchain &operator=(Swapchain &&other) {
    this->m_device = other.m_device;
    this->m_surface = other.m_surface;
    this->m_swapchain = other.m_swapchain;
    this->m_transform = other.m_transform;
    this->m_images = std::move(other.m_images);
    this->m_views = std::move(other.m_views);
    this->m_current_index = other.m_current_index;
    this->m_present_mode = other.m_present_mode;

    other.m_device = nullptr;
    other.m_surface = nullptr;
    other.m_swapchain = nullptr;
    other.m_current_index = 0;

    return *this;
  }

  FORCE_INLINE VkSwapchainKHR GetHandle() { return m_swapchain; }
  FORCE_INLINE VkSwapchainKHR *GetHandlePtr() { return &m_swapchain; }

  FORCE_INLINE VkImage *GetImagesPtr() { return m_images.data(); }
  FORCE_INLINE VkImageView *GetViewsPtr() { return m_views.data(); }

  std::pair<VkImage, VkImageView> AcquireNextImage(VkSemaphore wait_semaphore = nullptr,
                                                   uint64_t wait_time_ns = 1 * 1000 * 1000);
  void Resize(int width, int height);

  FORCE_INLINE uint32_t GetCurrentImageIndex() { return m_current_index; }

  FORCE_INLINE VkImage GetCurrentImage() { return m_images[m_current_index]; }
  FORCE_INLINE VkImageView GetCurrentView() { return m_views[m_current_index]; }

private:
  VkDevice m_device{};

  VkSurfaceKHR m_surface{};
  VkSwapchainKHR m_swapchain{};

  VkPresentModeKHR m_present_mode{};
  VkSurfaceTransformFlagBitsKHR m_transform{};

  std::vector<VkImage> m_images;
  std::vector<VkImageView> m_views;

  uint32_t m_current_index = 0;
};
} // namespace craft::vk
