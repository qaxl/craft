#pragma once

#include "block.hpp"
#include "math/vec.hpp"

namespace craft {
struct Chunk {
  static constexpr size_t const kSize = 32;

  bool empty = true;
  Vec3i world_position;
  Block blocks[kSize][kSize][kSize];
};
} // namespace craft
