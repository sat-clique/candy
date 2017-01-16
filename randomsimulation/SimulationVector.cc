#include "SimulationVector.h"

#include <cstdint>

namespace randsim {
    SimulationVector::~SimulationVector() {
    }
    
    void SimulationVector::initialize(varsimvec_field_t pattern) {
        for (int i = 0; i < VARSIMVECSIZE; i++) {
            vars[i] = pattern;
        }
    }
    
    const uint8_t SimulationVector::VARSIMVECSIZE;
    const unsigned int SimulationVector::VARSIMVECVARS;
    
    void allocateAligned(char* rawMem, char* alignedMem, size_t alignment, size_t size) {
        size_t minSize = (size * sizeof(SimulationVector) + alignment);
        rawMem = new char[minSize];
        
        std::uintptr_t offset = alignment - (reinterpret_cast<std::uintptr_t>(rawMem) % alignment);
        alignedMem = rawMem + offset;
        
        assert (reinterpret_cast<std::uintptr_t>(alignedMem) - reinterpret_cast<std::uintptr_t>(rawMem) < alignment);
    }
    
    
    
    SimulationVectors::SimulationVectors()
    : m_simulationVectors(nullptr), m_size(0), m_rawMemory(nullptr), m_isInitialized(false)  {
    }
    
    void SimulationVectors::initialize(unsigned int amount) {
        if (m_isInitialized) {
            return;
        }
        
        m_size = amount;
        
        char *rawMem = nullptr, *alignedMem = nullptr;
        allocateAligned(rawMem, alignedMem, RANDSIM_ALIGNMENT, m_size);
        
        assert (rawMem != nullptr);
        m_rawMemory = std::unique_ptr<char>(rawMem);
        m_simulationVectors = reinterpret_cast<AlignedSimVector*>(alignedMem);
        
        AlignedSimVector *toBeInitialized = m_simulationVectors;
        for (size_t i = 0; i < m_size; i++) {
            new(toBeInitialized) SimulationVector;
            toBeInitialized->initialize(0ull);
        }
        
        m_isInitialized = true;
    }
    
    SimulationVectors::~SimulationVectors() {
        
    }
}
