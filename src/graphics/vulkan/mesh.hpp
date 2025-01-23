#pragma once

#include <volk.h>

#include <glm/glm.hpp>

#include <span>

#include "buffer.hpp"
#include "world/chunk.hpp"

namespace craft::vk {
class Renderer;

struct Vertex {
  uint32_t data = 0;

  Vertex(uint8_t x, uint8_t y, uint8_t z, uint8_t face, uint8_t corner, uint16_t tex_id, uint8_t ao) {
    data |= (x & 0x1F) << 0;        // 5 bits
    data |= (y & 0x3F) << 5;        // 6 bits
    data |= (z & 0x1F) << 11;       // 5 bits
    data |= (face & 0x07) << 16;    // 3 bits
    data |= (corner & 0x03) << 19;  // 2 bits
    data |= (tex_id & 0x1FF) << 21; // 9 bits
    data |= (ao & 0x03) << 30;      // 2 bits
  }
};

/*
struct Vertex {
    vec3 pos;
    float _pad;
    vec2 uv;
    vec2 _pad2;
};*/
struct Vtx_ {
  glm::vec3 pos;
  float _pad;
  glm::vec2 uv;
  glm::vec2 _pad2;
};
struct MeshBuffers {
  AllocatedBuffer vertex;
  VkDeviceAddress vertex_addr;
  Chunk *chunk;
  uint32_t vertex_size, index_size;
};

MeshBuffers UploadMesh(Renderer *renderer, VkDevice device, VmaAllocator allocator, std::span<uint32_t> indices,
                       std::span<Vertex> vertices);

struct DrawPushConstants {
  glm::mat4 projection;
  // Mat<float, 4, 4> world;
  VkDeviceAddress vertex_buffer;
};

struct ChunkMesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  static ChunkMesh GenerateChunkMeshFromChunk(Chunk *chunk);
};
} // namespace craft::vk
