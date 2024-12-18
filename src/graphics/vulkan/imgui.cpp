#include "imgui.hpp"
#include "renderer.hpp"
#include "utils.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

namespace craft::vk {
ImGui::ImGui(VkInstance instance, VkPhysicalDevice gpu, VkDevice device, std::shared_ptr<Window> window, VkQueue queue,
             uint32_t queue_family)
    : m_instance{instance}, m_device{device} {
  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
  };

  VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1;
  pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(*pool_sizes);
  pool_info.pPoolSizes = pool_sizes;

  VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &m_pool));

  ::ImGui::CreateContext();
  ::ImGuiIO &io = ::ImGui::GetIO();

  ImGui_ImplSDL3_InitForVulkan(window->GetHandle());

  // TODO: get real format
  VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;
  ImGui_ImplVulkan_InitInfo init_info{
      .Instance = instance,
      .PhysicalDevice = gpu,
      .Device = device,
      .Queue = queue,
      .DescriptorPool = m_pool,
      .MinImageCount = kMinFramesInFlight,
      // TODO: get the real frame count?
      .ImageCount = kMaxFramesInFlight,
      .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
      .UseDynamicRendering = true,
      .PipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO, nullptr, 0, 1, &format},
      .CheckVkResultFn = [](VkResult res) { CheckResult(res); },
  };

  ImGui_ImplVulkan_LoadFunctions(
      [](const char *function_name, void *u) {
        return vkGetInstanceProcAddr(static_cast<ImGui *>(u)->m_instance, function_name);
      },
      this);

  ImGui_ImplVulkan_Init(&init_info);
  ImGui_ImplVulkan_CreateFontsTexture();
}

ImGui::~ImGui() {
  if (m_device && m_instance && m_pool) {
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplVulkan_Shutdown();
    ::ImGui::DestroyContext();

    vkDestroyDescriptorPool(m_device, m_pool, nullptr);
  }
}

void ImGui::Draw(VkCommandBuffer cmd, VkImageView view) const {
  VkRenderingAttachmentInfo color_attachment = AttachmentInfo(view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkRenderingInfo render_info{VK_STRUCTURE_TYPE_RENDERING_INFO};
  render_info.colorAttachmentCount = 1;
  render_info.pColorAttachments = &color_attachment;
  // TODO: where the fuck to get?
  render_info.renderArea.extent = {.width = 1024, .height = 768};
  render_info.layerCount = 1;

  vkCmdBeginRendering(cmd, &render_info);

  ImGui_ImplVulkan_RenderDrawData(::ImGui::GetDrawData(), cmd);

  vkCmdEndRendering(cmd);
}
} // namespace craft::vk
