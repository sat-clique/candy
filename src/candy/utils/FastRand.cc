#include "FastRand.h"

#include <random>

namespace Candy {
    FastRandomNumberGenerator::FastRandomNumberGenerator(fastnextrand_state_t seed) noexcept
    : m_state(seed) {
    }
}
