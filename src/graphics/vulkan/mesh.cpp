#include "mesh.hpp"

#include "platform/window.hpp"
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

enum class MeshFace { Front, Back, Left, Right, Top, Bottom };

static void AddQuad(const glm::vec3 top_left, const glm::vec3 right, const glm::vec3 bottom, glm::vec4 uv,
                    std::vector<Vtx> &vertices, std::vector<uint32_t> &indices) {
  uint32_t start_index = static_cast<uint32_t>(vertices.size());
  glm::vec2 uv_coords[] = {{uv.x, uv.w}, {uv.z, uv.w}, {uv.x, uv.y}, {uv.z, uv.y}};

  vertices.push_back(Vtx{.pos = top_left, .uv = uv_coords[0]});
  vertices.push_back(Vtx{.pos = top_left + right, .uv = uv_coords[1]});
  vertices.push_back(Vtx{.pos = top_left + bottom, .uv = uv_coords[2]});
  vertices.push_back(Vtx{.pos = top_left + right + bottom, .uv = uv_coords[3]});

  indices.push_back(start_index + 0);
  indices.push_back(start_index + 1);
  indices.push_back(start_index + 2);
  indices.push_back(start_index + 1);
  indices.push_back(start_index + 3);
  indices.push_back(start_index + 2);
}

static void AddFace(MeshFace face, glm::vec3 pos, glm::vec3 size, glm::vec4 uv, std::vector<Vtx> &vertices,
                    std::vector<uint32_t> &indices) {
  switch (face) {
  case MeshFace::Front:
    AddQuad(pos + glm::vec3(size.x, size.y, size.z), glm::vec3(-size.x, 0, 0), glm::vec3(0, -size.y, 0), uv, vertices,
            indices);
    break;
  case MeshFace::Back:
    AddQuad(pos + glm::vec3(0, size.y, 0), glm::vec3(size.x, 0, 0), glm::vec3(0, -size.y, 0), uv, vertices, indices);
    break;
  case MeshFace::Left:
    AddQuad(pos + glm::vec3(0, size.y, size.z), glm::vec3(0, 0, -size.z), glm::vec3(0, -size.y, 0), uv, vertices,
            indices);
    break;
  case MeshFace::Right:
    AddQuad(pos + glm::vec3(size.x, size.y, 0), glm::vec3(0, 0, size.z), glm::vec3(0, -size.y, 0), uv, vertices,
            indices);
    break;
  case MeshFace::Top:
    AddQuad(pos + glm::vec3(0, size.y, 0), glm::vec3(size.x, 0, 0), glm::vec3(0, 0, size.z), uv, vertices, indices);
    break;
  case MeshFace::Bottom:
    AddQuad(pos + glm::vec3(0, 0, size.z), glm::vec3(size.x, 0, 0), glm::vec3(0, 0, -size.z), uv, vertices, indices);
    break;
  }
}

static bool ShouldRender(Chunk *chunk, MeshFace face, int z, int x, int y) {
  // TODO: implement this
  // // Check bounds first
  // if (z < 0 || x < 0 || y < 0 || z >= kMaxChunkDepth || x >= kMaxChunkWidth || y >= kMaxChunkHeight) {
  //   return true;
  // }

  // // Check if neighboring block is air
  // switch (face) {
  // case MeshFace::Front:
  //   return z + 1 >= kMaxChunkDepth || chunk->blocks[z + 1][x][y].block_type == BlockType::Air;
  // case MeshFace::Back:
  //   return z - 1 < 0 || chunk->blocks[z - 1][x][y].block_type == BlockType::Air;
  // case MeshFace::Left:
  //   return x - 1 < 0 || chunk->blocks[z][x - 1][y].block_type == BlockType::Air;
  // case MeshFace::Right:
  //   return x + 1 >= kMaxChunkWidth || chunk->blocks[z][x + 1][y].block_type == BlockType::Air;
  // case MeshFace::Top:
  //   return y + 1 >= kMaxChunkHeight || chunk->blocks[z][x][y + 1].block_type == BlockType::Air;
  // case MeshFace::Bottom:
  //   return y - 1 < 0 || chunk->blocks[z][x][y - 1].block_type == BlockType::Air;
  // }
  // return false;

  return true;
}

ChunkMesh ChunkMesh::GenerateChunkMeshFromChunk(Chunk *chunk) {
  ChunkMesh mesh;
  mesh.indices.reserve(1 << 20);
  mesh.vertices.reserve(1 << 21);

  static constexpr glm::vec4 GRASS_TEXTURE = {64.0f / 512.0f, 32.0f / 512.0f, 96.0f / 512.0f, 64.0f / 512.0f};
  static constexpr glm::vec4 DIRT_TEXTURE = {0.0f, 32.0f / 512.0f, 32.0f / 512.0f, 64.0f / 512.0f};
  static constexpr glm::vec4 DIRT_WITH_GRASS = {32.0f / 512.0f, 32.0f / 512.0f, 64.0f / 512.0f, 64.0f / 512.0f};

  for (int z = 0; z < kMaxChunkDepth; ++z) {
    for (int x = 0; x < kMaxChunkWidth; ++x) {
      for (int y = 0; y < kMaxChunkHeight; ++y) {
        const auto &block = chunk->blocks[z][x][y];
        if (block.block_type == BlockType::Air)
          continue;

        glm::vec3 block_pos(-x, -y, -z);
        glm::vec3 block_size(1, 1, 1);

        BlockType current_type = block.block_type;
        if (current_type == BlockType::Dirt) {
          for (int face = 0; face < 6; face++) {
            MeshFace current_face = static_cast<MeshFace>(face);
            if (!ShouldRender(chunk, current_face, z, x, y))
              continue;

            glm::vec4 tex_coords;
            if (current_face == MeshFace::Top) {
              tex_coords = GRASS_TEXTURE;
            } else if (current_face == MeshFace::Bottom) {
              tex_coords = DIRT_TEXTURE;
            } else {
              tex_coords = DIRT_WITH_GRASS;
            }

            AddFace(current_face, block_pos, block_size, tex_coords, mesh.vertices, mesh.indices);
          }
        }
      }
    }
  }

  return mesh;
}
} // namespace craft::vk
