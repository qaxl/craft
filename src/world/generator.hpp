#pragma once

#include "chunk.hpp"

#include <FastNoiseLite/FastNoiseLite.h>

#include <cstring>

namespace craft {
inline void GenerateChunk(bool generate_only_one_block, float max_generated_height, FastNoiseLite &noise, Chunk &out,
                          float scale = 10.0f, int start_x = 0, int start_z = 0) {
  memset(out.blocks, 0, sizeof(out.blocks));

#pragma omp parallel for
  for (int z = 0; z < kMaxChunkDepth; ++z) {
    for (int x = 0; x < kMaxChunkWidth; ++x) {
      float height = noise.GetNoise(static_cast<float>(start_x + x) * scale, static_cast<float>(start_z + z) * scale);
      height = (height + 1.0f) / 2.0f;
      height *= max_generated_height;

      uint32_t y_ = (2 + static_cast<uint32_t>(height)) % kMaxChunkHeight;
      for (uint32_t y = y_; y > 0; --y) {
        if (y > 5) {
          out.blocks[z][x][y].block_type = BlockType::Dirt;
        } else if ((y > 3 && y <= 5)) {
          out.blocks[z][x][y].block_type = BlockType::Water;
        } else {
          out.blocks[z][x][y].block_type = BlockType::Stone;
        }
      }
    }
  }
}

} // namespace craft
