#include "mesh.hpp"

#include "math/vec.hpp"
#include "renderer.hpp"
#include "world/chunk.hpp"

namespace craft::vk {
MeshBuffers UploadMesh(Renderer *renderer, VkDevice device, VmaAllocator allocator, std::span<uint32_t> indices,
                       std::span<Vertex> vertices) {
  size_t vertex_size = vertices.size_bytes();
  size_t index_size = indices.size_bytes();

  MeshBuffers mesh;
  mesh.vertex = AllocateBuffer(allocator, vertex_size + index_size,
                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                               VMA_MEMORY_USAGE_GPU_ONLY);

  VkBufferDeviceAddressInfo addr_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  addr_info.buffer = mesh.vertex.buffer;

  mesh.vertex_addr = vkGetBufferDeviceAddress(device, &addr_info);

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
    vertex_copy.size = vertex_size + index_size;

    vkCmdCopyBuffer(cmd, staging.buffer, mesh.vertex.buffer, 1, &vertex_copy);

    // vertex_copy.srcOffset = vertex_size;
    // vertex_copy.size = index_size;

    // vkCmdCopyBuffer(cmd, staging.buffer, mesh.index.buffer, 1, &vertex_copy);
  });

  DestroyBuffer(allocator, std::move(staging));
  mesh.vertex_size = vertex_size;
  mesh.index_size = index_size;
  return mesh;
}

enum class MeshFace { Front, Back, Left, Right, Top, Bottom, Count };

static bool ShouldRender(Chunk *chunk, MeshFace face, int z, int x, int y) {
  switch (face) {
  case MeshFace::Front:
    return z == (kMaxChunkDepth - 1) || chunk->blocks[z + 1][x][y].block_type == BlockType::Air;
  case MeshFace::Back:
    return z == 0 || chunk->blocks[z - 1][x][y].block_type == BlockType::Air;
  case MeshFace::Left:
    return x == 0 || chunk->blocks[z][x - 1][y].block_type == BlockType::Air;
  case MeshFace::Right:
    return x == (kMaxChunkWidth - 1) || chunk->blocks[z][x + 1][y].block_type == BlockType::Air;
  case MeshFace::Top:
    return y == (kMaxChunkHeight - 1) || chunk->blocks[z][x][y + 1].block_type == BlockType::Air;
  case MeshFace::Bottom:
    return y == 0 || chunk->blocks[z][x][y - 1].block_type == BlockType::Air;

  default:
    return true;
  }
}

ChunkMesh ChunkMesh::GenerateChunkMeshFromChunk(Chunk *chunk) {
  ChunkMesh mesh;
  mesh.indices.reserve(1 << 20);
  mesh.vertices.reserve(1 << 21);

  static constexpr Rect GRASS_TEXTURE = {64.0f / 512.0f, 32.0f / 512.0f, 32.0f / 512.0f, 32.0f / 512.0f};
  static constexpr Rect DIRT_TEXTURE = {0.0f / 512.0f, 32.0f / 512.0f, 32.0f / 512.0f, 32.0f / 512.0f};
  static constexpr Rect DIRT_WITH_GRASS = {32.0f / 512.0f, 32.0f / 512.0f, 32.0f / 512.0f, 32.0f / 512.0f};
  static constexpr Rect LAVA_TEXTURE = {64.0f / 512.0f, 0.0f / 512.0f, 32.0f / 512.0f, 32.0f / 512.0f};

  static constexpr Rect TEXTURE_LOOKUP[static_cast<int>(BlockType::Count)][static_cast<int>(MeshFace::Count)] = {
      {DIRT_TEXTURE, DIRT_TEXTURE, DIRT_TEXTURE, DIRT_TEXTURE, DIRT_TEXTURE, DIRT_TEXTURE},
      {DIRT_WITH_GRASS, DIRT_WITH_GRASS, DIRT_WITH_GRASS, DIRT_WITH_GRASS, GRASS_TEXTURE, DIRT_TEXTURE},
      {LAVA_TEXTURE, LAVA_TEXTURE, LAVA_TEXTURE, LAVA_TEXTURE, LAVA_TEXTURE, LAVA_TEXTURE},
  };

  for (int z = 0; z < kMaxChunkDepth; ++z) {
    for (int x = 0; x < kMaxChunkWidth; ++x) {
      for (int y = 0; y < kMaxChunkHeight; ++y) {
        const auto &block = chunk->blocks[z][x][y];
        if (block.block_type == BlockType::Air)
          continue;

        BlockType current_type = block.block_type;
        for (int face = 0; face < 6; face++) {
          MeshFace current_face = static_cast<MeshFace>(face);
          if (!ShouldRender(chunk, current_face, z, x, y))
            continue;

          uint8_t tex_index = 1;
          switch (current_type) {
          case BlockType::Dirt:
            if (current_face == MeshFace::Top) {
              tex_index = 18;
            } else if (current_face == MeshFace::Bottom) {
              tex_index = 16;
            } else {
              tex_index = 17;
            }
            break;
          case BlockType::Lava:
            tex_index = 2;
            break;
          case BlockType::Water:
            tex_index = 3;
            break;

          case BlockType::Stone:
            tex_index = 1;
            break;

          case BlockType::Wood:
            tex_index = 0;
            break;

          case BlockType::Air:
          case BlockType::Count:
            break;
          }

          uint32_t base_index = mesh.vertices.size();

          mesh.vertices.push_back(Vertex(x, y, z, face, 0, tex_index, 0));
          mesh.vertices.push_back(Vertex(x, y, z, face, 1, tex_index, 0));
          mesh.vertices.push_back(Vertex(x, y, z, face, 2, tex_index, 0));
          mesh.vertices.push_back(Vertex(x, y, z, face, 3, tex_index, 0));

          mesh.indices.push_back(base_index);
          mesh.indices.push_back(base_index + 1);
          mesh.indices.push_back(base_index + 2);
          mesh.indices.push_back(base_index);
          mesh.indices.push_back(base_index + 2);
          mesh.indices.push_back(base_index + 3);
        }
      }
    }
  }

  return mesh;
}
} // namespace craft::vk
