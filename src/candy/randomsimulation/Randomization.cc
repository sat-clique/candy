/* Copyright (c) 2017 Felix Kutzner
 
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
                       std::vector<SimulationVectors::index_t> indices) override;
        
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
                                         std::vector<SimulationVectors::index_t> indices) {
        for (auto index : indices) {
            for (int i = 0; i < SimulationVector::VARSIMVECSIZE; ++i) {
                simVectors.get(index).vars[i] = nextRand();
            }
        }
    }
    
    std::unique_ptr<Randomization> createSimpleRandomization() {
        return backported_std::make_unique<SimpleRandomization>();
    }
}
