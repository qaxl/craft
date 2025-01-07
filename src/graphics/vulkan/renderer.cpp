#include "renderer.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <set>
#include <vector>

#include <SDL3/SDL_vulkan.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "descriptor.hpp"
#include "device.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "math/mat.hpp"
#include "mesh.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include "texture.hpp"
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
    auto it = std::find_if(exts.begin(), exts.end(), [&](const InstanceExtension &ext) {
      return strcmp(ext.name, available_ext.extensionName) == 0;
    });
    if (it != exts.end()) {
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
    auto it = std::find_if(layers.begin(), layers.end(),
                           [&](const char *name) { return strcmp(name, available_layer.layerName) == 0; });
    if (it != layers.end()) {
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

  create_info.pNext = &messenger_info;
#endif

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

Renderer::Renderer(std::shared_ptr<Window> window, Camera const &camera, Chunk &chunk)
    : m_window{window}, m_camera{camera}, m_chunk{chunk} {
  std::vector<const char *> enabled_exts;

  m_instance = CreateInstance(window,
                              {
#ifndef NDEBUG
                                  {VK_EXT_DEBUG_UTILS_EXTENSION_NAME, false},
#endif
                                  {VK_EXT_ROBUSTNESS_2_EXTENSION_NAME, false}},
                              {/* use vulkan configurator, thank you :) */}, "craft app", m_messenger);
  // This is just to make sure that the VkInstance gets destroyed the last.
  m_instance_raii.instance = m_instance;

  DeviceFeatures features{};
  features.vk_1_3_features.dynamicRendering = true;
  features.vk_1_3_features.synchronization2 = true;
  features.vk_1_2_features.bufferDeviceAddress = true;
  // TODO: don't enforce
  features.base_features.features.samplerAnisotropy = true;

  m_device = Device{m_instance, {DeviceExtension{VK_KHR_SWAPCHAIN_EXTENSION_NAME}}, &features};

  VmaVulkanFunctions funcs{.vkGetInstanceProcAddr = vkGetInstanceProcAddr, .vkGetDeviceProcAddr = vkGetDeviceProcAddr};

  VmaAllocatorCreateInfo allocator_info{};
  allocator_info.physicalDevice = m_device.GetPhysicalDevice();
  allocator_info.device = m_device.GetDevice();
  allocator_info.instance = m_instance;
  allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  allocator_info.pVulkanFunctions = &funcs;

  vmaCreateAllocator(&allocator_info, &m_allocator);
  m_allocator_raii.allocator = m_allocator;

  auto [width, height] = window->GetSize();
  VkExtent2D extent{width, height};

  m_surface = m_window->CreateSurface(m_instance);
  m_draw_extent = {width, height};

  VkSurfaceCapabilitiesKHR caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device.GetPhysicalDevice(), m_surface, &caps);

  m_swapchain = Swapchain{m_device.GetDevice(),
                          m_surface,
                          extent,
                          caps.currentTransform,
                          m_device.GetOptimalPresentMode(m_surface, false),
                          m_device.GetOptimalSurfaceFormat(m_surface)};
  m_instance_raii.surface = m_surface;

  for (auto &frame : m_frames) {
    frame.render_target = AllocatedImage{m_device.GetDevice(), m_allocator, m_draw_extent,
                                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};

    frame.depth_buffer = AllocatedImage{m_device.GetDevice(), m_allocator, m_draw_extent,
                                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT};
  }

  InitCommands();
  InitSyncStructures();
  // InitDescriptors();
  InitPipelines();
  InitDefaultData();

  std::array<Vtx, 4> vertices = {
      Vtx{.pos = {m_draw_extent.width / 2 - 5, m_draw_extent.height / 2 - 5, 0}, .uv = {0, 0}},
      Vtx{.pos = {m_draw_extent.width / 2 + 5, m_draw_extent.height / 2 - 5, 0}, .uv = {1, 0}},
      Vtx{.pos = {m_draw_extent.width / 2 - 5, m_draw_extent.height / 2 + 5, 0}, .uv = {0, 1}},
      Vtx{.pos = {m_draw_extent.width / 2 + 5, m_draw_extent.height / 2 + 5, 0}, .uv = {1, 1}},
  };
  std::array<uint32_t, 6> indices = {0, 2, 1, 1, 2, 3};

  m_crosshair_texture = std::make_shared<Texture>(m_allocator, m_device, this, "textures/crosshair.png");
  m_crosshair_mesh = UploadMesh(this, m_device.GetDevice(), m_allocator, indices, vertices);

  m_texture = std::make_shared<Texture>(m_allocator, m_device, this, "textures/spritesheet_blocks.png");
  // UpdateTexturedMeshDescriptors();
  m_imgui = ImGui{m_instance, m_window, &m_device};
}

Renderer::~Renderer() {
  m_device.WaitIdle();

  DestroyBuffer(m_allocator, std::move(m_mesh.index));
  DestroyBuffer(m_allocator, std::move(m_mesh.vertex));

  vkDestroyPipelineLayout(m_device.GetDevice(), m_textured_mesh_pipeline_layout, nullptr);
  vkDestroyPipeline(m_device.GetDevice(), m_textured_mesh_pipeline, nullptr);

  vkDestroyDescriptorSetLayout(m_device.GetDevice(), m_textured_mesh_descriptor_layout, nullptr);
  m_dallocator.DestroyPool(m_device.GetDevice());

  // vkDestroyPipelineLayout(m_device.GetDevice(), m_triangle_pipeline_layout, nullptr);
  // vkDestroyPipeline(m_device.GetDevice(), m_triangle_pipeline, nullptr);

  // vkDestroyPipelineLayout(m_device.GetDevice(), m_gradient_pipeline_layout, nullptr);
  // for (auto &bg_effect : m_bg_effects) {
  //   vkDestroyPipeline(m_device.GetDevice(), bg_effect.pipeline, nullptr);
  // }

  // m_descriptor_allocator.DestroyPool(m_device.GetDevice());
  // vkDestroyDescriptorSetLayout(m_device.GetDevice(), m_draw_image_descriptor_layout, nullptr);

  for (auto &frame : m_frames) {
    vkDestroyFence(m_device.GetDevice(), frame.fe_render, nullptr);
    vkDestroySemaphore(m_device.GetDevice(), frame.sp_render, nullptr);
    vkDestroySemaphore(m_device.GetDevice(), frame.sp_swapchain, nullptr);

    vkDestroyCommandPool(m_device.GetDevice(), frame.command_pool, nullptr);
  }

  // vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

#ifndef NDEBUG
  if (m_messenger) {
    vkDestroyDebugUtilsMessengerEXT(m_instance, m_messenger, nullptr);
  }
#endif
}

void Renderer::Draw() {
  auto &frame = GetCurrentFrame();
  VK_CHECK(vkWaitForFences(m_device.GetDevice(), 1, &frame.fe_render, true, 1'000'000'000));
  VK_CHECK(vkResetFences(m_device.GetDevice(), 1, &frame.fe_render));

  bool should_resize = false;

  AcquiredImage res = m_swapchain.AcquireNextImage(frame.sp_swapchain, 1'000'000'000);
  if (res.should_resize) {
    if (res.image && res.view) {
      should_resize = true;
    } else {
      ResizeSwapchain();
      Draw();
    }
  }

  VkImage image = res.image;
  VkImageView view = res.view;

  VkCommandBuffer cmd = frame.command_buffer;
  // VK_CHECK(vkResetCommandBuffer(cmd, 0));
  VK_CHECK(vkResetCommandPool(m_device.GetDevice(), frame.command_pool, 0));

  VkCommandBufferBeginInfo cmd_begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  m_draw_extent.width = frame.render_target.extent.width;
  m_draw_extent.height = frame.render_target.extent.height;

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

  TransitionImage(cmd, ImageTransitionBarrier(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, VK_ACCESS_2_NONE,
                                              VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                              VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, frame.render_target.image));
  DrawGeometry(cmd, frame.render_target, frame.depth_buffer);
  m_imgui.Draw(cmd, frame.render_target.view, m_draw_extent);

  TransitionImage(cmd, ImageTransitionBarrier(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                              VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                              VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, frame.render_target.image));
  TransitionImage(cmd, ImageTransitionBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                              VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                              VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image));

  CloneImage(cmd, frame.render_target.image, image, m_draw_extent, m_draw_extent);
  TransitionImage(cmd,
                  ImageTransitionBarrier(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                         VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_NONE,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, image));

  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmd_info = CommandBufferSubmitInfo(cmd);
  VkSemaphoreSubmitInfo wait_info =
      SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame.sp_swapchain);
  VkSemaphoreSubmitInfo signal_info = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame.sp_render);

  VkSubmitInfo2 submit = SubmitInfo(&cmd_info, &signal_info, &wait_info);

  VK_CHECK(vkQueueSubmit2(m_device.GetGraphicsQueue(), 1, &submit, frame.fe_render));

  VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = m_swapchain.GetHandlePtr();

  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &frame.sp_render;

  uint32_t current_index = m_swapchain.GetCurrentImageIndex();
  present_info.pImageIndices = &current_index;

  VkResult queue_present = vkQueuePresentKHR(m_device.GetGraphicsQueue(), &present_info);
  if (queue_present == VK_SUBOPTIMAL_KHR || queue_present == VK_ERROR_OUT_OF_DATE_KHR || should_resize) {
    ResizeSwapchain();
  } else {
    VK_CHECK(queue_present);
  }
}

void Renderer::DrawBackground(VkCommandBuffer cmd) {}

void Renderer::InitCommands() {
  VkCommandPoolCreateInfo create_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  create_info.queueFamilyIndex = m_device.GetGraphicsQueueFamily();

  for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
    VK_CHECK(vkCreateCommandPool(m_device.GetDevice(), &create_info, nullptr, &m_frames[i].command_pool));

    VkCommandBufferAllocateInfo alloc_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.commandPool = m_frames[i].command_pool;
    alloc_info.commandBufferCount = 1;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VK_CHECK(vkAllocateCommandBuffers(m_device.GetDevice(), &alloc_info, &m_frames[i].command_buffer));

    // TODO: Yeah we can add the render_targets here, why not?
    // m_frames[i].render_target = AllocatedImage{m_device.GetDevice(), m_allocator, m_draw_extent,
    //                                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    //                                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};
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

void Renderer::InitDescriptors() {}

void Renderer::UpdateDrawImageDescriptors() { throw "no"; }

void Renderer::InitPipelines() {
  // InitTrianglePipeline();
  // InitTriangleMeshPipeline();
  InitTexturedMeshPipeline();
}

void Renderer::InitBackgroundPipelines() {}

void Renderer::InitImmediateSubmit() {
  VkCommandPoolCreateInfo create_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  // create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  // TODO: dynamic
  create_info.queueFamilyIndex = m_device.GetTransferQueueFamily();
  VK_CHECK(vkCreateCommandPool(m_device.GetDevice(), &create_info, nullptr, &m_imm.pool));

  VkCommandBufferAllocateInfo alloc_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  alloc_info.commandPool = m_imm.pool;
  alloc_info.commandBufferCount = 1;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  VK_CHECK(vkAllocateCommandBuffers(m_device.GetDevice(), &alloc_info, &m_imm.cmd));

  VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
  VK_CHECK(vkCreateFence(m_device.GetDevice(), &fence_info, nullptr, &m_imm.fence));

  m_imm.device = m_device.GetDevice();
}

void Renderer::SubmitNow(std::function<void(VkCommandBuffer)> f) {
  if (!m_imm.device) {
    InitImmediateSubmit();
  }

  VK_CHECK(vkResetFences(m_device.GetDevice(), 1, &m_imm.fence));
  VK_CHECK(vkResetCommandPool(m_device.GetDevice(), m_imm.pool, 0));

  VkCommandBufferBeginInfo cmd_begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  cmd_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VK_CHECK(vkBeginCommandBuffer(m_imm.cmd, &cmd_begin));

  f(m_imm.cmd);

  VK_CHECK(vkEndCommandBuffer(m_imm.cmd));

  VkCommandBufferSubmitInfo cmd_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
  cmd_info.commandBuffer = m_imm.cmd;
  VkSubmitInfo2 submit = SubmitInfo(&cmd_info, nullptr, nullptr);

  VK_CHECK(vkQueueSubmit2(m_device.GetTransferQueue(), 1, &submit, m_imm.fence));
  VK_CHECK(vkWaitForFences(m_device.GetDevice(), 1, &m_imm.fence, VK_TRUE, 1000'000'000));
}

void Renderer::CreateDrawImage(VkExtent2D extent) {}

void Renderer::InitTrianglePipeline() {
  auto triangle_vertex = LoadShaderModule("./shaders/triangle.vert.spv", m_device.GetDevice());
  auto triangle_fragment = LoadShaderModule("./shaders/triangle.frag.spv", m_device.GetDevice());

  if (!triangle_vertex || !triangle_fragment) {
    RuntimeError::Throw("Couldn't load triangle shaders!");
  }

  VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  VK_CHECK(vkCreatePipelineLayout(m_device.GetDevice(), &layout_info, nullptr, &m_triangle_pipeline_layout));

  GraphicsPipelineBuilder builder;

  builder.pipeline_layout = m_triangle_pipeline_layout;
  builder.SetShaders(*triangle_vertex, *triangle_fragment);
  builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
  builder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
  builder.DisableMSAA();
  builder.DisableBlending();
  builder.EnableDepthTest();

  builder.SetColorAttachmentFormat(m_frames[0].render_target.format);

  m_triangle_pipeline = builder.Build(m_device.GetDevice());

  vkDestroyShaderModule(m_device.GetDevice(), *triangle_vertex, nullptr);
  vkDestroyShaderModule(m_device.GetDevice(), *triangle_fragment, nullptr);
}

void Renderer::DrawGeometry(VkCommandBuffer cmd, AllocatedImage &render_target, AllocatedImage &depth_buffer) {
  VkClearValue clear_value{{0.0f, 0.0f, 0.0f, 1.0f}};
  VkRenderingAttachmentInfo color_attachment =
      AttachmentInfo(render_target.view, &clear_value, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  clear_value.depthStencil.depth = 1.0f;

  VkRenderingAttachmentInfo depth_attachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
  depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depth_attachment.imageView = depth_buffer.view;
  depth_attachment.clearValue = clear_value;

  VkRenderingInfo rendering_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {.extent = m_draw_extent},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment,
      .pDepthAttachment = &depth_attachment,
  };

  vkCmdBeginRendering(cmd, &rendering_info);

  VkViewport viewport{
      .width = static_cast<float>(m_draw_extent.width),
      .height = static_cast<float>(m_draw_extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor{.extent = m_draw_extent};
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_textured_mesh_pipeline);

  {
    DrawPushConstants push_constants;
    push_constants.vertex_buffer = m_mesh.vertex_addr;
    push_constants.projection = glm::infinitePerspectiveLH_ZO(m_camera.GetFov(),
                                                              static_cast<float>(m_draw_extent.width) /
                                                                  static_cast<float>(m_draw_extent.height),
                                                              0.1f) *
                                m_camera.ViewMatrix();

    vkCmdPushConstants(cmd, m_textured_mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DrawPushConstants),
                       &push_constants);

    UpdateTexturedMeshDescriptors(m_texture);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_textured_mesh_pipeline_layout, 0, 1,
                            &m_textured_mesh_descriptor_set, 0, nullptr);

    vkCmdBindIndexBuffer(cmd, m_mesh.index.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, m_mesh.index.info.size / 4, 1, 0, 0, 0);
  }
  // FIXME: This is a temporary hack to draw the crosshair, which doesn't even work lol.
  // {
  //   DrawPushConstants push_constants;
  //   push_constants.vertex_buffer = m_crosshair_mesh.vertex_addr;
  //   push_constants.projection = glm::orthoLH_ZO(0.0f, static_cast<float>(m_draw_extent.width),
  //                                               static_cast<float>(m_draw_extent.height), 0.0f, 0.1f, 1.0f);

  //   vkCmdPushConstants(cmd, m_textured_mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
  //   sizeof(DrawPushConstants),
  //                      &push_constants);

  //   UpdateTexturedMeshDescriptors(m_crosshair_texture);
  //   vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_textured_mesh_pipeline_layout, 0, 1,
  //                           &m_textured_mesh_descriptor_set, 0, nullptr);

  //   vkCmdBindIndexBuffer(cmd, m_crosshair_mesh.index.buffer, 0, VK_INDEX_TYPE_UINT32);
  //   vkCmdDrawIndexed(cmd, m_crosshair_mesh.index.info.size / 4, 1, 0, 0, 0);
  // }

  vkCmdEndRendering(cmd);
}

void Renderer::InitTriangleMeshPipeline() {
  auto triangle_vertex = LoadShaderModule("./shaders/triangle_mesh.vert.spv", m_device.GetDevice());
  auto triangle_fragment = LoadShaderModule("./shaders/triangle_mesh.frag.spv", m_device.GetDevice());

  if (!triangle_vertex || !triangle_fragment) {
    RuntimeError::Throw("Couldn't load triangle shaders!");
  }

  VkPushConstantRange buffer_range{.stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .size = sizeof(DrawPushConstants)};

  VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  layout_info.pushConstantRangeCount = 1;
  layout_info.pPushConstantRanges = &buffer_range;

  VK_CHECK(vkCreatePipelineLayout(m_device.GetDevice(), &layout_info, nullptr, &m_triangle_mesh_pipeline_layout));

  GraphicsPipelineBuilder builder;

  builder.pipeline_layout = m_triangle_mesh_pipeline_layout;
  builder.SetShaders(*triangle_vertex, *triangle_fragment);
  builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
  builder.SetCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE);
  builder.DisableMSAA();
  builder.DisableBlending();
  builder.EnableDepthTest();

  builder.SetColorAttachmentFormat(m_frames[0].render_target.format);

  m_triangle_mesh_pipeline = builder.Build(m_device.GetDevice());

  vkDestroyShaderModule(m_device.GetDevice(), *triangle_vertex, nullptr);
  vkDestroyShaderModule(m_device.GetDevice(), *triangle_fragment, nullptr);
}

