#pragma once

#include <vector>

#include <FastNoiseLite/FastNoiseLite.h>

#include "generator.hpp"
#include "world/chunk.hpp"

namespace craft {
class World {
public:
  World(FastNoiseLite *noise) : m_noise{noise} {}

  void Generate() {
    for (int x = 0; x < 32; ++x) {
      for (int z = 0; z < 32; ++z) {
        auto &chunk = m_chunks.emplace_back();
        GenerateChunk(false, 12.0f, *m_noise, chunk, 1.0f, x * kMaxChunkWidth, z * kMaxChunkDepth);
        chunk.x = x;
        chunk.z = z;
        chunk.y = 0;
      }
    }
  }

  Chunk *GetChunk(int index) { return &m_chunks[index]; }
  std::vector<Chunk> &GetChunks() { return m_chunks; }

private:
  std::vector<Chunk> m_chunks;
  FastNoiseLite *m_noise;
};
} // namespace craft
