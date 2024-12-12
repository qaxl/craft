#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

#include <array>
#include <functional>
#include <memory>
#include <vector>

#include "descriptor.hpp"
#include "image.hpp"
#include "platform/window.hpp"

namespace craft::vk {
struct InstanceExtension {
  // the extension name to enable
  const char *name;
  // whether an extension must be loaded, or only loaded if existing
  bool required = true;
};

struct FrameData {
  VkCommandPool command_pool;
  VkCommandBuffer command_buffer;

  VkSemaphore sp_swapchain;
  VkSemaphore sp_render;
  VkFence fe_render;
};

constexpr const size_t kMaxFramesInFlight = 3;
constexpr const size_t kMinFramesInFlight = 2;

class Renderer {
public:
  Renderer(std::shared_ptr<Window> window);
  ~Renderer();

  // TODO: configurable?
  FrameData &GetCurrentFrame() { return m_frames[m_frame_number++ % kMaxFramesInFlight]; }

  void Draw();
  void SubmitNow(std::function<void(VkCommandBuffer)> f);

private:
  void InitCommands();
  void InitSyncStructures();
  void InitDescriptors();
  void InitPipelines();
  void InitBackgroundPipelines();
  void InitImGui();

  void DrawBackground(VkCommandBuffer cmd);
  void DrawGUI(VkCommandBuffer cmd, VkImageView view);

private:
  std::shared_ptr<Window> m_window;

  VkInstance m_instance;
  VkDebugUtilsMessengerEXT m_messenger;

  VkPhysicalDevice m_physical_device;
  VkDevice m_device;
  VkQueue m_queue;

  VkSurfaceKHR m_surface;
  VkSwapchainKHR m_swapchain;

  std::vector<VkImage> m_swapchain_images;
  std::vector<VkImageView> m_swapchain_image_views;

  uint32_t m_frame_number = 0;
  std::array<FrameData, kMaxFramesInFlight> m_frames;

  VmaAllocator m_allocator;

  AllocatedImage m_draw_image;
  VkExtent2D m_draw_extent;

  DescriptorAllocator m_descriptor_allocator;

  VkDescriptorSet m_draw_image_descriptors;
  VkDescriptorSetLayout m_draw_image_descriptor_layout;

  VkPipeline m_gradient_pipeline;
  VkPipelineLayout m_gradient_pipeline_layout;

  VkPipelineCache m_pipeline_cache;

  // ImGui
  VkDescriptorPool m_imgui_pool;
};
} // namespace craft::vk
