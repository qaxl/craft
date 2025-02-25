#pragma once

#include <vector>

#include <FastNoiseLite/FastNoiseLite.h>

#include "world/chunk.hpp"

namespace craft {
class World {
public:
  World();

  void LoadChunks(int64_t x, int64_t y, int64_t z, int16_t radius = 2);

  Chunk *GetChunk(int index) { return &m_chunks[index]; }
  std::vector<Chunk> &GetChunks() { return m_chunks; }

private:
  FastNoiseLite m_noise;

  std::vector<Chunk> m_chunks;
};
} // namespace craft
