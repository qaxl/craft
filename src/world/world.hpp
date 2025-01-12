#pragma once

#include <vector>

#include "world/chunk.hpp"

namespace craft {
class World {
public:
  World() {}

  void Generate() {
    m_chunks.emplace_back();
    GenerateChunk(false, 10.0f, m_noise, m_chunks.back());
  }

private:
  std::vector<Chunk> m_chunks;
};
} // namespace craft
