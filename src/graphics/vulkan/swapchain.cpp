#include "swapchain.hpp"
#include "util/error.hpp"
#include "utils.hpp"
#include "vulkan/vulkan_core.h"

#include <utility>
#include <volk.h>

namespace craft::vk {
Swapchain::Swapchain(VkDevice device, VkSurfaceKHR surface, VkExtent2D extent,
                     VkSurfaceTransformFlagBitsKHR current_transform, VkPresentModeKHR present_mode,
                     VkSurfaceFormatKHR surface_format, Swapchain *old_swapchain)
    : m_device{device}, m_surface{surface}, m_transform{current_transform}, m_surface_format{surface_format} {
  VkSwapchainCreateInfoKHR create_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  create_info.surface = surface;
  create_info.presentMode = present_mode;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  create_info.imageFormat = m_surface_format.format;         // VK_FORMAT_B8G8R8A8_UNORM;
  create_info.imageColorSpace = m_surface_format.colorSpace; // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  create_info.imageExtent = extent;
  create_info.minImageCount = 3;
  create_info.imageArrayLayers = 1;
  create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.preTransform = m_transform;
  create_info.clipped = VK_TRUE;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.oldSwapchain = old_swapchain ? old_swapchain->m_swapchain : nullptr;

  VK_CHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &m_swapchain));

  m_images = GetProperties<VkImage>(vkGetSwapchainImagesKHR, device, m_swapchain);
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

    VK_CHECK(vkCreateImageView(device, &create_info, nullptr, &m_views[i]));
  }
}

Swapchain::~Swapchain() {
  for (auto view : m_views) {
    vkDestroyImageView(m_device, view, nullptr);
  }

  if (m_swapchain) {
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    m_swapchain = nullptr;
  }
}

AcquiredImage Swapchain::AcquireNextImage(VkSemaphore wait_semaphore, uint64_t wait_time_ns) {
  VkResult res = vkAcquireNextImageKHR(m_device, m_swapchain, wait_time_ns, wait_semaphore, nullptr, &m_current_index);
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

void Swapchain::Resize(int width, int height) {
  Swapchain swapchain(m_device, m_surface, VkExtent2D{(uint32_t)width, (uint32_t)height}, m_transform, m_present_mode,
                      m_surface_format, this);
  *this = std::move(swapchain);
}
} // namespace craft::vk
