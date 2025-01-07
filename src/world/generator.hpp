#pragma once

#include "chunk.hpp"

#include <FastNoiseLite/FastNoiseLite.h>

#include <iostream>

namespace craft {
inline void GenerateChunk(bool generate_only_one_block, float max_generated_height, FastNoiseLite &noise, Chunk &out,
                          float scale = 10.0f) {
  memset(out.blocks, 0, sizeof(out.blocks));
  if (generate_only_one_block) {
    out.blocks[0][0][0].block_type = BlockType::Dirt;
    return;
  }

#pragma omp parallel for
  for (int z = 0; z < kMaxChunkDepth; ++z) {
    for (int x = 0; x < kMaxChunkWidth; ++x) {
      float height = noise.GetNoise(static_cast<float>(x) * scale, static_cast<float>(z) * scale);
      height = (height + 1.0f) / 2.0f;
      height *= max_generated_height;

      uint32_t y = (2 + static_cast<uint32_t>(height)) % kMaxChunkHeight;
      std::cout << y << std::endl;
      // out.blocks[y][x][z].block_type = BlockType::Dirt;
      for (uint32_t yy = y; yy > 0; --yy) {
        out.blocks[z][x][yy].block_type = BlockType::Dirt;
      }
    }
  }
}

} // namespace craft
