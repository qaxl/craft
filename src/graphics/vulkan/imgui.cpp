#include "imgui.hpp"
#include "utils.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

namespace craft::vk {
ImGui::ImGui(std::shared_ptr<Window> window, Device *device, Swapchain *swapchain)
    : m_instance{device->GetInstance()}, m_device{device} {
  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
  };

  VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1;
  pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(*pool_sizes);
  pool_info.pPoolSizes = pool_sizes;

  VK_CHECK(vkCreateDescriptorPool(m_device->GetDevice(), &pool_info, nullptr, &m_pool));

  ::ImGui::CreateContext();
  ::ImGuiIO &io = ::ImGui::GetIO();
  ::ImGui::StyleColorsLight();

  ImGui_ImplSDL3_InitForVulkan(window->GetHandle());

  // TODO: get real format
  VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
  ImGui_ImplVulkan_InitInfo init_info{
      .Instance = m_instance,
      .PhysicalDevice = m_device->GetPhysicalDevice(),
      .Device = m_device->GetDevice(),
      .Queue = m_device->GetGraphicsQueue(),
      .DescriptorPool = m_pool,
      .MinImageCount = swapchain->GetImageCount(),
      .ImageCount = swapchain->GetImageCount(),
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

    vkDestroyDescriptorPool(m_device->GetDevice(), m_pool, nullptr);
  }
}

void ImGui::Draw(VkCommandBuffer cmd, VkImageView view, VkExtent2D extent) const {
  VkRenderingAttachmentInfo color_attachment = AttachmentInfo(view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkRenderingInfo render_info{VK_STRUCTURE_TYPE_RENDERING_INFO};
  render_info.colorAttachmentCount = 1;
  render_info.pColorAttachments = &color_attachment;
  render_info.renderArea.extent = extent;
  render_info.layerCount = 1;

  vkCmdBeginRendering(cmd, &render_info);

  ImGui_ImplVulkan_RenderDrawData(::ImGui::GetDrawData(), cmd);

  vkCmdEndRendering(cmd);
}
} // namespace craft::vk
