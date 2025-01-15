#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

#include <functional>
#include <memory>
#include <vector>

#include "descriptor.hpp"
#include "device.hpp"
#include "graphics/camera.hpp"
#include "image.hpp"
#include "imgui.hpp"
#include "instance.hpp"
#include "mesh.hpp"
#include "platform/window.hpp"
#include "swapchain.hpp"
#include "util/raii.hpp"
#include "world/world.hpp"

namespace craft::vk {
class Texture;

struct RAIIDestructorForObjects {
  Renderer *renderer = nullptr;

  VmaAllocator allocator = nullptr;

  ~RAIIDestructorForObjects();
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

class Renderer {
public:
  Renderer(std::shared_ptr<Window> window, Camera const &camera, World *chunk);
  ~Renderer();

  Renderer(const Renderer &) = delete;
  Renderer(Renderer &&) = delete;

  FrameData &GetCurrentFrame() { return m_frames[m_frame_number++ % m_swapchain.GetImageCount()]; }

  void Draw();
  void SubmitNow(std::function<void(VkCommandBuffer)> f);

  void InitDefaultData();

private:
  void InitCommands();
  void InitSyncStructures();
  void InitPipelines();
  void InitImmediateSubmit();

  void InitTexturedMeshPipeline();
  void UpdateTexturedMeshDescriptors(std::shared_ptr<Texture> texture);

  void DrawBackground(VkCommandBuffer cmd);
  void DrawGeometry(VkCommandBuffer cmd, AllocatedImage &render_target, AllocatedImage &depth_buffer);

  void ResizeSwapchain();

private:
  std::shared_ptr<Window> m_window;
  Camera const &m_camera;
  World *m_world;

  Instance m_instance;

  Device m_device;
  RAII<VmaAllocator> m_allocator;

  VkSurfaceKHR m_surface;
  VkExtent2D m_draw_extent;

  Swapchain m_swapchain;

  std::shared_ptr<Texture> m_texture;
  std::shared_ptr<Texture> m_crosshair_texture;

  uint32_t m_frame_number = 0;
  std::vector<FrameData> m_frames;

  DescriptorAllocator m_descriptor_allocator;

  VkDescriptorSet m_draw_image_descriptors;
  VkDescriptorSetLayout m_draw_image_descriptor_layout;

  VkPipelineCache m_pipeline_cache;

  ImGui m_imgui;

  VkDescriptorSetLayout m_textured_mesh_descriptor_layout;
  VkDescriptorSet m_textured_mesh_descriptor_set;
  VkPipelineLayout m_textured_mesh_pipeline_layout;
  VkPipeline m_textured_mesh_pipeline;

  std::vector<MeshBuffers> m_meshes{};
  MeshBuffers m_crosshair_mesh{};

  ImmediateSubmit m_imm;
  DescriptorAllocator m_dallocator;

  friend struct RAIIDestructorForObjects;
};
} // namespace craft::vk
