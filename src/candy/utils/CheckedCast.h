#ifndef SRC_CANDY_UTILS_CHECKEDCAST_H_
#define SRC_CANDY_UTILS_CHECKEDCAST_H_

#include <limits>

#define DEBUG_CHECK_CASTS

template<typename FromType, typename ToType>
inline ToType checked_unsignedtosigned_cast(FromType value) {
#ifdef DEBUG_CHECK_CASTS
    assert(value <= std::numeric_limits<ToType>::max());
#endif
    return static_cast<ToType>(value);
}

template<typename FromType, typename ToType>
inline ToType checked_unsigned_cast(FromType value) {
#ifdef DEBUG_CHECK_CASTS
    assert(value <= std::numeric_limits<ToType>::max());
#endif
    return static_cast<ToType>(value);
}

#endif