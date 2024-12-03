#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

#include <array>
#include <memory>
#include <vector>

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
constexpr const size_t kMinFramesInFlight = 1;

class Renderer {
public:
  Renderer(std::shared_ptr<Window> window);
  ~Renderer();

  // TODO: configurable?
  FrameData &GetCurrentFrame() { return m_frames[m_frame_number++ % kMaxFramesInFlight]; }

  void Draw();

private:
  void InitCommands();
  void InitSyncStructures();

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
};
} // namespace craft::vk
