#include "Randomization.h"

#include <utils/FastRand.h>

namespace randsim {
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
        return std::make_unique<SimpleRandomization>();
    }
}
