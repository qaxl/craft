#pragma once

#include <volk.h>

#include <glm/glm.hpp>

#include <span>

#include "buffer.hpp"
#include "math/vec.hpp"

namespace craft::vk {
class Renderer;

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
  glm::vec4 color;
  // Vec<float, 3> position;
  // Vec<float, 3> normal;
  // Vec<float, 2> uv;
  // Vec<float, 4> color;
};

/*
struct Vertex {
    vec3 pos;
    float _pad;
    vec2 uv;
    vec2 _pad2;
};*/
struct Vtx {
  glm::vec3 pos;
  float _pad;
  glm::vec2 uv;
  glm::vec2 _pad2;
};

struct Vertex2 {};

struct MeshBuffers {
  AllocatedBuffer index;
  AllocatedBuffer vertex;
  VkDeviceAddress vertex_addr;
};

MeshBuffers UploadMesh(Renderer *renderer, VkDevice device, VmaAllocator allocator, std::span<uint32_t> indices,
                       std::span<Vtx> vertices);

struct DrawPushConstants {
  glm::mat4 projection;
  // Mat<float, 4, 4> world;
  VkDeviceAddress vertex_buffer;
};
} // namespace craft::vk
