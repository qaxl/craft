#include "device.hpp"

#include <algorithm>
#include <array>
#include <execution>
#include <optional>

#include <SDL3/SDL_vulkan.h>

#include "utils.hpp"

#include <iostream>

namespace craft::vk {
Device::Device(VkInstance instance, std::initializer_list<DeviceExtension> extensions, const DeviceFeatures *features)
    : m_instance{instance} {
  SelectPhysicalDevice(extensions, features);
}

Device::~Device() {
  if (m_device) {
    vkDestroyDevice(m_device, nullptr);
    m_device = nullptr;
  }
}

void Device::SelectPhysicalDevice(std::initializer_list<DeviceExtension> extensions, const DeviceFeatures *features) {
  auto devices = GetProperties<VkPhysicalDevice>(vkEnumeratePhysicalDevices, m_instance);

  std::vector<SuitableDevice> suitable_devices;
  std::mutex suitable_devices_lock;

  std::for_each(
      std::execution::par_unseq, devices.begin(), devices.end(),
      [this, &extensions, features, &suitable_devices_lock, &suitable_devices](VkPhysicalDevice device) {
        std::vector<DeviceExtension> required;
        std::vector<DeviceExtension> optional;
        for (auto extension : extensions) {
          if (extension.required)
            required.emplace_back(DeviceExtension{extension.name, false});
          else
            optional.emplace_back(DeviceExtension{extension.name, false});
        }

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);

        VkPhysicalDeviceVulkan13Features feats3{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceVulkan12Features feats2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, &feats3};
        VkPhysicalDeviceFeatures2 feats{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &feats2};
        vkGetPhysicalDeviceFeatures2(device, &feats);

        for (size_t offset = 0; offset < sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32); ++offset) {
          VkBool32 supported = reinterpret_cast<const VkBool32 *>(&feats.features)[offset];
          VkBool32 requested = reinterpret_cast<const VkBool32 *>(&features->base_features.features)[offset];

          if (!supported && requested) {
            return;
          }
        }

        for (size_t offset = offsetof(VkPhysicalDeviceVulkan12Features, samplerMirrorClampToEdge) / sizeof(VkBool32);
             offset <= offsetof(VkPhysicalDeviceVulkan12Features, subgroupBroadcastDynamicId) / sizeof(VkBool32);
             ++offset) {
          VkBool32 supported = reinterpret_cast<const VkBool32 *>(&feats2)[offset];
          VkBool32 requested = reinterpret_cast<const VkBool32 *>(&features->vk_1_2_features)[offset];

          if (!supported && requested) {
            return;
          }
        }

        for (size_t offset = offsetof(VkPhysicalDeviceVulkan13Features, robustImageAccess) / sizeof(VkBool32);
             offset <= offsetof(VkPhysicalDeviceVulkan13Features, maintenance4) / sizeof(VkBool32); ++offset) {
          VkBool32 supported = reinterpret_cast<const VkBool32 *>(&feats3)[offset];
          VkBool32 requested = reinterpret_cast<const VkBool32 *>(&features->vk_1_3_features)[offset];

          if (!supported && requested) {
            return;
          }
        }

        auto extensions = GetProperties<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, device, nullptr);
        auto queue_families = GetProperties<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, device);

        std::vector<const char *> extensions_enabled;
        for (auto &req : required) {
          if (std::find_if(extensions.begin(), extensions.end(), [&req](VkExtensionProperties ext) {
                return strcmp(req.name, ext.extensionName) == 0;
              }) != extensions.end()) {
            req.required = true;
            extensions_enabled.push_back(req.name);
          }
        }

        // TODO: do something with optional extensions?
        for (auto &opt : optional) {
          if (std::find_if(extensions.begin(), extensions.end(), [&opt](VkExtensionProperties ext) {
                return strcmp(opt.name, ext.extensionName) == 0;
              }) != extensions.end()) {
            opt.required = true;
            extensions_enabled.push_back(opt.name);
          }
        }

        if (std::find_if(required.begin(), required.end(), [](DeviceExtension ext) { return !ext.required; }) !=
            required.end()) {
          return;
        }

        QueueFamilyIndices indices;
        for (uint32_t i = 0; i < queue_families.size(); ++i) {
          auto &queue_family = queue_families[i];

          if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            if (SDL_Vulkan_GetPresentationSupport(m_instance, device, i)) {
              indices.graphics_queue_family = QueueFamily{i, queue_family.queueCount};
            }
          } else {
            if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
              indices.transfer_queue_family = QueueFamily{i, queue_family.queueCount};
            }

            if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
              indices.compute_queue_family = QueueFamily{i, queue_family.queueCount};
            }
          }
        }

        if (!indices.graphics_queue_family.has_value()) {
          return;
        }

        std::lock_guard<std::mutex> lock(suitable_devices_lock);
        suitable_devices.emplace_back(SuitableDevice{device, indices, *features,
                                                     props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
                                                     props.deviceName, extensions_enabled});
      });

  m_devices = std::move(suitable_devices);
  if (m_devices.empty()) {
    RuntimeError::Throw(
        "Not a single suitable Vulkan-capable device were found in the system. Please ensure you have Vulkan-capable "
        "graphics card(s) in the first place,\nand if you do - consider upgrading your graphics drivers.\n\nThis "
        "program "
        "cannot continue, since Vulkan-capable device is a hard requirement for this program to run.");
  } else {

    std::cout << m_devices.size() << ',' << m_devices.empty() << ',' << &m_devices[0] << ',' << m_devices[0].name
              << std::endl;

    if (auto it = std::find_if(m_devices.begin(), m_devices.end(),
                               [](const SuitableDevice &device) { return device.discrete; });
        it != m_devices.end()) {
      CreateDevice(&*it);
    } else {
      CreateDevice(&m_devices[0]);
    }
  }
}

