#ifndef _67D7538E_89FB_4F0D_BE32_5F8C2266D1E2_FASTRAND_H
#define _67D7538E_89FB_4F0D_BE32_5F8C2266D1E2_FASTRAND_H

// todo: namespace?

typedef std::uint64_t fastnextrand_state_t;

inline fastnextrand_state_t fastNextRand(fastnextrand_state_t state) {
    const std::uint64_t multArg = 2685821657736338717ull;
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return state * multArg;
}


#endif
