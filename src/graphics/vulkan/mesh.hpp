#pragma once

#include <volk.h>

#include <span>

#include "buffer.hpp"
#include "math/glm.hpp"
#include "world/chunk.hpp"
#include "world/mesh.hpp"

namespace craft::vk {
class Renderer;

struct MeshBuffers {
  AllocatedBuffer vertex;
  VkDeviceAddress vertex_addr;
  Chunk *chunk;
  uint32_t vertex_size, index_size;
};

struct DrawPushConstants {
  glm::mat4 projection;
  VkDeviceAddress vertex_buffer;
};

struct MeshPushConstants {
  glm::mat4 projection;
  uint32_t data_offset;
  uint32_t face_count;
  VkDeviceAddress vertex_buffer;
};
} // namespace craft::vk
