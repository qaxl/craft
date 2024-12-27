#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

#include "utils.hpp"
#include "vulkan/vulkan_core.h"

namespace craft::vk {
struct AllocatedImage {
  VkImage image{};
  VkImageView view{};
  VmaAllocation allocation{};
  VkExtent3D extent{};
  VkFormat format{};

  VkDevice device{};
  VmaAllocator allocator{};

  AllocatedImage() {}

  AllocatedImage(VkDevice device, VmaAllocator allocator, VkExtent2D extent) {
    VkExtent3D draw_image_extent{extent.width, extent.height, 1};
    this->format = VK_FORMAT_R16G16B16A16_SFLOAT;
    this->extent = draw_image_extent;

    VkImageUsageFlags draw_image_usage =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo img_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    img_info.format = format;
    img_info.usage = draw_image_usage;
    img_info.extent = draw_image_extent;
    img_info.mipLevels = 1;
    img_info.arrayLayers = 1;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_info.imageType = VK_IMAGE_TYPE_2D;

    VmaAllocationCreateInfo img_alloc{};
    img_alloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    img_alloc.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VK_CHECK(vmaCreateImage(allocator, &img_info, &img_alloc, &image, &allocation, nullptr));

    VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.image = image;
    view_info.format = format;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.layerCount = 1;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &view));
  }

  ~AllocatedImage() {
    if (!device || !view || !allocator || !image || !allocation)
      return;

    vkDestroyImageView(device, view, nullptr);
    vmaDestroyImage(allocator, image, allocation);
  }

  AllocatedImage(const AllocatedImage &) = delete;
  AllocatedImage(AllocatedImage &&other) { *this = std::move(other); }

  AllocatedImage &operator=(const AllocatedImage &) = delete;
  AllocatedImage &operator=(AllocatedImage &&other) {
    this->image = other.image;
    this->view = other.view;
    this->allocation = other.allocation;
    this->extent = other.extent;
    this->format = other.format;

    other.image = nullptr;
    other.view = nullptr;
    other.allocation = nullptr;
    other.device = nullptr;
    other.allocator = nullptr;

    return *this;
  }
};
} // namespace craft::vk
