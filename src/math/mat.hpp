#pragma once

namespace craft {
template <typename T, size_t N, size_t M> struct Mat {
  T v[N][M];

  template <typename... Args> constexpr Mat(Args &&...args) {
    static_assert(sizeof...(args) == N * M, "Number of arguments must match the size of the matrix");
    init(0, std::forward<Args>(args)...); // Delegate to helper function
  }

private:
  // Helper function to initialize matrix elements recursively
  template <typename... Args> constexpr void init(size_t idx, T value, Args &&...args) {
    v[idx / M][idx % M] = value; // Map 1D index to 2D
    init(idx + 1, std::forward<Args>(args)...);
  }

  constexpr void init(size_t) {}
};
} // namespace craft
