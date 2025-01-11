#pragma once

#include <volk.h>

#include <fstream>
#include <optional>
#include <string_view>
#include <vector>

#include "util/optimization.hpp"
#include "utils.hpp"

namespace craft::vk {
inline std::optional<VkShaderModule> LoadShaderModule(std::string_view path, VkDevice device) {
  std::ifstream file(path.data(), std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    return std::nullopt;
  }

  size_t size = static_cast<size_t>(file.tellg());
  std::vector<uint32_t> buffer(size / sizeof(uint32_t));

  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()), size);
  file.close();

  VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  info.codeSize = size;
  info.pCode = buffer.data();

  VkShaderModule module;
  // TODO: don't make shader module creations a ciritcal failure?
  VK_CHECK(vkCreateShaderModule(device, &info, nullptr, &module));

  return module;
}

struct GraphicsPipelineBuilder {
  std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
  VkFormat color_attachment_format;
  VkPipelineLayout pipeline_layout;
  VkPipelineColorBlendAttachmentState color_blend_attachment_state;

  VkPipelineVertexInputStateCreateInfo input_state{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state{
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  VkPipelineTessellationStateCreateInfo tess_state{VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};
  VkPipelineViewportStateCreateInfo viewport_state{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  VkPipelineRasterizationStateCreateInfo rasterization_state{
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  VkPipelineMultisampleStateCreateInfo ms_state{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  VkPipelineDepthStencilStateCreateInfo depth_stencil_state{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  VkPipelineColorBlendStateCreateInfo color_blend_state{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  VkPipelineDynamicStateCreateInfo dynamic_state{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  VkPipelineRenderingCreateInfo rendering_info{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};

  FORCE_INLINE void SetShaders(VkShaderModule vertex, VkShaderModule fragment) {
    shader_stages.clear();
    shader_stages.emplace_back(VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                               nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertex, "main"});
    shader_stages.emplace_back(VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                               nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragment,
                                                               "main"});
  }

  FORCE_INLINE void SetInputTopology(VkPrimitiveTopology topology) {
    input_assembly_state.topology = topology;
    input_assembly_state.primitiveRestartEnable = false;
  }

  FORCE_INLINE void SetPolygonMode(VkPolygonMode mode) {
    rasterization_state.polygonMode = mode;
    rasterization_state.lineWidth = 1.0f;
  }

  FORCE_INLINE void SetCullMode(VkCullModeFlags mode, VkFrontFace face) {
    rasterization_state.cullMode = mode;
    rasterization_state.frontFace = face;
  }

  FORCE_INLINE void DisableMSAA() {
    ms_state.sampleShadingEnable = false;
    ms_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    ms_state.minSampleShading = 1.0f;
    ms_state.pSampleMask = nullptr;
    ms_state.alphaToCoverageEnable = false;
    ms_state.alphaToOneEnable = false;
  }

  FORCE_INLINE void DisableBlending() {
    color_blend_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state.blendEnable = false;
  }

  FORCE_INLINE void EnableAlphaBlending() {
    color_blend_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state.blendEnable = true;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
  }

  FORCE_INLINE void SetColorAttachmentFormat(VkFormat format) {
    color_attachment_format = format;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachmentFormats = &color_attachment_format;
  }

  FORCE_INLINE void DisableDepthTest() {
    depth_stencil_state.maxDepthBounds = 1.0f;
    depth_stencil_state.minDepthBounds = 0.0f;
  }

  FORCE_INLINE void EnableDepthTest() {
    depth_stencil_state.depthTestEnable = VK_TRUE;
    depth_stencil_state.depthWriteEnable = VK_TRUE;
    depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state.stencilTestEnable = VK_FALSE;
    rendering_info.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
  }

  VkPipeline Build(VkDevice device) {
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &color_blend_attachment_state;

    VkDynamicState states[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    dynamic_state.pDynamicStates = states;
    dynamic_state.dynamicStateCount = 2;

    VkGraphicsPipelineCreateInfo create_info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    create_info.pNext = &rendering_info;

    create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
    create_info.pStages = shader_stages.data();

    create_info.pVertexInputState = &input_state;
    create_info.pInputAssemblyState = &input_assembly_state;
    create_info.pViewportState = &viewport_state;
    create_info.pRasterizationState = &rasterization_state;
    create_info.pMultisampleState = &ms_state;
    create_info.pDepthStencilState = &depth_stencil_state;
    create_info.pColorBlendState = &color_blend_state;
    create_info.layout = pipeline_layout;
    create_info.pDynamicState = &dynamic_state;

    VkPipeline pipeline;
    if (auto res = vkCreateGraphicsPipelines(device, nullptr, 1, &create_info, nullptr, &pipeline); res != VK_SUCCESS) {
      printf("Couldn't create pipeline because of a Vulkan error: %s\n", VkResultToStr(res).data());
      return nullptr;
    }

    return pipeline;
  }
};
} // namespace craft::vk