void Device::CreateDevice(SuitableDevice *device) {
  m_current_device = device;

  VkDeviceCreateInfo create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

  create_info.enabledExtensionCount = device->extensions.size();
  create_info.ppEnabledExtensionNames = device->extensions.data();

  create_info.pNext = &device->features.base_features;
  device->features.base_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  device->features.base_features.pNext = &device->features.vk_1_2_features;
  device->features.vk_1_2_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  device->features.vk_1_2_features.pNext = &device->features.vk_1_3_features;
  device->features.vk_1_3_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

  uint32_t queue_create_info_count = 1;
  std::array<VkDeviceQueueCreateInfo, QueueFamilyIndices::kMaxQueues> queue_info{};

  float priorities[] = {1.0f};

  queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_info[0].pQueuePriorities = priorities;
  queue_info[0].queueCount = 1;
  queue_info[0].queueFamilyIndex = device->indices.graphics_queue_family->index;

  if (device->indices.transfer_queue_family.has_value()) {
    queue_create_info_count += 1;
    queue_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[1].pQueuePriorities = priorities;

    queue_info[1].queueCount = 1;
    queue_info[1].queueFamilyIndex = device->indices.transfer_queue_family->index;
  }

  if (device->indices.compute_queue_family.has_value()) {
    queue_create_info_count += 1;
    queue_info[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[2].pQueuePriorities = priorities;

    queue_info[2].queueCount = 1;
    queue_info[2].queueFamilyIndex = device->indices.compute_queue_family->index;
  }

  create_info.queueCreateInfoCount = queue_create_info_count;
  create_info.pQueueCreateInfos = queue_info.data();

  VK_CHECK(vkCreateDevice(device->device, &create_info, nullptr, &m_device));
  volkLoadDevice(m_device);

  m_physical_device = device->device;

  vkGetDeviceQueue(m_device, device->indices.graphics_queue_family->index, 0, &m_graphics);

  if (device->indices.transfer_queue_family.has_value()) {
    vkGetDeviceQueue(m_device, device->indices.transfer_queue_family->index, 0, &m_transfer);
  } else {
    m_transfer = m_graphics;
  }

  if (device->indices.compute_queue_family.has_value()) {
    vkGetDeviceQueue(m_device, device->indices.compute_queue_family->index, 0, &m_compute);
  } else {
    m_compute = m_graphics;
  }
}
} // namespace craft::vk
