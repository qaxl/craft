#pragma once

#include <cstdint>

namespace craft {
enum class BlockType : uint8_t {
  Air,
  Dirt,
  Lava,
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
  uint8_t x, z;
  uint8_t y;
  Block blocks[kMaxChunkDepth][kMaxChunkWidth][kMaxChunkHeight];
};

} // namespace craft
