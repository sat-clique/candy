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

#ifndef X_6B81F094_386B_489F_8DD3_CC3F531C5F42_SIMULATIONVECTOR_H
#define X_6B81F094_386B_489F_8DD3_CC3F531C5F42_SIMULATIONVECTOR_H

#include <cstdint>
#include <cassert>
#include <memory>

#include <candy/core/SolverTypes.h>
#include <candy/utils/Attributes.h>

// Compiler option RANDSIM_VARSIMVECSIZE - the simulation vector siz in multiples
// of 64 single-variable assignments. By default, RANDSIM_VARSIMVECSIZE
// is set to 32 (i.e., 2048 single-variable assignments).
#ifndef RANDSIM_VARSIMVECSIZE
#define RANDSIM_VARSIMVECSIZE 32
#endif

#ifndef RANDSIM_ALIGNMENT
#define RANDSIM_ALIGNMENT 64
#endif

// Since these data structures are used in the hot loops of the random simulator,
// it is advisable to tune them to your memory controller and cpu capabilities
// (e.g., by aligning and sizing the structures appropriately, you can make the
// compiler use SIMD instruction sets such as AVX2, and get the most out of your
// memory bandwith).


namespace Candy {
    
    /**
     * \class SimulationVector
     *
     * \ingroup RandomSimulation
     *
     * \brief A sequence of assignments for a boolean variable.
     *
     * A SimulationVector represents SimulationVector::VARSIMVECVARS assignments of
     * a single variable, with the assignments stored as a sequence of bits in 
     * the vars[] array.
     */
    class SimulationVector {
    public:
        static const uint8_t VARSIMVECSIZE = RANDSIM_VARSIMVECSIZE;
        typedef std::uint64_t varsimvec_field_t;
        static const unsigned int VARSIMVECVARS = RANDSIM_VARSIMVECSIZE * (sizeof(varsimvec_field_t) * 8);
        
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
        
        /**
         * Initializes the simulation vector with the given bit pattern (copied to each element of var).
         */
        void initialize(varsimvec_field_t pattern);
        
        friend std::ostream& operator<<(const std::ostream& ostr, const SimulationVector& vec);
        
        ~SimulationVector();
    };
    
    /**
     * \class AlignedSimVector
     *
     * \ingroup RandomSimulation
     *
     * \brief A SimulationVector aligned to RANDSIM_ALIGNMENT bytes.
     *
     */
    typedef SimulationVector AlignedSimVector ATTR_ALIGNED(RANDSIM_ALIGNMENT);
    
    
    /**
     * \class SimulationVectors
     *
     * \ingroup RandomSimulation
     *
     * \brief A collection of aligned SimulationVector objects.
     *
     */
    class SimulationVectors {
    public:
        typedef Glucose::Var index_t;
        
        SimulationVectors();
        ~SimulationVectors();
        
        /**
         * Initializes the simulation vector collection for the given amount of variables.
         *
         * \param size  The amount of variables of which assignments need to be represented.
         */
        void initialize(unsigned int size);
        
        /**
         * Retrieves a simulation vector for a given variable.
         *
         * \param index The variable's index.
         */
        inline SimulationVector& get(unsigned int index) const {
            assert(m_isInitialized);
            assert(index < m_size);
            return m_simulationVectors[index];
        }
        
        /**
         * Retrieves a simulation vector for a given variable (const implementation).
         *
         * \param index The variable's index.
         */
        inline const SimulationVector& getConst(unsigned int index) const {
            return get(index);
        }
        
        
        SimulationVectors(const SimulationVectors& other) = delete;
        SimulationVectors& operator=(SimulationVectors& other) = delete;
        
    private:
        AlignedSimVector *ATTR_RESTRICT m_simulationVectors;
        unsigned int m_size;
        char* m_rawMemory;
        bool m_isInitialized;
    };
}

#endif
