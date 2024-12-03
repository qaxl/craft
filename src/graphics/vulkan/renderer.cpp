#include "renderer.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <ranges>
#include <set>
#include <vector>

#include <SDL3/SDL_vulkan.h>

#include "graphics/vulkan.hpp"
#include "util/error.hpp"
#include "vulkan/vulkan_core.h"

namespace craft::vk {
static std::vector<const char *> CheckInstanceExtensions(std::initializer_list<InstanceExtension> exts_) {
  auto available_exts = GetProperties<VkExtensionProperties>(vkEnumerateInstanceExtensionProperties, nullptr);

  std::vector<InstanceExtension> exts = exts_;
  uint32_t sdl_ext_count = 0;
  const char *const *sdl_exts = SDL_Vulkan_GetInstanceExtensions(&sdl_ext_count);

  for (uint32_t i = 0; i < sdl_ext_count; ++i) {
    exts.emplace_back(InstanceExtension{sdl_exts[i], true});
    std::cout << "SDL wants Vulkan instance-level extension enabled: " << sdl_exts[i] << std::endl;
  }

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

    RuntimeError::Throw("Not all required Vulkan extensions were enabled.");
  }

  return enabled_exts;
}

static std::vector<const char *> CheckInstanceLayers(std::initializer_list<const char *> layers) {
  auto available_layers = GetProperties<VkLayerProperties>(vkEnumerateInstanceLayerProperties);

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
    RuntimeError::Throw("Not all required Vulkan layers were enabled!");
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

static VkInstance CreateInstance(std::shared_ptr<Window> window, std::initializer_list<InstanceExtension> extensions,
                                 std::initializer_list<const char *> layers, std::string_view app_name,
                                 VkDebugUtilsMessengerEXT &messenger) {
  auto exts = CheckInstanceExtensions(extensions);
  auto layrs = CheckInstanceLayers(layers);

  VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
  app_info.pApplicationName = app_name.data();
  app_info.pEngineName = "craft engine";
  app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
  // TODO: not to depend on the latest and the greatest, add support for older.
  app_info.apiVersion = VK_API_VERSION_1_3;

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

  VkInstance instance;
  VK_CHECK(vkCreateInstance(&create_info, nullptr, &instance));
  volkLoadInstanceOnly(instance);

#ifndef NDEBUG
  VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &messenger_info, nullptr, &messenger));
#endif

  return instance;
}

static VkPhysicalDevice SelectPhysicalDevice(VkInstance instance, std::vector<const char *> &enabled_exts) {
  auto devices = GetProperties<VkPhysicalDevice>(vkEnumeratePhysicalDevices, instance);

  std::array<const char *, 1> req_exts{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  std::array<const char *, 0> opt_exts{};

  VkPhysicalDevice selected_device = VK_NULL_HANDLE;
  for (auto device : devices) {
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceFeatures feats;
    vkGetPhysicalDeviceProperties(device, &props);
    vkGetPhysicalDeviceFeatures(device, &feats);

    VkPhysicalDeviceVulkan13Features feats13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceVulkan12Features feats12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, &feats13};
    VkPhysicalDeviceFeatures2 feats2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &feats12};
    vkGetPhysicalDeviceFeatures2(device, &feats2);

    // TODO: make a static function in device to check features too?
    if (!feats13.dynamicRendering && !feats13.synchronization2) {
      RuntimeError::Throw("This Vulkan renderer won't work without dynamic rendering!");
    }

    std::cout << "Queried a physical device: " << props.deviceName << std::endl;

    auto exts = GetProperties<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, device, nullptr);
    auto queue_families = GetProperties<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, device);

    for (const auto &ext : exts) {
      std::cout << "Found a device-level extension: " << ext.extensionName << std::endl;

      if (auto it =
              std::find_if(req_exts.begin(), req_exts.end(),
                           [&](std::string_view wanted) { return strcmp(ext.extensionName, wanted.data()) == 0; });
          it != req_exts.end()) {
        std::cout << "Enabling required device-level extension: " << ext.extensionName << std::endl;
        enabled_exts.push_back(*it);
      } else if (auto it = std::find_if(
                     opt_exts.begin(), opt_exts.end(),
                     [&](std::string_view wanted) { return strcmp(ext.extensionName, wanted.data()) == 0; });
                 it != opt_exts.end()) {
        std::cout << "Enabling optional device-level extension: " << ext.extensionName << std::endl;
        enabled_exts.push_back(*it);
      }
    }

    if (!std::includes(enabled_exts.begin(), enabled_exts.end(), req_exts.begin(), req_exts.end())) {
      std::cout << "Can't use device " << props.deviceName << " because it doesn't support all necessary extensions."
                << std::endl;
      enabled_exts.clear();
      continue;
    }

    selected_device = device;

    for (const auto &[i, queue_family] : std::views::enumerate(queue_families)) {
      std::string capabilities = "";

      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        capabilities += "GRAPHICS ";
      if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)
        capabilities += "COMPUTE ";
      if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT)
        capabilities += "TRANSFER ";
      if (queue_family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
        capabilities += "SPARSE_BINDING ";

      // TODO: where the fuck this belongs?
      if (SDL_Vulkan_GetPresentationSupport(instance, device, i))
        capabilities += "PRESENT ";

      std::cout << "Found a queue family (ID=" << i << "): supports " << queue_family.queueCount
                << " queues, supporting features: " << capabilities << std::endl;
      ;
    }

    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      // TODO: allow override
      std::cout << "Selecting GPU as physical device, since it is an discrete graphics card and passed prior checks."
                << std::endl;
      break;
    }
  }

  return selected_device;
}

