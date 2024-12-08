#pragma once

#include "mat.hpp"

namespace craft {
template <typename T, size_t N> struct Vec : public Mat<T, N, 1> {};

// Common specializations of Vec
using Rect = Vec<float, 4>;
using Rgba8f = Vec<float, 4>;
using Rgba8 = Vec<int, 4>;
} // namespace craft
