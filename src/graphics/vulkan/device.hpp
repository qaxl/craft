#pragma once

#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

#include <volk.h>

#include "util/optimization.hpp"

namespace craft::vk {
struct DeviceExtension {
  const char *name;
  bool required = false;
};

struct DeviceFeatures {
  VkPhysicalDeviceFeatures base_features{};
  VkPhysicalDeviceVulkan12Features vk_1_2_features{};
  VkPhysicalDeviceVulkan13Features vk_1_3_features{};
};

struct QueueFamily {
  uint64_t index = -1;
  uint32_t supported_queues = 0;
};

struct QueueFamilyIndices {
  std::optional<QueueFamily> graphics_queue_family;
  std::optional<QueueFamily> transfer_queue_family;
  std::optional<QueueFamily> compute_queue_family;

  constexpr static const size_t kMaxQueues = 3;
};

struct SuitableDevice {
  VkPhysicalDevice device{};
  QueueFamilyIndices indices;
  DeviceFeatures features;
  VkPhysicalDeviceProperties properties;

  bool discrete = false;
  std::string name = "";

  std::vector<const char *> extensions;
};

class Device {
public:
  Device(VkInstance instance, std::initializer_list<DeviceExtension> extensions,
         const DeviceFeatures *features = nullptr);
  ~Device();

  Device(const Device &) = delete;
  Device(Device &&other) = delete;

  Device &operator=(const Device &) = delete;
  Device &operator=(Device &&other) = delete;

  FORCE_INLINE VkInstance GetInstance() { return m_instance; }
  FORCE_INLINE VkDevice GetDevice() { return m_device; }
  FORCE_INLINE VkPhysicalDevice GetPhysicalDevice() { return m_physical_device; }

  FORCE_INLINE VkQueue GetGraphicsQueue() { return m_graphics; }
  FORCE_INLINE VkQueue GetTransferQueue() { return m_transfer; }
  FORCE_INLINE VkQueue GetComputeQueue() { return m_compute; }

  FORCE_INLINE uint32_t GetGraphicsQueueFamily() { return m_current_device->indices.graphics_queue_family->index; }
  FORCE_INLINE uint32_t GetTransferQueueFamily() {
    return m_current_device->indices.transfer_queue_family.has_value()
               ? m_current_device->indices.transfer_queue_family->index
               : GetGraphicsQueueFamily();
  }
  FORCE_INLINE uint32_t GetComputeQueueFamily() {
    return m_current_device->indices.compute_queue_family.has_value()
               ? m_current_device->indices.compute_queue_family->index
               : GetGraphicsQueueFamily();
  }

  FORCE_INLINE float GetMaxSamplerAnisotropy() { return m_current_device->properties.limits.maxSamplerAnisotropy; }

  FORCE_INLINE void WaitIdle() { vkDeviceWaitIdle(m_device); }

  VkPresentModeKHR GetOptimalPresentMode(VkSurfaceKHR surface, bool vsync = true) const;
  VkSurfaceFormatKHR GetOptimalSurfaceFormat(VkSurfaceKHR surface) const;

private:
  void SelectPhysicalDevice(std::initializer_list<DeviceExtension> extensions, const DeviceFeatures *features);
  void CreateDevice(SuitableDevice *device);

private:
  VkInstance m_instance{};
  VkDevice m_device{};

  VkPhysicalDevice m_physical_device{};
  std::vector<SuitableDevice> m_devices;

  VkQueue m_graphics{};
  VkQueue m_transfer{};
  VkQueue m_compute{};

  SuitableDevice *m_current_device{};
};
} // namespace craft::vk
