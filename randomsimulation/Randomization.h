#ifndef _0F5ABD44_AD19_4EAC_8743_CD4D875C9AFE_RANDOMIZATION_H
#define _0F5ABD44_AD19_4EAC_8743_CD4D875C9AFE_RANDOMIZATION_H

#include <vector>

#include "SimulationVector.h"

namespace randsim {
    class SimulationVectors;
    
    class Randomization {
    public:
        virtual void randomize(SimulationVectors &simVectors,
                               std::vector<SimulationVectors::index_t> indices);
        
        Randomization();
        virtual ~Randomization();
        Randomization(const Randomization& other) = delete;
        Randomization& operator=(const Randomization& other) = delete;
    };
}

#endif
