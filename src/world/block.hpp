#pragma once

#include "math/vec.hpp"

#include <cstdint>

namespace craft {
enum class BlockType : uint8_t {
  Air,
  Dirt,
  Lava,
  Water,
  Stone,
  Wood,
  Count,
};

constexpr Vec3i kTextureAtlasCoords[] = {Vec3i(0, 0, 0), Vec3i(0, 2, 1), Vec3i(0, 2, 0),
                                         Vec3i(0, 3, 0), Vec3i(0, 1, 0), Vec3i(0, 0, 0)};

struct Block {
  BlockType type = BlockType::Air;

  Vec3i GetTextureAtlasCoords() { return kTextureAtlasCoords[static_cast<size_t>(type)]; }
};
} // namespace craft