/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.
 
 */

// TODO: documentation

#include "Randomization.h"

#include <utils/FastRand.h>
#include <utils/MemUtils.h>

namespace Candy {
    Randomization::~Randomization() {
        
    }
    
    Randomization::Randomization() {
        
    }
    
    class SimpleRandomization : public Randomization {
    public:
        SimpleRandomization();
        
        void randomize(SimulationVectors &simVectors,
                       const std::vector<SimulationVectors::index_t>& indices) override;
        
        virtual ~SimpleRandomization();
        
        SimpleRandomization(const SimpleRandomization &other) = delete;
        SimpleRandomization& operator=(const SimpleRandomization& other) = delete;
        
    private:
        fastnextrand_state_t nextRand();
        
        fastnextrand_state_t m_rand_state;
    };
    
    SimpleRandomization::SimpleRandomization() : m_rand_state(0xFFFF) {
        
    }
    
    SimpleRandomization::~SimpleRandomization() {
        
    }
    
    fastnextrand_state_t SimpleRandomization::nextRand() {
        m_rand_state = fastNextRand(m_rand_state);
        return m_rand_state;
    }
    
    void SimpleRandomization::randomize(SimulationVectors &simVectors,
                                        const std::vector<SimulationVectors::index_t>& indices) {
        for (auto index : indices) {
            for (int i = 0; i < SimulationVector::VARSIMVECSIZE; ++i) {
                simVectors.get(index).vars[i] = nextRand();
            }
        }
    }
    
    std::unique_ptr<Randomization> createSimpleRandomization() {
        return backported_std::make_unique<SimpleRandomization>();
    }
    
    
    
    std::unique_ptr<Randomization> createRandomizationBiasedToTrue(unsigned int bias) {
        return createRandomizationCyclicallyBiasedToTrue(bias, bias, 0);
    }
    
    std::unique_ptr<Randomization> createRandomizationBiasedToFalse(unsigned int bias) {
        return createRandomizationCyclicallyBiasedToFalse(bias, bias, 0);
    }
    
    
    
    class RandomizationCyclicallyBiased : public Randomization {
    public:
        RandomizationCyclicallyBiased(unsigned int minBias,
                                      unsigned int maxBias,
                                      unsigned int step,
                                      bool biasToTrue) noexcept;
        
        void randomize(SimulationVectors &simVectors,
                       const std::vector<SimulationVectors::index_t>& indices) override;
        
        virtual ~RandomizationCyclicallyBiased();
        
        RandomizationCyclicallyBiased(const RandomizationCyclicallyBiased &other) = delete;
        RandomizationCyclicallyBiased& operator=(const RandomizationCyclicallyBiased& other) = delete;
        
    private:
        fastnextrand_state_t nextRand();
        
        void randomizeWithFalseBias(SimulationVectors &simVectors,
                                    const std::vector<SimulationVectors::index_t>& indices);
        void randomizeWithTrueBias(SimulationVectors &simVectors,
                                   const std::vector<SimulationVectors::index_t>& indices);
        
        unsigned int getCurrentBias() const noexcept;
        
        fastnextrand_state_t m_rand_state;
        unsigned int m_minBias;
        unsigned int m_maxBias;
        unsigned int m_step;
        unsigned int m_biasToTrue;
        unsigned int m_incovations;
    };
    
    RandomizationCyclicallyBiased::RandomizationCyclicallyBiased(unsigned int minBias,
                                                                 unsigned int maxBias,
                                                                 unsigned int step,
                                                                 bool biasToTrue) noexcept
    : Candy::Randomization(),
    m_rand_state(0xFFFF),
    m_minBias(minBias),
    m_maxBias(maxBias),
    m_step(step),
    m_biasToTrue(biasToTrue),
    m_incovations(0) {
        
    }
    
    RandomizationCyclicallyBiased::~RandomizationCyclicallyBiased() {
    }
    
    void RandomizationCyclicallyBiased::randomize(SimulationVectors &simVectors,
                                                  const std::vector<SimulationVectors::index_t>& indices) {
        for (auto index : indices) {
            for (int i = 0; i < SimulationVector::VARSIMVECSIZE; ++i) {
                simVectors.get(index).vars[i] = nextRand();
            }
        }
        
        if (m_biasToTrue) {
            randomizeWithTrueBias(simVectors, indices);
        }
        else {
            randomizeWithFalseBias(simVectors, indices);
        }
        ++m_incovations;
    }
    
    unsigned int RandomizationCyclicallyBiased::getCurrentBias() const noexcept {
        return (m_minBias + m_incovations * m_step) % (m_maxBias+1);
    }
    
    void RandomizationCyclicallyBiased::randomizeWithTrueBias(SimulationVectors &simVectors,
                                                              const std::vector<SimulationVectors::index_t>& indices) {
        auto currentBias = getCurrentBias();
        if (currentBias > 1) {
            for (auto index : indices) {
                for (int i = 0; i < SimulationVector::VARSIMVECSIZE; ++i) {
                    for (unsigned int j = 1; j < currentBias; ++j) {
                        simVectors.get(index).vars[i] |= nextRand();
                    }
                }
            }
        }
    }
    
    void RandomizationCyclicallyBiased::randomizeWithFalseBias(SimulationVectors &simVectors,
                                                               const std::vector<SimulationVectors::index_t>& indices) {
        auto currentBias = getCurrentBias();
        if (currentBias > 1) {
            for (auto index : indices) {
                for (int i = 0; i < SimulationVector::VARSIMVECSIZE; ++i) {
                    for (unsigned int j = 1; j < currentBias; ++j) {
                        simVectors.get(index).vars[i] &= nextRand();
                    }
                }
            }
        }
    }
    
    fastnextrand_state_t RandomizationCyclicallyBiased::nextRand() {
        m_rand_state = fastNextRand(m_rand_state);
        return m_rand_state;
    }
    
    
    std::unique_ptr<Randomization> createRandomizationCyclicallyBiasedToTrue(unsigned int minBias,
                                                                             unsigned int maxBias,
                                                                             unsigned int step) {
        return backported_std::make_unique<RandomizationCyclicallyBiased>(minBias, maxBias, step, true);
    }
    
    std::unique_ptr<Randomization> createRandomizationCyclicallyBiasedToFalse(unsigned int minBias,
                                                                              unsigned int maxBias,
                                                                              unsigned int step) {
        return backported_std::make_unique<RandomizationCyclicallyBiased>(minBias, maxBias, step, false);
    }
    
    
    
    
    std::unique_ptr<Randomization> alternateRandomizations(std::unique_ptr<Randomization> rand1,
                                                           std::unique_ptr<Randomization> rand2,
                                                           unsigned int period) {
        (void)rand1;
        (void)rand2;
        (void)period;
        assert(false); /* remains to be implemented */
        return std::unique_ptr<Randomization>{};
    }
}
