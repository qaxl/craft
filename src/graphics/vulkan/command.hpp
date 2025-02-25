#pragma once

#include "utils.hpp"
#include "vulkan/vulkan_core.h"

#include <volk.h>

namespace craft::vk {
class CommandPool {
public:
  CommandPool(VkDevice device, uint32_t queue_family_index = 0) : m_device{device} {
    VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.queueFamilyIndex = queue_family_index;
    VK_CHECK(vkCreateCommandPool(device, &pool_info, nullptr, &m_pool));
  }

  ~CommandPool() { vkDestroyCommandPool(m_device, m_pool, nullptr); }

  VkCommandBuffer AllocateBuffer() const {
    VkCommandBufferAllocateInfo alloc_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = m_pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer buffer;
    VK_CHECK(vkAllocateCommandBuffers(m_device, &alloc_info, &buffer));

    return buffer;
  }

  void Reset() { vkResetCommandPool(m_device, m_pool, 0); }

private:
  VkDevice m_device = nullptr;
  VkCommandPool m_pool = nullptr;
};
} // namespace craft::vk
