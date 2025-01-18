#include "renderer.hpp"

#include <array>
#include <iostream>
#include <vector>

#include <SDL3/SDL_vulkan.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "descriptor.hpp"
#include "device.hpp"
#include "mesh.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include "texture.hpp"
#include "util/error.hpp"
#include "utils.hpp"
#include "world/chunk.hpp"

namespace craft::vk {
constexpr DeviceFeatures const kDeviceFeatures = DeviceFeatures{
    .base_features = {.sampleRateShading = true, .samplerAnisotropy = true},
    .vk_1_2_features = {.bufferDeviceAddress = true},
    .vk_1_3_features =
        {
            .synchronization2 = true,
            .dynamicRendering = true,
        },
};

static VmaAllocator CreateAllocator(Device *device) {
  VmaVulkanFunctions funcs{.vkGetInstanceProcAddr = vkGetInstanceProcAddr, .vkGetDeviceProcAddr = vkGetDeviceProcAddr};

  VmaAllocatorCreateInfo allocator_info{};
  allocator_info.physicalDevice = device->GetPhysicalDevice();
  allocator_info.device = device->GetDevice();
  allocator_info.instance = device->GetInstance();
  allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  allocator_info.pVulkanFunctions = &funcs;

  VmaAllocator allocator;
  VK_CHECK(vmaCreateAllocator(&allocator_info, &allocator));

  return allocator;
}

Renderer::Renderer(std::shared_ptr<Window> window, Camera const &camera, World *world)
    : m_window{window}, m_camera{camera}, m_world{world}, m_instance{},
      m_device{m_instance.GetInstance(), {DeviceExtension{VK_KHR_SWAPCHAIN_EXTENSION_NAME}}, &kDeviceFeatures},
      m_surface{m_window->CreateSurface(m_instance.GetInstance())}, m_draw_extent{m_window->GetExtent()},
      m_swapchain{&m_device, m_surface, m_draw_extent},
      m_allocator{CreateAllocator(&m_device), [](VmaAllocator a) { vmaDestroyAllocator(a); }} {

  m_frames.resize(m_swapchain.GetImageCount());

  for (auto &frame : m_frames) {
    frame.render_target = AllocatedImage{m_device.GetDevice(), *m_allocator, m_draw_extent,
                                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};

    frame.depth_buffer = AllocatedImage{m_device.GetDevice(), *m_allocator, m_draw_extent,
                                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT};
  }

  InitCommands();
  InitSyncStructures();
  InitPipelines();
  InitDefaultData();

  std::array<Vtx, 4> vertices = {
      Vtx{.pos = {m_draw_extent.width / 2 - 15, m_draw_extent.height / 2 - 15, 1}, .uv = {0, 0}},
      Vtx{.pos = {m_draw_extent.width / 2 - 15, m_draw_extent.height / 2 + 15, 1}, .uv = {0, 16.0f / 512.0f}},
      Vtx{.pos = {m_draw_extent.width / 2 + 15, m_draw_extent.height / 2 + 15, 1},
          .uv = {16.0f / 512.0f, 16.0f / 512.0f}},
      Vtx{.pos = {m_draw_extent.width / 2 + 15, m_draw_extent.height / 2 - 15, 1}, .uv = {16.0f / 512.0f, 0}},
  };
  std::array<uint32_t, 6> indices = {0, 1, 3, 1, 2, 3};

  m_crosshair_mesh = UploadMesh(this, m_device.GetDevice(), *m_allocator, indices, vertices);
  m_crosshair_texture = std::make_shared<Texture>(*m_allocator, m_device, this, "textures/crosshair.png");

  m_texture = std::make_shared<Texture>(*m_allocator, m_device, this, "textures/spritesheet_blocks.png");
  m_imgui = ImGui{m_window, &m_device, &m_swapchain};

  UpdateTexturedMeshDescriptors(m_texture);
}

Renderer::~Renderer() {
  m_device.WaitIdle();

  for (auto &mesh : m_meshes) {
    DestroyBuffer(*m_allocator, std::move(mesh.index));
    DestroyBuffer(*m_allocator, std::move(mesh.vertex));
  }
  m_meshes.clear();

  vkDestroyPipelineLayout(m_device.GetDevice(), m_textured_mesh_pipeline_layout, nullptr);
  vkDestroyPipeline(m_device.GetDevice(), m_textured_mesh_pipeline, nullptr);

  vkDestroyDescriptorSetLayout(m_device.GetDevice(), m_textured_mesh_descriptor_layout, nullptr);
  m_dallocator.DestroyPool(m_device.GetDevice());

  for (auto &frame : m_frames) {
    vkDestroyFence(m_device.GetDevice(), frame.fe_render, nullptr);
    vkDestroySemaphore(m_device.GetDevice(), frame.sp_render, nullptr);
    vkDestroySemaphore(m_device.GetDevice(), frame.sp_swapchain, nullptr);

    vkDestroyCommandPool(m_device.GetDevice(), frame.command_pool, nullptr);
  }
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

  for (auto &frame : m_frames) {
    VK_CHECK(vkCreateCommandPool(m_device.GetDevice(), &create_info, nullptr, &frame.command_pool));

    VkCommandBufferAllocateInfo alloc_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.commandPool = frame.command_pool;
    alloc_info.commandBufferCount = 1;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VK_CHECK(vkAllocateCommandBuffers(m_device.GetDevice(), &alloc_info, &frame.command_buffer));
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

void Renderer::InitPipelines() { InitTexturedMeshPipeline(); }

void Renderer::InitImmediateSubmit() {
  VkCommandPoolCreateInfo create_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  // TODO: dynamic
  create_info.queueFamilyIndex = m_device.GetGraphicsQueueFamily();
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

  VK_CHECK(vkQueueSubmit2(m_device.GetGraphicsQueue(), 1, &submit, m_imm.fence));
  VK_CHECK(vkWaitForFences(m_device.GetDevice(), 1, &m_imm.fence, VK_TRUE, 1000'000'000));
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
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_textured_mesh_pipeline_layout, 0, 1,
                          &m_textured_mesh_descriptor_set, 0, nullptr);

  {
    size_t index = 0;
    for (auto &mesh : m_meshes) {
      DrawPushConstants push_constants;
      push_constants.vertex_buffer = mesh.vertex_addr;
      // Position meshes in a grid with proper spacing (16 units between chunks)
      push_constants.projection =
          glm::perspective(m_camera.GetFov(),
                           static_cast<float>(m_draw_extent.width) / static_cast<float>(m_draw_extent.height), 1.0f,
                           m_camera.GetFarPlane()) *
          m_camera.ViewMatrix() *
          glm::translate(glm::mat4(1.0f), glm::vec3(mesh.chunk->x * kMaxChunkWidth, mesh.chunk->y * kMaxChunkHeight,
                                                    mesh.chunk->z * kMaxChunkDepth));

      vkCmdPushConstants(cmd, m_textured_mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DrawPushConstants),
                         &push_constants);

      vkCmdBindIndexBuffer(cmd, mesh.index.buffer, 0, VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(cmd, mesh.index.info.size / 4, 1, 0, 0, 0);

      index += 1;
    }
  }
  // FIXME: This is a temporary hack to draw the crosshair
  {
    DrawPushConstants push_constants;
    push_constants.vertex_buffer = m_crosshair_mesh.vertex_addr;
    push_constants.projection = glm::orthoLH_ZO(0.0f, static_cast<float>(m_draw_extent.width),
                                                static_cast<float>(m_draw_extent.height), 0.0f, 0.1f, 10.0f);

    vkCmdPushConstants(cmd, m_textured_mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DrawPushConstants),
                       &push_constants);

    vkCmdBindIndexBuffer(cmd, m_crosshair_mesh.index.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, m_crosshair_mesh.index.info.size / 4, 1, 0, 0, 0);
  }

  vkCmdEndRendering(cmd);
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
  builder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
  builder.DisableMSAA();
  builder.EnableAlphaBlending();
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
  m_device.WaitIdle();
  for (auto &mesh : m_meshes) {
    DestroyBuffer(*m_allocator, std::move(mesh.index));
    DestroyBuffer(*m_allocator, std::move(mesh.vertex));
  }
  m_meshes.clear();

  for (auto &chunk : m_world->GetChunks()) {
    ChunkMesh mesh = ChunkMesh::GenerateChunkMeshFromChunk(&chunk);
    auto &mesh_ =
        m_meshes.emplace_back(UploadMesh(this, m_device.GetDevice(), *m_allocator, mesh.indices, mesh.vertices));
    mesh_.chunk = &chunk;
  }
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

  // FIXME: temporary
  std::array<Vtx, 4> vertices = {
      Vtx{.pos = {m_draw_extent.width / 2 - 10, m_draw_extent.height / 2 - 10, 1}, .uv = {0, 0}},
      Vtx{.pos = {m_draw_extent.width / 2 - 10, m_draw_extent.height / 2 + 10, 1}, .uv = {0, 16.0f / 512.0f}},
      Vtx{.pos = {m_draw_extent.width / 2 + 10, m_draw_extent.height / 2 + 10, 1},
          .uv = {16.0f / 512.0f, 16.0f / 512.0f}},
      Vtx{.pos = {m_draw_extent.width / 2 + 10, m_draw_extent.height / 2 - 10, 1}, .uv = {16.0f / 512.0f, 0}},
  };
  std::array<uint32_t, 6> indices = {0, 1, 3, 1, 2, 3};

  m_crosshair_mesh = UploadMesh(this, m_device.GetDevice(), *m_allocator, indices, vertices);
  // END OF FIXME

  for (auto &frame : m_frames) {
    frame.render_target = AllocatedImage{m_device.GetDevice(), *m_allocator, m_draw_extent,
                                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};

    frame.depth_buffer = AllocatedImage{m_device.GetDevice(), *m_allocator, m_draw_extent,
                                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT};
  }
}

RAIIDestructorForObjects::~RAIIDestructorForObjects() {
  if (allocator) {
    vmaDestroyAllocator(allocator);
  }
}

} // namespace craft::vk
