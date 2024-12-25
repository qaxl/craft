#include "renderer.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <ranges>
#include <set>
#include <vector>

#include <SDL3/SDL_vulkan.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "device.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include "util/error.hpp"
#include "utils.hpp"
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

Renderer::Renderer(std::shared_ptr<Window> window) : m_window{window} {
  std::vector<const char *> enabled_exts;

  m_instance = CreateInstance(window, {{VK_EXT_DEBUG_UTILS_EXTENSION_NAME, false}}, {"VK_LAYER_KHRONOS_validation"},
                              "craft app", m_messenger);

  DeviceFeatures features{};
  features.vk_1_3_features.dynamicRendering = true;
  features.vk_1_3_features.synchronization2 = true;
  // ImGui requirement
  features.vk_1_2_features.bufferDeviceAddress = true;

  m_device = Device{m_instance, {DeviceExtension{VK_KHR_SWAPCHAIN_EXTENSION_NAME}}, &features};

  VmaVulkanFunctions funcs{.vkGetInstanceProcAddr = vkGetInstanceProcAddr, .vkGetDeviceProcAddr = vkGetDeviceProcAddr};

  VmaAllocatorCreateInfo allocator_info{};
  allocator_info.physicalDevice = m_device.GetPhysicalDevice();
  allocator_info.device = m_device.GetDevice();
  allocator_info.instance = m_instance;
  allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  allocator_info.pVulkanFunctions = &funcs;

  vmaCreateAllocator(&allocator_info, &m_allocator);

  auto [width, height] = window->GetSize();
  VkExtent2D extent{width, height};

  m_surface = m_window->CreateSurface(m_instance);

  VkSurfaceCapabilitiesKHR caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device.GetPhysicalDevice(), m_surface, &caps);

  m_swapchain = std::make_shared<Swapchain>(m_device.GetDevice(), m_surface, extent, caps.currentTransform);

  // TODO: MOVE OUT OF THIS FUNCTION
  VkExtent3D draw_image_extent{extent.width, extent.height, 1};
  m_draw_image.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  m_draw_image.extent = draw_image_extent;

  VkImageUsageFlags draw_image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                       VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VkImageCreateInfo img_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  img_info.format = m_draw_image.format;
  img_info.usage = draw_image_usage;
  img_info.extent = draw_image_extent;
  img_info.mipLevels = 1;
  img_info.arrayLayers = 1;
  img_info.samples = VK_SAMPLE_COUNT_1_BIT;
  img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  img_info.imageType = VK_IMAGE_TYPE_2D;

  VmaAllocationCreateInfo img_alloc{};
  img_alloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  img_alloc.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  VK_CHECK(vmaCreateImage(m_allocator, &img_info, &img_alloc, &m_draw_image.image, &m_draw_image.allocation, nullptr));

  VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.image = m_draw_image.image;
  view_info.format = m_draw_image.format;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.layerCount = 1;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

  VK_CHECK(vkCreateImageView(m_device.GetDevice(), &view_info, nullptr, &m_draw_image.view));
  // END OF MOVE OUT OF THIS FUNCTION.

  InitCommands();
  InitSyncStructures();
  InitDescriptors();
  InitPipelines();

  std::cout << m_device.GetGraphicsQueue() << std::endl;

  m_imgui = std::make_shared<ImGui>(m_instance, m_device.GetPhysicalDevice(), m_device.GetDevice(), m_window,
                                    m_device.GetGraphicsQueue(), 0);
}

