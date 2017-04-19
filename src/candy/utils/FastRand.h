#ifndef _67D7538E_89FB_4F0D_BE32_5F8C2266D1E2_FASTRAND_H
#define _67D7538E_89FB_4F0D_BE32_5F8C2266D1E2_FASTRAND_H

#include <cstdint>

namespace Candy {

    typedef std::uint64_t fastnextrand_state_t;
    
    /**
     * An xorshift* RNG function implementation
     * (see Sebastiano Vigna, "An experimental exploration of Marsaglia's
     * xorshift generators, scrambled",
     * http://vigna.di.unimi.it/ftp/papers/xorshift.pdf)
     *
     * \param state     The RNG state. \p state must be nonzero.
     */
    inline fastnextrand_state_t fastNextRand(fastnextrand_state_t state) {
        const std::uint64_t multArg = 2685821657736338717ull;
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return state * multArg;
    }
    
    /**
     * \brief A fast XOR-shift pseurorandom number generator with 64 bits of state.
     */
    class FastRandomNumberGenerator {
    public:
        /**
         * Constructs an instance of FastRandomNumberGenerator.
         *
         * \param seed      The random number generator's seed. \p seed must be nonzero.
         */
        explicit FastRandomNumberGenerator(fastnextrand_state_t seed) noexcept;
        
        /**
         * Computes and returns the next pseudorandom number.
         *
         * \returns the next pseudorandom number.
         */
        fastnextrand_state_t operator ()() noexcept;
        
        /**
         * Gets the current pseudorandom number.
         *
         * \returns the current pseudorandom number.
         */
        fastnextrand_state_t currentNumber() const noexcept;
        
    private:
        fastnextrand_state_t m_state;
    };
    
    inline fastnextrand_state_t FastRandomNumberGenerator::operator ()() noexcept {
        m_state = fastNextRand(m_state);
        return m_state;
    }
    
    inline fastnextrand_state_t FastRandomNumberGenerator::currentNumber() const noexcept {
        return m_state;
    }
}

#endif
