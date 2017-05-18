#ifndef SRC_CANDY_UTILS_ATTRIBUTES_H_
#define SRC_CANDY_UTILS_ATTRIBUTES_H_

#if defined(__gcc__) || defined(__clang__)
  #define ATTR_ALWAYSINLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
  // TODO
  #define ATTR_ALWAYSINLINE
#endif

#endif
