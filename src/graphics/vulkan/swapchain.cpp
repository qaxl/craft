#include "swapchain.hpp"

#include <volk.h>

#include "util/error.hpp"
#include "utils.hpp"
#include "vulkan/vulkan_core.h"

namespace craft::vk {
Swapchain::Swapchain(Device *device, VkSurfaceKHR surface, VkExtent2D extent)
    : m_device{device}, m_surface{surface}, m_extent{extent} {
  m_present_mode = m_device->GetOptimalPresentMode(m_surface);
  m_surface_format = m_device->GetOptimalSurfaceFormat(m_surface);

  VkSurfaceCapabilitiesKHR caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device->GetPhysicalDevice(), m_surface, &caps);

  m_min_image_count = std::min(std::max(caps.minImageCount + 1, 3U), caps.maxImageCount);
  m_transform = caps.currentTransform;

  CreateSwapchain();
}

Swapchain::~Swapchain() {
  Cleanup();
  vkDestroySwapchainKHR(m_device->GetDevice(), m_swapchain, nullptr);
}

AcquiredImage Swapchain::AcquireNextImage(VkSemaphore wait_semaphore, uint64_t wait_time_ns) {
  VkResult res = vkAcquireNextImageKHR(m_device->GetDevice(), m_swapchain, wait_time_ns, wait_semaphore, nullptr,
                                       &m_current_index);

  if (res == VK_SUBOPTIMAL_KHR || res == VK_SUCCESS) {
    return AcquiredImage{res == VK_SUBOPTIMAL_KHR, m_images[m_current_index], m_views[m_current_index]};
  } else if (res == VK_TIMEOUT) {
    RuntimeError::Throw("Swapchain Image Timeout; Swapchain May Be Frozen?");
  } else if (res == VK_ERROR_OUT_OF_DATE_KHR) {
    return AcquiredImage{true, nullptr, nullptr};
  } else {
    VK_CHECK(res);
    RuntimeError::Unreachable();
  }
}

void Swapchain::Resize(uint32_t width, uint32_t height) {
  Cleanup();
  m_extent = {width, height};
  CreateSwapchain(m_swapchain);
}

void Swapchain::CreateSwapchain(VkSwapchainKHR swapchain) {
  VkSwapchainCreateInfoKHR create_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  create_info.surface = m_surface;

  create_info.presentMode = m_present_mode;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  create_info.imageFormat = m_surface_format.format;
  create_info.imageColorSpace = m_surface_format.colorSpace;
  create_info.imageExtent = m_extent;
  create_info.minImageCount = m_min_image_count;
  create_info.imageArrayLayers = 1;
  create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.preTransform = m_transform;
  create_info.clipped = VK_TRUE;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.oldSwapchain = swapchain;

  VK_CHECK(vkCreateSwapchainKHR(m_device->GetDevice(), &create_info, nullptr, &m_swapchain));

  m_images = GetProperties<VkImage>(vkGetSwapchainImagesKHR, m_device->GetDevice(), m_swapchain);
  m_views.resize(m_images.size());

  for (size_t i = 0; i < m_images.size(); ++i) {
    VkImageViewCreateInfo create_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    create_info.image = m_images[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = m_surface_format.format;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(m_device->GetDevice(), &create_info, nullptr, &m_views[i]));
  }

  if (swapchain) {
    vkDestroySwapchainKHR(m_device->GetDevice(), swapchain, nullptr);
  }
}

void Swapchain::Cleanup() {
  for (auto view : m_views) {
    vkDestroyImageView(m_device->GetDevice(), view, nullptr);
  }
}
} // namespace craft::vk
