#include "mesh.hpp"

#include "math/vec.hpp"
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

enum class MeshFace { Front, Back, Left, Right, Top, Bottom, Count };

static void AddFace(MeshFace face, glm::vec3 pos, Rect uv, std::vector<Vtx> &vertices, std::vector<uint32_t> &indices) {
  uint32_t start_index = vertices.size();

  switch (face) {
  case MeshFace::Left:
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(0, 0, 0), .uv = {uv.x, uv.y}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(0, 1, 0), .uv = {uv.x, uv.y + uv.h}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(1, 1, 0), .uv = {uv.x + uv.w, uv.y + uv.h}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(1, 0, 0), .uv = {uv.x + uv.w, uv.y}});
    break;
  case MeshFace::Right:
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(0, 0, 1), .uv = {uv.x, uv.y}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(1, 0, 1), .uv = {uv.x + uv.w, uv.y}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(1, 1, 1), .uv = {uv.x + uv.w, uv.y + uv.h}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(0, 1, 1), .uv = {uv.x, uv.y + uv.h}});
    break;
  case MeshFace::Front:
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(0, 0, 0), .uv = {uv.x, uv.y}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(0, 0, 1), .uv = {uv.x + uv.w, uv.y}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(0, 1, 1), .uv = {uv.x + uv.w, uv.y + uv.h}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(0, 1, 0), .uv = {uv.x, uv.y + uv.h}});
    break;
  case MeshFace::Back:
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(1, 0, 0), .uv = {uv.x, uv.y}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(1, 1, 0), .uv = {uv.x, uv.y + uv.h}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(1, 1, 1), .uv = {uv.x + uv.w, uv.y + uv.h}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(1, 0, 1), .uv = {uv.x + uv.w, uv.y}});
    break;
  case MeshFace::Bottom:
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(0, 1, 0), .uv = {uv.x, uv.y}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(0, 1, 1), .uv = {uv.x, uv.y + uv.h}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(1, 1, 1), .uv = {uv.x + uv.w, uv.y + uv.h}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(1, 1, 0), .uv = {uv.x + uv.w, uv.y}});
    break;
  case MeshFace::Top:
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(0, 0, 0), .uv = {uv.x, uv.y}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(1, 0, 0), .uv = {uv.x + uv.w, uv.y}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(1, 0, 1), .uv = {uv.x + uv.w, uv.y + uv.h}});
    vertices.emplace_back(Vtx{.pos = pos + glm::vec3(0, 0, 1), .uv = {uv.x, uv.y + uv.h}});
    break;
  default:
    return;
  }

  indices.push_back(start_index + 0);
  indices.push_back(start_index + 1);
  indices.push_back(start_index + 3);
  indices.push_back(start_index + 1);
  indices.push_back(start_index + 2);
  indices.push_back(start_index + 3);
}

static bool ShouldRender(Chunk *chunk, MeshFace face, int z, int x, int y) {
  switch (face) {
  case MeshFace::Front:
    return x == (kMaxChunkWidth - 1) || chunk->blocks[z][x + 1][y].block_type == BlockType::Air;
  case MeshFace::Back:
    return x == 0 || chunk->blocks[z][x - 1][y].block_type == BlockType::Air;
  case MeshFace::Left:
    return z == (kMaxChunkDepth - 1) || chunk->blocks[z + 1][x][y].block_type == BlockType::Air;
  case MeshFace::Right:
    return z == 0 || chunk->blocks[z - 1][x][y].block_type == BlockType::Air;
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

        glm::vec3 block_pos(-x, -y, -z);
        glm::vec3 block_size(1, 1, 1);

        BlockType current_type = block.block_type;
        for (int face = 0; face < 6; face++) {
          MeshFace current_face = static_cast<MeshFace>(face);
          if (!ShouldRender(chunk, current_face, z, x, y))
            continue;

          if (current_type == BlockType::Dirt) {
            if (chunk->blocks[z][x][y + 1].block_type != BlockType::Air) {
              current_type = BlockType::Air;
            }
          }

          Rect tex_coords = TEXTURE_LOOKUP[static_cast<int>(current_type)][static_cast<int>(current_face)];
          float offsetX = 2.0f / 32.0f / 512.0f;
          float offsetY = 2.0f / 32.0f / 512.0f;

          tex_coords.x += offsetX;
          tex_coords.y += offsetY;
          tex_coords.w -= 2.0f * offsetX;
          tex_coords.h -= 2.0f * offsetY;

          AddFace(current_face, block_pos, tex_coords, mesh.vertices, mesh.indices);
        }
      }
    }
  }

  return mesh;
}
} // namespace craft::vk
