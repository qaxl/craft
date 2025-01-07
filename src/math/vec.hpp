#pragma once

#include "mat.hpp"

#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>

namespace craft {
template <typename T, size_t N> using Vec = Mat<T, 1, N>;

// Common specializations of Vec

using Vec3f = Vec<float, 3>;
using Vec4f = Vec<float, 4>;

inline glm::vec3 &operator+(const Vec3f &v, glm::vec3 &o) {
  o += glm::vec3(v.v[0][0], v.v[0][1], v.v[0][2]);
  return o;
}

inline glm::vec3 IntoGLM(Vec4f vec) { return glm::vec3(vec.v[0][0], vec.v[0][1], vec.v[0][2]); }

using Rect = Vec4f;

// TODO: specializations of Rgba8, if needed
using Rgba8 = Vec<int, 4>;
using Rgba8f = Vec4f;
} // namespace craft
