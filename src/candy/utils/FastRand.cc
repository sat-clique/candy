#include "FastRand.h"

#include <random>

namespace Candy {
    FastRandomNumberGenerator::FastRandomNumberGenerator(fastnextrand_state_t seed) noexcept
    : m_state(seed) {
    }
    
    fastnextrand_state_t FastRandomNumberGenerator::operator ()() noexcept {
        m_state = fastNextRand(m_state);
        return m_state;
    }
}
