#pragma once

#include <cmath>
#include <type_traits>

#include "glm.hpp"

namespace craft {
template <typename T, size_t N> struct Vec {
  union {
    T v[N];

    // TODO: a better way to do this?
    struct {
      T x, y, z, w;
    };
  };

  template <typename... Args> constexpr Vec(Args &&...args) : v{static_cast<T>(args)...} {}

  constexpr T &operator[](size_t index) { return v[index]; }
  constexpr const T &operator[](size_t index) const { return v[index]; }

  constexpr glm::vec<N, T> IntoGLMVector() const {
    glm::vec<N, T> val;
    for (size_t i = 0; i < N; ++i) {
      val[i] = v[i];
    }
    return val;
  }

  constexpr static Vec<T, N> FromGLMVector(const glm::vec<N, T> &vec) {
    Vec<T, N> converted;
    for (size_t i = 0; i < N; ++i) {
      converted.v[i] = vec[i];
    }
    return converted;
  }

  constexpr Vec &operator+=(const Vec &other) {
    for (size_t i = 0; i < N; ++i) {
      v[i] += other.v[i];
    }
    return *this;
  }

  constexpr Vec &operator-=(const Vec &other) {
    for (size_t i = 0; i < N; ++i) {
      v[i] -= other.v[i];
    }
    return *this;
  }

  constexpr Vec &operator*=(const Vec &other) {
    for (size_t i = 0; i < N; ++i) {
      v[i] *= other.v[i];
    }
    return *this;
  }

  constexpr Vec &operator/=(const Vec &other) {
    for (size_t i = 0; i < N; ++i) {
      v[i] /= other.v[i];
    }
    return *this;
  }

  constexpr Vec &operator+=(T scalar) {
    for (size_t i = 0; i < N; ++i) {
      v[i] += scalar;
    }
    return *this;
  }

  constexpr Vec &operator-=(T scalar) {
    for (size_t i = 0; i < N; ++i) {
      v[i] -= scalar;
    }
    return *this;
  }

  constexpr Vec &operator*=(T scalar) {
    for (size_t i = 0; i < N; ++i) {
      v[i] *= scalar;
    }
    return *this;
  }

  constexpr Vec &operator/=(T scalar) {
    for (size_t i = 0; i < N; ++i) {
      v[i] /= scalar;
    }
    return *this;
  }

  constexpr bool operator==(const Vec &other) const {
    for (size_t i = 0; i < N; ++i) {
      if (v[i] != other.v[i]) {
        return false;
      }
    }
    return true;
  }

  constexpr bool operator!=(const Vec &other) const { return !(*this == other); }

  constexpr Vec operator+(const Vec &other) const {
    Vec result = *this;
    result += other;
    return result;
  }

  constexpr Vec operator-(const Vec &other) const {
    Vec result = *this;
    result -= other;
    return result;
  }

  constexpr Vec operator*(const Vec &other) const {
    Vec result = *this;
    result *= other;
    return result;
  }

  constexpr Vec operator/(const Vec &other) const {
    Vec result = *this;
    result /= other;
    return result;
  }

  constexpr Vec operator+(T scalar) const {
    Vec result = *this;
    result += scalar;
    return result;
  }

  constexpr Vec operator-(T scalar) const {
    Vec result = *this;
    result -= scalar;
    return result;
  }

  constexpr Vec operator*(T scalar) const {
    Vec result = *this;
    result *= scalar;
    return result;
  }

  constexpr Vec operator/(T scalar) const {
    Vec result = *this;
    result /= scalar;
    return result;
  }

  constexpr Vec operator-() const {
    Vec result = *this;
    for (size_t i = 0; i < N; ++i) {
      result[i] = -result[i];
    }
    return result;
  }

  constexpr Vec Normalize() const {
    T magnitude = 0;
    for (size_t i = 0; i < N; ++i) {
      magnitude += v[i] * v[i];
    }

    if (magnitude == 0) {
      return Vec{};
    }

    magnitude = std::sqrt(magnitude);

    Vec normalized;
    for (size_t i = 0; i < N; ++i) {
      normalized.v[i] = v[i] / magnitude;
    }

    return normalized;
  }
};

// Common specializations of Vec
template <typename T> using Vec2 = Vec<T, 2>;
template <typename T> using Vec3 = Vec<T, 3>;
template <typename T> using Vec4 = Vec<T, 4>;

using Vec2i = Vec2<int64_t>;
using Vec3f = Vec3<float>;
using Vec3i = Vec3<int64_t>;

struct Rect {
  float x, y, w, h;
};

} // namespace craft
