#pragma once

#include <array>
#include <initializer_list>
#include <memory>
#include <string_view>

#include <vector>
#include <volk.h>

#include "platform/window.hpp"

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
  Instance(std::shared_ptr<Window> window, std::initializer_list<InstanceExtension> extensions,
           std::initializer_list<const char *> layers = {}, std::string_view app_name = "craft app");
  ~Instance();

  Device SelectPhysicalDevice();

private:
  std::shared_ptr<Window> m_window;

  VkInstance m_instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT m_messenger = VK_NULL_HANDLE;
};

class Device {
public:
  Device(VkPhysicalDevice device, std::vector<const char *> &&exts);
  ~Device();

  static consteval std::array<const char *, 1> GetExtensions() { return {"VK_KHR_swapchain"}; }
  static consteval std::array<const char *, 0> GetOptionalExtensions() { return {}; }

private:
  VkDevice m_device;
  // TODO: multiple queues
  VkQueue m_queue;
};
} // namespace craft::vk
