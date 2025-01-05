#pragma once

#include <cstdint>

namespace craft {
enum class BlockType : uint8_t {
  Air,
  Dirt,
};

struct Block {
  BlockType block_type = BlockType::Air;
};

constexpr size_t const kMaxChunkWidth = 32;
constexpr size_t const kMaxChunkHeight = 32;
constexpr size_t const kMaxChunkDepth = 64;

struct Chunk {
  Block blocks[kMaxChunkWidth][kMaxChunkHeight][kMaxChunkDepth];
};
} // namespace craft
