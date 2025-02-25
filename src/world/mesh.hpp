#pragma once

#include "chunk.hpp"

#include <vector>

namespace craft {
struct Vertex {
  uint32_t data;

  Vertex(uint8_t x, uint8_t y, uint8_t z, uint8_t face, uint16_t tex_id, uint8_t ao) : data{} {
    data |= (x & 0x1F) << 0;     // 5 bits (0-  5)
    data |= (y & 0x1F) << 5;     // 5 bits (5- 10)
    data |= (z & 0x1F) << 10;    // 5 bits (10-15)
    data |= (face & 0x07) << 15; // 3 bits (15-18)
    // data |= (tex_id & 0x1FF) << 18; // 9 bits (18-27)
    // data |= (ao & 0x03) << 27;      // 2 bits (27-29)
    // 2 bits (29-31), unused.
  }

  operator uint32_t() { return data; }
};

using Face = Vertex;

struct ChunkMesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  void GenerateChunkMesh(Chunk *chunk);
};

struct MeshChunk {
  std::vector<Face> faces;

  void GenerateChunkMesh(Chunk *chunk);
};
} // namespace craft
