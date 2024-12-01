#include "primitives.hpp"
#include "graphics/vulkan.hpp"
#include "util/error.hpp"
#include "vulkan/vulkan_core.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <vector>

namespace craft::vk {
static std::vector<const char *> CheckInstanceExtensions(std::initializer_list<InstanceExtension> exts) {
  uint32_t ext_count = 0;
  VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr));

  std::vector<VkExtensionProperties> available_exts(ext_count);
  VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, available_exts.data()));

  std::set<std::string_view> required_exts;
  for (const auto &ext : exts) {
    if (ext.required) {
      required_exts.insert(ext.name);
    }
  }

  std::vector<const char *> enabled_exts;
  enabled_exts.reserve(exts.size());

  std::set<std::string_view> enabled_required_exts;
  for (const auto &available_ext : available_exts) {
    std::cout << "Found an instance-level extension: " << available_ext.extensionName << std::endl;

    auto it = std::find_if(exts.begin(), exts.end(), [&](const InstanceExtension &ext) {
      return strcmp(ext.name, available_ext.extensionName) == 0;
    });
    if (it != exts.end()) {
      std::cout << std::boolalpha << "Enabling wanted extension (required = " << it->required << "): " << it->name
                << std::endl;
      enabled_exts.push_back(it->name);
      if (it->required) {
        enabled_required_exts.insert(it->name);
      }
    }
  }

  // Ensure all required extensions are enabled
  if (!std::includes(enabled_required_exts.begin(), enabled_required_exts.end(), required_exts.begin(),
                     required_exts.end())) {
    std::vector<std::string_view> missing_exts;
    std::set_difference(required_exts.begin(), required_exts.end(), enabled_required_exts.begin(),
                        enabled_required_exts.end(), std::back_inserter(missing_exts));

    std::cout << "Vulkan Error: The following extensions weren't enabled: " << std::endl;
    for (auto ext : missing_exts) {
      std::cout << "- " << ext << std::endl;
    }

    RuntimeError::SetErrorString("Not all required Vulkan extensions were enabled.");
  }

  return enabled_exts;
}

static std::vector<const char *> CheckInstanceLayers(std::initializer_list<const char *> layers) {
  uint32_t layer_count = 0;
  VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));

  std::vector<VkLayerProperties> available_layers(layer_count);
  VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data()));

  std::vector<const char *> enabled_layers;
  enabled_layers.reserve(layers.size());

  for (const auto &available_layer : available_layers) {
    std::cout << "Found an instance-level layer: " << available_layer.layerName << std::endl;

    auto it = std::find_if(layers.begin(), layers.end(),
                           [&](const char *name) { return strcmp(name, available_layer.layerName) == 0; });
    if (it != layers.end()) {
      std::cout << "Enabling wanted layer: " << *it << std::endl;
      enabled_layers.push_back(*it);
    }
  }

  size_t count = 0;
  for (auto enabled : enabled_layers) {
    for (auto wanted : layers) {
      if (strcmp(enabled, wanted) == 0) {
        count++;
      }
    }
  }

  if (count != layers.size()) {
    RuntimeError::SetErrorString("Not all required Vulkan layers were enabled.");
  }

  return enabled_layers;
}

static VKAPI_CALL VkBool32 VkDebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                  VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                  void *pUserData) {
  const char *severityStr = "";
  switch (messageSeverity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    severityStr = "VERBOSE";
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    severityStr = "INFO";
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    severityStr = "WARNING";
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    severityStr = "ERROR";
    break;

  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
    severityStr = "UNK";
    break;
  }

  std::string typeStr = "";
  if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
    if (!typeStr.empty())
      typeStr += "|";
    typeStr += "GENERAL";
  }
  if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
    if (!typeStr.empty())
      typeStr += "|";
    typeStr += "VALIDATION";
  }
  if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
    if (!typeStr.empty())
      typeStr += "|";
    typeStr += "PERFORMANCE";
  }

  std::cout << "[" << severityStr << "] [" << typeStr << "] " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}

Instance::Instance(std::initializer_list<InstanceExtension> extensions, std::initializer_list<const char *> layers,
                   std::string_view app_name) {
  auto exts = CheckInstanceExtensions(extensions);
  auto layrs = CheckInstanceLayers(layers);

  VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
  app_info.pApplicationName = app_name.data();
  app_info.pEngineName = "craft engine";
  app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);

  VkInstanceCreateInfo create_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

#ifndef NDEBUG
  VkDebugUtilsMessengerCreateInfoEXT messenger_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  messenger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
  messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  messenger_info.pfnUserCallback = VkDebugMessageCallback;
#endif

  create_info.pNext = &messenger_info;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledExtensionCount = exts.size();
  create_info.ppEnabledExtensionNames = exts.data();
  create_info.enabledLayerCount = layrs.size();
  create_info.ppEnabledLayerNames = layrs.data();

  VK_CHECK(vkCreateInstance(&create_info, nullptr, &m_instance));
  volkLoadInstanceOnly(m_instance);

#ifndef NDEBUG
  VK_CHECK(vkCreateDebugUtilsMessengerEXT(m_instance, &messenger_info, nullptr, &m_messenger));
#endif
}

Instance::~Instance() {
#ifndef NDEBUG
  if (m_messenger) {
    vkDestroyDebugUtilsMessengerEXT(m_instance, m_messenger, nullptr);
  }
#endif

  vkDestroyInstance(m_instance, nullptr);
}

Device Instance::SelectPhysicalDevice() {
  uint32_t device_count = 0;
  VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr));

  std::vector<VkPhysicalDevice> devices(device_count);
  VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data()));

  VkPhysicalDevice selected_device = VK_NULL_HANDLE;
  for (auto device : devices) {
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceFeatures feats;
    vkGetPhysicalDeviceProperties(device, &props);
    vkGetPhysicalDeviceFeatures(device, &feats);

    std::cout << "Queried a physical device: " << props.deviceName << std::endl;
    selected_device = device;

    uint32_t ext_count = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, nullptr));

    std::vector<VkExtensionProperties> exts(ext_count);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, exts.data()));

    for (const auto &ext : exts) {
      std::cout << "Found a device-level extension: " << ext.extensionName << std::endl;
    }

    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      // TODO: allow override
      std::cout << "Selecting GPU as physical device, since it is an discrete graphics card and passed prior checks."
                << std::endl;
      break;
    }
  }

  return Device(selected_device);
}

Device::Device(VkPhysicalDevice device) {
  VkDeviceCreateInfo create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

  vkCreateDevice(device, &create_info, nullptr, &m_device);
}

} // namespace craft::vk