Renderer::~Renderer() {
  std::cout << "Wait... ";
  vkDeviceWaitIdle(m_device.GetDevice());
  std::cout << "Destroy..." << std::endl;

  // Manually kill all other structures before...
  m_imgui = nullptr;

  vkDestroyPipelineLayout(m_device.GetDevice(), m_gradient_pipeline_layout, nullptr);
  for (auto &bg_effect : m_bg_effects) {
    vkDestroyPipeline(m_device.GetDevice(), bg_effect.pipeline, nullptr);
  }
  // vkDestroyPipeline(m_device, m_gradient_pipeline, nullptr);

  m_descriptor_allocator.DestroyPool(m_device.GetDevice());
  vkDestroyDescriptorSetLayout(m_device.GetDevice(), m_draw_image_descriptor_layout, nullptr);

  vkDestroyImageView(m_device.GetDevice(), m_draw_image.view, nullptr);
  vmaDestroyImage(m_allocator, m_draw_image.image, m_draw_image.allocation);

  vmaDestroyAllocator(m_allocator);

  for (auto &frame : m_frames) {
    vkDestroyFence(m_device.GetDevice(), frame.fe_render, nullptr);
    vkDestroySemaphore(m_device.GetDevice(), frame.sp_render, nullptr);
    vkDestroySemaphore(m_device.GetDevice(), frame.sp_swapchain, nullptr);

    vkDestroyCommandPool(m_device.GetDevice(), frame.command_pool, nullptr);
  }

  m_swapchain = nullptr;
  vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
  vkDestroyDevice(m_device.GetDevice(), nullptr);

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

static void CloneImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D source_size,
                       VkExtent2D destination_size) {
  VkImageBlit2 blit_region{VK_STRUCTURE_TYPE_IMAGE_BLIT_2};

  blit_region.srcOffsets[1].x = source_size.width;
  blit_region.srcOffsets[1].y = source_size.height;
  blit_region.srcOffsets[1].z = 1;

  blit_region.dstOffsets[1].x = destination_size.width;
  blit_region.dstOffsets[1].y = destination_size.height;
  blit_region.dstOffsets[1].z = 1;

  blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

  blit_region.srcSubresource.layerCount = 1;
  blit_region.dstSubresource.layerCount = 1;

  VkBlitImageInfo2 blit_info{VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2};
  blit_info.srcImage = source;
  blit_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  blit_info.dstImage = destination;
  blit_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  blit_info.filter = VK_FILTER_LINEAR;
  blit_info.regionCount = 1;
  blit_info.pRegions = &blit_region;

  vkCmdBlitImage2(cmd, &blit_info);
}

void Renderer::Draw() {
  auto &frame = GetCurrentFrame();

  VK_CHECK(vkWaitForFences(m_device.GetDevice(), 1, &frame.fe_render, true, 1 * 1000 * 1000 * 1000));
  VK_CHECK(vkResetFences(m_device.GetDevice(), 1, &frame.fe_render));

  auto [image, view] = m_swapchain->AcquireNextImage(frame.sp_swapchain);

  VkCommandBuffer cmd = frame.command_buffer;
  VK_CHECK(vkResetCommandBuffer(cmd, 0));

  VkCommandBufferBeginInfo cmd_begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  m_draw_extent.width = m_draw_image.extent.width;
  m_draw_extent.height = m_draw_image.extent.height;

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

  TransitionImage(cmd, m_draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
  DrawBackground(cmd);
  TransitionImage(cmd, m_draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  TransitionImage(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  CloneImage(cmd, m_draw_image.image, image, m_draw_extent, m_draw_extent);
  TransitionImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  m_imgui->Draw(cmd, view);
  TransitionImage(cmd, image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmd_info = CommandBufferSubmitInfo(cmd);
  VkSemaphoreSubmitInfo wait_info =
      SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame.sp_swapchain);
  VkSemaphoreSubmitInfo signal_info = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame.sp_render);

  VkSubmitInfo2 submit = SubmitInfo(&cmd_info, &signal_info, &wait_info);

  VK_CHECK(vkQueueSubmit2(m_device.GetGraphicsQueue(), 1, &submit, frame.fe_render));

  VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = m_swapchain->GetHandlePtr();

  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &frame.sp_render;

  uint32_t current_index = m_swapchain->GetCurrentImageIndex();
  present_info.pImageIndices = &current_index;

  VkResult queue_present = vkQueuePresentKHR(m_device.GetGraphicsQueue(), &present_info);
  if (queue_present != VK_SUCCESS) {
    if (queue_present == VK_SUBOPTIMAL_KHR) {
      // TODO: abstract swapchain
    } else {
      VK_CHECK(queue_present);
    }
  }
}

void Renderer::DrawBackground(VkCommandBuffer cmd) {
  float flash = std::abs(std::sin(m_frame_number / 120.f));
  VkClearColorValue clear_value{{0.f, 0.f, flash, 1.f}};

  VkImageSubresourceRange clear_range = ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

  ComputeEffect &effect = m_bg_effects[m_current_bg_effect];

  // vkCmdClearColorImage(cmd, m_draw_image.image, VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_gradient_pipeline_layout, 0, 1,
                          &m_draw_image_descriptors, 0, nullptr);

  // ComputePushConstants pc{};
  // pc.data[0] = Vec<float, 4>(1.f, 0.f, 0.f, 1.f);
  // pc.data[1] = Vec<float, 4>(0.f, 0.f, 1.f, 1.f);
  vkCmdPushConstants(cmd, m_gradient_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants),
                     &effect.pc);

  vkCmdDispatch(cmd, std::ceil(m_draw_extent.width / 16.0), std::ceil(m_draw_extent.height / 16.0), 1);
}

