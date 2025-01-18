#include "texture.hpp"

#include <stb/stb_image.h>

#include "util/error.hpp"
#include "utils.hpp"
#include "vulkan/vulkan_core.h"

namespace craft::vk {
void Texture::GenerateMipmaps(Device &device, Renderer *renderer, VmaAllocator allocator) {
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(device.GetPhysicalDevice(), VK_FORMAT_R8G8B8A8_SRGB, &formatProperties);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    RuntimeError::Throw("Texture image format does not support linear blitting!");
  }

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
      !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
    RuntimeError::Throw("Selected image format does not support blit source and destination");
  }

  AllocatedBuffer staging =
      AllocateBuffer(allocator, m_width * m_height * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
  uint8_t *data = new uint8_t[m_width * m_height * 4];
  memset(data, 0xFF, m_width * m_height * 4);

  void *mapped_data;
  VK_CHECK(vmaMapMemory(allocator, staging.allocation, &mapped_data));

  memcpy(mapped_data, data, m_width * m_height * 4);

  vmaUnmapMemory(allocator, staging.allocation);
  free(data);

  renderer->SubmitNow([&](VkCommandBuffer cmd) {
    for (uint32_t i = 1; i < m_mip_levels; i++) {
      VkImageBlit blit{};
      blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.srcSubresource.layerCount = 1;
      blit.srcSubresource.mipLevel = i - 1;
      blit.srcOffsets[1].x = std::max(1, m_width >> (i - 1));
      blit.srcOffsets[1].y = std::max(1, m_height >> (i - 1));
      blit.srcOffsets[1].z = 1;

      blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.dstSubresource.layerCount = 1;
      blit.dstSubresource.mipLevel = i;
      blit.dstOffsets[1].x = m_width >> i;
      blit.dstOffsets[1].y = m_height >> i;
      blit.dstOffsets[1].z = 1;

      TransitionImage(cmd, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, VK_REMAINING_ARRAY_LAYERS},
                      VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                      VK_ACCESS_2_TRANSFER_WRITE_BIT);

      vkCmdBlitImage(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     1, &blit, VK_FILTER_LINEAR);

      // VkBufferImageCopy copy{};
      // copy.imageExtent.width = std::max(1, m_width >> i);
      // copy.imageExtent.height = std::max(1, m_height >> i);
      // copy.imageExtent.depth = 1;
      // copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      // copy.imageSubresource.layerCount = 1;
      // copy.imageSubresource.mipLevel = i;

      // vkCmdCopyBufferToImage(cmd, staging.buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

      TransitionImage(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, VK_REMAINING_ARRAY_LAYERS},
                      VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                      VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
    }

    TransitionImage(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, m_mip_levels, 0, VK_REMAINING_ARRAY_LAYERS},
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
  });
}

Texture::Texture(VmaAllocator allocator, Device &device, Renderer *renderer, const char *path)
    : m_allocator{allocator}, m_device{device.GetDevice()} {
  int ch = 4;
  stbi_uc *data = stbi_load(path, &m_width, &m_height, &ch, 4);

  if (ch != 4) {
    RuntimeError::Throw("All textures used in the Vulkan renderer must be RGBA.");
  }

  m_mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1;
  if (m_mip_levels > 5) {
    m_mip_levels = 5;
  }

  VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.arrayLayers = 1;
  image_info.mipLevels = m_mip_levels;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.extent.width = m_width;
  image_info.extent.height = m_height;
  image_info.extent.depth = 1;

  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  VmaAllocationInfo info;
  VK_CHECK(vmaCreateImage(allocator, &image_info, &alloc_info, &m_image, &m_allocation, &info));

  AllocatedBuffer staging =
      AllocateBuffer(allocator, m_width * m_height * ch, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

  void *mapped_data;
  VK_CHECK(vmaMapMemory(allocator, staging.allocation, &mapped_data));

  memcpy(mapped_data, data, m_width * m_height * ch);

  vmaUnmapMemory(allocator, staging.allocation);

  renderer->SubmitNow([&](VkCommandBuffer cmd) {
    VkBufferImageCopy img_copy{};
    img_copy.imageExtent.width = m_width;
    img_copy.imageExtent.height = m_height;
    img_copy.imageExtent.depth = 1;
    img_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_copy.imageSubresource.layerCount = 1;
    img_copy.imageSubresource.mipLevel = 0;

    TransitionImage(cmd, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, VK_REMAINING_ARRAY_LAYERS},
                    VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    VK_ACCESS_2_TRANSFER_WRITE_BIT);

    vkCmdCopyBufferToImage(cmd, staging.buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &img_copy);

    TransitionImage(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, VK_REMAINING_ARRAY_LAYERS},
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    VK_ACCESS_2_TRANSFER_READ_BIT);
  });

  DestroyBuffer(allocator, std::move(staging));

  // FIXME:
  GenerateMipmaps(device, renderer, allocator);

  VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.image = m_image;
  view_info.format = image_info.format;
  view_info.subresourceRange.levelCount = m_mip_levels;
  view_info.subresourceRange.layerCount = 1;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  VK_CHECK(vkCreateImageView(device.GetDevice(), &view_info, nullptr, &m_view));

  VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  sampler_info.magFilter = VK_FILTER_NEAREST;
  sampler_info.minFilter = VK_FILTER_NEAREST;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.maxLod = static_cast<float>(m_mip_levels);

  VK_CHECK(vkCreateSampler(m_device, &sampler_info, nullptr, &m_sampler));
}

Texture::~Texture() {
  vmaDestroyImage(m_allocator, m_image, m_allocation);
  vkDestroyImageView(m_device, m_view, nullptr);
  vkDestroySampler(m_device, m_sampler, nullptr);
}
} // namespace craft::vk