void Renderer::InitTexturedMeshPipeline() {
  auto vertex = LoadShaderModule("./shaders/textured_mesh.vert.spv", m_device.GetDevice());
  auto fragment = LoadShaderModule("./shaders/textured_mesh.frag.spv", m_device.GetDevice());

  if (!vertex || !fragment) {
    RuntimeError::Throw("Couldn't load triangle shaders!");
  }

  VkPushConstantRange buffer_range{.stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .size = sizeof(DrawPushConstants)};

  DescriptorLayoutBuilder descriptor_builder;
  descriptor_builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  m_textured_mesh_descriptor_layout = descriptor_builder.Build(m_device.GetDevice(), VK_SHADER_STAGE_FRAGMENT_BIT);

  VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  layout_info.pushConstantRangeCount = 1;
  layout_info.pPushConstantRanges = &buffer_range;
  layout_info.setLayoutCount = 1;
  layout_info.pSetLayouts = &m_textured_mesh_descriptor_layout;

  VK_CHECK(vkCreatePipelineLayout(m_device.GetDevice(), &layout_info, nullptr, &m_textured_mesh_pipeline_layout));

  GraphicsPipelineBuilder builder;

  builder.pipeline_layout = m_textured_mesh_pipeline_layout;
  builder.SetShaders(*vertex, *fragment);
  builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
  builder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
  builder.DisableMSAA();
  builder.DisableBlending();
  builder.EnableDepthTest();

  builder.SetColorAttachmentFormat(m_frames[0].render_target.format);

  m_textured_mesh_pipeline = builder.Build(m_device.GetDevice());

  vkDestroyShaderModule(m_device.GetDevice(), *vertex, nullptr);
  vkDestroyShaderModule(m_device.GetDevice(), *fragment, nullptr);

  DescriptorAllocator::PoolSizeRatio ratios[] = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1.0f}};
  m_dallocator.InitPool(m_device.GetDevice(), 1, ratios);
  m_textured_mesh_descriptor_set = m_dallocator.Allocate(m_device.GetDevice(), m_textured_mesh_descriptor_layout);
}

