#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

#include <array>
#include <functional>
#include <memory>
#include <vector>

#include "descriptor.hpp"
#include "device.hpp"
#include "image.hpp"
#include "imgui.hpp"
#include "math/vec.hpp"
#include "platform/window.hpp"
#include "swapchain.hpp"

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

struct ComputePushConstants {
  std::array<Vec<float, 4>, 4> data;
};

struct ComputeEffect {
  std::string_view name;

  VkPipeline pipeline;
  VkPipelineLayout layout;

  ComputePushConstants pc;
};

constexpr const size_t kMaxFramesInFlight = 3;
constexpr const size_t kMinFramesInFlight = 2;

class Renderer {
public:
  Renderer(std::shared_ptr<Window> window);
  ~Renderer();

  // TODO: configurable?
  FrameData &GetCurrentFrame() { return m_frames[m_frame_number++ % kMaxFramesInFlight]; }

  ComputeEffect &GetCurrentEffect() { return m_bg_effects[m_current_bg_effect]; }
  void SetCurrentEffect(int index) { m_current_bg_effect = index % m_bg_effects.size(); }

  void Draw();
  void SubmitNow(std::function<void(VkCommandBuffer)> f);

private:
  void InitCommands();
  void InitSyncStructures();
  void InitDescriptors();
  void InitPipelines();
  void InitBackgroundPipelines();

  void DrawBackground(VkCommandBuffer cmd);

private:
  std::shared_ptr<Window> m_window;

  VkInstance m_instance;
  VkDebugUtilsMessengerEXT m_messenger;

  Device m_device;

  VkSurfaceKHR m_surface;
  std::shared_ptr<Swapchain> m_swapchain;

  uint32_t m_frame_number = 0;
  std::array<FrameData, kMaxFramesInFlight> m_frames;

  VmaAllocator m_allocator;

  AllocatedImage m_draw_image;
  VkExtent2D m_draw_extent;

  DescriptorAllocator m_descriptor_allocator;

  VkDescriptorSet m_draw_image_descriptors;
  VkDescriptorSetLayout m_draw_image_descriptor_layout;

  // VkPipeline m_gradient_pipeline;
  VkPipelineLayout m_gradient_pipeline_layout;

  VkPipelineCache m_pipeline_cache;

  // Custom backgrounds ;)
  int m_current_bg_effect = 0;
  std::vector<ComputeEffect> m_bg_effects;

  // ImGui (temp)
  std::shared_ptr<ImGui> m_imgui;
};
} // namespace craft::vk
