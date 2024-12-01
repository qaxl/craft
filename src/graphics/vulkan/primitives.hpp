#pragma once

#include <initializer_list>
#include <string_view>

#include <volk.h>

namespace craft::vk {
struct InstanceExtension {
  // the extension name to enable
  const char *name;
  // whether an extension must be loaded, or only loaded if existing
  bool required = true;
};

class Device;

class Instance {
public:
  Instance(std::initializer_list<InstanceExtension> extensions, std::initializer_list<const char *> layers = {},
           std::string_view app_name = "craft app");
  ~Instance();

  Device SelectPhysicalDevice();

private:
  VkInstance m_instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT m_messenger = VK_NULL_HANDLE;
};

class Device {
public:
  Device(VkPhysicalDevice device);

private:
  VkDevice m_device;
};
} // namespace craft::vk