static VkSurfaceKHR CreateSurfaceFromWindow(VkInstance instance, Window *window) {
  VkSurfaceKHR surface;
  if (!SDL_Vulkan_CreateSurface(window->GetHandle(), instance, nullptr, &surface)) {
    RuntimeError::Throw("Couldn't create a Vulkan surface", EF_AppendSDLErrors);
  }
  return surface;
}

static VkDevice CreateDevice(VkPhysicalDevice device, std::vector<const char *> &&exts) {
  float priorities[] = {0.5f};
  VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queue_info.queueCount = 1;
  // the 0th queue supports all operations,
  // TODO: add specialized transfer queue (with it's own queue family index), and specialized compute queue?
  queue_info.queueFamilyIndex = 0;
  queue_info.pQueuePriorities = priorities;

  VkPhysicalDeviceVulkan13Features features13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
  VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
  features.pNext = &features13;

  features13.dynamicRendering = true;
  features13.synchronization2 = true;

  VkDeviceCreateInfo create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  create_info.pNext = &features;
  create_info.pQueueCreateInfos = &queue_info;
  create_info.queueCreateInfoCount = 1;

  create_info.ppEnabledExtensionNames = exts.data();
  create_info.enabledExtensionCount = exts.size();

  std::cout << exts.size() << ": ";
  for (auto ext : exts)
    std::cout << ext << ", ";
  std::cout << std::endl;

  VkDevice result;
  VK_CHECK(vkCreateDevice(device, &create_info, nullptr, &result));
  volkLoadDevice(result);

  return result;
}

static VkSwapchainKHR CreateSwapchain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface,
                                      VkExtent2D extent) {
  VkSwapchainKHR swapchain;
  VkSurfaceCapabilitiesKHR caps;
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &caps));

  VkSwapchainCreateInfoKHR create_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  create_info.surface = surface;
  // TODO: select best mode for our purposes (FIFO_KHR), but it's not available on all GPUs; Hence we need to test this
  // too while selecting the best suited GPU.
  create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  create_info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
  create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  create_info.imageExtent = extent;
  create_info.minImageCount = 3;
  create_info.imageArrayLayers = 1;
  create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.preTransform = caps.currentTransform;
  // TODO: revisit
  create_info.clipped = VK_TRUE;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  VK_CHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain));
  return swapchain;
}

Renderer::Renderer(std::shared_ptr<Window> window) : m_window{window} {
  std::vector<const char *> enabled_exts;

  m_instance = CreateInstance(window, {{VK_EXT_DEBUG_UTILS_EXTENSION_NAME, false}}, {"VK_LAYER_KHRONOS_validation"},
                              "craft app", m_messenger);

  m_physical_device = SelectPhysicalDevice(m_instance, enabled_exts);
  m_device = CreateDevice(m_physical_device, std::move(enabled_exts));
  // TODO: support multiple queues and queue families
  vkGetDeviceQueue(m_device, 0, 0, &m_queue);

  auto [width, height] = window->GetSize();
  VkExtent2D extent{width, height};

  m_surface = CreateSurfaceFromWindow(m_instance, window.get());
  m_swapchain = CreateSwapchain(m_physical_device, m_device, m_surface, extent);

  m_swapchain_images = GetProperties<VkImage>(vkGetSwapchainImagesKHR, m_device, m_swapchain);
  m_swapchain_image_views.resize(m_swapchain_images.size());

  for (auto [i, image_view] : std::views::enumerate(m_swapchain_image_views)) {
    VkImageViewCreateInfo create_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    create_info.image = m_swapchain_images[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(m_device, &create_info, nullptr, &image_view));
  }

  InitCommands();
  InitSyncStructures();

  VmaAllocatorCreateInfo allocator_info{};
  allocator_info.physicalDevice = m_physical_device;
  allocator_info.device = m_device;
  allocator_info.instance = m_instance;
  allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  vmaCreateAllocator(&allocator_info, &m_allocator);
}

