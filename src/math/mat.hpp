#pragma once

#include <cmath>
#include <numbers>
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

  static constexpr Mat<T, 4, 4> perspective(T aspect, T z_near, T z_far, T fov) {
    Mat<T, 4, 4> matrix{};
    T const fov_factor = tan((fov * static_cast<T>(std::numbers::pi) / static_cast<T>(180)) / static_cast<T>(2));

    matrix.v[0][0] = static_cast<T>(1) / (aspect * fov_factor);
    matrix.v[1][1] = static_cast<T>(1) / (fov_factor);
    matrix.v[2][2] = z_far / (z_far - z_near);
    matrix.v[2][3] = static_cast<T>(1);
    matrix.v[3][2] = -(z_far * z_near) / (z_far - z_near);
    // matrix.v[3][2] = -z_near / z_near;

    return matrix;
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
