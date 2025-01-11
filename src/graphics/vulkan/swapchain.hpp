#pragma once

#include <vector>

#include <volk.h>

#include "device.hpp"
#include "util/optimization.hpp"

namespace craft::vk {
struct AcquiredImage {
  bool should_resize{false};

  VkImage image{};
  VkImageView view{};
};

class Swapchain {
public:
  Swapchain(Device *device, VkSurfaceKHR surface, VkExtent2D extent);
  ~Swapchain();

  Swapchain(const Swapchain &) = delete;
  Swapchain(Swapchain &&other) = delete;

  Swapchain &operator=(const Swapchain &) = delete;
  Swapchain &operator=(Swapchain &&other) = delete;

  FORCE_INLINE VkSwapchainKHR GetHandle() { return m_swapchain; }
  FORCE_INLINE VkSwapchainKHR *GetHandlePtr() { return &m_swapchain; }

  FORCE_INLINE VkImage *GetImagesPtr() { return m_images.data(); }
  FORCE_INLINE VkImageView *GetViewsPtr() { return m_views.data(); }

  FORCE_INLINE VkImage GetCurrentImage() { return m_images[m_current_index]; }
  FORCE_INLINE VkImageView GetCurrentView() { return m_views[m_current_index]; }
  FORCE_INLINE uint32_t GetCurrentImageIndex() { return m_current_index; }
  FORCE_INLINE uint32_t GetImageCount() { return m_images.size(); }

  AcquiredImage AcquireNextImage(VkSemaphore wait_semaphore = nullptr, uint64_t wait_time_ns = 1 * 1000 * 1000);

  void Resize(uint32_t width, uint32_t height);

private:
  void CreateSwapchain(VkSwapchainKHR swapchain = nullptr);
  void Cleanup();

private:
  Device *m_device{};

  VkSurfaceKHR m_surface{};
  VkSwapchainKHR m_swapchain{};

  VkExtent2D m_extent{};
  VkPresentModeKHR m_present_mode{};
  VkSurfaceFormatKHR m_surface_format{};
  VkSurfaceTransformFlagBitsKHR m_transform{};

  std::vector<VkImage> m_images;
  std::vector<VkImageView> m_views;

  uint32_t m_current_index = 0;
  uint32_t m_min_image_count = 3;
};
} // namespace craft::vk
