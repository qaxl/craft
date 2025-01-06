#include "mesh.hpp"

#include "renderer.hpp"
#include "world/chunk.hpp"

namespace craft::vk {
MeshBuffers UploadMesh(Renderer *renderer, VkDevice device, VmaAllocator allocator, std::span<uint32_t> indices,
                       std::span<Vtx> vertices) {
  size_t vertex_size = vertices.size_bytes();
  size_t index_size = indices.size_bytes();

  MeshBuffers mesh;
  mesh.vertex = AllocateBuffer(allocator, vertex_size,
                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                               VMA_MEMORY_USAGE_GPU_ONLY);

  VkBufferDeviceAddressInfo addr_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  addr_info.buffer = mesh.vertex.buffer;

  mesh.vertex_addr = vkGetBufferDeviceAddress(device, &addr_info);
  mesh.index =
      AllocateBuffer(allocator, index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VMA_MEMORY_USAGE_GPU_ONLY);

  AllocatedBuffer staging =
      AllocateBuffer(allocator, vertex_size + index_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

  void *data;
  VK_CHECK(vmaMapMemory(allocator, staging.allocation, &data));

  memcpy(data, vertices.data(), vertex_size);
  memcpy(static_cast<char *>(data) + vertex_size, indices.data(), index_size);

  vmaUnmapMemory(allocator, staging.allocation);

  // TODO: fucking optimize
  renderer->SubmitNow([&](VkCommandBuffer cmd) {
    VkBufferCopy vertex_copy{};
    vertex_copy.size = vertex_size;

    vkCmdCopyBuffer(cmd, staging.buffer, mesh.vertex.buffer, 1, &vertex_copy);

    vertex_copy.srcOffset = vertex_size;
    vertex_copy.size = index_size;

    vkCmdCopyBuffer(cmd, staging.buffer, mesh.index.buffer, 1, &vertex_copy);
  });

  DestroyBuffer(allocator, std::move(staging));
  return mesh;
}

ChunkMesh ChunkMesh::GenerateChunkMeshFromChunk(Chunk *chunk) {
  ChunkMesh mesh;

  uint32_t i = 0;
  for (int z = 0; z < kMaxChunkDepth; ++z) {
    for (int x = 0; x < kMaxChunkWidth; ++x) {
      for (int y = 0; y < kMaxChunkHeight; ++y) {
        if (chunk->blocks[z][x][y].block_type == BlockType::Dirt) {
          glm::vec4 grass(64.0f / 512.0f, 32.0f / 512.0f, 96.0f / 512.0f, 64.0f / 512.0f);
          glm::vec4 dirt(0.0f, 32.0f / 512.0f, 32.0f / 512.0f, 64.0f / 512.0f);
          glm::vec4 dirt_with_grass(32.0f / 512.0f, 32.0f / 512.0f, 64.0f / 512.0f, 64.0f / 512.0f);

          std::array<Vtx, 24> vertices{
              // Front face (+Z)
              Vtx{.pos = {1.0f - x, 0.0f - y, 1.0f - z}, .uv = {dirt_with_grass.z, dirt_with_grass.y}},
              Vtx{.pos = {1.0f - x, 1.0f - y, 1.0f - z}, .uv = {dirt_with_grass.z, dirt_with_grass.w}},
              Vtx{.pos = {0.0f - x, 0.0f - y, 1.0f - z}, .uv = {dirt_with_grass.x, dirt_with_grass.y}},
              Vtx{.pos = {0.0f - x, 1.0f - y, 1.0f - z}, .uv = {dirt_with_grass.x, dirt_with_grass.w}},

              // Back face (-Z)
              Vtx{.pos = {0.0f - x, 0.0f - y, 0.0f - z}, .uv = {dirt_with_grass.z, dirt_with_grass.y}},
              Vtx{.pos = {0.0f - x, 1.0f - y, 0.0f - z}, .uv = {dirt_with_grass.z, dirt_with_grass.w}},
              Vtx{.pos = {1.0f - x, 0.0f - y, 0.0f - z}, .uv = {dirt_with_grass.x, dirt_with_grass.y}},
              Vtx{.pos = {1.0f - x, 1.0f - y, 0.0f - z}, .uv = {dirt_with_grass.x, dirt_with_grass.w}},

              // Left face (-X)
              Vtx{.pos = {0.0f - x, 0.0f - y, 1.0f - z}, .uv = {dirt_with_grass.z, dirt_with_grass.y}},
              Vtx{.pos = {0.0f - x, 1.0f - y, 1.0f - z}, .uv = {dirt_with_grass.z, dirt_with_grass.w}},
              Vtx{.pos = {0.0f - x, 0.0f - y, 0.0f - z}, .uv = {dirt_with_grass.x, dirt_with_grass.y}},
              Vtx{.pos = {0.0f - x, 1.0f - y, 0.0f - z}, .uv = {dirt_with_grass.x, dirt_with_grass.w}},

              // Right face (+X)
              Vtx{.pos = {1.0f - x, 0.0f - y, 0.0f - z}, .uv = {dirt_with_grass.z, dirt_with_grass.y}},
              Vtx{.pos = {1.0f - x, 1.0f - y, 0.0f - z}, .uv = {dirt_with_grass.z, dirt_with_grass.w}},
              Vtx{.pos = {1.0f - x, 0.0f - y, 1.0f - z}, .uv = {dirt_with_grass.x, dirt_with_grass.y}},
              Vtx{.pos = {1.0f - x, 1.0f - y, 1.0f - z}, .uv = {dirt_with_grass.x, dirt_with_grass.w}},

              // Top face (+Y)
              Vtx{.pos = {1.0f - x, 1.0f - y, 1.0f - z}, .uv = {dirt.z, dirt.y}},
              Vtx{.pos = {1.0f - x, 1.0f - y, 0.0f - z}, .uv = {dirt.z, dirt.w}},
              Vtx{.pos = {0.0f - x, 1.0f - y, 1.0f - z}, .uv = {dirt.x, dirt.y}},
              Vtx{.pos = {0.0f - x, 1.0f - y, 0.0f - z}, .uv = {dirt.x, dirt.w}},

              // Bottom face (-Y)
              Vtx{.pos = {1.0f - x, 0.0f - y, 0.0f - z}, .uv = {grass.z, grass.y}},
              Vtx{.pos = {1.0f - x, 0.0f - y, 1.0f - z}, .uv = {grass.z, grass.w}},
              Vtx{.pos = {0.0f - x, 0.0f - y, 0.0f - z}, .uv = {grass.x, grass.y}},
              Vtx{.pos = {0.0f - x, 0.0f - y, 1.0f - z}, .uv = {grass.x, grass.w}}};

          mesh.vertices.insert(mesh.vertices.end(), vertices.begin(), vertices.end());

          std::array<uint32_t, 36> indices{// Front face
                                           0 + i, 1 + i, 2 + i, 1 + i, 3 + i, 2 + i,
                                           // Back face
                                           4 + i, 5 + i, 6 + i, 5 + i, 7 + i, 6 + i,
                                           // Left face
                                           8 + i, 9 + i, 10 + i, 9 + i, 11 + i, 10 + i,
                                           // Right face
                                           12 + i, 13 + i, 14 + i, 13 + i, 15 + i, 14 + i,
                                           // Top face
                                           16 + i, 17 + i, 18 + i, 17 + i, 19 + i, 18 + i,
                                           // Bottom face
                                           20 + i, 21 + i, 22 + i, 21 + i, 23 + i, 22 + i};
          i += 24;

          mesh.indices.insert(mesh.indices.end(), indices.begin(), indices.end());
        }
      }
    }
  }

  return mesh;
}
} // namespace craft::vk
