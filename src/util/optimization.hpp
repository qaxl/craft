#pragma once

#if defined(_MSC_VER) && !defined(__clang__)
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE __attribute__((always_inline)) inline
#endif
