#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

namespace craft::vk {
struct AllocatedImage {
  VkImage image;
  VkImageView view;
  VmaAllocation allocation;
  VkExtent3D extent;
  VkFormat format;
};
} // namespace craft::vk