Renderer::~Renderer() {
  vkDeviceWaitIdle(m_device);

  vmaDestroyAllocator(m_allocator);

  for (auto &frame : m_frames) {
    vkDestroyFence(m_device, frame.fe_render, nullptr);
    vkDestroySemaphore(m_device, frame.sp_render, nullptr);
    vkDestroySemaphore(m_device, frame.sp_swapchain, nullptr);

    vkDestroyCommandPool(m_device, frame.command_pool, nullptr);
  }

  for (auto image_view : m_swapchain_image_views) {
    vkDestroyImageView(m_device, image_view, nullptr);
  }

  vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
  vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
  vkDestroyDevice(m_device, nullptr);

#ifndef NDEBUG
  if (m_messenger) {
    vkDestroyDebugUtilsMessengerEXT(m_instance, m_messenger, nullptr);
  }
#endif

  vkDestroyInstance(m_instance, nullptr);
}

// TODO: vulkan_images.h?
static VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspect_mask) {
  return VkImageSubresourceRange{
      .aspectMask = aspect_mask, .levelCount = VK_REMAINING_MIP_LEVELS, .layerCount = VK_REMAINING_ARRAY_LAYERS};
}

static void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout,
                            VkImageLayout new_layout) {
  VkImageMemoryBarrier2 image_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};

  image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  image_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
  image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  image_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

  image_barrier.oldLayout = current_layout;
  image_barrier.newLayout = new_layout;

  VkImageAspectFlags aspect_mask =
      (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
  image_barrier.subresourceRange = ImageSubresourceRange(aspect_mask);
  image_barrier.image = image;

  VkDependencyInfo dep_info{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
  dep_info.imageMemoryBarrierCount = 1;
  dep_info.pImageMemoryBarriers = &image_barrier;

  vkCmdPipelineBarrier2(cmd, &dep_info);
}

static VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore) {
  return VkSemaphoreSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = semaphore,
      .value = 1,
      .stageMask = stage_mask,
  };
}

static VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd) {
  return VkCommandBufferSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .commandBuffer = cmd,
  };
}

static VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signal_semaphore,
                                VkSemaphoreSubmitInfo *wait_semaphore) {
  VkSubmitInfo2 info{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
  info.waitSemaphoreInfoCount = wait_semaphore != nullptr;
  info.pWaitSemaphoreInfos = wait_semaphore;

  info.signalSemaphoreInfoCount = signal_semaphore != nullptr;
  info.pSignalSemaphoreInfos = signal_semaphore;

  info.commandBufferInfoCount = 1;
  info.pCommandBufferInfos = cmd;

  return info;
}

void Renderer::Draw() {
  auto &frame = GetCurrentFrame();

  VK_CHECK(vkWaitForFences(m_device, 1, &frame.fe_render, true, 1 * 1000 * 1000 * 1000));
  VK_CHECK(vkResetFences(m_device, 1, &frame.fe_render));

  uint32_t swapchain_image_index;
  VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, 1 * 1000 * 1000 * 1000, frame.sp_swapchain, nullptr,
                                 &swapchain_image_index));

  VkCommandBuffer cmd = frame.command_buffer;
  VK_CHECK(vkResetCommandBuffer(cmd, 0));

  VkCommandBufferBeginInfo cmd_begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

  TransitionImage(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

  VkClearColorValue clearValue;
  float flash = std::abs(std::sin(m_frame_number / 120.f));
  float flash2 = std::abs(std::cos(m_frame_number / 240.f));
  float flash3 = std::abs(std::tan(m_frame_number / 120.f));
  clearValue = {{flash2, flash3, flash, 1.0f}};

  VkImageSubresourceRange clearRange = ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

  vkCmdClearColorImage(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1,
                       &clearRange);

  TransitionImage(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL,
                  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmd_info = CommandBufferSubmitInfo(cmd);
  VkSemaphoreSubmitInfo wait_info =
      SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame.sp_swapchain);
  VkSemaphoreSubmitInfo signal_info = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame.sp_render);

  VkSubmitInfo2 submit = SubmitInfo(&cmd_info, &signal_info, &wait_info);

  VK_CHECK(vkQueueSubmit2(m_queue, 1, &submit, frame.fe_render));

  VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &m_swapchain;

  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &frame.sp_render;

  present_info.pImageIndices = &swapchain_image_index;

  VK_CHECK(vkQueuePresentKHR(m_queue, &present_info));
}

void Renderer::InitCommands() {
  VkCommandPoolCreateInfo create_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  // TODO: dynamic
  create_info.queueFamilyIndex = 0;

  for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
    VK_CHECK(vkCreateCommandPool(m_device, &create_info, nullptr, &m_frames[i].command_pool));

    VkCommandBufferAllocateInfo alloc_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.commandPool = m_frames[i].command_pool;
    alloc_info.commandBufferCount = 1;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VK_CHECK(vkAllocateCommandBuffers(m_device, &alloc_info, &m_frames[i].command_buffer));
  }
}

void Renderer::InitSyncStructures() {
  VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
  VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  for (auto &frame : m_frames) {
    VK_CHECK(vkCreateFence(m_device, &fence_info, nullptr, &frame.fe_render));

    VK_CHECK(vkCreateSemaphore(m_device, &semaphore_info, nullptr, &frame.sp_swapchain));
    VK_CHECK(vkCreateSemaphore(m_device, &semaphore_info, nullptr, &frame.sp_render));
  }
}

} // namespace craft::vk