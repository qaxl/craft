#include "renderer.hpp"
#include "graphics/vulkan.hpp"

#include <iostream>
#include <vector>

namespace craft::vk {
Renderer::Renderer() {
  uint32_t ext_count = 0;
  VKCHECK(vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr));

  std::vector<VkExtensionProperties> exts(ext_count);
  VKCHECK(
      vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, exts.data()));

  for (auto ext : exts) {
    std::cout << "Found an extension: " << ext.extensionName << std::endl;
  }
}
} // namespace craft::vk