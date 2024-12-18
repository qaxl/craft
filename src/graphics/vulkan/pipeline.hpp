#pragma once

#include <volk.h>

#include <fstream>
#include <optional>
#include <string_view>
#include <vector>

#include "utils.hpp"

namespace craft::vk {
inline std::optional<VkShaderModule> LoadShaderModule(std::string_view path, VkDevice device) {
  std::ifstream file(path.data(), std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    return std::nullopt;
  }

  size_t size = static_cast<size_t>(file.tellg());
  std::vector<uint32_t> buffer(size / sizeof(uint32_t));

  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()), size);
  file.close();

  VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  info.codeSize = size;
  info.pCode = buffer.data();

  VkShaderModule module;
  // TODO: don't make shader module creations a ciritcal failure?
  VK_CHECK(vkCreateShaderModule(device, &info, nullptr, &module));

  return module;
}
} // namespace craft::vk
