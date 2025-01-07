#pragma once

#include <cmath>
#include <numbers>
#include <utility>

#include <glm/glm.hpp>

#include "util/optimization.hpp"

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
    // static_assert(sizeof...(args) == N * M, "Number of arguments must match the size of the matrix");
    init(std::forward<Args>(args)...);
  }

  FORCE_INLINE float *operator[](size_t index) { return v[index]; }

  static constexpr Mat<T, 4, 4> InfinitePerspective(T aspect, T fov, T z_near) {
    T const range = tan(fov / static_cast<T>(2)) * z_near;
    T const right = range * aspect;
    T const left = -right;
    T const top = -range;
    T const bottom = range;

    Mat<T, 4, 4> matrix{};
    matrix[0][0] = static_cast<T>(2) * z_near / (right - left);
    matrix[1][1] = static_cast<T>(2) * z_near / (top - bottom);
    matrix[2][2] = static_cast<T>(1);
    matrix[2][3] = static_cast<T>(1);
    matrix[3][2] = -z_near;

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

// Common matrix specializations
using Mat4f = Mat<float, 4, 4>;
using Mat4d = Mat<double, 4, 4>;
} // namespace craft
