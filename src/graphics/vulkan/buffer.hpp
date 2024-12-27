#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

#include "utils.hpp"

namespace craft::vk {
struct AllocatedBuffer {
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo info;
};

static AllocatedBuffer AllocateBuffer(VmaAllocator allocator, size_t size, VkBufferUsageFlags usage,
                                      VmaMemoryUsage memory_usage) {
  VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = size;
  buffer_info.usage = usage;

  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = memory_usage;
  alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

  AllocatedBuffer buffer;
  VK_CHECK(vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &buffer.buffer, &buffer.allocation, &buffer.info));

  return buffer;
}

static void DestroyBuffer(VmaAllocator allocator, AllocatedBuffer &&buffer) {
  vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}
} // namespace craft::vk
