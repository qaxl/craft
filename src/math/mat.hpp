#pragma once

#include <utility>

namespace craft {
template <typename T, size_t N, size_t M> struct Mat {
  T v[N][M];

  constexpr Mat() : v{} {}
  constexpr Mat(T ident_value) : v{} {
    for (size_t i = 0; i < M; ++i) {
      v[i][i] = ident_value;
    }
  }

  template <typename... Args> constexpr Mat(Args &&...args) {
    static_assert(sizeof...(args) == N * M, "Number of arguments must match the size of the matrix");
    init(std::forward<Args>(args)...);
  }

private:
  template <typename... Args> constexpr void init(Args &&...args) {
    T flat[] = {static_cast<T>(args)...};
    for (size_t i = 0; i < N * M; ++i) {
      v[i / M][i % M] = flat[i];
    }
  }
};
} // namespace craft