void Renderer::InitDefaultData() {
  ChunkMesh mesh = ChunkMesh::GenerateChunkMeshFromChunk(&m_chunk);
  m_device.WaitIdle();
  if (m_mesh.vertex.buffer && m_mesh.index.buffer) {
    DestroyBuffer(m_allocator, std::move(m_mesh.vertex));
    DestroyBuffer(m_allocator, std::move(m_mesh.index));
  }
  m_mesh = UploadMesh(this, m_device.GetDevice(), m_allocator, mesh.indices, mesh.vertices);
  std::cout << m_mesh.index.info.size / 4 << ',' << mesh.indices.size() << std::endl;
}

void Renderer::UpdateTexturedMeshDescriptors(std::shared_ptr<Texture> texture) {
  VkDescriptorImageInfo image_info{};
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_info.imageView = texture->GetView();
  image_info.sampler = texture->GetSampler();

  VkWriteDescriptorSet descriptor_write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  descriptor_write.descriptorCount = 1;
  descriptor_write.dstSet = m_textured_mesh_descriptor_set;
  descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptor_write.pImageInfo = &image_info;

  vkUpdateDescriptorSets(m_device.GetDevice(), 1, &descriptor_write, 0, nullptr);
}

void Renderer::ResizeSwapchain() {
  m_device.WaitIdle();
  auto [width, height] = m_window->GetSize();
  m_swapchain.Resize(width, height);

  m_draw_extent = {width, height};

  for (auto &frame : m_frames) {
    frame.render_target = AllocatedImage{m_device.GetDevice(), m_allocator, m_draw_extent,
                                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};

    frame.depth_buffer = AllocatedImage{m_device.GetDevice(), m_allocator, m_draw_extent,
                                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT};
  }
  // UpdateDrawImageDescriptors();
}
} // namespace craft::vk
