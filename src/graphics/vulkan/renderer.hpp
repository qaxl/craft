#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

#include <array>
#include <functional>
#include <memory>
#include <vector>

#include "descriptor.hpp"
#include "device.hpp"
#include "graphics/camera.hpp"
#include "image.hpp"
#include "imgui.hpp"
#include "math/vec.hpp"
#include "mesh.hpp"
#include "platform/window.hpp"
#include "swapchain.hpp"

namespace craft::vk {
class Texture;

struct Instance {
  VkInstance instance{};
  VkSurfaceKHR surface{};

  ~Instance() {
    if (surface)
      vkDestroySurfaceKHR(instance, surface, nullptr);

    if (instance)
      vkDestroyInstance(instance, nullptr);
  }
};

struct Allocator {
  VmaAllocator allocator{};

  ~Allocator() {
    if (allocator) {
      vmaDestroyAllocator(allocator);
    }
  }
};

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

  AllocatedImage render_target;
  AllocatedImage depth_buffer;
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

struct ImmediateSubmit {
  VkFence fence{};
  VkCommandBuffer cmd{};
  VkCommandPool pool{};
  VkDevice device{};

  ~ImmediateSubmit() {
    vkDestroyCommandPool(device, pool, nullptr);
    vkDestroyFence(device, fence, nullptr);
  }
};

constexpr const size_t kMaxFramesInFlight = 3;
constexpr const size_t kMinFramesInFlight = 2;

class Renderer {
public:
  Renderer(std::shared_ptr<Window> window, Camera const &camera, Chunk &chunk);
  ~Renderer();

  Renderer(const Renderer &) = delete;
  Renderer(Renderer &&) = delete;

  // TODO: configurable?
  FrameData &GetCurrentFrame() { return m_frames[m_frame_number++ % kMaxFramesInFlight]; }

  // ComputeEffect &GetCurrentEffect() { return m_bg_effects[m_current_bg_effect]; }
  // void SetCurrentEffect(int index) { m_current_bg_effect = index % m_bg_effects.size(); }

  void Draw();
  void SubmitNow(std::function<void(VkCommandBuffer)> f);

  void InitDefaultData();

private:
  void InitCommands();
  void InitSyncStructures();
  void InitDescriptors();
  void InitPipelines();
  void InitBackgroundPipelines();
  void CreateDrawImage(VkExtent2D);
  void UpdateDrawImageDescriptors();
  void InitTrianglePipeline();
  void InitTriangleMeshPipeline();
  void InitImmediateSubmit();

  void InitTexturedMeshPipeline();
  void UpdateTexturedMeshDescriptors(std::shared_ptr<Texture> texture);

  void DrawBackground(VkCommandBuffer cmd);
  void DrawGeometry(VkCommandBuffer cmd, AllocatedImage &render_target, AllocatedImage &depth_buffer);

  void ResizeSwapchain();

private:
  std::shared_ptr<Window> m_window;
  Camera const &m_camera;
  Chunk &m_chunk;

  Instance m_instance_raii;
  VkInstance m_instance;
  VkDebugUtilsMessengerEXT m_messenger;

  Device m_device;
  Swapchain m_swapchain;

  VmaAllocator m_allocator;
  Allocator m_allocator_raii;

  VkSurfaceKHR m_surface;
  std::shared_ptr<Texture> m_texture;
  std::shared_ptr<Texture> m_crosshair_texture;

  uint32_t m_frame_number = 0;
  std::array<FrameData, kMaxFramesInFlight> m_frames;

  // AllocatedImage m_draw_image;
  VkExtent2D m_draw_extent;

  DescriptorAllocator m_descriptor_allocator;

  VkDescriptorSet m_draw_image_descriptors;
  VkDescriptorSetLayout m_draw_image_descriptor_layout;

  // VkPipeline m_gradient_pipeline;
  // VkPipelineLayout m_gradient_pipeline_layout;

  VkPipelineCache m_pipeline_cache;

  // Custom backgrounds ;)
  // int m_current_bg_effect = 0;
  // std::vector<ComputeEffect> m_bg_effects;

  ImGui m_imgui;

  VkPipelineLayout m_triangle_pipeline_layout;
  VkPipeline m_triangle_pipeline;

  VkPipelineLayout m_triangle_mesh_pipeline_layout;
  VkPipeline m_triangle_mesh_pipeline;

  VkDescriptorSetLayout m_textured_mesh_descriptor_layout;
  VkDescriptorSet m_textured_mesh_descriptor_set;
  VkPipelineLayout m_textured_mesh_pipeline_layout;
  VkPipeline m_textured_mesh_pipeline;

  MeshBuffers m_mesh{};
  MeshBuffers m_crosshair_mesh{};

  ImmediateSubmit m_imm;
  DescriptorAllocator m_dallocator;
};
} // namespace craft::vk
