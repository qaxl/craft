#include "mesh.hpp"

namespace craft {
enum class FaceID { Front, Back, Left, Right, Top, Bottom };
static inline bool ShouldMesh(Chunk *chunk, size_t x, size_t y, size_t z, FaceID face) {
  // FIXME: temp
  return true;

  switch (face) {
  case FaceID::Front:
    return z == (Chunk::kSize - 1) || chunk->blocks[z + 1][x][y].type == BlockType::Air;
  case FaceID::Back:
    return z == 0 || chunk->blocks[z - 1][x][y].type == BlockType::Air;
  case FaceID::Left:
    return x == 0 || chunk->blocks[z][x - 1][y].type == BlockType::Air;
  case FaceID::Right:
    return x == (Chunk::kSize - 1) || chunk->blocks[z][x + 1][y].type == BlockType::Air;
  case FaceID::Top:
    return y == (Chunk::kSize - 1) || chunk->blocks[z][x][y + 1].type == BlockType::Air;
  case FaceID::Bottom:
    return y == 0 || chunk->blocks[z][x][y - 1].type == BlockType::Air;

  default:
    return true;
  }
}

void ChunkMesh::GenerateChunkMesh(Chunk *chunk) {
  for (size_t x = 0; x < Chunk::kSize; ++x) {
    for (size_t y = 0; y < Chunk::kSize; ++y) {
      for (size_t z = 0; z < Chunk::kSize; ++z) {
        Block &block = chunk->blocks[x][y][z];
        if (block.type == BlockType::Air) {
          continue;
        }

        for (int face = 0; face < 6; ++face) {
          if (!ShouldMesh(chunk, x, y, z, static_cast<FaceID>(face))) {
            continue;
          }
          uint32_t base_index = vertices.size();

          Vec3i coords = block.GetTextureAtlasCoords();
          Vec2i uv = Vec2i(coords.y, coords.z);

          uint16_t tex_index = 0;

          vertices.push_back(Vertex(x, y, z, face, tex_index, 0));
          vertices.push_back(Vertex(x, y, z, face, tex_index, 0));
          vertices.push_back(Vertex(x, y, z, face, tex_index, 0));
          vertices.push_back(Vertex(x, y, z, face, tex_index, 0));

          indices.push_back(base_index);
          indices.push_back(base_index + 1);
          indices.push_back(base_index + 2);
          indices.push_back(base_index);
          indices.push_back(base_index + 2);
          indices.push_back(base_index + 3);
        }
      }
    }
  }
}

void MeshChunk::GenerateChunkMesh(Chunk *chunk) {
  for (size_t x = 0; x < Chunk::kSize; ++x) {
    for (size_t y = 0; y < Chunk::kSize; ++y) {
      for (size_t z = 0; z < Chunk::kSize; ++z) {
        Block &block = chunk->blocks[x][y][z];
        if (block.type == BlockType::Air) {
          continue;
        }

        for (int face = 0; face < 6; ++face) {
          if (!ShouldMesh(chunk, x, y, z, static_cast<FaceID>(face))) {
            continue;
          }

          faces.emplace_back(Face(x, y, z, face, 0, 0));
        }
      }
    }
  }
}

} // namespace craft
