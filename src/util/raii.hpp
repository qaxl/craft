#pragma once

#include <memory>

#include "util/optimization.hpp"

namespace craft {
// Utility class for RAII-style destruction of objects in C APIs.
template <typename T> class RAII {
public:
  using DestroyFunction = void (*)(T);

  RAII(T &&resource, DestroyFunction f) : m_resource{std::move(resource)} {}
  ~RAII() { m_f(m_resource); }

  RAII(const RAII &) = delete;
  RAII(RAII &&other) = delete;
  RAII &operator=(const RAII &) = delete;
  RAII &operator=(RAII &&other) = delete;

  FORCE_INLINE T &operator*() { return m_resource; }
  FORCE_INLINE T *operator&() { return &m_resource; }

private:
  T m_resource;
  DestroyFunction m_f;
};
} // namespace craft
