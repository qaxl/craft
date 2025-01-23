#pragma once

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

struct Block {
  BlockType block_type = BlockType::Air;
};

constexpr size_t const kMaxChunkDepth = 32;
constexpr size_t const kMaxChunkWidth = 32;
constexpr size_t const kMaxChunkHeight = 64;

struct Chunk {
  // Z X Y
  int16_t x, z;
  int16_t y;
  Block blocks[kMaxChunkDepth][kMaxChunkWidth][kMaxChunkHeight];
};

} // namespace craft
