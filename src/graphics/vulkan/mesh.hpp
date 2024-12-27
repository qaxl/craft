#pragma once

#include <volk.h>

#include <span>

#include "buffer.hpp"
#include "math/vec.hpp"

namespace craft::vk {
class Renderer;

struct Vertex {
  Vec<float, 3> position;
  Vec<float, 3> normal;
  Vec<float, 2> uv;
  Vec<float, 4> color;
};

struct MeshBuffers {
  AllocatedBuffer index;
  AllocatedBuffer vertex;
  VkDeviceAddress vertex_addr;
};

MeshBuffers UploadMesh(Renderer *renderer, VkDevice device, VmaAllocator allocator, std::span<uint32_t> indices,
                       std::span<Vertex> vertices);

struct DrawPushConstants {
  Mat<float, 4, 4> world;
  VkDeviceAddress vertex_buffer;
};
} // namespace craft::vk
