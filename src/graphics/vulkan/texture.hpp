#pragma once

#include <volk.h>
//
#include <vk_mem_alloc.h>

#include "renderer.hpp"

namespace craft::vk {
class Texture {
public:
  Texture(VmaAllocator allocator, Device &device, Renderer *renderer, const char *path);
  ~Texture();

  Texture(Texture const &) = delete;
  Texture(Texture &&) = delete; // TODO: add move constructors

  Texture &operator=(Texture const &) = delete;
  Texture &operator=(Texture &&) = delete;

  FORCE_INLINE VkImageView GetView() { return m_view; }
  FORCE_INLINE VkSampler GetSampler() { return m_sampler; }

private:
  int m_width{};
  int m_height{};

  VmaAllocator m_allocator;
  VkDevice m_device;

  VkImage m_image{};
  VkImageView m_view{};
  VkSampler m_sampler{};
  VmaAllocation m_allocation{};
};
} // namespace craft::vk