void Renderer::InitCommands() {
  VkCommandPoolCreateInfo create_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  // TODO: dynamic
  create_info.queueFamilyIndex = 0;

  for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
    VK_CHECK(vkCreateCommandPool(m_device.GetDevice(), &create_info, nullptr, &m_frames[i].command_pool));

    VkCommandBufferAllocateInfo alloc_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.commandPool = m_frames[i].command_pool;
    alloc_info.commandBufferCount = 1;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VK_CHECK(vkAllocateCommandBuffers(m_device.GetDevice(), &alloc_info, &m_frames[i].command_buffer));
  }
}

void Renderer::InitSyncStructures() {
  VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
  VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  for (auto &frame : m_frames) {
    VK_CHECK(vkCreateFence(m_device.GetDevice(), &fence_info, nullptr, &frame.fe_render));

    VK_CHECK(vkCreateSemaphore(m_device.GetDevice(), &semaphore_info, nullptr, &frame.sp_swapchain));
    VK_CHECK(vkCreateSemaphore(m_device.GetDevice(), &semaphore_info, nullptr, &frame.sp_render));
  }
}

void Renderer::InitDescriptors() {
  std::array<DescriptorAllocator::PoolSizeRatio, 1> sizes{
      DescriptorAllocator::PoolSizeRatio{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};
  m_descriptor_allocator.InitPool(m_device.GetDevice(), 10, sizes);

  DescriptorLayoutBuilder builder;
  builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
  m_draw_image_descriptor_layout = builder.Build(m_device.GetDevice(), VK_SHADER_STAGE_COMPUTE_BIT);

  m_draw_image_descriptors = m_descriptor_allocator.Allocate(m_device.GetDevice(), m_draw_image_descriptor_layout);

  VkDescriptorImageInfo info{};
  info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  info.imageView = m_draw_image.view;

  VkWriteDescriptorSet draw_image_write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  draw_image_write.dstSet = m_draw_image_descriptors;
  draw_image_write.descriptorCount = 1;
  draw_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  draw_image_write.pImageInfo = &info;

  vkUpdateDescriptorSets(m_device.GetDevice(), 1, &draw_image_write, 0, nullptr);
}

void Renderer::InitPipelines() { InitBackgroundPipelines(); }

void Renderer::InitBackgroundPipelines() {
  VkPipelineLayoutCreateInfo compute_layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  compute_layout.setLayoutCount = 1;
  compute_layout.pSetLayouts = &m_draw_image_descriptor_layout;

  VkPushConstantRange push_constants{};
  push_constants.size = sizeof(ComputePushConstants);
  push_constants.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  compute_layout.pushConstantRangeCount = 1;
  compute_layout.pPushConstantRanges = &push_constants;

  VK_CHECK(vkCreatePipelineLayout(m_device.GetDevice(), &compute_layout, nullptr, &m_gradient_pipeline_layout));

  auto gradient_shader = LoadShaderModule("../shaders/gradient.comp.spv", m_device.GetDevice());
  auto sky_shader = LoadShaderModule("../shaders/sky.comp.spv", m_device.GetDevice());
  if (!gradient_shader || !sky_shader) {
    RuntimeError::Throw("Error occurred while trying to load compute shader.");
  }

  VkPipelineShaderStageCreateInfo stage_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  stage_info.module = gradient_shader.value();
  stage_info.pName = "main";

  VkComputePipelineCreateInfo compute_pipeline_info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  compute_pipeline_info.layout = m_gradient_pipeline_layout;
  compute_pipeline_info.stage = stage_info;

  ComputeEffect gradient{.name = "Gradient",
                         .layout = m_gradient_pipeline_layout,
                         .pc = {{Vec<float, 4>{1.f, 0.f, 0.f, 1.f}, {0.f, 0.f, 1.f, 1.f}}}};

  ComputeEffect sky{
      .name = "Sky", .layout = m_gradient_pipeline_layout, .pc = {{Vec<float, 4>{0.1f, 0.2f, 0.4f, 0.97f}}}};

  VK_CHECK(
      vkCreateComputePipelines(m_device.GetDevice(), nullptr, 1, &compute_pipeline_info, nullptr, &gradient.pipeline));
  vkDestroyShaderModule(m_device.GetDevice(), compute_pipeline_info.stage.module, nullptr);

  compute_pipeline_info.stage.module = sky_shader.value();
  VK_CHECK(vkCreateComputePipelines(m_device.GetDevice(), nullptr, 1, &compute_pipeline_info, nullptr, &sky.pipeline));
  vkDestroyShaderModule(m_device.GetDevice(), compute_pipeline_info.stage.module, nullptr);

  m_bg_effects.push_back(gradient);
  m_bg_effects.push_back(sky);
}

struct ImmSbm_ {
  VkFence fence;
  VkCommandBuffer cmd;
  VkCommandPool pool;
  VkDevice device;

  ~ImmSbm_() { // TODO: this most likely won't work because this is in static program memory which gets destroyed last.
    vkDestroyCommandPool(device, pool, nullptr);
    vkDestroyFence(device, fence, nullptr);
  }
};

static std::shared_ptr<ImmSbm_> CreateImmediateSubmissionStructures(VkDevice device) {
  auto sbm = std::make_shared<ImmSbm_>();

  VkCommandPoolCreateInfo create_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  // TODO: dynamic
  create_info.queueFamilyIndex = 0;
  VK_CHECK(vkCreateCommandPool(device, &create_info, nullptr, &sbm->pool));

  VkCommandBufferAllocateInfo alloc_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  alloc_info.commandPool = sbm->pool;
  alloc_info.commandBufferCount = 1;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  VK_CHECK(vkAllocateCommandBuffers(device, &alloc_info, &sbm->cmd));

  VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
  VK_CHECK(vkCreateFence(device, &fence_info, nullptr, &sbm->fence));

  return sbm;
}

// TODO: a dedicated submit queue, and perhaps a transfer queue
void Renderer::SubmitNow(std::function<void(VkCommandBuffer)> f) {
  static thread_local auto imm = CreateImmediateSubmissionStructures(m_device.GetDevice());

  VK_CHECK(vkResetFences(m_device.GetDevice(), 1, &imm->fence));
  VK_CHECK(vkResetCommandBuffer(imm->cmd, 0));

  VkCommandBufferBeginInfo cmd_begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  cmd_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VK_CHECK(vkBeginCommandBuffer(imm->cmd, &cmd_begin));

  f(imm->cmd);

  VK_CHECK(vkEndCommandBuffer(imm->cmd));

  VkCommandBufferSubmitInfo cmd_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
  cmd_info.commandBuffer = imm->cmd;
  VkSubmitInfo2 submit = SubmitInfo(&cmd_info, nullptr, nullptr);

  VK_CHECK(vkQueueSubmit2(m_device.GetTransferQueue(), 1, &submit, imm->fence));
  VK_CHECK(vkWaitForFences(m_device.GetDevice(), 1, &imm->fence, VK_TRUE, 1000'000'000));
}
} // namespace craft::vk
