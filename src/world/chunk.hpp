#pragma once

#include <cstdint>
#include <vector>

namespace craft {
enum class BlockType : uint8_t {
  Air,
  Dirt,
};

struct Block {
  BlockType block_type = BlockType::Air;
};

constexpr size_t const kMaxChunkDepth = 32;
constexpr size_t const kMaxChunkWidth = 32;
constexpr size_t const kMaxChunkHeight = 64;

struct Chunk {
  // Z X Y
  Block blocks[kMaxChunkDepth][kMaxChunkWidth][kMaxChunkHeight];
};

} // namespace craft
