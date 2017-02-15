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

#include "SimulationVector.h"

#include <cstdint>
#include <iostream>
#include <bitset>

namespace Candy {
    SimulationVector::~SimulationVector() {
    }
    
    void SimulationVector::initialize(varsimvec_field_t pattern) {
        for (int i = 0; i < VARSIMVECSIZE; i++) {
            vars[i] = pattern;
        }
    }

    std::ostream& operator<< (std::ostream& out, SimulationVector &vec) {
        std::bitset<8 * sizeof(AlignedSimVector::varsimvec_field_t)> o(vec.vars[0]);
        out << o;
        out.flush();
        return out;
    }
    
    const uint8_t SimulationVector::VARSIMVECSIZE;
    const unsigned int SimulationVector::VARSIMVECVARS;
    
    void allocateAligned(char** rawMem, char** alignedMem, size_t alignment, size_t size) {
        size_t minSize = (size * sizeof(SimulationVector) + alignment);
        *rawMem = new char[minSize]; // todo: need to call delete[] :(
        
        std::uintptr_t offset = alignment - (reinterpret_cast<std::uintptr_t>(*rawMem) % alignment);
        *alignedMem = *rawMem + offset;
        
        assert ((reinterpret_cast<std::uintptr_t>(*alignedMem) - reinterpret_cast<std::uintptr_t>(*rawMem)) <= alignment);
    }
    
    
    
    SimulationVectors::SimulationVectors()
    : m_simulationVectors(nullptr), m_size(0), m_rawMemory(nullptr), m_isInitialized(false)  {
    }
    
    void SimulationVectors::initialize(unsigned int amount) {
        if (m_isInitialized) {
            return;
        }
        
        m_size = amount;
        
        char *alignedMem = nullptr;
        allocateAligned(&m_rawMemory, &alignedMem, RANDSIM_ALIGNMENT, m_size);
        
        assert (m_rawMemory != nullptr);
        m_simulationVectors = reinterpret_cast<AlignedSimVector*>(alignedMem);
        
        AlignedSimVector *toBeInitialized = m_simulationVectors;
        for (size_t i = 0; i < m_size; i++) {
            new(toBeInitialized) SimulationVector;
            toBeInitialized->initialize(0ull);
        }
        
        m_isInitialized = true;
    }
    
    SimulationVectors::~SimulationVectors() {
        if (m_rawMemory) {
            delete[] m_rawMemory;
        }
    }
}
