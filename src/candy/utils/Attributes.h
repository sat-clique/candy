#ifndef SRC_CANDY_UTILS_ATTRIBUTES_H_
#define SRC_CANDY_UTILS_ATTRIBUTES_H_

#if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
  #define ATTR_ALWAYSINLINE __attribute__((always_inline))
  #define ATTR_ALIGNED(x) __attribute__((__aligned__(x)))
  #define ATTR_RESTRICT __restrict__
  #define ASSUME(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
  #define LIKELY(expr) __builtin_expect(!(expr), 0)
  #define UNLIKELY(expr) __builtin_expect((expr),  0)
#elif defined(_MSC_VER)
  // TODO
  #define ATTR_ALWAYSINLINE
  #define ATTR_ALIGNED(x)
  #define ATTR_RESTRICT
  #define ASSUME(cond) __assume(cond);
  #define LIKELY(expr)
  #define UNLIKELY(expr)
#endif

#endif
