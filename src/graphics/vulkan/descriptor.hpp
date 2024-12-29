#pragma once

#include "utils.hpp"

#include <span>
#include <vector>

namespace craft::vk {
struct DescriptorLayoutBuilder {
  std::vector<VkDescriptorSetLayoutBinding> bindings;

  void AddBinding(uint32_t binding, VkDescriptorType type) {
    bindings.emplace_back(
        VkDescriptorSetLayoutBinding{.binding = binding, .descriptorType = type, .descriptorCount = 1});
  }

  void Clear() { bindings.clear(); }

  VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaders, void *next = nullptr,
                              VkDescriptorSetLayoutCreateFlags flags = 0) {
    for (auto &b : bindings) {
      b.stageFlags |= shaders;
    }

    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext = next;

    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings = bindings.data();
    info.flags = flags;

    VkDescriptorSetLayout set_layout;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set_layout));

    return set_layout;
  }
};

struct DescriptorAllocator {
  struct PoolSizeRatio {
    VkDescriptorType type;
    float ratio;
  };

  VkDescriptorPool pool;

  DescriptorAllocator() {}

  DescriptorAllocator(const DescriptorAllocator &) = delete;
  DescriptorAllocator(DescriptorAllocator &&) = delete;

  DescriptorAllocator &operator=(const DescriptorAllocator &) = delete;
  DescriptorAllocator &operator=(DescriptorAllocator &&) = delete;

  void InitPool(VkDevice device, uint32_t size, std::span<PoolSizeRatio> ratios) {
    std::vector<VkDescriptorPoolSize> sizes;
    sizes.reserve(ratios.size());
    for (PoolSizeRatio ratio : ratios) {
      sizes.emplace_back(VkDescriptorPoolSize{ratio.type, static_cast<uint32_t>(ratio.ratio * size)});
    }

    VkDescriptorPoolCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    info.maxSets = size;
    info.poolSizeCount = static_cast<uint32_t>(sizes.size());
    info.pPoolSizes = sizes.data();

    VK_CHECK(vkCreateDescriptorPool(device, &info, nullptr, &pool));
  }

  void ClearDescriptors(VkDevice device) { vkResetDescriptorPool(device, pool, 0); }

  void DestroyPool(VkDevice device) { vkDestroyDescriptorPool(device, pool, nullptr); }

  VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout) {
    VkDescriptorSetAllocateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    info.descriptorPool = pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    VkDescriptorSet ds;
    VK_CHECK(vkAllocateDescriptorSets(device, &info, &ds));

    return ds;
  }
};
} // namespace craft::vk
