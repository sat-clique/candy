#ifndef _6B81F094_386B_489F_8DD3_CC3F531C5F42_SIMULATIONVECTOR_H
#define _6B81F094_386B_489F_8DD3_CC3F531C5F42_SIMULATIONVECTOR_H

#include <cstdint>
#include <cassert>
#include <memory>

// Compiler option RANDSIM_VARSIMVECSIZE - the simulation vector siz in multiples
// of 64 single-variable assignments. By default, RANDSIM_VARSIMVECSIZE
// is set to 32 (i.e., 2048 single-variable assignments).
#ifndef RANDSIM_VARSIMVECSIZE
#define RANDSIM_VARSIMVECSIZE 32
#endif

namespace randsim {
    
    class SimulationVector {
    public:
        static const uint8_t VARSIMVECSIZE = RANDSIM_VARSIMVECSIZE;
        typedef std::uint64_t varsimvec_field_t;
        static const unsigned int VARSIMVECVARS = RANDSIM_VARSIMVECSIZE * sizeof(varsimvec_field_t);
        
        varsimvec_field_t vars[VARSIMVECSIZE];
        
        inline SimulationVector operator~() {
            SimulationVector result;
            for (int i = 0; i < VARSIMVECSIZE; i++) {
                result.vars[i] = ~vars[i];
            }
            return result;
        }
        
        inline SimulationVector operator!() {
            SimulationVector result;
            for (int i = 0; i < VARSIMVECSIZE; i++) {
                result.vars[i] = ~vars[i];
            }
            return result;
        }
        
        inline SimulationVector operator|(SimulationVector const &r) {
            SimulationVector result;
            for (int i = 0; i < VARSIMVECSIZE; i++) {
                result.vars[i] = vars[i] | r.vars[i];
            }
            return result;
        }
        
        inline SimulationVector &operator|=(SimulationVector const &r) {
            for (int i = 0; i < VARSIMVECSIZE; i++) {
                vars[i] |= r.vars[i];
            }
            return *this;
        }
        
        inline SimulationVector &operator&=(SimulationVector const &r) {
            for (int i = 0; i < VARSIMVECSIZE; i++) {
                vars[i] &= r.vars[i];
            }
            return *this;
        }
        
        inline SimulationVector operator^(SimulationVector const &r) {
            SimulationVector result;
            for (int i = 0; i < VARSIMVECSIZE; i++) {
                result.vars[i] = vars[i] ^ r.vars[i];
            }
            return result;
        }
        
        
        inline bool
        operator==(SimulationVector const &other) {
            bool result = true;
            for (int i = 0; i < VARSIMVECSIZE; i++) {
                result &= (vars[i] == other.vars[i]);
            }
            return result;
        }
        
        void initialize(varsimvec_field_t pattern);
        
        ~SimulationVector();
    };
    
    class SimulationVectors {
    public:
        typedef size_t index_t;
        
        SimulationVectors();
        ~SimulationVectors();
        
        void initialize(unsigned int size);
        
        inline SimulationVector& get(unsigned int index) {
            assert(m_isInitialized);
            assert(index < m_size);
            return m_simulationVectors[index];
        }
        
    private:
        SimulationVector *m_simulationVectors;
        unsigned int m_size;
        std::unique_ptr<char> m_rawMemory;
        bool m_isInitialized;
    };
}

#endif
