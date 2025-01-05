#include "texture.hpp"

#include "graphics/stb_image.h"
#include "util/error.hpp"
#include "utils.hpp"

namespace craft::vk {
Texture::Texture(VmaAllocator allocator, Device &device, Renderer *renderer, const char *path)
    : m_allocator{allocator}, m_device{device.GetDevice()} {
  int ch = 4;
  stbi_uc *data = stbi_load(path, &m_width, &m_height, &ch, 4);

  if (ch != 4) {
    RuntimeError::Throw("All textures used in the Vulkan renderer must be RGBA.");
  }

  VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.arrayLayers = 1;
  image_info.mipLevels = 1;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.extent.width = m_width;
  image_info.extent.height = m_height;
  image_info.extent.depth = 1;

  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  VmaAllocationInfo info;
  VK_CHECK(vmaCreateImage(allocator, &image_info, &alloc_info, &m_image, &m_allocation, &info));

  VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.image = m_image;
  view_info.format = image_info.format;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.layerCount = 1;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  VK_CHECK(vkCreateImageView(device.GetDevice(), &view_info, nullptr, &m_view));

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

    TransitionImage(cmd,
                    ImageTransitionBarrier(VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                           VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_image));
    vkCmdCopyBufferToImage(cmd, staging.buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &img_copy);
    // This honestly probably should be VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, but we can't do that on transfer queue.
    TransitionImage(cmd, ImageTransitionBarrier(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                                VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_SHADER_READ_BIT,
                                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_image));
  });

  DestroyBuffer(allocator, std::move(staging));

  VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  sampler_info.magFilter = VK_FILTER_NEAREST;
  sampler_info.minFilter = VK_FILTER_NEAREST;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.anisotropyEnable = VK_TRUE;
  sampler_info.maxAnisotropy = device.GetMaxSamplerAnisotropy();
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

  VK_CHECK(vkCreateSampler(m_device, &sampler_info, nullptr, &m_sampler));

  printf("%f\n", sampler_info.maxAnisotropy);
}

Texture::~Texture() {
  vmaDestroyImage(m_allocator, m_image, m_allocation);
  vkDestroyImageView(m_device, m_view, nullptr);
  vkDestroySampler(m_device, m_sampler, nullptr);
}
} // namespace craft::vk
