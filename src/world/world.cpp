#include "world.hpp"
#include "world/chunk.hpp"

#include <ctime>
#include <iostream>

#include <FastNoiseLite/FastNoiseLite.h>

namespace craft {
World::World() {
  // TODO: customizability
  m_noise.SetSeed(time(nullptr));
  m_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  m_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
  m_noise.SetFractalOctaves(8);
}

void World::LoadChunks(int64_t x, int64_t y, int64_t z, int16_t radius) {
  m_chunks.reserve(radius * radius * radius);

  for (int64_t ix = 0; ix < radius; ++ix) {
    for (int64_t iy = 0; iy < radius; ++iy) {
      for (int64_t iz = 0; iz < radius; ++iz) {
        auto &chunk = m_chunks.emplace_back();
        chunk.world_position = Vec3i(ix, iy, iz);

        for (int64_t x = 0; x < Chunk::kSize; ++x) {
          for (int64_t y = 0; y < Chunk::kSize; ++y) {
            for (int64_t z = 0; z < Chunk::kSize; ++z) {
              float diff = m_noise.GetNoise(static_cast<float>((x + ix) * Chunk::kSize),
                                            static_cast<float>((y + iy) * Chunk::kSize),
                                            static_cast<float>((z + iz) * Chunk::kSize));

              if (diff > 0.0f) {
                chunk.blocks[x][y][z].type = BlockType::Stone;
              } else {
                chunk.blocks[x][y][z].type = BlockType::Air;
              }
              chunk.blocks[x][y][z].type = BlockType::Stone;
              chunk.empty = false;
            }
          }
        }
      }
    }
  }

  // TODO: make something cool happen at y > 100
}

} // namespace craft
