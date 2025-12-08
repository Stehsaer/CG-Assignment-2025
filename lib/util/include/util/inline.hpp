#pragma once

#ifdef _MSC_VER
#define FORCE_INLINE [[msvc::forceinline]]
#elif defined(__clang__) || defined(__GNUC__)
#define FORCE_INLINE [[gnu::always_inline]]
#else
#define FORCE_INLINE
#endif